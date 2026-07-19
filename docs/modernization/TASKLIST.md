# Modernization task list

This is the persistent execution checklist. A checked item requires implementation and proportional verification; it does not mean “planned,” mocked, or compile-only. The verbatim user briefs remain in `EXECUTION_BRIEF_2026-07-17.md` and `NETWORKING_NOTE_2026-07-17.md`.

## 1. Genuine 2026 in-experience UI (active)

- [ ] Backport the supplied 2026 Player's actual CoreScripts/CorePackages UI and every native engine contract it depends on; do not reproduce screenshots or substitute mocks, examples, stubs, placeholders, or compatibility fakes.
- [ ] Complete Chrome/top bar, hamburger menu, current pause menu, bounded desktop layout, responsive scaling, Retina/Hi-DPI, density variants, safe areas, nine-slice behavior, authentic fonts, icons, cursor resources, transparency, focus, hover, pressed, touch, keyboard, and controller navigation.
- [ ] Complete the current People page/card grid, populated leaderboard, modern chat (not LegacyChatService), Report, Emotes, Respawn, Switch Avatar when applicable, Gallery/Captures, Help, and every supported current menu flow. Remove Music from the current UI.
- [ ] Fix Chrome disappearing when a feature is selected; verify ESC, mouse/touch clicks, Tab, keyboard, and controller interaction.
- [ ] Complete every pause-menu setting and its engine behavior, including shift lock, graphics/display, audio/device, language/chat, accessibility, camera, controls, leave, respawn, and resume.
- [ ] Add `Enable Legacy UI` in the current Settings page and preserve the authentic 2016 UI behind it. Do not remove legacy UI until the current UI is fully verified.
- [ ] Add semantic layout assertions, standard/Retina/wide/narrow/resize captures, perceptual goldens, interaction tests, and temporal UI tests using the real packaged player.

## 2. Camera, input, cursor, and gameplay

- [ ] Backport the current camera/zoom/input behavior from clean-room Player evidence: stable third-person orbit, first person, shift lock, sensitivity, inversion, correct zoom rate/limits, mouse capture, focus/hover boundaries, and no snapping or off-window tracking.
- [ ] Use the authentic modern keyboard/mouse cursors and correct platform-native visibility/locking without double cursors or center-warp artifacts.
- [ ] Verify movement, jumping, camera, interaction, respawn, input focus, window focus loss, resize, Retina, controller, touch, and web pointer-lock behavior through shared interfaces.

## 3. Rendering, assets, and lighting

- [ ] Keep the real `VisualEngine`/GfxRender player path on the single portable bgfx architecture; complete buffers, textures, render targets, passes, states, readback, loss/reset, capabilities, and safe resource lifetimes.
- [ ] Complete owned bgfx shader sources and reproducible shaderc builds for geometry/skinning, materials, terrain, water, particles, sky, UI/text, post effects, and every target backend.
- [ ] Fix modern place textures, studs/inlets, spawn texture, skybox, materials, meshes, content IDs, resource mount precedence, compression variants, sRGB, and missing-asset diagnostics without unrelated substitutes or production-service dependence.
- [ ] Fully backport current ShadowMap lighting behavior and visual output from authorized clean-room evidence, including shadow passes, cascades/atlases, filtering, bias, materials, terrain, characters, quality tiers, capabilities, and fallback policy; verify visual parity.
- [ ] Fix washed-out output, color-space/gamma issues, frame pacing, the observed ~15 FPS regression, render-thread stalls, and shutdown handle leaks using repeatable p50/p95/p99 measurements.
- [ ] Keep legacy renderer source until full verification and explicit user authorization to remove it. Do not use `modern` naming/feature macros such as `RBX_MODERN_GFXCORE` for the actual implementation.

## 4. Animation, avatars, and physics

- [ ] Complete authentic R6 loading, face/appearance, idle/walk/run/jump/fall/climb/swim/tool animations, transitions, stopped state, looping, and events.
- [ ] Complete authentic Studio-derived R15/R15-plus/Rthro hierarchy, attachments, joints/Motor6D, Humanoid semantics, scaling, collision, meshes, CPU pose/blending, GPU skinning, animations, transitions, and emotes.
- [ ] Test sparse/zero-duration sequences, priorities, fades, reverse/zero/negative speed, large delta/hitches, joint invalidation, keyframe/stopped events, bone palette limits, and deterministic simulation/render interpolation.
- [ ] Backport required ragdoll, animation-stream, avatar-context, avatar-editor, and creation contracts used by current CoreScripts and places.

## 5. 2026 place/model compatibility

- [ ] Build a versioned, bounds-checked compatibility layer for current RBXL/RBXLX/RBXM/RBXMX serialization, reflection classes/properties/enums, attributes, shared strings, references, chunks, compression, Luau, services, and runtime behavior while preserving older content.
- [ ] Analyze `2026-place-files` selectively, implement every missing real engine feature needed by the chosen fixtures through clean-room research, and preserve unknown data/diagnostics instead of silently guessing or discarding it.
- [ ] Use exactly one representative paired modern RBXL/RBXLX for final guarded load/simulate/render/gameplay/UI acceptance and document meaningful equivalence/export drift.
- [ ] Keep the current `/Users/q8j/Downloads/Baseplate.rbxlx` as the active development fixture without adding or rewriting the external reference in Git.
- [ ] Provide local/offline `.rbxl`, `.rbxlx`, `.rbxm`, and `.rbxmx` launch, CLI, picker, drag/drop/associations, recents, safe local player creation, sandboxing, and actionable unresolved-asset errors.

## 6. Audio replacement

- [ ] Remove FMOD from the default player and replace it with the owned portable open-source audio layer while preserving loading, streaming, cache, play/pause/stop, seeking, loops, completion, length/position, pitch/speed, volume, priority/voice stealing, buses, 3D listener/source transforms, attenuation, doppler, reverb, effects, device changes, and suspend/resume.
- [ ] Fix sped-up/high-pitched/broken sounds (including the volume-preview OOF), sample-rate conversion, channel layout, clocking, and pitch independence.
- [ ] Add allocation-free/nonblocking realtime behavior, deterministic offline mix tests, 3D tests, lifecycle tests, corrupt-file handling, and package/source scans proving FMOD absence.

