# Shared player architecture

`RobloxPlayer` is a platform-neutral product target. macOS is the first host,
not the architecture. The target depends on owned engine contracts and one host
adapter selected by CMake. High-level rendering will continue to be the real
`GfxRender::VisualEngine`; migration replaces its GfxCore implementation rather
than adding a second scene engine.

## Current target graph

```text
RobloxPlayer
  +-- rbx-core
  +-- rbx-render-core -- bgfx (pinned source build)
  |                    +-- bx
  |                    +-- bimg
  +-- rbx-platform-macos -- AppKit / QuartzCore
       +-- rbx-platform-contract
```

`rbx-render-core` owns bgfx initialization, reset, frame submission, caps, and
shutdown exactly once. A host returns a portable `NativeSurface`; only the bgfx
implementation translates it to `bgfx::PlatformData`. No Metal types appear in
shared public interfaces. Backend selection is data-driven and already names
D3D11, D3D12, Metal, Vulkan, GL/GLES, and WebGPU.

## Ownership rules

- Public headers live below each module's `include/` directory. Source-private
  headers stay beside implementation.
- Targets expose only their own include roots and direct dependencies.
- Shared engine headers may not include OS windowing or graphics API headers.
- Generated shaders/resources live below the build directory.
- Reference corpora are research inputs only and never runtime mounts.
- Studio, installers, browser shells, and console integrations are absent from
  the default player graph.

## Adding a platform host

Implement `rbx::platform::Host` under `platform/<name>`, translate the platform's
window/lifecycle/input concepts there, and export the `Roblox::PlatformHost`
alias. Do not add native types to `Host.h`. Package resources through logical
mounts and implement surface loss/recreation in the adapter. Add a deferred
toolchain preset only when that platform's SDK is available and verification is
authorized.

## Migration milestones

The current player proves pinned bgfx can be built and that the portable host
and renderer ownership compile. It does **not** yet count as the required real
player vertical slice: `VisualEngine`, DataModel, authentic resources/UI,
serialization, animation, and audio must be moved behind these targets before
acceptance.
