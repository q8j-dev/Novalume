# Modernization status

Last updated: 2026-07-19

## Current phase

The user explicitly deferred further UI work on 2026-07-18. Rendering and
lighting, animation/R15, audio, camera/input, place compatibility, networking,
server software, launcher/platform shells, packaging, and cross-platform
readiness are active first. The genuine 2026 Player UI remains preserved for
the final return; it is not replaced or treated as accepted while deferred.

## Completed evidence

- Created the requested private GitHub repository as `q8j-dev/Novalume`, kept
  the historical upstream as the separate `origin` remote, and pushed the full
  reorganized modernization tree to the private `novalume` remote without
  credentials or browser-cookie material. The initial modernization commit is
  authored only as `q8j-dev`; no co-author attribution was added. Cross-platform
  Actions are intentionally not claimed complete until their native build
  configurations pass rather than publishing decorative or always-green jobs.
- Connected the replacement miniaudio engine to the packaged Player's normal
  `Sound`/`SoundChannel`/`SoundService` DataModel path. Headless verification
  now disables only the physical output device while retaining the same active
  48 kHz mixer used by the interactive Player. This exposed and fixed the
  volume-preview OOF pitch defect at its root: pinned miniaudio 0.11 leaves
  `ma_audio_buffer_ref.sampleRate` unset, so decoded 22.05 kHz PCM had been
  interpreted as 48 kHz and played 2.176x too fast/high. Owned voices now carry
  their decoded source rate into miniaudio's resampler; Sound time/seek remains
  expressed in source frames. A cross-rate unit regression proves a one-second
  22.05 kHz clip retains its duration in a 48 kHz mixer and that
  `PlaybackSpeed=0.5` halves source-frame advance. The packaged runtime test
  loads the genuine `rbxasset://sounds/uuhhh.mp3` through ContentProvider,
  verifies its 0.417959-second decoded length, spatial playback from a real
  anchored BasePart, non-silent mixed output, loop events, 0.433-second unit
  playback and 0.817-second half-speed playback, and exact voice teardown.
  `audio-runtime-contract` and `player-audio-runtime` pass. This closes the
  reported OOF/sample-rate and first Player-connected 3D-audio checkpoint;
  streaming, the current wireable audio graph/custom curves, remaining effect
  parity, and device-interruption/suspend matrices remain active before
  complete audio acceptance.
- Imported the authentic R15 runtime corpus from the user-designated
  `LIVE-WindowsStudio64-version-ed7d8193e8564b1f` Studio build through a
  ten-file, 81,794-byte SHA-256 manifest. The normal `Player::loadCharacter`
  path now deserializes the supplied 14-MeshPart/15-Motor6D R15 rig and runs
  Studio's complete `Animate` controller graph, including its animation,
  emote, mood, and health models. Added the native `MeshPart` DataModel class,
  authored-size render scaling, texture and double-sided material/geometry
  handling, child-bearing Attachments, the current Humanoid scaling flag, and
  the Animator/AnimationTrack surfaces required by that controller. Modern v2
  mesh files extend the legacy vertex stride from 36 to 40 bytes; the reader
  now honors the format's append-only contract by consuming the known prefix
  and bounds-checking skipped fields instead of rejecting genuine Studio
  meshes. A packaged 400-frame Metal run proves 14/14 MeshParts loaded, 2,808
  mesh vertices, 1,916 mesh faces, 2,790 total rendered scene faces, 31 live
  joints, character movement, and one playing animation track. The 2560x1440
  framebuffer proof was visually inspected and shows the real segmented R15
  mesh character on the textured baseplate. Four focused model, mesh-format,
  animation, and reflection tests pass, and strict bundle signing verifies.
  This closes the supplied standard-R15 hierarchy/mesh/controller render
  checkpoint. R15 is now the default in both the command-line entry point and
  `PlayerRuntime`; the signed release and debug `RobloxPlayer.app` bundles both
  carry the exact Studio `characterR15.rbxm` payload (`e841643e...f8c017`) in
  their active avatar-runtime overlay rather than relying on a source-tree
  model that an app launch cannot see.
- Imported the exact current Studio `R15-plus.rbxm` model and registered its
  concrete AnimationConstraint, BallSocketConstraint, NoCollisionConstraint,
  Bone, WrapTarget, FaceControls, and shared-string serialization contracts.
  The runtime selects it independently from standard R15, loads all 15
  MeshParts, renders 6,640 vertices and 10,567 faces, and supports four-weight
  CPU fallback plus GPU palette skinning for FileMeshData v7. A focused model
  contract now also places the authentic rig in a runtime container, enables
  its R15 Humanoid scaling path, changes the authored width/height/depth
  NumberValues, and proves that OriginalSize and OriginalPosition metadata
  drive the resulting part and attachment frames. BodyTypeScale and
  BodyProportionScale now interpolate through the canonical Classic,
  ProportionsNormal, and ProportionsSlender neutral-size tables derived from
  Studio's exact `characterR15.rbxm` and `AnthroRigs.rbxm`; focused tests prove
  both normal-to-slender and Rthro-to-classic UpperTorso transitions. Exact
  offline Rthro mesh delivery and live runtime acceptance are now complete as
  recorded in the later Rthro checkpoint below.
  Deterministic action/emote fade tests
  also exposed and fixed a real AnimationTrackState discontinuity: Stop and
  AdjustWeight used to overwrite the fade start time before sampling the live
  weight, snapping every transition back to the previous fade's initial
  weight. Both operations now preserve the current pose weight through the
  requested fade, including replicated track-state signals.
- Completed the current Humanoid rig-assembly and body-replacement surfaces
  used by the supplied Studio avatar unification, mannequin, thumbnailing, and
  UGC-validation Lua: `BuildRigFromAttachments`, `GetBodyPartR15`,
  `ReplaceBodyPartR15`, and the RobloxScript-only
  `GetAccessoryHandleScale`. The implementation operates on the authentic R15
  parts, matching RigAttachments, Motor6D endpoints/frames, authored
  `AvatarPartScaleType`, and the same canonical body-scale interpolation as
  normal Humanoid scaling. Public rig rebuilding removes only attachment-backed
  R15 motors, preserves custom motors, and is idempotent. A focused exact-Studio
  model test removes all 15 authored motors, rebuilds and validates every C0/C1
  attachment frame, repeats the build without duplication, replaces the real
  UpperTorso from a second exact rig, and proves every joint is repointed while
  the old part is detached. The signed release app was then relinked; strict
  code-sign verification, default R15 (14/14 MeshParts, one active track), and
  R15-plus (15/15 MeshParts, two active tracks) each pass 300 packaged Metal
  frames through the actual `RobloxPlayer.app`.
- Hardened animation advancement for real hitches and reverse playback. Loop
  normalization now uses bounded floating-point remainder math instead of
  narrowing a potentially large cycle count to `int`. Track stepping delivers
  every crossed keyframe and every `DidLoop` transition in deterministic time
  order across multi-cycle forward and reverse deltas, preserves phase when
  speed changes to zero, and delivers all non-looping keyframes before the
  automatic stop fade. Empty sequence pointers no longer crash stepping, and a
  zero-duration authored pose is applied once and stops instead of leaving an
  immortal playing track. The focused contract covers 3.5-cycle forward and
  reverse hitches, four repeated midpoint events, three loop events, final-pose
  sampling, zero speed, non-looping endpoint order, and zero-duration stop.
  `rbx-animation-pose-tests` passes, and the subsequently relinked signed app
  repeats both 300-frame default-R15 and R15-plus Metal proofs.
- Extended the packaged R15 verifier through the authentic Studio controller's
  own `Animate.PlayEmote` BindableFunction instead of manually starting a test
  animation. Both standard R15 and R15-plus accept the `wave` invocation and
  emit the real `WaveAnim` AnimationPlayed track from the supplied controller;
  the verifier rejects a missing function, rejected/yield error, absent
  AnimationPlayed event, or wrong track identity. Both signed 300-frame Metal
  runs pass, and strict bundle code-sign verification remains green.
- Made packaged R15/R15-plus geometry verification independent of asynchronous
  first-frame content timing. Headless gameplay proofs now prime the normal
  `MeshContentProvider` before VisualEngine binds the scene and still parse the
  exact MeshPart payloads at the final gate. The verifier requires stock-camera
  fixtures to render at least the loaded avatar's complete vertex/face counts;
  it only exempts a place-owned Scriptable first-person camera, which may
  intentionally hide its local body. The signed default R15 run renders all
  14 MeshParts and the R15-plus run renders all 15, with 6,696 scene vertices
  and 10,595 scene faces for R15-plus instead of the former scheduler-dependent
  28-vertex frame. Both execute the package-backed `WaveAnim`, move through the
  world, rotate the stock camera, complete 300 Metal frames, and exit cleanly.
- Extended the same signed R15-plus runtime proof through the exact model's 19
  `NoCollisionConstraint` instances. Every constraint resolves two distinct
  authentic body parts, every pair is unique, and each pair is suppressed in
  the actual Primitive collision filter while the rig is parented to Workspace.
  The verifier disables and reenables one genuine constraint and requires the
  pair to become collidable and then excluded again, proving live behavior
  rather than merely deserialized references. The packaged run reports
  `pairs=19 live-toggle=1` alongside all 15 rendered meshes and `WaveAnim`.
- Added the exact current `Script.RunContext` reflection contract recovered
  from the supplied Studio binary and API metadata: `Legacy=0`, `Server=1`,
  `Client=2`, and `Plugin=3`. A normal Script accepts current context changes
  and restarts with property notification; LocalScript retains its fixed
  Legacy context, matching Studio's own diagnostic behavior. The enum is
  registered globally and the focused Luau test passes in the freshly linked
  `ui-component-contract`. This removes the selected Backrooms fixture's real
  `RunContext is not a valid member of Script` failure.
