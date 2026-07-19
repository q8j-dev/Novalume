#!/usr/bin/env python3

import hashlib
import json
import tempfile
import unittest
from pathlib import Path

import sys

sys.path.insert(0, str(Path(__file__).parents[2] / "tools" / "resource_importer"))
from import_in_game_ui import ImportError, import_overlay  # noqa: E402


class InGameUiImportTests(unittest.TestCase):
    def test_imports_only_scoped_player_content(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            player = root / "player"
            core = player / "content/textures/ui/Chat/Chat.png"
            extra = player / "ExtraContent/textures/ui/InGameChat/Caret.png"
            app = player / "ExtraContent/textures/ui/LuaApp/Home.png"
            icon_font = player / "ExtraContent/LuaPackages/Packages/_Index/BuilderIcons/BuilderIcons/Font/BuilderIcons-Regular.ttf"
            for path, data in ((core, b"core"), (extra, b"extra"), (app, b"app"), (icon_font, b"font")):
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)

            inventory = {
                "scope": {"productSurface": "in-experience-player-core-ui"},
                "entries": [
                    self.entry("content/textures/ui/Chat/Chat.png", b"core", "player-ui"),
                    self.entry(
                        "ExtraContent/textures/ui/InGameChat/Caret.png",
                        b"extra",
                        "in-experience-player-extra-ui",
                    ),
                    self.entry("ExtraContent/textures/ui/LuaApp/Home.png", b"app", "app-ui"),
                    self.entry(
                        "ExtraContent/LuaPackages/Packages/_Index/BuilderIcons/BuilderIcons/Font/BuilderIcons-Regular.ttf",
                        b"font",
                        "in-experience-icon-font",
                    ),
                ],
            }
            inventory_path = root / "inventory.json"
            inventory_path.write_text(json.dumps(inventory))
            output = root / "generated"
            manifest = import_overlay(inventory_path, player, output)
            self.assertEqual(manifest["summary"]["assetCount"], 3)
            self.assertTrue((output / "content/textures/ui/Chat/Chat.png").is_file())
            self.assertTrue((output / "ExtraContent/textures/ui/InGameChat/Caret.png").is_file())
            self.assertTrue((output / "ExtraContent/LuaPackages/Packages/_Index/BuilderIcons/BuilderIcons/Font/BuilderIcons-Regular.ttf").is_file())
            self.assertEqual(manifest["summary"]["builderIconAssetCount"], 1)
            self.assertFalse((output / "ExtraContent/textures/ui/LuaApp/Home.png").exists())
            encoded = (output / "overlay-manifest.json").read_text()
            self.assertNotIn(str(player), encoded)
            self.assertFalse(manifest["policy"]["standaloneAppUiIncluded"])

    def test_rejects_hash_drift_and_unmarked_output(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            player = root / "player"
            source = player / "content/textures/ui/Chat/Chat.png"
            source.parent.mkdir(parents=True)
            source.write_bytes(b"changed")
            inventory = {
                "scope": {"productSurface": "in-experience-player-core-ui"},
                "entries": [self.entry("content/textures/ui/Chat/Chat.png", b"expected", "player-ui")],
            }
            inventory_path = root / "inventory.json"
            inventory_path.write_text(json.dumps(inventory))
            with self.assertRaises(ImportError):
                import_overlay(inventory_path, player, root / "output")

            source.write_bytes(b"expected")
            output = root / "existing"
            output.mkdir()
            (output / "user-file").write_text("preserve")
            with self.assertRaises(ImportError):
                import_overlay(inventory_path, player, output)
            self.assertTrue((output / "user-file").is_file())

    @staticmethod
    def entry(path: str, data: bytes, category: str) -> dict[str, object]:
        return {
            "logicalPath": "rbxasset://" + path.removeprefix("content/"),
            "sourceCorpus": "player",
            "sourcePath": path,
            "sha256": hashlib.sha256(data).hexdigest(),
            "size": len(data),
            "category": category,
            "platformVariant": "shared",
        }


if __name__ == "__main__":
    unittest.main()
