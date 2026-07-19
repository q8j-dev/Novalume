#!/usr/bin/env python3
"""Import only the scoped in-experience Player UI into a generated overlay.

Every payload is verified against the clean-room inventory before it is copied.
The output is a build artifact with independent content and ExtraContent mount
roots; it is never written into owned source or the supplied reference corpus.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
from pathlib import Path, PurePosixPath


MARKER = ".rbx-generated-in-game-ui-overlay"
ALLOWED_CATEGORIES = {
    "player-ui",
    "in-experience-player-extra-ui",
    "in-experience-icon-font",
    "in-experience-foundation-images",
    "in-experience-text-font",
    "in-experience-date-time-locale",
    "in-experience-ui-model",
}
FORBIDDEN_PARTS = {"LuaApp", "LuaChat", "LuaChatV2", "LuaDiscussions"}


class ImportError(RuntimeError):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def destination_for(entry: dict[str, object]) -> PurePosixPath:
    source = PurePosixPath(str(entry["sourcePath"]))
    if source.is_absolute() or ".." in source.parts or any(part in FORBIDDEN_PARTS for part in source.parts):
        raise ImportError(f"unsafe or excluded source path {source}")
    category = entry["category"]
    if category in {"player-ui", "in-experience-text-font", "in-experience-date-time-locale"}:
        prefix = PurePosixPath("content")
        if not source.parts or source.parts[0] != "content":
            raise ImportError(f"canonical Player UI path is outside content: {source}")
        relative = PurePosixPath(*source.parts[1:])
        return prefix / relative
    if category in {"in-experience-player-extra-ui", "in-experience-icon-font", "in-experience-foundation-images", "in-experience-ui-model"}:
        prefix = PurePosixPath("ExtraContent")
        if not source.parts or source.parts[0] != "ExtraContent":
            raise ImportError(f"in-experience extra UI path is outside ExtraContent: {source}")
        relative = PurePosixPath(*source.parts[1:])
        return prefix / relative
    raise ImportError(f"category {category!r} is not approved for the in-game UI overlay")


def prepare_output(output: Path) -> None:
    if output.exists():
        if output.is_dir() and not any(output.iterdir()):
            pass
        elif not (output / MARKER).is_file():
            raise ImportError(f"refusing to replace unmarked output directory {output}")
        else:
            shutil.rmtree(output)
    output.mkdir(parents=True, exist_ok=True)
    (output / MARKER).write_text("generated; safe to replace\n", encoding="utf-8")


def import_overlay(inventory_path: Path, player_root: Path, output: Path) -> dict[str, object]:
    inventory = json.loads(inventory_path.read_text(encoding="utf-8"))
    scope = inventory.get("scope") or {}
    if scope.get("productSurface") != "in-experience-player-core-ui":
        raise ImportError("inventory is not scoped to the in-experience Player CoreGui")
    if not player_root.is_dir():
        raise ImportError("Player reference root must be a directory")

    selected = [
        entry
        for entry in inventory.get("entries", [])
        if entry.get("sourceCorpus") == "player" and entry.get("category") in ALLOWED_CATEGORIES
    ]
    if not selected:
        raise ImportError("inventory contains no approved in-experience Player entries")

    destinations: dict[str, dict[str, object]] = {}
    for entry in selected:
        destination = destination_for(entry).as_posix()
        if destination in destinations:
            raise ImportError(f"duplicate package destination {destination}")
        destinations[destination] = entry

    prepare_output(output)
    records: list[dict[str, object]] = []
    for destination, entry in sorted(destinations.items()):
        source_relative = PurePosixPath(str(entry["sourcePath"]))
        source = player_root.joinpath(*source_relative.parts)
        if not source.is_file() or source.is_symlink():
            raise ImportError(f"selected Player file is missing or is a symlink: {source_relative}")
        actual_size = source.stat().st_size
        actual_hash = sha256(source)
        if actual_size != entry["size"] or actual_hash != entry["sha256"]:
            raise ImportError(f"Player file changed after inventory: {source_relative}")
        target = output.joinpath(*PurePosixPath(destination).parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        records.append(
            {
                "logicalPath": entry["logicalPath"],
                "sourceCorpus": "windows-player-ddf02245bdbb428c",
                "sourcePath": entry["sourcePath"],
                "sha256": actual_hash,
                "size": actual_size,
                "category": entry["category"],
                "packageDestination": f"Resources/overlays/player-2026/{destination}",
                "licenseDecision": "user-supplied-authentic-player-resource-local-package; redistribution-review-pending",
                "compatibilityStatus": "packaged-overlay; runtime-semantics-require-CoreGui-verification",
                "conversion": "none",
                "platformVariant": entry["platformVariant"],
            }
        )

    manifest = {
        "schemaVersion": 1,
        "productSurface": "in-experience-player-core-ui",
        "sourceInventorySha256": sha256(inventory_path),
        "policy": {
            "standaloneAppUiIncluded": False,
            "payloadHashesVerified": True,
            "referenceAbsolutePathsRecorded": False,
            "redistributionReviewComplete": False,
        },
        "summary": {
            "assetCount": len(records),
            "byteCount": sum(int(record["size"]) for record in records),
            "canonicalContentCount": sum(record["category"] == "player-ui" for record in records),
            "inExperienceExtraContentCount": sum(
                record["category"] in {"in-experience-player-extra-ui", "in-experience-icon-font", "in-experience-ui-model"}
                for record in records
            ),
            "builderIconAssetCount": sum(
                record["category"] == "in-experience-icon-font" for record in records
            ),
            "foundationImageAssetCount": sum(
                record["category"] == "in-experience-foundation-images" for record in records
            ),
            "textFontAssetCount": sum(
                record["category"] == "in-experience-text-font" for record in records
            ),
        },
        "assets": records,
    }
    manifest_path = output / "overlay-manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", type=Path, required=True)
    parser.add_argument("--player-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        manifest = import_overlay(
            args.inventory.resolve(strict=True),
            args.player_root.resolve(strict=True),
            args.output.resolve(strict=False),
        )
        print(json.dumps(manifest["summary"], sort_keys=True))
    except (ImportError, OSError, json.JSONDecodeError) as error:
        print(f"in-game UI import failed: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