- Closed the selected Backrooms pair's non-UI compatibility checkpoint through
  the actual signed `RobloxPlayer.app`. Both RBXL and RBXLX enter the normal
  serializer, authoritative loopback server/client replication, scripts,
  simulation, current R15 controller, and bgfx/Metal VisualEngine path. An
  isolated encoding comparison matches 4,112 instances, 780 parts, 16 scripts,
  one ProximityPrompt, and 16 FontFaces. Packaged 300-frame runs establish the
  fixture's legitimate place-owned Scriptable first-person camera, move the
  character, load 14/14 exact R15 meshes, and invoke the supplied controller's
  `WaveAnim`; the latest RBXLX run completed with 16 scene batches and 66 final
  draws. The place constrains movement in narrow scripted corridors, so its
  Scriptable-camera proof requires one stud of observed native root travel,
  while stock-camera fixtures retain the stronger five-stud traversal gate.
  The exact commands and deferred UI limitations are recorded in
  `selected-place-compatibility.md`; `modern-place-serialization-contract` and
  `ui-component-contract` pass together.
- Connected the reflected current `Lighting.Technology`, `GlobalShadows`, and
  `ShadowSoftness` properties to the real `SceneManager` shadow path. Voxel and
  Compatibility no longer silently receive the directional ShadowMap pass;
  ShadowMap and Future do when global shadows are enabled. Fixed
  `Lighting::setGlobalShadows` so a live property change notifies rendering
  instead of leaving stale shadow state. The separable filter radius now uses
  the serialized softness value. Added a packaged Metal verifier that requires
  the requested technology, renderer configuration, a square render target,
  nonzero caster batches, live off/on behavior, and a visible framebuffer
  effect. Three consecutive runs passed on the downloaded 2026 Baseplate with
  Technology=Future, ShadowSoftness=0.2, a 256x256 map, one caster batch, and
  17,255 pixels darkened by the enabled pass; the final capture was visually
  inspected. Extended that production pass to quality-selected one-, two-, and
  three-cascade directional shadows. High quality now renders three independently
  centered orthographic cameras into a 2x2 atlas, publishes all three world-to-
  atlas matrices and ordered split distances through the shared shader constants,
  selects the cascade per pixel for default, voxel-terrain, and smooth-terrain
  materials, and confines both separable blur taps and the legacy radial edge
  mask to the active tile. The packaged high-quality Metal verifier now requires
  all three cascades in addition to the live off/on and framebuffer-difference
  checks. The caster shader now writes the depth of the cascade actually being
  rendered instead of incorrectly running receiver-side cascade selection over
  incomplete matrices. The RGBA8 depth atlas uses a one-quantization-level
  constant bias plus a bounded derivative slope bias; receiver selection uses
  camera depth, falls through to wider cascades at frustum corners, and never
  samples across atlas tiles. All opaque BaseParts now cast by default through
  the reflected, live-invalidating `BasePart.CastShadow` property instead of
  limiting real caster geometry to humanoid clusters. Dedicated owned shadow
  vertex programs cover both classic and smooth terrain and are appended to the
  immutable historical shader corpus for both Metal and OpenGL, producing
  complete 137-entry packs. Allocation now checks framebuffer/shader support,
  maximum texture size, and stencil availability, scales down to 128 pixels,
  and falls back to D16 depth when D24S8 is unavailable. The strict Metal proof
  creates genuine smooth terrain, exercises character, BasePart, and terrain
  casters, verifies one/two/three quality cascades, toggles Baseplate
  `CastShadow`, toggles `GlobalShadows`, and compares enabled/disabled pixels.
  Three consecutive runs reported a 1024x1024 atlas, splits 32.256/92.16,
  15 enabled caster batches versus 12 with Baseplate casting disabled, and
  72,539-73,139 visibly darkened pixels. This closes the production ShadowMap
  implementation checkpoint; broader selected-place and non-Metal perceptual
  acceptance remains part of the cross-platform matrices.
- Completed native modern camera pointer delivery on macOS. Pointer lock now
  preserves the logical cursor position, centers and disassociates the hardware
  cursor without corrupting absolute event coordinates, suppresses warp events,
  and restores the pre-lock screen position across focus loss and multi-display
  layouts. Precise trackpad deltas are normalized separately from wheel detents.
  `UserInputService.PointerAction` is a reflected post-dispatch event carrying
  wheel, pan, pinch, and the real `gameProcessedEvent` result, matching the
  supplied current PlayerModule camera input and its spring-based zoom path.
- Made Player HTTP teardown explicit and deterministic. The main and curl worker
  pools now join outstanding work, the cache-statistics thread is interrupted
  and joined, shared curl handles are released, and `curl_global_cleanup` runs
  before OpenSSL/static destruction. This fixes the intermittent shutdown
  `EXC_BAD_ACCESS` in curl/OpenSSL; ten consecutive direct R15 runs and the
  focused replication/baseplate/ShadowMap/R15 CTest group exited cleanly.
- Fixed the downloaded 2026 Baseplate's actual surface-image and skybox path.
  Canonical `assetdelivery.roblox.com/v1/asset/?id=` references now survive
  `ContentId` reconstruction, L8/LA8 images are expanded correctly, and bgfx
  indexed draws no longer apply the minimum vertex twice. The packaged Metal
  verifiers now require pixel variation and loaded dimensions: Baseplate
  400x400, SpawnLocation 64x64, and all six sky faces 1024x1024. This is direct
  framebuffer proof; it supersedes the earlier fallback-solid-color observation.
  The Player also enables the existing integrity-checked HTTP cache and
  original-URL zero-latency lookup. `player-offline-asset-cache-metal` starts
  from an isolated empty cache, warms once, then launches fresh surface and sky
  verifier processes with HTTP(S) routed to a closed local port; both pass.
  Previously fetched place assets therefore work without Wi-Fi. Never-fetched
  arbitrary remote assets still require an initial network retrieval and are
  not misrepresented as packaged content.

- Completed the genuine Chrome Respawn lifecycle. The supplied `respawn`
  integration opens SettingsHub's real `ResetCharacterPage`; the strict proof
  requires the official confirmation sentence, both `Respawn` and
  `Don't Respawn` actions, a bounded SettingsHub, and continued Chrome
  presence before activating the package's own reset button. That action now
  reaches the normal Humanoid death path and the existing five-second
  authoritative `Player::loadCharacter` builder instead of a replacement GUI
  or synthetic avatar. A narrowly gated, non-reflected standalone-player
  authority contract owns only the local character when a Client and no Server
  are present. CameraSubject now supports the real nil transition, and the
  standalone host preserves the invariant that a nil subject rebinds to the
  current healthy local Humanoid without overriding valid game-selected
  subjects. Three consecutive 1,800-frame packaged bgfx/Metal runs proved the
  confirmation, death, distinct replacement model, health 100, exact camera
  ownership, closed SettingsHub, and retained Chrome with 14 final-frame
  draws. The official leaderboard regression still passes at 22 draws, strict
  code-sign verification passes, and `TASKLIST.md` remains byte-identical.
- Corrected the Chrome leaderboard target after direct user clarification on
  2026-07-18. The wide `PlayerListReskin` / `Leaderboard` / `People` table and
  its contextual card are explicitly rejected as a reskin, not accepted as the
  official Chrome leaderboard. The Player host had forced the package flag
  `PlayerListReskin2=true` whenever client settings were unavailable; that
  local override is removed. Chrome's authoritative `leaderboard` integration
  delegates to `PlayerListManager`, so offline startup now preserves the
  package's normal flag selection and mounts `CoreGui.PlayerList`, while a
  verifier rejects any `CoreGui.PlayerListReskin` root. The normal branch
  renders the compact upper-right 166-pixel PlayerList with its local `Player`
  row. A strict 400-frame verifier now opens it from Chrome, requires a bounded
  populated `CoreGui.PlayerList`, closes it through the package's own compact
  dismiss button, reopens it from Chrome, and rejects any
  `CoreGui.PlayerListReskin` root. Three consecutive packaged bgfx/Metal runs
  passed with 22 final-frame draws, the 2560x1440 capture was visually
  inspected, and the bundle passes strict code-sign verification. This proves
  the corrected official branch and round trip at the current desktop size;
  responsive/input-matrix and pixel-parity acceptance remain incomplete. All
  earlier reskin/context-card results are diagnostic dependency evidence only
  and do not close a leaderboard checkpoint.
- Removed Music from the current Chrome product through Chrome's genuine
  integration-availability path. The supplied `music_entrypoint` still exists
  in the immutable reference package, but an owned product policy applies
  `AvailabilitySignal:forceUnavailable()` after Chrome initializes, so the
  integration is excluded from submenu construction, shortcuts, focus, and
  activation rather than hidden visually. The semantic verifier rejects both
  a mounted `.music_entrypoint` node and a Chrome `Music` label. Three
  consecutive 400-frame packaged leaderboard round trips passed with no Music
  node/label in the complete logs, 22 final-frame draws, and strict code-sign
  verification; the updated native-pixel capture was visually inspected.
- Completed the first genuine Chrome Report round trip. The authoritative
  `trust_and_safety` integration now opens SettingsHub's real
  `ReportAbusePage` without removing Chrome. Two native compatibility gaps
  were repaired: `TextBox.OverlayNativeInput` is implemented as the current
  hidden, RobloxScript-only, non-replicated, non-serialized boolean property,
  and the implemented `AvatarChatService` is advertised through the matching
  `AvatarChatServiceEnabled` engine feature so VoiceChatCore receives the
  service required by its enabled AvatarChat route. The strict verifier clicks
  the packaged Chrome row, requires a bounded SettingsHub, selected Report
  tab, both official report prompts, a visible `AbuseReportsText` TextBox with
  `OverlayNativeInput=true`, and a still-visible Chrome menu icon, while
  rejecting Report/VoiceChat compatibility errors. Three consecutive
  400-frame packaged bgfx/Metal runs passed with 34 final-frame draws; the
  native capture was visually inspected, the official leaderboard regression
  still passes with 22 draws, and strict code-sign verification passes.
