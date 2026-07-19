# Repository Guidelines

## Purpose and governing rules

This repository is the circa-2016 Roblox source modernization project. Its end product is a well-organized, portable Roblox engine, Player, launcher, and relevant server stack using the real engine paths, authentic supplied resources, current place compatibility, and maintained open-source dependencies. It is not a macOS-only fork: macOS arm64 is the locally testable proof platform, while Windows, Linux, iOS, Android, Web/Emscripten, and Xbox remain genuine product targets sharing the same renderer, simulation, animation, UI, audio, serialization, scripting, assets, and networking contracts.

This is an implementation project, not a mock-up or planning exercise. Never add compatibility mocks, screenshot recreations, placeholders, examples, stubs, fake feature implementations, silent fallbacks, swallowed CoreScript failures, or unrelated asset substitutions. “Present,” “compiles,” or “opens” does not mean complete. A feature is complete only when its normal production path works and proportional semantic, behavioral, visual, audible, performance, and packaging verification passes.

Do not use `modern` as a parallel-code label or macro (for example, do not create `RBX_MODERN_GFXCORE`). Improve the actual implementation. Preserve the authentic 2016 UI and historical renderers until all replacement work is fully verified and the user separately authorizes deletion. Do not delete source, resources, tests, or user changes without explicit authorization.

## Authoritative project records

Every contributor or agent must completely read these sources before taking over work and after every context compaction:

- The current goal objective attachment: `/Users/q8j/.codex/attachments/6b234a17-ec05-4835-9638-e08e28680b3b/goal-objective.md`.
- The original goal: `/Users/q8j/.codex/attachments/38be2886-c30f-402e-8892-2173fe26fcf0/pasted-text-1.txt`.
- The preserved execution brief: `docs/modernization/EXECUTION_BRIEF_2026-07-17.md`.
- The implementation record: `docs/modernization/STATUS.md`.
- The persistent checklist: `docs/modernization/TASKLIST.md`.
- Reverse-engineering evidence: `docs/modernization/reference-research.md` and `docs/reverse-engineering/in-experience-ui.md`.

`docs/modernization/TASKLIST.md` is immutable until every item is complete. Do not edit, unlock, reorder, shorten, or regenerate it. Its expected SHA-256 is `a1c37bf3cb74388be01a0e8580e458b75a05b20420919874f7294ff88df65da6`. `STATUS.md` is the evidence ledger and may be extended only with truthful outcomes, exact tests, limitations, and the next buildable checkpoint.

## Workspace and reference corpora

The workspace root is `/Users/q8j/Desktop/roblox-modernization`; `src/` is the actual Git repository. The parent workspace also contains:

- `WindowsPlayer-version-ddf02245bdbb428c/`: the supplied current 2026 Windows Player, primary source of authentic Player resources and behavioral/static clean-room evidence.
- `LIVE-WindowsStudio64-version-ed7d8193e8564b1f/`: the supplied 2026 Studio, used only for engine/reflection/schema, authentic rig, model, animation, and serialization research. Studio is not the source of the in-experience Player UI.
- `Roblox_2.730.790.apk`: the supplied Android package, SHA-256 `bcfa47ad89cacb265d9bc98fd97c2567f69ce59024b5a97096c7c0edc30d3040`, for static mobile asset, density, lifecycle, touch, input, and renderer-capability research. Do not install, execute, resign, publish, or modify it.
- `2026-place-files/`: modern paired RBXL/RBXLX reference fixtures. Inspect selectively; exactly one representative pair is the final acceptance scenario.
- `2016-place-files/`: legacy compatibility references and a separate nested Git repository. Do not modify it or traverse its `.git` as fixture content.
- `/Users/q8j/Downloads/Baseplate.rbxlx`: the active Studio-2026 development place. Never rewrite or commit it.
- `github.env`: contains the user’s GitHub credential. Never print, log, commit, or load it before final cross-platform repository work.
- `ROBLOX_CROSS_PLATFORM_MODERNIZATION_GOAL.md`: a workspace-level project brief; it is reference material, not runtime content.

