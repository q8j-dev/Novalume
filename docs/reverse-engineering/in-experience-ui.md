# 2026 Player in-experience UI evidence

This document records clean-room facts extracted from the supplied Windows
Player. It deliberately excludes the standalone app shell and does not copy
the proprietary implementation source.

## Authoritative package

- Model: `ExtraContent/models/InExperience/InExperience.rbxm`
- SHA-256: `6cf84a6c34fcf514c029301508e1601cf8da0fd4fb781bc5789549a668f95bf1`
- Serialized instances: 26,634
- ModuleScripts: 24,388
- Root bootstrap: `PatchRoot/CoreScripts/StarterScript`
- Runtime tree: `PatchRoot/DataModelInstances/CoreGui/RobloxGui`
- Shared packages: `PatchRoot/DataModelInstances/CorePackages`

The complete path/hash/size inventory and selected literal contracts are
generated at `out/reverse-engineering/in-experience-full-inventory.json`.

## Protection and decoding

Each ModuleScript Source value contains an eight-byte Player integrity prefix,
a Luau bytecode version 7/type version 3 chunk, and a 24-byte integrity
trailer. Instruction opcode bytes are multiplied by 227 modulo 256. Multiplying
the stored opcode by 203 modulo 256 restores the standard Luau opcode because
`227 * 203 mod 256 = 1`.

Only instruction opcode bytes are transformed. Auxiliary instruction words,
the string table, constants, proto metadata, line information, and debug
metadata use the standard Luau serialization. The inspector validates proto
boundaries and auxiliary-word lengths after decoding; it never emits copied
implementation source into the repository.

For local analysis, the decoded chunks were passed to a source-built Luau v7
decompiler outside the repository. It recovered 24,384 of 24,388 modules; the
four control-flow failures are UGC validation and replay-camera modules, not
the in-experience UI roots. Decompiled implementation text remains temporary
research material and is not checked in.

## Genuine runtime surfaces

The Player bootstrap selects and connects these systems rather than a single
pre-rendered model or bitmap:

- `Chrome/ConfigureChrome`
- `Chrome/ChromeShared/Service/ChromeService`
- `Chrome/ChromeShared/Unibar/UnibarMenu`
- `Chrome/ChromeShared/Unibar/ComponentHosts/IconHost`
- `TopBar/ComponentsV2/TopBarApp`
- `PlayerList/PlayerListManager`
- `PlayerList/PlayerListController`
- `PlayerList/Components/PresentationCommon/PlayerListDisplayView`
- `CoreScripts/ExperienceChatMain`
- `Settings/SettingsHub` and `InGameMenuInit`
- the package-backed `Backpack/BackpackScript`

The bootstrap also proves that Chrome, PlayerList, ExperienceChat, Backpack,
and the in-game menu share services, state, input policy, keep-out areas,
Foundation/UIBlox tokens, and image sets. They must be backported as one
connected in-experience system.

## Measured contracts

The decoded desktop Chrome constants expose, among other values:

- icon cell width 44 and icon/highlight size 36;
- medium icon/divider height 28 and submenu row height 56;
- default panel width 176 and height 130;
- large panel width 176 and height 285;
- non-mobile slots 9, landscape-mobile slots 7, portrait-mobile slots 6;
- submenu corner radius 10 and padding 10;
- close button size 22 by 22 inside a 44 by 44 frame;
- pin icon size 18 by 18;
- window icon size 42;
- drag threshold 10 and window minimum drag distance 25;
- shortcut-bar display order 100.

Ten-foot mode applies the package's 1.5 scale where specified; it is not the
desktop baseline.

Token-backed measurements remain linked to their exact token names instead of
being guessed: `Size_1100` for an icon cell, `Size_900` for its icon and
highlight, `Size_700` for medium icons/dividers, `Padding.Small` for the left
margin and window padding, and `Gap.Large` for the menu screen-side offset.

The active `TopBar/ComponentsV2/MenuIcon` renders `IconName.Tilt` from the
regular Builder Icons font. The older presentation module uses the image-set
identifier `icons/logo/block` and retains
`rbxasset://textures/ui/TopBar/coloredlogo.png` as its fallback.
The supplied font family consists of the exact regular and filled TTF faces
plus `BuilderIcons.json`; all three are hash-verified into the Player overlay.
Builder Icons are named OpenType glyphs, so the renderer resolves the complete
icon name (for example `tilt` or `speech-bubble-align-center`) rather than
drawing its letters as ordinary text.
The common icon button retains `rbxasset://textures/ui/TopBar/iconBase.png`.
The player-list presentation uses the `icons/navigation/close` image-set icon
and the exact top/bottom rounded-rectangle assets. Asset filenames alone are
not sufficient evidence; imports are accepted only when a runtime-reachable
module references them.

## Native touch and responsive PlayerList proof

The shared platform contract now transports native contact identity, position,
and began/moved/ended/cancelled phase. The iOS host uses the `UITouch` identity;
the Web host uses the browser touch identifier. PlayerRuntime retains the same
Touch `InputObject` across that stream and lets `UserInputService` publish
TouchEnabled, LastInputType=Touch, and PreferredInput=Touch. Compatibility
mouse delivery remains separate and cannot duplicate `Activated`.

