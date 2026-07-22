#!/usr/bin/env python3

import os
import pathlib
import subprocess
import sys
import tempfile


def run(player: pathlib.Path, place: pathlib.Path, cache_root: pathlib.Path,
        proof: pathlib.Path, verifier: str, offline: bool) -> str:
    environment = os.environ.copy()
    environment["TMPDIR"] = str(cache_root)
    if offline:
        dead_proxy = "http://127.0.0.1:9"
        environment.update({
            "HTTPS_PROXY": dead_proxy,
            "HTTP_PROXY": dead_proxy,
            "ALL_PROXY": dead_proxy,
            "NO_PROXY": "",
            "no_proxy": "",
        })
    command = [
        str(player), "--headless-verify", verifier,
        "--place", str(place), "--frame-limit", "300",
        "--render-proof", str(proof),
    ]
    result = subprocess.run(command, env=environment, text=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=30)
    if result.returncode:
        print(result.stdout, file=sys.stderr)
        raise RuntimeError(f"{'offline' if offline else 'warm'} asset run failed")
    return result.stdout


def main() -> int:
    if len(sys.argv) != 4:
        raise RuntimeError("expected PLAYER PLACE OUTPUT_DIRECTORY")
    player, place, output = map(pathlib.Path, sys.argv[1:])
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="rbx-offline-cache-") as temporary:
        cache_root = pathlib.Path(temporary)
        run(player, place, cache_root, output / "cache-warm.ppm",
            "--verify-surface-textures", False)
        surface = run(player, place, cache_root, output / "cache-offline-surface.ppm",
                      "--verify-surface-textures", True)
        sky = run(player, place, cache_root, output / "cache-offline-sky.ppm",
                  "--verify-skybox", True)

    if "surface textures Baseplate status=3 size=400x400" not in surface or \
            "SpawnLocation status=3 size=64x64" not in surface:
        raise RuntimeError("offline process did not recover both surface textures")
    if "skybox faces 0=3:1024x1024" not in sky or \
            "5=3:1024x1024" not in sky:
        raise RuntimeError("offline process did not recover all skybox faces")
    print("cold cache warmed once; surface textures and six sky faces passed "
          "in fresh processes behind a dead proxy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
