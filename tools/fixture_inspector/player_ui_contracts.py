#!/usr/bin/env python3
"""Produce deterministic clean-room Player UI evidence from a supplied PE.

The report records printable strings, PE virtual addresses, and direct x86-64
RIP-relative references into those strings. It does not execute or patch the
binary, and it never reproduces proprietary implementation code. Source
matches remain identifier coverage, never a behavior claim.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import struct
from collections.abc import Iterable


SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inl"}
IGNORED_PARTS = {".git", "out", "third_party", "Library"}


def parse_pe(data: bytes) -> dict[str, object] | None:
    """Return the minimal PE mapping needed for deterministic static xrefs."""
    if len(data) < 0x40 or data[:2] != b"MZ":
        return None
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if pe_offset + 24 > len(data) or data[pe_offset : pe_offset + 4] != b"PE\0\0":
        return None
    section_count = struct.unpack_from("<H", data, pe_offset + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe_offset + 20)[0]
    optional_offset = pe_offset + 24
    if optional_offset + optional_size > len(data):
        return None
    magic = struct.unpack_from("<H", data, optional_offset)[0]
    if magic != 0x20B:  # PE32+ only; the supplied Player is x86-64.
        return None
    image_base = struct.unpack_from("<Q", data, optional_offset + 24)[0]
    sections = []
    section_offset = optional_offset + optional_size
    for index in range(section_count):
        offset = section_offset + index * 40
        if offset + 40 > len(data):
            return None
        name = data[offset : offset + 8].split(b"\0", 1)[0].decode("ascii", errors="replace")
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
            "<IIII", data, offset + 8
        )
        sections.append(
            {
                "name": name,
                "virtual_size": virtual_size,
                "virtual_address": virtual_address,
                "raw_size": raw_size,
                "raw_offset": raw_offset,
            }
        )
    return {"image_base": image_base, "sections": sections}


def file_offset_to_va(pe: dict[str, object], offset: int) -> tuple[int, str] | None:
    image_base = int(pe["image_base"])
    for section in pe["sections"]:  # type: ignore[union-attr]
        raw_offset = int(section["raw_offset"])
        raw_size = int(section["raw_size"])
        if raw_offset <= offset < raw_offset + raw_size:
            return (
                image_base + int(section["virtual_address"]) + offset - raw_offset,
                str(section["name"]),
            )
    return None


def rip_relative_references(data: bytes, pe: dict[str, object]) -> dict[int, list[dict[str, object]]]:
    """Find common direct RIP-relative LEA/MOV references in executable bytes."""
    references: dict[int, list[dict[str, object]]] = {}
    image_base = int(pe["image_base"])
    for section in pe["sections"]:  # type: ignore[union-attr]
        if section["name"] != ".text":
            continue
        raw_offset = int(section["raw_offset"])
        raw_size = min(int(section["raw_size"]), len(data) - raw_offset)
        virtual_address = image_base + int(section["virtual_address"])
        code = memoryview(data)[raw_offset : raw_offset + raw_size]
        for index in range(max(0, raw_size - 7)):
            rex = 0x40 <= code[index] <= 0x4F
            opcode_index = index + 1 if rex else index
            if opcode_index + 5 >= raw_size or code[opcode_index] not in (0x8D, 0x8B, 0x89):
                continue
            modrm = code[opcode_index + 1]
            if modrm & 0xC7 != 0x05:  # mod=00, r/m=101 means RIP + disp32.
                continue
            length = (1 if rex else 0) + 6
            displacement = struct.unpack_from("<i", code, opcode_index + 2)[0]
            instruction_va = virtual_address + index
            target_va = instruction_va + length + displacement
            references.setdefault(target_va, []).append(
                {
                    "instruction_va": instruction_va,
                    "instruction_va_hex": f"0x{instruction_va:x}",
                    "file_offset": raw_offset + index,
                    "file_offset_hex": f"0x{raw_offset + index:x}",
                    "kind": "lea" if code[opcode_index] == 0x8D else "mov",
                }
            )
    return references


def absolute_pointer_references(
    data: bytes,
    pe: dict[str, object],
    target_va: int,
    code_references: dict[int, list[dict[str, object]]],
) -> list[dict[str, object]]:
    """Locate PE64 data pointers to a mapped string virtual address."""
    encoded = struct.pack("<Q", target_va)
    hits: list[dict[str, object]] = []
    start = 0
    while True:
        offset = data.find(encoded, start)
        if offset < 0:
            break
        mapped = file_offset_to_va(pe, offset)
        hit: dict[str, object] = {
            "file_offset": offset,
            "file_offset_hex": f"0x{offset:x}",
        }
        if mapped:
            pointer_va, section = mapped
            hit.update(
                {
                    "virtual_address": pointer_va,
                    "virtual_address_hex": f"0x{pointer_va:x}",
                    "section": section,
                    "direct_code_references": code_references.get(pointer_va, []),
                }
            )
        hits.append(hit)
        start = offset + 1
    return hits


def binary_hits(
    data: bytes,
    needle: str,
    pe: dict[str, object] | None = None,
    references: dict[int, list[dict[str, object]]] | None = None,
) -> list[dict[str, object]]:
    encoded = needle.encode("ascii")
    hits: list[dict[str, object]] = []
    start = 0
    while True:
        index = data.find(encoded, start)
        if index < 0:
            break
        hit: dict[str, object] = {"offset": index, "offset_hex": f"0x{index:x}"}
        if pe:
            mapped = file_offset_to_va(pe, index)
            if mapped:
                va, section = mapped
                hit.update(
                    {
                        "virtual_address": va,
                        "virtual_address_hex": f"0x{va:x}",
                        "section": section,
                        "direct_code_references": (references or {}).get(va, []),
                        "absolute_pointer_references": absolute_pointer_references(
                            data, pe, va, references or {}
                        ),
                    }
                )
        hits.append(hit)
        start = index + 1
    return hits


def source_files(root: pathlib.Path) -> Iterable[pathlib.Path]:
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES:
            relative = path.relative_to(root)
            if not any(part in IGNORED_PARTS for part in relative.parts):
                yield path


def source_hits(files: list[pathlib.Path], root: pathlib.Path, needle: str) -> list[dict[str, object]]:
    pattern = re.compile(re.escape(needle))
    hits: list[dict[str, object]] = []
    for path in files:
        try:
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        except OSError:
            continue
        for number, line in enumerate(lines, 1):
            if pattern.search(line):
                hits.append({"path": path.relative_to(root).as_posix(), "line": number})
    return hits


def analyze(binary: pathlib.Path, source_root: pathlib.Path, spec: pathlib.Path) -> dict[str, object]:
    data = binary.read_bytes()
    pe = parse_pe(data)
    references = rip_relative_references(data, pe) if pe else {}
    definition = json.loads(spec.read_text(encoding="utf-8"))
    files = list(source_files(source_root))
    contracts = []
    for contract in definition["contracts"]:
        strings = {
            needle: binary_hits(data, needle, pe, references)
            for needle in contract.get("binary_strings", [])
        }
        rtti = {
            needle: binary_hits(data, needle, pe, references)
            for needle in contract.get("rtti_strings", [])
        }
        source = {
            needle: source_hits(files, source_root, needle)
            for needle in contract.get("source_strings", [])
        }
        contracts.append(
            {
                "id": contract["id"],
                "surface": contract["surface"],
                "binary_evidence_complete": all(strings.values()) and all(rtti.values()),
                "binary_strings": strings,
                "rtti_strings": rtti,
                "source_identifier_coverage": source,
                "source_identifiers_present": sum(bool(value) for value in source.values()),
                "source_identifiers_total": len(source),
            }
        )
    return {
        "schema": 2,
        "scope": definition["scope"],
        "method": "static-printable-string-offsets-and-direct-rip-relative-xrefs",
        "behavior_claim": "none; source matches are identifier coverage only",
        "binary": {
            "name": binary.name,
            "sha256": hashlib.sha256(data).hexdigest(),
            "size": len(data),
            "format": "PE32+" if pe else "unknown-or-test-fixture",
        },
        "contracts": contracts,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, type=pathlib.Path)
    parser.add_argument("--source-root", required=True, type=pathlib.Path)
    parser.add_argument("--spec", required=True, type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()
    report = analyze(args.binary.resolve(), args.source_root.resolve(), args.spec.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