- The rejected reskin's `Examine Avatar` diagnostic exposed two exact native
  marketplace event surfaces, which remain valid general CoreScript API work:
  `PromptBulkPurchaseFinished(player, status, result)` and
  `PromptBundlePurchaseFinished(player, bundleId, wasPurchased)`. The former
  uses the existing current bulk-purchase status enum and typed result table;
  the latter matches the package's shared purchase-finished callback. The
  Player target builds and that diagnostic advanced beyond both missing
  members. Inspect-and-buy is not accepted: the next observed native
  blockers are `PromptBulkPurchaseRequested` and
  `Players:CreateHumanoidModelFromUserId`, and the panel does not yet remain
  visibly mounted offline.
- Recovered and implemented the current Player contracts required by the 2026
  People page: `FeatureRestrictionManager` (including the observed abuse-vector
  enum and events), `UserGameSettings.PeoplePageLayout` (`Card=0`, `List=1`),
  `Chat.TimeoutChatAttempt`, and
  `LocalizationService:GetCountryRegionForPlayerAsync`. The genuine package now
  selects its card-layout branch and populates the local `Player` / `@Player`
  identity instead of failing during initialization.
- Fixed two shared automatic-layout feedback defects exposed by the genuine
  2026 Settings/People UI. Automatic content bounds now solve anchored,
  percentage-positioned children as an affine fixed point, and list layout
  bases include `UISizeConstraint`/`UIAspectRatioConstraint` results. Native
  proof keeps the pause menu at its intended 800x595 logical size, bounds the
  People onboarding tooltip at 254x132 instead of 1656 pixels high, and bounds
  the square-thumbnail player card at 145x201 instead of 2968 pixels high.
  Focused regression cases cover percentage-positioned tooltip decorations,
  centered automatic children, and aspect-constrained items in automatic
  lists. The Player target builds and the 244-frame native visual proof exits
  successfully; the focused test executable remains blocked at link time by
  its pre-existing missing scripting-debugger registrar definitions.
- Implemented the current `rbxthumb://type=Avatar&id=...&w=...&h=...`
  contract as a portable render-scene request. The player resolves the actual
  matching local Player character and renders its live model, face, body
  colors, and materials through the shared `ViewportRenderer`; it does not
  substitute a downloaded or hand-authored portrait. A focused parser test
  passes, both the test and signed Player targets build, and a 244-frame Metal
  proof shows the R6 avatar inside the genuine People card beside the populated
  `Player` / `@Player` identity.
- Verified the genuine People onboarding tooltip's real `OK` action through
  native pointer down/up and `Activated`; the 300-frame verifier now rejects a
  tooltip that remains visible. Fixed shared tagged-GUI style invalidation so
  Foundation rules are reapplied when React assigns a native class default
  after tags mount. The authentic `.gui-object-defaults` rule now leaves the
  card thumbnail transparent instead of exposing the legacy gray ImageButton
  plate. The same shared fix removes the opaque gray plates behind the genuine
  Report and Switch Avatar Chrome glyphs. A focused regression test preserves
  explicit non-default overrides, and packaged Metal verifiers assert the real
  card and both Chrome icons remain transparent.
- The People page is not accepted as complete: remaining card actions,
  list/grid switching, multi-player population, focus/controller behavior,
  responsive-size matrix, and the other 2026 feature pages remain active work.
- Mounted the authoritative 2026 `InExperience.rbxm` graph (26,634 instances,
  24,388 ModuleScripts) through the normal CoreScript path. Genuine Chrome,
  PlayerList, People, and Settings surfaces now render from that package; the
  headless input proof opens ESC, selects Settings, and retains a populated
  local Player row.
- Fixed shared GUI visibility so zero-area descendants are neither painted nor
  hit-tested. This removed the package's closed, `0x0` Chrome `Close` glyph
  from the render path instead of hiding it by name; native-view proof now
  shows the genuine closed hamburger and an interactive expanded close state.
  A real pointer click reaches the packaged `IconHitArea`, fires `Activated`,
  and opens the production submenu.
- Completed the missing shared `AutomaticSize`/layout composition contract:
  automatic hosts now consume `UIListLayout` and `UIGridLayout`
  `AbsoluteContentSize` through zero-offset, 100%-sized Foundation wrappers.
  The native 300-frame interaction proof now shows all six package-registered
  product-supported Chrome rows (Report, Switch avatar, Leaderboard,
  Inventory, and Respawn) in the correctly content-sized submenu, completes in 6.61
  seconds, and preserves genuine button activation. A second 350-frame proof
  drives the packaged Leaderboard integration through a complete round trip:
  it closes Chrome and hides PlayerList, then reopens Chrome and restores the
  real PlayerList with its local `Player` row; all four native buttons fire
  their own `Activated` handlers. A focused regression case
  is added to `UIComponentTests`; its standalone target is presently blocked
  at link time by pre-existing missing scripting-debugger registrar symbols,
  while the complete Player target builds and the end-to-end proof exits 0.
- Completed the Foundation flex/list cross-axis contract used by the genuine
  Settings action row. Zero-basis Fill containers and single-axis automatic
  children now retain intrinsic cross-axis content without consuming the flex
  main-axis basis. The authentic package renders the three 48-pixel Leave,
  Respawn, and Resume controls and their text instead of zero-height rows; two
  focused layout regressions cover both container forms.
- Fixed native stylesheet reflection for enum-token values and Luau numeric
  values stored by float/int UI properties. This removes the React
  `Unable to cast token to int` initialization failure and applies real
  Foundation transparency, text-size, and opacity rules instead of leaving
  default opaque Frames. The genuine Settings tab now renders its complete
  controls and correctly composed bottom action buttons in native proof.
- Added the supplied Player's exact 50-file FoundationImages package to the
  scoped, SHA-256-verified in-experience overlay. The 2026 sprite-sheet URI
  requested by the action buttons now resolves through the existing
  `player-extra` mount, and native proof shows the genuine L, R, and ESC input
  hints. No substitute icons or reconstructed imagery are used.
- Enabled the current DataModel teardown ordering in which ScriptContext states
  close before service descendants are removed. The 2026 SettingsHub camera
  observer is now disconnected before `Workspace.CurrentCamera` is cleared;
  a complete 300-frame shutdown proof no longer emits the former
  `SettingsHub:2212`/`onWorkspaceChanged` error. This corrects shared runtime
  lifetime behavior rather than suppressing the CoreScript exception.
- Completed the native offline `RtMessagingService` transport lifecycle and
  the platform `AchievementService` availability/async error contract recovered
  from the supplied Windows Player. Mounted the already-implemented
  `GenericChallengeService` and `ScriptProfilerService` at the client-owned
  bootstrap boundary. These changes advanced the authoritative 2026
  `StarterScript` through its previous line 433, 574, and profiler failures;
  it now reaches the end without a fatal StarterScript error.
- Fixed filtered property signals so
  `GetPropertyChangedSignal("AbsoluteSize")` accepts native
  `PropertyDescriptor*` event arguments. React geometry observers now receive
  real AbsoluteSize/AbsolutePosition changes instead of silently rejecting
  them.
- Added the initial native StyleSheet/StyleLink selector path for React tags,
  property resolution, attribute-backed token values, and pseudo
  `UIListLayout`/`UIFlexItem` creation. Resolution is event-driven at tag/link
  mutation points rather than scanning every ScreenGui on every frame.
- Profiled the running Player with the native macOS sampler and found the UI
  frame-time hot path reparsing complete LocalizationTable JSON once per
  visible TextLabel per paint. Localization entries are now parsed and indexed
  only when `Contents` changes. The same 300-frame genuine Settings proof fell
  from 35.55 seconds to 6.63 seconds while preserving 59 final-frame draws and
  the rendered UI. This cache is engine-wide and platform-neutral.

- Preserved the pre-existing `SECURITY.md` deletion and untracked `.DS_Store`.
- Recorded the historical module/dependency map and complete layout mapping.
- Moved the historical root build to `cmake/LegacyRoot.cmake`; it is available
  only through `RBX_ENABLE_LEGACY_BUILD=ON`.
- Added target-based CMake 3.28/C++20 configuration and macOS arm64 debug/release
  presets without `CONTRIB_PATH`.
- Added platform-neutral `RobloxPlayer`, platform host contract, isolated macOS
  adapter, portable renderer contract, and owned bgfx lifetime implementation.
- Pinned `bgfx.cmake`, bgfx, bx, and bimg to immutable commits in CMake.
- Disabled bgfx examples and legacy renderers in the default player graph.
- Added an automated shared-header dependency-boundary test.
- Selected exactly one paired modern acceptance place and recorded hashes and
  candidate-inspection provenance in `reference-research.md`.
- Compiled the historical GfxCore contracts without the missing FastLog source,
  added `API_Bgfx`, and passed an integration test that clears/submits through
  the old `DeviceContext` on real bgfx Metal.
- Moved the actual GfxCore contract/common implementation into
  `engine/rendering/core` and removed the temporary parallel compile-time fork.
- Implemented and Metal-tested vertex/index buffers, layouts, geometry, shader
  reflection/programs, state mapping, texture upload/partial locking, sampler
  binding, GPU readback, render-target textures, framebuffers, and ordered views.
- Split every tracked top-level `Rendering/GfxRender` header/source into the
  scene, materials, text, and UI modules with compatibility include shims and
  named opt-in CMake targets. The old global rendering PCH is being removed in
  favor of explicit dependencies.
- The extracted `rbx-render-materials`, `rbx-render-text`, and `rbx-render-ui`
  targets now build as C++20 libraries. `rbx-render-scene` also builds all of
  the moved VisualEngine/scene implementation on macOS arm64 after explicit
  dependency, ownership-boundary, and Apple-Silicon portability repairs.
- Moved the complete reflection, XML serialization, and DataModel tree source
  sets into owned engine modules with compatibility headers. Their C++20
  targets compile on macOS arm64.
- Moved all tracked `App/v8datamodel` headers and implementations out of the
  legacy tree. The complete 214-header/197-implementation runtime now compiles
  as `Roblox::DataModelRuntime` without the historical precompiled header.
