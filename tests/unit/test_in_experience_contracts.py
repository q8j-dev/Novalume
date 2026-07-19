#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import runpy
import struct
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
MODULE = runpy.run_path(
    str(ROOT / "tools/fixture_inspector/in_experience_contracts.py"),
    run_name="in_experience_contracts_test",
)


def varint(value: int) -> bytes:
    encoded = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        encoded.append(byte | (0x80 if value else 0))
        if not value:
            return bytes(encoded)


class InExperienceContractsTests(unittest.TestCase):
    def test_luau_literal_contract_ignores_integrity_wrapper(self) -> None:
        bytecode = bytearray((3,))
        bytecode += varint(1) + varint(16) + b"icons/logo/block"
        bytecode += varint(1)
        bytecode += bytes((2, 0, 0, 0))
        bytecode += varint(1)
        bytecode += bytes(((22 * 227) & 0xFF, 0, 0, 0))
        bytecode += varint(2)
        bytecode += bytes((3,)) + varint(1)
        bytecode += bytes((2,)) + struct.pack("<d", 42.0)
        bytecode += varint(0) + varint(0) + varint(0) + bytes((0, 0))
        bytecode += varint(0)
        source = b"12345678" + bytecode + b"x" * 24

        decoded: list[bytes] = []
        contract = MODULE["luau_contract"](source, decoded)

        self.assertEqual(contract["bytecode_version"], 3)
        self.assertEqual(contract["strings"], ["icons/logo/block"])
        self.assertEqual(contract["numbers"], [42.0])
        self.assertEqual(contract["integrity_trailer_size"], 24)
        self.assertEqual(contract["opcode_encoding"]["decode_multiplier"], 203)
        self.assertIn(bytes((22, 0, 0, 0)), decoded[0])

    def test_referent_delta_decoding(self) -> None:
        # Interleaved transformed values for deltas +2, +3, -1.
        encoded = bytes((0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 6, 1))
        values, offset = MODULE["decode_referents"](encoded, 0, 3)
        self.assertEqual(values, [2, 5, 4])
        self.assertEqual(offset, len(encoded))


if __name__ == "__main__":
    unittest.main()
