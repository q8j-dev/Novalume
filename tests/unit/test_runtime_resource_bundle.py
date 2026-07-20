#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import tempfile
import unittest
import zipfile
from pathlib import Path


TOOL = Path(__file__).parents[2] / "tools" / "resource_importer" / "runtime_resource_bundle.py"
SPEC = importlib.util.spec_from_file_location("runtime_resource_bundle", TOOL)
assert SPEC and SPEC.loader
bundle = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(bundle)


class RuntimeResourceBundleTests(unittest.TestCase):
    def make_resources(self, root: Path) -> None:
        for relative in sorted(bundle.REQUIRED_FILES):
            target = root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((relative + "\n").encode("utf-8"))
        extra = root / "overlays" / "player-2026" / "content" / "fonts" / "Example.ttf"
        extra.parent.mkdir(parents=True, exist_ok=True)
        extra.write_bytes(b"font")

    def test_reproducible_create_verify_and_extract(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            resources = root / "Resources"
            self.make_resources(resources)
            first = root / "first.zip"
            second = root / "second.zip"
            bundle.create_bundle(resources, first, "test-runtime")
            bundle.create_bundle(resources, second, "test-runtime")
            self.assertEqual(bundle.sha256_path(first), bundle.sha256_path(second))
            manifest, payloads = bundle.read_verified_bundle(first)
            self.assertEqual(manifest["bundleId"], "test-runtime")
            self.assertTrue(bundle.REQUIRED_FILES.issubset(payloads))
            extracted = root / "extracted"
            bundle.extract_bundle(first, extracted)
            self.assertEqual(
                (extracted / "models/InExperience/InExperience.rbxm").read_bytes(),
                b"models/InExperience/InExperience.rbxm\n",
            )

    def test_rejects_unrecorded_archive_payload(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            resources = root / "Resources"
            self.make_resources(resources)
            archive = root / "runtime.zip"
            bundle.create_bundle(resources, archive, "test-runtime")
            with zipfile.ZipFile(archive, "a") as value:
                value.writestr("overlays/player-2026/unrecorded.bin", b"unexpected")
            with self.assertRaisesRegex(bundle.BundleError, "unrecorded payload"):
                bundle.read_verified_bundle(archive)


if __name__ == "__main__":
    unittest.main()
