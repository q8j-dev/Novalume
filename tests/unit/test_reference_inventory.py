#!/usr/bin/env python3

import hashlib
import json
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile
import zlib
from pathlib import Path


TOOL = Path(__file__).parents[2] / "tools" / "resource_importer" / "reference_inventory.py"


def png(width: int, height: int) -> bytes:
    signature = b"\x89PNG\r\n\x1a\n"
    raw = b"".join(b"\x00" + b"\xff\x00\x00\xff" * width for _ in range(height))

    def chunk(kind: bytes, data: bytes) -> bytes:
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    return signature + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) + chunk(
        b"IDAT", zlib.compress(raw)
    ) + chunk(b"IEND", b"")


class ReferenceInventoryTests(unittest.TestCase):
    def test_directory_and_zip_inventory_is_deterministic_and_metadata_only(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            player = root / "player"
            studio = root / "studio"
            player.mkdir()
            studio.mkdir()
            (player / "ui").mkdir()
            (studio / "tokens").mkdir()
            image = png(7, 5)
            (player / "ui" / "Button@2x.png").write_bytes(image)
            (studio / "tokens" / "colors.json").write_text('{"surface":"#111111"}\n')

            apk = root / "reference.apk"
            with zipfile.ZipFile(apk, "w") as archive:
                archive.writestr("assets/mobile/Jump@3x.png", png(9, 9))
            apk_hash = hashlib.sha256(apk.read_bytes()).hexdigest()

            spec = {
                "schemaVersion": 1,
                "corpora": {
                    "apk": {"kind": "zip", "description": "fixture", "expectedSha256": apk_hash},
                    "player": {"kind": "directory", "description": "fixture"},
                    "studio": {"kind": "directory", "description": "fixture"},
                },
                "rules": [
                    self.rule("player-ui", "player", ["ui/**/*"], "ui/", "rbxasset://ui/"),
                    self.rule("studio-tokens", "studio", ["tokens/**/*"], "", "reference://studio/"),
                    self.rule("apk-controls", "apk", ["assets/mobile/**/*"], "", "reference://apk/"),
                ],
            }
            spec_path = root / "spec.json"
            spec_path.write_text(json.dumps(spec, sort_keys=True))
            first = root / "first.json"
            second = root / "second.json"
            command = [
                sys.executable,
                str(TOOL),
                "--spec",
                str(spec_path),
                "--corpus",
                f"player={player}",
                "--corpus",
                f"studio={studio}",
                "--corpus",
                f"apk={apk}",
            ]
            subprocess.run(command + ["--output", str(first)], check=True)
            subprocess.run(command + ["--output", str(second)], check=True)
            self.assertEqual(first.read_bytes(), second.read_bytes())

            manifest = json.loads(first.read_text())
            self.assertEqual(manifest["summary"]["entryCount"], 3)
            self.assertFalse(manifest["policy"]["payloadsCopied"])
            self.assertFalse(manifest["policy"]["redistributionApproved"])
            self.assertNotIn(str(root), first.read_text())
            entries = {entry["logicalPath"]: entry for entry in manifest["entries"]}
            player_entry = entries["rbxasset://ui/Button@2x.png"]
            self.assertEqual(player_entry["densityVariant"]["scale"], 2)
            self.assertEqual(player_entry["densityVariant"]["baseLogicalPath"], "rbxasset://ui/Button.png")
            self.assertEqual(player_entry["typeMetadata"]["image"]["width"], 7)
            self.assertEqual(player_entry["typeMetadata"]["image"]["height"], 5)
            self.assertEqual(player_entry["packageDestination"], None)
            self.assertEqual(player_entry["sha256"], hashlib.sha256(image).hexdigest())

    def test_rejects_archive_hash_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for name in ("player", "studio"):
                path = root / name
                path.mkdir()
                (path / "file.txt").write_text(name)
            archive_path = root / "reference.apk"
            with zipfile.ZipFile(archive_path, "w") as archive:
                archive.writestr("file.txt", "apk")
            spec = {
                "schemaVersion": 1,
                "corpora": {
                    "apk": {"kind": "zip", "expectedSha256": "0" * 64},
                    "player": {"kind": "directory"},
                    "studio": {"kind": "directory"},
                },
                "rules": [
                    self.rule("player", "player", ["file.txt"]),
                    self.rule("studio", "studio", ["file.txt"]),
                    self.rule("apk", "apk", ["file.txt"]),
                ],
            }
            spec_path = root / "spec.json"
            spec_path.write_text(json.dumps(spec))
            result = subprocess.run(
                [
                    sys.executable,
                    str(TOOL),
                    "--spec",
                    str(spec_path),
                    "--corpus",
                    f"player={root / 'player'}",
                    "--corpus",
                    f"studio={root / 'studio'}",
                    "--corpus",
                    f"apk={archive_path}",
                    "--output",
                    str(root / "output.json"),
                ],
                text=True,
                capture_output=True,
            )
            self.assertEqual(result.returncode, 2)
            self.assertIn("hash mismatch", result.stderr)

    @staticmethod
    def rule(
        identifier: str,
        corpus: str,
        includes: list[str],
        strip_prefix: str = "",
        logical_prefix: str = "reference://fixture/",
    ) -> dict[str, object]:
        return {
            "id": identifier,
            "corpus": corpus,
            "include": includes,
            "stripPrefix": strip_prefix,
            "logicalPrefix": logical_prefix,
            "category": "fixture",
            "purpose": "test",
            "platformVariant": "shared",
            "licenseDecision": "research-only",
            "compatibilityStatus": "reference-only",
            "conversion": "none",
            "packageDestination": None,
        }


if __name__ == "__main__":
    unittest.main()
