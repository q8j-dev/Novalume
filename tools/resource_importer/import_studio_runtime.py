#!/usr/bin/env python3
"""Copy hash-pinned runtime assets from the supplied Studio build."""

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


def safe_relative(value: str) -> pathlib.PurePosixPath:
    result = pathlib.PurePosixPath(value)
    if result.is_absolute() or ".." in result.parts or not result.parts:
        raise RuntimeError(f"unsafe runtime asset path: {value}")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=pathlib.Path, required=True)
    parser.add_argument("--studio-root", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    if manifest.get("schemaVersion") != 1:
        raise RuntimeError("unsupported Studio runtime manifest")
    studio_root = args.studio_root.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    copied = []
    for record in manifest["assets"]:
        source_relative = safe_relative(record["source"])
        target_relative = safe_relative(record["target"])
        if target_relative.parts[0] != "content":
            raise RuntimeError(f"runtime target must live under content: {target_relative}")
        source = studio_root.joinpath(*source_relative.parts)
        if not source.is_file() or source.is_symlink():
            raise RuntimeError(f"missing Studio runtime asset: {source_relative}")
        if source.stat().st_size != record["size"] or digest(source) != record["sha256"]:
            raise RuntimeError(f"Studio runtime asset differs from pinned build: {source_relative}")
        target = output.joinpath(*target_relative.parts)
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
    (output / "studio-runtime-manifest.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"Studio runtime assets={result['assetCount']} bytes={result['byteCount']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