## 7. Networking and server software

- [ ] Completely replace active RakNet client/server transport with pinned Valve GameNetworkingSockets while preserving reliable/unreliable ordered delivery, replication contracts, discovery/connection lifecycle, authentication/certificates, MTU/fragmentation, congestion, timeouts, reconnect, security limits, diagnostics, and deterministic local tests. Retain historical source until deletion is authorized.
- [ ] Adapt RCCService and relevant bundled game-server software to the same transport and prove full client/game-server replication on a current Ubuntu VPS deployment shape.
- [ ] Work fully offline and without Wi-Fi or a game-server connection now: local places, player creation, simulation, UI, assets, audio, and tests must not hang or fail merely because no network exists.
- [ ] Keep a clean endpoint/session boundary so a future real VPS and game-server connection can replace the offline/local session without rewriting replication, gameplay, serialization, or UI.
- [ ] Give Emscripten networking equal functional quality to native clients. Implement the browser-appropriate GameNetworkingSockets-compatible transport path (including WebRTC/WebSocket/browser security constraints as required) with equivalent replication semantics, reliability, ordering, reconnect, diagnostics, tests, and no silent feature downgrade.
- [ ] Preserve network capability sandboxing, offline defaults, no production impersonation/auth bypass, and telemetry disabled by default.

Verbatim networking instruction: see `NETWORKING_NOTE_2026-07-17.md` (preserved without normalization).

## 8. Cross-platform launcher and shells

- [ ] Port the authentic existing Durango/Xbox Roblox shell as the default game launcher, preserving its music, sounds, background, visuals, navigation, and controller behavior.
- [ ] Adapt the same launcher completely for desktop mouse/keyboard, mobile touch/safe areas, web, and Xbox controller/focus without turning it into the app/web Roblox GUI or replacing the in-experience UI.
- [ ] Keep shared engine APIs free of Cocoa/Metal, Win32/D3D, UIKit, JNI, X11/Wayland, and browser types; isolate native window, DPI, input, controller/touch, IME, clipboard, paths, timers, lifecycle, surface, audio, and networking adapters.

## 9. Build system, dependencies, organization, and quality

- [ ] Continue reorganizing the full top-level `src` tree—not only the backend—into clear apps/engine/platform/shaders/tools/tests/third_party/cmake/docs ownership using incremental moves and buildable checkpoints. Do not delete user or historical source unless explicitly authorized.
- [ ] Complete target-based CMake subsystem boundaries, public/private includes, cycle removal, generated-output separation, dependency-boundary enforcement, and platform-neutral `RobloxPlayer` naming.
- [ ] Pin and document bgfx/bx/bimg, GameNetworkingSockets, audio, text, image/compression, TLS/HTTP, physics, crash, and Luau dependencies with immutable versions, sources, licenses, patches, checksums, notices, and SBOM.
- [ ] Remove VMProtect and proprietary/obsolete default-runtime dependencies; keep Studio/installers/browser shells/console SDK projects out of the player dependency graph.
- [ ] Complete Unicode shaping/fallback (FreeType/HarfBuzz/ICU or justified equivalents), bounded glyph atlases, localization, bidi, CJK, Indic, combining marks, emoji/icons, wrapping, caret, selection, and malformed UTF-8 tests.
- [ ] Add unit, integration, rendering, animation, audio, networking, serializer, resource, text, platform-contract, security, fuzz, sanitizer, static-analysis, performance, sustained-run, reset/loss, lifecycle, packaging, and reproducibility tests without deleting valid tests to obtain green results.

## 10. Platform builds and final delivery

- [ ] Keep macOS arm64 Debug/Release locally buildable and produce/sign/validate the packaged `.app`; verify `file`, `codesign`, `otool`, resources, forbidden dependencies, direct/open launch, sustained gameplay, and clean teardown.
- [ ] Implement real headless verification against the packaged player with Metal, normal DataModel/VisualEngine/CoreScripts/resources, offscreen readback, semantic/perceptual/temporal artifacts, resize/DPI cases, animation, audio, UI, selected modern place, and failure rejection.
- [ ] Prepare and then use GitHub Actions for practical Windows x64/ARM, Linux x64/ARM, Android, iOS, Emscripten (WebGPU preferred), and Xbox/UWP APPX builds, preserving controller support and recording honest platform/device verification status.
- [ ] Only after the complete goal reaches final cross-platform work, load the PAT from the workspace `github.env` without printing or committing it, create a uniquely named private repository on `q8j-dev` (not `OpenRBLX`), push source/documentation, and run platform workflows. Do not attribute co-authorship to Codex/AI.
- [ ] Perform final source/package secret, absolute-path, production-endpoint, generated-artifact, reference-binary/place, license, dependency, and unrelated-user-change audits.
- [ ] Do not mark complete until every requirement in the original goal and subsequent verbatim briefs has implementation evidence, the user has performed requested external platform checks, and any legacy-renderer deletion has separate explicit approval.

## Locked verbatim execution brief — do not edit

The following user brief is included verbatim as part of this task list. It is
authoritative scope and must never be summarized, normalized, reordered, or
removed unless the user explicitly instructs that this file be changed.

