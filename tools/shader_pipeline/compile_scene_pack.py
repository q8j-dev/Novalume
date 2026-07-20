#!/usr/bin/env python3
"""Convert the archived GLSL scene pack into bgfx OpenGL shader binaries."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path


HEADER = struct.Struct("<4sI")
ENTRY = struct.Struct("<64s16sII8s")


def expand_hlsl(path: Path, source_root: Path, stack: tuple[Path, ...] = ()) -> str:
    import re

    resolved_root = source_root.resolve()
    resolved = (source_root / path).resolve()
    if resolved_root not in resolved.parents:
        raise ValueError(f"HLSL include escapes the source root: {path}")
    relative = resolved.relative_to(resolved_root)
    if relative in stack:
        chain = " -> ".join(str(item) for item in (*stack, relative))
        raise ValueError(f"cyclic HLSL include: {chain}")
    text = resolved.read_text(encoding="utf-8")
    include = re.compile(r'^\s*#include\s+"([^"]+)"\s*$', re.MULTILINE)

    def replace(match: re.Match[str]) -> str:
        nested = relative.parent / match.group(1)
        return expand_hlsl(nested, source_root, (*stack, relative))

    return include.sub(replace, text)


def hlsl_for_backend(
    source_root: Path, description: dict[str, object], profile: str
) -> bytes:
    import re

    source = expand_hlsl(Path(str(description["source"])), source_root)
    entrypoint = str(description["entrypoint"])
    source, replacements = re.subn(
        rf"\b{re.escape(entrypoint)}\b", "main", source
    )
    if replacements == 0:
        raise ValueError(
            f"HLSL entry point {entrypoint} is absent from {description['source']}"
        )
    if profile == "wgsl":
        # The archived shader-model-2 corpus spells the clip-space output
        # semantic POSITION. WebGPU validation requires the D3D10+ spelling so
        # SPIRV-Cross can emit WGSL's @builtin(position).
        source = re.sub(
            r"\bHPosition(\s*:\s*)POSITION\b",
            r"HPosition\1SV_Position",
            source,
        )
        entry = re.search(r"\b(\w+)\s+main\s*\(", source)
        if entry:
            result_type = entry.group(1)
            structure = re.compile(
                rf"(struct\s+{re.escape(result_type)}\s*\{{)(.*?)(\}}\s*;)",
                re.DOTALL,
            )

            def patch_result(match: re.Match[str]) -> str:
                body, count = re.subn(
                    r"(:\s*)POSITION\b", r"\1SV_Position", match.group(2), count=1
                )
                if count == 0:
                    return match.group(0)
                return match.group(1) + body + match.group(3)

            source = structure.sub(patch_result, source, count=1)
        source = re.sub(
            r"(\bmain\s*\([^)]*\)\s*:\s*)POSITION\b",
            r"\1SV_Position",
            source,
        )
    # bgfx's SPIR-V and D3D11 compilers consume HLSL. Select the source
    # corpus' modern texture/cbuffer branch instead of its shader-model-2
    # combined-sampler branch, which cannot be legalized to Vulkan bindings.
    definitions: list[str] = ["#define DX11 1"]
    for definition in str(description.get("defines", "")).split():
        name, separator, value = definition.partition("=")
        definitions.append(f"#define {name} {value if separator else '1'}")
    return ("\n".join(definitions) + "\n" + source).encode("utf-8")


def read_pack(path: Path) -> list[tuple[str, bytes]]:
    data = path.read_bytes()
    magic, count = HEADER.unpack_from(data)
    if magic != b"RBXS":
        raise ValueError(f"{path} is not an RBXS shader pack")
    table_end = HEADER.size + count * ENTRY.size
    if table_end > len(data):
        raise ValueError(f"{path} has a truncated entry table")

    result: list[tuple[str, bytes]] = []
    for index in range(count):
        name_data, _, offset, size, _ = ENTRY.unpack_from(
            data, HEADER.size + index * ENTRY.size
        )
        end = offset + size
        if offset < table_end or end > len(data):
            raise ValueError(f"{path} has an invalid shader payload range")
        name = name_data.split(b"\0", 1)[0].decode("ascii")
        result.append((name, data[offset:end]))
    return result


def read_stages(path: Path) -> dict[str, str]:
    descriptions = json.loads(path.read_text(encoding="utf-8"))
    stages: dict[str, str] = {}
    for description in descriptions:
        target = description.get("target", "")
        if target.startswith("vs_"):
            stages[description["name"]] = "vertex"
        elif target.startswith("ps_"):
            stages[description["name"]] = "fragment"
    return stages


def compile_shader(
    shaderc: Path,
    name: str,
    stage: str,
    source: bytes,
    directory: Path,
    platform: str,
    profile: str,
    preprocessor: Path | None,
) -> bytes:
    source_path = directory / f"{name}.glsl"
    output_path = directory / f"{name}.bin"
    source_path.write_bytes(source)
    hlsl_backend = (
        profile.startswith("spirv") or profile.startswith("s_") or profile == "wgsl"
    )
    if hlsl_backend:
        if preprocessor is None:
            raise ValueError("SPIR-V scene compilation requires a C preprocessor")
        preprocessed_path = directory / f"{name}.preprocessed.hlsl"
        if preprocessor.name.lower() in {"cl", "cl.exe"}:
            command = [str(preprocessor), "/nologo", "/EP", "/TC", str(source_path)]
        else:
            command = [str(preprocessor), "-E", "-P", "-x", "c", str(source_path)]
        process = subprocess.run(command, capture_output=True)
        if process.returncode:
            details = process.stderr.decode("utf-8", errors="replace").strip()
            raise RuntimeError(f"preprocessing failed for {name}:\n{details}")
        preprocessed_path.write_bytes(process.stdout)
        source_path = preprocessed_path
    process = subprocess.run(
        [
            str(shaderc),
            "-f",
            str(source_path),
            "-o",
            str(output_path),
            "--type",
            stage,
            "--platform",
            platform,
            "--profile",
            profile,
            "--raw",
        ],
        capture_output=True,
        text=True,
    )
    if process.returncode:
        details = process.stderr.strip() or process.stdout.strip()
        raise RuntimeError(f"shaderc failed for {name}:\n{details}")
    return output_path.read_bytes()


def translate_missing_shader(
    translator: Path, source_root: Path, description: dict[str, str], directory: Path
) -> bytes:
    local_translator = directory / "ShaderCompiler"
    if not local_translator.exists():
        shutil.copyfile(translator, local_translator)
        local_translator.chmod(0o700)
    command = [
        "/usr/bin/arch",
        "-x86_64",
        str(local_translator),
        description["source"],
        f"/T{description['target']}",
        f"/E{description['entrypoint']}",
        "/DGLSL3",
    ]
    command.extend(f"/D{define}" for define in description.get("defines", "").split())
    process = subprocess.run(command, capture_output=True, cwd=source_root)
    if process.returncode:
        raise RuntimeError(
            f"GLSL translation failed for {description['name']}:\n"
            + process.stderr.decode("utf-8", errors="replace")
        )
    return process.stdout


def write_pack(path: Path, entries: list[tuple[str, bytes]]) -> None:
    table_size = HEADER.size + len(entries) * ENTRY.size
    offset = table_size
    table = bytearray()
    payload = bytearray()
    for name, shader in entries:
        encoded_name = name.encode("ascii")
        if len(encoded_name) >= 64:
            raise ValueError(f"shader name is too long for RBXS: {name}")
        table.extend(
            ENTRY.pack(
                encoded_name.ljust(64, b"\0"),
                hashlib.md5(shader).digest(),
                offset,
                len(shader),
                b"\0" * 8,
            )
        )
        payload.extend(shader)
        offset += len(shader)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(HEADER.pack(b"RBXS", len(entries)) + table + payload)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-pack", required=True, type=Path)
    parser.add_argument("--shader-db", required=True, type=Path)
    parser.add_argument("--shaderc", required=True, type=Path)
    parser.add_argument("--translator", required=True, type=Path)
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--platform", default="osx")
    parser.add_argument("--profile", default="140")
    parser.add_argument("--preprocessor", type=Path)
    arguments = parser.parse_args()

    descriptions = json.loads(arguments.shader_db.read_text(encoding="utf-8"))
    stages = read_stages(arguments.shader_db)
    compiled: list[tuple[str, bytes]] = []
    with tempfile.TemporaryDirectory(prefix="rbx-scene-shaders-") as temporary:
        directory = Path(temporary)
        hlsl_backend = (
            arguments.profile.startswith("spirv")
            or arguments.profile.startswith("s_")
            or arguments.profile == "wgsl"
        )
        if hlsl_backend:
            complete_entries = [
                (
                    description["name"],
                    hlsl_for_backend(
                        arguments.source_root, description, arguments.profile
                    ),
                )
                for description in descriptions
            ]
        else:
            source_entries = read_pack(arguments.source_pack)
            source_names = {name for name, _ in source_entries}
            complete_entries = list(source_entries)
            for description in descriptions:
                if description["name"] not in source_names:
                    complete_entries.append(
                        (
                            description["name"],
                            translate_missing_shader(
                                arguments.translator,
                                arguments.source_root,
                                description,
                                directory,
                            ),
                        )
                    )
        for name, source in complete_entries:
            try:
                stage = stages[name]
            except KeyError as error:
                raise ValueError(f"shader database has no stage for {name}") from error
            compiled.append(
                (
                    name,
                    compile_shader(
                        arguments.shaderc,
                        name,
                        stage,
                        source,
                        directory,
                        arguments.platform,
                        arguments.profile,
                        arguments.preprocessor,
                    ),
                )
            )
    write_pack(arguments.output, compiled)
    print(
        f"compiled {len(compiled)} bgfx {arguments.profile} scene shaders "
        f"into {arguments.output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
