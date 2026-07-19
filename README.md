# Novalume

Novalume is a cross-platform modernization of the classic Roblox client and
server source tree. It preserves the historical engine where compatibility
requires it while replacing obsolete platform, rendering, networking, audio,
and build infrastructure with maintained equivalents.

The active macOS Player runs the real DataModel, Luau scripts, simulation,
bgfx/Metal renderer, GameNetworkingSockets transport, miniaudio runtime, and
package-backed current Roblox resources. Standard R15, R15-plus, and supported
Rthro rigs use hash-pinned assets from the designated Studio build. Modern
RBXL/RBXLX compatibility is verified against the selected Backrooms fixture;
the project also provides an RBXLP place package format for embedding authorized
assets without changing authored place semantics.

This remains an active modernization project. See
[`docs/modernization/STATUS.md`](docs/modernization/STATUS.md) for evidence and
known incomplete work. In particular, the authentic current in-experience UI
is preserved but its final semantic and visual acceptance is deferred until the
remaining non-UI runtime and platform work is complete.

## Repository layout

- `apps/player` — shared Player entry point and platform packaging
- `apps/services` — RCC and related server applications
- `engine` — subsystem-owned runtime libraries
- `platform` — host and platform integrations
- `shaders` — owned cross-platform shader sources and manifests
- `content` — preserved and imported runtime content
- `tests` — unit, integration, rendering, and compatibility contracts
- `tools` — dependency, packaging, import, and verification tools
- `third_party` — documented external and historical dependencies

## macOS arm64 build

Requirements are CMake 3.28+, Ninja, Xcode, Python 3, and the pinned dependency
sources fetched by CMake. Build the pinned OpenSSL archive first:

```sh
./tools/dependencies/build-openssl.sh macos-arm64
cmake --preset macos-arm64-release
cmake --build --preset macos-arm64-release -j 1
ctest --preset macos-arm64-release
```

The self-contained application is produced at:

```text
out/build/macos-arm64-release/apps/player/RobloxPlayer.app
```

Reference-dependent resource imports require the exact external roots recorded
in `CMakePresets.json`. Importers validate their manifests and hashes; missing
or mismatched reference data is an error rather than permission to substitute
or fabricate assets.

## Compatibility and verification

The default build excludes the historical D3D9, D3D11, and OpenGL renderers but
does not delete them. Current verification includes deterministic audio,
network transport and DataModel replication, serialization, R15 animation and
mesh loading, package-local asset delivery, and actual Metal Player runs.

Project boundaries and acceptance policy are documented in
[`AGENTS.md`](AGENTS.md), and the immutable execution checklist is in
[`docs/modernization/TASKLIST.md`](docs/modernization/TASKLIST.md).

## Provenance

The source began from the public 2016 Roblox tree maintained at
[`Julien-Rodot/Roblox-2016-`](https://github.com/Julien-Rodot/Roblox-2016-).
Third-party licensing and attribution are preserved in
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) and the files under
`third_party`.
