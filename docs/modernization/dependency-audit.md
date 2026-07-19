# Dependency modernization audit

The named engine graph now includes bgfx/bx/bimg, miniaudio,
GameNetworkingSockets, protobuf, Abseil, and the platform crypto provider. It
does not link the historical FMOD, RakNet, VMProtect, Qt, XULRunner, libwww,
CAB/DirectX SDK, Mesa, old VR SDK, or Breakpad trees. The foundation Player
executable still does not link every named engine archive, so this statement is
about the active engine build graph, not completed Player integration.

| Historical dependency | Default player decision | Replacement / state |
|---|---|---|
| D3D9/D3D11/OpenGL implementations | Disabled (`RBX_ENABLE_LEGACY_RENDERERS=OFF`) | bgfx backend attached to GfxCore; resource parity incomplete |
| FMOD | Converted audio/DataModel graph no longer includes or references it; quarantined historical files retained | Pinned miniaudio 0.11.25 runtime passes deterministic decode/playback/control/lifetime/streaming/3D/region/bus/effect tests plus a live CoreAudio device smoke test; the complete AudioPlayer/AudioEmitter/AudioListener graph and real Player acceptance remain |
| RakNet | Removed from the active CMake runtime, public replication contracts, and Windows debug network profiler; historical source retained | Pinned GameNetworkingSockets 1.5.0 native transport plus owned packet/endpoint/handler contracts and local diagnostic logging; unauthenticated remote IP peers are rejected, certificate provisioning is exposed, and full certificate lifecycle, Replicator client/server acceptance, and browser WebRTC adapter remain |
| VMProtect | Excluded | No replacement; not appropriate for portable default runtime |
| Qt/WebKit/XULRunner | Excluded with Studio/browser tooling | Native/portable host and existing CoreScript UI path |
| libwww/old curl/TLS | Excluded | Current curl/TLS only when networking module migrates; offline default |
| historical FreeType | Not in new graph | Maintained FreeType/HarfBuzz plus Unicode support planned |
| historical Bullet | Not in new graph | Current Bullet adapter planned after simulation target split |
| Breakpad | Excluded | Crashpad or native crash handling later, opt-in and no endpoint by default |
| bgfx/bx/bimg | Included from official immutable commits | BSD-2-Clause; exact pins in `RbxDependencies.cmake` |
| GameNetworkingSockets | Included from immutable commit `4fbfe83ef4d59a12dc32baeff2c33e511af93157` | BSD-3-Clause; native UDP transport for desktop/mobile/server targets |
| protobuf | Included from immutable commit `35cd01f9fe9afbeea38cc7b979a3b6bfcde82c03` (35.1) | BSD-3-Clause; GNS wire-schema dependency; cross-builds require host protoc 35.1 |
| Abseil | Included from immutable commit `76bb24329e8bf5f39704eb10d21b9a80befa7c81` (20250512.1) | Apache-2.0; explicitly pinned protobuf dependency |
| OpenSSL / BCrypt | OpenSSL 3.5.7 LTS is pinned to commit `8cf17aaeb4599f8af87fefd810b5b5fee90fe69e`; Windows selects Windows SDK BCrypt | Non-Windows builds require an explicit target-architecture OpenSSL root; the checked-in builder is verified for macOS arm64 and iOS arm64 |

Package `otool -L` currently shows Apple system frameworks and system C++/
Objective-C runtimes only. This is build-graph evidence for the foundation
bundle. The converted audio and VisualEngine-probe archives have no FMOD
middleware symbols, and the networking runtime archive has no RakNet, RakPeer,
SQLite-client-logger, or packetized-TCP symbols.
The preserved historical middleware trees are not evidence of completed Player
audio or networking integration.