Reference packages must stay outside source history and final packages. Hash each file actually used and record corpus, purpose, inspection method, observation, and conclusion. Static inspection, strings, imports/exports, manifests, resource parsing, model/plugin structure, and clean-room comparison are allowed. Never copy decompiled proprietary implementation code, defeat authentication or anti-tamper, recover secrets, contact production services unnecessarily, execute unknown plugins, or make runtime code depend on absolute reference paths.

## Project structure and dependency direction

- `apps/player/` owns the platform-neutral Player entry point, packaging, and thin host-specific launch code. Historical macOS and Windows shells are organized beneath their platform owners.
- `apps/studio/`, `apps/services/`, and related application folders contain Studio, RCCService, arbiter, upload, and historical application projects. They must not leak into the default Player graph.
- `engine/core/`, `reflection/`, `serialization/`, `datamodel/`, `simulation/`, `physics/`, `terrain/`, `geometry/`, `avatar/`, `animation/`, `rendering/`, `assets/`, `audio/`, `scripting/`, `networking/`, `security/`, and `ui/` own shared runtime subsystems.
- `engine/rendering/core/` is the portable GfxCore/RHI contract and bgfx implementation. Scene, materials, text, and UI build above it. High-level code must contain no direct Metal, D3D, Vulkan, OpenGL, or WebGPU calls.
- `platform/` owns native windows, display handles, DPI/safe area, events, input, controllers/touch, IME, clipboard, paths, lifecycle, timers, audio devices, graphics surface recreation, and platform transport adapters. OS types must not appear in shared engine public interfaces.
- `shaders/` owns reauthored portable shader sources, includes, varying definitions, manifests, and generated-artifact rules. Supplied packed shaders are evidence only.
- `content/` and `PlatformContent/` contain preserved runtime assets. Manifest and mount precedence determine logical resolution; never destructively overwrite legacy content.
- `tests/` contains unit, integration, rendering, packaged-player, contract, and fixture tests. `tools/` contains owned inspection, migration, shader, resource, lint, and verification utilities.
- `third_party/` contains pinned maintained dependencies or clearly labeled preserved historical snapshots. `out/` alone owns builds, generated overlays, research extraction, captures, dependency caches, and test output.
- `cmake/` owns current CMake modules and the quarantined historical root build. `docs/` owns architecture, layout, status, evidence, licensing, platform-readiness, and reproduction records.

Dependency direction is `apps -> engine/platform`, platform adapters implement engine contracts, scene/material/text/UI rendering depends on rendering core, and rendering core depends on bgfx. Engine core never depends on an application or OS UI. Use named, target-scoped dependencies and public/private includes; do not recreate workspace-wide include soup, cycles, whole-archive coupling, or a permanent `legacy/` dumping ground.

## Build, test, and development commands

```sh
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug --target RobloxPlayer -j6
ctest --preset macos-arm64-debug --output-on-failure
cmake --preset macos-arm64-release
cmake --build --preset macos-arm64-release --target RobloxPlayer -j6
```

The debug application is `out/build/macos-arm64-debug/apps/player/RobloxPlayer.app`. Run focused contracts with, for example:

```sh
ctest --test-dir out/build/macos-arm64-debug \
  -R 'ui-component-contract|animation-pose-contract|network.*loopback' \
  --output-on-failure
```

Headless proofs must launch the packaged Player and use normal DataModel, VisualEngine, CoreScripts, resource mounts, animation, audio, and the selected place. Save logs, semantic trees, images, diffs, and machine-readable summaries under `out/test-output/` or `/tmp`. A process exit, clear color, triangle, diagnostic overlay, compile-only archive, or unreviewed screenshot is insufficient. For UI/render claims, inspect the native capture with the available image-viewing tool. Run risky runtime regressions at least three consecutive times.

## Coding, testing, commit, and review conventions

Use C++20, four-space indentation, braces on the next line, RAII, explicit ownership, fixed-width serialized types, `nullptr`, `override`, move semantics, spans/views where appropriate, and target-scoped definitions. Follow established Roblox names and module namespaces. CMake implementation targets use `rbx-...`; exported aliases use `Roblox::...`. Keep Objective-C++ in Apple adapters, Win32 code in Windows adapters, Java/JNI in Android, and browser glue in Web.

