# Reference research log

All observations are clean-room interoperability evidence. Reference files stay
outside source, packages, and Git history. No reference binary has been run.

## Selected acceptance place

The single final scenario is **Backrooms Game - Fully Scripted - FREE**. Its pair
is relatively compact (170,718-byte RBXL and 8,013,591-byte RBXLX) while still
exercising a meaningful scripted environment. The two files are treated as two
encodings of one scenario, never as two acceptance places.

| File | SHA-256 | Observation / purpose |
|---|---|---|
| `2026-place-files/Backrooms Game - Fully Scripted - FREE.rbxl` | `647b5dfc5424f3a78e2d8959eabb7a59bf47b9910f59ff2dd09b22a60a834a29` | Binary encoding selected for guarded serializer equivalence and end-to-end load |
| `2026-place-files/Backrooms Game - Fully Scripted - FREE.rbxlx` | `b1a47bea80de10691b9c2e5de5bd4cf2da60b1797a823052e773c63a65d92e0d` | XML encoding selected for schema research and equivalence comparison |

The pair is not copied or mounted by the product. Acceptance invokes it as an
explicit external test fixture and records the hash before loading.

## Candidate inspection (not an acceptance scenario)

`BarryObby.rbxl` and `BarryObby.rbxlx` were inspected only for corpus sizing and
format identification before the final selection. They are explicitly not a
second tested place.

| File | SHA-256 | Tool / observation |
|---|---|---|
| `2026-place-files/BarryObby.rbxl` | `2fbc3d82fd792796b9040863d78825ecd8b93d7e56095a2fbec6bbec71b59c81` | `file`, `shasum`; binary data, 2,167,310 bytes |
| `2026-place-files/BarryObby.rbxlx` | `62274f9702bca5f16043dee7de8be2c0ae56200d413c5d7c92a02a24dcf44ac4` | `file`, `head`, `shasum`; Roblox XML, 105,988,066 bytes |

## Modern Player snapshot observations

Static directory inspection of
`WindowsPlayer-version-ddf02245bdbb428c` shows canonical R6/R15 animation
models, R15 character and unification models, avatar compositing meshes,
Builder font families plus script-specific Noto fonts, PC terrain material
manifests, BRDF/terrain textures, CoreScript localization, and `Mobile.rbxl`.
These names establish available evidence, not redistribution approval or proof
of implemented semantics. Individual files will be hashed when opened or used.

The user designated `LIVE-WindowsStudio64-version-ed7d8193e8564b1f` as the
authoritative R15 model, rig, and animation source. The product importer accepts
only the ten exact paths, sizes, and hashes recorded in
`tools/resource_importer/avatar-runtime-assets.json`; its aggregate is 81,794
bytes. Native deserialization confirms the standard rig has 14 MeshParts and
15 Motor6D joints, both dynamic-head variants have 15 MeshParts and 15 Motor6D
joints, and the supplied `Animate` controller contains its complete idle,
locomotion, swim, tool, emote, dance, mood, and `PlayEmote` child graph.

Representative files used for focused mobile/R15 research and implementation:

| File | SHA-256 | Purpose |
|---|---|---|
| `WindowsPlayer.../ExtraContent/places/Mobile.rbxl` | `d20cd3d24cfa27a32090d09ef8ae5754ecbe2d2c9c1d6b2553cc5fa5ff910768` | Canonical mobile place/lifecycle fixture reference |
| `Studio.../content/avatar/characterR15.rbxm` | `e841643e20711509082f48643d03ffeae2596391d17f4a49f26e3397d1f8c017` | Imported standard R15 hierarchy; native model and packaged render contracts pass |
| `Studio.../content/avatar/animations/humanoidR15AnimateChildren.rbxm` | `d3bd21b14670923c793e4d08d6be1f6306b2c4d6aac05b8f0fc11c1358641e40` | Imported authentic animation-child structure; native hierarchy contract passes |
| `Studio.../content/avatar/scripts/humanoidAnimateR15CharacterController.rbxm` | `2a8c520427d460cdb790c68786aa064957eea12f00cccb2fd389c0f588ccb515` | Imported complete R15 controller graph; packaged runtime has an active Animator track |

The exact supplied Studio Lua additionally establishes the production call
sites for four current Humanoid rig APIs. `content/avatar/unification/` invokes
`BuildRigFromAttachments`; Thumbnailing's `MannequinUtility` invokes both
`ReplaceBodyPartR15` and `BuildRigFromAttachments`; and UGCValidation's
`getAccessoryScale.lua` maps attachments with `GetBodyPartR15` before calling
`GetAccessoryHandleScale`. The owned implementation uses those public call
contracts and exact model metadata, not copied proprietary implementation
code. Focused runtime coverage rebuilds and replaces parts in the hash-recorded
`characterR15.rbxm` hierarchy and validates the resulting Motor6D attachment
frames and scale factors.

## In-experience Player UI boundary and client delta

The Player UI target is the in-experience CoreGui surface only: loading and
offline/error states, top bar/HUD, in-game chat, player list, backpack/hotbar,
in-game menu/settings, dialogs/prompts/notifications, text input and control
states, gamepad/VR/touch affordances, performance/network/voice overlays, safe
areas, and density variants. The standalone application shell is excluded,
including authentication, home/discovery, marketplace/catalog, profile/friends
and social messaging, app navigation/settings, the standalone avatar editor,
and WebView UI. `LuaApp`, `LuaChat`, `LuaChatV2`, and `LuaDiscussions` paths are
machine-excluded by `tools/resource_importer/ui-reference-spec.json`.

`tools/resource_importer/reference_inventory.py` performs a deterministic
metadata-only comparison without copying a reference payload. It validates PNG
headers and dimensions, hashes every selected file, groups density variants,
checks case collisions, and verifies the supplied APK hash. The current scoped
run records 2,207 entries with no excluded app-shell paths:

| Comparison | 2016 baseline | 2026 candidate | Added | Changed | Removed | Byte-identical |
|---|---:|---:|---:|---:|---:|---:|
| `content/textures/ui` | 307 | 1,439 | 1,192 | 247 | 60 | 0 |
| `content/fonts` | 73 | 119 | 114 | 4 | 68 | 1 |

An additional 114 Player files are selected only from explicit in-experience
`ExtraContent` families (`InGameChat`, `InGameMenu`, controls, gamepad, relevant
image sets, settings, and avatar-in-experience imagery). This is **observation**
evidence for the overlay and implementation backlog, not proof that every asset
is already mounted, semantically implemented, or approved for redistribution.

### InExperience model and local-avatar thumbnail contract

The supplied
`WindowsPlayer.../ExtraContent/models/InExperience/InExperience.rbxm` has
SHA-256
`6cf84a6c34fcf514c029301508e1601cf8da0fd4fb781bc5789549a668f95bf1`.
It was parsed structurally with the owned
`tools/fixture_inspector/in_experience_contracts.py`; the reference model was
not executed or rewritten. The Foundation thumbnail helper contains the exact
format string `rbxthumb://type=%*&id=%*&w=%*&h=%*`, and the People card selects
the Avatar thumbnail type. A temporary runtime observation, removed after the
contract was established, recorded the local card's concrete request as
`rbxthumb://type=Avatar&id=1&w=150&h=150`.

The owned implementation treats this as a render-scene request, validates the
type, positive user id, dimensions, duplicates, and bounds, then resolves only
the matching offline local Player's current character. This is an
**implementation inference** from the observed model contract: the supplied
model establishes the URI and thumbnail selection, while the local-character
viewport behavior is independently implemented and verified through a real
244-frame Metal capture. It is not yet a claim of pixel-for-pixel parity for
every avatar/bust/headshot camera variant.

### Foundation Image default-style contract

Static parsing of the same supplied `InExperience.rbxm` recovered these source
hashes and literal contracts without executing or copying their implementation:

- Foundation `Components/Image/Image`, SHA-256
  `a22d35b8ae0aa1e6fd207eef01ce43ed822418e50b67c8f59f12c4279424a21c`,
  references `gui-object-defaults`, `x-default-transparency`, `backgroundStyle`,
  `useDefaultTags`, and `useStyleTags`;
- Foundation `Providers/Style/useStyleTags`, SHA-256
  `09429467dc8467868d31e46b51d8e69e004c5d4fca8759d3c71ed26e1277c66f`,
  references formatted/split tags and layout-effect application;
- People `Components/Card/CardThumbnail`, SHA-256
  `7fc8cd8ec6477cadd007d08e4bfa0b232c70b5d4a54ed11c3a97ae64e3b59cb3`,
  selects Foundation `Image` with the authentic
  `size-full aspect-1-1 radius-medium` tag contract.

A temporary bounded runtime diagnostic, removed after verification, observed
the genuine live `CardThumbnail` carrying `aspect-1-1`,
`gui-object-defaults`, `radius-medium`, and `size-full`, linked to the supplied
`Dark-Desktop-1` stylesheet with 79 rules. Its exact
`.gui-object-defaults` rule stores `BackgroundTransparency = 1`. The owned
resolver initially applied that value, but React could subsequently assign the
native ImageButton default and leave it opaque because property mutation did
not invalidate resolved styles. The shared GUI implementation now re-resolves
tagged, authored property changes with a recursion guard and retains explicit
non-default property precedence. Focused unit coverage and a packaged
300-frame bgfx/Metal proof establish the implementation result; this is not a
claim that all remaining Foundation selectors, transitions, or pseudo-elements
are complete.

## Windows Player binary UI capability evidence

The following supplied binaries were inspected statically only. Neither was
executed, patched, injected, or used to contact a service.

| File | SHA-256 | Tools / purpose |
|---|---|---|
| `WindowsPlayer.../RobloxPlayerBeta.exe` | `d308719d121196c05d18475ab53fad83bc0d10e0d77756ddf76ac7fe4a5df0b3` | `file`, `rabin2 -I/-S/-i`, `strings`; PE structure, imports, RTTI/reflection identifiers, resource paths, and UI runtime capabilities |
| `WindowsPlayer.../RobloxPlayerBeta.dll` | `66e76ca5ee388d398a8554124c13da9bc5319bab4cc146b1048f79a5d0f00f94` | `file`, `rabin2 -I/-i`; loader boundary and imported platform facilities |

**Observed** identifiers in the Player executable include current reflection
contracts for `ScreenInsetsType`, `SafeAreaCompatMode`, `PreferredInput`,
display scaling and preferred text size, `AutomaticSize`, styled GUI object,
image-button, and text-button properties, `CanvasGroup`, CoreGui configuration,
player-list configuration, focus/selection boundaries, and current
ContentProvider variants. Exact enum strings observed include
`DeviceSafeInsets`, `CoreUISafeInsets`, `TopbarSafeInsets`,
`FullscreenExtension`, `KeyboardAndMouse`, `Gamepad`, and `Touch`.

`tools/fixture_inspector/player_ui_contracts.py` now makes this inspection
repeatable. It validates the executable hash, maps PE file offsets to virtual
addresses and sections, extracts each scoped printable identifier and MSVC
reflection/RTTI fragment, locates common x86-64 RIP-relative references and
absolute PE64 data pointers, and reports identifier coverage in owned source.
It deliberately makes no behavior claim when executable code is unavailable.
The generated report is kept under the build/output tree, not source control.
Representative observed offsets in this exact executable are:

| Contract | Property/class string offset | Reflection/RTTI offset |
|---|---:|---:|
| `ScreenGui.ScreenInsets` | `0x6081e68` | `0x7be08c6` |
| `ScreenGui.SafeAreaCompatibility` | `0x6081de0` | `0x7be0796` |
| `ScreenGui.ClipToDeviceSafeArea` | `0x6079c30` | ScreenGui descriptor cluster |
| `UserInputService.PreferredInput` | `0x5fc06a0` | `0x7af6a46` |
| `GuiObject.AutomaticSize` | `0x607fe00` | `0x7bd3036` |
| `GuiObject.GuiState` | `0x5dd2c4d` | `0x7bd34f6` |
| `CanvasGroup` | `0x5e4c0a0` | `0x7bf56a6` |
| `CoreGuiConfiguration` | `0x60829c4` | `0x7d0d756` |
| `PlayerListConfiguration` | `0x6083550` | `0x7d0daa6` |

The executable's `.text` raw data is filled with `0xff` and is not marked
executable in the on-disk section table, so ordinary static disassembly cannot
recover those functions directly. Data references are still useful. For
example, an absolute pointer at file offset `0x6054ef8` targets
`AutomaticSize` at VA `0x146081200`; the adjacent descriptor entry identifies
the declaring class as `GuiObject`. A matching table pairs `GuiState` with
`GuiObject`. These are **observed registration-table relationships**, not a
claim about the protected code's implementation.

The binary also contains `PreferredInputTouchDelayTimeSeconds`. This is
**observation** evidence that the current client can apply touch-transition
timing policy. The owned implementation currently provides the confirmed
read-only reflected enum and deterministic device-family changes, but does not
claim that the binary's delay/hysteresis policy has been recovered yet.

The conclusion that the 2026 PNG set depends on newer layout, inset, scaling,
input, focus, state, and styling behavior is an **inference** supported by those
binary contracts and the resource delta. Each behavior remains unimplemented
until owned DataModel/CoreGui code and focused runtime tests prove it. Studio
metadata may confirm property/schema names for serialization work, but Studio
UI packages are not a Player UI design authority.

## Android package static inspection

`Roblox_2.730.790.apk` hashes to the supplied expected SHA-256
`bcfa47ad89cacb265d9bc98fd97c2567f69ce59024b5a97096c7c0edc30d3040`.
The APK was not installed, executed, resigned, modified, or extracted into the
source or package trees. Native libraries were extracted under generated
`out/research` for offline analysis:

| Native library | SHA-256 | Build ID |
|---|---|---|
| `lib/x86_64/libroblox.so` | `4ecb8f47537996c44cb60cecfd58ba6b5c394d77daa2716ba4ceebe30b8695a7` | `48fc8ee1fb36fc39072fd8619154ce90eea4b316` |
| `lib/arm64-v8a/libroblox.so` | `4e416eb4062ade7a2baff9bc94f9a1d92f10c1854c344f0be83abdf9267535e7` | `e5708c7fb8bdc756600a1dc820146b4b6b0dc588` |

Unlike the protected Windows `.text`, the x86-64 ELF contains analyzable native
code and matching RTTI/reflection names. Focused `/usr/bin/objdump` disassembly
and string-reference analysis observed the following `AutomaticSize` behavior:

- property registration references `AutomaticSize` at `0x1f6b6f5`, assigns it
  to the `Data` category, and installs getter and setter functions;
- the getter at `0x234ea68` reads an enum-sized member (with a styled-property
  path when enabled);
- the setter at `0x234e936` returns without work when the value is unchanged,
  otherwise writes the enum and invokes property/layout invalidation paths;
- the registration cluster contains the styled-property override warning used
  by current GUI properties.

Studio API documentation and reflection metadata were used only to clarify the
public contract: `None`, `X`, `Y`, and `XY`; automatic axes grow for content,
retain the authored `Size` as a lower bound, and stop at the parent's maximum.
The metadata file inspected for that cross-check hashes to
`a76f15a25e35d9f6d41cb4daae54f9f1bea4b21813b0b98dc2a2a6c8a899795d`.
The owned implementation does not reproduce disassembly; it implements this
public behavior independently in the existing layout and `GuiObject` systems.

The same route recovered the `GuiState` public contract. Its enum-registration
constructor at `0x4faf75c` registers `Idle=0`, `Hover=1`, `Press=2`, and
`NonInteractable=3`. The property registration references `GuiState` at
`0x1f6c36a`, installs a getter-only descriptor, and points to a getter at
`0x238b0a0` that directly reads a 32-bit object member at offset `0xc0`.
The owned `GuiObject` keeps its more detailed 2016 down-over/down-away state
machine internally, exposes the current read-only enum, gives non-interactable
visibility/activity precedence, and raises `GuiState` property changes when the
observable state changes. Focused layout tests cover the four public states.

Observed entries include `assets/ExtraContent/places/Mobile.rbxl`, design-system
D-pad and thumbstick images at 1x/2x/3x densities, jump/action/menu touch-control
images, legacy D-pad sheets, touch-tap imagery, gamepad cursors, broad Builder/
Noto/SourceSans font coverage, and `assets/shaders/shaders_vulkan_mobile.pack`.
The packed shader is capability evidence only. No APK asset has been approved
for redistribution or imported; canonical Player equivalents are preferred.

## Current Player audio behavior evidence

The hashed APK x86-64 `libroblox.so` used above also contains the current audio
reflection and diagnostic surface. Static strings show that this exact 2026
build still has an internal FMOD implementation (`SoundService::openFmod`,
`closeFmod`, and FMOD issue diagnostics), but middleware choice is not copied:
the objective independently requires a maintained open-source replacement.

