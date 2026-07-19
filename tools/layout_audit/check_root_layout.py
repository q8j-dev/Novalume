#!/usr/bin/env python3

from __future__ import annotations

import json
import pathlib
import sys


def check(root: pathlib.Path, manifest_path: pathlib.Path) -> list[str]:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    entries = manifest["root_entries"]
    names = [entry["name"] for entry in entries]
    errors: list[str] = []
    duplicates = sorted({name for name in names if names.count(name) > 1})
    if duplicates:
        errors.append("duplicate root classifications: " + ", ".join(duplicates))

    actual = {
        path.name
        for path in root.iterdir()
        if path.is_dir() and path.name not in {".git", "out"}
    }
    declared = set(names)
    if actual - declared:
        errors.append("unclassified top-level directories: " + ", ".join(sorted(actual - declared)))
    if declared - actual:
        errors.append("classified directories missing from root: " + ", ".join(sorted(declared - actual)))

    for entry in entries:
        if not entry.get("role") or not entry.get("destination") or not entry.get("status"):
            errors.append("incomplete classification: " + entry.get("name", "<unnamed>"))
    for entry in manifest.get("migrated_entries", []):
        if (root / entry["name"]).exists():
            errors.append("migrated directory returned to root: " + entry["name"])
        if not (root / entry["destination"]).exists():
            errors.append("migrated destination is missing: " + entry["destination"])
    return errors


def main() -> int:
    root = pathlib.Path(__file__).resolve().parents[2]
    manifest = pathlib.Path(__file__).with_name("root-layout.json")
    errors = check(root, manifest)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"root layout classified: {len(json.loads(manifest.read_text())['root_entries'])} directories")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