Use `rg`/`rg --files` for discovery and `apply_patch` for hand edits. Generated code stays in the build tree. Do not silently patch vendored dependencies; record a patch and pinned upstream revision. Tests should be behavior-named (`*-contract`, `*-render`, `*-loopback`) and cover normal, boundary, malformed, lifecycle, and teardown cases. Never weaken a valid test or bless a golden solely to make CI green.

History favors short imperative/descriptive commit subjects such as `refactor: isolate player host`. Keep commits subsystem-focused. Exclude `out/`, credentials, reference packages, places, generated shaders, captures, and unrelated user changes. Pull requests must state behavior and architecture impact, exact commands/results, remaining limitations, and honest platform status; UI/rendering changes require reviewed screenshots and diffs. Do not add Codex/AI co-authorship or attribution.

# Complete Implemented-Work Ledger

The following is what has actually been implemented or proven so far. Qualifications are deliberate: compile, unit, packaged-runtime, visual, and acceptance evidence are not interchangeable.

## Architecture, build system, and source organization completed so far

- Recorded the pre-modernization architecture/dependency map, complete major-directory layout plan, target ownership rules, and root-layout manifest. The historical root build moved to `cmake/LegacyRoot.cmake`; current work uses target-based CMake 3.28 and C++20 with `macos-arm64-debug` and `macos-arm64-release` presets.
- Added the platform-neutral `RobloxPlayer`, shared platform-host contract, isolated macOS adapter, dependency-boundary test, reference-aware packaging presets, and target-scoped subsystem graph. Debug and release arm64 targets compile; the recorded focused suite reached 12/12, and the signed release foundation bundle has system-only dynamic dependencies.
- Moved the actual GfxCore contracts/common implementation into `engine/rendering/core`, split all tracked GfxRender files into scene/material/text/UI owners, and moved GfxBase to rendering foundation. The real `VisualEngine` now links into Player; this is not a sample renderer.
- Moved complete reflection, XML serialization, DataModel, App/v8datamodel, simulation (`v8world`, `v8kernel`, solver), animation, G3D/RbxG3D math, physics, AppDraw/tools, runtime GUI/widgets, scripting/embedded Roblox Lua VM, ContentProvider, terrain, CSG, humanoid state machine, security, and Lua public-header families into named subsystem owners.
- Split terrain into shared grid/material/water/meshing and DataModel-bound runtime targets to avoid a simulation/DataModel cycle. Fixed x86-only SSE2 selection so Apple arm64 no longer takes an invalid desktop shortcut.
- Moved foundational `Base`, all 355 tracked CSG files, the 556-file historical Bullet snapshot, the seven-file historical LZ4 snapshot, and Boost build metadata into explicit owned or historical dependency locations. These moves preserve history; maintained upstream replacement work remains where recorded.
- Moved historical Player macOS and Windows projects, hybrid mobile bridge, Xbox shell/sample, 1,042-file Studio/plugin family, RCCService/arbiter applications, PropertySheets/build rules, QTitanRibbon, deployment/upload wrappers, managed descriptors, shader scripts, lint/docs utilities, debugger visualizers, and plist files under appropriate app/platform/tool/test owners. Duplicate material was preserved with distinct names instead of deleted.
- Moved the proprietary FMOD corpus intact into its historical/quarantined location while removing it from the active runtime graph. No historical renderer, Bullet, FMOD corpus, or other source was deleted.
- Removed obsolete precompiled-header coupling from migrated targets; restored explicit missing standard/Boost/FreeType dependencies; normalized filesystem include case; fixed SDL’s Apple configuration selection; restored typed FastLog registration; and rebuilt the complete scripting/DataModel dependency graphs after moves.

## Renderer, shaders, resources, and packaging completed so far