Observed current contracts include `AudioPlayer` asset, volume, playback speed,
time, autoplay, loop/playback regions and play/stop scheduling; `AudioEmitter`
and `AudioListener` distance/angle attenuation curves and interaction groups;
wireable `AudioFader`, equalizer, compressor, echo, reverb, pitch-shifter and
analyzer nodes; plus the legacy `SoundService` doppler, distance, rolloff,
listener and ambient-reverb surface. Studio API documentation at
`content/api_docs/en-us.json` (SHA-256
`dfce01a3e58b9bfb1578e66764aca48e0b434472a7f29f902964bfac8d985def`)
confirms the public descriptions and pin/wire model. This is **observed schema
and capability evidence**, not proof that the owned graph/effects are complete.

The owned miniaudio implementation currently covers bounded clip decoding,
independent voices, looping and playback-range primitives, pitch/volume controls,
seeking, listener/source transforms, inverse and linear distance attenuation,
doppler parameters, priority stealing, master gain/mute, deterministic offline
mixing, and explicit CoreAudio device lifecycle. The real legacy `Sound`,
`SoundChannel`, `SoundService`, and `SoundWorld` family now compiles against it
without FMOD middleware symbols and is connected to the packaged Player.

The packaged legacy OOF asset at `content/sounds/uuhhh.mp3` decodes as mono
22,050 Hz with a 0.417959-second duration. A Player-connected runtime proof
exposed a concrete interoperability defect in the pinned miniaudio 0.11 API:
`ma_audio_buffer_ref_init` initializes the public `sampleRate` field to zero,
with an upstream TODO indicating that callers of the reference-buffer API must
provide it. The owned voice builder now assigns the decoded clip rate before
attaching the data source. This is an implementation finding from the owned
open-source mixer, not inferred proprietary behavior. Cross-rate unit coverage
and a packaged spatial SoundChannel proof establish correct 22.05-to-48 kHz
duration, `PlaybackSpeed=0.5`, loop signaling, audible PCM, and teardown.
Custom attenuation curves, the current wire graph, remaining effect parity,
streaming, device-interruption/suspend matrices, and selected-place audible
acceptance remain explicitly incomplete.

## Current Player ShadowMap architecture evidence

Static `strings -a` inspection of the already hash-recorded 2026 Windows
Player executable (`d308719d121196c05d18475ab53fad83bc0d10e0d77756ddf76ac7fe4a5df0b3`)
observed RTTI for `Graphics::Shadows::ShadowMapSystem`,
`ShadowMapSystemImpl`, `ShadowMapSystemImplNew`, `ShadowMapView`, and
`ShadowCascadeDispatchView`. The same binary contains the resource/pass names
`ShadowAtlas`, `ShadowMap 0`, `ShadowMap 1`, `ShadowMap Depth`,
`ShadowMap Blur Temp`, `ShadowMapSystemNew`, `clearShadowAtlas`, and
`render/shadowmap/depthcache`. Relevant configurable identifiers include
`RenderShadowmapBias`, `ShadowMapCascadeUpdateDivisor`, `ShadowMapUpdatesMin`,
`ShadowMapUpdatesMax`, `RenderMaxShadowAtlasUsageBeforeDownscale`,
`RenderAllocateShadowMapResourcesOnDemand`, `CullPixelsShadowMapLow`,
`CullPixelsShadowMapHigh`, and several cascade-resize/desynchronization safety
flags. This is direct architecture/capability evidence that the current client
uses a multi-view cascade/atlas system with scheduled updates and a depth
cache; it does not reveal numerical split, atlas, bias, or update policies.

The supplied Player `AppSettings.xml`, SHA-256
`bb11dd79ccb2f8cfd6fbbcceb7553cf99482939731c87cdbdd7ccef65d32fa1e`,
contains no values for those identifiers, so it cannot truthfully supply the
missing numeric policy. The owned renderer's verified 256/512/1024 legacy
single-map path is therefore only an implementation checkpoint. It must not be
represented as 2026 cascade/atlas parity until the atlas, multiple dispatch
views, update scheduling, bias/quality behavior, caster matrices, capability
fallback, and visual results are independently implemented and tested.

## Rejected PlayerList reskin and independent Inspect-and-buy events

A packaged pointer trace through the authoritative `InExperience.rbxm` graph
previously activated the populated `Player_1` row under a locally forced
`PlayerListReskin2=true` override. It mounted
`PlayerListReskin.ContextMenu` with a 260x109 menu, `Player`, `@Player`,
`Examine Avatar`, and
`rbxthumb://type=AvatarHeadShot&id=1&w=150&h=150`. The user explicitly
identified this entire presentation as a reskin rather than the official
Chrome leaderboard. It is therefore rejected acceptance evidence. The host
override has been removed and the verifier now rejects a `PlayerListReskin`
root.

Static corroboration of the Chrome integration used the following supplied
Studio CoreScript files. Studio is not the Player UI authority; the matching
paths and literal contracts in the hash-recorded Player `InExperience.rbxm`
establish the package relationship.