- Corrected SDL's generic configuration header to select its existing macOS
  configuration on Apple, while preserving the Windows configuration path.
  Windows-only VMProtect instrumentation is now explicitly platform-guarded.
- Restored a functional FastLog variable registry with typed static, dynamic,
  synchronized, and experiment variables, including mutation, lookup,
  enumeration, pending overrides, and synchronized-state tracking.
- Moved the complete tracked `GfxBase` source into
  `engine/rendering/foundation`. Render settings, caps, statistics, frame-rate
  management, adorn contracts, and part/render helpers now compile as the named
  `Roblox::RenderFoundation` target.
- Moved all tracked `v8world`, `v8kernel`, and solver headers and sources into
  `engine/simulation`. The 118-header/93-implementation world, physics kernel,
  contact/joint pipeline, spatial structures, and PGS solver compile together
  as `Roblox::Simulation` without the historical App precompiled header.
- Moved the authentic Animator, tracks, keyframes, poses, controllers, and
  animatable-joint source into `engine/animation`; `Roblox::Animation` compiles
  the complete moved set. This is the source foundation, not yet R15 runtime
  acceptance.
- Moved the complete tracked G3D/RbxG3D math source into `engine/core/math` and
  built it as `Roblox::Math`, including Apple-Silicon portability repairs.
- Added a compiling `Roblox::Physics` migration target for the checked-in
  Bullet collision/LinearMath/convex-decomposition source. Replacing this old
  copy with a maintained current Bullet dependency remains required.
- Moved the complete tracked AppDraw and interaction-tool source sets into
  `engine/rendering/adorn` and `engine/interaction/tools`. Both named C++20
  targets compile without the App precompiled header; their remaining coupling
  to DataModel is recorded for later boundary cleanup.
- Moved the complete tracked runtime GUI/widget source into `engine/ui/runtime`.
  The authentic chat, score HUD, layout, drawing, filtering, and widget code now
  compiles as `Roblox::RuntimeUI` without the App precompiled header.
- Moved the complete tracked script layer and embedded Roblox-modified Lua VM
  into `engine/scripting`. The client VM variant and authentic CoreScript,
  ScriptContext, debugger, analyzer, bridge, module, event, and thread-reference
  implementations compile together as `Roblox::Scripting` on Apple Silicon.
- Added the historical FreeType source as an explicit text dependency so the
  moved renderer links against a named target. Maintained FreeType plus
  HarfBuzz and Unicode support remain required for acceptance.
- Isolated macOS clipboard access in `platform/macos` behind the shared platform
  contract instead of compiling AppKit syntax into DataModel C++.
- Added a real main-framebuffer contract to the bgfx backend using native pixel
  dimensions, target-specific view rectangles, correct current-view clears,
  screenshot callbacks, and suspend/resume loss/restoration notifications.
  The Metal integration test exercises these paths and the explicit offline-mip
  policy.
- `RobloxPlayer` now initializes and submits Metal frames through the migrated
  GfxCore `Device::API_Bgfx` path. The temporary parallel `BgfxRenderer` API,
  implementation, target, and trivial test have been removed; focused GfxCore
  contract tests replaced them.
- Linked the complete VisualEngine scene stack into `RobloxPlayer`, initialized
  a real DataModel and Workspace, deserialized the preserved
  `FirstPlaceBasePlate.rbxl`, and rendered it through bgfx/Metal. The headless
  acceptance run completes 120 frames, reports one scene batch with 264 faces
  and 528 vertices plus three last-frame draws, and produces a visibly lit,
  studded baseplate against the sky rather than a clear-only frame.
- Recovered the legacy shader pack's logical sampler stages while compiling all
  135 scene programs to validated Metal binaries. Correct global matrix upload,
  culling, physical texture stages, render-target mip resolve, framebuffer
  readback, and shader-pack relink dependencies are now implemented in the
  bgfx GfxCore backend and Player build.
- Added `player-baseplate-metal-render`, a live render regression test that
  loads the packaged RBXL and rejects flat, black-geometry, missing-sky, or
  insufficiently textured output. Project tests are now enabled directly by
  `RBX_BUILD_TESTS` instead of inheriting a dependency's `BUILD_TESTING` cache
  value. `gfxcore-contract`, `gfxcore-bgfx-metal`, and the Player render gate
  pass together; the live Player exits cleanly after its scheduler threads join.
- Restored four files missing from the checked-in Boost 1.74 header subset from
  the matching upstream release so the compile frontier can proceed.
- Built pinned upstream `shaderc` and compiled an owned UI program to SPIR-V,
  GLSL, GLES, and Metal artifacts with warnings treated as errors.
- Generated a 673-entry SHA-256 resource manifest and packaged preserved Roblox
  source content, shader artifacts/manifests, and notices inside the `.app`.
- Added a deterministic, scoped reference inventory and measured the authentic
  in-game Player delta: 1,439 current core UI files and 114 selected
  in-experience ExtraContent files, with standalone app UI machine-excluded.
- Added a hash-verifying in-game UI importer and packaged 1,753 Player payloads
  under a versioned overlay. `rbxasset://` resolution now has an owned mount
  table with precedence, traversal/symlink containment, and density variants.
- Statically inspected the hashed 2026 Windows Player executable and recorded
  binary offsets plus MSVC reflection/RTTI evidence for ScreenGui safe areas,
  preferred input, automatic sizing/state, CanvasGroup, and CoreGui/player-list
  configuration. A deterministic contract-report tool tracks source identifier
  coverage without claiming behavior from names alone.
- Extended that tool with PE section/VA mapping and direct reference recovery,
  then used the APK's matching, analyzable x86-64 native library to recover the
  `AutomaticSize` registration/getter/setter contract. The owned layout helper
  now implements `None`, `X`, `Y`, and `XY` growth, authored lower bounds, and
  parent clamping with focused tests. The real reflected `GuiObject` property
  measures visible child content, raises changes, and invalidates both its own
  layout and an automatically-sized parent; the full DataModel archive compiles.
- Recovered the getter-only `GuiState` descriptor, exact enum values, direct
  member getter, and registration offsets from the APK native library. The real
  `GuiObject` now exposes `Idle`, `Hover`, `Press`, and `NonInteractable`, maps
  its existing captured mouse states without discarding their detail, and
  emits property changes; focused tests cover state precedence.
- Implemented the binary-confirmed `ScreenGui` inset/safe-area reflection
  surface and viewport resolution in the actual DataModel render path, with a
  portable layout target and focused tests.
- Implemented the binary-confirmed read-only `UserInputService.PreferredInput`
  surface using the exact `KeyboardAndMouse`, `Gamepad`, and `Touch` enum names.
  Real input events update the family and raise property changes; the binary's
  observed touch-delay policy remains research work rather than a guessed claim.
- Moved the real ContentProvider/cache/mesh/texture provider implementation
  family beside its already-migrated DataModel headers. ContentProvider now
  compiles in the named runtime target and shares the same ordered asset mount
  table as the Player bootstrap instead of maintaining a parallel resolver.
- Extended the platform-neutral native surface with device safe-area insets;
  the macOS adapter reports AppKit safe-area values in physical pixels.
- The completed debug app bundle now validates after resource packaging under
  `codesign --verify --deep --strict`, and its executable is arm64.
- The Player bootstrap and ContentProvider now share one asset mount table. A
  packaged headless run resolves `PlayerList/ViewAvatar@2x.png` from the
  `player-core` overlay, proving density selection and overlay precedence from
  inside the signed app; this is resource-path evidence, not full CoreGui render
  acceptance. Preserved PC `PlatformContent` is packaged as its own mount.
- Added an enforced root-layout manifest covering every top-level source
  directory. Include checker, settings comparison, policy refresh, CoreScript
  converter, model analyzer, script signer, test hooks, and the simulation
  regression corpus have moved under `tools/` or `tests/` with affected
  Visual Studio/Xcode references updated. No source was deleted.
- Moved the historical deployment wrapper projects under their owning Player,
  Studio, service, and tool directories, replacing depth-dependent MSBuild
  paths with repository-root references. Moved all root-level DataModel/core
  unit tests and support libraries, the 562-file Player integration suite,
  managed tests, RCC/thumbnail tests, mobile tests, and their runners under the
  organized `tests/` tree. The root-layout audit now classifies 42 source
  directories, down from 62 before these two batches; no source was deleted.
- Moved the complete historical macOS Player shell and its Xcode project under
  `apps/player/macos`, and its cross-project workspace under
  `platform/macos/historical`. Root-relative project semantics and external
  Android/tool references were updated; both `xcodebuild -list` project and
  workspace validation pass. This is organization of preserved source, not a
  return to the obsolete renderer or a macOS-only product architecture.
- Moved the historical Windows Player project under `apps/player/windows/source`
  and its shared Win32 implementation under `platform/windows/historical`.
  Player, Studio, RCC, installer, integration-test, deployment, solution, and
  CSG references now point to the organized platform location using stable
  solution-root paths. The preserved project remains excluded from the new
  default Player graph while serving future Windows behavior bring-up.
- Moved the complete hybrid mobile bridge under `platform/mobile/hybrid`, the
  Xbox shell under `platform/xbox/client`, and the Xbox network-mesh sample
  under `third_party/samples`. Android/iOS hybrid references, the JavaScript
  packaging path, Xbox project dependencies, and solution entries now resolve
  to those owned locations; the console sample remains excluded from Player.
- Moved the complete 1,042-file historical Studio source family and both plugin
  trees under `apps/studio`. Visual Studio, Xcode, workspace, deployment, and
  plugin-copy references now use the organized locations. Studio remains
  excluded from Player and is retained only for its separately scoped tooling
  and authorized R15/RBXL/schema interoperability work.
- Moved the complete RCC service and managed service-arbiter applications under
  `apps/services`, alongside their already organized deployment wrappers.
  Solution, MSBuild, managed dependency, Win32 platform, scripting, deployment,
  and RCC test-launch paths were updated without adding either service to the
  Player graph.
