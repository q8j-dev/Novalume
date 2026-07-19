#!/usr/bin/env python3
"""Extract clean-room UI contracts from a supplied InExperience model.

The Player ships its in-experience CoreGui as a binary Roblox model whose
ModuleScript Source properties contain standard Luau bytecode after an
eight-byte integrity prefix.  This tool records paths, hashes, string-table
entries, and literal constants for explicitly selected modules.  It does not
decompile or reproduce implementation code.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import struct
import subprocess
from dataclasses import dataclass


MODEL_HEADER_SIZE = 32


class FormatError(RuntimeError):
    pass


def read_u32(data: bytes, offset: int) -> tuple[int, int]:
    if offset + 4 > len(data):
        raise FormatError("unexpected end of model")
    return struct.unpack_from("<I", data, offset)[0], offset + 4


def read_string(data: bytes, offset: int) -> tuple[str, int]:
    size, offset = read_u32(data, offset)
    end = offset + size
    if end > len(data):
        raise FormatError("string extends past the end of a chunk")
    return data[offset:end].decode("utf-8", errors="replace"), end


def read_varint(data: bytes, offset: int) -> tuple[int, int]:
    value = 0
    shift = 0
    while True:
        if offset >= len(data) or shift >= 35:
            raise FormatError("invalid Luau varint")
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        if byte & 0x80 == 0:
            return value, offset
        shift += 7


def read_varint64(data: bytes, offset: int) -> tuple[int, int]:
    value = 0
    shift = 0
    while True:
        if offset >= len(data) or shift >= 70:
            raise FormatError("invalid Luau 64-bit varint")
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        if byte & 0x80 == 0:
            return value, offset
        shift += 7


def decode_referents(data: bytes, offset: int, count: int) -> tuple[list[int], int]:
    end = offset + count * 4
    if end > len(data):
        raise FormatError("referent array extends past the end of a chunk")
    encoded = data[offset:end]
    result: list[int] = []
    previous = 0
    for index in range(count):
        value = (
            encoded[index] << 24
            | encoded[index + count] << 16
            | encoded[index + count * 2] << 8
            | encoded[index + count * 3]
        )
        delta = (value >> 1) ^ -(value & 1)
        previous += delta
        result.append(previous)
    return result, end


def decompress_chunk(payload: bytes) -> bytes:
    try:
        process = subprocess.run(
            ["zstd", "-q", "-d", "-c"],
            input=payload,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
        )
    except FileNotFoundError as error:
        raise FormatError("zstd is required to inspect compressed Roblox models") from error
    except subprocess.CalledProcessError as error:
        raise FormatError(error.stderr.decode("utf-8", errors="replace")) from error
    return process.stdout


def model_chunks(data: bytes) -> list[tuple[bytes, bytes]]:
    if len(data) < MODEL_HEADER_SIZE or not data.startswith(b"<roblox!"):
        raise FormatError("not a binary Roblox model")
    chunks: list[tuple[bytes, bytes]] = []
    offset = MODEL_HEADER_SIZE
    while offset + 16 <= len(data):
        tag = data[offset : offset + 4]
        compressed, uncompressed, _ = struct.unpack_from("<III", data, offset + 4)
        stored = compressed or uncompressed
        payload = data[offset + 16 : offset + 16 + stored]
        if len(payload) != stored:
            raise FormatError("truncated model chunk")
        chunk = decompress_chunk(payload) if compressed else payload
        if len(chunk) != uncompressed:
            raise FormatError("model chunk has an unexpected decompressed size")
        chunks.append((tag, chunk))
        offset += 16 + stored
        if tag == b"END\0":
            break
    return chunks


@dataclass(frozen=True)
class Module:
    path: str
    source: bytes


def modules_from_model(data: bytes) -> dict[str, Module]:
    chunks = model_chunks(data)
    classes: dict[int, tuple[str, list[int]]] = {}
    names: dict[int, str] = {}
    parents: dict[int, int] = {}
    module_sources: dict[int, bytes] = {}

    for tag, chunk in chunks:
        if tag != b"INST":
            continue
        class_id, offset = read_u32(chunk, 0)
        class_name, offset = read_string(chunk, offset)
        offset += 1  # is-service byte
        count, offset = read_u32(chunk, offset)
        referents, offset = decode_referents(chunk, offset, count)
        if offset != len(chunk):
            raise FormatError("unexpected data at the end of an INST chunk")
        classes[class_id] = (class_name, referents)

    for tag, chunk in chunks:
        if tag != b"PROP":
            continue
        class_id, offset = read_u32(chunk, 0)
        property_name, offset = read_string(chunk, offset)
        if class_id not in classes or offset >= len(chunk):
            raise FormatError("property references an unknown class")
        property_type = chunk[offset]
        offset += 1
        class_name, referents = classes[class_id]
        if property_name == "Name" and property_type == 1:
            for referent in referents:
                names[referent], offset = read_string(chunk, offset)
        elif class_name == "ModuleScript" and property_name == "Source" and property_type == 0x1D:
            for referent in referents:
                size, offset = read_u32(chunk, offset)
                end = offset + size
                if end > len(chunk):
                    raise FormatError("ModuleScript source extends past the property chunk")
                module_sources[referent] = chunk[offset:end]
                offset = end

    for tag, chunk in chunks:
        if tag != b"PRNT":
            continue
        if not chunk or chunk[0] != 0:
            raise FormatError("unsupported parent chunk version")
        count, offset = read_u32(chunk, 1)
        children, offset = decode_referents(chunk, offset, count)
        parent_values, offset = decode_referents(chunk, offset, count)
        if offset != len(chunk):
            raise FormatError("unexpected data at the end of a PRNT chunk")
        parents.update(zip(children, parent_values))

    def instance_path(referent: int) -> str:
        components: list[str] = []
        visited: set[int] = set()
        while referent >= 0:
            if referent in visited:
                raise FormatError("cycle in model parent graph")
            visited.add(referent)
            components.append(names.get(referent, f"<{referent}>"))
            referent = parents.get(referent, -1)
        return "/".join(reversed(components))

    return {
        instance_path(referent): Module(instance_path(referent), source)
        for referent, source in module_sources.items()
    }


def skip_string_reference(data: bytes, offset: int) -> int:
    _, offset = read_varint(data, offset)
    return offset


def luau_contract(
    source: bytes, decoded_output: list[bytes] | None = None
) -> dict[str, object]:
    if len(source) < 10:
        raise FormatError("ModuleScript Source is too short")
    bytecode: bytes | bytearray = bytearray(source[8:]) if decoded_output is not None else source[8:]
    offset = 0
    version = bytecode[offset]
    offset += 1
    if version == 0 or version > 8:
        raise FormatError(f"unsupported Luau bytecode version {version}")
    types_version = bytecode[offset] if version >= 4 else 0
    offset += int(version >= 4)
    string_count, offset = read_varint(bytecode, offset)
    strings: list[str] = []
    for _ in range(string_count):
        size, offset = read_varint(bytecode, offset)
        end = offset + size
        if end > len(bytecode):
            raise FormatError("Luau string extends past the bytecode")
        strings.append(bytecode[offset:end].decode("utf-8", errors="replace"))
        offset = end

    if types_version == 3:
        while True:
            if offset >= len(bytecode):
                raise FormatError("truncated Luau userdata remapping table")
            index = bytecode[offset]
            offset += 1
            if index == 0:
                break
            offset = skip_string_reference(bytecode, offset)

    proto_count, offset = read_varint(bytecode, offset)
    numeric_constants: list[float | int] = []
    string_constant_ids: set[int] = set()
    encoded_opcode_counts: dict[int, int] = {}
    for _ in range(proto_count):
        if offset + 5 > len(bytecode):
            raise FormatError("truncated Luau proto header")
        offset += 4  # stack, params, upvalues, vararg
        if version >= 4:
            offset += 1  # flags
            type_size, offset = read_varint(bytecode, offset)
            offset += type_size
        code_size, offset = read_varint(bytecode, offset)
        code_start = offset
        pc = 0
        auxiliary_opcodes = {
            7, 8, 12, 15, 16, 20, 27, 28, 29, 30, 31, 32,
            53, 55, 58, 60, 66, 74, 75, 77, 78, 79, 80,
        }
        while pc < code_size:
            instruction_offset = code_start + pc * 4
            encoded_opcode = bytecode[instruction_offset]
            encoded_opcode_counts[encoded_opcode] = (
                encoded_opcode_counts.get(encoded_opcode, 0) + 1
            )
            decoded_opcode = (encoded_opcode * 203) & 0xFF
            if decoded_opcode > 82:
                raise FormatError(f"invalid decoded Luau opcode {decoded_opcode}")
            if decoded_output is not None:
                bytecode[instruction_offset] = decoded_opcode
            pc += 2 if decoded_opcode in auxiliary_opcodes else 1
        if pc != code_size:
            raise FormatError("Luau auxiliary instruction extends past a proto")
        offset += code_size * 4
        constant_count, offset = read_varint(bytecode, offset)
        for _ in range(constant_count):
            if offset >= len(bytecode):
                raise FormatError("truncated Luau constant table")
            kind = bytecode[offset]
            offset += 1
            if kind == 0:  # nil
                pass
            elif kind == 1:  # boolean
                offset += 1
            elif kind == 2:  # number
                if offset + 8 > len(bytecode):
                    raise FormatError("truncated Luau number constant")
                numeric_constants.append(struct.unpack_from("<d", bytecode, offset)[0])
                offset += 8
            elif kind == 3:  # string
                string_id, offset = read_varint(bytecode, offset)
                if string_id:
                    string_constant_ids.add(string_id - 1)
            elif kind == 4:  # import
                offset += 4
            elif kind == 5:  # table keys
                key_count, offset = read_varint(bytecode, offset)
                for _ in range(key_count):
                    _, offset = read_varint(bytecode, offset)
            elif kind == 6:  # closure
                _, offset = read_varint(bytecode, offset)
            elif kind == 7:  # vector
                if offset + 16 > len(bytecode):
                    raise FormatError("truncated Luau vector constant")
                numeric_constants.extend(struct.unpack_from("<ffff", bytecode, offset))
                offset += 16
            elif kind == 8 and version >= 7:  # table with constant values
                key_count, offset = read_varint(bytecode, offset)
                for _ in range(key_count):
                    _, offset = read_varint(bytecode, offset)
                    offset += 4
            elif kind == 9 and version >= 8:  # 64-bit integer
                if offset >= len(bytecode):
                    raise FormatError("truncated Luau integer constant")
                negative = bytecode[offset] != 0
                offset += 1
                magnitude, offset = read_varint64(bytecode, offset)
                numeric_constants.append(-magnitude if negative else magnitude)
            else:
                raise FormatError(f"unsupported Luau constant kind {kind}")
        nested_count, offset = read_varint(bytecode, offset)
        for _ in range(nested_count):
            _, offset = read_varint(bytecode, offset)
        _, offset = read_varint(bytecode, offset)  # line defined
        offset = skip_string_reference(bytecode, offset)
        if offset >= len(bytecode):
            raise FormatError("truncated Luau debug information")
        has_lines = bytecode[offset]
        offset += 1
        if has_lines:
            line_gap = bytecode[offset]
            offset += 1
            offset += code_size
            interval_count = ((code_size - 1) >> line_gap) + 1 if code_size else 0
            offset += interval_count * 4
        has_debug = bytecode[offset]
        offset += 1
        if has_debug:
            local_count, offset = read_varint(bytecode, offset)
            for _ in range(local_count):
                offset = skip_string_reference(bytecode, offset)
                _, offset = read_varint(bytecode, offset)
                _, offset = read_varint(bytecode, offset)
                offset += 1
            upvalue_count, offset = read_varint(bytecode, offset)
            for _ in range(upvalue_count):
                offset = skip_string_reference(bytecode, offset)

    _, offset = read_varint(bytecode, offset)  # main proto
    trailer = bytecode[offset:]
    # Signed Player packages append a fixed 24-byte integrity trailer after
    # the standard Luau chunk. It is metadata, not executable bytecode.
    if trailer and len(trailer) != 24:
        raise FormatError(
            f"unexpected data at the end of Luau bytecode ({offset} of {len(bytecode)} bytes)"
        )
    constants = sorted({strings[index] for index in string_constant_ids})
    numbers = sorted({value for value in numeric_constants if value == value})
    if decoded_output is not None:
        decoded_output.append(bytes(bytecode[:offset]))
    return {
        "bytecode_version": version,
        "type_version": types_version,
        "integrity_trailer_size": len(trailer),
        "opcode_encoding": {
            "kind": "multiply-modulo-256",
            "encoded_multiplier": 227,
            "decode_multiplier": 203,
        },
        "encoded_opcode_histogram": {
            str(opcode): count for opcode, count in sorted(encoded_opcode_counts.items())
        },
        "strings": constants,
        "numbers": numbers,
    }


def analyze(
    model_path: pathlib.Path,
    selected_paths: list[str],
    include_inventory: bool = False,
) -> dict[str, object]:
    data = model_path.read_bytes()
    modules = modules_from_model(data)
    selected = []
    for path in selected_paths:
        if path not in modules:
            raise FormatError(f"module not found: {path}")
        module = modules[path]
        selected.append(
            {
                "path": path,
                "source_sha256": hashlib.sha256(module.source).hexdigest(),
                "source_size": len(module.source),
                "contract": luau_contract(module.source),
            }
        )
    report = {
        "schema": 1,
        "scope": "in-experience-player-ui",
        "method": "rbxm-paths-and-luau-literal-contracts",
        "behavior_claim": "none; no proprietary implementation code is reproduced",
        "model": {
            "name": model_path.name,
            "sha256": hashlib.sha256(data).hexdigest(),
            "size": len(data),
            "module_count": len(modules),
        },
        "modules": selected,
    }
    if include_inventory:
        report["inventory"] = [
            {
                "path": path,
                "source_sha256": hashlib.sha256(module.source).hexdigest(),
                "source_size": len(module.source),
            }
            for path, module in sorted(modules.items())
        ]
    return report


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", required=True, type=pathlib.Path)
    parser.add_argument("--module", action="append", required=True)
    parser.add_argument("--include-inventory", action="store_true")
    parser.add_argument("--output", required=True, type=pathlib.Path)
    arguments = parser.parse_args()
    report = analyze(
        arguments.model.resolve(), arguments.module, arguments.include_inventory
    )
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
