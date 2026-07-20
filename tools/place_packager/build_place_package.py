#!/usr/bin/env python3
"""Build a deterministic, self-contained RBXLP place package."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import struct
import tempfile
import zlib


MAGIC = b"RBXLPK1\0"
VERSION = 1


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def archived_assets(archive: pathlib.Path) -> tuple[list[tuple[str, pathlib.Path]], list[str]]:
    manifest_path = archive / "manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    records = manifest.get("assets")
    if not isinstance(records, dict):
        raise ValueError("asset archiver manifest must contain an assets object")

    embedded: list[tuple[str, pathlib.Path]] = []
    missing: list[str] = []
    for key, record in sorted(records.items(), key=lambda item: int(item[0])):
        asset_id = str(record.get("assetId", key))
        if not asset_id.isdecimal():
            raise ValueError(f"invalid asset ID in archiver manifest: {asset_id!r}")
        if record.get("status") not in ("downloaded", "complete"):
            missing.append(asset_id)
            continue
        relative = record.get("path") or record.get("file") or record.get("filename")
        if not isinstance(relative, str):
            candidates = sorted((archive / "assets").glob(f"{asset_id}.*"))
            if len(candidates) != 1:
                raise ValueError(f"archived asset {asset_id} has no unique payload")
            payload = candidates[0]
        else:
            payload = archive / relative
        payload = payload.resolve()
        if archive.resolve() not in payload.parents or not payload.is_file():
            raise ValueError(f"archived asset {asset_id} has an unsafe or missing payload")
        expected_hash = record.get("sha256")
        actual_hash = sha256(payload)
        if expected_hash and expected_hash.lower() != actual_hash:
            raise ValueError(f"archived asset {asset_id} failed SHA-256 verification")
        embedded.append((asset_id, payload))
    return embedded, missing


def write_entry(output, name: str, payload: pathlib.Path | bytes) -> dict[str, object]:
    encoded_name = name.encode("utf-8")
    if not encoded_name or len(encoded_name) > 65535:
        raise ValueError(f"invalid RBXLP entry name: {name!r}")
    if isinstance(payload, pathlib.Path):
        size = payload.stat().st_size
        crc = 0
        digest = hashlib.sha256()
        with payload.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                crc = zlib.crc32(chunk, crc)
                digest.update(chunk)
        output.write(struct.pack("<HQI", len(encoded_name), size, crc & 0xFFFFFFFF))
        output.write(encoded_name)
        with payload.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                output.write(chunk)
        return {"path": name, "bytes": size, "sha256": digest.hexdigest()}

    crc = zlib.crc32(payload) & 0xFFFFFFFF
    output.write(struct.pack("<HQI", len(encoded_name), len(payload), crc))
    output.write(encoded_name)
    output.write(payload)
    return {"path": name, "bytes": len(payload), "sha256": hashlib.sha256(payload).hexdigest()}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--place", type=pathlib.Path, required=True)
    parser.add_argument("--archive", type=pathlib.Path, required=True,
                        help="output directory created by roblox_asset_archiver.py")
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--allow-missing", action="store_true",
                        help="record inaccessible assets instead of rejecting the package")
    parser.add_argument("--local-solo", action="store_true",
                        help="run the packaged place as an offline Play Solo session")
    parser.add_argument("--bootstrap-script", type=pathlib.Path,
                        help="trusted server bootstrap used only by a local-solo package")
    parser.add_argument("--client-bootstrap-script", type=pathlib.Path,
                        help="trusted client bootstrap used only by a local-solo package")
    args = parser.parse_args()

    place = args.place.resolve()
    if not place.is_file() or place.suffix.lower() not in (".rbxl", ".rbxlx"):
        parser.error("--place must be an existing RBXL or RBXLX file")
    embedded, missing = archived_assets(args.archive.resolve())
    if missing and not args.allow_missing:
        parser.error("archive is incomplete; missing asset IDs: " + ", ".join(missing))
    bootstrap_script = None
    if args.bootstrap_script:
        if not args.local_solo:
            parser.error("--bootstrap-script requires --local-solo")
        bootstrap_script = args.bootstrap_script.resolve()
        if not bootstrap_script.is_file() or bootstrap_script.suffix.lower() != ".lua":
            parser.error("--bootstrap-script must be an existing Lua file")
    client_bootstrap_script = None
    if args.client_bootstrap_script:
        if not args.local_solo:
            parser.error("--client-bootstrap-script requires --local-solo")
        client_bootstrap_script = args.client_bootstrap_script.resolve()
        if (not client_bootstrap_script.is_file() or
                client_bootstrap_script.suffix.lower() != ".lua"):
            parser.error("--client-bootstrap-script must be an existing Lua file")

    place_name = "place/main" + place.suffix.lower()
    asset_entries = [(f"assets/{asset_id}{payload.suffix.lower()}", payload)
                     for asset_id, payload in embedded]
    metadata = {
        "schema": "rbxlp-place-package-v1",
        "place": place_name,
        "placeSha256": sha256(place),
        "embeddedAssetCount": len(asset_entries),
        "missingAssetIds": missing,
        "assets": [name for name, _ in asset_entries],
        "executionMode": "local-solo" if args.local_solo else "player",
        "serverBootstrap": "launch/local-solo.lua" if bootstrap_script else None,
        "clientBootstrap": ("launch/client-local-solo.lua"
                            if client_bootstrap_script else None),
    }
    metadata_bytes = (json.dumps(metadata, sort_keys=True, separators=(",", ":")) + "\n").encode()
    entries: list[tuple[str, pathlib.Path | bytes]] = [
        ("manifest.json", metadata_bytes),
        (place_name, place),
        *([("launch/local-solo", b"RBXLP local-solo v1\n")]
          if args.local_solo else []),
        *([("launch/local-solo.lua", bootstrap_script)]
          if bootstrap_script else []),
        *([("launch/client-local-solo.lua", client_bootstrap_script)]
          if client_bootstrap_script else []),
        *asset_entries,
    ]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=args.output.parent, delete=False) as temporary:
        temporary_path = pathlib.Path(temporary.name)
        temporary.write(MAGIC)
        temporary.write(struct.pack("<II", VERSION, len(entries)))
        for name, payload in entries:
            write_entry(temporary, name, payload)
        temporary.flush()
    temporary_path.replace(args.output)
    print(f"wrote {args.output} with {len(asset_entries)} embedded assets"
          + (f" ({len(missing)} inaccessible recorded)" if missing else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
