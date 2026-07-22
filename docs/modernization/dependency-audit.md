# Dependency modernization audit

The named engine graph now includes bgfx/bx/bimg, miniaudio, FFmpeg, FreeType,
HarfBuzz, utf8proc, SheenBidi, SDL, GameNetworkingSockets, protobuf, Abseil,
curl, zlib, Zstandard, Luau, Draco, Boost, the retained legacy LZ4 codec, and
the platform crypto provider. It
does not link the historical FMOD, RakNet, VMProtect, Qt, XULRunner, libwww,
CAB/DirectX SDK, Mesa, old VR SDK, or Breakpad trees. The foundation Player
executable still does not link every named engine archive, so this statement is
about the active engine build graph, not completed Player integration.

| Historical dependency | Default player decision | Replacement / state |
|---|---|---|
| D3D9/D3D11/OpenGL implementations | Disabled (`RBX_ENABLE_LEGACY_RENDERERS=OFF`) | bgfx backend attached to GfxCore; resource parity incomplete |
| FMOD | Converted audio/DataModel graph no longer includes or references it; quarantined historical files retained | Pinned miniaudio 0.11.25 runtime passes deterministic decode/playback/control/lifetime/streaming/3D/region/bus/effect tests plus a live CoreAudio device smoke test; the complete AudioPlayer/AudioEmitter/AudioListener graph and real Player acceptance remain |
| RakNet | Removed from the active CMake runtime, public replication contracts, and Windows debug network profiler; historical source retained | Pinned GameNetworkingSockets 1.5.0 native transport plus owned packet/endpoint/handler contracts and local diagnostic logging; unauthenticated remote IP peers are rejected, certificate provisioning is exposed, and full certificate lifecycle, Replicator client/server acceptance, and browser WebRTC adapter remain |
| VMProtect | Excluded by default; the optional marker target accepts only a separately supplied genuine SDK and target import library | No portable replacement; proprietary code transformation is not appropriate for the default runtime |
| Qt/WebKit/XULRunner | Excluded with Studio/browser tooling | Native/portable host and existing CoreScript UI path |
| libwww/old curl/TLS | Excluded | Pinned curl 8.21.0 uses HTTP(S) only with the target TLS provider and zlib; offline default |
| historical FreeType | Not in new graph | Replaced by immutable FreeType 2.14.3, HarfBuzz 14.2.1, utf8proc 2.11.3/Unicode 17, and SheenBidi 3.0.0 static targets; full text acceptance remains |
| historical Bullet | Not in new graph | Current Bullet adapter planned after simulation target split |
| Breakpad | Excluded | Crashpad or native crash handling later, opt-in and no endpoint by default |
| bgfx/bx/bimg | Included from official immutable commits | BSD-2-Clause; exact pins in `RbxDependencies.cmake` |
| FFmpeg | macOS and iOS builds use the official FFmpeg 8.1.2 archive at SHA-256 `464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c`; non-Apple builds still require compatible pkg-config modules and are not yet pinned to this archive | `libavformat`, `libavcodec`, `libavutil`, `libswscale`, and `libswresample` are built as replaceable macOS shared libraries or target-architecture iOS static libraries with GPL, nonfree, version3, network, external autodetection, encoders, muxers, filters, devices, and hardware acceleration disabled; macOS packages include the exact source archive, upstream `LICENSE.md`, LGPL 2.1, and IJG attribution |
| GameNetworkingSockets | Included from immutable commit `4fbfe83ef4d59a12dc32baeff2c33e511af93157` | BSD-3-Clause; native UDP transport for desktop/mobile/server targets |
| protobuf | Included from immutable commit `35cd01f9fe9afbeea38cc7b979a3b6bfcde82c03` (35.1) | BSD-3-Clause; GNS wire-schema dependency; cross-builds require host protoc 35.1 |
| Abseil | Included from immutable commit `76bb24329e8bf5f39704eb10d21b9a80befa7c81` (20250512.1) | Apache-2.0; explicitly pinned protobuf dependency |
| OpenSSL / BCrypt | OpenSSL 3.5.7 LTS is pinned to commit `8cf17aaeb4599f8af87fefd810b5b5fee90fe69e`; Windows selects Windows SDK BCrypt | Non-Windows builds require an explicit target-architecture OpenSSL root; the checked-in builder is verified for macOS arm64 and iOS arm64 |
| Zstandard / LZ4 | Zstandard 1.5.7 is pinned for current binary place chunks; the small 2013 LZ4 snapshot remains active only for legacy place and bytecode compatibility | BSD-3-Clause / BSD-2-Clause; both are statically linked and packaged notices cover the exact sources |
| Luau | Included from immutable commit `5e76a0a162d82cbe13a09a4beacd09d7e53cc856` between upstream 0.729 and 0.730 tags | MIT; VM/compiler only, with upstream CLI, analysis, tests, and web targets disabled |
| Draco | Included from immutable 1.5.7 commit `8786740086a9f4d83f44aa83badfbea4dce7a1b5` | Apache-2.0; decoder supports current FileMesh v7 COREMESH payloads while encoders, plug-ins, transcoding, tests, and JavaScript glue are disabled |
| Boost | The active engine still uses the vendored 1.74.0 headers plus a scoped filesystem/iostreams/thread runtime archive | BSL-1.0; replacement with current standard-library or pinned upstream facilities remains incremental work |
| zlib | macOS uses SDK zlib 1.2.12 for curl and the scoped Boost runtime | Zlib; target-platform SBOMs must record their actual provider/version separately |
| SDL | Linux host only, pinned to SDL 3.4.10 commit `8e37db5e797b6167f3a00d697d816a684bd259c7` | Zlib-licensed window, native-surface, input, clipboard, drag/drop, and file-dialog adapter; Apple and Windows retain native hosts |

