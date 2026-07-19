#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import subprocess
import sys
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]


class RootLayoutTests(unittest.TestCase):
    def test_every_top_level_directory_is_classified(self) -> None:
        result = subprocess.run(
            [sys.executable, str(ROOT / "tools/layout_audit/check_root_layout.py")],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
