#!/usr/bin/env python3
"""Build a deterministic, metadata-only inventory of supplied references.

The scanner never copies payloads into the repository or package.  Directory
corpora are read in place and ZIP/APK corpora are streamed from the archive.
Only paths selected by the checked-in specification are opened and hashed.
"""

from __future__ import annotations

import argparse
import fnmatch
import hashlib
import json
import mimetypes
import re
import struct
import sys
import zipfile
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Any, Iterable


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
DENSITY_SUFFIX = re.compile(r"@(?P<scale>[1-9][0-9]*)x(?=\.[^.]+$)", re.IGNORECASE)
DENSITY_DIRECTORY = re.compile(
    r"(?:^|/)(?:drawable|mipmap)-(?P<density>ldpi|mdpi|hdpi|xhdpi|xxhdpi|xxxhdpi)(?:/|$)",
    re.IGNORECASE,
)


class InventoryError(RuntimeError):
    pass


@dataclass(frozen=True)
class CorpusInput:
    name: str
    path: Path
    kind: str
    description: str
    expected_sha256: str | None


@dataclass(frozen=True)
class SelectedFile:
    corpus: CorpusInput
    source_path: str
    rule: dict[str, Any]
    data: bytes


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_corpus_arguments(values: list[str]) -> dict[str, Path]:
    result: dict[str, Path] = {}
    for value in values:
        name, separator, path = value.partition("=")
        if not separator or not name or not path:
            raise InventoryError(f"invalid --corpus value {value!r}; expected NAME=PATH")
        if name in result:
            raise InventoryError(f"duplicate --corpus name {name!r}")
        result[name] = Path(path).expanduser().resolve(strict=True)
    return result


def load_spec(path: Path) -> tuple[dict[str, Any], str]:
    raw = path.read_bytes()
    try:
        spec = json.loads(raw)
    except json.JSONDecodeError as error:
        raise InventoryError(f"invalid inventory specification: {error}") from error
    if spec.get("schemaVersion") != 1:
        raise InventoryError("inventory specification must use schemaVersion 1")
    if not isinstance(spec.get("corpora"), dict) or not isinstance(spec.get("rules"), list):
        raise InventoryError("inventory specification requires corpora and rules")
    return spec, sha256_bytes(raw)


def build_corpora(spec: dict[str, Any], supplied: dict[str, Path]) -> dict[str, CorpusInput]:
    unknown = sorted(set(supplied) - set(spec["corpora"]))
    missing = sorted(set(spec["corpora"]) - set(supplied))
    if unknown:
        raise InventoryError(f"unknown corpora supplied: {', '.join(unknown)}")
    if missing:
        raise InventoryError(f"required corpora not supplied: {', '.join(missing)}")

    corpora: dict[str, CorpusInput] = {}
    for name in sorted(spec["corpora"]):
        config = spec["corpora"][name]
        kind = config.get("kind")
        path = supplied[name]
        if kind == "directory" and not path.is_dir():
            raise InventoryError(f"corpus {name!r} must be a directory")
        if kind == "zip" and not path.is_file():
            raise InventoryError(f"corpus {name!r} must be a ZIP-compatible file")
        if kind not in {"directory", "zip"}:
            raise InventoryError(f"corpus {name!r} has unsupported kind {kind!r}")
        expected = config.get("expectedSha256")
        if expected is not None and (not isinstance(expected, str) or len(expected) != 64):
            raise InventoryError(f"corpus {name!r} has an invalid expectedSha256")
        corpora[name] = CorpusInput(
            name=name,
            path=path,
            kind=kind,
            description=str(config.get("description", "")),
            expected_sha256=expected,
        )
    return corpora