- Moved the root `PropertySheets` directory and all loose
  `CustomBuildRules.*` files under `cmake/historical/msbuild`. Every affected
  Visual Studio import now uses a stable solution-root path, reducing both
  top-level directories and loose build metadata without deleting it.
- Moved the complete Studio-only QTitanRibbon tree under
  `third_party/studio/qtitan_ribbon`; Studio Visual Studio and Xcode references
  resolve to the new location, while the dependency remains outside Player.
- Moved the historical Boost build-project metadata under
  `third_party/boost/build`; all Visual Studio, Xcode, test, tool, Player,
  Studio, service, and Xbox references were updated without moving or deleting
  the separately audited library source corpus.
- Moved the proprietary FMOD corpus intact under
  `third_party/quarantined/fmod`. Every CMake, Android, iOS, macOS, Windows,
  Xbox, Studio, service, tool, and test path was updated so legacy compilation
  checkpoints remain possible, while the final Player replacement/removal gate
  remains explicit and no FMOD file was deleted.
- Replaced the misleading unbuilt audio-runtime CMake definition with the real
  pinned miniaudio 0.11.25 mixer target. Its engine now has generation-safe
  clip/voice handles and deterministic offline coverage for playback,
  pause/resume, seeking with bounded mixer latency, volume, looping, lifetime,
  inverse/linear distance attenuation, stereo panning, and independent cursors
  for voices sharing a decoded clip.
- Added encoded in-memory audio decoding with corruption and resource-limit
  rejection, bounded priority-based voice stealing, and master gain/mute tests.
  Updated third-party notices and added a packaged SPDX 2.3 inventory covering
  the pinned default open-source graph.
- Added a requirement-by-requirement compatibility report for the one selected
  Backrooms RBXL/RBXLX scenario, including its modern UI class inventory, three
  Sound contracts, offline asset policy, evidence strength, and remaining real
  serializer/runtime/render acceptance gaps.
- Converted and compiled the complete real
  `Sound`, `SoundChannel`, `SoundService`, and `SoundWorld` DataModel family
  beside the replacement runtime. The selected acceptance place's three legacy
  sounds now have current reflection contracts for playback speed, playing,
  rolloff bounds/mode, regions, and acoustic simulation. Playback is not yet
  connected to the foundation Player, so this remains integration evidence
  rather than completed Player audio.
- Removed all FMOD headers, types, and symbols from the converted audio archive
  and its active engine consumers while retaining the quarantined historical
  corpus and Windows capture implementation intact. `SoundService` now drives
  the replacement listener, voices, master gain/mute, and a real CoreAudio
  output device; deterministic tests and a live device start/stop smoke test
  pass. The VisualEngine link probe includes this DataModel audio archive and
  has no FMOD middleware unresolved symbols; its unrelated engine backlog is
  now 565 unresolved symbols.
- Moved the complete 633-file networking and RakNet family under
  `engine/networking`. All modern CMake include roots plus historical Visual
  Studio/Xcode app, platform, service, tool, and test references now target the
  organized module; logical `Network/...` public includes remain stable.
- Moved the complete Player host-support, shared-client, and Xbox in-game chat
  families under `apps/player/shared` and `engine/ui/chat`. CMake, Visual
  Studio, Xcode, service, Studio, platform, and test consumers now use their
  owned paths; the in-game chat family remains distinct from standalone app UI.
- Rebuilt the complete 337-step DataModel runtime archive after the networking
  and quarantined-FMOD moves; `librbx-datamodel-runtime.a` linked successfully.
- Replaced RakNet in the active networking graph with pinned
  GameNetworkingSockets 1.5.0. Replication now uses owned endpoint, packet,
  message-ID, priority, reliability, time, statistics, bounded packet-buffer,
  and handler contracts; no RakNet source or include path is present in the
  `rbx-networking-runtime` CMake target, and its archive exports no RakNet
  symbols. The historical RakNet corpus remains preserved and unmodified for
  the explicit final deletion gate.
- Replaced the Windows debug network profiler's RakNet SQLite and packetized-TCP
  logger with an engine-owned local diagnostic logger. The profiler is compiled
  as part of the macOS networking proof to keep that normally Windows-only path
  build-checked, and the resulting runtime archive contains no RakNet, RakPeer,
  SQLite-client-logger, or packetized-TCP symbols.
- Added native IPv4/IPv6 listen/connect/accept/close, asynchronous hostname
  resolution with stable engine-owned connection IDs and sequential fallback
  across every unique resolved address, cancellation of pending resolution and
  retry state without late events, cleanup of per-connection traffic state,
  reliable and unreliable delivery, eight ordered lanes, connection events,
  metrics, timeout control, and send-rate control through the owned transport
  boundary. The native loopback and packet-buffer contract tests pass 20
  consecutive runs; this is transport evidence, not yet a full client/server
  DataModel replication run.
- Completed the full client/server DataModel replication checkpoint over the
  owned GameNetworkingSockets transport. The integration starts independent
  heartbeat-enabled server and client DataModels, completes the normal
  Replicator join, then proves initial instance creation, live Name/CFrame/
  BrickColor changes, authoritative removal, and three ordered round-trip
  markers through the production replication queues. Two idle-progress defects
  found by this test were fixed in the runtime: GNS outgoing frames now flush
  independently of inbound polling, and packet receive/send jobs retain bounded
  progress after the join burst instead of relying on a losable scheduler wake.
  Character auto-loading is explicitly disabled in this replication-only test
  so a missing avatar web endpoint cannot turn a transport assertion into a
  character-loading failure. The clean test passed 20 consecutive runs.
- Network send failures from the asynchronous replication job are now reported
  with the logical connection ID and the transport error instead of being
  silently discarded; null packet submissions are rejected before queueing.
- Replaced the open-source GNS default unauthenticated-IP policy with an
  explicit engine policy. Outbound and accepted remote IP peers must
  authenticate; only loopback may remain unauthenticated for offline tests.
  The transport accepts coordinator-issued GNS certificate blobs and exposes
  authentication state in connection events and metrics. Production
  certificate issuance, rotation, and Player/backend delivery remain to be
  integrated and verified.
- Exposed the real `ISteamNetworkingSockets::GetCertificateRequest` flow
  through the engine Transport and ConcurrentPeer contracts. It performs the
  required size query, bounds the coordinator payload to 64 KiB, obtains the
  actual private-key-bound request blob, and clears partial output on failure;
  the existing `SetCertificate` path consumes the coordinator's signed reply.
  The native transport test now requires a nonempty real request before its
  loopback handshake, and full DataModel replication remains green. A deployed
  coordinator issuer and expiry-driven rotation policy are still required for
  production remote authentication.
- Pinned GameNetworkingSockets, protobuf 35.1, and Abseil 20250512.1 to
  immutable commits. Windows selects BCrypt; non-Windows cross-builds require
  an explicit target-architecture OpenSSL root and host protoc, preventing host
  libraries or target executables from being used accidentally. A guarded
  upstream patch enables the pinned GNS CMake platform branches for iOS and
  Android. The upstream GNS static library and the owned transport adapter now
  cross-compile for iPhoneOS arm64 at deployment target 15.0; both archives
  were inspected as arm64. Android remains uncompiled because no Android SDK or
  NDK is installed locally.
- Added a reproducible OpenSSL 3.5.7 LTS source builder pinned to immutable
  commit `8cf17aaeb4599f8af87fefd810b5b5fee90fe69e`. Verified static target builds
  now cover macOS arm64 at deployment target 13.0 and iOS arm64 at deployment
  target 15.0. The macOS preset consumes this pinned tree instead of the
  incompatible Homebrew archive that had been built for macOS 15.
- Moved the complete foundational `Base` tree into `engine/core`, separating
  public headers, owned implementation, and preserved historical project
  metadata. Updated every CMake, Xcode, Visual Studio, solution, app, service,
  platform, renderer, and test include path; no root compatibility directory
  remains. The owned core target now compiles the actual scheduler, timing,
  logging, profiling, system, assertion, and utility implementations rather
  than only a bootstrap source. During the arm64 proof, replaced the obsolete
  Apple profiler shim that injected types into `std`, removed its direct
  OpenGL include, corrected Intel-only macOS detection, and replaced the
  private CoreFoundation version query. `rbx-core` and the signed
  `RobloxPlayer.app` both link successfully after the move.
- Moved the complete 30-source scalar/value-type implementation set out of the
  mixed `App/util` directory into `engine/value_types/src`, including Content,
  Font, BrickColor, regions, UDim, physical properties, identifiers, hashes,
  and math-facing engine values. The owned `rbx-value-types` archive now builds
  entirely from its subsystem directory, and the actual signed Player relinks
  against the moved implementation without a compatibility source path.
- Moved the complete 44-source implementation set already owned by
  `Roblox::RuntimeSupport` out of the mixed `App/util` tree into
  `engine/runtime_support/src`, preserving the HTTP subdirectory and the
  platform-specific sources that still require separate classification. The
  named archive now compiles solely from its subsystem directory instead of
  reaching back into the legacy utility dump.
- Moved the five common and Darwin implementations already owned by
  `Roblox::PlatformRuntime` into `platform/common/src` and
  `platform/macos/src`. The macOS runtime archive no longer compiles active
  filesystem, memory-statistics, or string-conversion code from `App/util`.
- Moved the complete 355-file CSG family under `engine/geometry/csg` and
  updated its Player, Studio, Xbox, model-analyzer, converter, solution,
  Visual Studio, and Xcode consumers. Added the named
  `Roblox::GeometryCSG` CMake target and compiled both that target and the
  preserved Xcode archive on arm64. The compile proof also removed the SDK's
  hard-coded Windows DLL platform selection, replaced its i386/C++11 project
  settings with arm64-capable C++20 settings, and pointed CSG at the organized
  DataModel and math include surfaces. No CSG source or prebuilt artifact was
  deleted.
