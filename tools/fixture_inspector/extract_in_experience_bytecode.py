#!/usr/bin/env python3
"""Extract decoded Luau chunks from a supplied InExperience model for analysis.

The output is an analysis workspace, not a runtime content package.  Every
entry is hash-indexed in a manifest so later reverse-engineering results can be
traced to the exact signed Player fixture that supplied it.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import pathlib

from in_experience_contracts import FormatError, luau_contract, modules_from_model


def safe_relative_path(module_path: str, prefix: str) -> pathlib.Path:
    relative = module_path[len(prefix) :].lstrip("/")
    components = [component for component in relative.split("/") if component]
    if not components or any(component in {".", ".."} for component in components):
        raise FormatError(f"unsafe or empty module path: {module_path}")
    return pathlib.Path(*components[:-1], components[-1] + ".lua")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", required=True, type=pathlib.Path)
    parser.add_argument("--prefix", required=True)
    parser.add_argument(
        "--module",
        help="extract only this exact module path (it must remain under --prefix)",
    )
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument(
        "--synsave-wrapper",
        action="store_true",
        help="write comment-plus-base64 files accepted by folder decompilers",
    )
    parser.add_argument(
        "--include-signed",
        action="store_true",
        help="also write the original signed property bytes beside each decoded chunk",
    )
    arguments = parser.parse_args()

    model_data = arguments.model.resolve().read_bytes()
    modules = modules_from_model(model_data)
    if arguments.module and not arguments.module.startswith(arguments.prefix.rstrip("/") + "/"):
        raise FormatError("--module must be a descendant of --prefix")
    selected = [
        module
        for path, module in sorted(modules.items())
        if path.startswith(arguments.prefix)
        and (arguments.module is None or path == arguments.module)
    ]
    if not selected:
        raise FormatError(f"no modules match prefix: {arguments.prefix}")

    arguments.output.mkdir(parents=True, exist_ok=True)
    entries: list[dict[str, object]] = []
    for module in selected:
        decoded: list[bytes] = []
        contract = luau_contract(module.source, decoded)
        relative_path = safe_relative_path(module.path, arguments.prefix)
        destination = arguments.output / relative_path
        destination.parent.mkdir(parents=True, exist_ok=True)
        if arguments.synsave_wrapper:
            destination.write_text(
                "-- decoded from the supplied signed InExperience fixture\n"
                + base64.b64encode(decoded[0]).decode("ascii")
                + "\n"
            )
        else:
            destination.write_bytes(decoded[0])
        signed_file = None
        if arguments.include_signed:
            signed_destination = destination.with_suffix(".signed")
            signed_destination.write_bytes(module.source)
            signed_file = signed_destination.relative_to(arguments.output).as_posix()
        entries.append(
            {
                "path": module.path,
                "file": relative_path.as_posix(),
                "source_sha256": hashlib.sha256(module.source).hexdigest(),
                "decoded_sha256": hashlib.sha256(decoded[0]).hexdigest(),
                "bytecode_version": contract["bytecode_version"],
                "signed_file": signed_file,
            }
        )

    manifest = {
        "schema": 1,
        "model_sha256": hashlib.sha256(model_data).hexdigest(),
        "prefix": arguments.prefix,
        "module_count": len(entries),
        "modules": entries,
    }
    (arguments.output / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