| File | SHA-256 | Observation |
|---|---|---|
| `ExtraContent/scripts/CoreScripts/Modules/Chrome/ConfigureChrome.lua` | `aaca231c6776ff3f1af488f64f8e1131f76378b75d08bea2c79861ec6681f08a` | places the `leaderboard` integration in the Chrome submenu |
| `ExtraContent/scripts/CoreScripts/Modules/Chrome/Integrations/OverflowMenu.lua` | `959613058d43498ef413f58e20300f5a1360c26acf5e9a44ea57635881b6d668` | the `leaderboard` activation delegates visibility to `PlayerListManager` |
| `ExtraContent/scripts/CoreScripts/Modules/Chrome/Integrations/Pages/People.lua` | `6897150c2c130d54289e130f7e01b335d2db6a3085f020e32fb0e70f0ba34c74` | registers separate id `people` and opens the in-game menu `PlayersPage`; it is not the leaderboard integration |
| `ExtraContent/scripts/CoreScripts/Modules/PlayerList/PlayerListController.lua` | `b248633832fd1e6f97303aecead39a8852a989de016c8d653421efbf31edf728` | selects the reskin only when `FFlagPlayerListReskin` is enabled |
| `ExtraContent/scripts/CoreScripts/Modules/PlayerList/Components/Presentation/PlayerDropDown.lua` | `721efdfc2d7221501d1047ea9237a6ad4d4edf02e78b14006dcb5a9b4a462ee1` | the normal presentation renders `DropDownPlayerHeader`, labels the official action `Examine Avatar`, and invokes `GuiService:InspectPlayerFromUserIdWithCtx(..., "leaderBoard")` |
| `ExtraContent/scripts/CoreScripts/Modules/PlayerList/Components/Presentation/DropDownPlayerHeader.lua` | `7b36ba3bd9eb3f782f8398a1f893193030f30f3f9e06174c72a71e1fad21128a` | renders the selected player's display name, username, and `rbxthumb://type=AvatarHeadShot` image |
| `ExtraContent/scripts/CoreScripts/Modules/PlayerList/Components/Presentation/DropDownButton.lua` | `26a3e3143efe8f09da958839528245ae1f40d0d3286c0a125c0c3f3ec736a7ad` | supplies the normal dropdown action-button structure exercised by native hit testing |

With no local reskin override, three consecutive 400-frame packaged runs report
`PlayerListReskin2=false` and mount `CoreGui.PlayerList`. The normal package
tree produces a bounded compact upper-right surface and a populated local
`Player` entry. The verifier opens it from Chrome, selects the local row,
requires the normal avatar header and `Examine Avatar` action, mounts and
dismisses the responsive Inspect & Buy surface, closes PlayerList, reopens it,
and rejects the reskin root. Three consecutive final packaged runs exited 0
with 30-31 final-frame draws. This is the corrected implementation target and
desktop interaction proof, not yet a broad responsive/input-matrix or
pixel-parity claim.

The subsequent touch checkpoint preserves the package's compatibility signal
surface while adding the current input identity that the normal presentation
expects: one contact owns one `InputObject` from Begin through End/Cancel,
`GuiButton.Activated` receives that Touch object once, and the synthesized
mouse path does not create a second Activated event. This is implemented
behavior backed by a focused native contract, not an inference from a screen
capture.

Three signed 440-frame Metal runs exercise the same Chrome and normal
PlayerList graph at 640x480, 1280x720, and 1920x1080 logical viewports. Each
run requires a bounded populated panel, the normal player dropdown header,
avatar and `Examine Avatar` action, a real close, and a real reopen, then checks
TouchEnabled, LastInputType=Touch, and PreferredInput=Touch. The captured final
frames were reviewed after converting the renderer's raw Netpbm proof payloads
to PNG for inspection. This is responsive mouse/touch evidence for this
surface; it is not keyboard/controller, orientation, safe-area, or pixel-parity
acceptance.

The controller checkpoint adds direct evidence for the supplied focus path.
`FocusNavigation/EngineInterface.lua`, SHA-256
`b312d337af8a33034f0922c8b44f60511466a7c457cc99517b5a6608f79eb870`,
calls `GuiService:Select(guiObject)` whenever a requested focus container is
not itself selectable. `PlayerListDisplayView.lua`, SHA-256
`d260f3771f5e70e5643df352c2ad44b9ee33c1852f4d6bd9e927dce9bc2dcd5d`,
requests focus for the scrolling container when the PlayerList is visible,
directional input is preferred, and gamepad input is active. The supplied
settings dictionary, SHA-256
`6f1b3e3134142abbd3dbb5a48d1fd084042cf24b5a5c01eb3956d1de1deef107`,
sets `FFlagAddNewPlayerListFocusNav=True`,
`FFlagPlayerListUseFocusNavHook2=True`, and
`FFlagEnableMobilePlayerListOnConsole=False`. In that exact TenFoot branch the
hook-two path registers entries but does not call its focus acquisition path;
the retained non-hook-two compatibility branch does call the supplied
FocusNavigation engine interface. The runtime therefore selects that branch
without changing the package, and implements the missing current
`GuiService:Select` engine surface it requires.

The final 440-frame controller run uses an XboxOne/TenFoot platform identity,
retained Gamepad1 input objects, D-pad navigation, A activation, and the
package's B close binding. It observes the selected object as the real
`CoreGui.PlayerList...PlayerEntry_1.NameFrame.BackgroundFrame`, then observes
selection cleared and the panel closed. This is controller navigation and
lifecycle evidence for the official PlayerList; it is not keyboard, broad
console-platform, or perceptual-parity acceptance.

The keyboard checkpoint is grounded in the supplied
`KeyboardUINavigation.lua`, SHA-256
`30d0f5e8f9c145f94ed9a9d614c6ff6fc7ab258e5fc0ea48564c4779b6650bbe`.
That module binds Backslash through `ContextActionService`, calls
`GuiService:Select(PlayerGui)` when enabling keyboard navigation, and clears
the selected object when disabling it. The 350-frame packaged proof drives
that exact module with retained keyboard Begin/End objects, selects a visible
two-button PlayerGui fixture through the owned `GuiService:Select` engine
surface, follows its explicit downward selection edge, activates the exact
second button with Enter, and then toggles navigation off. The activation
observer requires one Keyboard-typed delivery plus Keyboard last/preferred
input state. The fixture is verifier-only and establishes general production
keyboard GUI traversal and activation; it is not a claim that every supplied
CoreGui surface has received separate keyboard acceptance or perceptual review.

## Chrome Music product exclusion

The supplied Studio corroboration file
`ExtraContent/scripts/CoreScripts/Modules/Chrome/Integrations/MusicUtility/MusicEntrypoint.lua`
hashes to
`4560f4ba924b4583431e9603d7ca84f3b65e3a5fb5a03236a366f85df8d09d5b`.
It registers `music_entrypoint` with initial availability `Available`. The
matching Player model literal contract establishes the same integration id;
Studio is used only to corroborate Chrome's public package relationship.