```text
I'm going to sleep soon. Here is what you need to hear.

ISSUES:

- Textures/skybox are not working (on this baseplate, at least), the textures fall back to solid grey, and the skybox falls back to a solid blue. Fix this.
- The 2026 UI is still buggy. The Report and Switch Avatar icons in the hamburger menu have a grey background, instead of being the transparent white icons they're supposed to be.
- The 2026 esc/pause menu needs LOTS of work. It is supposed to look like the image attached, as that is the current state of it in the 2026 Roblox client.
- When moving the camera, my cursor snaps to the middle of the screen. Overall, the cursor system is buggy. 
- The current state of the UI doesn't scale the same way as the 2026 client, and is sometimes stretched. I want the whole scaling system from the 2026 client to be reverse engineered/decompiled, and implemented.
- The "ShadowMap" lighting technology is not implemented in this version of our client, but is in the latest 2026 one. I want this Engine Lighting Technology to be fully reverse engineered/decompiled, and implemented correctly for the games that use it.
- The leaderboard UI is mostly still broken, and has a weird, grey background, and is broken in most areas. Completely fix it.
- None of the features that are in the current 2026 In Experience UI have been implemented. Notably, "Chat", "Report", "Emotes", "Leaderboard (just needs more work), "Respawn", "Switch Avatar (when applicable)", and Music. Music should be completely removed from the UI and not implemented, as it is not useful. These are the main features that need to be implemented. Note that the Chat UI must be the modern UI from the 2026 client, not "LegacyChatService". Upon clicking the unsupported features, the entire "top bar/chrome" UI disappears, for an unknown reason. 
- Some sounds are incorrectly sped up, higher pitched, or outright broken. Example: the volume control example sound (OOF) is high pitched.
- Many esc menu features (such as shift-lock), do not work. As well as this, zooming in and out in-game is really fast and NOTHING like the 2026 client. I want you to decompile/reverse engineer the current client's zooming and overall camera system, so it is 1:1 to the 2026 client.

GOALS: 

- Complete 2026 UI working, completely, with a "Switch to Legacy UI" toggle in Settings (in the esc menu), that when enabled, switches back to the legacy UI (original 2016).
- Complete "Xbox UI", (already implemented in the Durango work) used as a launcher for games, with adapted controls for PC, mobile, and web. Keep the existing controller support (this will be explained later here). Must have completely working navigation, sound, and visuals.
- Complete ShadowMap implementation, from reverse engineered/decompiled 2026 client code. Nothing should be missing from this implementation. Ensure that it has 1:1 visual parity. (Do not open Roblox, or attempt to use the "Computer Use" tool for this. You should just be confident in the implementation that you know it is 1:1 via decompiled/reverse engineered code.
- Complete R15 implementation, with working emotes (if possible), and animations (required). 
- COMPLETE compatibility layer for 2026 RBXL/RBXLX files, and 2026 Roblox games. Analyze the features that "2026-place-files" use, and if something from there is missing in the current source, completely implement it by reverse engineering/decompiling the feature from the 2026 client. Most 2026 Roblox games/RBXL/RBXLXs should work as a result of this. Do not break <2026 games from working. In the future, I will add more RBXLs and RBXLXs for you to study, and expand the compatibility layer.
- Use my GitHub .env to create a PRIVATE repository for the project on my q8j-dev GitHub account. The old name "OpenRBLX" doesn't fit, as it is already being used by another project. Come up with a unique and creative name for this. Do NOT mention that you are the "co-author" of this project, or anything like that (in commits, README, etc). This GitHub repository will be useful, as you can utilize GitHub Actions to build for Windows (x64, ARM), Linux (x64, ARM), iOS, Android, Emscripten (WebGPU preferred) and Xbox (UWP, APPX ready for sideloading in Dev Mode on Series X/S). This is why we need to keep controller support, as the Xbox build will actively need it. Make sure the controller support works for the platforms mentioned (when possible). 
- I do not want to see any visual/audible bugginess when playing on the client. If something may introduce visual/audible bugginess, provide a solid implementation that fixes it.
- Adapt RCCService, and all Roblox server software bundled in the source to the new networking system (https://github.com/ValveSoftware/GameNetworkingSockets). Do not adapt server software that isn't relevant. The server software should be fully compatible with our modernized client, and work for everything that it originally does (full game-server networking). It should additionally work on a modern VPS, such as an Ubuntu VPS.
- Continue to reorganize stuff that hasn't already been organized, and modernize implementations the the client source uses. The same applies to server source.
- Anything else mentioned in the original goal that I haven't covered, I assume you will complete. If you don't remember, recap on the very original goal. 

NOTES:

- If you are adding/fixing/implementing something, do NOT make stubs, examples, placeholders, or fake it. This is against the rules.
- Save this prompt to a file, so you can recap on it when needed.
- For the other platforms (apart from macOS), I assume you know which renderer to pick, and be confident that it will work without any issues. I recommend double checking implementations, like in the renderer, so that you are sure it will fully work on other platforms, as intended.
- Do not ask any questions, or do anything that requires extensive permission (e.g. sudo).
- I will assume that you are capable of properly debugging this if something breaks, and understanding what I want from you.
- This is possible. It will take lots of work, but it's possible, so don't stop until you're fully done.

That's mostly all. In the future, I may have more work beyond this, but this is what you need to do now, so get started.
```

## Locked verbatim original goal — do not edit

The complete original goal is included verbatim below so this task list is
self-contained. It is authoritative scope and must never be summarized,
normalized, reordered, replaced, or removed.