def validate_rule(rule: dict[str, Any], corpora: dict[str, CorpusInput]) -> None:
    required = {
        "id",
        "corpus",
        "include",
        "category",
        "purpose",
        "platformVariant",
        "licenseDecision",
        "compatibilityStatus",
        "conversion",
        "logicalPrefix",
        "stripPrefix",
    }
    missing = sorted(required - set(rule))
    if missing:
        raise InventoryError(f"rule is missing fields: {', '.join(missing)}")
    if rule["corpus"] not in corpora:
        raise InventoryError(f"rule {rule['id']!r} references unknown corpus {rule['corpus']!r}")
    if not isinstance(rule["include"], list) or not rule["include"]:
        raise InventoryError(f"rule {rule['id']!r} requires a non-empty include list")
    if rule.get("packageDestination") is not None:
        raise InventoryError(
            f"rule {rule['id']!r} may not package reference files before provenance approval"
        )


def directory_names(root: Path, patterns: list[str]) -> Iterable[str]:
    names: set[str] = set()
    for pattern in patterns:
        for path in root.glob(pattern):
            if not path.is_file() or path.is_symlink():
                continue
            relative = path.relative_to(root).as_posix()
            names.add(relative)
    return sorted(names)


def matches_glob(path: str, pattern: str) -> bool:
    """Match recursive globs while allowing ** to span zero directories."""
    candidates = {pattern}
    pending = [pattern]
    while pending:
        candidate = pending.pop()
        marker = candidate.find("**/")
        if marker != -1:
            collapsed = candidate[:marker] + candidate[marker + 3 :]
            if collapsed not in candidates:
                candidates.add(collapsed)
                pending.append(collapsed)
    return any(fnmatch.fnmatchcase(path, candidate) for candidate in candidates)


def zip_names(archive: zipfile.ZipFile, patterns: list[str]) -> Iterable[str]:
    names = {
        info.filename
        for info in archive.infolist()
        if not info.is_dir()
        and not PurePosixPath(info.filename).is_absolute()
        and ".." not in PurePosixPath(info.filename).parts
        and any(matches_glob(info.filename, pattern) for pattern in patterns)
    }
    return sorted(names)


def select_files(corpora: dict[str, CorpusInput], rules: list[dict[str, Any]]) -> list[SelectedFile]:
    selected: list[SelectedFile] = []
    seen: dict[tuple[str, str], str] = {}
    archives: dict[str, zipfile.ZipFile] = {}
    try:
        for rule in rules:
            validate_rule(rule, corpora)
            corpus = corpora[rule["corpus"]]
            if corpus.kind == "directory":
                names = directory_names(corpus.path, rule["include"])
            else:
                archive = archives.setdefault(corpus.name, zipfile.ZipFile(corpus.path, "r"))
                names = zip_names(archive, rule["include"])

            count = 0
            for source_path in names:
                key = (corpus.name, source_path)
                if key in seen:
                    raise InventoryError(
                        f"{corpus.name}:{source_path} matched both {seen[key]!r} and {rule['id']!r}"
                    )
                seen[key] = rule["id"]
                if corpus.kind == "directory":
                    data = (corpus.path / PurePosixPath(source_path)).read_bytes()
                else:
                    data = archives[corpus.name].read(source_path)
                selected.append(SelectedFile(corpus, source_path, rule, data))
                count += 1
            if count == 0:
                raise InventoryError(f"rule {rule['id']!r} selected no files")
    finally:
        for archive in archives.values():
            archive.close()
    return selected


def png_metadata(data: bytes) -> dict[str, Any]:
    if len(data) < 33 or data[:8] != PNG_SIGNATURE or data[12:16] != b"IHDR":
        raise InventoryError("file has a .png extension but no valid PNG IHDR")
    width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(
        ">IIBBBBB", data[16:29]
    )
    if width == 0 or height == 0:
        raise InventoryError("PNG dimensions must be non-zero")
    return {
        "width": width,
        "height": height,
        "bitDepth": bit_depth,
        "colorType": color_type,
        "compressionMethod": compression,
        "filterMethod": filtering,
        "interlaceMethod": interlace,
    }