The product objective explicitly removes Music from the current UI. The owned
Player policy therefore uses Chrome's existing
`AvailabilitySignal:forceUnavailable()` contract after Chrome initializes.
That contract is the package's normal filtering mechanism for menus,
shortcuts, focus, notifications, and activation. It does not alter the hashed
reference model or delete an arbitrary GUI descendant. A packaged semantic
verifier rejects any `.music_entrypoint` descendant or Chrome text label equal
to `Music`. Three consecutive 400-frame Chrome/leaderboard runs contained
neither and exited 0 with 22 final-frame draws.

## Chrome Trust & Safety / Report

The supplied Studio file
`ExtraContent/scripts/CoreScripts/Modules/Chrome/Integrations/TrustAndSafety.lua`
hashes to
`d2b9b44327ea3cc7572b77ca0d13b1ad50ecdd03ddb7e0e2f518f93251238d5d`.
It registers `trust_and_safety`, labels it `Report`, and toggles the actual
`SettingsHub.Instance.ReportAbusePage`. Studio corroborates the package
relationship; the hash-recorded Player model and packaged pointer trace remain
the runtime authority.

The first unchanged-package failure occurred while React created the report
description TextBox: `OverlayNativeInput is not a valid member of TextBox`.
The supplied
`ExtraContent/scripts/CoreScripts/Modules/AbuseReportMenu/Components/ReportTextEntry.lua`
hashes to
`84b01359e00c324fd661107e5954a7298b74a3764876fbe0631b56b67a5ea198`
and assigns `OverlayNativeInput=true` to `AbuseReportsText`. The supplied API
dump defines that member as a hidden, RobloxScriptSecurity, non-replicated,
non-serialized boolean in category `Data`. Independent Android Player binaries
also contain the exact member string: the arm64 library hashes to
`4e416eb4062ade7a2baff9bc94f9a1d92f10c1854c344f0be83abdf9267535e7`
and the x86_64 library hashes to
`4ecb8f47537996c44cb60cecfd58ba6b5c394d77daa2716ba4ceebe30b8695a7`.
The owned native TextBox descriptor now matches those reflection and storage
semantics; desktop rendering/input remains owned by the shared TextBox path,
while platforms with a native overlay can consume the preference.

VoiceChatCore then exposed a separate feature-advertisement mismatch. The
supplied PC settings enable `VoiceChatServiceManagerUseAvatarChat`, and the
package's shared gate queries the `AvatarChatServiceEnabled` engine feature.
The service and `GetClientFeaturesAsync` contract were already implemented,
but that engine feature was absent, causing SettingsHub to pass `nil` into
CoreVoiceManager. Registering the feature with the DataModel factory makes the
package select its service-backed route.

Three consecutive 400-frame packaged Report runs activate
`MainCanvas.trust_and_safety`, render the bounded selected Report page with its
official prompts and `AbuseReportsText`, retain Chrome, and exit 0 with 34
final-frame draws. A separate 400-frame official leaderboard regression exits
0 with 22 draws.

## Chrome Respawn

The supplied Studio Chrome integration files provide direct static
corroboration for the packaged runtime route:

| File | SHA-256 | Observation |
|---|---|---|
| `ExtraContent/scripts/CoreScripts/Modules/Chrome/Integrations/OverflowMenu.lua` | `959613058d43498ef413f58e20300f5a1360c26acf5e9a44ea57635881b6d668` | registers `respawn`, follows SettingsHub's respawn behavior, and calls `RespawnUtils.respawnPage()` |
| `ExtraContent/scripts/CoreScripts/Modules/Chrome/Integrations/RespawnUtils.lua` | `3fd1a97dacfbce0eb039aa862ac1237bdd8696932b010f536fd225634e4512bd` | opens `SettingsHub.Instance.ResetCharacterPage` |
| `ExtraContent/scripts/CoreScripts/Modules/Chrome/Integrations/Pages/RespawnConfirmation.lua` | `8efbc7229688e7bf8ad05f0f80bee44d30cc33a6637b2c1d3c2eb91824269952` | defines the package's SideSheet confirmation variant; the active desktop profile selects the SettingsHub route |

The packaged pointer trace reaches
`MainCanvas.respawn`, then the real SettingsHub
`ResetCharacterButtonsContainer`. It renders the exact prompt
`Are you sure you want to respawn your character?`, a 200x48
`ResetCharacterButton` labeled `Respawn`, and the paired
`DontResetCharacterButton` labeled `Don't Respawn`. Activating the former
kills the current Humanoid through the unchanged package behavior.

The historical offline Player created its character while the DataModel was
backend-capable and then added `Network::Client` so CoreScripts saw the proper
frontend-only role. That transition also disabled `Player::onCharacterDied`,
leaving the authentic reset action at a dead ragdoll. The owned fix does not
weaken network-client authority: a non-reflected host contract activates only
for `Players.LocalPlayer` when Client is present and Server is absent. It uses
the existing five-second death timer and complete `Player::loadCharacter`
implementation, including the packaged R6/R15 model, Animator, joints,
scripts, Workspace parenting, and `SpawnerService` placement.

The same run exposed an older camera lifecycle mismatch. `CameraSubject`
could not transition to nil, strongly retaining the removed Humanoid. The
native setter now accepts nil, and the standalone host repairs only a nil
subject to the current local Humanoid; any valid experience-selected subject
is preserved. Three consecutive 1,800-frame runs prove a distinct replacement
character, health 100, exact new-Humanoid CameraSubject identity, a closed
confirmation page, retained Chrome, and 14 final-frame draws. A separate
400-frame official Chrome leaderboard round trip still exits 0 with 22 draws.