```text
Modernize this circa-2016 Roblox source tree into a well-organized, cross-platform engine and player built around `bkaradzic/bgfx`, modern open-source dependencies, authentic supplied Roblox resources, modern place compatibility, and a clean portable architecture. The product is not a macOS-specific client. Windows, macOS, Linux, iOS, Android, and Web/Emscripten are all genuine target platforms and must share the same core renderer, animation, UI, audio, asset, scripting, serialization, and simulation systems.

macOS arm64 with bgfx Metal is only the first platform available for compilation, packaging, runtime testing, and final proof in this initial task. Use it to prove the shared cross-platform engine. Do not let Cocoa, Metal, `.app` packaging, or the current host dictate the engine architecture. Do not rename the product or primary target into a macOS-only client, make Apple-only code the default shared implementation, or introduce design decisions that must later be undone for another platform.

This is an implementation goal, not a planning exercise. Work persistently in buildable phases, make real code changes, run the available macOS builds and tests, diagnose root causes, and continue until the definition of done is satisfied or a genuinely external blocker remains. A dependency probe, colored triangle, standalone bgfx sample, fake UI, renderer skeleton, or compile-only library is not completion.

## Workspace and authoritative inputs

The workspace root is `/Users/q8j/Desktop/roblox-modernization`.

- `src/` is the actual Git repository containing the 2016-era source. Inspect and preserve pre-existing user changes. At the time this prompt was authored, `SECURITY.md` was already deleted and `.DS_Store` was untracked; do not restore, delete, or commit those incidentally.
- The former top-level `resources/` directory was intentionally deleted. Do not depend on it, recreate it, or fetch a replacement macOS bundle.
- `WindowsPlayer-version-ddf02245bdbb428c/` is a supplied current 2026 Windows Roblox Player snapshot. It is the primary modern player-resource and behavioral reference.
- `LIVE-WindowsStudio64-version-ed7d8193e8564b1f/` is a supplied current 2026 Windows Roblox Studio snapshot. It is authorized for clean-room research into engine behavior, reflection metadata, modern serialization, Studio-authored models, classes/properties/enums, UI/property metadata, import behavior, materials, terrain, animation, and authentic avatar rigs.
- `Roblox_2.730.790.apk` is a supplied current Android Roblox package, approximately 225 MB, with recorded SHA-256 `bcfa47ad89cacb265d9bc98fd97c2567f69ce59024b5a97096c7c0edc30d3040`. It is authorized for static clean-room inspection of mobile assets, touch controls, density variants, lifecycle expectations, mobile rendering capabilities, and related behavior.
- `2026-place-files/` is a modern reference library containing matching RBXL/RBXLX place pairs. Use it selectively for implementation research. Choose exactly one paired place for the final end-to-end acceptance scenario.
- `2016-place-files/` is a large mostly-legacy place reference library and a separate nested Git repository. Use files selectively for legacy behavior questions. Do not modify it or traverse its `.git` directory as fixture data.

All reference inputs remain outside the runtime source and final packages. Never rewrite them in place, add them to `src` Git history, or make binaries depend on their absolute paths. Hash every reference file actually used and record its provenance and purpose.

## Clean-room and safety rules

The user authorizes static inspection and reverse engineering of the supplied Player, Studio, and APK packages for interoperability, resource mapping, UI/format understanding, authentic rig implementation, mobile preservation, and general modernization.

- Allowed techniques include archive/resource inspection, metadata and manifest parsing, imports/exports, strings, file-format analysis, offline screenshots, structured model/plugin inspection, and clean-room behavioral comparison.
- Do not copy decompiled proprietary implementation code into the project.
- Do not defeat anti-tamper, recover secrets, bypass authentication, impersonate a client, contact production services unnecessarily, or invoke publishing, collaboration, telemetry, account, cloud, StudioMCP, or moderation systems.
- Do not install, execute, resign, upload, or modify the APK. Do not execute unknown Studio plugins merely to inspect them. Structurally parse packages first; run only selected fixtures in an isolated offline sandbox.
- Treat modern packed shaders as capability/behavior references, not reusable source code. Reauthor owned shaders and compile them through the bgfx toolchain.
- Audit asset provenance and licensing. A file being present in a supplied package does not automatically make every third-party binary or editor-only resource appropriate for redistribution.

## Core architecture observed in the legacy tree

- `Rendering/GfxCore` already abstracts devices, contexts, buffers, layouts, textures, framebuffers, shader programs, states, draws, caps, and resource lifetimes over D3D9, D3D11, and OpenGL.
- `Rendering/GfxRender` owns `VisualEngine`, scenes, render queues, materials, terrain clusters, lighting, water, particles, post effects, texture management/composition, text, UI adorns, and shader management.
- `shaders/source` contains old HLSL-like sources for skinned/default geometry, terrain, materials, water, particles, sky, UI, shadows, SSAO, and screen-space effects.
- Animation already includes `Animator`, `AnimationTrack`, `AnimationTrackState`, `KeyframeSequence`, `Pose`, Motor joints, priorities, fades, looping, speed/reverse playback, keyframe events, and GPU skinning hooks.
- The legacy macOS path is OpenGL/i386/macOS-10.6-era and currently not a reliable build.
- Existing `iOS/` and `Android/` trees contain mobile shells, lifecycle, touch/input, and OpenGL ES integration. Preserve their behavior and intent even though they are not built now.
- The old build is CMake 2.8/C++11-era, depends on `CONTRIB_PATH`, and mixes platform/global flags and obsolete project files.
- FMOD is deeply embedded in sound loading, streaming, playback, seeking, looping, completion, pitch, 3D listener/source behavior, reverb, groups, and DSP.
- Other dependencies include VMProtect, obsolete Qt/WebKit, XULRunner, CAB/DirectX SDK assumptions, old Mesa, hlsl2glsl/glsl-optimizer, libwww, curl, Boost, SDL2, Bullet, zlib, old VR SDKs, and Breakpad. Some are open source but obsolete; some are proprietary or redistribution hazards.

## Non-negotiable product outcomes

1. The renderer used by the real player is bgfx. High-level rendering contains no direct D3D, Vulkan, Metal, OpenGL, or WebGPU calls.
2. There is one shared render architecture for every target. macOS uses bgfx Metal now; future Windows, Linux, iOS, Android, and Web builds must not require a second renderer or a rewrite of `GfxRender`.
3. The real existing engine path is modernized. Do not substitute a separate sample application, new generic scene engine, or hand-authored “Roblox-style” demo for `GfxCore`, `GfxRender`, `VisualEngine`, DataModel, existing UI, animation, and content systems.
4. The final macOS proof is a packaged arm64 `.app` running the shared player through bgfx Metal, authentic player UI/resources, animation, audio, and exactly one selected modern RBXL/RBXLX place.
5. FMOD, VMProtect, and other proprietary default-runtime middleware are removed or replaced with suitable maintained open-source alternatives.
6. The codebase is substantially reorganized into clear, enforceable subsystem and platform boundaries, with target-level dependencies and documentation.
7. Authentic character animation works end to end, including CPU pose evaluation, blending, joint application, bone matrices, GPU skinning, transitions, and events.
8. R15 support is never faked. It is valid only when authentic Studio-supplied R15 structures, attachments, joints, Humanoid semantics, scaling, collision, meshes, skinning, and animations work.
9. Windows, Linux, iOS, Android, and Web/Emscripten remain explicit product goals. Although unbuilt now, the finished architecture must be credibly prepared and easy to bring up later without undoing the modernization.
10. Performance, stability, security, testing, licensing, packaging, and reproducibility are engineering deliverables, not afterthoughts.

## Permission to clone and build open-source dependencies

You are explicitly allowed to use network access to clone or fetch official open-source repositories needed for this work, including:

- `https://github.com/bkaradzic/bgfx`
- `https://github.com/bkaradzic/bx`
- `https://github.com/bkaradzic/bimg`
- `https://github.com/bkaradzic/bgfx.cmake`
- other justified maintained open-source dependencies and build tools required by the implementation.