- Implemented the bgfx backend seam in the real GfxCore path: initialization/shutdown ownership, vertex/index buffers, layouts, static/dynamic geometry, textures, samplers, shader programs, render states, framebuffers, main-framebuffer native-pixel sizing, readback, view submission, caps mapping, and resource lifetimes have executable Metal coverage.
- `RobloxPlayer` initializes and submits bgfx Metal frames through the real VisualEngine. The DataModel/serialized place path now reaches actual scene rendering. A live `player-baseplate-metal-render` regression rejects blank output and proves more than a dependency probe.
- Recovered legacy sampler stages and compiled 135 owned scene shader programs. Pinned upstream `shaderc` was built, and the portable shader pipeline has produced Metal plus cross-backend proof artifacts including SPIR-V. Packed modern shaders remain research evidence, not copied implementation.
- Generated the resource manifest, a deterministic scoped reference inventory, a hash-verifying in-game UI importer, packaged Player overlays, and a shared Player/ContentProvider asset mount table. Recorded checkpoints include 673 manifest entries, a 1,553-file scoped release overlay, and 1,753 imported Player payloads as the work evolved.
- Added explicit resource origin/precedence reporting and package validation. Packaged applications pass ad-hoc `codesign --verify --deep --strict`; release cache/package and headless bgfx/Metal checkpoints have passed with `player-core` winning the recorded asset resolution test.
- Added platform-safe-area insets to the native surface and current ScreenGui inset/safe-area reflection contracts. Retina/native-pixel handling is present, though complete current-client scaling parity remains unfinished.
- Implemented current `Decal.Color3` serialization/render tinting and `Texture.OffsetStudsU/V` across block, wedge, torso, sphere, cylinder, and CSG generated UV paths, with native contracts and repeated packaged runs.
- The supplied Baseplate and Sky numeric remote asset IDs do not exist in the supplied corpora or inspected caches. They remain honestly unresolved offline; unrelated legacy images were not substituted. The packaged SpawnLocation PNG and its decal path have independent coverage.

## Reverse engineering and genuine 2026 UI work completed so far

- Statically inspected the hash-recorded 2026 Player executable and APK using strings, PE section/VA mapping, reference recovery, package/resource inspection, manifests, and structured content. Recovered native contracts include AutomaticSize behavior, `GuiState`, ScreenGui insets, safe areas, and `UserInputService.PreferredInput` rather than guessing them from screenshots.
- Mounted the authoritative supplied `InExperience.rbxm` graph through the normal CoreScript/CorePackage path: 26,634 instances and 24,388 ModuleScripts. The package hash recorded by the project is `6cf84a6c34fcf514c029301508e1601cf8da0fd4fb781bc5789549a668f95bf1`.
- Implemented/fixed native UI dependencies exposed by that genuine package: zero-area visibility and hit testing, tagged GUI invalidation, filtered property signals (including `AbsoluteSize`), current teardown ordering, lower-camel Instance member resolution, two-`UDim` `UDim2.new(x, y)`, AutomaticSize composition, affine feedback/fixed-point layout, UI constraints, grid/list/flex cross-axis behavior, stylesheet enum/numeric reflection, selector matching, style derivation/cascade, Foundation opacity/text sizing, text editing, task scheduling, and context-action propagation.
- Packaged the exact 50-file FoundationImages set and current cursor/input-hint resources. Corrected transparent thumbnail rendering and Report/Switch Avatar icon transparency paths. Implemented real `rbxthumb://type=Avatar...` rendering through the local character/ViewportRenderer rather than a canned avatar image.
- Recovered and implemented Player dependencies including FeatureRestrictionManager, PeoplePage card/list selection, Chat timeout behavior, localization country lookup, cached localization JSON, offline RtMessagingService, AchievementService, GenericChallengeService, ScriptProfilerService, and CoreScript service/event contracts. Localization initialization improved from the recorded 35.55 seconds to 6.63 seconds.
- Genuine Chrome hamburger/close activation, submenu rendering, People onboarding tooltip `OK`, Settings/People page mounting, Player row population, and close/reopen behavior have native interaction proofs. The supplied submenu resolves to a 192x356 six-row layout; recovered fixed-point layout produces recorded 800x595 menu, 254x132 tooltip, and 145x201 People cards.
- The genuine `PlayerListReskin` close button was previously authored at zero due to the old two-argument UDim2 bridge; it now resolves to 24x24. A 400-frame packaged run exercised supplied Chrome hamburger, Leaderboard action, populated local Player row, close, and Chrome reopen, with native state assertions.
- The leaderboard verifier now activates the real `MainCanvas.leaderboard` entry at logical y=320. The actual `CoreGui.PlayerListReskin` panel opens in the upper-right and contains a populated `Player` row. This is progress, not acceptance: it currently presents the desktop `Leaderboard`/`People` table rather than the reference compact player card, and the clicked-player contextual panel with live avatar, display name, handle, and `Examine Avatar` has not been proven.
- Current UI remains visually and functionally incomplete: pause-menu geometry/pages, leaderboard styling/presentation, some icons, responsive scaling, Chrome feature flows, chat, report, emotes, respawn, switch avatar, settings, controller/touch navigation, and interaction stability still require genuine package-backed implementation and visual proof.

