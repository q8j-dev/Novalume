#!/usr/bin/env python3
"""Compare the selected modern RBXL/RBXLX through isolated engine processes."""

from __future__ import annotations

import re
import subprocess
import sys


PATTERN = re.compile(
    r"inventory instances=(\d+) parts=(\d+) scripts=(\d+) prompts=(\d+) fontFaces=(\d+)"
)


def inspect(executable: str, path: str) -> tuple[int, ...]:
    completed = subprocess.run(
        [executable, path], check=False, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"place inspection failed for {path} with {completed.returncode}\n"
            f"{completed.stdout[-4000:]}"
        )
    matches = PATTERN.findall(completed.stdout)
    if len(matches) != 1:
        raise RuntimeError(f"place inspection produced no unique inventory for {path}")
    return tuple(int(value) for value in matches[0])


def main() -> int:
    if len(sys.argv) != 4:
        raise RuntimeError("expected inspector, RBXL, and RBXLX paths")
    binary = inspect(sys.argv[1], sys.argv[2])
    xml = inspect(sys.argv[1], sys.argv[3])
    if binary != xml:
        raise RuntimeError(f"selected place inventory drift: binary={binary}, xml={xml}")
    print(
        "selected pair equivalent: "
        f"instances={binary[0]} parts={binary[1]} scripts={binary[2]} "
        f"prompts={binary[3]} fontFaces={binary[4]}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