Rules for fetched code:

- Use official upstream repositories or verified package sources.
- Pin every dependency to an immutable commit/tag and record the exact revision, license, source URL, patches, and reason for inclusion.
- It is acceptable to clone dependencies into a documented `third_party/` source location, a Git submodule layout, or a reproducible CMake dependency cache. Choose one coherent strategy. Do not rely on floating `main/master`, mutable latest releases, or unexplained prebuilt binaries.
- Ensure bgfx, bx, and bimg revisions are mutually compatible. If `bgfx.cmake` uses gitlinks/submodules, pin the wrapper commit and verify the transitive revisions.
- Keep dependency source separate from owned engine source and generated build output. Never edit vendored code silently; maintain explicit patch files or a small documented fork when unavoidable.
- Do not commit downloaded SDKs, build caches, toolchains, or generated artifacts.
- Prefer source builds for host tools and libraries. Verify checksums/signatures where upstream supplies them.
- Produce `THIRD_PARTY_NOTICES.md` and an SPDX or CycloneDX SBOM for the default player.

## Required codebase reorganization

Reorganize the player/runtime into a predictable structure. Do this incrementally with `git mv`, compatibility include shims where needed, buildable checkpoints, and tests; do not perform a blind mass move that leaves thousands of broken includes.

Target organization should converge toward this intent (adapt names when the repository requires a better fit, but preserve the boundaries):

```text
src/
  CMakeLists.txt
  apps/
    player/
    studio/                 # separately scoped; not required by the player
    tools/
  engine/
    core/
    reflection/
    serialization/
    datamodel/
    simulation/
    physics/
    animation/
    rendering/
      core/                 # portable RHI contracts and bgfx implementation
      scene/                # GfxRender/VisualEngine/render graph
      materials/
      text/
      ui/
    assets/
    audio/
    scripting/
    networking/
    diagnostics/
  platform/
    common/
    macos/
    windows/
    linux/
    ios/
    android/
    web/
  shaders/
    source/
    include/
    varying/
    manifests/
  tools/
    shader_pipeline/
    resource_importer/
    fixture_inspector/
  tests/
    unit/
    integration/
    rendering/
    fixtures/
  third_party/
  cmake/
  docs/
```

Reorganization requirements:

- First create `docs/modernization/architecture-before.md`, a module/dependency map, and `docs/modernization/layout-plan.md` mapping every major legacy directory to its destination or retained role.
- Define target dependency direction and enforce it. Applications depend on engine/platform adapters; platform adapters implement interfaces; high-level rendering depends on the portable RHI; engine core never depends on an app or OS UI.
- Break the monolithic runtime into named CMake targets such as core, reflection, serialization, DataModel, animation, render-core/bgfx, render-scene, UI/text, assets, audio, scripting, network, physics, platform host, and player.
- Eliminate cyclic target dependencies rather than hiding them with global include directories or whole-archive linkage.
- Give each module a clear public include surface and private implementation. Remove reliance on workspace-wide include paths, accidental transitive headers, and shared precompiled-header state.
- Put Objective-C++ only in macOS/iOS adapters, Java/JNI only in Android adapters, browser glue only in Web adapters, and Win32/X11/Wayland only in their platform adapters.
- Keep generated files under the build tree. Keep downloaded dependencies and generated shader/resource artifacts distinct from owned sources.
- Separate Player from Studio, installers, console SDK projects, old browser shells, and unrelated utilities. The player must not build Qt, XULRunner, CAB tools, Studio, or console integrations.
- Preserve history with moves where practical. Avoid duplicate “old” and “new” implementations that drift indefinitely. Temporary adapters must have an explicit removal milestone.
- Add `docs/modernization/architecture.md`, module ownership descriptions, dependency diagrams, naming rules, and instructions for adding a future platform backend.
- Add an automated dependency-boundary check using include analysis, CMake target rules, or a small owned tool.
- Do not create a permanent `legacy/` dumping ground. Classify and migrate player-critical code; quarantine truly obsolete projects with documented status.

## Phase 1: modern cross-platform build foundation

- Upgrade to target-based modern CMake and remove the mandatory mystery `CONTRIB_PATH` workflow from the new player.
- Use C++20 when practical, with C++17 as a temporary floor for difficult legacy modules. Avoid a repository-wide mechanical rewrite.
- The primary product target should have a platform-neutral name such as `RobloxPlayer`, not `MacClient` or `RobloxMac`.
- On this host, configure and exercise `macos-arm64-debug` and `macos-arm64-release` presets.
- The top-level design must permit future platform selection without rejecting non-Apple hosts structurally. Optional deferred presets/toolchain files for other targets are useful when inexpensive, but do not install their SDKs or claim they build.
- Preserve the old monolithic build behind an explicitly named migration option only where it helps comparison. Do not let the legacy option dictate new architecture.
- Add a thin platform contract for native window/display handles, events, DPI/density, input, controllers/touch, timers, filesystem paths, lifecycle, clipboard, threading, and graphics-surface recreation.
- Build the real player vertical slice early. A tiny bgfx probe may validate dependency/toolchain setup, but it must be a test/tool target, clearly non-product, and must not become the main implementation checkpoint.

## Phase 2: bgfx renderer migration

Use the existing GfxCore boundary as the migration seam:

- Add a bgfx device/resource backend under the organized portable rendering module or replace GfxCore internals behind equivalent contracts.
- Integrate bgfx into the actual `VisualEngine` path immediately after dependency validation. Do not build a parallel generic renderer and promise to connect it later.
- Map window/display handles through `bgfx::PlatformData` inside platform adapters.
- Own bgfx initialization, reset, frame submission, and shutdown exactly once. Reassess old OpenGL render-thread assumptions and avoid scheduler/UI/render deadlocks.
- Implement buffers/layouts, static/dynamic/transient geometry, textures/cubes/3D textures, updates/readback, formats/mips, renderbuffers/framebuffers, MSAA, clears/resolves/blits/discard, shader programs/uniforms/samplers, transforms, raster/blend/depth/stencil state, view IDs/pass order, scissor/viewport, debug markers, stats, screenshots, reset/loss, and safe deferred destruction.
- Use RAII bgfx handle wrappers, invalid-handle checks, debug names, and resource lifetime tests. Never retain transient memory across `bgfx::frame()`.
- Populate caps from `bgfx::Caps`, not OS guesses. Correctly handle clip/depth conventions, render-target origin, winding, sRGB, Retina/density, and texture compression availability.
- Define a documented pass/view order for shadows, opaque geometry, terrain, transparent geometry, particles, water, post effects, UI, and presentation.
- Port the real static/skinned meshes, materials, terrain, lighting, shadows, water, particles, sky, UI/adorns, text, texture composition, MSAA, and supported post effects.
- Legacy D3D/GL implementations may remain temporarily behind `RBX_ENABLE_LEGACY_RENDERERS=OFF`, but must not be linked into the final macOS app or remain the default architecture.

