# Deferred platform readiness

The target matrix below is still incomplete. The full Player, native networking,
audio, Metal shaders, resources, and compliance payload now cross-compile and
link into an unsigned iPhoneOS arm64 application bundle. Native Windows and
Linux shared-engine workflows exercise both x64 and arm64, but their initial
bring-up runs are still being repaired and are not acceptance evidence. No
non-macOS target has completed real-device or distribution acceptance.

The shared `Host` and renderer interfaces contain no Cocoa, Win32, UIKit, JNI,
X11/Wayland, browser, Metal, D3D, Vulkan, GL, or WebGPU public types. Native
handles use `uintptr_t`; bgfx performs API translation. Paths use
`std::filesystem`, dimensions use fixed-width types, and density is explicit.
The shared host contract now also owns local-document selection, queued
application-open events, and confirmed document-session launch without
exposing a native picker or process type. AppKit, Win32, and SDL3 adapters now
implement that desktop contract. The same neutral event vocabulary carries up
to eight gamepads into `UserInputService` and `GamepadService`; AppKit uses
GameController, Windows uses XInput, and Linux uses SDL3 mappings.
The root build selects the AppKit adapter only for Darwin and the UIKit adapter
only for iOS. The iOS host provides a UIKit application lifecycle, CAMetalLayer
surface, touch and controller input, clipboard, document picker, safe area,
resource paths, and writable application storage without exposing UIKit types
through the shared contract.

| Target | Expected bring-up | Current follow-up |
|---|---|---|
| Windows x64/arm64 | VS 2022+, Windows SDK, Ninja/MSVC; the production Win32 host owns DPI-aware windowing, input/raw pointer lock, XInput controllers, clipboard, picker, drag/drop, fresh document launch, and the portable runtime; select bgfx D3D11 initially and D3D12 after parity; package logical resources beside executable | Complete and verify the D3D11 Player link/package in both native CI architectures |
| Linux x64/arm64 | GCC/Ninja, pinned SDL3 with X11/Wayland, native surfaces, input/relative lock, mapped controllers, clipboard, picker, drag/drop, document launch, XDG paths, and the portable runtime; bgfx Vulkan with GL fallback only when explicitly supported | Complete and verify the Vulkan Player link/package and case-sensitive resource mounts in both native CI architectures |
| iOS arm64 | Xcode/iOS SDK; UIKit lifecycle, touch, safe area, document picker, clipboard and GameController adapter; bgfx Metal; package resources in app bundle | Sign and run on physical hardware; verify lifecycle suspend/resume, touch, audio interruption, memory pressure, text/IME and GPU surface recreation |
| Android arm64/x86_64 | Android SDK/NDK and Gradle only in a later authorized task; preserve JNI lifecycle/touch/surface recreation; bgfx Vulkan/GLES capability selection; density-specific assets | Reconcile old Android shell, ANativeWindow ownership, audio focus and memory pressure |
| Web/Emscripten | Emscripten SDK, CMake/Ninja; browser host glue under `platform/web`; bgfx WebGPU with WebGL fallback policy; preload versioned resource package | Define persistent storage, threading/COOP-COEP, browser IME and asset streaming limits |

Networking uses an engine-owned transport boundary. Windows selects the GNS
BCrypt backend; Darwin, Linux, and Android use pinned OpenSSL 3.5.7 LTS, with
builds required to name a target-architecture OpenSSL installation and a host protoc
35.1 executable. The pinned upstream CMake is guarded-patched for its iOS and
Android source branches. OpenSSL 3.5.7, the GNS static library, and the owned
transport adapter and full Player/DataModel graph have been cross-compiled and
linked locally for iPhoneOS arm64; signing and a physical-device runtime test
remain. Windows, Linux, and Android still need real toolchain verification. Web must implement
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

Shader source and manifests compile by backend profile. The complete 137-entry
scene corpus now compiles into owned bgfx GLSL, Metal, SPIR-V, and WGSL packs;
the same HLSL-source pipeline emits the D3D11 pack with Windows shaderc. Player
bootstrap selects Metal on Apple, D3D11 on Windows, Vulkan on Linux/Android,
and WebGPU on Emscripten instead of hard-coding the macOS renderer. Cross builds
require an explicit native host shaderc and do not attempt to execute a target
architecture tool. Package texture-compression selection,
surface recreation, controller/touch coexistence, endian/alignment serializer
audits, and real device/browser testing remain follow-up work.