The supplied Studio CoreScript files were used only to corroborate the public
MarketplaceService event payloads required after activating `Examine Avatar`:

| File | SHA-256 | Observation |
|---|---|---|
| `ExtraContent/scripts/CoreScripts/Modules/InspectAndBuy/Components/InspectAndBuyBaseContainer.lua` | `8f24c2e6b0a79ff90cc98cdc01c82663df204005d467d9dde854ef8c810855d6` | subscribes to `PromptBulkPurchaseFinished(player, status, result)` and consumes `result.Items` |
| `ExtraContent/scripts/CoreScripts/Modules/InspectAndBuy/Components/InspectAndBuy.lua` | `498b3115da3f38788f40a8a2ed4a18e1b1bfd829a634997c93b7efec8ba94712` | connects `PromptBundlePurchaseFinished` to the existing `(player, itemId, isPurchased)` callback |
| `ExtraContent/scripts/CoreScripts/Modules/InspectAndBuy/Components/Container.lua` | `19a588c9cab8dfbfabbefcccf6a0eaf8b482c58d044198967e49ebb12b6bf538` | the wide overlay's `Activated` route calls `GuiService:CloseInspectMenu()` while the inner container consumes activation |

The owned native descriptors implement those event surfaces independently.
The unchanged action next required legacy `PromptBulkPurchaseRequested`,
variadic `Vector2:Min`/`Max`, and both reflected
`Players:CreateHumanoidModelFromUserId` names. Those native contracts are now
implemented. For the offline local identity only, the model request reuses the
packaged R15 character appearance and local player name; it does not add a Lua
substitute event, canned UI result, or enabled remote endpoint. The final
verifier requires an 83-descendant bounded responsive surface and proves its
official outside-overlay close route unmounts `RobloxGui.InspectAndBuy` before
Chrome and PlayerList reopen.

## Foundation `UDim2` constructor compatibility

The supplied Foundation `IconButton.lua` constructs its container size as
`UDim2.new(sizeX, UDim.new(0, containerSize.Y.Offset))`, where both arguments
are `UDim` values. This is direct source evidence for the current two-argument
constructor contract, not a visual inference. The historical native bridge
accepted only four numeric arguments and passed the userdata values through
numeric coercion, producing `UDim2.new(0, 0, 0, 0)` without a script error.

After adding the two-`UDim` overload, the unmodified supplied PlayerList
Foundation `CloseButton` changed from an authored/absolute `0x0` to `24x24`.
The packaged input trace then selected
`CoreGui.PlayerListReskin.Container.PlayerListPanel.Header.CloseButton`, closed
the real panel, selected the Chrome Leaderboard action again, and reopened the
populated panel. This is implemented engine compatibility evidence and is
shared by every current package that composes `UDim2` from axis `UDim` values.

The supplied `Modules/ErrorPrompt.lua` also provides direct compatibility
evidence for Instance lower-camel access: its button cleanup loop checks
`child.name ~= "ButtonLayout"`. The historical bridge intended to support this
alias but gated capitalization on a process-wide interned-name lookup. Because
the lowercase word `name` was already interned elsewhere, a concrete
`UIGridLayout` could not resolve its inherited `Name` property. Resolution now
tests the capitalized property, function, event, callback, or child candidate
against the actual Instance before applying the existing alias behavior. The
packaged startup log no longer contains the `UIGridLayout.name` failure.

## Current `CollisionGroupData` place format

Focused inspection used three supplied 2026 XML places without modifying or
packaging them:

| Reference | SHA-256 | Purpose |
|---|---|---|
| `/Users/q8j/Downloads/Baseplate.rbxlx` | `fa654d7fabc9f2dedf435a302342766d379a10ad6f90840b3909693e45cd2789` | current Studio single-Default-group acceptance place |
| `2026-place-files/Shoot a Brainrot.rbxlx` | `cd58687a4c4fb1aaf0bb2f96f24aa3e41e0d9a32f5580d444a598bb8bc93591e` | compact three-group matrix with non-collidable pairs |
| `2026-place-files/Grow a Garden.rbxlx` | `72ef139b216ca9a335988295d30587e6accf2ba3a3cce9a375dd1c0cdc967681` | fourteen-group matrix and longer current group names |

Base64 decoding and exact-consumption parsing produced the same version-1
record layout in all three files: one-byte version, one-byte registered count,
then for each group a one-byte ID, the byte value `4`, a four-byte
little-endian collision mask, a one-byte name length, and the name bytes.
Group IDs index mask bits directly. All observed matrices are symmetric;
unregistered high bits remain set in the serialized masks. This is direct
format observation, not an inferred substitute schema.

The owned implementation reflects the hidden serialized
`Workspace.CollisionGroupData` property and atomically installs the decoded
registry into the same `PhysicsService` matrix used by BasePart, contacts, and
raycasts. It rejects unsupported versions, invalid counts/IDs/mask widths,
duplicate IDs or names, invalid Default group placement, invalid lengths,
truncation, trailing bytes, and asymmetric registered-group pairs. The normal
XML place loader test consumes the exact three-group payload, proves exact
round-trip bytes, and proves the imported masks alter real Workspace ray
queries. A packaged 400-frame Player run loads the hashed downloaded Baseplate
through the same loader and exits successfully.

## Current Decal and Texture color tint

The supplied Studio-authored `/Users/q8j/Downloads/Baseplate.rbxlx` (SHA-256
`fa654d7fabc9f2dedf435a302342766d379a10ad6f90840b3909693e45cd2789`)
serializes `Color3` on both current surface-image classes: the Baseplate
`Texture` stores `(0, 0, 0)`, while the SpawnLocation `Decal` stores
`(1, 1, 1)`. The exact Studio `ReflectionMetadata.xml` used for corroborating
class identity hashes to
`a76f15a25e35d9f6d41cb4daae54f9f1bea4b21813b0b98dc2a2a6c8a899795d`.