Do not invent a substitute “Roblox-style UI layer.” Temporary diagnostic overlays must be labeled as such and cannot satisfy UI acceptance. Final UI must use the existing player UI/DataModel/CoreScript path and authentic supplied Player resources.

## Phase 3: portable shader pipeline

- Replace the obsolete runtime shader compiler/pack dependency with pinned bgfx `shaderc` host tools and a reproducible CMake shader pipeline.
- Port the sources under the old `shaders/source` into owned bgfx shader source, shared includes, and `varying.def.sc` definitions.
- Preserve real material/feature variants without uncontrolled permutation growth.
- Compile and verify Metal shader artifacts for the initial macOS proof.
- Structure sources, macros, manifests, semantics, and capability variants so later D3D11/12, Vulkan/SPIR-V, GLSL/GLES, and Emscripten builds use the same shader source. Do not hard-code Metal behavior into high-level shader logic.
- Validate uniform names/types, sampler slots, vertex semantics, bone palettes, capability requirements, and missing-program failure behavior.
- Modern Player/Studio/APK packed shaders are reference-only.

## Phase 4: resources, UI, fonts, and mobile preservation

Build a versioned resource import/overlay system rather than copying reference trees into source:

- Generate a manifest with logical path, source corpus, hash, size, type metadata, license/provenance decision, compatibility status, conversion, platform variant, and package destination.
- Compare old content with Player resources and relevant shared Studio/APK resources. Classify duplicates, aliases, safe replacements, new supported assets, platform/editor-only assets, conversion candidates, and unsupported formats.
- Preserve legacy resources required by old places. Resolve `rbxasset://` through explicit mounts/precedence rather than destructive overwrite.
- Validate JSON/XML, images/color/alpha, font tables, meshes/models, sounds, Lua syntax expectations, path case/collisions, and compressed content.
- Integrate useful modern terrain material definitions, BRDF/material textures, water normals, authentic UI imagery, models, animations, sounds, and localization only with implemented semantics and documented fallback.
- Extract authentic style/color/font tokens from supplied resources and behavior. Do not redesign Roblox into a generic game, stock SDL interface, ImGui product UI, or unrelated modern aesthetic.
- Support DPI/density variants, nine-slice/cap insets, dark/light states where authentic, safe areas, touch/controller focus, and missing-asset diagnostics.
- Replace the fixed legacy font table with a manifest-driven registry using suitable supplied Builder/Source Sans/decorative/emoji/icon/non-Latin fonts when licensing permits.
- Use FreeType plus HarfBuzz and ICU/utf8proc as needed for shaping, bidi, segmentation, fallback, and Unicode correctness. Test Latin, CJK, Arabic/RTL, Indic/combining marks, emoji, icons, wrapping, alignment, caret/selection, and malformed UTF-8.
- Use bounded multi-page glyph atlases with eviction and batched uploads.

Preserve mobile support without installing Android tooling:

- Keep existing iOS/Android lifecycle, input, touch, orientation, safe-area, keyboard/IME, memory-pressure, suspend/resume, and graphics/audio surface semantics.
- Statically inspect the APK for authentic touch/D-pad/thumbstick/jump/action/gesture imagery, density variants, `Mobile.rbxl`, R15 mobile controller content, and Vulkan-mobile capability evidence.
- Import an APK asset only after provenance review and only when Player/Studio does not provide the same canonical resource.
- Keep mobile-only resources out of the macOS package unless shared code genuinely requires them.
- Shared UI/input must model touch hit targets, display cutouts, multi-touch, virtual controls, controller coexistence, and density without showing mobile controls incorrectly on desktop.

## Phase 5: open-source dependency replacement

Perform a real dependency/license/security audit and replacement table.

Recommended starting points, subject to verified fit:

- bgfx + bx + bimg for rendering/image support.
- SDL3 for portable window/input/controller/lifecycle support where native adapters are not preferable.
- miniaudio for replacing FMOD, with permissive custom DSP/effect nodes where required.
- FreeType + HarfBuzz + ICU/utf8proc for text.
- current libcurl and a current TLS implementation instead of libwww/obsolete TLS.
- current zlib/zstd/libpng/libjpeg-turbo/Ogg/Vorbis/Opus components as formats require.
- current Bullet for existing physics unless evidence supports another incremental choice.
- Crashpad or platform-native crash handling instead of obsolete Breakpad, disabled by default for local builds with no production endpoint.

For FMOD replacement, first introduce an engine-owned audio interface and preserve file/memory loading, async streaming, caching, play/pause/stop, seeking, looping, completion, length/position, pitch/speed, volume, priority/voice stealing, listener/source transforms and velocity, attenuation, doppler, groups/buses, suspend/resume/device change, reverb environments, and exposed effects. Keep the real-time callback allocation-free, nonblocking, and lock-safe. Add deterministic offline mix tests.

Remove FMOD headers/types/libs, VMProtect, and other proprietary default-runtime dependencies from the final player and prove absence with source/package scans and `otool -L`. Remove obsolete editor/installer/console dependencies from the player graph rather than unnecessarily deleting unrelated historical source.

## Phase 6: animation and authentic R15

Animation correctness is a release blocker:

