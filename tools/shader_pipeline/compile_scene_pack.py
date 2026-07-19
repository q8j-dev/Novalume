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
    shaderc: Path, name: str, stage: str, source: bytes, directory: Path
) -> bytes:
    source_path = directory / f"{name}.glsl"
    output_path = directory / f"{name}.bin"
    source_path.write_bytes(source)
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
            "osx",
            "--profile",
            "140",
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
    arguments = parser.parse_args()

    source_entries = read_pack(arguments.source_pack)
    descriptions = json.loads(arguments.shader_db.read_text(encoding="utf-8"))
    stages = read_stages(arguments.shader_db)
    compiled: list[tuple[str, bytes]] = []
    with tempfile.TemporaryDirectory(prefix="rbx-scene-shaders-") as temporary:
        directory = Path(temporary)
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
                (name, compile_shader(arguments.shaderc, name, stage, source, directory))
            )
    write_pack(arguments.output, compiled)
    print(f"compiled {len(compiled)} bgfx OpenGL scene shaders into {arguments.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
