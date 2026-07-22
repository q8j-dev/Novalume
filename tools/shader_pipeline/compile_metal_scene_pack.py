#!/usr/bin/env python3
"""Translate the complete scene shader database to bgfx Metal containers."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path


HEADER = struct.Struct("<4sI")
ENTRY = struct.Struct("<64s16sII8s")


def read_pack_names(path: Path) -> list[str]:
    data = path.read_bytes()
    magic, count = HEADER.unpack_from(data)
    if magic != b"RBXS" or HEADER.size + count * ENTRY.size > len(data):
        raise ValueError(f"{path} is not a valid RBXS shader pack")
    names: list[str] = []
    for index in range(count):
        name_data, _, _, _, _ = ENTRY.unpack_from(
            data, HEADER.size + index * ENTRY.size
        )
        names.append(name_data.split(b"\0", 1)[0].decode("ascii"))
    return names


def read_sampler_slots(path: Path) -> dict[str, dict[str, int]]:
    data = path.read_bytes()
    magic, count = HEADER.unpack_from(data)
    if magic != b"RBXS" or HEADER.size + count * ENTRY.size > len(data):
        raise ValueError(f"{path} is not a valid RBXS shader pack")
    result: dict[str, dict[str, int]] = {}
    for index in range(count):
        name_data, _, offset, size, _ = ENTRY.unpack_from(
            data, HEADER.size + index * ENTRY.size
        )
        name = name_data.split(b"\0", 1)[0].decode("ascii")
        source = data[offset : offset + size].decode("utf-8", errors="replace")
        result[name] = {
            sampler: int(slot)
            for sampler, slot in re.findall(r"//\$\$(\w+)=s(\d+)", source)
        }
    return result


def varying_location(name: str) -> int:
    match = re.fullmatch(r"xlv_TEXCOORD(\d+)", name)
    if match:
        return int(match.group(1))
    match = re.fullmatch(r"xlv_COLOR(\d+)", name)
    if match:
        return 16 + int(match.group(1))
    digest = hashlib.sha256(name.encode("ascii")).digest()
    return 20 + int.from_bytes(digest[:2], "little") % 8


def attribute_name(name: str) -> str:
    mappings = {
        "vertex": "a_position",
        "position": "a_position",
        "normal": "a_normal",
        "tangent": "a_tangent",
        "binormal": "a_bitangent",
        "color": "a_color0",
        "colour": "a_color0",
        "color0": "a_color0",
        "color1": "a_color1",
        "secondary_colour": "a_color1",
        "indices": "a_indices",
        "weights": "a_weight",
        "weight": "a_weight",
    }
    lowered = name.lower()
    if lowered in mappings:
        return mappings[lowered]
    match = re.fullmatch(r"(?:texcoord|uv)(\d*)", lowered)
    if match:
        return "a_texcoord" + (match.group(1) or "0")
    return name


def annotate_struct(source: str, struct_name: str, vertex_output: bool) -> str:
    pattern = re.compile(rf"(struct {struct_name} \{{\n)(.*?)(\n\}};)", re.DOTALL)
    match = pattern.search(source)
    if not match:
        return source
    fields = []
    promoted_vertex_outputs: list[tuple[str, str]] = []
    for line in match.group(2).splitlines():
        field = re.fullmatch(r"(\s*\S+\s+)(\w+)(\s*;)", line)
        if not field or (vertex_output and field.group(2) == "gl_Position"):
            fields.append(line)
            continue
        name = field.group(2)
        type_prefix = field.group(1)
        half_type = re.search(r"\bhalf([234]?)\s+$", type_prefix)
        if vertex_output and half_type:
            promoted_type = "float" + half_type.group(1)
            type_prefix = re.sub(
                r"\bhalf([234]?)\s+$", promoted_type + " ", type_prefix
            )
            promoted_vertex_outputs.append((name, promoted_type))
        fields.append(
            f"{type_prefix}{name} [[user(locn{varying_location(name)})]];"
        )
    replacement = match.group(1) + "\n".join(fields) + match.group(3)
    source = source[: match.start()] + replacement + source[match.end() :]
    for name, promoted_type in promoted_vertex_outputs:
        assignment = re.compile(
            rf"(_mtl_o\.{re.escape(name)}\s*=\s*)([^;]+)(;)"
        )
        source = assignment.sub(
            lambda value: (
                value.group(1)
                + promoted_type
                + "("
                + value.group(2)
                + ")"
                + value.group(3)
            ),
            source,
        )
    return source


def patch_attributes(source: str) -> str:
    pattern = re.compile(r"(\s*\S+\s+)(\w+)(\s+\[\[attribute\(\d+\)\]\];)")
    replacements: list[tuple[str, str]] = []

    def replace(match: re.Match[str]) -> str:
        old = match.group(2)
        new = attribute_name(old)
        replacements.append((old, new))
        return match.group(1) + new + match.group(3)

    source = pattern.sub(replace, source)
    for old, new in replacements:
        if old != new:
            source = source.replace(f"_mtl_i.{old}", f"_mtl_i.{new}")
    return source


def patch_uniform_alignment(source: str) -> str:
    pattern = re.compile(r"(struct xlatMtlShaderUniform \{\n)(.*?)(\n\};)", re.DOTALL)
    match = pattern.search(source)
    if not match:
        return source
    fields = []
    narrow: list[tuple[str, str]] = []
    for line in match.group(2).splitlines():
        field = re.fullmatch(r"(\s*)float([123])\s+(\w+);", line)
        if field:
            component = {"1": "x", "2": "xy", "3": "xyz"}[field.group(2)]
            narrow.append((field.group(3), component))
            fields.append(f"{field.group(1)}float4 {field.group(3)};")
        else:
            fields.append(line)
    replacement = match.group(1) + "\n".join(fields) + match.group(3)
    source = source[: match.start()] + replacement + source[match.end() :]
    for name, component in narrow:
        source = re.sub(
            rf"_mtl_u\.{re.escape(name)}(?![.\w\[])",
            f"_mtl_u.{name}.{component}",
            source,
        )
    return source


def patch_fragment_outputs(source: str) -> str:
    outputs = sorted({int(value) for value in re.findall(r"_mtl_o\.gl_FragData_(\d+)", source)})
    if not outputs:
        return source
    source = re.sub(
        r"(_mtl_o\.gl_FragData_\d+\s*=\s*)([^;]+)(;)",
        r"\1float4(\2)\3",
        source,
    )
    fields = "\n".join(
        f"  float4 gl_FragData_{index} [[color({index})]];" for index in outputs
    )
    return source.replace(
        "struct xlatMtlShaderOutput {\n};",
        f"struct xlatMtlShaderOutput {{\n{fields}\n}};",
        1,
    )


def patch_sampler_stages(source: str, sampler_slots: dict[str, int]) -> str:
    for name, stage in sampler_slots.items():
        texture_pattern = re.compile(
            rf"\b{re.escape(name)}\s+\[\[texture\(\d+\)\]\]"
        )
        if not texture_pattern.search(source):
            continue
        tagged_name = f"s{stage}_{name}"
        source = re.sub(rf"\b{re.escape(name)}\b", tagged_name, source)
        source = re.sub(
            rf"(_mtlsmp_{re.escape(name)}\s+\[\[sampler\()\d+(\)\]\])",
            rf"\g<1>{stage}\g<2>",
            source,
        )
        source = re.sub(
            rf"\b{re.escape(tagged_name)}\s+\[\[texture\(\d+\)\]\]",
            f"{tagged_name} [[texture({stage})]]",
            source,
        )
    return source


def patch_metal(source: str, stage: str) -> str:
    source = patch_attributes(source)
    source = patch_uniform_alignment(source)
    source = patch_fragment_outputs(source)
    if stage == "vertex":
        source = re.sub(
            r"\bhalf4\s+gl_Position\s+\[\[position\]\]",
            "float4 gl_Position [[position]]",
            source,
        )
        source = re.sub(
            r"(_mtl_o\.gl_Position\s*=\s*)([^;]+)(;)",
            r"\1float4(\2)\3",
            source,
        )
        source = annotate_struct(source, "xlatMtlShaderOutput", True)
    else:
        source = annotate_struct(source, "xlatMtlShaderInput", False)
    return source


def scene_program_pairs(shader_names: set[str]) -> list[tuple[str, str]]:
    """Return every vertex/fragment pairing constructed by the scene renderer."""
    pairs = {
        ("AdornAALineVS", "AdornAALineFS"),
        ("AdornLightingVS", "AdornFS"),
        ("AdornOutlineVS", "AdornOutlineFS"),
        ("AdornSelfLitHighlightVS", "AdornFS"),
        ("AdornSelfLitVS", "AdornFS"),
        ("AdornVS", "AdornFS"),
        ("DownSample4x4VS", "DownSample4x4GlowFS"),
        ("GBufferResolveVS", "GBufferResolveFS"),
        ("MegaClusterHQVS", "MegaClusterHQFS"),
        ("MegaClusterHQVS", "MegaClusterHQGBufferFS"),
        ("MegaClusterShadowVS", "DefaultShadowFS"),
        ("MegaClusterVS", "MegaClusterFS"),
        ("ParticleCustomVS", "ParticleCustomFS"),
        ("ParticleVS", "ParticleAddFS"),
        ("ParticleVS", "ParticleCrazyFS"),
        ("ParticleVS", "ParticleCrazySparklesFS"),
        ("ParticleVS", "ParticleModulateFS"),
        ("ProfilerVS", "ProfilerFS"),
        ("SSAODepthDownVS", "SSAODepthDownFS"),
        ("SSAOBlurXVS", "SSAOBlurXFS"),
        ("SSAOBlurYVS", "SSAOBlurYFS"),
        ("SSAOCompositVS", "SSAOCompositFS"),
        ("SkyVS", "SkyFS"),
        ("SmoothClusterHQVS", "SmoothClusterHQFS"),
        ("SmoothClusterHQVS", "SmoothClusterHQGBufferFS"),
        ("SmoothClusterShadowVS", "DefaultShadowFS"),
        ("SmoothClusterVS", "SmoothClusterFS"),
        ("SmoothWaterHQVS", "SmoothWaterHQFS"),
        ("SmoothWaterHQVS", "SmoothWaterHQGBufferFS"),
        ("SmoothWaterVS", "SmoothWaterFS"),
        ("TexCompVS", "TexCompFS"),
        ("TexCompVS", "TexCompPMAFS"),
        ("UIFogVS", "UIFogFS"),
        ("UIVS", "UIFS"),
        ("WaterHQVS", "WaterHQFS"),
        ("WaterVS", "WaterFS"),
    }

    for fragment in (
        "Blur3FS",
        "Blur5FS",
        "Blur7FS",
        "GlowApplyFS",
        "ImageProcessFS",
        "PassThroughFS",
        "SSAOFS",
        "ShadowBlurFS",
    ):
        pairs.add(("PassThroughVS", fragment))

    for skinning in ("Static", "Skinned"):
        pairs.add((f"Default{skinning}HQVS", "DefaultHQFS"))
        pairs.add((f"Default{skinning}HQVS", "DefaultHQGBufferFS"))
        pairs.add((f"Default{skinning}VS", "DefaultFS"))
        pairs.add((f"DefaultShadow{skinning}VS", "DefaultShadowFS"))

        for vertex in (
            f"Default{skinning}VS",
            f"Default{skinning}ReflectionVS",
        ):
            for fragment in (
                "LowQMaterialFS",
                "LowQMaterialWangFS",
                "LowQMaterialWangFallbackFS",
            ):
                pairs.add((vertex, fragment))
        pairs.add((f"Default{skinning}VS", "DefaultPlasticFS"))
        pairs.add((f"Default{skinning}VS", "DefaultNeonFS"))
        pairs.add(
            (f"Default{skinning}ReflectionVS", "DefaultPlasticReflectionFS")
        )
        pairs.add(
            (f"Default{skinning}ReflectionVS", "DefaultSmoothPlasticReflectionFS")
        )

        for fragment in shader_names:
            match = re.fullmatch(r"Default(.+)HQ(?:GBuffer)?FS", fragment)
            if not match:
                continue
            material = match.group(1).removesuffix("Reflection")
            vertex = (
                f"Default{skinning}HQVS"
                if material in {"SmoothPlastic", "Neon"}
                else f"Default{skinning}SurfaceHQVS"
            )
            pairs.add((vertex, fragment))

    return sorted(
        pair for pair in pairs
        if pair[0] in shader_names and pair[1] in shader_names
    )


def extract_varying_interface(
    source: str, struct_name: str
) -> dict[int, tuple[str, str]]:
    match = re.search(
        rf"struct {re.escape(struct_name)} \{{\n(.*?)\n\}};", source, re.DOTALL
    )
    if not match:
        return {}
    interface: dict[int, tuple[str, str]] = {}
    field_pattern = re.compile(
        r"\s*(\w+)\s+(\w+)\s+\[\[user\(locn(\d+)\)\]\];"
    )
    for line in match.group(1).splitlines():
        field = field_pattern.fullmatch(line)
        if not field:
            continue
        type_name, varying_name, location_text = field.groups()
        location = int(location_text)
        if location in interface:
            raise ValueError(
                f"Metal {struct_name} contains duplicate varying location {location}"
            )
        interface[location] = (type_name, varying_name)
    return interface


def validate_program_interfaces(translated: dict[str, str]) -> None:
    """Reject stage containers that compile alone but cannot link together."""
    for vertex_name, fragment_name in scene_program_pairs(set(translated)):
        outputs = extract_varying_interface(
            translated[vertex_name], "xlatMtlShaderOutput"
        )
        inputs = extract_varying_interface(
            translated[fragment_name], "xlatMtlShaderInput"
        )
        for location, (input_type, input_name) in inputs.items():
            output = outputs.get(location)
            if output is None:
                raise ValueError(
                    f"Metal program {vertex_name}/{fragment_name} has fragment "
                    f"input {input_name} at locn{location} with no vertex output"
                )
            output_type, output_name = output
            if output_type != input_type:
                raise ValueError(
                    f"Metal program {vertex_name}/{fragment_name} varying locn{location} "
                    f"type mismatch: vertex {output_name} is {output_type}, "
                    f"fragment {input_name} is {input_type}"
                )


def extract_uniforms(source: str) -> list[tuple[str, int, int, int]]:
    uniforms: list[tuple[str, int, int, int]] = []
    struct_match = re.search(
        r"struct xlatMtlShaderUniform \{\n(.*?)\n\};", source, re.DOTALL
    )
    if struct_match:
        field_pattern = re.compile(
            r"\s*(float4x4|float3x3|float4)\s+(\w+)(?:\[(\d+)\])?;"
        )
        for line in struct_match.group(1).splitlines():
            field = field_pattern.fullmatch(line)
            if not field:
                raise ValueError(f"unsupported Metal uniform declaration: {line.strip()}")
            type_name, name, count_text = field.groups()
            count = int(count_text or "1")
            if count > 255:
                raise ValueError(f"Metal uniform array is too large: {name}[{count}]")
            uniform_type = {"float4": 2, "float3x3": 3, "float4x4": 4}[type_name]
            register_count = count * {"float4": 1, "float3x3": 3, "float4x4": 4}[type_name]
            uniforms.append((name, uniform_type, count, register_count))

    texture_pattern = re.compile(
        r"\b(?:texture\w*|depth\w*)\s*<[^>]+>\s+(\w+)\s+\[\[texture\((\d+)\)\]\]"
    )
    textures = sorted(
        ((int(slot), name) for name, slot in texture_pattern.findall(source)),
        key=lambda value: value[0],
    )
    uniforms.extend((name, 0, 1, 0) for _, name in textures)

    names = [uniform[0] for uniform in uniforms]
    if len(names) != len(set(names)):
        raise ValueError("Metal shader contains duplicate uniform names")
    return uniforms


def bgfx_container(stage: str, source: str) -> bytes:
    magic = (b"VSH" if stage == "vertex" else b"FSH") + bytes([11])
    encoded = source.encode("utf-8")
    uniforms = extract_uniforms(source)
    metadata = bytearray(struct.pack("<IIH", 0, 0, len(uniforms)))
    fragment_bit = 0x10 if stage == "fragment" else 0
    for name, uniform_type, count, register_count in uniforms:
        encoded_name = name.encode("ascii")
        if len(encoded_name) > 255:
            raise ValueError(f"Metal uniform name is too long: {name}")
        metadata.extend(struct.pack("<B", len(encoded_name)))
        metadata.extend(encoded_name)
        metadata.extend(
            struct.pack(
                "<BBHHBBH",
                uniform_type | fragment_bit,
                count,
                0,
                register_count,
                0,
                0,
                0,
            )
        )
    return (
        magic
        + metadata
        + struct.pack("<I", len(encoded))
        + encoded
        + b"\0"
        + struct.pack("<BH", 0, 0)
    )


def validate_metal(name: str, source: str, directory: Path) -> None:
    source_path = directory / f"{name}.metal"
    output_path = directory / f"{name}.air"
    source_path.write_text(source, encoding="utf-8")
    process = subprocess.run(
        [
            "/usr/bin/xcrun",
            "-sdk",
            "macosx",
            "metal",
            "-c",
            str(source_path),
            "-o",
            str(output_path),
        ],
        capture_output=True,
        text=True,
    )
    if process.returncode:
        raise RuntimeError(f"Metal validation failed for {name}:\n{process.stderr}")


def write_pack(path: Path, entries: list[tuple[str, bytes]]) -> None:
    offset = HEADER.size + len(entries) * ENTRY.size
    table = bytearray()
    payload = bytearray()
    for name, shader in entries:
        encoded = name.encode("ascii")
        table.extend(
            ENTRY.pack(
                encoded.ljust(64, b"\0"),
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
    parser.add_argument("--translator", required=True, type=Path)
    parser.add_argument("--source-pack", required=True, type=Path)
    parser.add_argument("--shader-db", required=True, type=Path)
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    arguments = parser.parse_args()

    descriptions = {
        item["name"]: item
        for item in json.loads(arguments.shader_db.read_text(encoding="utf-8"))
    }
    names = read_pack_names(arguments.source_pack)
    names.extend(name for name in descriptions if name not in names)
    sampler_slots = read_sampler_slots(arguments.source_pack)
    entries: list[tuple[str, bytes]] = []
    translated: dict[str, str] = {}
    with tempfile.TemporaryDirectory(prefix="rbx-metal-scene-") as temporary:
        translator = Path(temporary) / "ShaderCompiler"
        shutil.copyfile(arguments.translator, translator)
        translator.chmod(0o700)
        for name in names:
            description = descriptions[name]
            target = description["target"]
            stage = "vertex" if target.startswith("vs_") else "fragment"
            command = [
                "/usr/bin/arch",
                "-x86_64",
                str(translator),
                description["source"],
                f"/T{target}",
                f"/E{description['entrypoint']}",
                "/DMETAL",
            ]
            command.extend(f"/D{define}" for define in description.get("defines", "").split())
            process = subprocess.run(
                command, capture_output=True, cwd=arguments.source_root
            )
            if process.returncode:
                raise RuntimeError(
                    f"Metal translation failed for {name}:\n"
                    + process.stderr.decode("utf-8", errors="replace")
                )
            metal = patch_metal(process.stdout.decode("utf-8"), stage)
            metal = patch_sampler_stages(metal, sampler_slots.get(name, {}))
            validate_metal(name, metal, Path(temporary))
            translated[name] = metal
            entries.append((name, bgfx_container(stage, metal)))
        validate_program_interfaces(translated)
    write_pack(arguments.output, entries)
    print(f"translated {len(entries)} Metal scene shaders into {arguments.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