- Moved the complete 556-file checked-in Bullet corpus from the root into
  `third_party/bullet_historical`; `engine/physics` remains the owned engine
  boundary. Updated modern CMake plus preserved Player, Studio, service,
  renderer, networking, mobile, Xbox, test, tool, solution, Visual Studio,
  and Xcode consumers. The core math header now uses the Bullet public include
  surface instead of reaching through a repository-relative legacy path. No
  Bullet source, project, or prebuilt artifact was deleted; replacing this
  historical snapshot with a maintained pinned upstream remains an acceptance
  task.
- Moved all tracked humanoid state-machine sources and headers from `App` into
  `engine/avatar/humanoid`, updated the historical App projects, and added the
  named `Roblox::AvatarHumanoid` CMake target. The full 17-source target now
  compiles against the real DataModel, simulation, physics, animation, UI,
  networking, reflection, and rendering-foundation headers. Its historical
  VMProtect include and mutation/virtualization calls were replaced with an
  owned portable no-op marker interface, removing that proprietary dependency
  from this gameplay subsystem while preserving control flow. This is a build
  and organization checkpoint, not proof of complete R15 behavior.
- Merged the remaining tracked App security headers and implementations into
  `engine/security` and added the named `Roblox::Security` target below
  reflection in the dependency graph. Removed legacy precompiled-header
  coupling and the unused VMProtect SDK include from `FuzzyTokens`; the owned
  security archive, full DataModel archive, and humanoid archive compile
  together. Historical security behavior remains preserved, while proprietary
  code-marker middleware is no longer a dependency of these owned modules.
- Moved the checked-in seven-file LZ4 snapshot from `App` into
  `third_party/lz4_historical`, exposed it only through the named
  `Roblox::Lz4` target, and linked serialization and scripting explicitly.
  The complete 424-step scripting dependency graph rebuilt and linked after
  the move. This is historical-source organization, not the still-required
  maintained pinned compression replacement.
- Moved all 33 tracked legacy-voxel and smooth-terrain headers and sources from
  `App` into `engine/terrain`. The dependency graph now separates shared grid,
  material, water, and meshing code (`Roblox::Terrain`) from DataModel-bound
  serialization/voxelization (`Roblox::TerrainRuntime`) instead of retaining a
  simulation/DataModel cycle. Both archives compile on arm64. Removed their
  App precompiled-header dependence by adding explicit standard/Boost headers,
  and corrected `Voxelizer` so SSE2 is selected only on x86/x64 rather than on
  every desktop Apple architecture.
- Moved the complete 12-file public Lua header family from `App/include/lua`
  into `engine/scripting/include/lua`, updated CMake, Visual Studio, and Xcode
  ownership, and normalized scripting consumers to the filesystem's actual
  include case. The scripting archive and arm64 Player checkpoint build, all
  11 focused tests pass, and the Player bundle passes strict code-sign
  verification after the move.
- Moved loose historical shader scripts, documentation cleanup script, lint
  script, debugger visualizers, Player plist, and the duplicate root managed
  test descriptor under their owning `tools/`, `apps/player/macos`, and
  `tests/integration/managed` directories. The duplicate descriptor was
  preserved with a distinct filename rather than deleted.
- Debug and release arm64 targets compile; the focused test suite passes 12/12. The release
  foundation bundle validates under ad-hoc signing and has system-only dynamic
  dependencies.
- Refreshed the release cache from the checked-in reference-aware preset,
  regenerated and packaged the 1,553-file scoped Player overlay, and passed the
  release headless checkpoint on bgfx/Metal with `player-core` winning 2x asset
  resolution. This verifies the foundation shell and packaging, not the pending
  VisualEngine/DataModel acceptance path.
- Verified the supplied APK hash and recorded static mobile/touch/density/Vulkan
  observations without installing it or extracting it into source. Hashed
  x86-64 and arm64 native libraries are isolated under generated `out/research`
  for offline implementation analysis.
- Backported the current two-`UDim` `UDim2.new(x, y)` constructor used directly
  by Foundation. The 2016 bridge had silently coerced both userdata arguments
  to zero, leaving the supplied PlayerList Foundation close button present but
  with a `0x0` authored and hit-test size. The same genuine component now
  resolves to `24x24`. A 400-frame packaged Player run exercised the supplied
  Chrome hamburger, supplied Leaderboard action, populated local-player row,
  supplied close button, and Chrome reopen action. Native state assertions
  proved open, closed, and reopened states; the final Metal proof contains the
  dark rounded PlayerList panel, its working X icon, `People` header, and
  populated `Player` row. The standalone Luau UI/component target now links
  against the same scripting runtime as Player and passes its direct
  constructor and engine-contract regressions.
- Corrected Instance lower-camel member resolution to test the capitalized
  candidate against the concrete object rather than against the process-wide
  interned-name table. The supplied `ErrorPrompt.lua` legitimately reads
  `child.name`; the globally interned word `name` previously prevented that
  access from resolving to `Instance.Name` and aborted `ScreenTimeInGame` while
  iterating a `UIGridLayout`. The unchanged supplied CoreScript now initializes
  past this point, and the complete leaderboard interaction proof still passes.
- Added repeatable performance instrumentation to the signed packaged Player.
  It reports load time and p50/p95/p99 CPU frame-processing time separately
  from the synthetic headless sleep, at the actual native/logical surface
  resolution and renderer. The measured steady gameplay window excludes the
  60-frame warmup, final blocking semantic inspections, and post-frame-245
  diagnostic UI/emote operations. Three consecutive default-R15 Metal runs at
  2560x1440 native / 1280x720 logical resolution produced 185 samples each:
  p50 0.629-0.670 ms, p95 2.938-3.923 ms, p99 3.702-4.476 ms, with zero frames
  over either 16.67 ms or 33.33 ms. Load time was 1287.60-1548.36 ms. Every run
  rendered all 300 verification frames and exited successfully; the completed
  app then passed strict deep code-signature verification.
- Replaced the foundation Player's legacy camera/control bootstrap with the
  exact current Studio `PlayerScriptsLoader` and complete 42-source
  `PlayerModule` graph from version `ed7d8193e8564b1f`. An owned deterministic
  importer converts the exact filesystem hierarchy into a normal RBXMX model,
  records every source hash, and packages it inside the actual signed
  `RobloxPlayer.app`; the packaged model SHA-256 is
  `3eccab7e55e185c2d67aa5b4d5a9d2a9cc586fa376c066d00f894eb590c8d836`.
  The runtime now creates only the current `PlayerScriptsLoader` and
  `PlayerModule` pair when that package is available, retaining the historical
  scripts solely as a source-build fallback rather than running two control
  stacks in parallel.
- Closed the exact module's native camera/control dependencies through real
  engine contracts: `CFrame.fromOrientation` and lower-case vector aliases,
  current developer camera/movement enum items, `Camera.NearPlaneZ`, stored
  `BillboardGui.LightInfluence`, stored `GuiService.TouchControlsEnabled`, and
  `OverlapParams` with real filtered oriented-box/radius Workspace queries.
  `RunService:BindToRenderStep` now supplies the frame delta, keeps anonymous
  module callback threads alive for the binding lifetime, and releases those
  pins before Lua-state teardown. This fixed both the frozen stock camera and a
  shutdown use-after-close exposed by the genuine module.
- The actual release application now passes the current PlayerModule path with
  no PlayerScripts errors: default R15 and R15-plus both complete 300 Metal
  frames, move, rotate through native right-drag input, load all 14/14 and
  15/15 exact meshes respectively, invoke the authentic `WaveAnim`, and exit
  cleanly. Both selected Backrooms encodings independently pass the same
  package with their legitimate place-owned Scriptable camera, 14/14 meshes,
  authentic wave, 300 frames, and 66 final draws. Strict deep code-signature
  verification passes, and no build or Player process remains running.

## Not yet acceptance evidence

The current Player proves VisualEngine, DataModel place loading, scene shaders,
resource packaging, Metal headless rendering, the selected modern pair's
non-UI gameplay path, and the supplied standard R15/R15-plus hierarchy,
MeshPart rendering, scaling, skinning, movement, animation-controller, and
emote paths. It does not yet satisfy full CoreGui rendering, complete current
audio-graph parity, or complete cross-platform verification. The native GNS adapter has an iOS
arm64 archive proof, but no complete non-macOS Player has been built or
verified.

## Next buildable checkpoint

Active priority changed on 2026-07-18: complete all remaining non-UI scope
before returning to the authentic 2026 in-experience UI. The UI implementation
remains the genuine Player package plus its native engine dependency closure;
screenshot recreations and compatibility mocks are not acceptable. Its paused
checkpoint is recorded in `docs/reverse-engineering/in-experience-ui.md`.

1. Continue classifying and migrating the remaining mixed `App/util` sources
   into subsystem-owned modules without recreating a global utility dump or
   breaking preserved projects. The Lua public surface is now scripting-owned.
2. Continue text/fonts, streaming/current-graph audio parity, launcher, and interactive
   Metal acceptance in buildable checkpoints.
3. Integrate and verify the production networking certificate issuance/rotation
   path, and build the native Windows/Linux/iOS/Android matrices. Implement the Web
   transport with WebRTC behind the same engine contract; browser builds cannot
   use the native UDP GNS adapter.
4. Return to Chrome, PlayerList, ESC pages, ExperienceChat, Backpack, and the
   remaining genuine package-backed UI closure only after the non-UI work is
   complete. Add the requested `Enable legacy UI` setting only after the 2026
   UI is complete, then perform semantic, perceptual, and interactive
   verification before acceptance.

## Blockers

None external. The actual VisualEngine now links and runs inside the Player.
The remaining items are implementation and verification tasks, not unresolved
linker prerequisites or external blockers.

## Renderer source deletion gate

The historical D3D9, D3D11, and OpenGL renderer sources remain present and are
only excluded from the default build. They must not be removed until all stated
verification is complete and the user separately gives explicit permission.
- Backported the current `CFrame.fromMatrix(position, right, up[, back])`
  constructor with the engine's right/up/back column convention and implicit
  `right:Cross(up)` back vector. This removes the real `RagdollRigging` load
  failure and allows the supplied `PlayerRagdoll` CoreScript to mount.