Package `otool -L` currently shows bundled, replaceable pinned FFmpeg libraries
plus Apple system frameworks and system C++/Objective-C runtimes. This is
build-graph evidence for the foundation bundle. The converted audio and VisualEngine-probe archives have no FMOD
middleware symbols, and the networking runtime archive has no RakNet, RakPeer,
SQLite-client-logger, or packetized-TCP symbols.
The preserved historical middleware trees are not evidence of completed Player
audio or networking integration.

## Client protection boundary

Default source builds compile the historical Win32/x86 memory-hashing,
Cheat Engine scanning, debugger polling, watchdog jobs, their state, and their
scheduler/callback call sites out together. They are not replaced by fake
scanner symbols or no-op job classes. The explicit
`RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY` vendor path hard-fails unless the target
is 32-bit MSVC/Windows and `RBX_ENABLE_VMPROTECT` names a genuine SDK header and
target import library; the historical implementations are then compiled from
source. Windows x64/ARM64 packages therefore do not claim that obsolete x86
anti-tamper behavior.

The former portable program-memory-checker substitute is absent from every
active CMake graph. Its retained tracked quarantine path contains only an
unconditional compile error, so adding it to a target fails instead of emitting
placeholder hashes, regions, or marker strings. `RBX_RCC_SECURITY` remains a
historical server-build boundary and is deliberately rejected if injected into
the modern Player networking target; modern CMake does not claim an RCC server
security configuration yet.

The portable Player still enforces its script security identities and
permission checks, verifies the private runtime-resource bundle against its
SHA-256 pin and internal manifest at build time, rejects unsafe/oversized or
CRC-corrupt RBXLP entries at runtime, and rejects unauthenticated non-loopback
GameNetworkingSockets peers. macOS packaging also performs strict signature
verification. These controls are distinct from client anti-tamper; production
Windows signing, hardened-runtime acceptance, and a current process-integrity
design remain explicit follow-up work.