- Test empty/sparse/zero-duration sequences, exact boundaries, interpolation, joint masks, priorities, fades, looping over large deltas, reverse/negative/zero speed, keyframe/stopped events, and hierarchy mutation.
- Make time advancement deterministic and resilient to hitches; separate simulation update from render interpolation correctly.
- Verify CPU poses through Motor/Motor6D-style joint application into GPU bone matrices and bgfx skinning.
- Define matrix conventions, bone palette layout, platform cap limits, large-rig splitting/fallback, missing-bone behavior, and resource ownership.
- Optimize the current per-step descendant joint discovery with invalidation-driven caching only after correctness tests and profiling.
- Use Studio's authentic R6, R15, R15-plus, Rthro, avatar-unification, rig-generator/migrator, validation, and animation resources as clean-room evidence.
- Never stretch/rename R6 into “R15,” use a generic humanoid, fabricate missing body parts, or count a placeholder as support.
- R15 is complete only when authentic hierarchy, attachments, joints, Humanoid behavior, scaling, collision, meshes, skinning, animations, and transitions work through normal serialization/runtime/rendering.

## Phase 7: modern place compatibility and offline launcher

- Create an authentic-looking offline player launcher supporting `.rbxl`, `.rbxlx`, `.rbxm`, and `.rbxmx` via file picker, drag/drop where appropriate, recent files, Finder association, and `--place` CLI.
- Run places without production login. Define safe local player creation, asset mounts, scripts, save policy, and explicit offline service fallbacks.
- Extend `SerializerV2`, `SerializerBinary`, reflection registration, and related loaders through versioned readers/adapters. Preserve old formats.
- Handle modern classes/properties/enums/attributes/shared strings/references/chunks with bounds, recursion, decompression, allocation, and corruption protections.
- Preserve unknown data for diagnostics/round-trip where safely possible; do not silently discard or guess unknown bytes.
- Use Studio reflection metadata/API docs to diff modern schema against the legacy registry, while verifying behavior through fixtures rather than assuming metadata proves implementation.
- Integrate open-source Luau or a compatibility layer if required and suitable. Structurally inspect scripts before execution; run permitted code in an offline sandbox with capabilities, instruction/time, memory, filesystem, and network limits.
- Resolve local/packaged assets; report unresolved remote asset IDs without silently contacting production services or substituting unrelated assets.

Place-library policy:

- All files in `2026-place-files/` and `2016-place-files/` are selective references, not an exhaustive test suite.
- Do not bulk parse, execute, render, or report on every place.
- Select exactly one representative paired modern place for final end-to-end testing. Record why it was selected.
- Treat its RBXL and RBXLX as two encodings of one scenario. Check meaningful equivalence and export drift.
- The selected place alone receives the detailed compatibility report and final guarded load/simulate/render/gameplay/UI test.
- Other files may answer focused implementation or regression questions but must not be represented as fully tested.

## Phase 8: cross-platform readiness without non-macOS builds

Do not install Android Studio/SDK/NDK/emulators, Emscripten, Vulkan SDKs, extra simulators, or other large optional platform toolchains. Do not configure, compile, link, package, run, emulate, or claim verification for Windows, Linux, iOS, Android, or Web in this initial task.

Nevertheless, these are real targets. Perform a source-level portability audit and fix avoidable structural blockers in modernized code:

- No Cocoa/Metal, Win32/D3D, UIKit, Android/JNI, X11/Wayland, or browser types in shared engine public interfaces.
- Explicitly handle byte order, pointer width, alignment, paths/case/Unicode, clocks, threads, filesystems, sockets, and dynamic libraries.
- Keep bgfx caps/backend selection and shader/resource manifests prepared for D3D11/12, Vulkan, Metal, GL/GLES, and Emscripten.
- Keep platform package layouts, texture compression families, density variants, touch/controller resources, and lifecycle differences modeled.
- Verify from upstream documentation/source that selected open-source dependencies advertise the intended targets.
- Preserve or map the existing iOS/Android implementations rather than deleting them for macOS convenience.
- Provide concise deferred build guides/toolchain notes for Windows, Linux, iOS, Android, and Emscripten: expected SDK/compiler/generator/architecture/backend/package steps and known blockers.
- The expected future work should be SDK installation, thin host glue, build adjustment, and device/browser testing—not replacing core engine systems.

Label future targets honestly as `architecturally prepared, not built or verified` or `needs follow-up`. Confidence must come from the dependency graph, interfaces, portable sources, and preserved platform behavior—not unsupported assurances.

## Phase 9: performance, stability, and security

- Establish repeatable benchmarks for CPU frame, render submission, GPU frame, presentation, animation, physics, streaming, audio, load time, draw/state counts, memory, and frame pacing.
- Report p50/p95/p99 and stutters, not only average FPS. Record hardware, resolution, scene, renderer, warmup, and run count.
- Profile before optimizing. Candidates include cached animation hierarchy, render sorting/batching/instancing, transient UI/particles, async decode/upload queues, bounded caches, texture compression/mips, allocation reduction, and bgfx encoders.
- Keep I/O/decode off render/audio threads and stage GPU work safely.
- Fix undefined behavior, races, lifetime errors, uninitialized state, overflow/narrowing, unsafe strings, and 32-bit assumptions exposed by arm64.
- Apply RAII, fixed-width serialized types, `nullptr`, `override`, move semantics, spans/views, standard synchronization, and warnings incrementally.
- Run ASan/UBSan on macOS and focused TSan tests where supported. Use Metal validation/debug markers.
- Fuzz modernized image, sound, mesh, model, XML/JSON, RBXL/RBXM, shader/resource manifest, and decompression loaders with strict resource limits.
- Handle corrupt/missing assets, unsupported caps, shader failure, zero-size windows, reset, out-of-memory, audio-device failure, offline network, suspend/resume, and shutdown with actionable errors.
- Keep telemetry external transmission disabled by default.

## macOS proof, packaging, and genuine headless rendering verification

Produce a reproducible arm64 `.app` from the platform-neutral `RobloxPlayer` target using bgfx Metal.

- Correct `Contents/MacOS`, `Contents/Resources`, Info.plist, icon, loader-relative/rpath dependencies, and resource/shader manifests.
- No source-tree, Homebrew, or reference-corpus absolute runtime paths.
- Ad-hoc sign locally if no Developer ID exists; do not claim notarization without a real successful result.
- Verify with `file`, `codesign --verify --deep --strict`, `otool -L`, resource validation, and forbidden dependency scans.
- Launch from `open` and directly, exercise Retina/resize/input/audio/animation/sustained rendering/clean quit, and capture logs.

