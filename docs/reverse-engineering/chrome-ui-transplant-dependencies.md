# Chrome/Unibar transplant — full dependency closure

This documents what a full backport of the in-experience Chrome/Unibar UI
into a different codebase actually requires, beyond the Lua presentation
layer in `content/scripts/Modules/InExperience/`. It was produced while
assembling an external transplant package and is kept here because it's
useful reference for any future work moving this UI outside Novalume.

## Lua presentation layer

`content/scripts/Modules/InExperience/{Chrome,ChromeService,Tokens,
IntegrationIcons,BuilderIcon}.lua` — the state/presentation split and
measured geometry described in `docs/reverse-engineering/in-experience-ui.md`.
Portable as source to any Lua host that satisfies the native requirements
below.

## Backported native services and GUI primitives

These engine service/component pairs directly implement the Chrome/
in-experience UI runtime contract:

| File (under `engine/datamodel/runtime/`) | Backs |
|---|---|
| `include/v8datamodel/UIComponent.h`, `src/UIComponent.cpp` | `UICorner`, `UIGridLayout`, `UIPageLayout`, `UIAspectRatioConstraint`, `UIFlexItem`, `UIStroke`, `UIGradient`, `UIDragDetector`, `UIListLayout` flex extensions |
| `include/v8datamodel/GuiObject.h`, `src/GuiObject.cpp` | `GuiButton.Activated` multi-input path, `UIPageLayout` integration, general GuiObject layout/draw |
| `include/v8datamodel/GuiBase2d.h`, `src/GuiBase2d.cpp` | `AutoLocalize`, legacy `Localize`, `RootLocalizationTable` |
| `include/v8datamodel/ScreenGui.h`, `src/ScreenGui.cpp` | `Enabled`, `ResetOnSpawn`, `DisplayOrder`, `IgnoreGuiInset`, `ZIndexBehavior`, sibling draw/input/gesture/selection ordering |
| `include/v8datamodel/GuiService.h`, `src/GuiService.cpp` | Safe-area insets, `GuiService:Select`, special keys, dialog/message types |
| `include/v8datamodel/UserInputService.h`, `src/UserInputService.cpp` | Touch capability, `TouchEnabled`/`LastInputType`/`PreferredInput`, gamepad control-image paths |
| `include/v8datamodel/ScrollingFrame.h`, `src/ScrollingFrame.cpp` | `ScrollingDirection`, `ScrollBarInset`, `VerticalScrollBarPosition` |
| `include/v8datamodel/ViewportFrame.h`, `src/ViewportFrame.cpp` | Native `ViewportFrame` primitive used by avatar/settings panels |
| `include/v8datamodel/SafetyService.h`, `src/SafetyService.cpp` | Report/capture surface signals, screenshot job queue, `IsCaptureModeForReport` — the literal blocker that gated `SettingsHub`/`TopBarApp` during the 2026-07-17 pause |
| `include/v8datamodel/TextChatService.h`, `src/TextChatService.cpp` | Modern chat service surface (`ChatVersion`, default channels/commands, translation flag) |
| `include/v8datamodel/TextChannel.h`, `src/TextChannel.cpp` | Per-channel message delivery backing `TextChatService` |
| `include/v8datamodel/ChatService.h`, `src/ChatService.cpp` | Legacy chat service `TextChatService` still interoperates with |

`src/factoryregistration.cpp` is the single class-registration site for the
entire engine (1,000+ lines, hundreds of unrelated classes). It's where the
`RBX_REGISTER_CLASS(...)`/`RBX_REGISTER_ENUM(...)` entries for all of the
above live — grep it for `UICorner`, `SafetyService`, `TextChatService`, etc.
rather than treating the whole file as Chrome-specific.

## The font-resolution chain

`BuilderIcon.lua` sets `Enum.Font.BuilderIconsRegular`/`BuilderIconsFilled`
on every icon `TextLabel`. Resolving that enum to a renderable glyph runs
through:

| File | Role |
|---|---|
| `App/include/util/Font.h` | The `Font` value type (family + weight) everything else builds on |
| `engine/value_types/src/Font.cpp` | Maps legacy numeric font IDs to family URIs, including `kBuilderIconsRegular`/`kBuilderIconsFilled` → `rbxasset://LuaPackages/Packages/_Index/BuilderIcons/BuilderIcons/Font/BuilderIcons-*.ttf` |
| `engine/datamodel/runtime/include/v8datamodel/GuiText.h`, `src/GuiText.cpp` | `applyFontFace()` — pattern-matches the family string (`"BuilderIcons-Filled"`, `"BuilderIcons"`, `"BuilderSans"` + weight) to the internal `TextService::Font` enum |
| `engine/datamodel/runtime/include/v8datamodel/TextService.h`, `src/TextService.cpp` | Defines that enum (`FONT_BUILDER_ICONS_REGULAR = 101`, `FONT_BUILDER_ICONS_FILLED = 102`, `FONT_BUILDERSANS*`) and does the text shaping/measurement `GuiText` calls into |

The rasterization step that turns a resolved font enum into on-screen pixels
lives in `engine/rendering/scene/src/VisualEngine.cpp` — it resolves the
`rbxasset://...BuilderIcons-Regular.ttf` path to a loadable asset alongside
every other font/texture the renderer knows about (grep it for
`BuilderIcons`/`BuilderSans`). It's a general-purpose renderer file, not
Chrome-specific.

The actual `.ttf`/`.json` font files (`BuilderIcons-Regular.ttf`,
`BuilderIcons-Filled.ttf`, `BuilderSans.json` and its weight variants) are
proprietary binary assets pulled from a real Player build and are **not** in
this repository. Any transplant needs to source those independently.

## Why there's no smaller, self-contained slice

The files above transitively include a meaningful fraction of Novalume's own
architecture:

- **Instance/Reflection core**: `v8tree/Instance.h`, `v8tree/Service.h`,
  `reflection/Object.h`.
- **DataModel**: `v8datamodel/DataModel.h`, `Workspace.h`, `WorldModel.h`,
  `PlayerGui.h`, plus sibling GUI classes: `TextBox.h`, `TextButton.h`,
  `TextLabel.h`, `ImageButton.h`, `ImageLabel.h`, `Frame.h`, `BillboardGui.h`,
  `ClickDetector.h`, `DialogRoot.h`, `GuiLayerCollector.h`, `GuiCore.h`,
  `StyleSheet.h`, `CanvasGroup.h`.
- **Networking**: `network/Player.h`, `network/Players.h`,
  `network/WebChatFilter.h`.
- **Physics/rendering**: `v8world/World.h`, `v8world/ContactManager.h`,
  `GfxBase/Adorn.h`, `GfxBase/AdornBillboarder2D.h`,
  `GfxBase/ViewportTextureProvider.h`, `Gui/GuiDraw.h`, `Gui/GuiEvent.h`,
  `GfxBase/Typesetter.h`.
- **Other services**: `ContextActionService.h`, `CollectionService.h`,
  `GamepadService.h`, `TouchInputService.h`, `TweenService.h`,
  `LocalizationService.h`, `LocalizationTable.h`, `HttpRbxApiService.h`,
  `RbxAnalyticsService.h`, `JointsService.h`, `MegaCluster.h`,
  `PartInstance.h`, `Camera.h`.
- **Utility/filter layer**: `util/UDim.h`, `util/Rect.h`, `util/Font.h`,
  `util/BrickColor.h`, `util/ContentId.h`, `util/DateTime.h`,
  `util/Quaternion.h`, `util/UserInputBase.h`, `util/ContentFilter.h`,
  `Gui/ProfanityFilter.h`, `security/SecurityContext.h`, `rbx/signal.h`,
  `rbx/core/EngineFeatures.h`, `rbx/ui/*`.

This matches `docs/reverse-engineering/in-experience-ui.md`'s own conclusion
that Chrome, PlayerList, ExperienceChat, Backpack, and the in-game menu
"must be backported as one connected in-experience system" — they share this
same well, so there's no smaller correct cut.

## Recommended paths for a transplant

1. **Target shares Novalume's Instance/Reflection/DataModel architecture**
   (another tree in the same Roblox-source lineage): diff/patch at the
   engine-source level — `git diff`/`git format-patch` against the commits
   that introduced the files above, applied/rebased onto the target tree —
   rather than hand-copying a curated file list. This preserves the include
   graph correctly.
2. **Target does not share that architecture** (different `Instance` base,
   reflection system, or rendering backend): none of this C++ is portable as
   source. Re-implement the *behavior* — measured pixel geometry, the
   state/presentation contract, the `SafetyService`/`TextChatService` API
   surface — against the target engine's own primitives, using the Lua
   `InExperience/` module and `docs/reverse-engineering/in-experience-ui.md`
   as the spec.
3. Either way, budget for the full transitive list above, not just the
   directly-backported files.