- Recovered `MakeupType`, `MarketplaceItemPurchaseStatus`,
  `PromptPublishAssetResult`, and `RaycastFilterType` directly from the exact
  `version-ed7d8193e8564b1f` Studio API dump, including numeric values and the
  Blacklist/Whitelist compatibility names. The packaged run now progresses
  beyond the former Makeup and marketplace enum failures; PublishAssetPrompt
  receives its complete result enum rather than a compatibility table.
- Restored the typed `PromptBulkPurchaseRequestedV2` MarketplaceService event
  with its seven exact API-dump payload fields, the complete
  `MarketplaceBulkPurchasePromptStatus` enum, and SocialService's real
  `OpenShareSheetWithLink(string)` event. These are engine-owned signals used
  by the supplied React/CoreScript listeners; they are not Lua substitute
  events. The packaged runtime now mounts those listeners without either
  missing-member error.
- Implemented native `RaycastParams`, `RaycastResult`, and
  `Workspace:Raycast` contracts through the existing ContactManager query
  path. The query now provides real instance/position/normal/material/distance
  results, 15,000-stud and finite-vector validation, descendant Include and
  Exclude filters, `IgnoreWater`, and `RespectCanCollide`. The supplied
  `AvatarContextMenu` no longer fails at `RaycastParams.new`; its real GUI now
  mounts in CoreGui. Fixed the Reflection-to-Luau return dispatcher so typed
  `RaycastResult` values are returned instead of falling through to nil. Rays
  beyond the legacy contact manager's 5,000-stud limit are now segmented up to
  the current 15,000-stud API limit, and `BruteForceAllSlow` runs a genuine
  all-world-primitives query. Three consecutive native runs prove matching
  fast/slow results against a target more than 6,000 studs away.
- Backported the current 32-group collision matrix into the real simulation
  path. `PhysicsService` now owns registration, rename/unregister, symmetric
  collidability masks, current and deprecated query APIs, and registered-group
  tables. `BasePart.CollisionGroup`/`CollisionGroupId`, contact resolution, and
  `Workspace:Raycast` all consume the same matrix. A native Luau regression
  proves group registration/listing, BasePart assignment, Default-vs-group
  ray exclusion, same-group hits, matrix re-enable, rename propagation, and
  unregister migration back to Default.
- Disabled the obsolete FormFactor size conversion by default. A current
  `Part.Size = Vector3.new(4, 4, 4)` now produces a true 4-stud physics shape
  instead of silently rounding Y to three legacy 1.2-stud brick units; the
  native raycast proof reaches the correct `y=2`, normal, and distance.
- Restored `Content.fromObject(game).Object` identity by returning Instance
  content through the canonical Instance bridge, and added the current
  `CFrame:GetComponents()` alias over the existing 12-value decomposition.
- Implemented the real version-1 `Workspace.CollisionGroupData` place format
  from three hash-recorded 2026 RBXLX references. The strict atomic reader and
  writer preserve exact group IDs, four-byte little-endian masks, and names;
  malformed versions, counts, IDs, duplicates, mask widths, Default placement,
  truncation, trailing bytes, and asymmetric matrices fail explicitly. The
  normal XML `Serializer` test loads the exact current three-group payload,
  round-trips it byte-for-byte, and proves the imported matrix changes actual
  `Workspace:Raycast` results. The packaged Player then loaded the downloaded
  2026 Baseplate for 400 Metal frames, exercised Chrome/PlayerList, wrote
  `/tmp/collision-data-baseplate-proof.ppm`, and exited 0.
- Native inspection of that historical proof confirmed compatibility progress,
  not UI/visual acceptance: genuine Chrome and populated PlayerList were visible,
  while Baseplate/sky still rendered as fallback solid colors at that checkpoint.
  The later indexed-render/content-ID/image-format fix and dedicated pixel gates
  above supersede that asset-rendering observation; menu/leaderboard geometry
  remains deferred and incomplete.
- Implemented the current reflected `Decal.Color3` property, inherited by
  `Texture`, and replaced the renderer's hard-coded white decal vertices with
  the serialized tint. A normal current-form RBXLX serializer regression proves
  `(0.125, 0.5, 0.875)` survives loading on a Decal using the packaged
  SpawnLocation content path. The unit executable passed three consecutive
  runs; all 7 in-experience Python contracts passed; the signed packaged Player
  loaded the downloaded 2026 Baseplate, exercised genuine Chrome and populated
  PlayerList for 400 Metal frames, wrote
  `/tmp/decal-color-baseplate.ppm`, and exited 0.
- At this historical checkpoint the supplied Baseplate's numeric Baseplate and
  Sky asset IDs had no matching bytes in the inspected Player/Studio/APK corpora
  or then-empty cache. No unrelated legacy image was installed as a substitute.
  The later canonical asset-delivery fix retrieved the exact referenced bytes
  and the persistent-cache verifier above now proves subsequent offline starts.
- Implemented the current public `Texture.OffsetStudsU/V` behavior from the
  hash-recorded official 2026 documentation and offset reference images.
  Reflected properties now deserialize through the normal RBXLX path, trigger
  live graphics invalidation, and translate real generated UVs consistently
  across block, wedge, torso, sphere, cylinder, and CSG surface paths. A native
  render-scene contract generates an actual textured block and proves the
  recovered right/up offset convention on every emitted vertex. The unit
  executable passed three consecutive runs, all 7 in-experience Python
  contracts passed, and the signed packaged Player loaded the downloaded 2026
  Baseplate and rendered 400 Metal frames with genuine Chrome and a populated
  PlayerList before exiting 0. This closes the public Texture offset behavior;
  it did not at that checkpoint claim the remote Baseplate image or Sky bytes
  existed offline; the later cache-backed render proof above supersedes that
  limitation for assets which have been fetched once.
- Corrected the joined-client LocalScript lifecycle instead of hiding place
  errors. Normal scripts are held pending while the join replicates globals,
  `game.Loaded`, and `LocalPlayer.Character`, then released through the fixed
  `ScriptContext.ScriptsDisabled` transition. The selected Backrooms place no
  longer starts `BobbingCamera` against a nil Character. R15 spawn placement
  now consumes the safe-volume center directly while retaining the historical
  seven-stud lift for R6; this prevents enclosed modern places from resolving
  the avatar onto the roof. A reviewed 240-frame Metal capture shows the
  character inside the actual place geometry rather than empty sky/floor.
- Added the genuine legacy `Workspace:FindPartOnRayWithWhitelist` compatibility
  contract over the modern ContactManager-backed ray query. The exact current
  PlayerModule Popper no longer throws once per frame in the 2016 Starter Place.
  The packaged Player completed 300 frames with 6.93 studs of character motion,
  a 1.97 right-drag camera look-vector delta, and 115 final-frame draws. Happy
  House independently completed with current PlayerModule camera rotation,
  authentic R15 wave playback, and 14/14 loaded character MeshParts.
- Added the client-owned `.rbxlp` offline place package format. Its deterministic
  container embeds one unmodified RBXL/RBXLX, numeric assets under
  `assets/<id>.<format>`, and a provenance manifest; the reader enforces bounded
  counts/sizes, safe relative paths, CRC-32 payload integrity, exactly one place,
  atomic private-cache materialization, and a dedicated numeric asset mount
  ahead of base content and HTTP delivery while exact client runtime overlays
  retain precedence. The packager independently rechecks every
  Roblox Asset Archiver payload SHA-256 and rejects incomplete archives by
  default. Native mount/package tests cover asset-ID resolution, traversal
  rejection, and corruption rejection.
- Scanned the selected Backrooms RBXLX with the user-supplied Roblox Asset
  Archiver: 96 numeric IDs were found, 95 authorized payloads downloaded and
  hash-recorded, and ID `9702238603` was explicitly rejected by delivery as
  archived (it is recorded as missing, never substituted). The resulting
  18 MiB local package has SHA-256
  `8da084bdec97423c3c1d927bfc0508ae240ee58c5d9afe5da1f0a194e6ff334d`.
  The actual packaged `RobloxPlayer.app` materialized it from the `.rbxlp`,
  mounted all 95 embedded assets, rendered 240 Metal frames inside the place,
  wrote the reviewed `backrooms-embedded-assets.png`, and exited 0.
- Imported the exact Normal and Slender Rthro rigs from Studio build
  `ed7d8193e8564b1f` and archived all 30 referenced MeshPart payloads with
  pinned source paths, sizes, and SHA-256 values. The build now verifies those
  records and packages the authentic meshes into the actual signed
  `RobloxPlayer.app`; neither rig uses substituted geometry.
- Completed the live Rthro scale path instead of treating the Studio previews
  as static models. Replicated non-R6 characters consistently become R15,
  late-arriving Humanoid scale values reconnect and rescale the skeleton,
  `RootRigAttachment` follows the current Normal/Slender proportions, current
  R15 HipHeight semantics keep the feet above the floor, and preview-authored
  collision flags are normalized to the live R15 convention. Native model
  contracts cover exact hierarchy, interpolation, root-attachment offsets,
  meshes, and joints.
- The actual packaged Player passed three consecutive 300-frame runs for both
  Rthro variants. Normal loaded 15/15 meshes (5,693 vertices and 5,614 faces),
  Slender loaded 15/15 meshes (4,718 vertices and 5,452 faces), and both moved
  more than 5.4 studs while accepting and observing the authentic Wave
  animation. Four consecutive R15-plus runs also passed genuine PlayerModule
  movement, camera drag, and Wave playback. The five focused avatar model and
  packaged-runtime CTest targets pass together.
- Added the genuine public `Humanoid:PlayEmoteAsync` yielding surface and the
  RobloxScript-only `Humanoid.EmoteTriggered(emoteSuccess, emoteTrack)` event.
  The implementation invokes the exact character `Animate.PlayEmote`
  BindableFunction, returns its real result, exposes the actual resulting
  AnimationTrack, and fires the event without a synthetic track. The signed
  Player's R15 proof now calls that public Humanoid path and requires the
  supplied controller's authentic `WaveAnim` track.
