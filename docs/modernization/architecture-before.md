# Architecture before modernization

## Baseline

The historical runtime is assembled by a CMake 2.8 root into one `roblox`
shared library from object libraries. Configuration requires an undocumented
`CONTRIB_PATH`, global include paths, global compiler flags, and caller-supplied
standard-library paths. The root rejects non-Unix hosts even though Windows
projects live in the same tree. FMOD is linked directly at the root.

The player-critical dependency flow observed in source is:

```text
Mac / WindowsClient / iOS / Android shells
  -> ClientShared / ClientBase
  -> App (DataModel, reflection, serialization, animation, scripting, simulation)
  -> Rendering/GfxRender (VisualEngine, scene, materials, terrain, UI/text)
  -> Rendering/GfxCore (device/resource contract)
  -> D3D9 | D3D11 | OpenGL platform implementations
```

`GfxCore` is the narrowest viable renderer migration seam. Its public surface
owns device/context lifetime, buffers/layouts, textures, framebuffers, shaders,
states, draws, caps, and frame submission. `GfxRender::VisualEngine` consumes
that surface. The old factory includes OpenGL unconditionally and selects D3D
with preprocessor branches; it has no backend-neutral native-surface contract.

## Major coupling and risks

| Area | Baseline coupling | Migration consequence |
|---|---|---|
| Build | Global flags/includes and object-library aggregation | Create target-scoped module libraries and explicit public surfaces |
| Rendering | OS/API implementations compiled beside contracts | Put bgfx behind the portable contract; isolate native handles in hosts |
| Player/Studio | Product and tool trees share broad libraries | Establish a player-only graph; Studio remains separately scoped |
| Audio | FMOD types and binaries appear throughout runtime | Introduce an owned audio contract before replacing call sites |
| Text | Vendored historical FreeType and fixed font assumptions | Move to manifest-driven shaping/fallback with maintained libraries |
| Mobile | JNI/Objective-C lifecycle and GL integration | Preserve behavior in adapters; remove platform types from shared headers |
| Dependencies | `Library/` mixes source, SDKs, obsolete and proprietary code | Pin maintained source dependencies and audit redistribution |

This document describes source relationships, not a claim that the historical
build configures or runs on current macOS.
