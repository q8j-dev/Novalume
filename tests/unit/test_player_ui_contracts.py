#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
import pathlib
import struct
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
MODULE_PATH = ROOT / "tools/fixture_inspector/player_ui_contracts.py"
SPEC = importlib.util.spec_from_file_location("player_ui_contracts", MODULE_PATH)
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class PlayerUiContractsTests(unittest.TestCase):
    def test_offsets_hash_and_source_coverage_are_deterministic(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            binary = root / "Player.exe"
            binary.write_bytes(b"\0noise\0ScreenInsets\0.?EnumPropDescriptor@VScreenGui\0")
            source = root / "source"
            source.mkdir()
            (source / "ScreenGui.cpp").write_text("const char* value = \"ScreenInsets\";\n", encoding="utf-8")
            spec = root / "spec.json"
            spec.write_text(
                json.dumps(
                    {
                        "scope": "in-experience-player-ui",
                        "contracts": [
                            {
                                "id": "screen",
                                "surface": "ScreenGui",
                                "binary_strings": ["ScreenInsets"],
                                "rtti_strings": ["EnumPropDescriptor@VScreenGui"],
                                "source_strings": ["ScreenInsets", "NotImplemented"],
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )
            report = MODULE.analyze(binary, source, spec)
            contract = report["contracts"][0]
            self.assertTrue(contract["binary_evidence_complete"])
            self.assertEqual(contract["binary_strings"]["ScreenInsets"][0]["offset_hex"], "0x7")
            self.assertEqual(contract["source_identifiers_present"], 1)
            serialized = json.dumps(report)
            self.assertNotIn(str(root), serialized)

    def test_pe_virtual_address_and_direct_xref_are_reported(self) -> None:
        data = bytearray(0x600)
        data[:2] = b"MZ"
        struct.pack_into("<I", data, 0x3C, 0x80)
        data[0x80:0x84] = b"PE\0\0"
        struct.pack_into("<H", data, 0x86, 2)
        struct.pack_into("<H", data, 0x94, 0xF0)
        struct.pack_into("<H", data, 0x98, 0x20B)
        struct.pack_into("<Q", data, 0x98 + 24, 0x140000000)
        section_table = 0x80 + 24 + 0xF0
        data[section_table : section_table + 8] = b".text\0\0\0"
        struct.pack_into("<IIII", data, section_table + 8, 0x100, 0x1000, 0x100, 0x200)
        rdata_section = section_table + 40
        data[rdata_section : rdata_section + 8] = b".rdata\0\0"
        struct.pack_into("<IIII", data, rdata_section + 8, 0x100, 0x2000, 0x100, 0x400)
        string_offset = 0x420
        data[string_offset : string_offset + 14] = b"AutomaticSize\0"
        instruction_offset = 0x220
        instruction_va = 0x140001020
        target_va = 0x140002020
        struct.pack_into("<Q", data, 0x440, target_va)
        displacement = target_va - (instruction_va + 7)
        data[instruction_offset : instruction_offset + 7] = b"\x48\x8d\x0d" + struct.pack(
            "<i", displacement
        )

        pe = MODULE.parse_pe(bytes(data))
        self.assertIsNotNone(pe)
        references = MODULE.rip_relative_references(bytes(data), pe)
        hits = MODULE.binary_hits(bytes(data), "AutomaticSize", pe, references)
        self.assertEqual(hits[0]["virtual_address_hex"], "0x140002020")
        self.assertEqual(
            hits[0]["direct_code_references"][0]["instruction_va_hex"], "0x140001020"
        )
        self.assertEqual(
            hits[0]["absolute_pointer_references"][0]["virtual_address_hex"], "0x140002040"
        )


if __name__ == "__main__":
    unittest.main()