- Closed the selected Backrooms audible-asset checkpoint. `ContentProvider`
  now resolves package-backed numeric IDs synchronously before Asset Delivery,
  so audio and mesh decoders share the embedded path instead of falling through
  to HTTP. The audio runtime adds bounded FFmpeg decoding as the production
  fallback for Vorbis containers unsupported by miniaudio's built-in decoders.
  The explicit actual-app verifier clones the exact three serialized
  ServerStorage emitter templates into its listener workspace, preserves and
  checks every authored Sound property, decodes both embedded OGG payloads,
  starts three spatial voices, observes all three advance, mixes non-silent
  output, and tears the channels down. The 300-frame proof reported 14/14 R15
  meshes, sound maxima `4.49306/4.49306/0.18576`, and RMS `0.0105223`;
  `audio-runtime-contract` also remains green.
- Hardened that FFmpeg fallback so conversion checks the configured maximum
  frame count before every PCM allocation; a compressed payload cannot expand
  past `maxClipFrames` before the normal clip validator runs. Added an
  engine-owned suspend/resume state which returns silence without advancing
  voices, preserves active voice cursors, stops and conditionally restores the
  real output device, and is driven by interactive Player focus changes while
  leaving the headless verification mixer active. The deterministic suspension
  regression, strict app signature check, and selected packaged-place audio
  proof pass; the latter again decoded all three sounds and reported maxima
  `4.49306/4.49306/0.18576` with RMS `0.0101132`.
- Began the current wireable-audio DataModel path from the exact
  `ed7d8193e8564b1f` API dump rather than inventing a compatibility schema.
  Genuine `Wire` and `AudioDeviceOutput` classes now serialize their authored
  references, expose the current pin-filtered `GetConnectedWires` contract,
  validate same-tree source/target pins for read-only `Connected`, and deliver
  the four-argument `WiringChanged(connected, pin, wire, instance)` event.
  Parenting changes invalidate connection state, detached and incomplete
  wires do not report connected, and binary serialization preserves the sink
  endpoint. The direct reflection/event and binary round-trip regression
  passes. This is the first real graph slice; `AudioPlayer`, emitters,
  listeners, graph DSP routing, and the remaining nodes are still active work.
- Added bounded piecewise-linear custom distance attenuation to replacement
  engine voices. Current graph curves accept an empty/default curve and up to
  400 points while rejecting non-finite, out-of-range, duplicate, or unordered
  inputs; valid curves are sampled against the live listener and
  source transforms, preserve the spatializer's panning/doppler path, compose
  with voice volume, and transition through miniaudio's click-free gain
  smoothing. The deterministic runtime contract proves midpoint
  interpolation, source/listener movement to the first and terminal keys, and
  invalid-curve rejection. After relinking the actual signed Player, the
  selected Backrooms package again completed 300 Metal frames with all three
  authored sounds loaded, spatial, advancing, and audible (RMS `0.0105269`).
- Extended the current wireable-audio slice with the exact `AudioFader`
  reflection, serialization, pins, wiring event, bypass, and live nonnegative
  volume contract, including the current finite `0..3` bound. `AudioPlayer`
  now resolves bounded acyclic linear graph
  routes through faders to an `AudioEmitter` or `AudioDeviceOutput`, applies
  every active fader gain to the live voice, and responds to graph changes
  without losing the authored playback position. The packaged Player audio
  verifier now uses an actual `AudioPlayer -> AudioFader -> AudioEmitter`
  graph; its 300-frame run preserved the genuine OOF's 0.417959-second length,
  0.433-second unit-speed and 0.817-second half-speed durations, six loops,
  spatial output, and non-silent RMS `0.0616755`. The binary compatibility
  contract round-trips the fader and both wires.
- Corrected the selected-place audio acceptance gate to retain per-Sound
  evidence that decoding succeeded. The fixture's short non-looping flashlight
  sample legitimately finishes and releases its decoder before frame 300, so
  requiring all three decoders to remain resident at shutdown was a timing-
  dependent verifier defect. The gate still requires all three to be loaded
  simultaneously, spatial, advancing, and audible; the actual app now reports
  `loaded=3`, `spatial=3`, `advanced=3`, `loaded-frame=30`, and RMS
  `0.0102266`.
- Added the current `SoundService:GetMixerTime()` contract over miniaudio's
  global PCM clock. The value begins at zero and advances monotonically and
  sample-accurately as the shared mixer produces frames, including the
  deterministic headless path; the runtime contract verifies an exact
  64/48000-second advance. This supplies the clock required by current
  timestamped `AudioPlayer:Play`/`Stop`; scheduling and cancellation remain the
  next graph action checkpoint.
- Added sample-accurate absolute mixer-time start/stop primitives to the owned
  audio engine using miniaudio's global PCM timeline, plus cancellation of a
  pending stop without interrupting playback. Deterministic coverage proves a
  scheduled voice is exactly silent before its requested frame, starts on that
  frame, survives a cancelled stop, and becomes silent on a committed stop
  frame. The signed packaged Player exposes `GetMixerTime`, advanced it to
  exactly 5 seconds during the 300-frame audio proof, and retained all prior
  pitch, looping, graph-routing, and RMS gates.
- Connected those primitives through the exact nullable current Luau command
  surface: `AudioPlayer:Play(atTime)` and `Stop(atTime)` return unique action
  IDs only for future mixer deadlines, while immediate/no-argument calls retain
  nil returns; `Cancel(actionId)` reports whether the pending action was still
  cancellable. A small reflection extension records an optional default on a
  custom one-argument function without changing its actual Lua return shape.
  Scheduled starts retain their mixer deadline across graph/region voice
  rebuilds, and asset replacement or an immediate stop clears stale actions.
  The actual signed Player now cancels one scheduled graph start, reschedules
  it, requires deadline playback through the real fader/emitter route, and then
  completes the existing five-second audio proof. Engine tests separately prove
  scheduled and cancelled stop behavior at exact PCM frames.
- Added the exact `ed7d8193e8564b1f` `AudioListener` DataModel surface,
  including parent/instance positioning, interaction groups, current acoustic
  simulation enums, serialized angle/distance curves, reset, audibility and
  interaction queries, pins, wires, and wiring events. `AudioEmitter` now also
  exposes the matching simulation and interaction members. A listener wired
  to `AudioDeviceOutput` takes over the live miniaudio listener transform;
  listener distance and angle curves compose with emitter curves inside the
  mixer. Focused tests prove curve sampling, group-filtered symmetric
  audibility, binary round-trip, and the listener-to-output route. The actual
  signed `RobloxPlayer.app` now runs its audio proof through both the existing
  player/fader/emitter graph and a camera-parented
  `AudioListener -> AudioDeviceOutput` graph; its five-second proof passed with
  correct OOF timing, six loops, spatial output, and RMS `0.0214992`.
- Added the exact current `AudioChannelLayout` enum plus the reflected,
  serializable `AudioChannelMixer` and `AudioChannelSplitter` node surfaces.
  Their combined and twelve component pin inventories, connected-wire queries,
  layout changes, and wiring events now match the current API. The production
  route resolver carries the combined `Input`/`Output` stream through both
  nodes without falsely treating a component pin as a full stream; component
  channel extraction/mixing remains a separate DSP checkpoint. The binary
  contract round-trips a Quad player/fader/mixer/splitter/emitter graph, and the
  signed Player audio proof runs through the same combined-node chain.
- Implemented `SoundService.DefaultListenerLocation` and the exact
  `ListenerLocation` enum. `None` creates no graph, `Camera` owns a listener
  under `Workspace.CurrentCamera`, and `Character` owns the documented
  primary-part Attachment and updates its orientation from the camera each
  audio step; both modes create a device output under SoundService and a real
  connected Wire. `Default` follows the documented VoiceChatService policy,
  resolving to Camera only when default voice and the audio API are enabled.
  Mode and camera/character changes tear down and rebuild the owned graph
  cleanly. The actual signed Player switches through Character and Camera,
  verifies both generated graphs and attachment cleanup, then completes its
  sample-accurate five-second audio acceptance run.
- Split legacy `Sound` and advanced-graph spatial listening across two owned
  miniaudio listeners. AudioPlayer voices are explicitly pinned to the graph
  listener while legacy Sound voices retain the `SoundService:SetListener`
  camera/object listener, so a CoreScript-created default AudioListener cannot
  attenuate or pan unrelated legacy place audio. Deterministic coverage proves
  that each listener's custom curves affect only its pinned voices. The actual
  Backrooms package returned to its established audible level after this fix:
  all three authored sounds loaded, remained spatial, advanced to
  `4.49306/4.49306/0.18576`, and mixed at RMS `0.0105201`.
- Completed the current writable `SoundService.ListenerCFrame`,
  `ListenerObject`, and `ListenerType` property surface over the same legacy
  listener state used by `GetListener`/`SetListener`, including property-change
  delivery and location-type validation. Corrected the service's authored
  `DistanceFactor` default from the historical 10 studs to the documented
  current 3.33 studs per meter while retaining consistent source, listener,
  and rolloff-unit conversion. The binary compatibility contract checks the
  reflected listener state and default, and the actual Backrooms package
  retained all three audible channels at RMS `0.0103454`.
- Added the exact current `AudioDistortion` reflected, serializable, pin/wire,
  bypass, and bounded `Level` surface from the supplied Studio API metadata.
  AudioPlayer routes now carry every active distortion node in graph order
  into a bounded per-voice miniaudio DSP chain, so unrelated legacy and graph
  voices are not processed globally and live bypass/level changes reach the
  realtime node without reallocating in the callback. Deterministic engine
  coverage proves nonlinear drive and dry restoration, the binary contract
  round-trips the node and wires, and the actual signed Player audio proof now
  runs through `AudioPlayer -> AudioFader -> AudioDistortion ->
  AudioChannelMixer -> AudioChannelSplitter -> AudioEmitter`.
