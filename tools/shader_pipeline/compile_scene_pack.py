#!/usr/bin/env python3
"""Convert the archived GLSL scene pack into bgfx OpenGL shader binaries."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import subprocess
import tempfile
from pathlib import Path


HEADER = struct.Struct("<4sI")
ENTRY = struct.Struct("<64s16sII8s")

# The archived GLSL pack predates the current clip-depth shadow path and does
# not contain the terrain shadow vertex shaders at all.  Keep these programs
# in portable bgfx shader source so every host can build the complete pack
# without executing the historical x86_64-only ShaderCompiler binary.
GLSL_SOURCE_OVERRIDES = {
    "DefaultShadowStaticVS": "vs_default_shadow_static.sc",
    "DefaultShadowSkinnedVS": "vs_default_shadow_skinned.sc",
    "DefaultShadowFS": "fs_default_shadow.sc",
    "MegaClusterShadowVS": "vs_mega_cluster_shadow.sc",
    "SmoothClusterShadowVS": "vs_smooth_cluster_shadow.sc",
}


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


def read_sampler_stages(
    source_root: Path, description: dict[str, object]
) -> dict[str, int]:
    import re

    source = expand_hlsl(Path(str(description["source"])), source_root)
    declarations = re.findall(
        r"\b(?:TEX_DECLARE(?:2D|3D|CUBE)|LGRID_SAMPLER)"
        r"\(\s*([A-Za-z_]\w*)\s*,\s*(\d+)\s*\)",
        source,
    )
    result: dict[str, int] = {}
    for name, stage_text in declarations:
        stage = int(stage_text)
        previous = result.setdefault(name, stage)
        if previous != stage:
            raise ValueError(
                f"sampler {name} has conflicting stages in {description['source']}"
            )
    return result


def translate_essl(
    name: str, source: bytes, stage: str, sampler_stages: dict[str, int]
) -> bytes:
    import re

    text = source.decode("utf-8")
    text = re.sub(
        r"^#version\s+\d+\s*$",
        "precision highp float;\nprecision highp int;\n"
        "precision highp sampler2D;\nprecision highp samplerCube;\n"
        "precision highp sampler3D;\nprecision highp sampler2DShadow;\n"
        "precision highp sampler2DArray;\nprecision highp sampler2DArrayShadow;\n"
        "precision highp isampler2D;\nprecision highp isampler3D;\n"
        "precision highp isampler2DArray;\nprecision highp usampler2D;\n"
        "precision highp usampler3D;\nprecision highp usampler2DArray;",
        text,
        count=1,
        flags=re.MULTILINE,
    )
    text = re.sub(r"\btexture2D\s*\(", "texture(", text)
    text = re.sub(r"\btextureCube\s*\(", "texture(", text)
    text = re.sub(r"\bvoid\s+main\s*\(\s*\)", "void main()", text, count=1)
    for sampler_name, sampler_stage in sampler_stages.items():
        text = re.sub(
            rf"\b{re.escape(sampler_name)}\b",
            f"s{sampler_stage}_{sampler_name}",
            text,
        )
    if stage == "vertex":
        attributes = {
            "vertex": "a_position",
            "normal": "a_normal",
            "colour": "a_color0",
            "secondary_colour": "a_color1",
            "uv0": "a_texcoord0",
            "uv1": "a_texcoord1",
            "uv2": "a_texcoord2",
            "uv3": "a_texcoord3",
        }
        for source_name, target_name in attributes.items():
            text = re.sub(rf"\b{source_name}\b", target_name, text)
    if stage == "fragment":
        targets = [int(value) for value in re.findall(r"gl_FragData\[(\d+)\]", text)]
        if targets:
            size = max(targets) + 1
            text = text.replace(
                "precision highp int;",
                f"precision highp int;\nout vec4 bgfx_FragData[{size}];",
                1,
            )
            text = text.replace("gl_FragData", "bgfx_FragData")
    return text.encode("utf-8")


def patch_essl_fragment_outputs(bytecode: bytes) -> bytes:
    cursor = 12
    uniform_count = struct.unpack_from("<H", bytecode, cursor)[0]
    cursor += 2
    for _ in range(uniform_count):
        name_size = bytecode[cursor]
        cursor += 1 + name_size + 10
    shader_size = struct.unpack_from("<I", bytecode, cursor)[0]
    shader_start = cursor + 4
    shader_end = shader_start + shader_size
    shader = bytecode[shader_start:shader_end].replace(
        b"_glFragData", b"bgfx_FragData"
    )
    return (
        bytecode[:cursor]
        + struct.pack("<I", len(shader))
        + shader
        + bytecode[shader_end:]
    )


def compile_shader(
    shaderc: Path,
    name: str,
    stage: str,
    source: bytes,
    directory: Path,
    platform: str,
    profile: str,
    preprocessor: Path | None,
    raw: bool,
    varying_def: Path | None,
    include_paths: list[Path],
    sampler_stages: dict[str, int],
) -> bytes:
    compiler_raw = raw
    translated_raw = raw and profile.endswith("_es")
    if translated_raw:
        source = translate_essl(name, source, stage, sampler_stages)
        compiler_raw = False
    source_path = directory / f"{name}{'.glsl' if compiler_raw else '.sc'}"
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
    command = [
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
    ]
    if compiler_raw:
        command.append("--raw")
    else:
        if varying_def is None:
            raise ValueError(
                f"portable GLSL shader {name} requires a varying definition"
            )
        command.extend(("--varyingdef", str(varying_def)))
        for include_path in include_paths:
            command.extend(("-i", str(include_path)))
    process = subprocess.run(command, capture_output=True, text=True)
    if process.returncode:
        details = process.stderr.strip() or process.stdout.strip()
        raise RuntimeError(f"shaderc failed for {name}:\n{details}")
    result = output_path.read_bytes()
    if translated_raw and stage == "fragment":
        result = patch_essl_fragment_outputs(result)
    return result


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
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--varying-def", type=Path)
    parser.add_argument("--bgfx-include", action="append", default=[], type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--platform", default="osx")
    parser.add_argument("--profile", default="140")
    parser.add_argument("--preprocessor", type=Path)
    arguments = parser.parse_args()

    descriptions = json.loads(arguments.shader_db.read_text(encoding="utf-8"))
    descriptions_by_name = {
        str(description["name"]): description for description in descriptions
    }
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
                    True,
                )
                for description in descriptions
            ]
        else:
            source_entries = dict(read_pack(arguments.source_pack))
            complete_entries = []
            for description in descriptions:
                name = description["name"]
                override = GLSL_SOURCE_OVERRIDES.get(name)
                if override is not None:
                    source_path = arguments.source_root / override
                    if not source_path.is_file():
                        raise ValueError(
                            f"portable GLSL override for {name} is absent: "
                            f"{source_path}"
                        )
                    complete_entries.append((name, source_path.read_bytes(), False))
                elif name in source_entries:
                    complete_entries.append((name, source_entries[name], True))
                else:
                    raise ValueError(
                        f"archived GLSL pack has no {name} entry and no portable "
                        "source override is registered"
                    )
        for name, source, raw in complete_entries:
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
                        raw,
                        arguments.varying_def,
                        arguments.bgfx_include,
                        read_sampler_stages(
                            arguments.source_root, descriptions_by_name[name]
                        ),
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