Implement a noninteractive `macos-headless-verify` command against the packaged app:

- It must use the actual shared player, GfxCore/GfxRender/VisualEngine, authentic UI, resource mounts, animation, and selected place—not a mock, Noop backend, sample renderer, or substitute UI.
- Require a real Metal device and assert bgfx selected Metal. OpenGL, Noop, or placeholder fallback fails.
- Render to offscreen targets and read back results. A hidden NSWindow/CAMetalLayer is acceptable only if bgfx requires it; the test remains unattended.
- Load resources from inside the `.app` and open both encodings of the one selected modern place through the normal launcher/serializer/runtime path.
- Capture standard/Retina, wide/narrow, and resize cases.
- Exercise all supported/mapped core player UI states relevant to the selected scenario: loading/offline/error, HUD/top bar, menus/settings, chat, player list, backpack/inventory, dialogs/notifications, text input, focus/hover/pressed/disabled, and debug overlays where implemented.
- Assert semantic UI tree/layout plus perceptual golden comparisons: bounds, anchors, clipping, z-order, nine-slice, opacity, authentic colors/icons/fonts, wrapping, fallback, safe areas, and missing assets.
- Exercise static/skinned geometry, terrain/materials, lighting/shadows, sky, water, particles, transparency, post effects, UI, and animation.
- Capture a temporal sequence proving animation progression, blending, loops, reverse/speed, water/UI transitions, and no frozen-after-first-frame behavior.
- Reject blank/solid/transparent frames, NaNs, shader error colors, missing critical textures, clipped controls, and nondeterminism.
- Save PNGs, diffs/heatmaps, structured layout results, logs, and machine-readable summary with actionable failure paths.
- Run a sustained offscreen test and verify clean bgfx/resource teardown with no leaks.
- Golden images may be updated only after deliberate reference review, never blindly to accept broken output.

A successful compile, process exit, clear color, triangle, diagnostic overlay, or unreviewed screenshot is insufficient.

## Testing and CI

- Unit tests: renderer mappings/lifetimes, shader manifests, resource overlay, text shaping/fallback, audio behavior, animation boundaries/blending, serialization versions/types, unknown preservation, malformed input, and dependency boundaries.
- Integration tests: real Metal offscreen player/UI, selected place launcher/load/simulation, animation, resource overlay, offline audio mix, resize/lifecycle, and packaged resources.
- Perceptual render tests with documented tolerances for static/skinned geometry, terrain, water, particles, text, UI, and representative transitions.
- macOS arm64 is the only active CI/build platform for this initial task. Do not create SDK-downloading jobs for deferred targets.
- Never delete or weaken valid tests to make CI green. Quarantine obsolete tests only with a reason and replacement coverage.

## Definition of done

Do not mark this goal complete until:

- The reorganized source tree and CMake target graph have clear subsystem/platform ownership, documented dependencies, and no unexplained cycles or global include soup for the new player.
- Official pinned bgfx/bx/bimg integration builds reproducibly from documented source revisions.
- The real player uses bgfx through the portable renderer; no parallel sample renderer substitutes for engine integration.
- The owned portable shader sources rebuild verified Metal artifacts.
- Authentic Player resources, UI, fonts, text, materials, and resource overlays function through the real runtime.
- FMOD, VMProtect, and other identified proprietary default-runtime dependencies are absent from the new player and final `.app`.
- Audio replacement behavior is tested, including offline mix and 3D semantics.
- Animation and authentic R15 tests prove the complete CPU-to-GPU path without fake rigs.
- The offline launcher supports local place/model formats and the one selected RBXL/RBXLX place passes its documented compatibility scenario.
- The arm64 macOS `.app` builds, signs/validates locally, launches, selects bgfx Metal, renders authentic 3D/UI/animation, runs sustained, and quits cleanly.
- `macos-headless-verify` passes against the packaged app with real Metal, the selected place, proper UI, semantic assertions, perceptual and temporal captures, and diagnostic artifacts.
- Performance before/after results, sanitizer/static analysis, security hardening, SBOM/licenses, architecture/layout documentation, and reproduction commands are present.
- Windows, Linux, iOS, Android, and Web/Emscripten each have a source-backed readiness assessment and deferred build guide. They remain explicitly unbuilt/unverified but have no known avoidable core-architecture blocker, or the exact remaining follow-up is documented.
- Existing mobile support has not been casually removed, and APK-derived mobile findings are documented without installing Android tooling.
- The final Git diff contains no generated builds, dependency caches, reference binaries/places, secrets, production endpoints, unrelated user reversions, or unexplained large files.

## Working and reporting rules

- Maintain `docs/modernization/STATUS.md` with current phase, completed evidence, blockers, and next buildable checkpoint.
- Lead with outcomes and keep the tree buildable at phase boundaries.
- Do not paper over problems with swallowed exceptions, disabled assertions, arbitrary sleeps, forced software fallback, blanket warning suppression, or fake feature flags.
- When binary-derived evidence is uncertain, label it as inference and record file hash, tool, observation, and conclusion in `docs/modernization/reference-research.md`.
- Ask the user only for credentials, unavailable required Apple tooling, ambiguous licensing/provenance, or a product choice that materially changes scope. Continue all independent safe work first.
- Do not install Android Studio/SDK/NDK/emulators, Emscripten, Vulkan SDKs, extra simulators, or other large optional platform toolchains without explicit later authorization.
- Final handoff must state implemented outcomes, macOS build/test results, artifact path, performance evidence, selected place status, known limitations, organized architecture, dependency revisions, and exact reproduction commands. Clearly label every non-macOS target unbuilt/unverified while explaining why the architecture is prepared for it.
```

## Locked verbatim networking instruction — do not edit

The complete networking follow-up is included verbatim below and is
authoritative scope. It must never be summarized, normalized, reordered,
replaced, or removed.

```text
Also ensure that networking fully works on Emscripten (for the future). Don't have Emscripten be worse. It should be equally as good as the other platform clients.

I will buy a VPS for this in the future (KEYWORD future), so don't make this not work without Wi-Fi, or a gameserver connection. It should work offline for now but BE ready for a real server connection migration in the future. Add these notes to the tasklist, and note it in a file (preferably verbatim).
```