def type_metadata(source_path: str, data: bytes) -> dict[str, Any]:
    extension = PurePosixPath(source_path).suffix.lower()
    metadata: dict[str, Any] = {
        "extension": extension,
        "mime": mimetypes.guess_type(source_path)[0] or "application/octet-stream",
    }
    if extension == ".png":
        metadata["image"] = png_metadata(data)
    elif extension == ".json":
        try:
            value = json.loads(data)
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise InventoryError(f"invalid JSON in {source_path}: {error}") from error
        metadata["json"] = {"valid": True, "rootType": type(value).__name__}
    elif extension in {".ttf", ".otf"}:
        metadata["font"] = {"container": extension[1:].upper(), "parsed": False}
    elif extension in {".rbxl", ".rbxm"}:
        metadata["robloxBinary"] = {"parsed": False}
    return metadata


def density_metadata(logical_path: str) -> dict[str, Any]:
    suffix = DENSITY_SUFFIX.search(logical_path)
    directory = DENSITY_DIRECTORY.search(logical_path)
    result: dict[str, Any] = {"scale": 1, "baseLogicalPath": logical_path}
    if suffix:
        result["scale"] = int(suffix.group("scale"))
        result["baseLogicalPath"] = DENSITY_SUFFIX.sub("", logical_path)
    if directory:
        result["androidDensity"] = directory.group("density").lower()
    return result


def logical_path(rule: dict[str, Any], source_path: str) -> str:
    strip_prefix = rule["stripPrefix"]
    if strip_prefix and not source_path.startswith(strip_prefix):
        raise InventoryError(
            f"{source_path!r} selected by {rule['id']!r} does not start with stripPrefix {strip_prefix!r}"
        )
    suffix = source_path[len(strip_prefix) :] if strip_prefix else source_path
    result = f"{rule['logicalPrefix']}{suffix}"
    if result.startswith("/") or "://../" in result or "/../" in result:
        raise InventoryError(f"rule {rule['id']!r} produced unsafe logical path {result!r}")
    return result


def make_entry(selected: SelectedFile) -> dict[str, Any]:
    rule = selected.rule
    logical = logical_path(rule, selected.source_path)
    return {
        "logicalPath": logical,
        "sourceCorpus": selected.corpus.name,
        "sourcePath": selected.source_path,
        "sha256": sha256_bytes(selected.data),
        "size": len(selected.data),
        "typeMetadata": type_metadata(selected.source_path, selected.data),
        "category": rule["category"],
        "purpose": rule["purpose"],
        "platformVariant": rule["platformVariant"],
        "densityVariant": density_metadata(logical),
        "licenseDecision": rule["licenseDecision"],
        "compatibilityStatus": rule["compatibilityStatus"],
        "conversion": rule["conversion"],
        "packageDestination": None,
        "rule": rule["id"],
    }


def comparison_entry(entry: dict[str, Any]) -> dict[str, Any]:
    return {
        "sourcePath": entry["sourcePath"],
        "sha256": entry["sha256"],
        "size": entry["size"],
        "typeMetadata": entry["typeMetadata"],
    }


def build_comparisons(spec: dict[str, Any], entries: list[dict[str, Any]]) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    for comparison in spec.get("comparisons", []):
        required = {"id", "baselineCorpus", "candidateCorpus", "logicalPrefix"}
        missing = sorted(required - set(comparison))
        if missing:
            raise InventoryError(f"comparison is missing fields: {', '.join(missing)}")
        prefix = comparison["logicalPrefix"]
        baseline = {
            entry["logicalPath"]: entry
            for entry in entries
            if entry["sourceCorpus"] == comparison["baselineCorpus"]
            and entry["logicalPath"].startswith(prefix)
        }
        candidate = {
            entry["logicalPath"]: entry
            for entry in entries
            if entry["sourceCorpus"] == comparison["candidateCorpus"]
            and entry["logicalPath"].startswith(prefix)
        }
        if not baseline:
            raise InventoryError(f"comparison {comparison['id']!r} has an empty baseline")
        if not candidate:
            raise InventoryError(f"comparison {comparison['id']!r} has an empty candidate")

        added = []
        removed = []
        changed = []
        unchanged = []
        for logical in sorted(set(baseline) | set(candidate)):
            before = baseline.get(logical)
            after = candidate.get(logical)
            if before is None:
                added.append({"logicalPath": logical, "candidate": comparison_entry(after)})
            elif after is None:
                removed.append({"logicalPath": logical, "baseline": comparison_entry(before)})
            elif before["sha256"] == after["sha256"]:
                unchanged.append(logical)
            else:
                changed.append(
                    {
                        "logicalPath": logical,
                        "baseline": comparison_entry(before),
                        "candidate": comparison_entry(after),
                    }
                )
        result.append(
            {
                "id": comparison["id"],
                "baselineCorpus": comparison["baselineCorpus"],
                "candidateCorpus": comparison["candidateCorpus"],
                "logicalPrefix": prefix,
                "summary": {
                    "baselineCount": len(baseline),
                    "candidateCount": len(candidate),
                    "addedCount": len(added),
                    "removedCount": len(removed),
                    "changedCount": len(changed),
                    "unchangedCount": len(unchanged),
                },
                "added": added,
                "removed": removed,
                "changed": changed,
                "unchanged": unchanged,
            }
        )
    return result


