# Deferred platform readiness

The target matrix below is still incomplete. The native networking dependency
and owned transport adapter have been cross-compiled for iPhoneOS arm64; full
Player targets and the other deferred platforms are not yet built or verified.

The shared `Host` and renderer interfaces contain no Cocoa, Win32, UIKit, JNI,
X11/Wayland, browser, Metal, D3D, Vulkan, GL, or WebGPU public types. Native
handles use `uintptr_t`; bgfx performs API translation. Paths use
`std::filesystem`, dimensions use fixed-width types, and density is explicit.
The shared host contract now also owns local-document selection, queued
application-open events, and confirmed document-session launch without
exposing a native picker or process type. The macOS adapter implements the
contract with NSOpenPanel, NSApplicationDelegate, and NSTask; equivalent
Windows, Linux, mobile, and browser adapters remain required before their
launcher builds can claim local-file parity.

| Target | Expected bring-up | Current follow-up |
|---|---|---|
| Windows x64/arm64 | VS 2022+, Windows SDK, Ninja/MSVC; implement Win32/SDL3 host; select bgfx D3D11 initially and D3D12 after parity; package logical resources beside executable | Migrate historical input/window behavior and add CI only when authorized |
| Linux x64/arm64 | Clang/GCC, Ninja, X11 or Wayland development packages; implement SDL3/native host; select bgfx Vulkan with GL fallback only when explicitly supported | Decide X11/Wayland policy and test case-sensitive resource mounts |
| iOS arm64 | Xcode/iOS SDK; preserve existing lifecycle/touch/IME/safe-area intent in Objective-C++ adapter; bgfx Metal; package resources in app bundle | Map existing iOS shell without sharing AppKit implementation |
| Android arm64/x86_64 | Android SDK/NDK and Gradle only in a later authorized task; preserve JNI lifecycle/touch/surface recreation; bgfx Vulkan/GLES capability selection; density-specific assets | Reconcile old Android shell, ANativeWindow ownership, audio focus and memory pressure |
| Web/Emscripten | Emscripten SDK, CMake/Ninja; browser host glue under `platform/web`; bgfx WebGPU with WebGL fallback policy; preload versioned resource package | Define persistent storage, threading/COOP-COEP, browser IME and asset streaming limits |

Networking uses an engine-owned transport boundary. Windows selects the GNS
BCrypt backend; Darwin, Linux, and Android use pinned OpenSSL 3.5.7 LTS, with
builds required to name a target-architecture OpenSSL installation and a host protoc
35.1 executable. The pinned upstream CMake is guarded-patched for its iOS and
Android source branches. OpenSSL 3.5.7, the GNS static library, and the owned
transport adapter have been cross-compiled locally for iPhoneOS arm64; a full
Player/DataModel iOS link and runtime test remain. Windows, Linux, and Android
still need real toolchain verification. Web must implement
the same transport interface over WebRTC because browser sandboxes do not
provide native UDP sockets. Console adapters require each platform SDK and any
non-public GNS platform source supplied under its terms.

The native adapter overrides the open-source GNS unauthenticated-IP default:
remote IP peers must authenticate, while loopback is permitted for deterministic
offline tests. It accepts coordinator-issued GNS certificate blobs and reports
peer authentication state. Certificate issuance, secure delivery, and rotation
belong to the future production backend integration and are not yet accepted.
Hostname resolution runs away from the engine thread, keeps a stable logical
connection ID, and retries each unique IPv4/IPv6 result sequentially.

Shader source and manifests compile by backend profile: SPIR-V, GLSL, GLES, and
Metal are emitted on the current host; D3D profiles are structurally supported
by the same `shaderc` pipeline on Windows. Package texture-compression selection,
surface recreation, controller/touch coexistence, endian/alignment serializer
audits, and real device/browser testing remain follow-up work.
