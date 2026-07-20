#!/usr/bin/env python3
"""Create and verify the private, generated Player runtime resource bundle.

The bundle is a build input for hosts that cannot access the user-supplied
reference installations.  It contains only the already-scoped generated
overlays and models needed by the Player; source history remains payload-free.
Every archive member is recorded by path, size, and SHA-256, and extraction is
refused unless the archive is exact and path-safe.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import stat
import sys
import zipfile
from pathlib import Path, PurePosixPath


MANIFEST_NAME = "runtime-resource-bundle.json"
EXTRACTED_MARKER = ".rbx-runtime-resource-bundle"
FIXED_ZIP_TIME = (2026, 1, 1, 0, 0, 0)
REQUIRED_FILES = {
    "overlays/player-2026/overlay-manifest.json",
    "overlays/avatar-runtime/avatar-runtime-manifest.json",
    "overlays/studio-runtime/studio-runtime-manifest.json",
    "models/InExperience/InExperience.rbxm",
    "models/InExperience/InExperience_checksum",
    "content/scripts/PlayerScripts.rbxmx",
    "content/scripts/PlayerScripts.manifest.json",
    "PlatformContent/pc/fonts/NotoSansCJKjp-Regular.otf",
}
ALLOWED_PREFIXES = (
    "overlays/player-2026/",
    "overlays/avatar-runtime/",
    "overlays/studio-runtime/",
    "models/InExperience/",
    "content/scripts/PlayerScripts.",
    "PlatformContent/pc/fonts/NotoSansCJKjp-Regular.otf",
)


class BundleError(RuntimeError):
    pass


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_path(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def safe_payload_path(value: str) -> PurePosixPath:
    path = PurePosixPath(value)
    if (
        path.is_absolute()
        or not path.parts
        or ".." in path.parts
        or "\\" in value
        or value == MANIFEST_NAME
        or not any(value.startswith(prefix) for prefix in ALLOWED_PREFIXES)
    ):
        raise BundleError(f"unsafe or out-of-scope bundle path: {value}")
    return path


def selected_files(resource_root: Path) -> list[tuple[str, Path]]:
    candidates: list[Path] = []
    for relative in (
        "overlays/player-2026",
        "overlays/avatar-runtime",
        "overlays/studio-runtime",
    ):
        root = resource_root / relative
        if root.is_dir():
            candidates.extend(path for path in root.rglob("*") if path.is_file())
    candidates.extend(resource_root / relative for relative in sorted(REQUIRED_FILES))

    selected: dict[str, Path] = {}
    for path in candidates:
        if not path.is_file() or path.is_symlink():
            continue
        try:
            relative = path.relative_to(resource_root).as_posix()
        except ValueError as error:
            raise BundleError(f"payload escaped resource root: {path}") from error
        safe_payload_path(relative)
        selected[relative] = path

    missing = sorted(REQUIRED_FILES - selected.keys())
    if missing:
        raise BundleError("resource root is incomplete: " + ", ".join(missing))
    return sorted(selected.items())


def zip_info(name: str) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(name, FIXED_ZIP_TIME)
    info.compress_type = zipfile.ZIP_DEFLATED
    info.create_system = 3
    info.external_attr = (stat.S_IFREG | 0o644) << 16
    return info


def create_bundle(resource_root: Path, output: Path, bundle_id: str) -> dict[str, object]:
    resource_root = resource_root.resolve(strict=True)
    if output.exists():
        raise BundleError(f"refusing to overwrite existing bundle: {output}")
    files = selected_files(resource_root)
    records: list[dict[str, object]] = []
    payloads: list[tuple[str, bytes]] = []
    for relative, source in files:
        payload = source.read_bytes()
        records.append(
            {"path": relative, "sha256": sha256_bytes(payload), "size": len(payload)}
        )
        payloads.append((relative, payload))

    manifest: dict[str, object] = {
        "schemaVersion": 1,
        "bundleId": bundle_id,
        "policy": {
            "generatedBuildInput": True,
            "hashesVerified": True,
            "payloadsStoredInSourceHistory": False,
        },
        "fileCount": len(records),
        "byteCount": sum(int(record["size"]) for record in records),
        "files": records,
    }
    manifest_bytes = (
        json.dumps(manifest, indent=2, ensure_ascii=False, sort_keys=True) + "\n"
    ).encode("utf-8")
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        archive.writestr(zip_info(MANIFEST_NAME), manifest_bytes, compresslevel=9)
        for relative, payload in payloads:
            archive.writestr(zip_info(relative), payload, compresslevel=9)
    return manifest


def read_verified_bundle(bundle: Path) -> tuple[dict[str, object], dict[str, bytes]]:
    try:
        with zipfile.ZipFile(bundle.resolve(strict=True), "r") as archive:
            infos = archive.infolist()
            names = [info.filename for info in infos]
            if len(names) != len(set(names)):
                raise BundleError("bundle contains duplicate archive paths")
            if MANIFEST_NAME not in names:
                raise BundleError("bundle manifest is missing")
            if any(info.is_dir() for info in infos):
                raise BundleError("bundle must contain files only")
            for info in infos:
                mode = info.external_attr >> 16
                if stat.S_ISLNK(mode):
                    raise BundleError(f"bundle contains a symbolic link: {info.filename}")

            manifest_bytes = archive.read(MANIFEST_NAME)
            manifest = json.loads(manifest_bytes.decode("utf-8"))
            if manifest.get("schemaVersion") != 1:
                raise BundleError("unsupported runtime resource bundle schema")
            records = manifest.get("files")
            if not isinstance(records, list):
                raise BundleError("bundle manifest has no file records")

            payloads: dict[str, bytes] = {}
            expected_names = {MANIFEST_NAME}
            total_size = 0
            for record in records:
                if not isinstance(record, dict):
                    raise BundleError("invalid bundle file record")
                relative = str(record.get("path", ""))
                safe_payload_path(relative)
                if relative in payloads:
                    raise BundleError(f"duplicate manifest path: {relative}")
                if relative not in names:
                    raise BundleError(f"manifest payload is missing: {relative}")
                payload = archive.read(relative)
                if len(payload) != record.get("size") or sha256_bytes(payload) != record.get("sha256"):
                    raise BundleError(f"payload failed size/SHA-256 verification: {relative}")
                payloads[relative] = payload
                expected_names.add(relative)
                total_size += len(payload)

            if set(names) != expected_names:
                extras = sorted(set(names) - expected_names)
                raise BundleError("bundle contains unrecorded payloads: " + ", ".join(extras))
            missing = sorted(REQUIRED_FILES - payloads.keys())
            if missing:
                raise BundleError("bundle is missing required runtime payloads: " + ", ".join(missing))
            if manifest.get("fileCount") != len(payloads) or manifest.get("byteCount") != total_size:
                raise BundleError("bundle aggregate counts do not match its payloads")
            return manifest, payloads
    except (OSError, zipfile.BadZipFile, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise BundleError(f"cannot read runtime resource bundle: {error}") from error


def extract_bundle(bundle: Path, output: Path) -> dict[str, object]:
    manifest, payloads = read_verified_bundle(bundle)
    output = output.resolve()
    if output.exists() and any(output.iterdir()):
        if not (output / EXTRACTED_MARKER).is_file():
            raise BundleError(f"refusing to replace unmarked extraction directory: {output}")
        shutil.rmtree(output)
    output.mkdir(parents=True, exist_ok=True)
    for relative, payload in sorted(payloads.items()):
        target = output.joinpath(*PurePosixPath(relative).parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(payload)
    (output / EXTRACTED_MARKER).write_text(
        f"{manifest['bundleId']}\n{sha256_path(bundle)}\n", encoding="utf-8"
    )
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    create = subparsers.add_parser("create")
    create.add_argument("--resource-root", type=Path, required=True)
    create.add_argument("--output", type=Path, required=True)
    create.add_argument("--bundle-id", required=True)
    verify = subparsers.add_parser("verify")
    verify.add_argument("--bundle", type=Path, required=True)
    verify.add_argument("--extract", type=Path)
    args = parser.parse_args()
    try:
        if args.command == "create":
            manifest = create_bundle(args.resource_root, args.output, args.bundle_id)
        elif args.extract:
            manifest = extract_bundle(args.bundle, args.extract)
        else:
            manifest, _ = read_verified_bundle(args.bundle)
        print(
            json.dumps(
                {
                    "bundleId": manifest["bundleId"],
                    "byteCount": manifest["byteCount"],
                    "fileCount": manifest["fileCount"],
                },
                sort_keys=True,
            )
        )
    except BundleError as error:
        print(f"runtime resource bundle failed: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
