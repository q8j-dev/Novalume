# Layout migration plan

Moves are incremental and preserve history where practical. Compatibility
include shims are temporary and receive removal milestones in `STATUS.md`.

| Legacy location | Destination / retained role | Status |
|---|---|---|
| `Base/`, foundational `App/util` | `engine/core`, then focused utility modules | Complete `Base` tree moved into public includes, owned sources, and historical project metadata; core sources compile in `rbx-core`; focused `App/util` migration continues |
| `App/reflection` | `engine/reflection` | Moved; compatibility headers remain during consumer migration |
| `App/v8xml`, serializer sources | `engine/serialization` | Moved; compatibility headers remain during consumer migration |
| `App/v8datamodel`, `v8tree` | `engine/datamodel` | Complete tracked tree/runtime source moved; both C++20 targets compile; dependency-cycle cleanup remains |
| `App/util/{ContentProvider,CacheableContentProvider,MeshContentProvider,TextureContentProvider}.cpp` | `engine/datamodel/runtime/src` with portable mount policy in `engine/assets` | Moved with history; headers were already in the DataModel public surface; networking/cache boundary extraction remains |
| `App/v8world`, `v8kernel`, solver | `engine/simulation` | Complete tracked source moved; C++20 target compiling; dependency-cycle cleanup remains |
| `App.BulletPhysics` | `third_party/bullet_historical`, consumed only through `engine/physics` | Complete 556-file checked-in Bullet corpus moved and compiling through the owned physics target; maintained pinned Bullet replacement remains |
| `CSG` | `engine/geometry/csg` | Complete 355-file geometry family moved; owned CMake and preserved Xcode targets compile on arm64; Studio/Player/tool/project consumers updated |
| Animator/track/keyframe/pose sources in `App` | `engine/animation` | Complete authentic source set moved and compiling |
| `App/humanoid`, `App/include/humanoid` | `engine/avatar/humanoid` | Complete state-machine source/header family moved and compiling as `Roblox::AvatarHumanoid`; authentic R15 semantic work remains |
| `App/security`, `App/include/security` | `engine/security` | Complete tracked security family moved and compiling as `Roblox::Security`; proprietary VMProtect includes removed from owned security/humanoid sources |
| `App/lz4` | `third_party/lz4_historical`, consumed through named serialization/scripting dependencies | Complete checked-in seven-file snapshot moved and compiling as `Roblox::Lz4`; replacing the historical snapshot with a maintained pinned implementation remains required |
| `App/voxel`, `App/voxel2`, matching public headers | `engine/terrain` | Complete tracked terrain family moved; shared voxel/grid/meshing code compiles as `Roblox::Terrain`, while serialization and DataModel voxelization compile in the later `Roblox::TerrainRuntime` integration target to preserve dependency direction |
| `Rendering/g3d`, `Rendering/RbxG3D` | `engine/core/math` | Complete tracked source moved and compiling; include-case cleanup remains |
| `Rendering/GfxCore` contracts | `engine/rendering/core` | Moved; compatibility headers remain during consumer migration |
| `Rendering/GfxCore/{D3D9,D3D11,GL}` | migration-only renderer sources | Disabled by default; retain until full verification and explicit user authorization |
| `Rendering/GfxBase` | `engine/rendering/foundation` | Complete tracked source moved; C++20 target compiling; public-surface reduction remains |
| `Rendering/AppDraw` | `engine/rendering/adorn` | Complete tracked source moved and compiling |
| `Rendering/GfxRender` | `engine/rendering/scene`, `materials`, `text`, `ui` | Tracked C++ source moved and split; explicit-include and dependency migration in progress |
| `content`, `PlatformContent` | preserved base mounts plus versioned generated resource overlays/manifests | Active; base and PC platform content stay preserved, scoped Player UI is generated outside source, and `rbxasset://` uses ordered mounts |
| FMOD call sites and `fmod/` | `engine/audio` plus open-source backend; preserved binary/header corpus quarantined at `third_party/quarantined/fmod` | Quarantine move complete; pinned miniaudio runtime and offline tests now build under `engine/audio`, while the real `Sound`/`SoundService` DataModel bridge still requires conversion |
| `App/script`, `App/include/lua`, embedded Lua VM | `engine/scripting` | Complete tracked source and public Lua-header families moved; scripting archive compiles after case-sensitive include normalization; Player integration remains active |
| `App/tool` | `engine/interaction/tools` | Complete tracked source moved; C++20 target migration active |
| `App/gui` | `engine/ui/runtime` | Complete tracked source moved; C++20 target migration active |
| Root developer utilities and deployment wrappers | `tools/*`, `tools/deployment`, and `tests/simulation_regression` | Include checker, settings comparison, policy refresh, CoreScript converter, deployment orchestration, and simulation regression utility moved with history; project-relative paths updated |
| `PropertySheets`, `CustomBuildRules.*` | `cmake/historical/msbuild` | Historical MSBuild infrastructure moved; all project imports use stable solution-root paths |
| `QTitanRibbon` | `third_party/studio/qtitan_ribbon` | Complete Studio-only third-party library moved; Studio project references updated; excluded from Player |
| `boostlibs` | `third_party/boost/build` | Historical Boost build metadata moved; Visual Studio and Xcode consumers updated |
| Root test projects and runners | `tests/unit`, `tests/support`, `tests/integration`, `tests/services`, `tests/platform`, and `tests/runners` | DataModel/core test sources and libraries, the complete Player integration corpus, managed/RCC/mobile tests, hooks, and runner projects moved with history; Visual Studio and Xcode references updated |
| `Network` | `engine/networking` | Complete 633-file family moved; CMake, Visual Studio, Xcode, platform, app, and test include roots updated; target dependency cleanup remains |
| `ClientBase`, `ClientShared`, `GameChat` | `apps/player/shared/client_base`, `apps/player/shared/client`, `engine/ui/chat` | Complete shared Player support and Xbox in-game chat families moved; all build/project consumers updated |
| Mac/Windows/iOS/Android shells | `platform/<target>` and `apps/player/<target>` | Shared macOS contract active; complete historical macOS and Windows Player sources/projects moved under their app/platform owners with project references updated; iOS/Android shell migration pending |
| Hybrid mobile and Xbox shells | `platform/mobile/hybrid`, `platform/xbox/client`, and `third_party/samples/xbox_network_mesh` | Complete preserved families moved; Android/iOS hybrid references and Xbox solution/project paths updated |
| `WindowsClient` / `RobloxMac` player entry | `apps/player` | Platform-neutral target started |
| `RobloxStudio`, `StudioPlugins`, `BuiltInPlugins` | `apps/studio/source`, `apps/studio/plugins/custom`, `apps/studio/plugins/built_in` | Complete preserved Studio-only families moved; out of Player graph and retained for authorized schema/R15/RBXL research |
| `RCCService`, `Roblox.RccServiceArbiter` | `apps/services/rcc/source`, `apps/services/rcc_arbiter/source` | Complete preserved server application families moved; excluded from Player graph, project/deployment/test paths updated |
| Installer, XULRunner, Qt, console projects | documented obsolete/tool quarantine | Excluded from new player graph; no dumping-ground move |
| `shaders/source` | `shaders/source`, includes, varying, manifests | Existing location retained; conversion planned |
| fixture utilities | `tools/fixture_inspector` | Started; deterministic Windows Player binary UI contract reporting is present |
| unit/integration/render fixtures | `tests/*` | Started |

## Enforced dependency direction

```text
apps -> engine modules + one platform host
platform adapters -> platform contract + narrowly required engine interfaces
render scene/UI -> render core contract
render core bgfx implementation -> bgfx
engine core -X-> apps, OS UI frameworks, graphics APIs
```

Platform-native surface values cross the shared boundary as integer-sized
opaque values in `NativeSurface`; conversion to bgfx `PlatformData` is owned by
the bgfx implementation. AppKit code is Objective-C++ under `platform/macos`.

The automated `check-dependency-boundaries` target rejects OS/API headers in
shared engine headers. It will grow with each migrated module.

The historical D3D/OpenGL source directories are not cleanup candidates merely
because they are excluded from the default graph. Deletion requires both the
complete verification program to pass and a separate explicit instruction from
the user.