## DataModel, reflection, physics, and 2026 format compatibility completed so far

- Recovered and registered current enums/properties/events from exact Studio metadata, including `MakeupType`, `MarketplaceItemPurchaseStatus`, `PromptPublishAssetResult`, `RaycastFilterType` with compatibility names, `MarketplaceBulkPurchasePromptStatus`, the seven-field typed `PromptBulkPurchaseRequestedV2`, and SocialService `OpenShareSheetWithLink(string)`.
- Backported `CFrame.fromMatrix(position, right, up[, back])` with current column convention and implicit cross-product back vector, `CFrame:GetComponents()`, `Content.fromObject(game).Object` identity, and relevant current Foundation/CoreScript construction behavior.
- Implemented native `RaycastParams`, typed `RaycastResult`, and `Workspace:Raycast` through the real ContactManager. It supports position/normal/material/distance, finite and 15,000-stud validation, descendant Include/Exclude filters, `IgnoreWater`, `RespectCanCollide`, segmented rays beyond the historical 5,000-stud limit, and genuine all-primitives `BruteForceAllSlow`. Three repeated runs agree beyond 6,000 studs.
- Implemented the current 32-group collision matrix in PhysicsService, BasePart assignment, contact resolution, and raycasts. Registration, rename, unregister, masks, current/deprecated queries, propagation, and Default migration are covered by native Luau tests.
- Implemented strict version-1 `Workspace.CollisionGroupData` parsing/writing from three hash-recorded 2026 RBXLX fixtures. It preserves group IDs, little-endian masks, and names; rejects malformed version/count/ID/duplicate/mask/default/truncation/trailing/asymmetric forms; round-trips byte-for-byte; and affects real raycast results.
- Disabled obsolete FormFactor size quantization by default, so a current 4x4x4 Part creates a real four-stud physics shape. The related ray proof reaches the correct surface position, normal, and distance.
- The downloaded 2026 Baseplate loads through the normal XML serializer and has completed multiple 400-frame Metal runs with genuine Chrome/PlayerList interaction. This proves compatibility progress, not visual acceptance: remote sky/baseplate assets, materials, complete UI geometry, gameplay, and the final selected paired-place scenario remain unfinished.

## Animation, avatar, audio, and networking completed so far

- Moved the genuine Animator, AnimationTrack/State, KeyframeSequence/Pose, controller, and joint pipeline into the animation subsystem. Added real playback-track querying/events, deterministic safety regressions, and compile integration with DataModel/rendering. This does not yet prove full in-player R6 gameplay or authentic R15.
- The humanoid state-machine target compiles against real DataModel, simulation, physics, animation, UI, networking, reflection, and rendering-foundation modules. VMProtect include/calls were removed from this owned gameplay subsystem through an owned portable marker contract while preserving control flow. Complete R6 appearance/animation and R15 hierarchy/skinning/emotes remain blockers.
- Replaced the misleading audio placeholder graph with the real miniaudio 0.11.25 mixer/runtime. Implemented owned handles, playback, pause, stop, seek, volume, looping, attenuation, pan, encoded in-memory decoding, corruption/resource limits, voice stealing, master gain/mute, deterministic offline mixing, reflected Sound/SoundChannel/SoundService/SoundWorld, listener/source updates, and CoreAudio device smoke coverage.
- Removed FMOD headers/types/symbols from the active owned audio archive and verified no FMOD linkage in active Player archives. Player-connected audio exists, but the observed high-pitched volume-preview OOF and full rate/channel/device/lifecycle parity still require acceptance work.
- Moved the complete 633-file networking/RakNet family, Player host support/shared-client, and Xbox in-game chat into organized owners without deleting historical source.
- Replaced RakNet in the active network graph with pinned Valve GameNetworkingSockets 1.5.0 plus pinned protobuf 35.1 and Abseil 20250512.1. Added owned endpoints, packets/messages, reliable/unreliable ordered delivery, buffers, stats, IPv4/IPv6 listen/connect/accept/close, asynchronous hostname resolution/cancellation, lanes, events, metrics, timeouts, send-rate controls, reported send failures, and repeated deterministic loopback tests.
- Added authentication policy, local-loopback exception, certificate blobs, platform patches, and pinned OpenSSL 3.5.7 source-building support. A native iOS arm64 GNS archive proof exists. This is not a complete iOS Player or cross-platform verification.
- Full client/server Replicator/DataModel sessions, certificate issuance/rotation, RCCService game-server adaptation, Ubuntu VPS proof, reconnect/MTU/congestion acceptance, and equal browser/Emscripten transport remain unfinished. Browser networking must use an appropriate WebRTC/WebSocket-compatible adapter behind the same engine contract; native UDP cannot simply be compiled into a browser.