The package-backed verifier drives the genuine Chrome overflow integration and
normal `CoreGui.PlayerList` with touch. It proves the populated local row, the
official dropdown avatar/header and `Examine Avatar` action, dismiss, and
Chrome reopen lifecycle. State-aware retries observe the real mounted controls
instead of assuming animation completion at one frame. Signed Metal runs pass
at 640x480, 1280x720, and 1920x1080 logical viewports, and the final frames
show the official panel bounded at the upper-right at each size. The original
mouse path continues through the distinct Inspect-and-Buy lifecycle.

This closes native touch identity and a three-size responsive PlayerList proof.
It does not close keyboard/controller navigation, orientation and safe-area
changes, the remaining CoreGui surfaces, or final perceptual acceptance.

## Backport rule

The temporary compatibility UI is disabled and is not a deliverable. The
shipped replacement must execute a behaviorally equivalent port of the
genuine package graph against implemented engine contracts. Screenshots are
used for visual verification only, not as the implementation specification.

## Native engine dependency closure

`tools/fixture_inspector/in_experience_engine_contracts.py` scans both the
authoritative model and the locally decoded module graph. The current fixture
constructs 6,706 UI instances in source across 25 UI classes. This catches
runtime-created dependencies that cannot appear as serialized instances in
the RBXM inventory.

The package directly constructs the newer native primitives
`UIAspectRatioConstraint`, `UIFlexItem`, `UIGradient`, `UIStroke`,
`UIDragDetector`, `UIPageLayout`, and `ViewportFrame`. These are implementation
requirements, not names to accept and ignore. Each surface is mounted only
after the properties and behavior it exercises exist in the engine and have
native tests. `UIAspectRatioConstraint` now has real sizing behavior for
`FitWithinMaxSize` and `ScaleWithParentSize`, both dominant axes, reflected
properties and enums, live invalidation, and post-layout constraint priority.
The generated report is stored at
`out/reverse-engineering/in-experience-engine-contracts.json`.

Direct package-scope construction confirms the ordering of the remaining
work: Chrome creates one `UIFlexItem`; ExperienceChat creates two flex items,
13 gradients, and four strokes; TopBar creates one stroke; Settings creates
two flex items, seven strokes, and one gradient. `UIFlexItem` and the flex
extensions on `UIListLayout` are now native engine behavior. The backport
implements grow, shrink, fill and custom ratios, main-axis space distribution,
wrapping, cross-line distribution, per-item line alignment and stretching,
with immediate relayout when either container or item properties change.

## Deferred implementation checkpoint (2026-07-17)

The user explicitly deferred the 2026 in-experience UI while animation and the
remaining modernization goals are completed. This is a pause, not a target
change. Do not replace this work with a screenshot recreation, compatibility
mock, placeholder, example UI, or the Roblox app shell. Resume from the genuine
Player `InExperience.rbxm` graph and the engine-contract failure described
below.

The packaged Player currently mounts all 26,634 model instances and 24,388
ModuleScripts. The active CoreScript graph reaches `SettingsHub` and creates
`SettingsClippingShield`, but the normal app does not yet show the 2026 UI and
the headless renderer still reports eight final-frame draws. A successful UI
render pass is not evidence that the UI itself is mounted or visible.

Implemented and verified engine contracts at this checkpoint include:

- ScreenGui `Enabled`, `ResetOnSpawn`, `DisplayOrder`, `IgnoreGuiInset`, and
  `ZIndexBehavior`, including stable global and hierarchy-aware sibling draw,
  input, gesture, and selection traversal;
- `UIGridLayout` alignment, start corner, absolute cell size/count, and real
  placement behavior;
- `GuiBase2d.AutoLocalize`, legacy `Localize`, `RootLocalizationTable`, and
  render-time CoreScript localization without overwriting source text;
- current gamepad control-image lookup paths in `UserInputService`;
- `RunService:GetRobloxVersion()` returning the supplied Player's extracted
  `0.730.23.7300792` version;
- `ScrollingDirection`, `ScrollBarInset`, and
  `VerticalScrollBarPosition`, including inset clipping/window sizing,
  scroll-range calculation, and left/right scrollbar positioning;
- `GuiButton.Activated(inputObject, clickCount)` on the real mouse,
  keyboard, and gamepad activation path, with multi-click sequence tracking;
- stateful `CollectionService`, capture, localization, stylesheet,
  microprofiler, CoreGui configuration, value-object, DateTime, and related
  contracts documented in the source and STATUS history.

The last verified `SettingsHub` advance removed the `Activated is not a valid
member of ImageButton` failure. The next verified blocker is that
`SafetyService` is not a valid service while loading
`Modules.Settings.Pages.GameSettings` (then `SettingsHub` line 3919 and
`TopBarApp` line 51). A complete `SafetyService` source/header and factory
registration are present as unbuilt work in progress. Before resuming the UI,
build and test that service; compare its client-only
`DecodeAvatarMovementProto` failure, screenshot job/event behavior, report
surface signals, and `IsCaptureModeForReport` property against the supplied
2026 Player strings/API dump. Then continue the exact CoreScript blocker chain.

Other known independent failures at the pause include SelfieView's
`AudioAnimationEnabled`, `ErrorPrompt.Create`, ragdoll rigging, `Player.Team`,
`AnimationClipProvider`, `AvatarCreationService`, `InsertService.LoadLocalAsset`,
`ContextActionPriority`, `BundleType`, virtual-cursor property signals,
`Animator.AnimationStreamTrackPlayed`, and `AdService.AdTeleportInitiated`.
These are evidence of unfinished engine closure, not permission to swallow
errors or add fake members.