The historical class discarded `Color3` during place loading and hard-coded
all decal vertex colors to white. `Decal.Color3` is now a reflected stored
property inherited by `Texture`; property changes use the existing graphics
invalidation path, and geometry generation converts the stored color to the
decal vertex tint while preserving transparency. A normal XML serializer test
loads a current-form `Color3` and local SpawnLocation content reference and
proves that the exact tint reaches the instantiated Decal rather than being
discarded. This implements the observed property and render semantics; it does
not claim the still-unimplemented current UV/rotation/texture-pack properties.

The same focused inspection confirmed that the current Baseplate texture and
Sky use numeric remote asset IDs whose bytes are not present in the supplied
Player, Studio, APK, or source content. No legacy sky face or unrelated studs
image is substituted for those IDs. Subsequent live asset-delivery inspection
resolved the exact references from the place: Baseplate `6444884337`, sky sides
`6444884337`, down `6444884785`, up `6412503613`, moon `6444320592`, and sun
`6196665106`. The owned Player preserves those canonical asset-delivery URLs
and enables the legacy cache's validated original-URL lookup. An integration
test uses an isolated empty cache, performs one online warm run, and then proves
fresh processes load the 400x400 Baseplate, 64x64 SpawnLocation, and six
1024x1024 sky faces while every HTTP(S) request is routed to a closed local
port. This proves repeat offline operation for fetched assets, not cold-offline
availability of arbitrary remote place content.

## Current Texture scale and offset

The official Creator Hub `Texture` reference and its texture-customization
guide establish the current public surface as `StudsPerTileU`,
`StudsPerTileV`, `OffsetStudsU`, and `OffsetStudsV`. The guide was retrieved as
Markdown from
`https://create.roblox.com/docs/en-us/parts/textures-decals.md`; its
2026-07-15 revision hashes to
`98cdaf8c3352831ac27ff7c070ded795c2b8904fd33ae8d167090445447e41b0`.
The official zero-offset and positive-0.5-stud reference images hash to
`02a2637d164f986ee7456163b2801e9dc17cee345f99271949f817a5646f836f`
and
`69b2f0ac9de107fdccd30572b3357451c92ed8adf128ed666b95e304298fba16`.
They are behavior references only and are not copied into source or packages.

Direct image comparison shows that positive U moves the repeated image right
and positive V moves it up. Against the historical generator's existing face
coordinates, the corresponding normalized UV translation is
`(-OffsetStudsU / StudsPerTileU, +OffsetStudsV / StudsPerTileV)`. The owned
implementation reflects and stores both offsets, invalidates live geometry on
changes, and applies that recovered translation in the block, wedge, torso,
sphere, cylinder, and CSG surface-generation paths. A render-scene-linked
native test generates an actual 8x2x6 textured block before and after a
0.5-stud U/V offset and proves every emitted UV moves by `(-0.25, +0.25)` for
2-stud tiles. The normal XML place loader independently proves scale and offset
values survive current-form RBXLX deserialization.

## Confidence labels

- **Observation** means a file name, manifest value, hash, or parsed structure.
- **Inference** means expected behavior derived from observations and must be
  tested against a fixture before becoming a compatibility claim.
- **Implemented** requires owned code plus a passing test.
- Exact 2026 enum metadata was recovered from Roblox's versioned API dump at
  `https://setup.rbxcdn.com/version-ed7d8193e8564b1f-API-Dump.json`. It defines
  `MakeupType` as Face=0/Lip=1/Eye=2,
  `MarketplaceItemPurchaseStatus` as Success=1 through PlaceInvalid=13,
  `PromptPublishAssetResult` as Success=1 through UnknownFailure=6, and
  `RaycastFilterType` as Exclude=0/Include=1 with the legacy names
  Blacklist/Whitelist.
- The downloaded versioned API dump hashes to
  `f5197bdcd415eb7e04b00437a593d2f307a6adfefc2a933f15e3f8c7992190f7`.
  Its `PhysicsService` descriptor establishes the current 32-group API surface:
  `RegisterCollisionGroup`, `UnregisterCollisionGroup`,
  `RenameCollisionGroup`, `IsCollisionGroupRegistered`,
  `GetRegisteredCollisionGroups`, `CollisionGroupSetCollidable`, and
  `CollisionGroupsAreCollidable`, while retaining the deprecated ID/list and
  part-assignment calls. The same dump marks `BasePart.CollisionGroup` as a
  serialized non-replicated string and `CollisionGroupId` as its serialized
  deprecated integer counterpart.
- The same versioned API dump defines
  `MarketplaceBulkPurchasePromptStatus` as Completed=1, Aborted=2, Error=3;
  `MarketplaceService.PromptBulkPurchaseRequestedV2` as a RobloxScript event
  carrying player, displayData, orderRequest, purchaserRobuxBalance,
  orderTotalRobux, options, and discountInformation; and
  `SocialService.OpenShareSheetWithLink` as a RobloxScript string event.
- The supplied `CoreScripts/Modules/Common/RagdollRigging.lua` constructs R15
  shoulder attachments with `CFrame.fromMatrix(position, right, up)`. The
  supplied call sites therefore require the real implicit third basis vector,
  not merely the four-vector overload.
- The supplied current Player camera modules construct `RaycastParams` for
  Popper, Invisicam, VR vehicle camera, path display, click-to-move, and VR
  navigation. Their observed fields are `FilterDescendantsInstances`,
  `FilterType`, `IgnoreWater`, and `RespectCanCollide`, followed by real
  `Workspace:Raycast(origin, direction, params)` calls. The supplied
  `CoreScripts/CoreScripts/AvatarContextMenu.lua` also constructs an Exclude
  filter at line 109. These call sites establish that a constructor without a
  native physics query would not satisfy the Player dependency.
