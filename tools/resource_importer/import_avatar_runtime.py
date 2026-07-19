#!/usr/bin/env python3
"""Copy only hash-pinned current avatar runtime assets into a generated overlay."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import shutil


def digest(path: pathlib.Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=pathlib.Path, required=True)
    parser.add_argument("--studio-root", type=pathlib.Path, required=True)
    parser.add_argument("--asset-archive-root", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    if manifest.get("schemaVersion") != 1:
        raise RuntimeError("unsupported avatar asset manifest")
    studio_root = args.studio_root.resolve(strict=True)
    archive_root = (args.asset_archive_root.resolve(strict=True)
                    if args.asset_archive_root else None)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    copied = []
    for record in manifest["assets"]:
        relative = pathlib.PurePosixPath(record["path"])
        if relative.is_absolute() or ".." in relative.parts or relative.parts[:2] != ("content", "avatar"):
            raise RuntimeError(f"unsafe avatar asset path: {relative}")
        source_relative = pathlib.PurePosixPath(record.get("sourcePath", record["path"]))
        allowed_source = (
            source_relative.parts[:2] == ("content", "avatar")
            or source_relative == pathlib.PurePosixPath(
                "content/models/Thumbnails/Mannequins/R15-plus.rbxm")
            or source_relative == pathlib.PurePosixPath(
                "StudioContent/models/RigBuilder/AnthroRigs.rbxm")
        )
        if source_relative.is_absolute() or ".." in source_relative.parts or not allowed_source:
            raise RuntimeError(f"unsafe Studio avatar source path: {source_relative}")
        source = studio_root.joinpath(*source_relative.parts)
        if not source.is_file() or source.is_symlink():
            raise RuntimeError(f"missing avatar asset: {relative}")
        if source.stat().st_size != record["size"] or digest(source) != record["sha256"]:
            raise RuntimeError(f"avatar asset differs from pinned Studio build: {relative}")
        target = output.joinpath(*relative.parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        copied.append(record)
    for record in manifest.get("archiveAssets", []):
        if archive_root is None:
            raise RuntimeError("avatar asset archive is required by the manifest")
        relative = pathlib.PurePosixPath(record["path"])
        source_relative = pathlib.PurePosixPath(record["sourcePath"])
        if (relative.is_absolute() or ".." in relative.parts or
                relative.parts[:2] != ("content", "assets")):
            raise RuntimeError(f"unsafe archived avatar asset path: {relative}")
        if (source_relative.is_absolute() or ".." in source_relative.parts or
                source_relative.parts[:1] != ("assets",)):
            raise RuntimeError(f"unsafe archived avatar source path: {source_relative}")
        source = archive_root.joinpath(*source_relative.parts)
        if not source.is_file() or source.is_symlink():
            raise RuntimeError(f"missing archived avatar asset: {source_relative}")
        if source.stat().st_size != record["size"] or digest(source) != record["sha256"]:
            raise RuntimeError(f"archived avatar asset differs from pinned payload: {relative}")
        target = output.joinpath(*relative.parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        copied.append(record)
    result = {
        "schemaVersion": 1,
        "sourceBuild": manifest["sourceBuild"],
        "assets": copied,
        "assetCount": len(copied),
        "byteCount": sum(record["size"] for record in copied),
    }
    (output / "avatar-runtime-manifest.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"avatar runtime assets={result['assetCount']} bytes={result['byteCount']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
