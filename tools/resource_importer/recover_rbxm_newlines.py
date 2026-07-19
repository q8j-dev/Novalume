#!/usr/bin/env python3
"""Recover RBXM files that were accidentally subjected to CRLF conversion."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


CORRUPTED_SIGNATURE = b"<roblox!\x89\xff\r\n\x1a\r\n\x00"
RBXM_SIGNATURE = b"<roblox!\x89\xff\r\n\x1a\n\x00\x00"


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--input-sha256", required=True)
    parser.add_argument("--output-sha256", required=True)
    arguments = parser.parse_args()

    source = arguments.input.read_bytes()
    source_hash = sha256(source)
    if source_hash != arguments.input_sha256:
        raise SystemExit(
            f"refusing to recover unrecognized RBXM {arguments.input}: "
            f"expected {arguments.input_sha256}, got {source_hash}"
        )
    if not source.startswith(CORRUPTED_SIGNATURE):
        raise SystemExit(f"{arguments.input} does not have the known CRLF-corrupted signature")

    recovered = source.replace(b"\r\n", b"\n")
    recovered_hash = sha256(recovered)
    if recovered_hash != arguments.output_sha256:
        raise SystemExit(
            f"recovered RBXM hash mismatch for {arguments.input}: "
            f"expected {arguments.output_sha256}, got {recovered_hash}"
        )
    if not recovered.startswith(RBXM_SIGNATURE):
        raise SystemExit(f"{arguments.input} did not recover to the canonical RBXM signature")

    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_bytes(recovered)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