def build_manifest(spec: dict[str, Any], spec_hash: str, corpora: dict[str, CorpusInput]) -> dict[str, Any]:
    corpus_records: list[dict[str, Any]] = []
    for name in sorted(corpora):
        corpus = corpora[name]
        record: dict[str, Any] = {
            "name": name,
            "kind": corpus.kind,
            "description": corpus.description,
        }
        if corpus.kind == "zip":
            actual = sha256_file(corpus.path)
            record["sha256"] = actual
            if corpus.expected_sha256 and actual != corpus.expected_sha256:
                raise InventoryError(
                    f"corpus {name!r} hash mismatch: expected {corpus.expected_sha256}, got {actual}"
                )
            record["expectedSha256"] = corpus.expected_sha256
        corpus_records.append(record)

    selected = select_files(corpora, spec["rules"])
    entries = sorted(
        (make_entry(item) for item in selected),
        key=lambda entry: (entry["logicalPath"], entry["sourceCorpus"], entry["sourcePath"]),
    )

    case_groups: dict[str, list[str]] = {}
    density_groups: dict[str, list[dict[str, Any]]] = {}
    for entry in entries:
        case_groups.setdefault(entry["logicalPath"].casefold(), []).append(entry["logicalPath"])
        density = entry["densityVariant"]
        density_groups.setdefault(density["baseLogicalPath"], []).append(
            {
                "logicalPath": entry["logicalPath"],
                "scale": density["scale"],
                **({"androidDensity": density["androidDensity"]} if "androidDensity" in density else {}),
            }
        )

    collisions = [sorted(set(paths)) for paths in case_groups.values() if len(set(paths)) > 1]
    variant_groups = [
        {"baseLogicalPath": base, "variants": sorted(variants, key=lambda item: item["logicalPath"])}
        for base, variants in sorted(density_groups.items())
        if len(variants) > 1
    ]
    comparisons = build_comparisons(spec, entries)
    return {
        "schemaVersion": 1,
        "purpose": "clean-room-reference-inventory",
        "scope": spec.get("scope"),
        "specSha256": spec_hash,
        "policy": {
            "payloadsCopied": False,
            "packageDestinationsAssigned": False,
            "redistributionApproved": False,
        },
        "corpora": corpus_records,
        "summary": {
            "entryCount": len(entries),
            "byteCount": sum(entry["size"] for entry in entries),
            "caseCollisionCount": len(collisions),
            "densityVariantGroupCount": len(variant_groups),
        },
        "caseCollisions": sorted(collisions),
        "densityVariantGroups": variant_groups,
        "comparisons": comparisons,
        "entries": entries,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--spec", type=Path, required=True)
    parser.add_argument("--corpus", action="append", default=[], metavar="NAME=PATH")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(argv)

    try:
        spec, spec_hash = load_spec(args.spec.resolve(strict=True))
        corpora = build_corpora(spec, parse_corpus_arguments(args.corpus))
        manifest = build_manifest(spec, spec_hash, corpora)
        encoded = json.dumps(manifest, indent=2, ensure_ascii=False, sort_keys=True) + "\n"
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded, encoding="utf-8", newline="\n")
    except (InventoryError, OSError, zipfile.BadZipFile) as error:
        print(f"reference inventory failed: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