## Current checkpoint and explicit non-acceptance

The current Player proves the real VisualEngine, DataModel place loading, scene shaders, packaged resources, bgfx/Metal rendering, genuine supplied Chrome/CorePackages mounting, partial real UI interaction, current collision/raycast contracts, and substantial audio/network architecture. It does **not** yet satisfy full CoreGui/UI visual and functional parity, the reference Chrome leaderboard, stable cursor/camera control, complete settings/chat/report/emotes/respawn/switch-avatar flows, authentic R6 gameplay, full R15, ShadowMap, selected-place acceptance, all remote/offline assets, full client/server replication, RCCService deployment, Xbox launcher parity, performance acceptance, or non-macOS Player builds.

There is no external blocker. Remaining work is implementation and verification. Legacy D3D9, D3D11, and OpenGL renderers remain excluded from the default macOS build but physically present; never remove them until every verification requirement passes and the user separately gives explicit permission.

# Complete Remaining Execution Plan

The following reproduces the persistent execution plan as closely as possible. Every checkbox remains incomplete until implementation and proportional proof exist.

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

# Verbatim User Execution Brief

The following brief is preserved verbatim, including capitalization, punctuation, and original wording, so future contributors cannot normalize away the acceptance criteria.

> I'm going to sleep soon. Here is what you need to hear.
>
> ISSUES:
>
> - Textures/skybox are not working (on this baseplate, at least), the textures fall back to solid grey, and the skybox falls back to a solid blue. Fix this.
> - The 2026 UI is still buggy. The Report and Switch Avatar icons in the hamburger menu have a grey background, instead of being the transparent white icons they're supposed to be.
> - The 2026 esc/pause menu needs LOTS of work. It is supposed to look like the image attached, as that is the current state of it in the 2026 Roblox client.
> - When moving the camera, my cursor snaps to the middle of the screen. Overall, the cursor system is buggy. 
> - The current state of the UI doesn't scale the same way as the 2026 client, and is sometimes stretched. I want the whole scaling system from the 2026 client to be reverse engineered/decompiled, and implemented.
> - The "ShadowMap" lighting technology is not implemented in this version of our client, but is in the latest 2026 one. I want this Engine Lighting Technology to be fully reverse engineered/decompiled, and implemented correctly for the games that use it.
> - The leaderboard UI is mostly still broken, and has a weird, grey background, and is broken in most areas. Completely fix it.
> - None of the features that are in the current 2026 In Experience UI have been implemented. Notably, "Chat", "Report", "Emotes", "Leaderboard (just needs more work), "Respawn", "Switch Avatar (when applicable)", and Music. Music should be completely removed from the UI and not implemented, as it is not useful. These are the main features that need to be implemented. Note that the Chat UI must be the modern UI from the 2026 client, not "LegacyChatService". Upon clicking the unsupported features, the entire "top bar/chrome" UI disappears, for an unknown reason. 
> - Some sounds are incorrectly sped up, higher pitched, or outright broken. Example: the volume control example sound (OOF) is high pitched.
> - Many esc menu features (such as shift-lock), do not work. As well as this, zooming in and out in-game is really fast and NOTHING like the 2026 client. I want you to decompile/reverse engineer the current client's zooming and overall camera system, so it is 1:1 to the 2026 client.
>
> GOALS: 
>
> - Complete 2026 UI working, completely, with a "Switch to Legacy UI" toggle in Settings (in the esc menu), that when enabled, switches back to the legacy UI (original 2016).
> - Complete "Xbox UI", (already implemented in the Durango work) used as a launcher for games, with adapted controls for PC, mobile, and web. Keep the existing controller support (this will be explained later here). Must have completely working navigation, sound, and visuals.
> - Complete ShadowMap implementation, from reverse engineered/decompiled 2026 client code. Nothing should be missing from this implementation. Ensure that it has 1:1 visual parity. (Do not open Roblox, or attempt to use the "Computer Use" tool for this. You should just be confident in the implementation that you know it is 1:1 via decompiled/reverse engineered code.
> - Complete R15 implementation, with working emotes (if possible), and animations (required). 
> - COMPLETE compatibility layer for 2026 RBXL/RBXLX files, and 2026 Roblox games. Analyze the features that "2026-place-files" use, and if something from there is missing in the current source, completely implement it by reverse engineering/decompiling the feature from the 2026 client. Most 2026 Roblox games/RBXL/RBXLXs should work as a result of this. Do not break <2026 games from working. In the future, I will add more RBXLs and RBXLXs for you to study, and expand the compatibility layer.
> - Use my GitHub .env to create a PRIVATE repository for the project on my q8j-dev GitHub account. The old name "OpenRBLX" doesn't fit, as it is already being used by another project. Come up with a unique and creative name for this. Do NOT mention that you are the "co-author" of this project, or anything like that (in commits, README, etc). This GitHub repository will be useful, as you can utilize GitHub Actions to build for Windows (x64, ARM), Linux (x64, ARM), iOS, Android, Emscripten (WebGPU preferred) and Xbox (UWP, APPX ready for sideloading in Dev Mode on Series X/S). This is why we need to keep controller support, as the Xbox build will actively need it. Make sure the controller support works for the platforms mentioned (when possible). 
> - I do not want to see any visual/audible bugginess when playing on the client. If something may introduce visual/audible bugginess, provide a solid implementation that fixes it.
> - Adapt RCCService, and all Roblox server software bundled in the source to the new networking system (https://github.com/ValveSoftware/GameNetworkingSockets). Do not adapt server software that isn't relevant. The server software should be fully compatible with our modernized client, and work for everything that it originally does (full game-server networking). It should additionally work on a modern VPS, such as an Ubuntu VPS.
> - Continue to reorganize stuff that hasn't already been organized, and modernize implementations the the client source uses. The same applies to server source.
> - Anything else mentioned in the original goal that I haven't covered, I assume you will complete. If you don't remember, recap on the very original goal. 
>
> NOTES:
>
> - If you are adding/fixing/implementing something, do NOT make stubs, examples, placeholders, or fake it. This is against the rules.
> - Save this prompt to a file, so you can recap on it when needed.
> - For the other platforms (apart from macOS), I assume you know which renderer to pick, and be confident that it will work without any issues. I recommend double checking implementations, like in the renderer, so that you are sure it will fully work on other platforms, as intended.
> - Do not ask any questions, or do anything that requires extensive permission (e.g. sudo).
> - I will assume that you are capable of properly debugging this if something breaks, and understanding what I want from you.
> - This is possible. It will take lots of work, but it's possible, so don't stop until you're fully done.
>
> That's mostly all. In the future, I may have more work beyond this, but this is what you need to do now, so get started.

## Final agent handoff rules

Continue from the next buildable checkpoint; do not restart or replace working genuine paths. The immediate priority is the authentic Chrome leaderboard: recover the authoritative compact presentation, prove populated identity, row click, selected-player contextual card, live avatar, `Examine Avatar`, close/reopen, responsive scaling, and mouse/keyboard/controller/touch behavior. Then complete the rest of genuine in-experience UI and its native engine contracts before the final legacy-UI toggle.

Maintain offline operation without Wi-Fi or a game server while retaining clean future server migration boundaries. Emscripten must not be a reduced client. Do not create or push the private repository until the full implementation reaches final cross-platform work. Do not run the real Roblox client or use computer-control automation to judge ShadowMap. Use authorized static clean-room evidence and owned implementation. Preserve the user’s dirty worktree, never expose credentials, and report limitations plainly.
