#include "PlayerRuntime.h"
#include "LocalPlayerThumbnailProvider.h"
#include "rbx/platform/Utf8Path.h"

#include "GfxBase/RenderSettings.h"
#include "GfxBase/FrameRateManager.h"
#include "GfxBase/RenderStats.h"
#include "GfxBase/FileMeshData.h"
#include "GfxBase/Typesetter.h"
#include "GfxCore/Device.h"
#include "GfxCore/Framebuffer.h"
#include "GfxCore/Texture.h"
#include "GfxRender/GlobalShaderData.h"
#include "GfxRender/AdornRender.h"
#include "GfxRender/SceneManager.h"
#include "GfxRender/SceneUpdater.h"
#include "GfxRender/Sky.h"
#include "GfxRender/TextureCompositor.h"
#include "GfxRender/TextureAtlas.h"
#include "GfxRender/TextureManager.h"
#include "GfxRender/VisualEngine.h"
#include "GfxRender/RenderView.h"
#include "GfxRender/ViewportRenderer.h"
#include "Script/LuaSettings.h"
#include "Script/script.h"
#include "Script/ScriptContext.h"
#include "util/Http.h"
#include "util/Profiling.h"
#include "v8datamodel/BasicPartInstance.h"
#include "v8datamodel/Bindable.h"
#include "v8datamodel/AchievementService.h"
#include "v8datamodel/AvatarChatService.h"
#include "v8datamodel/Attachment.h"
#include "v8datamodel/Camera.h"
#include "v8datamodel/CaptureService.h"
#include "v8datamodel/CommonVerbs.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/CorePackages.h"
#include "v8datamodel/CoreGuiConfiguration.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/DataModelPatch.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/factoryregistration.h"
#include "v8datamodel/FastLogSettings.h"
#include "v8datamodel/GameBasicSettings.h"
#include "v8datamodel/GamepadService.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/GenericChallengeService.h"
#include "v8datamodel/TweenService.h"
#include "v8datamodel/GuiObject.h"
#include "v8datamodel/InputObject.h"
#include "v8datamodel/ImageButton.h"
#include "v8datamodel/ImageLabel.h"
#include "v8datamodel/JointInstance.h"
#include "v8datamodel/Lighting.h"
#include "v8datamodel/PathfindingService.h"
#include "v8datamodel/LocalStorageService.h"
#include "v8datamodel/LinkingService.h"
#include "v8datamodel/LocalizationService.h"
#include "v8datamodel/PlatformService.h"
#include "v8datamodel/EventIngestService.h"
#include "v8datamodel/FaceAnimatorService.h"
#include "v8datamodel/FeatureRestrictionManager.h"
#include "v8datamodel/Folder.h"
#include "v8datamodel/RbxAnalyticsService.h"
#include "v8datamodel/RtMessagingService.h"
#include "v8datamodel/ExperienceService.h"
#include "v8datamodel/ExperienceNotificationService.h"
#include "v8datamodel/SessionService.h"
#include "v8datamodel/ScriptProfilerService.h"
#include "v8datamodel/VRService.h"
#include "v8datamodel/PolicyService.h"
#include "v8datamodel/ProximityPrompt.h"
#include "v8datamodel/MemStorageService.h"
#include "v8datamodel/MessageBusService.h"
#include "v8datamodel/MeshContentProvider.h"
#include "v8datamodel/MeshPart.h"
#include "v8datamodel/ModernAvatar.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/IXPService.h"
#include "v8datamodel/PhysicsSettings.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/PartCookie.h"
#include "v8datamodel/PostEffect.h"
#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/ScreenGui.h"
#include "v8datamodel/ServerScriptService.h"
#include "v8datamodel/StarterPlayerService.h"
#include "v8datamodel/Frame.h"
#include "v8datamodel/Sky.h"
#include "v8datamodel/TextChatService.h"
#include "v8datamodel/TextChatConfiguration.h"
#include "v8datamodel/TextService.h"
#include "v8datamodel/TextButton.h"
#include "v8datamodel/TextLabel.h"
#include "v8datamodel/TextBox.h"
#include "v8datamodel/UIComponent.h"
#include "v8datamodel/TelemetryService.h"
#include "v8datamodel/ViewportFrame.h"
#include "v8world/Primitive.h"
#include "v8datamodel/VideoFrame.h"
#include "v8datamodel/VideoCaptureService.h"
#include "v8datamodel/Value.h"
#include "util/Content.h"
#include "v8datamodel/WorldModel.h"
#include "v8datamodel/PlayerScripts.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/UserInputService.h"
#include "audio/SoundService.h"
#include "audio/SoundChannel.h"
#include "audio/AudioGraph.h"
#include "lua/lua.hpp"
#include "FastLog.h"
#include "network/Player.h"
#include "network/Players.h"
#include "voxel2/Grid.h"
#include "humanoid/Humanoid.h"
#include "v8datamodel/AnimationTrack.h"
#include "Client.h"
#include "ClientReplicator.h"
#include "network/api.h"
#include "Server.h"
#include "ServerReplicator.h"
#include "security/SecurityContext.h"
#include "util/RunStateOwner.h"
#include "v8xml/Serializer.h"
#include "v8xml/WebParser.h"
#include "RenderSettingsItem.h"
#include "rbx/core/BuildInfo.h"

#include <algorithm>
#include <atomic>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

FASTFLAG(UseDynamicTypesetterUTF8)
FASTFLAG(UserAllCamerasInLua)
FASTFLAG(Durango3DBackground)
DYNAMIC_FASTFLAG(ContentProviderHttpCaching)
DYNAMIC_FASTFLAG(HttpZeroLatencyCaching)

namespace rbx::player {
namespace {

RBX::Enums::ScreenOrientation screenOrientation(
    rbx::platform::DisplayOrientation orientation)
{
    switch (orientation) {
    case rbx::platform::DisplayOrientation::landscapeRight:
        return RBX::Enums::SCREEN_ORIENTATION_LANDSCAPE_RIGHT;
    case rbx::platform::DisplayOrientation::portrait:
        return RBX::Enums::SCREEN_ORIENTATION_PORTRAIT;
    case rbx::platform::DisplayOrientation::landscapeLeft:
        return RBX::Enums::SCREEN_ORIENTATION_LANDSCAPE_LEFT;
    }
    throw std::invalid_argument("unknown display orientation");
}

constexpr unsigned long kLauncherFirstSettledFrame = 240;
constexpr unsigned long kLauncherSecondSettledFrame = 420;

struct LauncherPixelEvidence
{
    std::size_t colorBuckets = 0;
    std::size_t spatialEdges = 0;
    unsigned int minimumLuminance = 255;
    unsigned int maximumLuminance = 0;
    double luminanceDeviation = 0.0;
};

struct LauncherTemporalPixelEvidence
{
    std::size_t changedPixels = 0;
    std::array<std::size_t, 4> changedPixelsByQuadrant{};
    double meanChannelDelta = 0.0;
};

LauncherPixelEvidence analyzeLauncherPixels(
    const std::vector<std::uint8_t>& pixels,
    unsigned int width, unsigned int height)
{
    const std::size_t expectedSize =
        static_cast<std::size_t>(width) * height * 4U;
    if (pixels.size() != expectedSize || width == 0 || height == 0)
        throw std::runtime_error(
            "Durango launcher temporal readback has invalid dimensions");

    LauncherPixelEvidence evidence;
    std::array<bool, 32768> buckets{};
    double luminanceSum = 0.0;
    double luminanceSquaredSum = 0.0;
    for (std::size_t pixel = 0; pixel < expectedSize / 4U; ++pixel)
    {
        const std::size_t index = pixel * 4U;
        const unsigned int red = pixels[index + 0U];
        const unsigned int green = pixels[index + 1U];
        const unsigned int blue = pixels[index + 2U];
        const std::size_t bucket = ((red >> 3U) << 10U) |
            ((green >> 3U) << 5U) | (blue >> 3U);
        if (!buckets[bucket])
        {
            buckets[bucket] = true;
            ++evidence.colorBuckets;
        }

        const unsigned int luminance =
            (54U * red + 183U * green + 19U * blue) >> 8U;
        evidence.minimumLuminance =
            std::min(evidence.minimumLuminance, luminance);
        evidence.maximumLuminance =
            std::max(evidence.maximumLuminance, luminance);
        luminanceSum += luminance;
        luminanceSquaredSum +=
            static_cast<double>(luminance) * luminance;

        const unsigned int x = static_cast<unsigned int>(pixel % width);
        if (x != 0)
        {
            const std::size_t previous = index - 4U;
            const int previousLuminance =
                (54 * pixels[previous + 0U] +
                 183 * pixels[previous + 1U] +
                 19 * pixels[previous + 2U]) >> 8;
            evidence.spatialEdges +=
                std::abs(static_cast<int>(luminance) - previousLuminance) >= 6;
        }
    }

    const double pixelCount = static_cast<double>(expectedSize / 4U);
    const double mean = luminanceSum / pixelCount;
    evidence.luminanceDeviation = std::sqrt(std::max(
        0.0, luminanceSquaredSum / pixelCount - mean * mean));
    return evidence;
}

LauncherTemporalPixelEvidence compareLauncherPixels(
    const std::vector<std::uint8_t>& first,
    const std::vector<std::uint8_t>& second,
    unsigned int width, unsigned int height)
{
    const std::size_t expectedSize =
        static_cast<std::size_t>(width) * height * 4U;
    if (first.size() != expectedSize || second.size() != expectedSize)
        throw std::runtime_error(
            "Durango launcher temporal readbacks have mismatched dimensions");

    LauncherTemporalPixelEvidence evidence;
    std::uint64_t totalChannelDelta = 0;
    for (std::size_t pixel = 0; pixel < expectedSize / 4U; ++pixel)
    {
        const std::size_t index = pixel * 4U;
        const unsigned int channelDelta =
            static_cast<unsigned int>(std::abs(
                static_cast<int>(second[index + 0U]) - first[index + 0U])) +
            static_cast<unsigned int>(std::abs(
                static_cast<int>(second[index + 1U]) - first[index + 1U])) +
            static_cast<unsigned int>(std::abs(
                static_cast<int>(second[index + 2U]) - first[index + 2U]));
        totalChannelDelta += channelDelta;
        if (channelDelta < 12U)
            continue;

        ++evidence.changedPixels;
        const unsigned int x = static_cast<unsigned int>(pixel % width);
        const unsigned int y = static_cast<unsigned int>(pixel / width);
        const std::size_t quadrant =
            (y >= height / 2U ? 2U : 0U) + (x >= width / 2U ? 1U : 0U);
        ++evidence.changedPixelsByQuadrant[quadrant];
    }
    evidence.meanChannelDelta = static_cast<double>(totalChannelDelta) /
        static_cast<double>(expectedSize / 4U) / 3.0;
    return evidence;
}

class DesktopClientSettings final : public RBX::FastLogJSON
{
public:
    void ProcessVariable(const std::string& name, const std::string& value,
        FastVarType) override
    {
        // CorePackages defines many rollout flags after bootstrap. The
        // synchronized path retains those values until DefineFastFlag runs.
        FLog::SetValueFromServer(name, value);
    }
};

void loadDesktopClientSettings()
{
    std::string response;
    RBX::Http("https://clientsettingscdn.roblox.com/v2/settings/application/PCDesktopClient")
        .get(response);
    if (response.empty())
        throw std::runtime_error("PCDesktopClient settings response was empty");
    DesktopClientSettings settings;
    boost::shared_ptr<const RBX::Reflection::ValueTable> root;
    if (!RBX::WebParser::parseJSONTable(response, root) || !root)
        throw std::runtime_error("PCDesktopClient settings response was invalid JSON");
    const auto applicationSettings = root->find("applicationSettings");
    if (applicationSettings == root->end() ||
        !applicationSettings->second.isType<
            boost::shared_ptr<const RBX::Reflection::ValueTable>>())
        throw std::runtime_error("PCDesktopClient settings response omitted applicationSettings");
    const boost::shared_ptr<const RBX::Reflection::ValueTable> values =
        applicationSettings->second.cast<
            boost::shared_ptr<const RBX::Reflection::ValueTable>>();
    std::size_t loaded = 0;
    for (const auto& [name, value] : *values) {
        if (!value.isType<std::string>())
            continue;
        loaded += settings.DefaultHandler(name, value.cast<std::string>()) ? 1U : 0U;
    }
    if (loaded == 0)
        throw std::runtime_error("PCDesktopClient settings contained no fast variables");
    std::cout << "PCDesktopClient fast variables=" << loaded << '\n';
}

void loadIxpFlagValues(const std::filesystem::path& clientSettingsRoot)
{
    if (clientSettingsRoot.empty())
        return;
    const std::filesystem::path path = clientSettingsRoot / "IxpSettings.json";
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return;
    const std::string response((std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());
    boost::shared_ptr<const RBX::Reflection::ValueTable> values;
    if (!RBX::WebParser::parseJSONTable(response, values) || !values)
        throw std::runtime_error("cached IxpSettings.json was invalid JSON");

    DesktopClientSettings settings;
    constexpr std::string_view suffix = "_IXPValue";
    std::size_t loaded = 0;
    for (const auto& [linkedName, linkedValue] : *values)
    {
        if (!linkedValue.isType<std::string>() ||
            !linkedName.ends_with(suffix))
            continue;
        const std::string encoded = linkedValue.cast<std::string>();
        const std::size_t separator = encoded.find(';');
        if (separator == std::string::npos)
            continue;
        const std::string resolvedName =
            linkedName.substr(0, linkedName.size() - suffix.size());
        const std::string resolvedValue = encoded.substr(0, separator);
        loaded += settings.DefaultHandler(resolvedName, resolvedValue) ? 1U : 0U;
    }
    std::cout << "cached IXP flag-linked values=" << loaded << '\n';
}

void initializeRuntime(const std::filesystem::path& clientSettingsRoot)
{
    static std::once_flag flag;
    std::call_once(flag, [clientSettingsRoot] {
        RBX::Profiling::init(false);
        static RBX::FactoryRegistrator factoryObjects;
        RBX::Http::init(RBX::Http::WinHttp,
            RBX::Http::CookieSharingSingleProcessMultipleThreads);
#if !defined(__EMSCRIPTEN__)
        try {
            loadDesktopClientSettings();
        } catch (const std::exception& error) {
            std::cerr << "PCDesktopClient settings unavailable; using packaged defaults: "
                      << error.what() << '\n';
        }
#endif
        loadIxpFlagValues(clientSettingsRoot);
        RBX::GameSettings::singleton();
        RBX::LuaSettings::singleton();
        RBX::DebugSettings::singleton();
        RBX::PhysicsSettings::singleton();
        RBX::Network::initWithoutSecurity();

        // UserGameSettings must be created while the engine is still anonymous;
        // its settings parent intentionally rejects lower-trust UI identities.
        RBX::GameBasicSettings::singleton();

        // Effective PCDesktopClient value for the supplied 2026 client. The
        // compiled default remains registered in GlobalSettings and this uses
        // the same synchronized override path as a production join.
        FLog::SetValueFromServer("LuaAppNonFinalThumbnailMaxRetries", "1");
        FLog::SetValueFromServer("PercentReportingNetworkProfileAfterStartup", "20");
        FLog::SetValueFromServer("PlayerListUseFocusNavHook2", "false");
        // This is the verified offline fallback for the supplied Player build;
        // the production PCDesktopClient profile above normally provides it.
        FLog::SetValueFromServer("EnableInGameMenuChrome", "true");
        // Asset delivery is expected to remain available across launches.  The
        // legacy cache implementation is content-validated on read; the zero
        // latency path keys the final response by the original asset URL so a
        // previously fetched place can also start without network access.
        FLog::SetValue("ContentProviderHttpCaching", "true", FASTVARTYPE_DYNAMIC);
        FLog::SetValue("HttpZeroLatencyCaching", "true", FASTVARTYPE_DYNAMIC);
        if (!DFFlag::ContentProviderHttpCaching ||
            !DFFlag::HttpZeroLatencyCaching)
            throw std::runtime_error("persistent asset delivery cache did not become active");
        // These 2016 diagnostic groups defaulted to verbose console output.
        // A current Player does not print every scheduler job and moving-part
        // heartbeat to its foreground terminal; doing so serializes thousands
        // of lines per second and materially stalls the render loop.
        FLog::SetValue("DataModelJobs", "0", FASTVARTYPE_STATIC);
        FLog::SetValue("GfxClusters", "0", FASTVARTYPE_STATIC);
        FLog::SetValue("UserInputProfile", "1", FASTVARTYPE_STATIC);
        if (!FLog::SetValue("UseInGameTopBar", "true", FASTVARTYPE_STATIC) ||
            !FLog::SetValue("UserAllCamerasInLua", "true", FASTVARTYPE_STATIC))
            throw std::runtime_error(
                "legacy TopBar or player-script camera flag is not registered");
        if (!FFlag::UserAllCamerasInLua)
            throw std::runtime_error("player-script camera flag did not become active");
    });
}

boost::shared_ptr<RBX::BasicPartInstance> createPart(
    RBX::Workspace* workspace, const char* name, const RBX::Vector3& size,
    const RBX::Vector3& position, const RBX::BrickColor& color)
{
    boost::shared_ptr<RBX::BasicPartInstance> part =
        RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
    part->setName(name);
    part->setAnchored(true);
    part->setPartSizeUi(size);
    part->setCoordinateFrame(RBX::CoordinateFrame(position));
    part->setColor(color);
    part->setParent(workspace);
    return part;
}

void configureLighting(RBX::Graphics::VisualEngine& visualEngine,
                       RBX::Lighting& lighting)
{
    const G3D::LightingParameters& sky = lighting.getSkyParameters();
    const float exposure = std::pow(2.0f, lighting.getExposureCompensation());
    const RBX::Color3 ambient = lighting.getGlobalAmbient() * exposure;
    const RBX::Vector3 sunDirection = -sky.lightDirection.unit();
    const RBX::Color3 sunColor = sky.lightColor * exposure *
        G3D::clamp(lighting.getGlobalBrightness(), 0.0f, 5.0f);

    RBX::Graphics::SceneManager* scene = visualEngine.getSceneManager();
    scene->setLighting(ambient, sunDirection, sunColor.min(RBX::Color3::white()),
        (sunColor * 0.4f).min(RBX::Color3::white()));
    const bool usesShadowMap = lighting.getGlobalShadows() &&
        lighting.getTechnology() >= RBX::Lighting::TECHNOLOGY_SHADOW_MAP;
    scene->setShadowMapConfiguration(usesShadowMap, lighting.getShadowSoftness());
    RBX::BloomEffect* bloom = lighting.findFirstChildOfType<RBX::BloomEffect>();
    scene->setBloomConfiguration(bloom && bloom->getEnabled(),
        bloom ? bloom->getIntensity() : 0.0f,
        bloom ? bloom->getSize() : 0.0f,
        bloom ? bloom->getThreshold() : 0.0f);
    scene->setFog(lighting.getFogColor(), lighting.getFogStart(), lighting.getFogEnd());
    RBX::Graphics::Sky* renderSky = scene->getSky();
    if (lighting.sky) {
        renderSky->setSkyBox(lighting.sky->skyRt, lighting.sky->skyLf,
            lighting.sky->skyBk, lighting.sky->skyFt, lighting.sky->skyUp,
            lighting.sky->skyDn);
        renderSky->setCelestialBodies(
            lighting.sky->sunTexture, lighting.sky->moonTexture);
        renderSky->update(sky, lighting.sky->getNumStars(),
            lighting.sky->drawCelestialBodies, lighting.sky->sunAngularSize,
            lighting.sky->moonAngularSize);
    } else {
        renderSky->setSkyBoxDefault();
        renderSky->setCelestialBodiesDefault();
        renderSky->update(sky, 3000, true, 21.0f, 11.0f);
    }
    scene->setSkyEnabled(!lighting.isSkySuppressed());
    scene->setClearColor(RBX::Color4(sky.skyAmbient, lighting.getClearAlpha()));

    RBX::Graphics::GlobalShaderData& globals = scene->writeGlobalShaderData();
    globals.FadeDistance_GlowFactor = RBX::Vector4(1000.0f, 0.001f, 1.0f, 0.0f);
    const float shadowFade = std::pow(G3D::clamp(
        sky.lightDirection.unit().y, 0.0f, 1.0f), 0.25f);
    const float shadowIntensity = usesShadowMap
        ? shadowFade * 0.75f
        : 0.0f;
    globals.OutlineBrightness_ShadowInfo =
        RBX::Vector4(0.43f, 0.2f, 0.0f, shadowIntensity);
}

RBX::KeyCode translateKey(rbx::platform::InputEvent::Key key)
{
    using Key = rbx::platform::InputEvent::Key;
    switch (key) {
    case Key::backspace: return RBX::SDLK_BACKSPACE;
    case Key::tab: return RBX::SDLK_TAB;
    case Key::enter: return RBX::SDLK_RETURN;
    case Key::escape: return RBX::SDLK_ESCAPE;
    case Key::space: return RBX::SDLK_SPACE;
    case Key::quote: return RBX::SDLK_QUOTE;
    case Key::comma: return RBX::SDLK_COMMA;
    case Key::minus: return RBX::SDLK_MINUS;
    case Key::period: return RBX::SDLK_PERIOD;
    case Key::slash: return RBX::SDLK_SLASH;
    case Key::zero: return RBX::SDLK_0;
    case Key::one: return RBX::SDLK_1;
    case Key::two: return RBX::SDLK_2;
    case Key::three: return RBX::SDLK_3;
    case Key::four: return RBX::SDLK_4;
    case Key::five: return RBX::SDLK_5;
    case Key::six: return RBX::SDLK_6;
    case Key::seven: return RBX::SDLK_7;
    case Key::eight: return RBX::SDLK_8;
    case Key::nine: return RBX::SDLK_9;
    case Key::semicolon: return RBX::SDLK_SEMICOLON;
    case Key::equals: return RBX::SDLK_EQUALS;
    case Key::leftBracket: return RBX::SDLK_LEFTBRACKET;
    case Key::backslash: return RBX::SDLK_BACKSLASH;
    case Key::rightBracket: return RBX::SDLK_RIGHTBRACKET;
    case Key::backquote: return RBX::SDLK_BACKQUOTE;
    case Key::a: return RBX::SDLK_a;
    case Key::b: return RBX::SDLK_b;
    case Key::c: return RBX::SDLK_c;
    case Key::d: return RBX::SDLK_d;
    case Key::e: return RBX::SDLK_e;
    case Key::f: return RBX::SDLK_f;
    case Key::g: return RBX::SDLK_g;
    case Key::h: return RBX::SDLK_h;
    case Key::i: return RBX::SDLK_i;
    case Key::j: return RBX::SDLK_j;
    case Key::k: return RBX::SDLK_k;
    case Key::l: return RBX::SDLK_l;
    case Key::m: return RBX::SDLK_m;
    case Key::n: return RBX::SDLK_n;
    case Key::o: return RBX::SDLK_o;
    case Key::p: return RBX::SDLK_p;
    case Key::q: return RBX::SDLK_q;
    case Key::r: return RBX::SDLK_r;
    case Key::s: return RBX::SDLK_s;
    case Key::t: return RBX::SDLK_t;
    case Key::u: return RBX::SDLK_u;
    case Key::v: return RBX::SDLK_v;
    case Key::w: return RBX::SDLK_w;
    case Key::x: return RBX::SDLK_x;
    case Key::y: return RBX::SDLK_y;
    case Key::z: return RBX::SDLK_z;
    case Key::left: return RBX::SDLK_LEFT;
    case Key::right: return RBX::SDLK_RIGHT;
    case Key::up: return RBX::SDLK_UP;
    case Key::down: return RBX::SDLK_DOWN;
    case Key::leftShift: return RBX::SDLK_LSHIFT;
    case Key::rightShift: return RBX::SDLK_RSHIFT;
    case Key::leftControl: return RBX::SDLK_LCTRL;
    case Key::rightControl: return RBX::SDLK_RCTRL;
    case Key::leftAlt: return RBX::SDLK_LALT;
    case Key::rightAlt: return RBX::SDLK_RALT;
    case Key::leftMeta: return RBX::SDLK_LMETA;
    case Key::rightMeta: return RBX::SDLK_RMETA;
    case Key::f1: return RBX::SDLK_F1;
    case Key::f2: return RBX::SDLK_F2;
    case Key::f3: return RBX::SDLK_F3;
    case Key::f4: return RBX::SDLK_F4;
    case Key::f5: return RBX::SDLK_F5;
    case Key::f6: return RBX::SDLK_F6;
    case Key::f7: return RBX::SDLK_F7;
    case Key::f8: return RBX::SDLK_F8;
    case Key::f9: return RBX::SDLK_F9;
    case Key::f10: return RBX::SDLK_F10;
    case Key::f11: return RBX::SDLK_F11;
    case Key::f12: return RBX::SDLK_F12;
    default: return RBX::SDLK_UNKNOWN;
    }
}

RBX::ModCode translateModifiers(std::uint32_t modifiers)
{
    unsigned int result = RBX::KMOD_NONE;
    result |= (modifiers & (1U << 0U)) ? RBX::KMOD_LSHIFT : 0U;
    result |= (modifiers & (1U << 1U)) ? RBX::KMOD_LCTRL : 0U;
    result |= (modifiers & (1U << 2U)) ? RBX::KMOD_LALT : 0U;
    result |= (modifiers & (1U << 3U)) ? RBX::KMOD_LMETA : 0U;
    result |= (modifiers & (1U << 4U)) ? RBX::KMOD_CAPS : 0U;
    return static_cast<RBX::ModCode>(result);
}

void collectPlaceSounds(RBX::Instance& instance,
    std::vector<boost::shared_ptr<RBX::Soundscape::SoundChannel>>& sounds)
{
    if (RBX::Soundscape::SoundChannel* sound =
            RBX::Instance::fastDynamicCast<RBX::Soundscape::SoundChannel>(&instance)) {
        const std::string id = sound->getSoundId().toString();
        if (id.find("9065112164") != std::string::npos ||
            id.find("115959318") != std::string::npos)
            sounds.push_back(RBX::shared_from<RBX::Soundscape::SoundChannel>(sound));
    }
    for (std::size_t index = 0; index < instance.numChildren(); ++index)
        collectPlaceSounds(*instance.getChild(index), sounds);
}

constexpr std::size_t kMaximumExternalUriLength = 2048;

bool asciiStartsWithIgnoringCase(std::string_view value, std::string_view prefix)
{
    if (value.size() < prefix.size())
        return false;
    for (std::size_t index = 0; index < prefix.size(); ++index) {
        const unsigned char character = static_cast<unsigned char>(value[index]);
        const unsigned char expected = static_cast<unsigned char>(prefix[index]);
        const unsigned char folded = character >= 'A' && character <= 'Z'
            ? static_cast<unsigned char>(character - 'A' + 'a') : character;
        if (folded != expected)
            return false;
    }
    return true;
}

bool isAllowedExternalUri(std::string_view uri)
{
    if (uri.empty() || uri.size() > kMaximumExternalUriLength)
        return false;

    const bool secure = asciiStartsWithIgnoringCase(uri, "https://");
    const bool plain = asciiStartsWithIgnoringCase(uri, "http://");
    if (!secure && !plain)
        return false;
    for (const unsigned char character : uri) {
        if (character <= 0x20 || character >= 0x7f)
            return false;
    }

    const std::size_t authorityStart = secure ? 8 : 7;
    const std::size_t authorityEnd = uri.find_first_of("/?#", authorityStart);
    const std::string_view authority = uri.substr(authorityStart,
        authorityEnd == std::string_view::npos
            ? std::string_view::npos : authorityEnd - authorityStart);
    return !authority.empty() && authority.front() != '.' &&
        authority.find('@') == std::string_view::npos;
}

class DesktopAppShellPlatform final : public RBX::IPlatformAPI
{
public:
    using ExternalUriRequest = std::function<bool(std::string)>;

    DesktopAppShellPlatform(RBX::DataModel* dataModel,
        ExternalUriRequest externalUriRequest)
        : dataModel(dataModel)
        , externalUriRequest(std::move(externalUriRequest))
    {
    }

    RBX::AccountAuthResult performAuthorization(
        RBX::InputObject::UserInputType, bool) override
    {
        return RBX::AccountAuth_Error;
    }

    int performAccountLink(const std::string&, const std::string&,
        std::string* response) override
    {
        if (response)
            response->clear();
        return -1;
    }

    int performUnlinkAccount(std::string* response) override
    {
        if (response)
            response->clear();
        return -1;
    }

    int performSetRobloxCredentials(const std::string&, const std::string&,
        std::string* response) override
    {
        if (response)
            response->clear();
        return -1;
    }

    RBX::AccountAuthResult performHasRobloxCredentials() override
    {
        return RBX::AccountAuth_Error;
    }

    RBX::AccountAuthResult performHasLinkedAccount() override
    {
        return RBX::AccountAuth_Error;
    }

    RBX::GameStartResult startGame3(RBX::GameJoinType, int) override
    {
        return RBX::GameStart_Weird;
    }

    void requestGameShutdown(bool) override
    {
    }
    int netConnectionCheck() override { return -1; }

    int fetchFriends(RBX::InputObject::UserInputType, std::string* result) override
    {
        if (result)
            result->clear();
        return -1;
    }

    int popupHelpUI() override
    {
        return queueExternalUri("https://en.help.roblox.com/");
    }
    int launchPlatformUri(const std::string baseUri) override
    {
        return queueExternalUri(baseUri);
    }
    int popupPartyUI(RBX::InputObject::UserInputType) override { return -1; }
    int popupProfileUI(RBX::InputObject::UserInputType, std::string) override
    {
        return -1;
    }
    int popupAccountPickerUI(RBX::InputObject::UserInputType) override
    {
        return -1;
    }
    void popupGameInviteUI() override
    {
        reportUnsupported("game invite UI");
    }
    void showKeyBoard(std::string&, std::string&, std::string&, unsigned,
        RBX::DataModel*) override
    {
        reportUnsupported("Xbox virtual keyboard");
    }
    void setScreenResolution(double, double) override
    {
        reportUnsupported("Xbox game-render overscan resolution");
    }

    int fetchCatalogInfo(
        boost::shared_ptr<RBX::Reflection::ValueArray> result) override
    {
        catalogRequestCount.fetch_add(1, std::memory_order_relaxed);
        if (result)
            result->clear();
        return 0;
    }
    int fetchInventoryInfo(
        boost::shared_ptr<RBX::Reflection::ValueArray> result) override
    {
        if (result)
            result->clear();
        return 0;
    }
    int getPlatformPartyMembers(
        boost::shared_ptr<RBX::Reflection::ValueArray> result) override
    {
        partyRequestCount.fetch_add(1, std::memory_order_relaxed);
        if (result)
            result->clear();
        return 0;
    }
    int getInGamePlayers(
        boost::shared_ptr<RBX::Reflection::ValueArray> result) override
    {
        if (result)
            result->clear();
        return 0;
    }
    RBX::PlatformPurchaseResult requestPurchase(const std::string&) override
    {
        return RBX::PurchaseResult_Error;
    }
    int getPMPCreatorId() override { return -1; }
    int getTitleId() override { return -1; }

    boost::shared_ptr<const RBX::Reflection::ValueTable>
    getVersionIdInfo() override
    {
        boost::shared_ptr<RBX::Reflection::ValueTable> result =
            boost::make_shared<RBX::Reflection::ValueTable>();
        (*result)["Major"] = rbx::core::BuildInfo::versionMajor;
        (*result)["Minor"] = rbx::core::BuildInfo::versionMinor;
        (*result)["Build"] = rbx::core::BuildInfo::versionPatch;
        (*result)["Revision"] = rbx::core::BuildInfo::versionRevision;
        (*result)["Product"] =
            std::string(rbx::core::BuildInfo::productName);
        (*result)["Architecture"] =
            std::string(rbx::core::BuildInfo::architecture);
        return result;
    }

    boost::shared_ptr<const RBX::Reflection::ValueTable>
    getPlatformUserInfo() override
    {
        boost::shared_ptr<RBX::Reflection::ValueTable> result =
            boost::make_shared<RBX::Reflection::ValueTable>();
        RBX::Network::Players* players = dataModel
            ? RBX::ServiceProvider::find<RBX::Network::Players>(dataModel)
            : nullptr;
        RBX::Network::Player* player = players
            ? players->getLocalPlayer() : nullptr;
        if (player) {
            const std::string displayName = player->getDisplayName().empty()
                ? player->getName()
                : player->getDisplayName();
            (*result)["Gamertag"] = displayName;
            (*result)["RobloxUserName"] = player->getName();
            (*result)["RobloxUserId"] = player->getUserID();
        }
        return result;
    }

    RBX::AwardResult awardAchievement(const std::string&) override
    {
        return RBX::Award_Fail;
    }
    RBX::AwardResult setHeroStat(const std::string&, double*) override
    {
        return RBX::Award_Fail;
    }
    void voiceChatSetMuteState(int, bool) override
    {
        reportUnsupported("Xbox voice-chat mute state");
    }
    unsigned voiceChatGetState(int) override
    {
        return RBX::voiceChatState_UnknownUser;
    }

    unsigned int getCatalogRequestCount() const
    {
        return catalogRequestCount.load(std::memory_order_relaxed);
    }

    unsigned int getPartyRequestCount() const
    {
        return partyRequestCount.load(std::memory_order_relaxed);
    }

private:
    int queueExternalUri(const std::string& uri)
    {
        if (!isAllowedExternalUri(uri) || !externalUriRequest)
            return -1;
        return externalUriRequest(uri) ? 0 : -1;
    }

    static void reportUnsupported(const char* capability)
    {
        std::cerr << "Desktop AppShell does not support " << capability << '\n';
    }

    RBX::DataModel* dataModel;
    ExternalUriRequest externalUriRequest;
    std::atomic<unsigned int> catalogRequestCount{0};
    std::atomic<unsigned int> partyRequestCount{0};
};

RBX::PartInstance* findPartByName(RBX::Instance* root, const char* name)
{
    if (!root)
        return nullptr;
    if (RBX::PartInstance* direct = RBX::Instance::fastDynamicCast<RBX::PartInstance>(
            root->findFirstChildByName(name)))
        return direct;
    boost::shared_ptr<const RBX::Instances> descendants = root->getDescendants();
    for (const boost::shared_ptr<RBX::Instance>& descendant : *descendants)
        if (descendant->getName() == name)
            if (RBX::PartInstance* part =
                    RBX::Instance::fastDynamicCast<RBX::PartInstance>(descendant.get()))
                return part;
    return nullptr;
}

} // namespace

struct PlayerRuntime::State final {
    RBX::Graphics::Device* device = nullptr;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int logicalWidth = 0;
    unsigned int logicalHeight = 0;
    CRenderSettingsItem* renderSettings = nullptr;
    std::unique_ptr<DesktopAppShellPlatform> launcherPlatform;
    boost::shared_ptr<RBX::DataModel> dataModel;
    boost::shared_ptr<RBX::DataModel> serverDataModel;
    RBX::Network::Client* localClient = nullptr;
    RBX::Network::Server* localServer = nullptr;
    std::unique_ptr<RBX::CommonVerbs> commonVerbs;
    std::unique_ptr<RBX::Graphics::VisualEngine> visualEngine;
    rbx::signals::scoped_connection scriptErrorConnection;
    rbx::signals::scoped_connection serverScriptErrorConnection;
    rbx::signals::scoped_connection cameraFrameConnection;
    rbx::signals::scoped_connection inputUpdatedConnection;
    rbx::signals::scoped_connection offlineChatMountConnection;
    rbx::signals::scoped_connection screenshotRequestConnection;
    rbx::signals::scoped_connection captureSavedConnection;
    rbx::signals::scoped_connection lightingChangedConnection;
    rbx::signals::scoped_connection openDocumentConnection;
    rbx::signals::scoped_connection r15AnimationPlayedConnection;
    rbx::signals::scoped_connection r15EmoteTriggeredConnection;
    rbx::signals::scoped_connection keyboardNavigationActivationConnection;
    RBX::Graphics::shared_ptr<RBX::Graphics::Texture> verificationColor;
    RBX::Graphics::shared_ptr<RBX::Graphics::Framebuffer> verificationFramebuffer;
    std::unordered_map<RBX::KeyCode, boost::shared_ptr<RBX::InputObject>> keyInputs;
    std::unordered_map<std::uint64_t, boost::shared_ptr<RBX::InputObject>> pointerInputs;
    RBX::Vector3 verificationStart;
    float maximumVerificationDisplacement = 0.0F;
    float minimumR6ShoulderAngle = 0.0F;
    float maximumR6ShoulderAngle = 0.0F;
    float minimumStoppedR6ShoulderAngle = 0.0F;
    float maximumStoppedR6ShoulderAngle = 0.0F;
    RBX::Vector3 cameraLookBeforeDrag;
    RBX::Vector3 cameraLookAfterDrag;
    float maximumCameraLookDeltaDuringDrag = 0.0F;
    bool sampledR6Shoulder = false;
    bool sampledStoppedR6Shoulder = false;
    bool sampledCameraBeforeDrag = false;
    bool sampledCameraAfterDrag = false;
    bool verifiesMovement = false;
    bool r15EmoteInvoked = false;
    bool r15EmoteCompleted = false;
    bool r15EmoteAccepted = false;
    bool r15EmoteTrackObserved = false;
    bool r15EmoteTriggeredObserved = false;
    bool r15EmoteTriggeredTrackObserved = false;
    unsigned int r15AnimationPlayedCount = 0;
    int r15AnimationPlayedPriority = -1;
    std::string r15AnimationPlayedName;
    std::string r15EmoteError;
    bool usesR15Character = false;
    bool verifiedChromeR15MeshGeometry = false;
    AvatarRigVariant avatarRig = AvatarRigVariant::R6;
    bool usesCurrentInExperienceUi = false;
    bool usesDurangoLauncher = false;
    bool verifiesDurangoLauncher = false;
    bool launcherPostProcessApplied = false;
    float launcherPostProcessBrightness = 0.0f;
    float launcherPostProcessContrast = 0.0f;
    float launcherPostProcessGrayscale = 0.0f;
    float launcherPostProcessBlur = 0.0f;
    RBX::Color3 launcherPostProcessTint = RBX::Color3::white();
    bool audioOutputDisabled = false;
    bool verifiesViewportRendering = false;
    bool verifiesVideoRendering = false;
    bool verifiesTextRendering = false;
    boost::shared_ptr<RBX::TextBox> verificationTextBox;
    bool verifiesPeoplePage = false;
    bool verifiesExperienceChat = false;
    bool verifiesCaptureGallery = false;
    bool verifiesChromeLeaderboard = false;
    bool verifiesChromeLeaderboardTouch = false;
    bool verifiesChromeLeaderboardController = false;
    bool verifiesKeyboardNavigation = false;
    bool verifiesSafeArea = false;
    bool verifiesOrientation = false;
    bool overridesInputPlatform = false;
    bool keyboardNavigationActivated = false;
    bool keyboardNavigationSelectionProved = false;
    int keyboardNavigationClickCount = 0;
    RBX::InputObject::UserInputType keyboardNavigationInputType =
        RBX::InputObject::TYPE_NONE;
    boost::shared_ptr<RBX::ScreenGui> keyboardNavigationScreen;
    boost::shared_ptr<RBX::GuiTextButton> keyboardNavigationFirstButton;
    boost::shared_ptr<RBX::GuiTextButton> keyboardNavigationSecondButton;
    boost::shared_ptr<RBX::ScreenGui> safeAreaScreen;
    boost::shared_ptr<RBX::Frame> safeAreaFrame;
    boost::shared_ptr<RBX::ScreenGui> fullViewportScreen;
    boost::shared_ptr<RBX::Frame> fullViewportFrame;
    boost::shared_ptr<RBX::ScreenGui> orientationScreen;
    boost::shared_ptr<RBX::Frame> orientationFrame;
    rbx::signals::scoped_connection orientationConnection;
    unsigned int orientationChangeCount = 0;
    bool verifiesReport = false;
    bool verifiesRespawn = false;
    bool verifiesSwitchAvatar = false;
    bool verifiesSurfaceTextures = false;
    bool verifiesShadowMap = false;
    bool verifiesSkybox = false;
    bool verifiesAudio = false;
    bool verifiesPlaceAudio = false;
    bool verifiesPlaceVisual = false;
    boost::shared_ptr<RBX::BasicPartInstance> verificationAudioEmitter;
    boost::shared_ptr<RBX::Soundscape::SoundChannel> verificationSound;
    boost::shared_ptr<RBX::Soundscape::AudioEmitter> verificationGraphEmitter;
    boost::shared_ptr<RBX::Soundscape::AudioFader> verificationAudioFader;
    boost::shared_ptr<RBX::Soundscape::AudioDistortion> verificationAudioDistortion;
    boost::shared_ptr<RBX::Soundscape::AudioTremolo> verificationAudioTremolo;
    boost::shared_ptr<RBX::Soundscape::AudioChorus> verificationAudioChorus;
    boost::shared_ptr<RBX::Soundscape::AudioFlanger> verificationAudioFlanger;
    boost::shared_ptr<RBX::Soundscape::AudioCompressor> verificationAudioCompressor;
    boost::shared_ptr<RBX::Soundscape::AudioGate> verificationAudioGate;
    boost::shared_ptr<RBX::Soundscape::AudioLimiter> verificationAudioLimiter;
    boost::shared_ptr<RBX::Soundscape::AudioEqualizer> verificationAudioEqualizer;
    boost::shared_ptr<RBX::Soundscape::AudioFilter> verificationAudioFilter;
    boost::shared_ptr<RBX::Soundscape::AudioPitchShifter> verificationAudioPitchShifter;
    boost::shared_ptr<RBX::Soundscape::AudioEcho> verificationAudioEcho;
    boost::shared_ptr<RBX::Soundscape::AudioReverb> verificationAudioReverb;
    boost::shared_ptr<RBX::Soundscape::AudioAnalyzer> verificationAudioAnalyzer;
    boost::shared_ptr<RBX::Soundscape::AudioChannelMixer> verificationAudioMixer;
    boost::shared_ptr<RBX::Soundscape::AudioChannelSplitter> verificationAudioSplitter;
    boost::shared_ptr<RBX::Soundscape::AudioPlayer> verificationAudioPlayer;
    boost::shared_ptr<RBX::Soundscape::AudioListener> verificationAudioListener;
    boost::shared_ptr<RBX::Soundscape::AudioDeviceOutput> verificationAudioOutput;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioFaderWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioDistortionWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioTremoloWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioChorusWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioFlangerWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioCompressorWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioGateWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioLimiterWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioEqualizerWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioFilterWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioPitchShifterWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioEchoWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioReverbWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioAnalyzerWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioMixerWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioSplitterWire;
    boost::shared_ptr<RBX::Soundscape::Wire> verificationAudioListenerWire;
    double audioPlayerMaximumPosition = 0.0;
    bool audioScheduledPlayCancelled = false;
    long long audioScheduledPlayAction = 0;
    double audioScheduledPlayTime = 0.0;
    rbx::signals::scoped_connection audioLoopConnection;
    unsigned long audioLoadedFrame = 0;
    double audioPreviousPosition = 0.0;
    std::uint64_t audioUnitSpeedFrames = 0;
    std::uint64_t audioHalfSpeedFrames = 0;
    std::uint64_t audioCurrentSpeedFrames = 0;
    unsigned int audioObservedLoops = 0;
    bool audioTestingHalfSpeed = false;
    double audioSquaredSampleSum = 0.0;
    std::size_t audioSampleCount = 0;
    std::vector<boost::shared_ptr<RBX::Instance>> placeAudioEmitters;
    std::vector<boost::shared_ptr<RBX::Soundscape::SoundChannel>> placeSounds;
    std::vector<double> placeSoundMaximumPositions;
    std::vector<bool> placeSoundObservedLoaded;
    unsigned long placeAudioLoadedFrame = 0;
    double placeAudioSquaredSampleSum = 0.0;
    std::size_t placeAudioSampleCount = 0;
    RBX::Graphics::TextureRef baseplateSurfaceTexture;
    RBX::Graphics::TextureRef spawnSurfaceTexture;
    RBX::Graphics::TextureRef wallpaperTexture;
    std::vector<std::uint8_t> shadowEnabledPixels;
    std::vector<std::uint8_t> shadowDisabledPixels;
    std::size_t shadowDarkenedPixels = 0;
    unsigned int shadowCasterBatches = 0;
    unsigned int shadowNoCastBatches = 0;
    unsigned int shadowMapWidth = 0;
    unsigned int shadowCascadeCount = 0;
    RBX::Vector4 shadowCascadeInfo;
    bool shadowLowQualityVerified = false;
    bool shadowMediumQualityVerified = false;
    bool reportFlowScriptError = false;
    const RBX::ModelInstance* initialRespawnCharacter = nullptr;
    bool screenshotRequested = false;
    bool captureSaved = false;
    bool captureVerified = false;
    bool captureThumbnailVisible = false;
    bool captureThumbnailPixelsVerified = false;
    std::filesystem::path verificationCapturePath;
    RBX::Vector2 verificationCaptureSize;
    RBX::Rect2D verificationCaptureThumbnailRect;
    long long verificationCaptureBytes = 0;
    bool offlineChatAccessPending = false;
    bool experienceChatMounted = false;
    unsigned long renderingFrame = 0;
    std::vector<RBX::Vector3> cameraChangesThisFrame;
    std::vector<RBX::Vector3> mouseChangesThisFrame;
    bool launcherFirstFrameCaptured = false;
    bool launcherSecondFrameCaptured = false;
    unsigned long launcherFirstFrameNumber = 0;
    unsigned long launcherSecondFrameNumber = 0;
    RBX::CoordinateFrame launcherFirstCameraFrame;
    RBX::CoordinateFrame launcherSecondCameraFrame;
    std::vector<std::uint8_t> launcherFirstFramePixels;
    std::vector<std::uint8_t> launcherSecondFramePixels;
    std::atomic<bool> openDocumentRequested{false};
    std::mutex recentDocumentMutex;
    std::optional<std::filesystem::path> recentDocumentRequested;
    std::mutex externalUriMutex;
    std::vector<std::string> externalUriRequests;

    ~State()
    {
        // Script jobs may still report errors while a constructor failure is
        // unwinding. Disconnect the callback that captures PlayerRuntime
        // before either DataModel begins asynchronous shutdown.
        scriptErrorConnection.disconnect();
        serverScriptErrorConnection.disconnect();
        offlineChatMountConnection.disconnect();
        lightingChangedConnection.disconnect();
        openDocumentConnection.disconnect();
        keyboardNavigationActivationConnection.disconnect();
        if (!verificationCapturePath.empty())
        {
            std::error_code error;
            std::filesystem::remove(verificationCapturePath, error);
        }
        // TextService shares the renderer-owned typesetters so scripts can
        // perform the same measurements as the draw path.  Release that
        // service-side ownership before VisualEngine destroys its texture
        // manager; otherwise a launcher that rendered bitmap shell text keeps
        // fonts.dds alive through device shutdown.
        if (dataModel)
        {
            RBX::DataModel::LegacyLock lock(
                dataModel.get(), RBX::DataModelJob::Write);
            if (RBX::TextService* textService =
                    RBX::ServiceProvider::find<RBX::TextService>(dataModel.get()))
                textService->clearTypesetters();
        }
        visualEngine.reset();
        commonVerbs.reset();
        if (localClient && dataModel)
        {
            RBX::DataModel::LegacyLock lock(dataModel.get(), RBX::DataModelJob::Write);
            localClient->disconnect(100);
            localClient = nullptr;
        }
        if (localServer && serverDataModel)
        {
            RBX::DataModel::LegacyLock lock(
                serverDataModel.get(), RBX::DataModelJob::Write);
            localServer->stop(100);
            localServer = nullptr;
        }
        if (dataModel)
            RBX::DataModel::closeDataModel(dataModel);
        if (serverDataModel)
            RBX::DataModel::closeDataModel(serverDataModel);
        if (overridesInputPlatform)
            RBX::UserInputService::clearPlatformOverride();
    }
};

PlayerRuntime::PlayerRuntime(RBX::Graphics::Device* device,
    const std::filesystem::path& resourceRoot,
    const std::filesystem::path& clientSettingsRoot,
    const std::filesystem::path& placePath, unsigned int renderWidth,
    unsigned int renderHeight, unsigned int logicalWidth,
    unsigned int logicalHeight, rbx::platform::DisplayOrientation orientation,
    float safeAreaLeft, float safeAreaTop,
    float safeAreaRight, float safeAreaBottom, bool disableAudioOutput,
    bool useCurrentInExperienceUi, bool useDurangoLauncher,
    bool verifyDurangoLauncher,
    AvatarRigVariant avatarRig,
    bool verifyViewportRendering,
    const std::filesystem::path& videoVerificationPath,
    bool verifyPeoplePage,
    bool verifyExperienceChat,
    bool verifyCaptureGallery,
    bool verifyChromeLeaderboard,
    bool verifyChromeLeaderboardTouch,
    bool verifyChromeLeaderboardController,
    bool verifyKeyboardNavigation,
    bool verifySafeArea,
    bool verifyOrientation,
    bool verifyReport,
    bool verifyRespawn,
    bool verifySwitchAvatar,
    bool verifySurfaceTextures,
    bool verifyShadowMap,
    bool verifySkybox,
    bool verifyAudio,
    bool verifyPlaceAudio,
    bool verifyTextRendering,
    bool verifyPlaceVisual,
    const std::vector<std::filesystem::path>& recentDocuments)
    : state(std::make_unique<State>())
{
    if (!device || renderWidth == 0 || renderHeight == 0 ||
        logicalWidth == 0 || logicalHeight == 0 ||
        !std::isfinite(safeAreaLeft) || safeAreaLeft < 0.0f ||
        !std::isfinite(safeAreaTop) || safeAreaTop < 0.0f ||
        !std::isfinite(safeAreaRight) || safeAreaRight < 0.0f ||
        !std::isfinite(safeAreaBottom) || safeAreaBottom < 0.0f)
        throw std::invalid_argument("PlayerRuntime requires a valid renderer and viewport");

    initializeRuntime(clientSettingsRoot);
    if (verifyChromeLeaderboardController) {
        RBX::UserInputService::setPlatformOverride(
            RBX::UserInputService::PLATFORM_XBOXONE);
        state->overridesInputPlatform = true;
    }
    if (useDurangoLauncher &&
        (!FLog::SetValue("Durango3DBackground", "true", FASTVARTYPE_STATIC) ||
         !FFlag::Durango3DBackground))
        throw std::runtime_error(
            "authentic Durango launcher requires its live ScaledWorld 3D background");
    const bool useR15Character = avatarRig != AvatarRigVariant::R6;
    const RBX::DataModel::AvatarRigVariant dataModelRig = [&]() {
        switch (avatarRig)
        {
        case AvatarRigVariant::R6:
            return RBX::DataModel::AVATAR_RIG_R6;
        case AvatarRigVariant::R15:
            return RBX::DataModel::AVATAR_RIG_R15;
        case AvatarRigVariant::R15Plus:
            return RBX::DataModel::AVATAR_RIG_R15_PLUS;
        case AvatarRigVariant::RthroNormal:
            return RBX::DataModel::AVATAR_RIG_RTHRO_NORMAL;
        case AvatarRigVariant::RthroSlender:
            return RBX::DataModel::AVATAR_RIG_RTHRO_SLENDER;
        }
        throw std::invalid_argument("unknown avatar rig variant");
    }();
    // The local authoritative server creates the replicated character during
    // join, before the client-side post-load setup below.  Select the requested
    // rig before either DataModel exists so backend Humanoid construction and
    // the later client reflection path observe one consistent contract.
    if (!FLog::SetValue("UseR15Character",
            useR15Character ? "true" : "false", FASTVARTYPE_DYNAMIC))
        throw std::runtime_error("UseR15Character dynamic flag is not registered");
    // The bundled legacy CoreGui was authored against the bitmap SourceSans
    // atlas.  The current UI overlay uses BuilderSans/BuilderIcons and needs
    // the UTF-8 FreeType path, so select the renderer that belongs to the UI
    // package instead of forcing the legacy menu through the newer path.
    FFlag::UseDynamicTypesetterUTF8 = useCurrentInExperienceUi;
    state->renderSettings = &CRenderSettingsItem::singleton();
    RBX::Soundscape::SoundService::soundDisabled = false;
    RBX::Soundscape::SoundService::outputDeviceDisabled = disableAudioOutput;
    const std::string contentPath = (resourceRoot / "content").string();
    RBX::ContentProvider::setAssetFolder(contentPath.c_str());
    if (useDurangoLauncher)
    {
        const std::filesystem::path launcherContent =
            resourceRoot / "launcher" / "content";
        if (!std::filesystem::is_regular_file(
                launcherContent / "scripts" / "XStarterScript.lua") ||
            !std::filesystem::is_regular_file(
                launcherContent / "ScaledWorldv4.7.rbxl") ||
            !std::filesystem::is_regular_file(
                launcherContent / "terrain" / "materials.json") ||
            !std::filesystem::is_regular_file(
                launcherContent / "sounds" / "ui" / "Shell" /
                "RobloxMusic.ogg"))
            throw std::runtime_error(
                "packaged authentic Durango launcher resources are incomplete");
        RBX::ContentProvider::addAssetOverlay(
            launcherContent.string().c_str(), "durango-launcher", 1000);
        RBX::ScriptContext::setAdminScriptPath(
            (launcherContent / "scripts").string());
    }
    const std::string avatarRuntimePath =
        (resourceRoot / "overlays" / "avatar-runtime" / "content").string();
    if (std::filesystem::is_directory(avatarRuntimePath))
        RBX::ContentProvider::addAssetOverlay(
            avatarRuntimePath.c_str(), "avatar-runtime", 120);
    const std::string studioRuntimePath =
        (resourceRoot / "overlays" / "studio-runtime" / "content").string();
    if (std::filesystem::is_directory(studioRuntimePath))
        RBX::ContentProvider::addAssetOverlay(
            studioRuntimePath.c_str(), "studio-runtime", 115);
    const std::filesystem::path materializedPackageRoot =
        placePath.parent_path().parent_path();
    const bool localSoloMode =
        placePath.parent_path().filename() == "place" &&
        std::filesystem::is_regular_file(
            materializedPackageRoot / "launch" / "local-solo");
    if (placePath.parent_path().filename() == "place" &&
        std::filesystem::is_directory(materializedPackageRoot / "assets") &&
        std::filesystem::is_regular_file(materializedPackageRoot / "manifest.json"))
        RBX::ContentProvider::addAssetOverlay(
            materializedPackageRoot.c_str(), "embedded-place", 90);
    if (useCurrentInExperienceUi) {
        const std::string playerCorePath =
            (resourceRoot / "overlays" / "player-2026" / "content").string();
        const std::string playerExtraPath =
            (resourceRoot / "overlays" / "player-2026" / "ExtraContent").string();
        RBX::ContentProvider::addAssetOverlay(playerCorePath.c_str(), "player-core", 100);
        RBX::ContentProvider::addAssetOverlay(playerExtraPath.c_str(), "player-extra", 110);
    }

    state->device = device;
    state->width = renderWidth;
    state->height = renderHeight;
    state->logicalWidth = logicalWidth;
    state->logicalHeight = logicalHeight;
    state->verifiesViewportRendering = verifyViewportRendering;
    state->verifiesVideoRendering = !videoVerificationPath.empty();
    state->verifiesTextRendering = verifyTextRendering;
    state->verifiesPeoplePage = verifyPeoplePage;
    state->verifiesExperienceChat = verifyExperienceChat;
    state->verifiesCaptureGallery = verifyCaptureGallery;
    state->verifiesChromeLeaderboard = verifyChromeLeaderboard;
    state->verifiesChromeLeaderboardTouch = verifyChromeLeaderboardTouch;
    state->verifiesChromeLeaderboardController =
        verifyChromeLeaderboardController;
    state->verifiesKeyboardNavigation = verifyKeyboardNavigation;
    state->verifiesSafeArea = verifySafeArea;
    state->verifiesOrientation = verifyOrientation;
    state->verifiesReport = verifyReport;
    state->verifiesRespawn = verifyRespawn;
    state->verifiesSwitchAvatar = verifySwitchAvatar;
    state->verifiesSurfaceTextures = verifySurfaceTextures;
    state->verifiesShadowMap = verifyShadowMap;
    state->verifiesSkybox = verifySkybox;
    state->verifiesAudio = verifyAudio;
    state->verifiesPlaceAudio = verifyPlaceAudio;
    state->verifiesPlaceVisual = verifyPlaceVisual;
    if (verifyShadowMap) {
        state->renderSettings->setEnableFRM(false);
        state->renderSettings->setQualityLevel(RBX::CRenderSettings::QualityLevel15);
        state->renderSettings->setEditQualityLevel(RBX::CRenderSettings::QualityLevel15);
    }
    state->usesR15Character = useR15Character;
    state->avatarRig = avatarRig;
    state->usesCurrentInExperienceUi = useCurrentInExperienceUi;
    state->usesDurangoLauncher = useDurangoLauncher;
    state->verifiesDurangoLauncher = verifyDurangoLauncher;
    state->audioOutputDisabled = disableAudioOutput;
    state->dataModel = RBX::DataModel::createDataModel(
        true, new RBX::NullVerb(nullptr, ""), false);
    state->dataModel->setIsAppShell(useDurangoLauncher);
    state->dataModel->setIsStudio(localSoloMode);

    // Local files run as an actual loopback server/client pair.  A single
    // DataModel's historical Play Solo role is useful in Studio, but it makes
    // server-only CoreScripts observe no NetworkServer and lets client and
    // server state share authority.  Keeping the authoritative place in a
    // server DataModel also exercises the same Replicator and transport path
    // used by a future remote game server.
    if (!placePath.empty() && !useDurangoLauncher)
    {
        state->serverDataModel = RBX::DataModel::createDataModel(
            true, new RBX::NullVerb(nullptr, ""), false);
        state->serverDataModel->setIsStudio(localSoloMode);
        {
            RBX::Security::Impersonator permission(RBX::Security::COM);
            RBX::DataModel::LegacyLock serverLock(
                state->serverDataModel.get(), RBX::DataModelJob::Write);
            RBX::ServiceProvider::create<RBX::ContentProvider>(
                state->serverDataModel.get())->setBaseUrl("https://www.roblox.com/");
            RBX::ServiceProvider::create<RBX::ProximityPromptService>(
                state->serverDataModel.get());
            state->localServer =
                RBX::ServiceProvider::create<RBX::Network::Server>(
                    state->serverDataModel.get());
            RBX::ServiceProvider::create<RBX::StarterPlayerService>(
                state->serverDataModel.get());
            RBX::ScriptContext* serverScriptContext =
                RBX::ServiceProvider::create<RBX::ScriptContext>(
                    state->serverDataModel.get());
            state->serverScriptErrorConnection =
                serverScriptContext->errorSignal.connect(
                    [](std::string message, std::string stack,
                       boost::shared_ptr<RBX::Instance> script) {
                        std::cerr << "server script error "
                                  << (script ? script->getFullName() : "<unknown>")
                                  << ": " << message;
                        if (!stack.empty())
                            std::cerr << '\n' << stack;
                        std::cerr << '\n';
                    });
            state->serverDataModel->setAvatarRigVariant(dataModelRig);

            std::ifstream input(placePath, std::ios::binary);
            if (!input)
                throw std::runtime_error("could not open the requested Roblox place");
            Serializer serializer;
            serializer.load(input, state->serverDataModel.get());
            state->serverDataModel->processAfterLoad();
            state->serverDataModel->workspaceLoadedSignal();
            if (verifyPlaceAudio) {
                std::vector<boost::shared_ptr<RBX::Soundscape::SoundChannel>>
                    authoredSounds;
                collectPlaceSounds(*state->serverDataModel, authoredSounds);
                std::cout << "selected-place server authored sounds="
                          << authoredSounds.size() << '\n';
                if (authoredSounds.size() != 3)
                    throw std::runtime_error(
                        "selected place serializer did not preserve its authored sounds");
                for (const auto& sound : authoredSounds) {
                    if (!sound->getParent())
                        throw std::runtime_error(
                            "selected place authored Sound has no emitter parent");
                    state->placeAudioEmitters.push_back(
                        sound->getParent()->clone(RBX::SerializationCreator));
                }
            }

            state->localServer->start(0, 0);
            // Execute the package bootstrap before RunService releases authored
            // server Scripts. This gives an offline replacement for a live
            // elevator handoff the same chance to observe CharacterAdded that
            // the upstream matchmaking server has in production.
            const std::filesystem::path localSoloBootstrap =
                materializedPackageRoot / "launch" / "local-solo.lua";
            if (localSoloMode && std::filesystem::is_regular_file(localSoloBootstrap)) {
                std::ifstream bootstrapInput(localSoloBootstrap, std::ios::binary);
                if (!bootstrapInput)
                    throw std::runtime_error(
                        "could not read local-solo server bootstrap");
                std::string bootstrapSource(
                    (std::istreambuf_iterator<char>(bootstrapInput)),
                    std::istreambuf_iterator<char>());
                if (bootstrapSource.empty() || bootstrapSource.size() > 1024U * 1024U)
                    throw std::runtime_error(
                        "local-solo server bootstrap is empty, unreadable, or too large");
                serverScriptContext->executeInNewThread(
                    RBX::Security::GameScript_,
                    RBX::ProtectedString::fromTrustedSource(bootstrapSource),
                    "RBXLPLocalSoloBootstrap");
            }
            RBX::ServiceProvider::create<RBX::RunService>(
                state->serverDataModel.get())->run();
        }

        std::atomic<bool> accepted{false};
        std::atomic<bool> receivedGlobals{false};
        std::atomic<bool> gameLoaded{false};
        std::atomic<bool> characterReplicated{false};
        const bool requiresReplicatedCharacter = !useDurangoLauncher;
        std::atomic<bool> failed{false};
        std::string failureReason;
        rbx::signals::scoped_connection acceptedConnection;
        rbx::signals::scoped_connection globalsConnection;
        rbx::signals::scoped_connection gameLoadedConnection;
        rbx::signals::scoped_connection clientDisconnectConnection;
        rbx::signals::scoped_connection failedConnection;
        rbx::signals::scoped_connection rejectedConnection;
        rbx::signals::scoped_connection serverIncomingConnection;
        rbx::signals::scoped_connection serverDisconnectConnection;
        serverIncomingConnection = state->localServer->incommingConnectionSignal.connect(
            [&](std::string, boost::shared_ptr<RBX::Instance> instance) {
                boost::shared_ptr<RBX::Network::ServerReplicator> replicator =
                    RBX::Instance::fastSharedDynamicCast<
                        RBX::Network::ServerReplicator>(instance);
                if (replicator)
                    serverDisconnectConnection = replicator->disconnectionSignal.connect(
                        [&, replicator](std::string peer, bool) {
                            failureReason = "server disconnected local client " + peer +
                                " (reason " + std::to_string(
                                    replicator->getLastDisconnectReason()) + ")";
                            if (!replicator->getLastPacketError().empty())
                                failureReason += ": " + replicator->getLastPacketError();
                            failed = true;
                        });
            });
        {
            RBX::Security::Impersonator permission(RBX::Security::COM);
            RBX::DataModel::LegacyLock clientLock(
                state->dataModel.get(), RBX::DataModelJob::Write);
            RBX::ServiceProvider::create<RBX::ContentProvider>(state->dataModel.get())
                ->setBaseUrl("https://www.roblox.com/");
            RBX::ServiceProvider::create<RBX::ProximityPromptService>(
                state->dataModel.get());
            state->dataModel->setAvatarRigVariant(dataModelRig);
            // The current PlayerModule can be copied into the local Player as
            // soon as Client::playerConnect creates it.  Current camera code
            // immediately reads Workspace.CurrentCamera during module
            // initialization, so establish the normal client camera before
            // beginning replication rather than racing that initialization
            // against the post-join renderer bootstrap.  Large legacy places
            // make the race especially visible because PlayerScripts can
            // finish loading while the initial DataModel is still arriving.
            RBX::Workspace* clientWorkspace = state->dataModel->getWorkspace();
            clientWorkspace->replenishCamera();
            clientWorkspace->getCamera()->setViewport(RBX::Vector2int16(
                static_cast<std::int16_t>(logicalWidth),
                static_cast<std::int16_t>(logicalHeight)));
            // Lua GetService resolves the services already owned by this
            // historical DataModel.  The exact current PlayerModule imports
            // these three during module initialization, before the broader UI
            // bootstrap creates them, so make them part of the pre-script
            // client contract as they are in the current Player.
            RBX::ServiceProvider::create<RBX::VRService>(state->dataModel.get());
            RBX::ServiceProvider::create<RBX::TweenService>(state->dataModel.get());
            RBX::ServiceProvider::create<RBX::PathfindingService>(
                state->dataModel.get());
            // Observe bootstrap failures before Client::playerConnect creates
            // the local Player. PlayerScripts may start during the join, well
            // before the renderer-side setup below is reached.
            RBX::ScriptContext* joinScriptContext =
                RBX::ServiceProvider::create<RBX::ScriptContext>(
                    state->dataModel.get());
            // Initial replication may parent StarterGui/PlayerScripts before
            // the Player.Character reference packet arrives. Current games
            // commonly read LocalPlayer.Character immediately in those
            // LocalScripts, matching the official join contract where normal
            // game scripts are released only after initial player state is
            // ready. CoreScripts remain runnable while ScriptsDisabled is set.
            RBX::ScriptContext::propScriptsDisabled.setValue(
                joinScriptContext, true);
            State* runtimeState = state.get();
            state->scriptErrorConnection = joinScriptContext->errorSignal.connect(
                [runtimeState](std::string message, std::string stack,
                   boost::shared_ptr<RBX::Instance> script) {
                    if (runtimeState->verifiesReport &&
                        (message.find("AbuseReport") != std::string::npos ||
                         message.find("OverlayNativeInput") != std::string::npos ||
                         message.find("CoreVoiceManager") != std::string::npos))
                        runtimeState->reportFlowScriptError = true;
                    std::cerr << "script error "
                              << (script ? script->getFullName() : "<unknown>")
                              << ": " << message;
                    if (!stack.empty())
                        std::cerr << '\n' << stack;
                    std::cerr << '\n';
                });
            state->localClient = RBX::ServiceProvider::create<RBX::Network::Client>(
                state->dataModel.get());
            acceptedConnection = state->localClient->connectionAcceptedSignal.connect(
                [&](std::string, boost::shared_ptr<RBX::Instance> instance) {
                    accepted = true;
                    boost::shared_ptr<RBX::Network::ClientReplicator> replicator =
                        RBX::Instance::fastSharedDynamicCast<
                            RBX::Network::ClientReplicator>(instance);
                    if (!replicator)
                    {
                        failureReason = "connection returned no ClientReplicator";
                        failed = true;
                        return;
                    }
                    globalsConnection = replicator->receivedGlobalsSignal.connect(
                        [&] { receivedGlobals = true; });
                    gameLoadedConnection = replicator->gameLoadedSignal.connect(
                        [&] { gameLoaded = true; });
                    clientDisconnectConnection = replicator->disconnectionSignal.connect(
                        [&, replicator](std::string peer, bool) {
                            failureReason = "client disconnected from local server " + peer +
                                " (reason " + std::to_string(
                                    replicator->getLastDisconnectReason()) + ")";
                            if (!replicator->getLastPacketError().empty())
                                failureReason += ": " + replicator->getLastPacketError();
                            failed = true;
                        });
                });
            failedConnection = state->localClient->connectionFailedSignal.connect(
                [&](std::string, int, std::string reason) {
                    failureReason = std::move(reason);
                    failed = true;
                });
            rejectedConnection = state->localClient->connectionRejectedSignal.connect(
                [&](std::string) {
                    failureReason = "connection rejected";
                    failed = true;
                });
            state->localClient->playerConnect(
                1, "127.0.0.1", state->localServer->getPort(), 0, 0);
        }

        const auto connectionDeadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(60);
        while ((!receivedGlobals || !gameLoaded ||
                (requiresReplicatedCharacter && !characterReplicated)) && !failed &&
               std::chrono::steady_clock::now() < connectionDeadline)
        {
            {
                RBX::DataModel::LegacyLock clientLock(
                    state->dataModel.get(), RBX::DataModelJob::Write);
                if (RBX::Network::ClientReplicator* replicator =
                        state->localClient->findFirstChildOfType<
                            RBX::Network::ClientReplicator>())
                {
                    // The native host is not yet running its client RunService
                    // during join. Pump already-received replication packets
                    // under the same DataModel write contract used by the
                    // ProcessPackets job so initial globals cannot be starved
                    // behind application bootstrap.
                    while (replicator->processNextIncomingPacket())
                    {
                    }
                    receivedGlobals = replicator->hasReceivedGlobals();
                    if (!replicator->getLastPacketError().empty())
                    {
                        failureReason = replicator->getLastPacketError();
                        failed = true;
                    }
                }
                if (RBX::Network::Players* players =
                        RBX::ServiceProvider::find<RBX::Network::Players>(
                            state->dataModel.get()))
                {
                    if (RBX::Network::Player* player = players->getLocalPlayer())
                        characterReplicated = player->getCharacter() != nullptr;
                }
            }
            {
                RBX::DataModel::LegacyLock serverLock(
                    state->serverDataModel.get(), RBX::DataModelJob::Read);
                if (RBX::Network::ServerReplicator* replicator =
                        state->localServer->findFirstChildOfType<
                            RBX::Network::ServerReplicator>())
                {
                    if (!replicator->getLastPacketError().empty())
                    {
                        failureReason = replicator->getLastPacketError();
                        failed = true;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (failed)
            throw std::runtime_error(
                "local DataModel connection failed: " + failureReason);
        if (!accepted || !receivedGlobals || !gameLoaded ||
            (requiresReplicatedCharacter && !characterReplicated))
            throw std::runtime_error(
                "timed out receiving the local authoritative DataModel and character");

        // The loaded marker can precede dynamically cloned StarterGui and
        // StarterPlayer descendants on a large join. Starting LocalScripts at
        // that boundary lets a parent execute before its ModuleScript children
        // arrive. Keep game scripts held until the authoritative server has no
        // new instances queued and the client has consumed the resulting
        // packets for a short quiescent interval.
        const auto replicationDrainDeadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(60);
        auto quietSince = std::chrono::steady_clock::now();
        bool initialInstancesDrained = false;
        while (!failed && std::chrono::steady_clock::now() < replicationDrainDeadline)
        {
            bool processedPacket = false;
            {
                RBX::DataModel::LegacyLock clientLock(
                    state->dataModel.get(), RBX::DataModelJob::Write);
                if (RBX::Network::ClientReplicator* replicator =
                        state->localClient->findFirstChildOfType<
                            RBX::Network::ClientReplicator>())
                {
                    while (replicator->processNextIncomingPacket())
                        processedPacket = true;
                    if (!replicator->getLastPacketError().empty())
                    {
                        failureReason = replicator->getLastPacketError();
                        failed = true;
                    }
                }
            }
            bool serverHasPendingInstances = true;
            {
                RBX::DataModel::LegacyLock serverLock(
                    state->serverDataModel.get(), RBX::DataModelJob::Read);
                if (RBX::Network::ServerReplicator* replicator =
                        state->localServer->findFirstChildOfType<
                            RBX::Network::ServerReplicator>())
                    serverHasPendingInstances = replicator->hasPendingNewInstances();
            }
            if (processedPacket || serverHasPendingInstances)
                quietSince = std::chrono::steady_clock::now();
            else if (std::chrono::steady_clock::now() - quietSince >=
                     std::chrono::milliseconds(250))
            {
                initialInstancesDrained = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (failed)
            throw std::runtime_error(
                "local DataModel connection failed while draining instances: " +
                failureReason);
        if (!initialInstancesDrained)
            throw std::runtime_error(
                "timed out draining initial replicated instances before starting scripts");

        {
            RBX::DataModel::LegacyLock clientLock(
                state->dataModel.get(), RBX::DataModelJob::Write);
            RBX::ScriptContext* joinedScriptContext =
                RBX::ServiceProvider::find<RBX::ScriptContext>(
                    state->dataModel.get());
            if (!joinedScriptContext)
                throw std::runtime_error(
                    "local join lost its ScriptContext before releasing game scripts");
            RBX::ScriptContext::propScriptsDisabled.setValue(
                joinedScriptContext, false);
            const std::filesystem::path clientBootstrap =
                materializedPackageRoot / "launch" / "client-local-solo.lua";
            if (localSoloMode && std::filesystem::is_regular_file(clientBootstrap)) {
                std::ifstream bootstrapInput(clientBootstrap, std::ios::binary);
                if (!bootstrapInput)
                    throw std::runtime_error(
                        "could not read local-solo client bootstrap");
                std::string bootstrapSource(
                    (std::istreambuf_iterator<char>(bootstrapInput)),
                    std::istreambuf_iterator<char>());
                if (bootstrapSource.empty() || bootstrapSource.size() > 1024U * 1024U)
                    throw std::runtime_error(
                        "local-solo client bootstrap is empty, unreadable, or too large");
                joinedScriptContext->executeInNewThread(
                    RBX::Security::GameScript_,
                    RBX::ProtectedString::fromTrustedSource(bootstrapSource),
                    "RBXLPLocalSoloClientBootstrap");
            }
        }
    }

    {
        RBX::Security::Impersonator permission(RBX::Security::COM);
        RBX::DataModel::LegacyLock lock(
            state->dataModel.get(), RBX::DataModelJob::Write);
        RBX::Workspace* workspace = state->dataModel->getWorkspace();
        // Current CorePackages derive their endpoint set from this host-owned
        // value during module initialization.  Offline play still uses the
        // production domain grammar even though requests remain subject to the
        // normal client HTTP policy.
        RBX::ServiceProvider::create<RBX::ContentProvider>(state->dataModel.get())
            ->setBaseUrl("https://www.roblox.com/");
        state->commonVerbs = std::make_unique<RBX::CommonVerbs>(state->dataModel.get());
        RBX::ScriptContext* scriptContext =
            RBX::ServiceProvider::create<RBX::ScriptContext>(state->dataModel.get());
        if (useDurangoLauncher) {
            state->scriptErrorConnection = scriptContext->errorSignal.connect(
                [](std::string message, std::string stack,
                   boost::shared_ptr<RBX::Instance> script) {
                    std::cerr << "launcher script error "
                              << (script ? script->getFullName() : "<unknown>")
                              << ": " << message;
                    if (!stack.empty())
                        std::cerr << '\n' << stack;
                    std::cerr << '\n';
                });
        }
        // PlayerRuntime drives VisualEngine without constructing RenderView.
        // Complete the shared DataModel screenshot contract by capturing the
        // next rendered framebuffer whenever any engine or CoreScript client
        // raises the normal screenshot signal.
        state->screenshotRequestConnection =
            state->dataModel->screenshotSignal.connect([this]() {
                state->screenshotRequested = true;
            });
        if (verifyCaptureGallery) {
            RBX::CaptureService* captureService =
                RBX::ServiceProvider::create<RBX::CaptureService>(state->dataModel.get());
            state->captureSavedConnection =
                captureService->captureObjectSavedInternalSignal.connect(
                    [this](boost::shared_ptr<RBX::Reflection::DescribedBase> value,
                           std::string triggerSource) {
                        boost::shared_ptr<RBX::Capture> capture =
                            RBX::Reflection::DescribedBase::fastSharedDynamicCast<
                                RBX::Capture>(value);
                        if (!capture || triggerSource != "UserSave")
                            throw std::runtime_error(
                                "CaptureService emitted an invalid saved capture");
                        state->verificationCapturePath = capture->getFilePath();
                        state->captureSaved = true;
                    });
        }
        RBX::UserInputService* inputService =
            RBX::ServiceProvider::create<RBX::UserInputService>(state->dataModel.get());
        // Link and materialize the current prompt implementation before place
        // deserialization so ProximityPrompt children and the serialized
        // ProximityPromptService are resolved by their real descriptors.
        RBX::ServiceProvider::create<RBX::ProximityPromptService>(state->dataModel.get());
        inputService->setKeyboardEnabled(true);
        inputService->setMouseEnabled(true);
        const unsigned int horizontalScale =
            (renderWidth + logicalWidth / 2U) / logicalWidth;
        const unsigned int verticalScale =
            (renderHeight + logicalHeight / 2U) / logicalHeight;
        RBX::GuiService* guiService =
            RBX::ServiceProvider::create<RBX::GuiService>(state->dataModel.get());
        guiService->setResolutionScale(static_cast<int>(std::clamp(
            std::max(horizontalScale, verticalScale), 1U, 3U)));
        guiService->setHardwareSafeAreaInsets(
            safeAreaLeft, safeAreaTop, safeAreaRight, safeAreaBottom);
        if (placePath.empty() && !useDurangoLauncher) {
            createPart(workspace, "Baseplate", RBX::Vector3(96.0f, 2.0f, 96.0f),
                RBX::Vector3(0.0f, -1.0f, 0.0f), RBX::BrickColor::brickGreen());
            createPart(workspace, "RedBlock", RBX::Vector3(8.0f, 8.0f, 8.0f),
                RBX::Vector3(-10.0f, 4.0f, 0.0f), RBX::BrickColor::brickRed());
            createPart(workspace, "BlueBlock", RBX::Vector3(6.0f, 14.0f, 6.0f),
                RBX::Vector3(10.0f, 7.0f, -5.0f), RBX::BrickColor::brickBlue());
        }

        // These switches select the avatar created immediately below. The
        // Player join order loads the place before it creates the character.
        if (!FLog::SetValue("UseR15Character",
                useR15Character ? "true" : "false", FASTVARTYPE_DYNAMIC))
            throw std::runtime_error("UseR15Character dynamic flag is not registered");
        state->dataModel->setAvatarRigVariant(dataModelRig);

        // Workspace::getCamera() falls back to a private utility camera when a
        // place has no Camera instance.  Rendering through that fallback works,
        // but Workspace.CurrentCamera remains nil, which breaks current camera
        // scripts and SettingsHub's ViewportSize observer.  Materialize the
        // normal Workspace child before any CoreScript observes the service.
        workspace->replenishCamera();

        if (useDurangoLauncher)
        {
            RBX::PlatformService* platformService =
                RBX::ServiceProvider::create<RBX::PlatformService>(
                    state->dataModel.get());
            State* runtimeState = state.get();
            state->launcherPlatform = std::make_unique<DesktopAppShellPlatform>(
                state->dataModel.get(),
                [runtimeState](std::string uri) {
                    std::scoped_lock lock(runtimeState->externalUriMutex);
                    constexpr std::size_t maximumPendingUriRequests = 8;
                    if (runtimeState->externalUriRequests.size() >=
                            maximumPendingUriRequests)
                        return false;
                    runtimeState->externalUriRequests.push_back(std::move(uri));
                    return true;
                });
            platformService->setPlatform(
                state->launcherPlatform.get(), RBX::AppShellDatamodel);
            RBX::CoreGuiService* coreGui =
                RBX::ServiceProvider::create<RBX::CoreGuiService>(
                    state->dataModel.get());
            boost::shared_ptr<RBX::BindableEvent> openDocument =
                RBX::Creatable<RBX::Instance>::create<RBX::BindableEvent>();
            openDocument->setName("OpenLocalDocument");
            openDocument->setParent(coreGui);
            boost::shared_ptr<RBX::Folder> recentFolder =
                RBX::Creatable<RBX::Instance>::create<RBX::Folder>();
            recentFolder->setName("LocalRecentDocuments");
            recentFolder->setParent(coreGui);
            for (std::size_t index = 0; index < recentDocuments.size(); ++index) {
                boost::shared_ptr<RBX::StringValue> recent =
                    RBX::Creatable<RBX::Instance>::create<RBX::StringValue>();
                recent->setName("RecentDocument" + std::to_string(index + 1));
                recent->setValue(
                    rbx::platform::pathToUtf8(recentDocuments[index]));
                recent->setParent(recentFolder.get());
            }
            state->openDocumentConnection = openDocument->event.connect(
                [runtimeState](boost::shared_ptr<const RBX::Reflection::Tuple> arguments) {
                    if (arguments && arguments->values.size() == 1 &&
                        arguments->values.front().isType<std::string>()) {
                        std::scoped_lock lock(runtimeState->recentDocumentMutex);
                        runtimeState->recentDocumentRequested =
                            rbx::platform::pathFromUtf8(
                                arguments->values.front().cast<std::string>());
                        return;
                    }
                    runtimeState->openDocumentRequested.store(
                        true, std::memory_order_release);
                });

            state->dataModel->startCoreScripts(true, "XStarterScript");
            state->dataModel->loadContent(
                RBX::ContentId::fromAssets("ScaledWorldv4.7.rbxl"));
            state->dataModel->setAvatarRigVariant(dataModelRig);
        }

        RBX::Network::Players* players =
            RBX::ServiceProvider::create<RBX::Network::Players>(state->dataModel.get());
        boost::shared_ptr<RBX::Network::Player> localPlayer;
        if (placePath.empty() || useDurangoLauncher)
        {
            localPlayer = RBX::Instance::fastSharedDynamicCast<RBX::Network::Player>(
                players->createLocalPlayer(1, false));
        }
        else if (RBX::Network::Player* connectedPlayer = players->getLocalPlayer())
        {
            localPlayer = RBX::shared_from(connectedPlayer);
        }
        if (!localPlayer)
            throw std::runtime_error("offline Player bootstrap did not create a local player");
        RBX::PlayerGui* playerGui =
            localPlayer->findFirstChildOfType<RBX::PlayerGui>();
        if (!playerGui)
            throw std::runtime_error("offline Player bootstrap did not create PlayerGui");
        playerGui->setCurrentScreenOrientation(screenOrientation(orientation));
        if (verifyOrientation) {
            State* runtimeState = state.get();
            state->orientationConnection = playerGui->propertyChangedSignal.connect(
                [runtimeState](const RBX::Reflection::PropertyDescriptor* property) {
                    if (property && property->name == "CurrentScreenOrientation")
                        ++runtimeState->orientationChangeCount;
                });
            state->orientationScreen =
                RBX::Creatable<RBX::Instance>::create<RBX::ScreenGui>();
            state->orientationScreen->setName("OrientationVerification");
            state->orientationScreen->setScreenInsets(RBX::SCREEN_INSETS_NONE);
            state->orientationScreen->setClipToDeviceSafeArea(false);
            state->orientationScreen->setParent(
                RBX::ServiceProvider::create<RBX::CoreGuiService>(
                    state->dataModel.get()));
            state->orientationFrame =
                RBX::Creatable<RBX::Instance>::create<RBX::Frame>();
            state->orientationFrame->setName("ViewportBounds");
            state->orientationFrame->setSize(RBX::UDim2(1.0f, 0, 1.0f, 0));
            state->orientationFrame->setParent(state->orientationScreen.get());
        }
        localPlayer->setCanLoadCharacterAppearance(false);
        if (placePath.empty() || useDurangoLauncher)
            localPlayer->loadCharacter(true, "");
        if (!localPlayer->getCharacter())
            throw std::runtime_error("local server did not replicate a player character");
        if (verifyKeyboardNavigation) {
            RBX::PlayerGui* playerGui =
                localPlayer->findFirstChildOfType<RBX::PlayerGui>();
            if (!playerGui)
                throw std::runtime_error(
                    "keyboard navigation verification requires PlayerGui");
            state->keyboardNavigationScreen =
                RBX::Creatable<RBX::Instance>::create<RBX::ScreenGui>();
            state->keyboardNavigationScreen->setName(
                "KeyboardNavigationVerification");
            state->keyboardNavigationScreen->setParent(playerGui);
            state->keyboardNavigationFirstButton =
                RBX::Creatable<RBX::Instance>::create<RBX::GuiTextButton>();
            state->keyboardNavigationFirstButton->setName("FirstAction");
            state->keyboardNavigationFirstButton->setText("First action");
            state->keyboardNavigationFirstButton->setPosition(
                RBX::UDim2(0.0f, 80, 0.0f, 120));
            state->keyboardNavigationFirstButton->setSize(
                RBX::UDim2(0.0f, 240, 0.0f, 56));
            state->keyboardNavigationFirstButton->setParent(
                state->keyboardNavigationScreen.get());
            state->keyboardNavigationSecondButton =
                RBX::Creatable<RBX::Instance>::create<RBX::GuiTextButton>();
            state->keyboardNavigationSecondButton->setName("SecondAction");
            state->keyboardNavigationSecondButton->setText("Second action");
            state->keyboardNavigationSecondButton->setPosition(
                RBX::UDim2(0.0f, 80, 0.0f, 200));
            state->keyboardNavigationSecondButton->setSize(
                RBX::UDim2(0.0f, 240, 0.0f, 56));
            state->keyboardNavigationSecondButton->setParent(
                state->keyboardNavigationScreen.get());
            state->keyboardNavigationFirstButton->setNextSelectionDown(
                state->keyboardNavigationSecondButton.get());
            state->keyboardNavigationSecondButton->setNextSelectionUp(
                state->keyboardNavigationFirstButton.get());
            State* runtimeState = state.get();
            state->keyboardNavigationActivationConnection =
                state->keyboardNavigationSecondButton->activatedSignal.connect(
                    [runtimeState](boost::shared_ptr<RBX::InputObject> input,
                                   int clickCount) {
                        runtimeState->keyboardNavigationActivated = true;
                        runtimeState->keyboardNavigationClickCount = clickCount;
                        runtimeState->keyboardNavigationInputType = input
                            ? input->getUserInputType()
                            : RBX::InputObject::TYPE_NONE;
                    });
        }
        if (verifySafeArea) {
            RBX::PlayerGui* playerGui =
                localPlayer->findFirstChildOfType<RBX::PlayerGui>();
            if (!playerGui)
                throw std::runtime_error(
                    "safe-area verification requires PlayerGui");
            state->safeAreaScreen =
                RBX::Creatable<RBX::Instance>::create<RBX::ScreenGui>();
            state->safeAreaScreen->setName("SafeAreaVerification");
            state->safeAreaScreen->setScreenInsets(RBX::SCREEN_INSETS_DEVICE_SAFE);
            state->safeAreaScreen->setParent(playerGui);
            state->safeAreaFrame =
                RBX::Creatable<RBX::Instance>::create<RBX::Frame>();
            state->safeAreaFrame->setName("SafeAreaBounds");
            state->safeAreaFrame->setSize(RBX::UDim2(1.0f, 0, 1.0f, 0));
            state->safeAreaFrame->setParent(state->safeAreaScreen.get());
            state->fullViewportScreen =
                RBX::Creatable<RBX::Instance>::create<RBX::ScreenGui>();
            state->fullViewportScreen->setName("FullViewportVerification");
            state->fullViewportScreen->setScreenInsets(RBX::SCREEN_INSETS_NONE);
            state->fullViewportScreen->setClipToDeviceSafeArea(false);
            state->fullViewportScreen->setParent(playerGui);
            state->fullViewportFrame =
                RBX::Creatable<RBX::Instance>::create<RBX::Frame>();
            state->fullViewportFrame->setName("FullViewportBounds");
            state->fullViewportFrame->setSize(RBX::UDim2(1.0f, 0, 1.0f, 0));
            state->fullViewportFrame->setParent(state->fullViewportScreen.get());
        }
        if (useDurangoLauncher) {
            RBX::ServiceProvider::create<RBX::RunService>(
                state->dataModel.get())->run();
            state->dataModel->gameLoaded();
        }
        if (verifyPlaceVisual) {
            RBX::ModelInstance* character = localPlayer->getCharacter();
            RBX::PartInstance* root = character->getPrimaryPartSetByUser();
            if (!root)
                root = character->findFirstChildByName("HumanoidRootPart")
                    ? RBX::Instance::fastDynamicCast<RBX::PartInstance>(
                        character->findFirstChildByName("HumanoidRootPart"))
                    : nullptr;
            if (!root)
                throw std::runtime_error(
                    "selected-place visual proof requires a character root");
            // The authored spawn is an intentionally unlit edge corridor.
            // Place the proof camera in the nearest authored fluorescent bay
            // so the acceptance image exercises wallpaper, carpet, local
            // PointLights, Neon fixtures, and BloomEffect together.
            character->translateBy(RBX::Vector3(-82.5f, 3.0f, 102.0f) -
                root->getCoordinateFrame().translation);
        }
        if (verifyRespawn)
            state->initialRespawnCharacter = localPlayer->getCharacter();

        const bool verifiesHeadlessMovement =
            disableAudioOutput && !useDurangoLauncher &&
            !verifySurfaceTextures && !verifyShadowMap &&
            !verifySkybox;
        if (useR15Character && verifiesHeadlessMovement) {
            RBX::MeshContentProvider* meshContentProvider =
                RBX::ServiceProvider::create<RBX::MeshContentProvider>(
                    state->dataModel.get());
            RBX::ModelInstance* character = localPlayer->getCharacter();
            for (std::size_t index = 0; index < character->numChildren(); ++index) {
                if (RBX::MeshPart* meshPart =
                        RBX::Instance::fastDynamicCast<RBX::MeshPart>(
                            character->getChild(index)))
                    meshContentProvider->blockingRequestContent(
                        meshPart->getMeshId(), true);
            }
        }

        // A local place uses the engine's established Play Solo role: with no
        // NetworkClient or NetworkServer, frontend and backend processing are
        // both active and RemoteEvent/RemoteFunction dispatch stays local.
        // This is what lets genuine ServerScriptService and LocalScript code
        // coexist without weakening either side's remote authority checks.
        // The generated Player smoke place remains a client-only session and
        // uses its explicit offline character authority for respawning.
        if (placePath.empty() && !useDurangoLauncher) {
            RBX::ServiceProvider::create<RBX::Network::Client>(state->dataModel.get());
            localPlayer->enableOfflineCharacterAutoSpawn();
        }
        if (verifyAudio) {
            RBX::PartInstance* audioEmitter =
                localPlayer->getCharacter()->getPrimaryPartSetByUser();
            if (!audioEmitter)
                audioEmitter = localPlayer->getCharacter()->findFirstChildOfType<RBX::PartInstance>();
            if (!audioEmitter)
                throw std::runtime_error(
                    "Player audio verification requires a character emitter part");
            state->verificationAudioEmitter = createPart(
                workspace, "PlayerAudioVerificationEmitter",
                RBX::Vector3(0.2f, 0.2f, 0.2f),
                audioEmitter->getCoordinateFrame().translation,
                RBX::BrickColor::brickWhite());
            state->verificationSound =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::SoundChannel>();
            state->verificationSound->setName("PlayerAudioVerification");
            state->verificationSound->setSoundId(
                RBX::Soundscape::SoundId("rbxasset://sounds/uuhhh.mp3"));
            state->verificationSound->setVolume(0.5f);
            state->verificationSound->setPlaybackSpeed(1.0f);
            state->verificationSound->setLooped(true);
            state->verificationSound->setParent(state->verificationAudioEmitter.get());
            state->audioLoopConnection =
                state->verificationSound->soundLoopedSignal.connect(
                    [runtimeState = state.get()](const std::string&, int) {
                        ++runtimeState->audioObservedLoops;
                    });
            state->verificationSound->play();
            state->verificationGraphEmitter =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioEmitter>();
            state->verificationGraphEmitter->setName(
                "PlayerAudioVerificationGraphEmitter");
            state->verificationGraphEmitter->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioPlayer =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioPlayer>();
            state->verificationAudioPlayer->setName(
                "PlayerAudioVerificationAudioPlayer");
            state->verificationAudioPlayer->setAutoLoad(false);
            state->verificationAudioPlayer->setAssetId(
                "rbxasset://sounds/uuhhh.mp3");
            state->verificationAudioPlayer->setVolume(0.25f);
            state->verificationAudioPlayer->setLooping(true);
            state->verificationAudioPlayer->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioFader =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioFader>();
            state->verificationAudioFader->setName(
                "PlayerAudioVerificationFader");
            state->verificationAudioFader->setVolume(0.5f);
            state->verificationAudioFader->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioDistortion =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioDistortion>();
            state->verificationAudioDistortion->setName(
                "PlayerAudioVerificationDistortion");
            state->verificationAudioDistortion->setLevel(0.15f);
            state->verificationAudioDistortion->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioTremolo =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioTremolo>();
            state->verificationAudioTremolo->setName(
                "PlayerAudioVerificationTremolo");
            state->verificationAudioTremolo->setDepth(0.2f);
            state->verificationAudioTremolo->setDuty(1.0f);
            state->verificationAudioTremolo->setFrequency(4.0f);
            state->verificationAudioTremolo->setShape(0.5f);
            state->verificationAudioTremolo->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioChorus =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioChorus>();
            state->verificationAudioChorus->setName(
                "PlayerAudioVerificationChorus");
            state->verificationAudioChorus->setDepth(0.1f);
            state->verificationAudioChorus->setMix(0.1f);
            state->verificationAudioChorus->setRate(1.5f);
            state->verificationAudioChorus->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioFlanger =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioFlanger>();
            state->verificationAudioFlanger->setName(
                "PlayerAudioVerificationFlanger");
            state->verificationAudioFlanger->setDepth(0.1f);
            state->verificationAudioFlanger->setMix(0.1f);
            state->verificationAudioFlanger->setRate(1.0f);
            state->verificationAudioFlanger->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioCompressor =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioCompressor>();
            state->verificationAudioCompressor->setName(
                "PlayerAudioVerificationCompressor");
            state->verificationAudioCompressor->setAttack(0.01f);
            state->verificationAudioCompressor->setRatio(2.0f);
            state->verificationAudioCompressor->setRelease(0.1f);
            state->verificationAudioCompressor->setThreshold(-6.0f);
            state->verificationAudioCompressor->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioGate =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioGate>();
            state->verificationAudioGate->setName(
                "PlayerAudioVerificationGate");
            state->verificationAudioGate->setThreshold(
                RBX::NumberRange(-70.0f, -60.0f));
            state->verificationAudioGate->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioLimiter =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioLimiter>();
            state->verificationAudioLimiter->setName(
                "PlayerAudioVerificationLimiter");
            state->verificationAudioLimiter->setMaxLevel(-1.0f);
            state->verificationAudioLimiter->setRelease(0.1f);
            state->verificationAudioLimiter->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioEqualizer =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioEqualizer>();
            state->verificationAudioEqualizer->setName(
                "PlayerAudioVerificationEqualizer");
            state->verificationAudioEqualizer->setLowGain(-1.0f);
            state->verificationAudioEqualizer->setMidRange(
                RBX::NumberRange(400.0f, 4000.0f));
            state->verificationAudioEqualizer->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioFilter =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioFilter>();
            state->verificationAudioFilter->setName(
                "PlayerAudioVerificationFilter");
            state->verificationAudioFilter->setFilterType(
                RBX::Soundscape::AUDIO_FILTER_LOWPASS_24DB);
            state->verificationAudioFilter->setFrequency(12000.0f);
            state->verificationAudioFilter->setQ(0.70710678f);
            state->verificationAudioFilter->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioPitchShifter =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioPitchShifter>();
            state->verificationAudioPitchShifter->setName(
                "PlayerAudioVerificationPitchShifter");
            state->verificationAudioPitchShifter->setPitch(1.01f);
            state->verificationAudioPitchShifter->setWindowSize(
                RBX::Soundscape::AUDIO_WINDOW_SMALL);
            state->verificationAudioPitchShifter->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioEcho =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioEcho>();
            state->verificationAudioEcho->setName(
                "PlayerAudioVerificationEcho");
            state->verificationAudioEcho->setDelayTime(0.05f);
            state->verificationAudioEcho->setFeedback(0.1f);
            state->verificationAudioEcho->setWetLevel(-12.0f);
            state->verificationAudioEcho->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioReverb =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioReverb>();
            state->verificationAudioReverb->setName(
                "PlayerAudioVerificationReverb");
            state->verificationAudioReverb->setDecayTime(0.5f);
            state->verificationAudioReverb->setEarlyDelayTime(0.01f);
            state->verificationAudioReverb->setLateDelayTime(0.01f);
            state->verificationAudioReverb->setWetLevel(-18.0f);
            state->verificationAudioReverb->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioAnalyzer =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioAnalyzer>();
            state->verificationAudioAnalyzer->setName(
                "PlayerAudioVerificationAnalyzer");
            state->verificationAudioAnalyzer->setWindowSize(
                RBX::Soundscape::AUDIO_WINDOW_SMALL);
            state->verificationAudioAnalyzer->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioMixer =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioChannelMixer>();
            state->verificationAudioMixer->setName(
                "PlayerAudioVerificationMixer");
            state->verificationAudioMixer->setLayout(
                RBX::Soundscape::AUDIO_CHANNEL_STEREO);
            state->verificationAudioMixer->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioSplitter =
                RBX::Creatable<RBX::Instance>::create<
                    RBX::Soundscape::AudioChannelSplitter>();
            state->verificationAudioSplitter->setName(
                "PlayerAudioVerificationSplitter");
            state->verificationAudioSplitter->setLayout(
                RBX::Soundscape::AUDIO_CHANNEL_STEREO);
            state->verificationAudioSplitter->setParent(
                state->verificationAudioEmitter.get());
            state->verificationAudioWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioWire->setName(
                "PlayerAudioVerificationWire");
            state->verificationAudioWire->setSourceInstance(
                state->verificationAudioPlayer.get());
            state->verificationAudioWire->setTargetInstance(
                state->verificationAudioFader.get());
            state->verificationAudioWire->setParent(
                state->verificationAudioPlayer.get());
            state->verificationAudioFaderWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioFaderWire->setName(
                "PlayerAudioVerificationFaderWire");
            state->verificationAudioFaderWire->setSourceInstance(
                state->verificationAudioFader.get());
            state->verificationAudioFaderWire->setTargetInstance(
                state->verificationAudioDistortion.get());
            state->verificationAudioFaderWire->setParent(
                state->verificationAudioFader.get());
            state->verificationAudioDistortionWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioDistortionWire->setName(
                "PlayerAudioVerificationDistortionWire");
            state->verificationAudioDistortionWire->setSourceInstance(
                state->verificationAudioDistortion.get());
            state->verificationAudioDistortionWire->setTargetInstance(
                state->verificationAudioTremolo.get());
            state->verificationAudioDistortionWire->setParent(
                state->verificationAudioDistortion.get());
            state->verificationAudioTremoloWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioTremoloWire->setName(
                "PlayerAudioVerificationTremoloWire");
            state->verificationAudioTremoloWire->setSourceInstance(
                state->verificationAudioTremolo.get());
            state->verificationAudioTremoloWire->setTargetInstance(
                state->verificationAudioChorus.get());
            state->verificationAudioTremoloWire->setParent(
                state->verificationAudioTremolo.get());
            state->verificationAudioChorusWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioChorusWire->setName(
                "PlayerAudioVerificationChorusWire");
            state->verificationAudioChorusWire->setSourceInstance(
                state->verificationAudioChorus.get());
            state->verificationAudioChorusWire->setTargetInstance(
                state->verificationAudioFlanger.get());
            state->verificationAudioChorusWire->setParent(
                state->verificationAudioChorus.get());
            state->verificationAudioFlangerWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioFlangerWire->setName(
                "PlayerAudioVerificationFlangerWire");
            state->verificationAudioFlangerWire->setSourceInstance(
                state->verificationAudioFlanger.get());
            state->verificationAudioFlangerWire->setTargetInstance(
                state->verificationAudioCompressor.get());
            state->verificationAudioFlangerWire->setParent(
                state->verificationAudioFlanger.get());
            state->verificationAudioCompressorWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioCompressorWire->setName(
                "PlayerAudioVerificationCompressorWire");
            state->verificationAudioCompressorWire->setSourceInstance(
                state->verificationAudioCompressor.get());
            state->verificationAudioCompressorWire->setTargetInstance(
                state->verificationAudioGate.get());
            state->verificationAudioCompressorWire->setParent(
                state->verificationAudioCompressor.get());
            state->verificationAudioGateWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioGateWire->setName(
                "PlayerAudioVerificationGateWire");
            state->verificationAudioGateWire->setSourceInstance(
                state->verificationAudioGate.get());
            state->verificationAudioGateWire->setTargetInstance(
                state->verificationAudioLimiter.get());
            state->verificationAudioGateWire->setParent(
                state->verificationAudioGate.get());
            state->verificationAudioLimiterWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioLimiterWire->setName(
                "PlayerAudioVerificationLimiterWire");
            state->verificationAudioLimiterWire->setSourceInstance(
                state->verificationAudioLimiter.get());
            state->verificationAudioLimiterWire->setTargetInstance(
                state->verificationAudioEqualizer.get());
            state->verificationAudioLimiterWire->setParent(
                state->verificationAudioLimiter.get());
            state->verificationAudioEqualizerWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioEqualizerWire->setName(
                "PlayerAudioVerificationEqualizerWire");
            state->verificationAudioEqualizerWire->setSourceInstance(
                state->verificationAudioEqualizer.get());
            state->verificationAudioEqualizerWire->setTargetInstance(
                state->verificationAudioFilter.get());
            state->verificationAudioEqualizerWire->setParent(
                state->verificationAudioEqualizer.get());
            state->verificationAudioFilterWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioFilterWire->setName(
                "PlayerAudioVerificationFilterWire");
            state->verificationAudioFilterWire->setSourceInstance(
                state->verificationAudioFilter.get());
            state->verificationAudioFilterWire->setTargetInstance(
                state->verificationAudioPitchShifter.get());
            state->verificationAudioFilterWire->setParent(
                state->verificationAudioFilter.get());
            state->verificationAudioPitchShifterWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioPitchShifterWire->setName(
                "PlayerAudioVerificationPitchShifterWire");
            state->verificationAudioPitchShifterWire->setSourceInstance(
                state->verificationAudioPitchShifter.get());
            state->verificationAudioPitchShifterWire->setTargetInstance(
                state->verificationAudioEcho.get());
            state->verificationAudioPitchShifterWire->setParent(
                state->verificationAudioPitchShifter.get());
            state->verificationAudioEchoWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioEchoWire->setName(
                "PlayerAudioVerificationEchoWire");
            state->verificationAudioEchoWire->setSourceInstance(
                state->verificationAudioEcho.get());
            state->verificationAudioEchoWire->setTargetInstance(
                state->verificationAudioReverb.get());
            state->verificationAudioEchoWire->setParent(
                state->verificationAudioEcho.get());
            state->verificationAudioReverbWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioReverbWire->setName(
                "PlayerAudioVerificationReverbWire");
            state->verificationAudioReverbWire->setSourceInstance(
                state->verificationAudioReverb.get());
            state->verificationAudioReverbWire->setTargetInstance(
                state->verificationAudioMixer.get());
            state->verificationAudioReverbWire->setTargetName("Right");
            state->verificationAudioReverbWire->setParent(
                state->verificationAudioReverb.get());
            state->verificationAudioAnalyzerWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioAnalyzerWire->setName(
                "PlayerAudioVerificationAnalyzerWire");
            state->verificationAudioAnalyzerWire->setSourceInstance(
                state->verificationAudioReverb.get());
            state->verificationAudioAnalyzerWire->setTargetInstance(
                state->verificationAudioAnalyzer.get());
            state->verificationAudioAnalyzerWire->setParent(
                state->verificationAudioAnalyzer.get());
            state->verificationAudioMixerWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioMixerWire->setName(
                "PlayerAudioVerificationMixerWire");
            state->verificationAudioMixerWire->setSourceInstance(
                state->verificationAudioMixer.get());
            state->verificationAudioMixerWire->setTargetInstance(
                state->verificationAudioSplitter.get());
            state->verificationAudioMixerWire->setParent(
                state->verificationAudioMixer.get());
            state->verificationAudioSplitterWire =
                RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
            state->verificationAudioSplitterWire->setName(
                "PlayerAudioVerificationSplitterWire");
            state->verificationAudioSplitterWire->setSourceInstance(
                state->verificationAudioSplitter.get());
            state->verificationAudioSplitterWire->setSourceName("Right");
            state->verificationAudioSplitterWire->setTargetInstance(
                state->verificationGraphEmitter.get());
            state->verificationAudioSplitterWire->setParent(
                state->verificationAudioSplitter.get());
            if (!state->verificationAudioWire->getConnected() ||
                !state->verificationAudioFaderWire->getConnected() ||
                !state->verificationAudioDistortionWire->getConnected() ||
                !state->verificationAudioTremoloWire->getConnected() ||
                !state->verificationAudioChorusWire->getConnected() ||
                !state->verificationAudioFlangerWire->getConnected() ||
                !state->verificationAudioCompressorWire->getConnected() ||
                !state->verificationAudioGateWire->getConnected() ||
                !state->verificationAudioLimiterWire->getConnected() ||
                !state->verificationAudioEqualizerWire->getConnected() ||
                !state->verificationAudioMixerWire->getConnected() ||
                !state->verificationAudioSplitterWire->getConnected())
                throw std::runtime_error(
                    "current combined-channel AudioPlayer graph did not connect");
            RBX::Camera* graphCamera = workspace->getCamera();
            if (!graphCamera)
                throw std::runtime_error(
                    "current AudioListener graph requires the workspace camera");
            RBX::Soundscape::SoundService* audioService =
                RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(
                    state->dataModel.get());
            if (!audioService)
                throw std::runtime_error(
                    "current default AudioListener proof has no SoundService");
            audioService->setDefaultListenerLocation(
                RBX::Soundscape::CharacterListener);
            RBX::PartInstance* listenerCharacterPart =
                localPlayer->getCharacter()
                ? localPlayer->getCharacter()->getPrimaryPart() : nullptr;
            RBX::Attachment* characterListenerAttachment = listenerCharacterPart
                ? RBX::Instance::fastDynamicCast<RBX::Attachment>(
                    listenerCharacterPart->findFirstChildByName(
                        "DefaultAudioListenerAttachment"))
                : nullptr;
            RBX::Soundscape::AudioListener* characterListener =
                characterListenerAttachment
                ? characterListenerAttachment->findFirstChildOfType<
                    RBX::Soundscape::AudioListener>() : nullptr;
            if (!characterListenerAttachment || !characterListener ||
                !characterListener->findFirstChildOfType<RBX::Soundscape::Wire>() ||
                !audioService->findFirstChildOfType<
                    RBX::Soundscape::AudioDeviceOutput>())
                throw std::runtime_error(
                    "current automatic character AudioListener graph did not connect");
            audioService->setDefaultListenerLocation(
                RBX::Soundscape::CameraDefaultListener);
            if (listenerCharacterPart->findFirstChildByName(
                    "DefaultAudioListenerAttachment"))
                throw std::runtime_error(
                    "automatic character AudioListener attachment was not replaced");
            state->verificationAudioListener = RBX::shared_from(
                graphCamera->findFirstChildOfType<
                    RBX::Soundscape::AudioListener>());
            state->verificationAudioOutput = RBX::shared_from(
                audioService->findFirstChildOfType<
                    RBX::Soundscape::AudioDeviceOutput>());
            state->verificationAudioListenerWire = state->verificationAudioListener
                ? RBX::shared_from(
                    state->verificationAudioListener->findFirstChildOfType<
                        RBX::Soundscape::Wire>())
                : boost::shared_ptr<RBX::Soundscape::Wire>();
            if (!state->verificationAudioListener ||
                !state->verificationAudioOutput ||
                !state->verificationAudioListenerWire ||
                !state->verificationAudioListenerWire->getConnected())
                throw std::runtime_error(
                    "current automatic camera AudioListener graph did not connect");
            lua_State* schedulingState = luaL_newstate();
            if (!audioService || !schedulingState)
                throw std::runtime_error(
                    "current AudioPlayer scheduling proof could not initialize");
            const double scheduledTime = audioService->getMixerTime() + 0.05;
            state->audioScheduledPlayTime = scheduledTime;
            lua_pushnil(schedulingState);
            lua_pushnumber(schedulingState, scheduledTime);
            if (state->verificationAudioPlayer->playLua(schedulingState) != 1 ||
                !lua_isnumber(schedulingState, -1))
                throw std::runtime_error(
                    "current AudioPlayer did not return a scheduled Play action ID");
            const long long cancelledAction = static_cast<long long>(
                lua_tonumber(schedulingState, -1));
            lua_settop(schedulingState, 0);
            lua_pushnil(schedulingState);
            lua_pushnumber(schedulingState,
                static_cast<lua_Number>(cancelledAction));
            if (state->verificationAudioPlayer->cancelLua(schedulingState) != 1 ||
                !lua_toboolean(schedulingState, -1))
                throw std::runtime_error(
                    "current AudioPlayer did not cancel a scheduled Play action");
            state->audioScheduledPlayCancelled = true;
            lua_settop(schedulingState, 0);
            lua_pushnil(schedulingState);
            lua_pushnumber(schedulingState, scheduledTime);
            if (state->verificationAudioPlayer->playLua(schedulingState) != 1 ||
                !lua_isnumber(schedulingState, -1))
                throw std::runtime_error(
                    "current AudioPlayer did not reschedule Play after cancellation");
            state->audioScheduledPlayAction = static_cast<long long>(
                lua_tonumber(schedulingState, -1));
            lua_close(schedulingState);
        }
        if (verifiesHeadlessMovement) {
            RBX::PartInstance* rootPart = localPlayer->getCharacter()->getPrimaryPartSetByUser();
            if (!rootPart)
                rootPart = localPlayer->getCharacter()->findFirstChildOfType<RBX::PartInstance>();
            if (!rootPart)
                throw std::runtime_error("headless gameplay verification requires a character part");
            state->verificationStart = rootPart->getCoordinateFrame().translation;
            state->verifiesMovement = true;
        }

        RBX::Camera* camera = workspace->getCamera();
        camera->setViewport(RBX::Vector2int16(
            static_cast<std::int16_t>(logicalWidth),
            static_cast<std::int16_t>(logicalHeight)));
        State* runtimeState = state.get();
        state->cameraFrameConnection = camera->cframeChangedSignal.connect(
            [runtimeState](RBX::CoordinateFrame frame) {
                if (runtimeState->verifiesMovement &&
                    runtimeState->renderingFrame >= 185 &&
                    runtimeState->renderingFrame <= 212) {
                    runtimeState->cameraChangesThisFrame.push_back(frame.lookVector());
                }
            });
        // Observe the public stream consumed by the gameplay CameraScript,
        // rather than CoreScript-only input, when diagnosing camera motion.
        state->inputUpdatedConnection = inputService->inputUpdatedEvent.connect(
            [runtimeState](boost::shared_ptr<RBX::Instance> instance, bool processed) {
                RBX::InputObject* input =
                    RBX::Instance::fastDynamicCast<RBX::InputObject>(instance.get());
                if (input && runtimeState->verifiesMovement &&
                    runtimeState->renderingFrame >= 185 &&
                    runtimeState->renderingFrame <= 212 &&
                    (input->getUserInputType() ==
                         RBX::InputObject::TYPE_MOUSEMOVEMENT ||
                     input->getUserInputType() ==
                         RBX::InputObject::TYPE_MOUSEDELTA))
                {
                    runtimeState->mouseChangesThisFrame.push_back(input->getDelta());
                    std::cout << "camera input event frame="
                              << runtimeState->renderingFrame
                              << " type=" << input->getUserInputType()
                              << " delta=" << input->getDelta()
                              << " processed=" << processed << '\n';
                }
            });

        if (useCurrentInExperienceUi) {
        const std::filesystem::path inExperienceRoot =
            resourceRoot / "models" / "InExperience";
        // These client-owned services exist before the current CoreScript graph
        // starts. Several genuine packages intentionally use FindService so
        // they can distinguish the native Player environment from Studio/tests.
        RBX::ServiceProvider::create<RBX::AppStorageService>(state->dataModel.get());
        RBX::AchievementService* achievements =
            RBX::ServiceProvider::create<RBX::AchievementService>(state->dataModel.get());
        if (achievements->isAvailable())
            throw std::runtime_error("offline AchievementService is unexpectedly available");
        RBX::MemStorageService* memoryStorage =
            RBX::ServiceProvider::create<RBX::MemStorageService>(state->dataModel.get());
        // This policy describes capabilities of the offline host, not a UI
        // rollout override: screenshot persistence, gallery retrieval, size
        // accounting, and bulk deletion are all implemented by CaptureService.
        // Network sharing and video remain false because this host cannot
        // perform them. The supplied 2026 CapturesPolicy consumes this exact
        // app-policy contract before SettingsHub mounts.
        const std::string capturePolicyKey =
            "GUAC:" + std::to_string(localPlayer->getUserID()) + ":app-policy";
        memoryStorage->setItem(capturePolicyKey,
            "{\"EligibleForCapturesFeature\":true,"
            "\"EligibleForVideoCapture\":false,"
            "\"EnableCapturesBulkManagement\":true,"
            "\"EnableShareCaptureCTA\":false,"
            "\"HasVideoCaptureCapability\":false}");
        RBX::ServiceProvider::create<RBX::MessageBusService>(state->dataModel.get());
        RBX::RtMessagingService* realtimeMessaging =
            RBX::ServiceProvider::create<RBX::RtMessagingService>(state->dataModel.get());
        if (realtimeMessaging->getTransportState() !=
                RBX::RtMessagingService::TRANSPORT_NONE ||
            realtimeMessaging->hasTransport())
            throw std::runtime_error("offline RtMessaging transport is not None");
        RBX::ServiceProvider::create<RBX::IXPService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::LocalizationService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::TextChatService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::AvatarChatService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::CaptureService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::FaceAnimatorService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::FeatureRestrictionManager>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::VideoCaptureService>(state->dataModel.get());
        RBX::EventIngestService* eventIngest =
            RBX::ServiceProvider::create<RBX::EventIngestService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::RbxAnalyticsService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::TelemetryService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::ExperienceService>(state->dataModel.get());
        RBX::ExperienceNotificationService* experienceNotifications =
            RBX::ServiceProvider::create<RBX::ExperienceNotificationService>(
                state->dataModel.get());
        experienceNotifications->initializePromptEligibility(false);
        RBX::ServiceProvider::create<RBX::SessionService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::ScriptProfilerService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::VRService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::PolicyService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::LinkingService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::GamepadService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::GenericChallengeService>(state->dataModel.get());
        RBX::ServiceProvider::create<RBX::TweenService>(state->dataModel.get());
        if (disableAudioOutput)
        {
            eventIngest->setTransportEnabled(false);
            boost::shared_ptr<RBX::Reflection::ValueTable> eventFields(
                new RBX::Reflection::ValueTable());
            (*eventFields)["source"] =
                RBX::Reflection::Variant(std::string("runtime-probe"));
            (*eventFields)["count"] = RBX::Reflection::Variant(2);
            eventIngest->sendEventDeferred("client", "uiBackport",
                "serviceContract", eventFields);
            eventIngest->sendEventImmediately("client", "uiBackport",
                "immediateContract", eventFields);
            const std::vector<std::string> serializedEvents =
                eventIngest->drainSerializedEventsForTesting();
            if (serializedEvents.size() != 2 ||
                serializedEvents[0].find("target=client&ctx=uiBackport&evt=immediateContract&lt=") != 0 ||
                serializedEvents[1].find("target=client&ctx=uiBackport&evt=serviceContract&lt=") != 0 ||
                serializedEvents[0].find("&count=2") == std::string::npos ||
                serializedEvents[0].find("&source=runtime%2Dprobe") == std::string::npos)
                throw std::runtime_error("EventIngestService transport contract failed");
        }
        RBX::DataModelPatch::applyBundled(state->dataModel.get(),
            inExperienceRoot / "InExperience.rbxm",
            inExperienceRoot / "InExperience_checksum");

        RBX::CoreGuiService* patchedCoreGui =
            RBX::ServiceProvider::find<RBX::CoreGuiService>(state->dataModel.get());
        RBX::Instance* robloxGui = patchedCoreGui
            ? patchedCoreGui->findFirstChildByName("RobloxGui")
            : nullptr;
        if (!robloxGui)
            throw std::runtime_error("InExperience patch has no RobloxGui root");
        if (!robloxGui->findFirstChildByName("Sounds"))
        {
            boost::shared_ptr<RBX::Folder> sounds =
                RBX::Creatable<RBX::Instance>::create<RBX::Folder>();
            sounds->setName("Sounds");
            sounds->setParent(robloxGui);
        }
        RBX::CorePackages* corePackages =
            RBX::ServiceProvider::find<RBX::CorePackages>(state->dataModel.get());
        if (!corePackages || !corePackages->findFirstChildByName("Packages") ||
            !corePackages->findFirstChildByName("Workspace"))
            throw std::runtime_error("InExperience CorePackages mount is incomplete");
        const RBX::Reflection::Tuple noArguments;
        std::auto_ptr<RBX::Reflection::Tuple> packageProbe =
            scriptContext->executeInNewThread(RBX::Security::RobloxGameScript_,
                RBX::ProtectedString::fromTrustedSource(
                    "return game:GetService('CorePackages').Workspace, "
                    "game:GetService('CorePackages').Workspace.Packages"),
                "=CorePackagesMountProbe", noArguments);
        if (!packageProbe.get() || packageProbe->values.size() != 2)
            throw std::runtime_error("InExperience CorePackages are not visible through Luau");
        std::auto_ptr<RBX::Reflection::Tuple> foundationServicesProbe =
            scriptContext->executeInNewThread(RBX::Security::RobloxGameScript_,
                RBX::ProtectedString::fromTrustedSource(
                    "local localization = game:GetService('LocalizationService')\n"
                    "assert(localization ~= nil, 'LocalizationService is not public')\n"
                    "assert(localization.RobloxLocaleId ~= nil, 'LocalizationService has no RobloxLocaleId')\n"
                    "local packages = game:GetService('CorePackages').Packages\n"
                    "local services = require(packages._Index.Foundation.Foundation.Utility.Wrappers.Services)\n"
                    "assert(type(services) == 'table', 'Foundation Services did not return a table')\n"
                    "assert(services.LocalizationService ~= nil, 'Foundation Services dropped LocalizationService')\n"
                    "assert(services.LocalizationService == localization, 'Foundation Services returned a different LocalizationService')\n"
                    "local avatarChat = game:FindService('AvatarChatService')\n"
                    "assert(avatarChat ~= nil, 'AvatarChatService was not created by the Player host')\n"
                    "local avatarChatOk, clientFeatures = pcall(function() return avatarChat:GetClientFeaturesAsync() end)\n"
                    "assert(avatarChatOk and clientFeatures == 0, 'offline AvatarChatService feature query failed: ' .. tostring(clientFeatures))\n"
                    "local experienceNotifications = game:FindService('ExperienceNotificationService')\n"
                    "assert(experienceNotifications ~= nil, 'ExperienceNotificationService was not created by the Player host')\n"
                    "local notificationOk, canPrompt = pcall(function() return experienceNotifications:CanPromptOptInAsync() end)\n"
                    "assert(notificationOk and canPrompt == false, 'offline ExperienceNotificationService eligibility failed: ' .. tostring(canPrompt))\n"
                    "local utc = DateTime.fromUniversalTime(2000, 1, 2, 3, 4, 5, 6)\n"
                    "assert(utc.UnixTimestampMillis == 946782245006, 'DateTime UTC conversion failed')\n"
                    "assert(utc:ToIsoDate() == '2000-01-02T03:04:05.006Z', 'DateTime ISO conversion failed')\n"
                    "assert(utc:FormatUniversalTime('L', 'en-us') == '01/02/2000', 'DateTime locale formatting failed')\n"
                    "local localDate = DateTime.fromLocalTime(2000, 1, 2, 3, 4, 5, 6):ToLocalTime()\n"
                    "assert(localDate.Year == 2000 and localDate.Month == 1 and localDate.Day == 2 and localDate.Hour == 3 and localDate.Minute == 4 and localDate.Second == 5 and localDate.Millisecond == 6, 'DateTime local conversion failed')\n"
                    "local color = Color3.fromHex('#2BB1FF')\n"
                    "assert(color:ToHex() == '2BB1FF' and math.abs(color.R - 43 / 255) < 1e-6 and math.abs(color.G - 177 / 255) < 1e-6 and color.B == 1, 'Color3 hex conversion failed')\n"
                    "assert(Enum.TextTruncate.None.Value == 0 and Enum.TextTruncate.AtEnd.Value == 1 and Enum.TextTruncate.SplitWord.Value == 2, 'TextTruncate enum contract failed')\n"
                    "local truncateLabel = Instance.new('TextLabel')\n"
                    "assert(truncateLabel.TextTruncate == Enum.TextTruncate.None, 'TextTruncate default failed')\n"
                    "truncateLabel.TextTruncate = Enum.TextTruncate.SplitWord\n"
                    "assert(truncateLabel.TextTruncate == Enum.TextTruncate.SplitWord, 'TextTruncate reflection failed')\n"
                    "local camera = workspace.CurrentCamera\n"
                    "assert(camera ~= nil, 'Workspace has no CurrentCamera')\n"
                    "local viewport = camera.ViewportSize\n"
                    "assert(viewport ~= nil, 'Camera ViewportSize reflection returned nil')\n"
                    "assert(viewport.X > 0 and viewport.Y > 0, 'Camera ViewportSize was not initialized')\n"
                    "assert(Vector2.zero == Vector2.new(0, 0) and Vector2.one == Vector2.new(1, 1), 'Vector2 constants failed')\n"
                    "assert(Vector3.zero == Vector3.new(0, 0, 0) and Vector3.one == Vector3.new(1, 1, 1), 'Vector3 constants failed')\n"
					"local wrapped = coroutine.wrap(function() return 42 end)\n"
					"assert(type(wrapped) == 'function', 'coroutine.wrap returned ' .. type(wrapped))\n"
					"local wrappedOk, wrappedValue = pcall(wrapped)\n"
					"assert(wrappedOk and wrappedValue == 42, 'yieldable pcall/coroutine.wrap contract failed: ' .. tostring(wrappedValue))\n"
					"local resumed, resumedA, resumedB = coroutine.resume(coroutine.create(function() return 17, 'signals' end))\n"
					"assert(resumed and resumedA == 17 and resumedB == 'signals', 'coroutine.resume return contract failed: ' .. tostring(resumedA) .. ', ' .. tostring(resumedB))\n"
					"local signals = require(packages.Signals)\n"
					"local computed = signals.createComputed(function() return { marker = 29 } end)\n"
					"local computedValue = computed(false)\n"
					"assert(type(computedValue) == 'table' and computedValue.marker == 29, 'Signals computed contract failed: ' .. tostring(computedValue))\n"
                    "local http = game:GetService('HttpService')\n"
                    "assert(http:JSONEncode('a\\\"b') == '\"a\\\\\\\"b\"', 'JSON string encoding failed')\n"
                    "assert(http:JSONEncode(true) == 'true' and http:JSONEncode(5) == '5', 'JSON primitive encoding failed')\n"
                    "assert(http:JSONDecode('false') == false and http:JSONDecode('\"ok\"') == 'ok', 'JSON primitive decoding failed')\n"
                    "local captures = game:GetService('CaptureService'):RetrieveCaptures()\n"
                    "assert(type(captures) == 'table', 'CaptureService RetrieveCaptures did not return an array')\n"
                    "return localization, services.LocalizationService, localization.RobloxLocaleId"),
                "=FoundationServicesProbe", noArguments);
		if (!foundationServicesProbe.get() || foundationServicesProbe->values.size() != 3)
			throw std::runtime_error("Foundation service wrapper verification failed");
        }
		if (useCurrentInExperienceUi)
		{
			// Connected clients receive this policy result through Player
			// replication.  The offline launcher instead observes the point where
			// the authentic ExperienceChat application finishes mounting.  The
			// next render step completes the pending Player state transition after
			// the mount call stack has returned; package and GUI state remain owned
			// entirely by the supplied CoreScripts.
			RBX::CoreGuiService* coreGui =
				RBX::ServiceProvider::create<RBX::CoreGuiService>(state->dataModel.get());
			State* runtimeState = state.get();
			state->offlineChatAccessPending = true;
			state->offlineChatMountConnection =
				coreGui->getOrCreateDescendantAddedSignal()->connect(
					[runtimeState](boost::shared_ptr<RBX::Instance> descendant) {
						if (descendant && descendant->getFullName() ==
								"CoreGui.ExperienceChat.appLayout")
							runtimeState->experienceChatMounted = true;
					});

            // Music is intentionally not part of this product's current
            // in-experience UI. Keep the supplied Chrome graph intact and use
            // its own availability contract so the integration is filtered
            // from every menu, shortcut, focus, and activation path.
            const RBX::Reflection::Tuple noArguments;
            std::auto_ptr<RBX::Reflection::Tuple> chromePolicyResult =
                scriptContext->executeInNewThread(
                    RBX::Security::RobloxGameScript_,
                    RBX::ProtectedString::fromTrustedSource(
                        "local coreGui = game:GetService('CoreGui')\n"
                        "task.spawn(function()\n"
                        "    coreGui:WaitForChild('TopBarApp')\n"
                        "    local robloxGui = coreGui:WaitForChild('RobloxGui')\n"
                        "    local chrome = robloxGui:WaitForChild('Modules'):WaitForChild('Chrome')\n"
                        "    local chromeService = require(chrome.Service)\n"
                        "    local function applyProductPolicy(integration)\n"
                        "        if integration and integration.id == 'music_entrypoint' then\n"
                        "            integration.availability:forceUnavailable()\n"
                        "        end\n"
                        "    end\n"
                        "    for _, integration in chromeService:integrations() do\n"
                        "        applyProductPolicy(integration)\n"
                        "    end\n"
                        "    chromeService:onIntegrationRegistered():connect(applyProductPolicy)\n"
                        "end)\n"
                        "return true"),
                    "=ChromeProductPolicy", noArguments);
            if (!chromePolicyResult.get() || chromePolicyResult->values.size() != 1 ||
                !chromePolicyResult->values.front().isType<bool>() ||
                !chromePolicyResult->values.front().cast<bool>())
                throw std::runtime_error(
                    "Chrome product policy did not disable the Music integration");
		}
		if (!useDurangoLauncher)
            state->dataModel->startCoreScripts(true, std::string());
		if (!useCurrentInExperienceUi && !useDurangoLauncher)
			localPlayer->setChatAvailabilityStatus("Enabled");

		if (verifyViewportRendering)
        {
            RBX::CoreGuiService* coreGui =
                RBX::ServiceProvider::create<RBX::CoreGuiService>(state->dataModel.get());
            boost::shared_ptr<RBX::ScreenGui> screen =
                RBX::Creatable<RBX::Instance>::create<RBX::ScreenGui>();
            screen->setName("ViewportRenderingVerification");
            screen->setParent(coreGui);

            boost::shared_ptr<RBX::ViewportFrame> viewport =
                RBX::Creatable<RBX::Instance>::create<RBX::ViewportFrame>();
            viewport->setPosition(RBX::UDim2(0.0f, 900, 0.0f, 440));
            viewport->setSize(RBX::UDim2(0.0f, 320, 0.0f, 240));
            viewport->setBackgroundColor3(RBX::Color3(0.08f, 0.1f, 0.13f));
            viewport->setBackgroundTransparency(0.0f);
            viewport->setParent(screen.get());

            boost::shared_ptr<RBX::WorldModel> world =
                RBX::Creatable<RBX::Instance>::create<RBX::WorldModel>();
            world->setParent(viewport.get());
            const bool characterWasArchivable = localPlayer->getCharacter()->getIsArchivable();
            localPlayer->getCharacter()->setIsArchivable(true);
            boost::shared_ptr<RBX::Instance> characterCopy =
                localPlayer->getCharacter()->clone(RBX::EngineCreator);
            localPlayer->getCharacter()->setIsArchivable(characterWasArchivable);
            if (!characterCopy)
                throw std::runtime_error("could not clone the character for ViewportFrame verification");
            characterCopy->setParent(world.get());
            if (RBX::ModelInstance* model =
                    RBX::Instance::fastDynamicCast<RBX::ModelInstance>(characterCopy.get()))
            {
                model->setPrimaryCFrame(RBX::CoordinateFrame(RBX::Vector3::zero()));
                RBX::Instances descendants;
                model->visitDescendants([&descendants](boost::shared_ptr<RBX::Instance> instance) {
                    descendants.push_back(instance);
                });
                std::size_t coloredPartIndex = 0;
                for (const boost::shared_ptr<RBX::Instance>& descendant : descendants)
                    if (RBX::PartInstance* part =
                            RBX::Instance::fastDynamicCast<RBX::PartInstance>(descendant.get())) {
                        part->setAnchored(true);
                        part->setColor((coloredPartIndex++ & 1U) == 0U
                            ? RBX::BrickColor::brickRed()
                            : RBX::BrickColor::brickBlue());
                    }
            }

            boost::shared_ptr<RBX::Camera> viewportCamera =
                RBX::Creatable<RBX::Instance>::create<RBX::Camera>();
            RBX::CoordinateFrame viewportCFrame(RBX::Vector3(0.0f, 3.0f, 12.0f));
            viewportCFrame.lookAt(RBX::Vector3::zero());
            viewportCamera->setCameraCoordinateFrame(viewportCFrame);
            viewportCamera->setFieldOfViewDegrees(45.0f);
            viewportCamera->setParent(viewport.get());
            viewport->setCurrentCamera(viewportCamera.get());
        }

        if (!videoVerificationPath.empty())
        {
            std::ifstream videoInput(videoVerificationPath, std::ios::binary);
            if (!videoInput)
                throw std::runtime_error("could not open VideoFrame verification media");
            std::vector<std::uint8_t> videoBytes{
                std::istreambuf_iterator<char>(videoInput), std::istreambuf_iterator<char>()};
            if (videoBytes.empty())
                throw std::runtime_error("VideoFrame verification media is empty");

            RBX::CoreGuiService* coreGui =
                RBX::ServiceProvider::create<RBX::CoreGuiService>(state->dataModel.get());
            boost::shared_ptr<RBX::ScreenGui> screen =
                RBX::Creatable<RBX::Instance>::create<RBX::ScreenGui>();
            screen->setName("VideoRenderingVerification");
            screen->setParent(coreGui);
            boost::shared_ptr<RBX::VideoFrame> videoFrame =
                RBX::Creatable<RBX::Instance>::create<RBX::VideoFrame>();
            videoFrame->setPosition(RBX::UDim2(0.0f, 60, 0.0f, 440));
            videoFrame->setSize(RBX::UDim2(0.0f, 320, 0.0f, 180));
            videoFrame->setBackgroundColor3(RBX::Color3(0.0f, 0.0f, 0.0f));
            videoFrame->setBackgroundTransparency(0.0f);
            videoFrame->setLooped(true);
            boost::shared_ptr<const RBX::OpaqueContent> opaque(
                new RBX::OpaqueContent(std::move(videoBytes)));
            videoFrame->setVideoContent(RBX::Content::fromOpaque(opaque));
            videoFrame->setPlaying(true);
            videoFrame->setParent(screen.get());
        }

        if (!useDurangoLauncher) {
            state->dataModel->setIsGameLoaded(true);
            RBX::ServiceProvider::create<RBX::RunService>(
                state->dataModel.get())->run();
        }
        // PlayerScripts injects the packaged ControlScript and CameraScript as
        // part of the normal LocalPlayer startup path.  A second manual load
        // races that injection and leaves two camera controllers fighting over
        // CurrentCamera, so offline play must use the same single-owner path.
    }

    // CoreScripts and the Lua GC job are live after the bootstrap lock above is
    // released. Renderer attachment mutates TextService and delivers viewport
    // property/layout signals into those scripts, so the complete attachment
    // must be one DataModel write transaction. Performing registration or the
    // initial resize outside this lock lets a scheduler GC step mark the same
    // VM concurrently with a property callback.
    {
    RBX::DataModel::LegacyLock rendererAttachmentLock(
        state->dataModel.get(), RBX::DataModelJob::Write);
    state->visualEngine = std::make_unique<RBX::Graphics::VisualEngine>(
        device, state->renderSettings);
    state->visualEngine->setThumbnailSceneProvider(
        boost::shared_ptr<RBX::ThumbnailSceneProvider>(
            new LocalPlayerThumbnailProvider(state->dataModel)));
    if (useCurrentInExperienceUi) {
        const RBX::Vector2 builderIconBounds = state->visualEngine
            ->getTypesetter(RBX::Text::FONT_BUILDER_ICONS_REGULAR)
            ->measure("tilt", 24.0f, RBX::Vector2(64.0f, 64.0f));
        if (builderIconBounds.x <= 0.0f || builderIconBounds.y <= 0.0f)
            throw std::runtime_error("Builder Icons font did not resolve the Tilt glyph");
        const RBX::Vector2 builderSansBounds = state->visualEngine
            ->getTypesetter(RBX::Text::FONT_BUILDERSANS_BOLD)
            ->measure("Party", 24.0f, RBX::Vector2(160.0f, 64.0f));
        if (builderSansBounds.x <= 0.0f || builderSansBounds.y <= 0.0f)
            throw std::runtime_error("Builder Sans font did not resolve in-experience text");
    }
    state->visualEngine->setViewport(static_cast<int>(logicalWidth),
                                     static_cast<int>(logicalHeight));
    if (RBX::TextService* textService =
            RBX::ServiceProvider::create<RBX::TextService>(state->dataModel.get())) {
        for (RBX::Text::Font font = RBX::Text::FONT_LEGACY;
             font != RBX::Text::FONT_LAST;
             font = RBX::Text::Font(font + 1)) {
            if (!RBX::Text::isValidFont(font))
                continue;
            textService->registerTypesetter(
                RBX::TextService::FromTextFont(font),
                state->visualEngine->getTypesetter(font));
        }
    }
    if (state->verifiesPlaceVisual) {
        const auto fontRenders = [this](RBX::Text::Font font) {
            const RBX::Vector2 bounds = state->visualEngine->getTypesetter(font)
                ->measure("Backrooms", 24.0f, RBX::Vector2(300.0f, 80.0f));
            return bounds.x > 0.0f && bounds.y > 0.0f;
        };
        if (!fontRenders(RBX::Text::FONT_FONDAMENTO) ||
            !fontRenders(RBX::Text::FONT_MERRIWEATHER) ||
            !fontRenders(RBX::Text::FONT_SPECIALELITE))
            throw std::runtime_error(
                "selected place custom font resources did not resolve to renderable faces");
    }
    if (state->verifiesTextRendering) {
        RBX::CoreGuiService* coreGui =
            RBX::ServiceProvider::create<RBX::CoreGuiService>(state->dataModel.get());
        boost::shared_ptr<RBX::ScreenGui> screen =
            RBX::Creatable<RBX::Instance>::create<RBX::ScreenGui>();
        screen->setName("TextRenderingVerification");
        screen->setDisplayOrder(1000);
        screen->setIgnoreGuiInset(true);
        screen->setParent(coreGui);

        boost::shared_ptr<RBX::Frame> panel =
            RBX::Creatable<RBX::Instance>::create<RBX::Frame>();
        panel->setName("TextGoldenPanel");
        panel->setPosition(RBX::UDim2(0.0f, 360, 0.0f, 470));
        panel->setSize(RBX::UDim2(0.0f, 800, 0.0f, 190));
        panel->setBackgroundColor3(RBX::Color3(0.025f, 0.03f, 0.04f));
        panel->setBackgroundTransparency(0.0f);
        panel->setParent(screen.get());

        boost::shared_ptr<RBX::TextLabel> label =
            RBX::Creatable<RBX::Instance>::create<RBX::TextLabel>();
        label->setName("UnicodeGoldenLabel");
        label->setPosition(RBX::UDim2(0.0f, 24, 0.0f, 12));
        label->setSize(RBX::UDim2(0.0f, 752, 0.0f, 74));
        label->setBackgroundTransparency(1.0f);
        label->setText("日本語 한국어 العربية עברית 😀");
        label->setFont(RBX::TextService::FONT_SOURCESANS);
        label->setTextSize(32.0f);
        label->setTextColor3(RBX::Color3(1.0f, 1.0f, 1.0f));
        label->setXAlignment(RBX::TextService::XALIGNMENT_LEFT);
        label->setYAlignment(RBX::TextService::YALIGNMENT_CENTER);
        label->setParent(panel.get());

        boost::shared_ptr<RBX::TextBox> textBox =
            RBX::Creatable<RBX::Instance>::create<RBX::TextBox>();
        textBox->setName("BidiSelectionGoldenTextBox");
        textBox->setPosition(RBX::UDim2(0.0f, 24, 0.0f, 100));
        textBox->setSize(RBX::UDim2(0.0f, 752, 0.0f, 64));
        textBox->setBackgroundColor3(RBX::Color3(0.06f, 0.07f, 0.09f));
        textBox->setBackgroundTransparency(0.0f);
        textBox->setText("abc אבג 123");
        textBox->setFont(RBX::TextService::FONT_SOURCESANS);
        textBox->setTextSize(30.0f);
        textBox->setTextColor3(RBX::Color3(1.0f, 1.0f, 1.0f));
        textBox->setXAlignment(RBX::TextService::XALIGNMENT_LEFT);
        textBox->setYAlignment(RBX::TextService::YALIGNMENT_CENTER);
        textBox->setClearTextOnFocus(false);
        textBox->setParent(panel.get());
        textBox->captureFocus();
        textBox->setSelectionStart(5);
        textBox->setCursorPosition(8);
        state->verificationTextBox = textBox;
        if (textBox->getSelectionStart() != 5 || textBox->getCursorPosition() != 8)
            throw std::runtime_error("text visual proof did not retain its logical bidi selection");
    }
    // CoreScripts mount before the graphics typesetters are available. Run the
    // host's first real viewport layout after VisualEngine initialization so
    // AutomaticSize text obtains intrinsic bounds and React AbsoluteSize
    // observers receive the settled package geometry. Production hosts perform
    // this same post-device viewport pass; keeping it here is platform-neutral.
    if (RBX::CoreGuiService* coreGui =
            RBX::ServiceProvider::find<RBX::CoreGuiService>(state->dataModel.get())) {
        boost::shared_ptr<const RBX::Instances> guiDescendants = coreGui->getDescendants();
        for (const boost::shared_ptr<RBX::Instance>& descendant : *guiDescendants)
            if (RBX::ScreenGui* screen =
                    RBX::Instance::fastDynamicCast<RBX::ScreenGui>(descendant.get()))
                screen->handleResize(screen->getViewport(), true);
    }
    if (disableAudioOutput) {
        state->verificationColor = device->createTexture(
            RBX::Graphics::Texture::Type_2D, RBX::Graphics::Texture::Format_RGBA8,
            renderWidth, renderHeight, 1, 1,
            RBX::Graphics::Texture::Usage_Renderbuffer);
        RBX::Graphics::shared_ptr<RBX::Graphics::Renderbuffer> depth =
            device->createRenderbuffer(RBX::Graphics::Texture::Format_D24S8,
                renderWidth, renderHeight, 1);
        state->verificationFramebuffer = device->createFramebuffer(
            state->verificationColor->getRenderbuffer(0, 0), depth);
    }
    RBX::Workspace* workspace = state->dataModel->getWorkspace();
    if (state->verifiesShadowMap) {
        // Exercise the terrain-specific caster material in the end-to-end
        // ShadowMap proof.  The stock Baseplate place contains an allocated but
        // empty Terrain instance, so ordinary character/part batches alone do
        // not validate this path.
        RBX::MegaClusterInstance* terrain = RBX::Instance::fastDynamicCast<RBX::MegaClusterInstance>(
            workspace->getTerrain());
        if (!terrain)
            throw std::runtime_error("ShadowMap proof requires the Workspace Terrain instance");
        if (terrain->isSmooth()) {
            const RBX::Voxel2::Region region(RBX::Vector3int32(3, 1, 0),
                RBX::Vector3int32(4, 4, 1));
            RBX::Voxel2::Box box(1, 3, 1);
            for (int y = 0; y < 3; ++y)
                box.set(0, y, 0, RBX::Voxel2::Cell(2,
                    RBX::Voxel2::Cell::Occupancy_Max));
            terrain->getSmoothGrid()->write(region, box);
        } else {
            RBX::Voxel::Cell cell;
            cell.solid.setBlock(RBX::Voxel::CELL_BLOCK_Solid);
            cell.solid.setOrientation(RBX::Voxel::CELL_ORIENTATION_NegZ);
            for (int y = 1; y <= 3; ++y)
                terrain->getVoxelGrid()->setCell(RBX::Vector3int16(3, y, 0), cell,
                    RBX::Voxel::CELL_MATERIAL_Grass);
        }
        RBX::PartInstance* baseplate = findPartByName(workspace, "BasePlate");
        if (!baseplate)
            throw std::runtime_error("ShadowMap proof could not find Baseplate CastShadow control");
        baseplate->setCastShadow(false);
    }
    state->visualEngine->bindWorkspace(state->dataModel);
    if (state->verifiesSurfaceTextures) {
        RBX::Decal* baseplateTexture = nullptr;
        RBX::Decal* spawnTexture = nullptr;
        boost::shared_ptr<const RBX::Instances> descendants = workspace->getDescendants();
        for (const boost::shared_ptr<RBX::Instance>& descendant : *descendants) {
            RBX::Decal* decal = RBX::Instance::fastDynamicCast<RBX::Decal>(descendant.get());
            if (!decal)
                continue;
            if (decal->getTexture().toString() == "rbxassetid://6372755229")
                baseplateTexture = decal;
            else if (decal->getTexture().toString() ==
                     "rbxasset://textures/SpawnLocation.png")
                spawnTexture = decal;
        }
        if (!baseplateTexture || !spawnTexture)
            throw std::runtime_error(
                "surface-texture proof place is missing the official Baseplate or SpawnLocation decal");
        state->baseplateSurfaceTexture = state->visualEngine->getTextureManager()->load(
            baseplateTexture->getTexture(), RBX::Graphics::TextureManager::Fallback_BlackTransparent,
            baseplateTexture->getFullName() + ".Texture verification");
        state->spawnSurfaceTexture = state->visualEngine->getTextureManager()->load(
            spawnTexture->getTexture(), RBX::Graphics::TextureManager::Fallback_BlackTransparent,
            spawnTexture->getFullName() + ".Texture verification");
    }
    if (state->verifiesPlaceVisual) {
        RBX::DecalTexture* wallpaper = nullptr;
        boost::shared_ptr<const RBX::Instances> descendants = workspace->getDescendants();
        for (const boost::shared_ptr<RBX::Instance>& descendant : *descendants) {
            RBX::DecalTexture* texture =
                RBX::Instance::fastDynamicCast<RBX::DecalTexture>(descendant.get());
            if (texture && texture->getTexture().toString() ==
                    "rbxassetid://3255302920") {
                wallpaper = texture;
                break;
            }
        }
        if (!wallpaper)
            throw std::runtime_error(
                "selected place is missing its authored Backrooms wallpaper Texture");
        state->wallpaperTexture = state->visualEngine->getTextureManager()->load(
            wallpaper->getTexture(),
            RBX::Graphics::TextureManager::Fallback_BlackTransparent,
            wallpaper->getFullName() + ".Texture verification");
    }
    RBX::Camera* camera = workspace->getCamera();
    state->visualEngine->setCamera(*camera, camera->getCameraFocus().translation);
    RBX::Lighting* runtimeLighting =
        RBX::ServiceProvider::create<RBX::Lighting>(state->dataModel.get());
    if (state->verifiesShadowMap) {
        runtimeLighting->setGlobalShadows(true);
        runtimeLighting->setTechnology(RBX::Lighting::TECHNOLOGY_SHADOW_MAP);
    }
    configureLighting(*state->visualEngine, *runtimeLighting);
    if (state->verifiesShadowMap) {
        RBX::Lighting* lighting =
            RBX::ServiceProvider::create<RBX::Lighting>(state->dataModel.get());
        RBX::Graphics::SceneManager* scene =
            state->visualEngine->getSceneManager();
        if (!lighting->getGlobalShadows() ||
            lighting->getTechnology() < RBX::Lighting::TECHNOLOGY_SHADOW_MAP)
            throw std::runtime_error(
                "shadow-map proof place does not request ShadowMap or Future lighting");
        if (!scene->isShadowMapEnabled() ||
            std::abs(scene->getShadowMapSoftness() -
                     lighting->getShadowSoftness()) > 0.0001f)
            throw std::runtime_error(
                "Lighting ShadowMap configuration did not reach the render scene");
    }
    state->lightingChangedConnection =
        RBX::ServiceProvider::create<RBX::Lighting>(state->dataModel.get())
            ->lightingChangedSignal.connect([this](bool) {
                configureLighting(*state->visualEngine,
                    *RBX::ServiceProvider::create<RBX::Lighting>(
                        state->dataModel.get()));
            });
}
}

PlayerRuntime::~PlayerRuntime() = default;

void PlayerRuntime::resize(unsigned int renderWidth, unsigned int renderHeight,
    unsigned int logicalWidth, unsigned int logicalHeight, float pixelDensity,
    rbx::platform::DisplayOrientation orientation, float safeAreaLeft,
    float safeAreaTop, float safeAreaRight,
    float safeAreaBottom)
{
    if (renderWidth == 0 || renderHeight == 0 ||
        logicalWidth == 0 || logicalHeight == 0 ||
        !std::isfinite(safeAreaLeft) || safeAreaLeft < 0.0f ||
        !std::isfinite(safeAreaTop) || safeAreaTop < 0.0f ||
        !std::isfinite(safeAreaRight) || safeAreaRight < 0.0f ||
        !std::isfinite(safeAreaBottom) || safeAreaBottom < 0.0f)
        throw std::invalid_argument("PlayerRuntime resize requires a nonzero viewport");

    RBX::DataModel::LegacyLock lock(
        state->dataModel.get(), RBX::DataModelJob::Write);
    state->device->resize(renderWidth, renderHeight, pixelDensity);
    state->width = renderWidth;
    state->height = renderHeight;
    state->logicalWidth = logicalWidth;
    state->logicalHeight = logicalHeight;
    state->visualEngine->setViewport(
        static_cast<int>(logicalWidth), static_cast<int>(logicalHeight));
    if (RBX::Camera* camera = state->dataModel->getWorkspace()->getCamera())
        camera->setViewport(RBX::Vector2int16(
            static_cast<std::int16_t>(logicalWidth),
            static_cast<std::int16_t>(logicalHeight)));
    const unsigned int horizontalScale =
        (renderWidth + logicalWidth / 2U) / logicalWidth;
    const unsigned int verticalScale =
        (renderHeight + logicalHeight / 2U) / logicalHeight;
    RBX::GuiService* guiService =
        RBX::ServiceProvider::create<RBX::GuiService>(state->dataModel.get());
    guiService->setResolutionScale(static_cast<int>(std::clamp(
        std::max(horizontalScale, verticalScale), 1U, 3U)));
    guiService->setHardwareSafeAreaInsets(
        safeAreaLeft, safeAreaTop, safeAreaRight, safeAreaBottom);
    if (RBX::Network::Players* players =
            RBX::ServiceProvider::find<RBX::Network::Players>(state->dataModel.get()))
        if (RBX::Network::Player* localPlayer = players->getLocalPlayer())
            if (RBX::PlayerGui* playerGui =
                    localPlayer->findFirstChildOfType<RBX::PlayerGui>())
                playerGui->setCurrentScreenOrientation(
                    screenOrientation(orientation));
}

void PlayerRuntime::handleInput(const rbx::platform::InputEvent& event)
{
    using HostEvent = rbx::platform::InputEvent;
    RBX::DataModel::LegacyLock lock(state->dataModel.get(), RBX::DataModelJob::Write);

    if (event.kind == HostEvent::Kind::nativeCloseRequested) {
        RBX::ServiceProvider::create<RBX::GuiService>(state->dataModel.get())
            ->fireNativeCloseSignal();
        return;
    }

    RBX::UserInputService* input =
        RBX::ServiceProvider::create<RBX::UserInputService>(state->dataModel.get());

    if (event.kind == HostEvent::Kind::gamepadButtonDown ||
        event.kind == HostEvent::Kind::gamepadButtonUp ||
        event.kind == HostEvent::Kind::gamepadAxis) {
        const auto gamepadType = RBX::GamepadService::getGamepadEnumForInt(
            static_cast<int>(event.gamepadIndex));
        if (gamepadType == RBX::InputObject::TYPE_NONE)
            return;

        RBX::KeyCode code = RBX::SDLK_UNKNOWN;
        switch (event.gamepadControl) {
        case HostEvent::GamepadControl::buttonA: code = RBX::SDLK_GAMEPAD_BUTTONA; break;
        case HostEvent::GamepadControl::buttonB: code = RBX::SDLK_GAMEPAD_BUTTONB; break;
        case HostEvent::GamepadControl::buttonX: code = RBX::SDLK_GAMEPAD_BUTTONX; break;
        case HostEvent::GamepadControl::buttonY: code = RBX::SDLK_GAMEPAD_BUTTONY; break;
        case HostEvent::GamepadControl::leftShoulder: code = RBX::SDLK_GAMEPAD_BUTTONL1; break;
        case HostEvent::GamepadControl::rightShoulder: code = RBX::SDLK_GAMEPAD_BUTTONR1; break;
        case HostEvent::GamepadControl::leftTrigger: code = RBX::SDLK_GAMEPAD_BUTTONL2; break;
        case HostEvent::GamepadControl::rightTrigger: code = RBX::SDLK_GAMEPAD_BUTTONR2; break;
        case HostEvent::GamepadControl::leftStick: code = RBX::SDLK_GAMEPAD_THUMBSTICK1; break;
        case HostEvent::GamepadControl::rightStick: code = RBX::SDLK_GAMEPAD_THUMBSTICK2; break;
        case HostEvent::GamepadControl::start: code = RBX::SDLK_GAMEPAD_BUTTONSTART; break;
        case HostEvent::GamepadControl::select: code = RBX::SDLK_GAMEPAD_BUTTONSELECT; break;
        case HostEvent::GamepadControl::dpadLeft: code = RBX::SDLK_GAMEPAD_DPADLEFT; break;
        case HostEvent::GamepadControl::dpadRight: code = RBX::SDLK_GAMEPAD_DPADRIGHT; break;
        case HostEvent::GamepadControl::dpadUp: code = RBX::SDLK_GAMEPAD_DPADUP; break;
        case HostEvent::GamepadControl::dpadDown: code = RBX::SDLK_GAMEPAD_DPADDOWN; break;
        case HostEvent::GamepadControl::none: break;
        }
        if (code == RBX::SDLK_UNKNOWN)
            return;

        RBX::GamepadService* gamepadService =
            RBX::ServiceProvider::create<RBX::GamepadService>(state->dataModel.get());
        RBX::Gamepad gamepad = gamepadService->getGamepadState(
            static_cast<int>(event.gamepadIndex));
        const auto found = gamepad.find(code);
        if (found == gamepad.end() || !found->second)
            return;

        boost::shared_ptr<RBX::InputObject> object = found->second;
        const RBX::Vector3 previous = object->getRawPosition();
        RBX::Vector3 position = previous;
        RBX::InputObject::UserInputState inputState;
        if (event.kind == HostEvent::Kind::gamepadButtonDown) {
            inputState = RBX::InputObject::INPUT_STATE_BEGIN;
            position = RBX::Vector3(0.0F, 0.0F, 1.0F);
        } else if (event.kind == HostEvent::Kind::gamepadButtonUp) {
            inputState = RBX::InputObject::INPUT_STATE_END;
            position = RBX::Vector3::zero();
        } else {
            position = code == RBX::SDLK_GAMEPAD_BUTTONL2 ||
                    code == RBX::SDLK_GAMEPAD_BUTTONR2
                ? RBX::Vector3(0.0F, 0.0F, event.x)
                : RBX::Vector3(event.x, event.y, 0.0F);
            inputState = position == RBX::Vector3::zero()
                ? RBX::InputObject::INPUT_STATE_END
                : position.z >= 1.0F
                    ? RBX::InputObject::INPUT_STATE_BEGIN
                    : RBX::InputObject::INPUT_STATE_CHANGE;
        }
        if (object->getUserInputState() == inputState && previous == position)
            return;
        object->setPosition(position);
        object->setDelta(position - previous);
        object->setInputState(inputState);
        input->setConnectedGamepad(gamepadType, true);
        input->dangerousFireInputEvent(object, nullptr);
        return;
    }

    if (event.kind == HostEvent::Kind::keyDown ||
        event.kind == HostEvent::Kind::keyUp) {
        const RBX::KeyCode code = translateKey(event.key);
        if (code == RBX::SDLK_UNKNOWN)
            return;
        const bool down = event.kind == HostEvent::Kind::keyDown;
        const RBX::ModCode modifiers = translateModifiers(event.modifiers);
        const char text = event.text ? event.text
                                     : RBX::UserInputService::getModifiedKey(code, modifiers);
        input->setKeyState(code, modifiers, text, down);

        boost::shared_ptr<RBX::InputObject>& object = state->keyInputs[code];
        if (!object) {
            object = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(
                RBX::InputObject::TYPE_KEYBOARD,
                down ? RBX::InputObject::INPUT_STATE_BEGIN
                     : RBX::InputObject::INPUT_STATE_END,
                code, modifiers, text, state->dataModel.get());
        } else {
            object->setInputState(down ? RBX::InputObject::INPUT_STATE_BEGIN
                                       : RBX::InputObject::INPUT_STATE_END);
            object->mod = modifiers;
            object->modifiedKey = text;
        }
        input->dangerousFireInputEvent(object, nullptr);
        if (!down)
            state->keyInputs.erase(code);
        return;
    }

    if (event.kind == HostEvent::Kind::touchDown ||
        event.kind == HostEvent::Kind::touchMove ||
        event.kind == HostEvent::Kind::touchUp ||
        event.kind == HostEvent::Kind::touchCancel) {
        input->setTouchEnabled(true);
        const auto stateValue = event.kind == HostEvent::Kind::touchDown
            ? RBX::InputObject::INPUT_STATE_BEGIN
            : event.kind == HostEvent::Kind::touchMove
                ? RBX::InputObject::INPUT_STATE_CHANGE
                : event.kind == HostEvent::Kind::touchUp
                    ? RBX::InputObject::INPUT_STATE_END
                    : RBX::InputObject::INPUT_STATE_CANCEL;
        boost::shared_ptr<RBX::InputObject>& object =
            state->pointerInputs[event.touchId];
        if (!object) {
            object = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(
                RBX::InputObject::TYPE_TOUCH, stateValue,
                RBX::Vector3(event.x, event.y, 0.0F), RBX::Vector3::zero(),
                state->dataModel.get());
        } else {
            const RBX::Vector3 previous = object->getRawPosition();
            object->setInputState(stateValue);
            object->setPosition(RBX::Vector3(event.x, event.y, 0.0F));
            object->setDelta(object->getRawPosition() - previous);
        }
        input->dangerousFireInputEvent(object,
            reinterpret_cast<void*>(static_cast<std::uintptr_t>(event.touchId + 1U)));
        if (event.kind == HostEvent::Kind::touchUp ||
            event.kind == HostEvent::Kind::touchCancel)
            state->pointerInputs.erase(event.touchId);
        return;
    }

    if (event.kind == HostEvent::Kind::focusGained ||
        event.kind == HostEvent::Kind::focusLost) {
        const bool focused = event.kind == HostEvent::Kind::focusGained;
        if (!state->audioOutputDisabled) {
            if (RBX::Soundscape::SoundService* soundService =
                    RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(
                        state->dataModel.get())) {
                if (focused)
                    soundService->getAudioEngine().resume();
                else
                    soundService->getAudioEngine().suspend();
            }
        }
        boost::shared_ptr<RBX::InputObject> object =
            RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(
                RBX::InputObject::TYPE_FOCUS,
                focused ? RBX::InputObject::INPUT_STATE_BEGIN
                        : RBX::InputObject::INPUT_STATE_END,
                state->dataModel.get());
        input->dangerousFireInputEvent(object, nullptr);
        if (!focused) {
            for (const auto& [code, keyObject] : state->keyInputs) {
                input->setKeyState(code, keyObject->mod,
                    keyObject->modifiedKey, false);
                keyObject->setInputState(RBX::InputObject::INPUT_STATE_END);
                input->dangerousFireInputEvent(keyObject, nullptr);
            }
            input->resetKeyState();
            state->keyInputs.clear();
        }
        return;
    }

    RBX::InputObject::UserInputType type = RBX::InputObject::TYPE_NONE;
    RBX::InputObject::UserInputState inputState = RBX::InputObject::INPUT_STATE_CHANGE;
    if (event.kind == HostEvent::Kind::pointerMove) {
        type = RBX::InputObject::TYPE_MOUSEMOVEMENT;
    } else if (event.kind == HostEvent::Kind::scroll) {
        type = RBX::InputObject::TYPE_MOUSEWHEEL;
    } else if (event.kind == HostEvent::Kind::pointerDown ||
               event.kind == HostEvent::Kind::pointerUp) {
        inputState = event.kind == HostEvent::Kind::pointerDown
            ? RBX::InputObject::INPUT_STATE_BEGIN
            : RBX::InputObject::INPUT_STATE_END;
        if (event.button == HostEvent::PointerButton::primary)
            type = RBX::InputObject::TYPE_MOUSEBUTTON1;
        else if (event.button == HostEvent::PointerButton::secondary)
            type = RBX::InputObject::TYPE_MOUSEBUTTON2;
        else if (event.button == HostEvent::PointerButton::middle)
            type = RBX::InputObject::TYPE_MOUSEBUTTON3;
    }
    if (type == RBX::InputObject::TYPE_NONE)
        return;

    const RBX::Vector3 position(event.x, event.y,
        type == RBX::InputObject::TYPE_MOUSEWHEEL ? event.deltaY : 0.0F);
    const RBX::Vector3 delta(event.deltaX, event.deltaY, 0.0F);
    const auto firePointerEvent = [&](RBX::InputObject::UserInputType eventType) {
		// The production desktop hosts enqueue input and UserInputService drains
		// it immediately before render-step callbacks. Each queued sample owns its
		// state so a down/up pair or a later motion cannot mutate an event that Lua
		// has not consumed yet.
		boost::shared_ptr<RBX::InputObject> sample =
			RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(
				eventType, inputState, position, delta, state->dataModel.get());
		input->fireInputEvent(sample, nullptr);
    };

    // The historical desktop hosts send a relative MouseDelta before the
    // absolute MouseMovement whenever the mouse is wrapped/captured.  Camera
    // commands consume MouseDelta; CoreGui and hover handling consume
    // MouseMovement.  Omitting either event breaks one half of that contract.
    const RBX::UserInputService::WrapMode wrap = input->getMouseWrapMode();
    const bool mouseCaptured =
        input->getMouseType() == RBX::UserInputService::MOUSETYPE_LOCKCENTER ||
        input->getMouseType() == RBX::UserInputService::MOUSETYPE_LOCKCURRENT ||
        wrap == RBX::UserInputService::WRAP_CENTER ||
        wrap == RBX::UserInputService::WRAP_NONEANDCENTER;
    if (type == RBX::InputObject::TYPE_MOUSEMOVEMENT && mouseCaptured &&
        (event.deltaX != 0.0F || event.deltaY != 0.0F))
        firePointerEvent(RBX::InputObject::TYPE_MOUSEDELTA);
    firePointerEvent(type);
}

std::optional<std::array<float, 2>>
PlayerRuntime::findVisibleGuiCenterBySuffix(
    const std::string& fullNameSuffix) const
{
    RBX::DataModel::LegacyLock lock(
        state->dataModel.get(), RBX::DataModelJob::Write);
    RBX::CoreGuiService* coreGui =
        RBX::ServiceProvider::find<RBX::CoreGuiService>(state->dataModel.get());
    if (!coreGui)
        return std::nullopt;

    std::optional<std::array<float, 2>> descendantCenter;
    boost::shared_ptr<const RBX::Instances> descendants = coreGui->getDescendants();
    for (const boost::shared_ptr<RBX::Instance>& descendant : *descendants) {
        const std::string fullName = descendant->getFullName();
        const bool exactMatch = fullName.ends_with(fullNameSuffix);
        if (!exactMatch && fullName.find(fullNameSuffix) == std::string::npos)
            continue;
        RBX::GuiObject* gui =
            RBX::Instance::fastDynamicCast<RBX::GuiObject>(descendant.get());
        if (!gui || !gui->isCurrentlyVisible())
            continue;
        const RBX::Vector2 position = gui->getAbsolutePosition();
        const RBX::Vector2 size = gui->getAbsoluteSize();
        if (size.x <= 0.0f || size.y <= 0.0f)
            continue;
        const float x = position.x + size.x * 0.5f;
        const float y = position.y + size.y * 0.5f;
        if (x >= 0.0f && y >= 0.0f &&
            x < static_cast<float>(state->logicalWidth) &&
            y < static_cast<float>(state->logicalHeight)) {
            if (exactMatch)
                return std::array<float, 2>{x, y};
            if (!descendantCenter)
                descendantCenter = std::array<float, 2>{x, y};
        }
    }
    return descendantCenter;
}

std::optional<std::array<float, 2>>
PlayerRuntime::findBoundedGuiCenterBySuffix(
    const std::string& fullNameSuffix) const
{
    RBX::DataModel::LegacyLock lock(
        state->dataModel.get(), RBX::DataModelJob::Write);
    RBX::CoreGuiService* coreGui =
        RBX::ServiceProvider::find<RBX::CoreGuiService>(state->dataModel.get());
    if (!coreGui)
        return std::nullopt;

    boost::shared_ptr<const RBX::Instances> descendants = coreGui->getDescendants();
    for (const boost::shared_ptr<RBX::Instance>& descendant : *descendants) {
        if (!descendant->getFullName().ends_with(fullNameSuffix))
            continue;
        RBX::GuiObject* gui =
            RBX::Instance::fastDynamicCast<RBX::GuiObject>(descendant.get());
        if (!gui || !gui->getVisible())
            continue;
        const RBX::Vector2 position = gui->getAbsolutePosition();
        const RBX::Vector2 size = gui->getAbsoluteSize();
        if (size.x <= 0.0f || size.y <= 0.0f)
            continue;
        const float x = position.x + size.x * 0.5f;
        const float y = position.y + size.y * 0.5f;
        if (x >= 0.0f && y >= 0.0f &&
            x < static_cast<float>(state->logicalWidth) &&
            y < static_cast<float>(state->logicalHeight))
            return std::array<float, 2>{x, y};
    }
    return std::nullopt;
}

std::optional<std::array<float, 2>>
PlayerRuntime::findVisibleGuiPointOutsideDescendantBySuffix(
    const std::string& outerFullNameSuffix,
    const std::string& innerFullNameSuffix) const
{
    RBX::DataModel::LegacyLock lock(
        state->dataModel.get(), RBX::DataModelJob::Write);
    RBX::CoreGuiService* coreGui =
        RBX::ServiceProvider::find<RBX::CoreGuiService>(state->dataModel.get());
    if (!coreGui)
        return std::nullopt;

    RBX::GuiObject* outer = nullptr;
    RBX::GuiObject* inner = nullptr;
    boost::shared_ptr<const RBX::Instances> descendants = coreGui->getDescendants();
    for (const boost::shared_ptr<RBX::Instance>& descendant : *descendants) {
        RBX::GuiObject* gui =
            RBX::Instance::fastDynamicCast<RBX::GuiObject>(descendant.get());
        if (!gui || !gui->getVisible())
            continue;
        const std::string fullName = descendant->getFullName();
        if (fullName.ends_with(outerFullNameSuffix))
            outer = gui;
        if (fullName.ends_with(innerFullNameSuffix))
            inner = gui;
    }
    if (!outer || !inner)
        return std::nullopt;

    const RBX::Vector2 outerPosition = outer->getAbsolutePosition();
    const RBX::Vector2 outerSize = outer->getAbsoluteSize();
    const RBX::Vector2 innerPosition = inner->getAbsolutePosition();
    const RBX::Vector2 innerSize = inner->getAbsoluteSize();
    const float inset = 8.0f;
    const std::array<std::array<float, 2>, 4> candidates{{
        {outerPosition.x + inset, outerPosition.y + inset},
        {outerPosition.x + outerSize.x - inset, outerPosition.y + inset},
        {outerPosition.x + inset, outerPosition.y + outerSize.y - inset},
        {outerPosition.x + outerSize.x - inset,
         outerPosition.y + outerSize.y - inset},
    }};
    for (const std::array<float, 2>& candidate : candidates) {
        const bool insideOuter =
            candidate[0] >= outerPosition.x &&
            candidate[1] >= outerPosition.y &&
            candidate[0] < outerPosition.x + outerSize.x &&
            candidate[1] < outerPosition.y + outerSize.y;
        const bool insideInner =
            candidate[0] >= innerPosition.x &&
            candidate[1] >= innerPosition.y &&
            candidate[0] < innerPosition.x + innerSize.x &&
            candidate[1] < innerPosition.y + innerSize.y;
        const bool insideViewport =
            candidate[0] >= 0.0f && candidate[1] >= 0.0f &&
            candidate[0] < static_cast<float>(state->logicalWidth) &&
            candidate[1] < static_cast<float>(state->logicalHeight);
        if (insideOuter && !insideInner && insideViewport)
            return candidate;
    }
    return std::nullopt;
}

std::optional<std::string> PlayerRuntime::selectedGuiObjectFullName() const
{
    RBX::DataModel::LegacyLock lock(
        state->dataModel.get(), RBX::DataModelJob::Write);
    RBX::GuiService* guiService =
        RBX::ServiceProvider::find<RBX::GuiService>(state->dataModel.get());
    RBX::GuiObject* selected = guiService
        ? guiService->getSelectedGuiObject() : nullptr;
    return selected ? std::optional<std::string>(selected->getFullName())
                    : std::nullopt;
}

bool PlayerRuntime::wantsPointerLock() const
{
    RBX::DataModel::LegacyLock lock(
        state->dataModel.get(), RBX::DataModelJob::Read);
    const RBX::UserInputService* input =
        RBX::ServiceProvider::find<RBX::UserInputService>(state->dataModel.get());
    if (!input)
        return false;
    const RBX::UserInputService::WrapMode wrap = input->getMouseWrapMode();
    return input->getMouseType() == RBX::UserInputService::MOUSETYPE_LOCKCENTER ||
        input->getMouseType() == RBX::UserInputService::MOUSETYPE_LOCKCURRENT ||
        wrap == RBX::UserInputService::WRAP_CENTER ||
        wrap == RBX::UserInputService::WRAP_NONEANDCENTER;
}

bool PlayerRuntime::takeOpenDocumentRequest()
{
    return state->openDocumentRequested.exchange(false, std::memory_order_acq_rel);
}

std::optional<std::filesystem::path> PlayerRuntime::takeRecentDocumentRequest()
{
    std::scoped_lock lock(state->recentDocumentMutex);
    std::optional<std::filesystem::path> result;
    result.swap(state->recentDocumentRequested);
    return result;
}

std::optional<std::string> PlayerRuntime::takeExternalUriRequest()
{
    std::scoped_lock lock(state->externalUriMutex);
    if (state->externalUriRequests.empty())
        return std::nullopt;
    std::string result = std::move(state->externalUriRequests.front());
    state->externalUriRequests.erase(state->externalUriRequests.begin());
    return result;
}

void PlayerRuntime::renderFrame(unsigned long frameNumber)
{
    RBX::DataModel::LegacyLock lock(state->dataModel.get(), RBX::DataModelJob::Write);
    state->renderingFrame = frameNumber;
    state->cameraChangesThisFrame.clear();
    state->mouseChangesThisFrame.clear();
    state->dataModel->renderStep(1.0F / 60.0F);
    if (state->verifiesKeyboardNavigation && frameNumber == 329UL) {
        RBX::GuiService* guiService =
            RBX::ServiceProvider::find<RBX::GuiService>(state->dataModel.get());
        RBX::UserInputService* inputService =
            RBX::ServiceProvider::find<RBX::UserInputService>(state->dataModel.get());
        if (!guiService ||
            guiService->getSelectedGuiObject() !=
                state->keyboardNavigationSecondButton.get() ||
            !state->keyboardNavigationFirstButton->isCurrentlyVisible() ||
            !state->keyboardNavigationSecondButton->isCurrentlyVisible() ||
            !state->keyboardNavigationActivated ||
            state->keyboardNavigationClickCount != 1 ||
            state->keyboardNavigationInputType !=
                RBX::InputObject::TYPE_KEYBOARD ||
            !inputService ||
            inputService->getLastInputType() !=
                RBX::InputObject::TYPE_KEYBOARD ||
            inputService->getPreferredInput() !=
                RBX::Enums::PREFERRED_INPUT_KEYBOARD_AND_MOUSE)
            throw std::runtime_error(
                "keyboard GUI navigation did not select and activate the second action");
        state->keyboardNavigationSelectionProved = true;
    }
    if (state->verifiesKeyboardNavigation && frameNumber == 349UL) {
        RBX::GuiService* guiService =
            RBX::ServiceProvider::find<RBX::GuiService>(state->dataModel.get());
        if (!state->keyboardNavigationSelectionProved || !guiService ||
            guiService->getSelectedGuiObject())
            throw std::runtime_error(
                "keyboard GUI navigation did not clear selection on its second toggle");
    }
    if (state->verifiesTextRendering && state->verificationTextBox) {
        if (state->verificationTextBox->getSelectionStart() < 0)
            state->verificationTextBox->captureFocus();
        state->verificationTextBox->setSelectionStart(5);
        state->verificationTextBox->setCursorPosition(8);
    }
    if (state->verifiesAudio && state->verificationSound) {
        RBX::Soundscape::SoundService* soundService =
            RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(
                state->dataModel.get());
        if (!soundService || !soundService->enabled())
            throw std::runtime_error(
                "Player audio verification has no active SoundService mixer");

        if (state->verificationSound->isSoundLoaded() &&
            state->verificationSound->isPlaying()) {
            if (state->verificationAudioPlayer) {
                if (!state->verificationAudioPlayer->getIsReady())
                    throw std::runtime_error(
                        "current AudioPlayer graph did not load its asset");
                if (state->verificationAudioPlayer->getIsPlaying())
                    state->audioPlayerMaximumPosition = std::max(
                        state->audioPlayerMaximumPosition,
                        state->verificationAudioPlayer->getTimePosition());
                else if (soundService->getMixerTime() >
                    state->audioScheduledPlayTime + 0.05)
                    throw std::runtime_error(
                        "current AudioPlayer graph missed its scheduled playback deadline");
            }
            if (state->audioLoadedFrame == 0) {
                state->audioLoadedFrame = frameNumber;
                state->verificationSound->setSoundPosition(0.0);
                state->audioPreviousPosition = 0.0;
                const double length = state->verificationSound->getSoundLength();
                if (length < 0.39 || length > 0.44)
                    throw std::runtime_error(
                        "packaged OOF sound decoded at an incorrect duration");
            }

            std::array<float, 1600> output = {};
            if (!soundService->getAudioEngine().mix(output))
                throw std::runtime_error("Player SoundService mixer rejected output");
            for (float sample : output) {
                state->audioSquaredSampleSum +=
                    static_cast<double>(sample) * static_cast<double>(sample);
                ++state->audioSampleCount;
            }
            state->audioCurrentSpeedFrames += output.size() / 2U;
            const double position = state->verificationSound->getSoundPosition();
            if (position + 0.01 < state->audioPreviousPosition) {
                if (!state->audioTestingHalfSpeed) {
                    state->audioUnitSpeedFrames = state->audioCurrentSpeedFrames;
                    state->audioCurrentSpeedFrames = 0;
                    state->audioTestingHalfSpeed = true;
                    state->verificationSound->setPlaybackSpeed(0.5f);
                    state->verificationSound->setSoundPosition(0.0);
                    state->audioPreviousPosition = 0.0;
                } else if (state->audioHalfSpeedFrames == 0) {
                    state->audioHalfSpeedFrames = state->audioCurrentSpeedFrames;
                    state->audioCurrentSpeedFrames = 0;
                }
            } else {
                state->audioPreviousPosition = position;
            }
        }
    }
    if (state->verifiesPlaceAudio) {
        if (state->placeSounds.empty() && state->dataModel && frameNumber >= 30) {
            if (!state->placeAudioEmitters.empty()) {
                RBX::Workspace* workspace =
                    RBX::ServiceProvider::find<RBX::Workspace>(state->dataModel.get());
                RBX::Camera* camera = workspace ? workspace->getCamera() : nullptr;
                if (!workspace || !camera)
                    throw std::runtime_error(
                        "selected-place audio verification has no listener workspace");
                const RBX::Vector3 listener =
                    camera->getCameraCoordinateFrame().translation;
                for (std::size_t index = 0;
                     index < state->placeAudioEmitters.size(); ++index) {
                    const auto& emitter = state->placeAudioEmitters[index];
                    if (RBX::PartInstance* part =
                            RBX::Instance::fastDynamicCast<RBX::PartInstance>(
                                emitter.get()))
                        part->setCoordinateFrame(RBX::CoordinateFrame(
                            listener + RBX::Vector3(
                                static_cast<float>(index + 1), 0.0f, 0.0f)));
                    emitter->setParent(workspace);
                    collectPlaceSounds(*emitter, state->placeSounds);
                }
            }
            if (!state->placeSounds.empty()) {
                if (state->placeSounds.size() != 3)
                    throw std::runtime_error(
                        "selected place did not replicate its three authored sounds");
                RBX::ContentProvider* contentProvider =
                    RBX::ServiceProvider::find<RBX::ContentProvider>(
                        state->dataModel.get());
                if (!contentProvider)
                    throw std::runtime_error(
                        "selected-place audio verification has no ContentProvider");
                unsigned ambienceCount = 0;
                unsigned flashlightCount = 0;
                for (const auto& sound : state->placeSounds) {
                    const std::string cachedContent =
                        contentProvider->getFile(sound->getSoundId());
                    if (cachedContent.empty() ||
                        !std::filesystem::is_regular_file(cachedContent))
                        throw std::runtime_error(
                            "selected place embedded Sound payload path is invalid: " +
                            cachedContent);
                    const std::string id = sound->getSoundId().toString();
                    if (id.find("9065112164") != std::string::npos) {
                        ++ambienceCount;
                        if (!sound->getLooped() ||
                            std::abs(sound->getVolume() - 0.25f) > 0.0001f ||
                            sound->getRollOffMode() != RBX::Soundscape::Linear ||
                            std::abs(sound->getMinDistance() - 1.0f) > 0.0001f ||
                            std::abs(sound->getMaxDistance() - 40.0f) > 0.0001f)
                            throw std::runtime_error(
                                "selected place ambience Sound schema changed");
                    } else {
                        ++flashlightCount;
                        if (sound->getLooped() ||
                            std::abs(sound->getVolume() - 1.0f) > 0.0001f ||
                            sound->getRollOffMode() != RBX::Soundscape::Linear ||
                            std::abs(sound->getMinDistance() - 5.0f) > 0.0001f ||
                            std::abs(sound->getMaxDistance() - 20.0f) > 0.0001f)
                            throw std::runtime_error(
                                "selected place flashlight Sound schema changed");
                    }
                    sound->play();
                    std::cout << "selected-place Sound start id=" << id
                              << " loaded=" << sound->isSoundLoaded()
                              << " spatial=" << sound->isSpatial()
                              << " playing=" << sound->isPlaying()
                              << " length=" << sound->getSoundLength() << '\n';
                }
                if (ambienceCount != 2 || flashlightCount != 1)
                    throw std::runtime_error(
                        "selected place Sound asset IDs changed");
                state->placeSoundMaximumPositions.assign(
                    state->placeSounds.size(), 0.0);
                state->placeSoundObservedLoaded.assign(
                    state->placeSounds.size(), false);
            }
        }

        if (!state->placeSounds.empty()) {
            RBX::Soundscape::SoundService* soundService =
                RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(
                    state->dataModel.get());
            if (!soundService || !soundService->enabled())
                throw std::runtime_error(
                    "selected place has no active SoundService mixer");
            bool allLoaded = true;
            bool anyPlaying = false;
            for (std::size_t index = 0; index < state->placeSounds.size(); ++index) {
                const auto& sound = state->placeSounds[index];
                const bool loaded = sound->isSoundLoaded();
                state->placeSoundObservedLoaded[index] =
                    state->placeSoundObservedLoaded[index] || loaded;
                allLoaded &= loaded;
                anyPlaying |= loaded && sound->isPlaying();
                state->placeSoundMaximumPositions[index] = std::max(
                    state->placeSoundMaximumPositions[index],
                    sound->getSoundPosition());
            }
            if (allLoaded && state->placeAudioLoadedFrame == 0)
                state->placeAudioLoadedFrame = frameNumber;
            if (anyPlaying) {
                std::array<float, 1600> output = {};
                if (!soundService->getAudioEngine().mix(output))
                    throw std::runtime_error(
                        "selected-place SoundService mixer rejected output");
                for (float sample : output) {
                    state->placeAudioSquaredSampleSum +=
                        static_cast<double>(sample) * static_cast<double>(sample);
                    ++state->placeAudioSampleCount;
                }
            }
        }
    }
    if (RBX::Network::Players* players =
            RBX::ServiceProvider::find<RBX::Network::Players>(state->dataModel.get()))
        if (RBX::Network::Player* localPlayer = players->getLocalPlayer()) {
            localPlayer->maintainOfflineCharacterCamera();
            if (state->verifiesMovement && localPlayer->getCharacter()) {
                RBX::PartInstance* rootPart =
                    localPlayer->getCharacter()->getPrimaryPartSetByUser();
                if (!rootPart)
                    rootPart = localPlayer->getCharacter()
                        ->findFirstChildOfType<RBX::PartInstance>();
                if (rootPart)
                    state->maximumVerificationDisplacement = std::max(
                        state->maximumVerificationDisplacement,
                        (rootPart->getCoordinateFrame().translation -
                            state->verificationStart).magnitude());
            }
        }
    if (state->verifiesCaptureGallery && frameNumber == 80) {
        RBX::CaptureService* captureService =
            RBX::ServiceProvider::find<RBX::CaptureService>(state->dataModel.get());
        if (!captureService)
            throw std::runtime_error("CaptureService was not created for verification");
        captureService->saveScreenshotCapture("rendered-gallery-integration");
    }
    if (state->offlineChatAccessPending && state->experienceChatMounted)
    {
        RBX::Network::Players* players =
            RBX::ServiceProvider::find<RBX::Network::Players>(state->dataModel.get());
        RBX::Network::Player* localPlayer = players ? players->getLocalPlayer() : nullptr;
        if (!localPlayer || localPlayer->getChatAvailabilityStatus() != "Unknown")
            throw std::runtime_error(
                "offline chat access did not remain pending until ExperienceChat mounted");
        localPlayer->setChatAvailabilityStatus("Enabled");
        state->offlineChatAccessPending = false;
        state->offlineChatMountConnection.disconnect();
    }
    if (state->verifiesMovement && frameNumber >= 186 && frameNumber <= 189) {
        std::cout << "camera writes frame=" << frameNumber;
        for (const RBX::Vector3& look : state->cameraChangesThisFrame)
            std::cout << ' ' << look;
        std::cout << " mouse";
        for (const RBX::Vector3& delta : state->mouseChangesThisFrame)
            std::cout << ' ' << delta;
        std::cout << '\n';
    }
    if (state->verifiesMovement && (frameNumber == 185 || frameNumber == 190 ||
                                    frameNumber == 206)) {
        RBX::UserInputService* input =
            RBX::ServiceProvider::find<RBX::UserInputService>(state->dataModel.get());
        std::cout << "camera input frame=" << frameNumber
                  << " mouse-type=" << (input ? input->getMouseType() : -1)
                  << " wrap=" << (input ? input->getMouseWrapMode() : -1)
                  << '\n';
    }
    if (state->verifiesMovement && !state->usesR15Character) {
        RBX::Network::Players* players =
            RBX::ServiceProvider::find<RBX::Network::Players>(state->dataModel.get());
        if (players && frameNumber == 299) {
            boost::shared_ptr<const RBX::Instances> playerInstances = players->getPlayers();
            std::cout << "Players service entries="
                      << (playerInstances ? playerInstances->size() : 0U)
                      << " local=" << (players->getLocalPlayer() ? "yes" : "no");
            if (playerInstances) {
                for (const boost::shared_ptr<RBX::Instance>& playerInstance : *playerInstances)
                    std::cout << " [" << playerInstance->getName() << ': '
                              << playerInstance->getClassName().toString() << ']';
            }
            std::cout << '\n';
        }
        RBX::ModelInstance* character = players && players->getLocalPlayer()
            ? players->getLocalPlayer()->getCharacter()
            : nullptr;
        RBX::Instance* torso = character
            ? character->findFirstChildByName("Torso")
            : nullptr;
        RBX::Motor* shoulder = torso
            ? RBX::Instance::fastDynamicCast<RBX::Motor>(
                  torso->findFirstChildByName("Right Shoulder"))
            : nullptr;
        if (shoulder) {
            const float angle = shoulder->getDesiredAngle();
            if (!state->sampledR6Shoulder) {
                state->minimumR6ShoulderAngle = angle;
                state->maximumR6ShoulderAngle = angle;
                state->sampledR6Shoulder = true;
            } else {
                state->minimumR6ShoulderAngle =
                    std::min(state->minimumR6ShoulderAngle, angle);
                state->maximumR6ShoulderAngle =
                    std::max(state->maximumR6ShoulderAngle, angle);
            }
            // Allow the physical controller to decelerate after the synthetic
            // W release at frame 180. Sampling during that coast mixes the
            // final running swing into the standing interval.
            if (frameNumber >= 270) {
                if (!state->sampledStoppedR6Shoulder) {
                    state->minimumStoppedR6ShoulderAngle = angle;
                    state->maximumStoppedR6ShoulderAngle = angle;
                    state->sampledStoppedR6Shoulder = true;
                } else {
                    state->minimumStoppedR6ShoulderAngle =
                        std::min(state->minimumStoppedR6ShoulderAngle, angle);
                    state->maximumStoppedR6ShoulderAngle =
                        std::max(state->maximumStoppedR6ShoulderAngle, angle);
                }
            }
        }
    }
    if (state->verifiesMovement && state->usesR15Character &&
        frameNumber == 244 &&
        (state->avatarRig == AvatarRigVariant::RthroNormal ||
         state->avatarRig == AvatarRigVariant::RthroSlender)) {
        RBX::Network::Players* players =
            RBX::ServiceProvider::find<RBX::Network::Players>(state->dataModel.get());
        RBX::ModelInstance* character = players && players->getLocalPlayer()
            ? players->getLocalPlayer()->getCharacter() : nullptr;
        RBX::PartInstance* root = character
            ? RBX::Instance::fastDynamicCast<RBX::PartInstance>(
                  character->findFirstChildByName("HumanoidRootPart")) : nullptr;
        RBX::PartInstance* foot = character
            ? RBX::Instance::fastDynamicCast<RBX::PartInstance>(
                  character->findFirstChildByName("LeftFoot")) : nullptr;
        RBX::Humanoid* humanoid = character
            ? RBX::Humanoid::modelIsCharacter(character) : nullptr;
        RBX::DoubleValue* bodyType = humanoid
            ? RBX::Instance::fastDynamicCast<RBX::DoubleValue>(
                  humanoid->findFirstChildByName("BodyTypeScale")) : nullptr;
        RBX::DoubleValue* bodyProportion = humanoid
            ? RBX::Instance::fastDynamicCast<RBX::DoubleValue>(
                  humanoid->findFirstChildByName("BodyProportionScale")) : nullptr;
        const double expectedProportion =
            state->avatarRig == AvatarRigVariant::RthroSlender ? 1.0 : 0.0;
        RBX::Motor6D* rootMotor = nullptr;
        if (character) {
            boost::shared_ptr<const RBX::Instances> descendants =
                character->getDescendants();
            for (const boost::shared_ptr<RBX::Instance>& descendant : *descendants) {
                RBX::Motor6D* motor =
                    RBX::Instance::fastDynamicCast<RBX::Motor6D>(descendant.get());
                if (motor && motor->getName() == "Root") {
                    rootMotor = motor;
                    break;
                }
            }
        }
        const float expectedRootOffset =
            state->avatarRig == AvatarRigVariant::RthroSlender ? -0.917431f : -0.987f;
        const float rootToFoot = root && foot
            ? root->getCoordinateFrame().translation.y -
                foot->getCoordinateFrame().translation.y : 0.0f;
        if (!root || !foot || !humanoid || !bodyType || !bodyProportion ||
            humanoid->getRigType() != RBX::Humanoid::HUMANOID_RIG_TYPE_R15 ||
            std::abs(bodyType->getValue() - 1.0) > 0.0001 ||
            std::abs(bodyProportion->getValue() - expectedProportion) > 0.0001 ||
            !rootMotor ||
            std::abs(rootMotor->getC0().translation.y - expectedRootOffset) > 0.001f ||
            rootToFoot < 3.4f)
            throw std::runtime_error(
                "Rthro runtime did not preserve the selected authored proportions");
        std::cout << "Rthro authored proportions body-type=" << bodyType->getValue()
                  << " body-proportion=" << bodyProportion->getValue()
                  << " root-to-foot=" << rootToFoot
                  << " sole-y=" << foot->getCoordinateFrame().translation.y -
                      foot->getPartSizeXml().y * 0.5f << '\n';
    }
    if (state->verifiesMovement && state->usesR15Character &&
        frameNumber == 245 && !state->r15EmoteInvoked) {
        RBX::Network::Players* players =
            RBX::ServiceProvider::find<RBX::Network::Players>(state->dataModel.get());
        RBX::ModelInstance* character = players && players->getLocalPlayer()
            ? players->getLocalPlayer()->getCharacter()
            : nullptr;
        RBX::Humanoid* humanoid = character
            ? RBX::Humanoid::modelIsCharacter(character)
            : nullptr;
        if (!humanoid)
            throw std::runtime_error(
                "headless R15 verification found no Humanoid for emote observation");
        State* runtimeState = state.get();
        state->r15AnimationPlayedConnection = humanoid->animationPlayedSignal.connect(
            [runtimeState](boost::shared_ptr<RBX::Instance> instance) {
                RBX::AnimationTrack* track =
                    RBX::Instance::fastDynamicCast<RBX::AnimationTrack>(instance.get());
                if (!track)
                    return;
                ++runtimeState->r15AnimationPlayedCount;
                runtimeState->r15AnimationPlayedPriority =
                    static_cast<int>(track->getPriority());
                runtimeState->r15AnimationPlayedName = track->getAnimationName();
                if (track->getAnimationName() == "WaveAnim")
                    runtimeState->r15EmoteTrackObserved = true;
            });
        state->r15EmoteTriggeredConnection = humanoid->emoteTriggeredSignal.connect(
            [runtimeState](bool success, boost::shared_ptr<RBX::Instance> instance) {
                runtimeState->r15EmoteTriggeredObserved = success;
                RBX::AnimationTrack* track =
                    RBX::Instance::fastDynamicCast<RBX::AnimationTrack>(instance.get());
                runtimeState->r15EmoteTriggeredTrackObserved =
                    track && track->getAnimationName() == "WaveAnim";
            });

        state->r15EmoteInvoked = true;
        humanoid->playEmoteAsync("wave",
            [runtimeState](bool accepted) {
                runtimeState->r15EmoteCompleted = true;
                runtimeState->r15EmoteAccepted = accepted;
            },
            [runtimeState](std::string error) {
                runtimeState->r15EmoteCompleted = true;
                runtimeState->r15EmoteError = std::move(error);
            });
    }
    if (state->verifiesMovement && state->usesR15Character &&
        frameNumber >= 246) {
        RBX::Network::Players* players =
            RBX::ServiceProvider::find<RBX::Network::Players>(state->dataModel.get());
        RBX::ModelInstance* character = players && players->getLocalPlayer()
            ? players->getLocalPlayer()->getCharacter()
            : nullptr;
        RBX::Humanoid* humanoid = character
            ? RBX::Humanoid::modelIsCharacter(character)
            : nullptr;
        boost::shared_ptr<const RBX::Reflection::ValueArray> tracks =
            humanoid ? humanoid->getPlayingAnimationTracks()
                     : boost::shared_ptr<const RBX::Reflection::ValueArray>();
        if (tracks) {
            for (const RBX::Reflection::Variant& value : *tracks) {
                if (!value.isType<boost::shared_ptr<RBX::Instance>>())
                    continue;
                boost::shared_ptr<RBX::Instance> instance =
                    value.cast<boost::shared_ptr<RBX::Instance>>();
                RBX::AnimationTrack* track =
                    RBX::Instance::fastDynamicCast<RBX::AnimationTrack>(instance.get());
                if (track && track->getIsPlaying() &&
                    track->getAnimationName() == "WaveAnim")
                    state->r15EmoteTrackObserved = true;
            }
        }
    }
    RBX::Camera* camera = state->dataModel->getWorkspace()->getCamera();
    if (state->verifiesSurfaceTextures || state->verifiesShadowMap) {
        RBX::CoordinateFrame surfaceCamera(RBX::Vector3(18.0f, 20.0f, 18.0f));
        surfaceCamera.lookAt(RBX::Vector3::zero());
        camera->setCameraCoordinateFrame(surfaceCamera);
        camera->setCameraFocus(RBX::CoordinateFrame(RBX::Vector3::zero()));
    } else if (state->verifiesSkybox) {
        RBX::CoordinateFrame skyCamera(RBX::Vector3(0.0f, 12.0f, 0.0f));
        skyCamera.lookAt(RBX::Vector3(40.0f, 5.0f, 0.0f));
        camera->setCameraCoordinateFrame(skyCamera);
        camera->setCameraFocus(RBX::CoordinateFrame(RBX::Vector3(40.0f, 5.0f, 0.0f)));
    }
    if (state->verifiesShadowMap && frameNumber == 55) {
        RBX::PartInstance* baseplate = findPartByName(
            state->dataModel->getWorkspace(), "BasePlate");
        if (!baseplate)
            throw std::runtime_error("ShadowMap proof could not find Baseplate CastShadow control");
        baseplate->setCastShadow(true);
    }
    if (state->verifiesMovement && frameNumber >= 184 && frameNumber <= 242)
        std::cout << "camera look frame=" << frameNumber << ' '
                  << camera->getCameraCoordinateFrame().lookVector() << '\n';
    if (state->verifiesMovement && frameNumber == 184) {
        state->cameraLookBeforeDrag = camera->getCameraCoordinateFrame().lookVector();
        state->sampledCameraBeforeDrag = true;
    } else if (state->verifiesMovement && frameNumber == 242) {
        state->cameraLookAfterDrag = camera->getCameraCoordinateFrame().lookVector();
        state->sampledCameraAfterDrag = true;
    }
    if (state->verifiesMovement && state->sampledCameraBeforeDrag &&
        frameNumber >= 185 && frameNumber <= 241) {
        state->maximumCameraLookDeltaDuringDrag = std::max(
            state->maximumCameraLookDeltaDuringDrag,
            (camera->getCameraCoordinateFrame().lookVector() -
                state->cameraLookBeforeDrag).magnitude());
    }
    state->visualEngine->setCamera(*camera, camera->getCameraFocus().translation);
    RBX::Graphics::AdornRender* adorn = state->visualEngine->getAdorn();
    adorn->preSubmitPass();
    state->dataModel->renderPass3dAdorn(adorn);
    state->dataModel->renderPass2d(adorn, state->dataModel.get());
    adorn->postSubmitPass();
    if (FFlag::UseDynamicTypesetterUTF8)
        state->visualEngine->getGlyphAtlas()->upload();
    state->visualEngine->getTextureCompositor()->update(
        camera->getCameraFocus().translation);
    state->visualEngine->getSceneUpdater()->updatePrepare(
        frameNumber, *state->visualEngine->getUpdateFrustum());
    state->visualEngine->getTextureManager()->processPendingRequests();

    adorn->prepareRenderPass();
    RBX::Graphics::DeviceContext* context = state->device->beginFrame();
    state->visualEngine->getSceneUpdater()->updatePerform();
    state->visualEngine->getTextureCompositor()->render(context);
    state->visualEngine->getViewportRenderer()->render(context);
    RBX::Graphics::Framebuffer* target = state->verificationFramebuffer
        ? state->verificationFramebuffer.get()
        : state->device->getMainFramebuffer();
    if (state->verifiesShadowMap) {
        state->renderSettings->setEnableFRM(false);
        state->renderSettings->setEditQualityLevel(frameNumber < 20
            ? RBX::CRenderSettings::QualityLevel1
            : frameNumber < 40
                ? RBX::CRenderSettings::QualityLevel6
                : RBX::CRenderSettings::QualityLevel15);
        state->visualEngine->getFrameRateManager()->synchronizeQualitySettings();
    }
    if (state->usesDurangoLauncher) {
        if (RBX::PlatformService* platformService =
                RBX::ServiceProvider::find<RBX::PlatformService>(
                    state->dataModel.get())) {
            state->visualEngine->getSceneManager()->setPostProcess(
                platformService->brightness, platformService->contrast,
                platformService->grayscaleLevel, platformService->blurIntensity,
                platformService->tintColor);
            state->launcherPostProcessApplied = true;
            state->launcherPostProcessBrightness = platformService->brightness;
            state->launcherPostProcessContrast = platformService->contrast;
            state->launcherPostProcessGrayscale = platformService->grayscaleLevel;
            state->launcherPostProcessBlur = platformService->blurIntensity;
            state->launcherPostProcessTint = platformService->tintColor;
        }
    }
    state->visualEngine->getSceneManager()->renderScene(
        context, target, state->visualEngine->getCamera(),
        state->logicalWidth, state->logicalHeight);
    if (state->verifiesShadowMap && frameNumber == 10) {
        state->shadowLowQualityVerified =
            state->visualEngine->getSceneManager()->getShadowCascadeCount() == 1;
        if (!state->shadowLowQualityVerified)
            throw std::runtime_error("low-quality ShadowMap did not select one cascade");
    }
    if (state->verifiesShadowMap && frameNumber == 30) {
        state->shadowMediumQualityVerified =
            state->visualEngine->getSceneManager()->getShadowCascadeCount() == 2;
        if (!state->shadowMediumQualityVerified)
            throw std::runtime_error("medium-quality ShadowMap did not select two cascades");
    }
    if (state->verifiesShadowMap && frameNumber == 50)
        state->shadowNoCastBatches =
            state->visualEngine->getRenderStats()->passShadow.batches;
    state->device->endFrame();
    adorn->finishRenderPass();
    if (state->verifiesDurangoLauncher &&
        (frameNumber == kLauncherFirstSettledFrame ||
         frameNumber == kLauncherSecondSettledFrame))
    {
        if (camera->getCameraType() != RBX::Camera::LOCKED_CAMERA)
            throw std::runtime_error(
                "Durango CameraManager lost Scriptable ownership during temporal proof");
        std::vector<std::uint8_t>& capture =
            frameNumber == kLauncherFirstSettledFrame
            ? state->launcherFirstFramePixels
            : state->launcherSecondFramePixels;
        capture.resize(
            static_cast<std::size_t>(state->width) * state->height * 4U);
        target->download(capture.data(), static_cast<unsigned int>(capture.size()));
        if (frameNumber == kLauncherFirstSettledFrame)
        {
            state->launcherFirstFrameCaptured = true;
            state->launcherFirstFrameNumber = frameNumber;
            state->launcherFirstCameraFrame =
                camera->getCameraCoordinateFrame();
        }
        else
        {
            state->launcherSecondFrameCaptured = true;
            state->launcherSecondFrameNumber = frameNumber;
            state->launcherSecondCameraFrame =
                camera->getCameraCoordinateFrame();
        }
    }
    if (state->verifiesShadowMap && frameNumber == 80) {
        const RBX::RenderStats* stats = state->visualEngine->getRenderStats();
        RBX::Graphics::SceneManager* scene =
            state->visualEngine->getSceneManager();
        state->shadowEnabledPixels.resize(
            static_cast<std::size_t>(state->width) * state->height * 4U);
        target->download(state->shadowEnabledPixels.data(),
            static_cast<unsigned int>(state->shadowEnabledPixels.size()));
        state->shadowCasterBatches = stats->passShadow.batches;
        if (!scene->isShadowMapEnabled() || stats->passShadow.batches < 12)
            throw std::runtime_error(
                "ShadowMap proof did not submit character, BasePart, and terrain caster passes");
        const RBX::Graphics::shared_ptr<RBX::Graphics::Texture>& shadowTexture =
            scene->getShadowMap().getTexture();
        state->shadowMapWidth = shadowTexture ? shadowTexture->getWidth() : 0U;
        state->shadowCascadeCount = scene->getShadowCascadeCount();
        state->shadowCascadeInfo = scene->getShadowCascadeInfo();
        if (!shadowTexture || state->shadowMapWidth < 256 ||
            state->shadowMapWidth != shadowTexture->getHeight())
            throw std::runtime_error(
                "ShadowMap caster pass did not publish a valid square render target");
        if (state->shadowCascadeCount != 3 ||
            state->shadowCascadeInfo.x <= 0 ||
            state->shadowCascadeInfo.y <= state->shadowCascadeInfo.x)
            throw std::runtime_error(RBX::format(
                "high-quality ShadowMap did not publish three ordered atlas cascades (count=%u splits=%f,%f)",
                state->shadowCascadeCount, state->shadowCascadeInfo.x,
                state->shadowCascadeInfo.y));

        RBX::Lighting* lighting =
            RBX::ServiceProvider::create<RBX::Lighting>(state->dataModel.get());
        lighting->setGlobalShadows(false);
        configureLighting(*state->visualEngine, *lighting);
        RBX::Graphics::DeviceContext* disabledContext = state->device->beginFrame();
        scene->renderScene(disabledContext, target,
            state->visualEngine->getCamera(), state->logicalWidth,
            state->logicalHeight);
        state->device->endFrame();
        state->shadowDisabledPixels.resize(state->shadowEnabledPixels.size());
        target->download(state->shadowDisabledPixels.data(),
            static_cast<unsigned int>(state->shadowDisabledPixels.size()));
        if (scene->isShadowMapEnabled() ||
            state->visualEngine->getRenderStats()->passShadow.batches != 0)
            throw std::runtime_error(
                "GlobalShadows=false did not disable the ShadowMap caster pass");
        for (std::size_t index = 0; index < state->shadowDisabledPixels.size();
             index += 4U) {
            const int enabled = state->shadowEnabledPixels[index] +
                state->shadowEnabledPixels[index + 1U] +
                state->shadowEnabledPixels[index + 2U];
            const int disabled = state->shadowDisabledPixels[index] +
                state->shadowDisabledPixels[index + 1U] +
                state->shadowDisabledPixels[index + 2U];
            state->shadowDarkenedPixels += enabled + 24 < disabled;
        }
        lighting->setGlobalShadows(true);
        configureLighting(*state->visualEngine, *lighting);
        if (state->shadowDarkenedPixels < 64U)
            throw std::runtime_error(
                "ShadowMap caster pass ran but did not darken visible scene pixels");
    }
    if (state->verifiesSafeArea &&
        (frameNumber == 120UL || frameNumber == 180UL)) {
        const RBX::Vector4 expected = frameNumber == 120UL
            ? RBX::Vector4(24.0f, 18.0f, 32.0f, 28.0f)
            : RBX::Vector4(48.0f, 10.0f, 12.0f, 34.0f);
        const RBX::Vector2 safePosition =
            state->safeAreaFrame->getAbsolutePosition();
        const RBX::Vector2 safeSize = state->safeAreaFrame->getAbsoluteSize();
        const RBX::Vector2 safeScreenPosition =
            state->safeAreaScreen->getAbsolutePosition();
        const RBX::Vector2 safeScreenSize =
            state->safeAreaScreen->getAbsoluteSize();
        const RBX::Vector2 fullPosition =
            state->fullViewportFrame->getAbsolutePosition();
        const RBX::Vector2 fullSize =
            state->fullViewportFrame->getAbsoluteSize();
        RBX::GuiService* guiService =
            RBX::ServiceProvider::find<RBX::GuiService>(state->dataModel.get());
        const RBX::Vector2 safeViewport = guiService
            ? guiService->getHardwareSafeViewport() : RBX::Vector2::zero();
        const float expectedWidth = static_cast<float>(state->logicalWidth) -
            expected.x - expected.z;
        const float expectedHeight = static_cast<float>(state->logicalHeight) -
            expected.y - expected.w;
        const auto near = [](float left, float right) {
            return std::abs(left - right) < 0.01f;
        };
        if (!near(safePosition.x, expected.x) ||
            !near(safePosition.y, expected.y) ||
            !near(safeSize.x, expectedWidth) ||
            !near(safeSize.y, expectedHeight) ||
            !near(safeViewport.x, expectedWidth) ||
            !near(safeViewport.y, expectedHeight) ||
            !near(fullPosition.x, 0.0f) ||
            !near(fullPosition.y, 0.0f) ||
            !near(fullSize.x, static_cast<float>(state->logicalWidth)) ||
            !near(fullSize.y, static_cast<float>(state->logicalHeight)))
            throw std::runtime_error(RBX::format(
                "host safe-area insets did not reach ScreenGui layout (safe=%f,%f,%f,%f screen=%f,%f,%f,%f full=%f,%f,%f,%f viewport=%f,%f expected=%f,%f,%f,%f)",
                safePosition.x, safePosition.y, safeSize.x, safeSize.y,
                safeScreenPosition.x, safeScreenPosition.y,
                safeScreenSize.x, safeScreenSize.y,
                fullPosition.x, fullPosition.y, fullSize.x, fullSize.y,
                safeViewport.x, safeViewport.y, expected.x, expected.y,
                expectedWidth, expectedHeight));
        std::cout << "safe-area frame=" << frameNumber << " inset="
                  << expected.x << ',' << expected.y << ',' << expected.z
                  << ',' << expected.w << " viewport=" << safePosition.x
                  << ',' << safePosition.y << ',' << safeSize.x << ','
                  << safeSize.y << '\n';
    }
    if (state->verifiesOrientation &&
        (frameNumber == 120UL || frameNumber == 180UL ||
         frameNumber == 240UL)) {
        const bool portrait = frameNumber == 180UL;
        const bool landscapeRight = frameNumber == 240UL;
        const RBX::Vector2 expectedSize = portrait
            ? RBX::Vector2(720.0f, 1280.0f)
            : RBX::Vector2(1280.0f, 720.0f);
        RBX::Network::Players* players =
            RBX::ServiceProvider::find<RBX::Network::Players>(state->dataModel.get());
        RBX::Network::Player* localPlayer = players ? players->getLocalPlayer() : nullptr;
        RBX::PlayerGui* playerGui = localPlayer
            ? localPlayer->findFirstChildOfType<RBX::PlayerGui>() : nullptr;
        RBX::GuiService* guiService =
            RBX::ServiceProvider::find<RBX::GuiService>(state->dataModel.get());
        RBX::CoreGuiService* coreGui =
            RBX::ServiceProvider::find<RBX::CoreGuiService>(state->dataModel.get());
        const RBX::Vector2 layerSize = state->orientationScreen->getAbsoluteSize();
        const RBX::Vector2 frameSize = state->orientationFrame->getAbsoluteSize();
        const RBX::Vector2 screenResolution = guiService
            ? guiService->getScreenResolution() : RBX::Vector2::zero();
        const RBX::Vector2 cameraViewport = camera
            ? RBX::Vector2(static_cast<float>(camera->getViewport().x),
                  static_cast<float>(camera->getViewport().y))
            : RBX::Vector2::zero();
        const RBX::Enums::ScreenOrientation expectedOrientation = portrait
            ? RBX::Enums::SCREEN_ORIENTATION_PORTRAIT
            : landscapeRight
                ? RBX::Enums::SCREEN_ORIENTATION_LANDSCAPE_RIGHT
                : RBX::Enums::SCREEN_ORIENTATION_LANDSCAPE_LEFT;
        const unsigned int expectedChangeCount = frameNumber == 120UL
            ? 0U : frameNumber == 180UL ? 1U : 2U;
        if (!playerGui ||
            playerGui->getCurrentScreenOrientation() != expectedOrientation ||
            state->orientationScreen->getParent() != coreGui ||
            layerSize != expectedSize || frameSize != expectedSize ||
            screenResolution != expectedSize ||
            cameraViewport != expectedSize ||
            state->orientationChangeCount != expectedChangeCount)
            throw std::runtime_error(RBX::format(
                "display orientation did not reach PlayerGui and viewport layout (frame=%lu orientation=%d events=%u parented=%d layer-size=%f,%f frame-size=%f,%f screen=%f,%f camera=%f,%f)",
                frameNumber,
                playerGui ? static_cast<int>(
                    playerGui->getCurrentScreenOrientation()) : -1,
                state->orientationChangeCount,
                state->orientationScreen->getParent() == coreGui,
                layerSize.x, layerSize.y,
                frameSize.x, frameSize.y,
                screenResolution.x, screenResolution.y,
                cameraViewport.x, cameraViewport.y));
        std::cout << "orientation frame=" << frameNumber << " value="
                  << static_cast<int>(expectedOrientation) << " viewport="
                  << expectedSize.x << ',' << expectedSize.y << " events="
                  << state->orientationChangeCount << '\n';
    }
    if (state->screenshotRequested) {
        state->screenshotRequested = false;
        std::string screenshotPath;
        RBX::Graphics::saveFramebufferScreenshot(*target, screenshotPath);
        state->dataModel->screenshotReadySignal(screenshotPath);
    }
    if (state->verifiesCaptureGallery && frameNumber == 120) {
        if (!state->captureSaved ||
            !std::filesystem::is_regular_file(state->verificationCapturePath))
            throw std::runtime_error(
                "rendered screenshot did not reach the CaptureService gallery");

        RBX::CaptureService* captureService =
            RBX::ServiceProvider::find<RBX::CaptureService>(state->dataModel.get());
        bool sizeReturned = false;
        RBX::Vector2 captureSize;
        captureService->getCaptureSizeAsync(
            RBX::Content::fromUri(
                "file://" + state->verificationCapturePath.string()),
            [&](RBX::Vector2 value) {
                captureSize = value;
                sizeReturned = true;
            },
            [](std::string error) {
                throw std::runtime_error("capture size failed: " + error);
            });
        if (!sizeReturned ||
            captureSize != RBX::Vector2(
                static_cast<float>(target->getWidth()),
                static_cast<float>(target->getHeight())))
            throw std::runtime_error("saved capture dimensions do not match the renderer");

        boost::shared_ptr<const RBX::Reflection::ValueArray> captures =
            captureService->retrieveCaptures();
        bool retrievedCreatedCapture = false;
        for (const RBX::Reflection::Variant& value : *captures) {
            if (!value.isType<boost::shared_ptr<const RBX::Reflection::ValueTable> >())
                continue;
            boost::shared_ptr<const RBX::Reflection::ValueTable> info =
                value.cast<boost::shared_ptr<const RBX::Reflection::ValueTable> >();
            const auto path = info->find("filePath");
            if (path != info->end() && path->second.isType<std::string>() &&
                path->second.cast<std::string>() ==
                    state->verificationCapturePath.string())
                retrievedCreatedCapture = true;
        }
        if (!retrievedCreatedCapture)
            throw std::runtime_error(
                "RetrieveCaptures omitted the rendered verification capture");

        boost::shared_ptr<RBX::Reflection::DescribedBase> captureObject =
            captureService->getScreenshotCaptureObject(
                state->verificationCapturePath.string());
        boost::shared_ptr<RBX::Capture> capture =
            RBX::Reflection::DescribedBase::fastSharedDynamicCast<RBX::Capture>(
                captureObject);
        if (!capture || capture->getCaptureType() !=
                RBX::Enums::CAPTURE_TYPE_SCREENSHOT ||
            capture->getFilePath() != state->verificationCapturePath)
            throw std::runtime_error(
                "GetScreenshotCaptureObject returned incorrect metadata");

        boost::shared_ptr<RBX::Reflection::ValueArray> paths(
            new RBX::Reflection::ValueArray());
        paths->push_back(RBX::Reflection::Variant(
            state->verificationCapturePath.string()));
        bool storageReturned = false;
        long long storedBytes = 0;
        captureService->getCaptureStorageSizeAsync(
            paths,
            [&](long long value) {
                storedBytes = value;
                storageReturned = true;
            },
            [](std::string error) {
                throw std::runtime_error("capture storage size failed: " + error);
            });
        if (!storageReturned || storedBytes <= 24)
            throw std::runtime_error("saved capture has no valid PNG payload");

        state->verificationCaptureSize = captureSize;
        state->verificationCaptureBytes = storedBytes;
        state->captureVerified = true;
        std::cout << "CaptureService rendered gallery persistence="
                  << static_cast<unsigned int>(captureSize.x) << 'x'
                  << static_cast<unsigned int>(captureSize.y)
                  << " bytes=" << storedBytes << " ui-proof=pending\n";
    }
    if (frameNumber == 299 || frameNumber == 319 || frameNumber == 329 ||
        frameNumber == 349 || frameNumber == 355 || frameNumber == 369 ||
        frameNumber == 374 || frameNumber == 399 || frameNumber == 439 ||
        (state->verifiesRespawn && frameNumber == 1799)) {
        const bool characterCamera =
            camera->getCameraType() == RBX::Camera::CUSTOM_CAMERA &&
            camera->getCameraSubject() != nullptr;
        // A place-owned first-person controller legitimately takes complete
        // camera authority with CameraType.Scriptable and may intentionally
        // omit CameraSubject. The selected Backrooms fixture does exactly
        // this; rejecting it would make the verifier enforce Roblox's stock
        // PlayerModule over a real developer camera implementation.
        const bool scriptableCamera =
            camera->getCameraType() == RBX::Camera::LOCKED_CAMERA &&
            camera->getCameraCoordinateFrame().translation.isFinite() &&
            camera->getCameraFocus().translation.isFinite();
        if ((state->usesR15Character || state->verifiesMovement) &&
            !characterCamera && !scriptableCamera)
            throw std::runtime_error(
                "local character did not establish the gameplay camera");
        const RBX::RenderStats* stats = state->visualEngine->getRenderStats();
        std::cout << "scene batches=" << stats->passScene.batches
                  << " faces=" << stats->passScene.faces
                  << " vertices=" << stats->passScene.vertices << '\n';
        const RBX::CoordinateFrame cameraFrame = camera->getCameraCoordinateFrame();
        RBX::Instance* cameraSubjectInstance =
            dynamic_cast<RBX::Instance*>(camera->getCameraSubject());
        std::cout << (state->usesDurangoLauncher
                          ? "shell camera type="
                          : "gameplay camera type=")
                  << camera->getCameraType()
                  << " position=" << cameraFrame.translation
                  << " focus=" << camera->getCameraFocus().translation
                  << " look=" << cameraFrame.lookVector()
                  << " subject=" << (cameraSubjectInstance
                      ? cameraSubjectInstance->getFullName()
                      : (camera->getCameraSubject() ? "<non-instance>" : "<none>"))
                  << '\n';
        std::cout << "render passes scene=" << stats->passScene.batches
                  << " ui=" << stats->passUI.batches
                  << " adorn3d=" << stats->pass3DAdorns.batches
                  << " total=" << stats->passTotal.batches << '\n';
        for (const char* flagName : {"EnableInGameMenuChrome", "ReverseUnibar",
                 "UnibarMenuOpenHamburger", "TokenizeUnibarConstantsWithStyleProvider",
                 "UseNewUnibarIcon", "PlayerListReskin2",
                 "VoiceChatServiceManagerUseAvatarChat",
                 "TopBarSignalizeSetCores", "ReplacePlayerIconRoduxWithSignal",
                 "PlayerListPersistVisibility"}) {
            std::string flagValue;
            std::cout << "effective flag " << flagName << '='
                      << (FLog::GetValue(flagName, flagValue) ? flagValue : "<undefined>")
                      << '\n';
        }
        RBX::Network::Players* players =
            RBX::ServiceProvider::find<RBX::Network::Players>(state->dataModel.get());
        RBX::ModelInstance* character = players && players->getLocalPlayer()
            ? players->getLocalPlayer()->getCharacter()
            : nullptr;
        if (players && players->getLocalPlayer()) {
            RBX::Network::Player* player = players->getLocalPlayer();
            std::cout << "local Player children=" << player->numChildren();
            for (std::size_t index = 0; index < player->numChildren(); ++index)
            {
                RBX::Instance* child = player->getChild(index);
                std::cout << (index == 0 ? " [" : ", ") << child->getName();
                if (child->getName() == "PlayerScripts") {
                    std::cout << '{';
                    for (std::size_t nested = 0; nested < child->numChildren(); ++nested)
                        std::cout << (nested == 0 ? "" : ",")
                                  << child->getChild(nested)->getName();
                    std::cout << '}';
                }
            }
            if (player->numChildren() != 0)
                std::cout << ']';
            std::cout << '\n';
        }
        std::cout << "character children=" << (character ? character->numChildren() : 0U);
        RBX::PartInstance* verificationPart = nullptr;
        RBX::PartInstance* characterHead = nullptr;
        RBX::MeshContentProvider* meshContentProvider =
            RBX::ServiceProvider::find<RBX::MeshContentProvider>(state->dataModel.get());
        std::size_t meshPartCount = 0;
        std::size_t loadedMeshPartCount = 0;
        std::size_t loadedMeshVertexCount = 0;
        std::size_t loadedMeshFaceCount = 0;
        if (character) {
            for (std::size_t index = 0; index < character->numChildren(); ++index) {
                RBX::Instance* child = character->getChild(index);
                if (RBX::MeshPart* meshPart =
                        RBX::Instance::fastDynamicCast<RBX::MeshPart>(child)) {
                    ++meshPartCount;
                    const boost::shared_ptr<void> meshData = meshContentProvider
                        ? meshContentProvider->blockingRequestContent(
                              meshPart->getMeshId(), true)
                        : boost::shared_ptr<void>();
                    if (meshData) {
                        const boost::shared_ptr<RBX::FileMeshData> parsedMesh =
                            boost::static_pointer_cast<RBX::FileMeshData>(meshData);
                        ++loadedMeshPartCount;
                        loadedMeshVertexCount += parsedMesh->vnts.size();
                        loadedMeshFaceCount += parsedMesh->faces.size();
                    }
                }
                if (RBX::PartInstance* part =
                        RBX::Instance::fastDynamicCast<RBX::PartInstance>(child)) {
                    if (!verificationPart || part->getName() == "HumanoidRootPart")
                        verificationPart = part;
                    std::cout << " [" << part->getName()
                              << " p=" << part->getCoordinateFrame().translation
                              << " a=" << part->alpha() << ']';
                    if (part->getName() == "Head") {
                        characterHead = part;
                        std::cout << " cookie=" << part->getCookie();
                        for (std::size_t nested = 0; nested < part->numChildren(); ++nested) {
                            if (RBX::Decal* decal =
                                    RBX::Instance::fastDynamicCast<RBX::Decal>(
                                        part->getChild(nested))) {
                                std::cout << " decal=" << decal->getName()
                                          << ':' << decal->getTexture().toString()
                                          << " file="
                                          << RBX::ContentProvider::findAsset(
                                              decal->getTexture());
                            }
                        }
                    }
                }
            }
        }
        std::cout << '\n';
        if (scriptableCamera && state->verifiesMovement) {
            if (!characterHead)
                throw std::runtime_error(
                    "place-owned first-person camera has no character Head");
            const float cameraHeadDistance =
                (cameraFrame.translation -
                    characterHead->getCoordinateFrame().translation).magnitude();
            std::cout << "first-person camera head-distance="
                      << cameraHeadDistance << '\n';
            if (!std::isfinite(cameraHeadDistance) || cameraHeadDistance > 3.0f)
                throw std::runtime_error(
                    "place-owned Scriptable camera is not attached to the local player view");
        }
        if (state->usesR15Character)
            std::cout << "R15 MeshParts=" << meshPartCount
                      << " loaded=" << loadedMeshPartCount
                      << " mesh-vertices=" << loadedMeshVertexCount
                      << " mesh-faces=" << loadedMeshFaceCount
                      << " provider=" << (meshContentProvider ? "yes" : "no") << '\n';
        const std::size_t expectedMeshPartCount =
            state->avatarRig == AvatarRigVariant::R15 ? 14U : 15U;
        const bool completeR15MeshGeometry =
            meshPartCount == expectedMeshPartCount &&
            loadedMeshPartCount == meshPartCount &&
            loadedMeshVertexCount > 0 && loadedMeshFaceCount > 0 &&
            (scriptableCamera ||
                (stats->passScene.vertices >= loadedMeshVertexCount &&
                 stats->passScene.faces >= loadedMeshFaceCount));
        if (state->verifiesMovement && state->usesR15Character) {
            if (state->verifiesChromeLeaderboard &&
                !state->verifiesChromeLeaderboardTouch) {
                state->verifiedChromeR15MeshGeometry |=
                    completeR15MeshGeometry;
                if (frameNumber == 399 &&
                    !state->verifiedChromeR15MeshGeometry)
                    throw std::runtime_error(
                        "headless R15 verification never rendered complete MeshPart geometry");
            } else if (!state->verifiesChromeLeaderboardTouch &&
                       !completeR15MeshGeometry) {
                throw std::runtime_error(
                    "headless R15 verification found incomplete rendered MeshPart geometry");
            }
        }
        if (state->verifiesMovement &&
            state->avatarRig == AvatarRigVariant::R15Plus && character) {
            boost::shared_ptr<const RBX::Instances> descendants =
                character->getDescendants();
            std::set<std::pair<RBX::PartInstance*, RBX::PartInstance*> >
                collisionPairs;
            RBX::NoCollisionConstraint* liveConstraint = nullptr;
            for (const boost::shared_ptr<RBX::Instance>& descendant : *descendants) {
                RBX::NoCollisionConstraint* constraint =
                    RBX::Instance::fastDynamicCast<RBX::NoCollisionConstraint>(
                        descendant.get());
                if (!constraint)
                    continue;
                RBX::PartInstance* first = constraint->getPart0();
                RBX::PartInstance* second = constraint->getPart1();
                if (!constraint->getEnabled() || !first || !second || first == second)
                    throw std::runtime_error(
                        "R15-plus NoCollisionConstraint lost its authentic endpoints");
                if (std::less<RBX::PartInstance*>()(second, first))
                    std::swap(first, second);
                if (!collisionPairs.emplace(first, second).second ||
                    first->getPartPrimitive()->canCollideWith(
                        *second->getPartPrimitive()))
                    throw std::runtime_error(
                        "R15-plus NoCollisionConstraint did not suppress its unique collision pair");
                if (!liveConstraint)
                    liveConstraint = constraint;
            }
            if (collisionPairs.size() != 19 || !liveConstraint)
                throw std::runtime_error(
                    "R15-plus runtime did not bind all 19 exact collision exclusions");
            RBX::PartInstance* first = liveConstraint->getPart0();
            RBX::PartInstance* second = liveConstraint->getPart1();
            liveConstraint->setEnabled(false);
            const bool collisionRestored = first->getPartPrimitive()->canCollideWith(
                *second->getPartPrimitive());
            liveConstraint->setEnabled(true);
            if (!collisionRestored ||
                first->getPartPrimitive()->canCollideWith(
                    *second->getPartPrimitive()))
                throw std::runtime_error(
                    "R15-plus collision exclusion did not respond to Enabled");
            std::cout << "R15-plus NoCollisionConstraint pairs="
                      << collisionPairs.size() << " live-toggle=1\n";
        }
        RBX::Humanoid* humanoid = character
            ? RBX::Humanoid::modelIsCharacter(character)
            : nullptr;
        const boost::shared_ptr<const RBX::Reflection::ValueArray> playingTracks =
            humanoid ? humanoid->getPlayingAnimationTracks()
                     : boost::shared_ptr<const RBX::Reflection::ValueArray>();
        std::cout << "playing animation tracks="
                  << (playingTracks ? playingTracks->size() : 0U) << '\n';
        if (state->verifiesMovement && state->usesR15Character &&
            (!playingTracks || playingTracks->empty()))
            throw std::runtime_error(
                "headless R15 verification found no playing animation track");
        if (state->verifiesMovement && state->usesR15Character) {
            std::cout << "R15 authentic wave emote invoked="
                      << state->r15EmoteInvoked
                      << " completed=" << state->r15EmoteCompleted
                      << " accepted=" << state->r15EmoteAccepted
                      << " wave-track=" << state->r15EmoteTrackObserved
                      << " emote-event=" << state->r15EmoteTriggeredObserved
                      << " emote-event-track=" << state->r15EmoteTriggeredTrackObserved
                      << " played-events=" << state->r15AnimationPlayedCount
                      << " played-priority=" << state->r15AnimationPlayedPriority
                      << " played-name=" << state->r15AnimationPlayedName;
            if (!state->r15EmoteError.empty())
                std::cout << " error=" << state->r15EmoteError;
            std::cout << '\n';
            if (!state->r15EmoteInvoked || !state->r15EmoteCompleted ||
                !state->r15EmoteAccepted || !state->r15EmoteError.empty() ||
                !state->r15EmoteTrackObserved ||
                !state->r15EmoteTriggeredObserved ||
                !state->r15EmoteTriggeredTrackObserved)
                throw std::runtime_error(
                    "headless R15 verification did not execute the package-backed wave emote");
        }
        if (state->verifiesMovement && !state->usesR15Character) {
            const float shoulderRange = state->maximumR6ShoulderAngle -
                state->minimumR6ShoulderAngle;
            std::cout << "R6 Right Shoulder desired-angle range="
                      << shoulderRange << '\n';
            if (!state->sampledR6Shoulder || shoulderRange < 0.25F)
                throw std::runtime_error(
                    "headless R6 verification found no procedural limb animation");
            const float stoppedShoulderRange =
                state->maximumStoppedR6ShoulderAngle -
                state->minimumStoppedR6ShoulderAngle;
            std::cout << "R6 stopped Right Shoulder desired-angle range="
                      << stoppedShoulderRange << '\n';
            if (!state->sampledStoppedR6Shoulder || stoppedShoulderRange > 0.3F)
                throw std::runtime_error(
                    "headless R6 verification did not transition from walking to idle");
        }
        if (state->verifiesMovement) {
            std::cout << "maximum character displacement="
                      << state->maximumVerificationDisplacement << '\n';
        }
        // The selected first-person fixture constrains the character inside
        // narrow scripted corridors; one stud of observed root travel is a
        // meaningful native input/simulation proof there.  The stock camera
        // fixture remains subject to the stronger five-stud traversal check.
        const float requiredMovementDistance = scriptableCamera ? 1.0F : 5.0F;
        if (state->verifiesMovement &&
            (!verificationPart ||
             state->maximumVerificationDisplacement < requiredMovementDistance)) {
            throw std::runtime_error(
                "headless input verification did not move the playable character");
        }
        if (state->verifiesMovement) {
            const float cameraRotation =
                (state->cameraLookAfterDrag - state->cameraLookBeforeDrag).magnitude();
            std::cout << "right-drag camera look-vector delta="
                      << cameraRotation << " peak="
                      << state->maximumCameraLookDeltaDuringDrag << '\n';
            RBX::CoreGuiService* diagnosticCoreGui =
                RBX::ServiceProvider::find<RBX::CoreGuiService>(state->dataModel.get());
            if (diagnosticCoreGui) {
                if (state->verifiesExperienceChat) {
                    RBX::TextChatService* textChatService =
                        RBX::ServiceProvider::find<RBX::TextChatService>(
                            state->dataModel.get());
                    RBX::ChatInputBarConfiguration* inputConfiguration =
                        textChatService
                        ? RBX::Instance::fastDynamicCast<
                              RBX::ChatInputBarConfiguration>(
                              textChatService->findFirstChildByName(
                                  "ChatInputBarConfiguration"))
                        : nullptr;
                    std::cout << "ChatInputBarConfiguration enabled="
                              << (inputConfiguration
                                      ? inputConfiguration->getEnabled()
                                      : false)
                              << " textbox=";
                    if (inputConfiguration && inputConfiguration->getTextBox())
                        std::cout << inputConfiguration->getTextBox()->getFullName();
                    else
                        std::cout << "<none>";
                    std::cout << " target-channel="
                              << (inputConfiguration &&
                                      inputConfiguration->getTargetTextChannel()
                                      ? "set"
                                      : "<none>")
                              << " focused="
                              << (inputConfiguration
                                      ? inputConfiguration->getIsFocused()
                                      : false)
                              << '\n';
                }
                if (RBX::GuiService* guiService = RBX::ServiceProvider::find<RBX::GuiService>(
                        state->dataModel.get()))
                    std::cout << "CoreGui selected="
                              << (guiService->getSelectedGuiObject()
                                      ? guiService->getSelectedGuiObject()->getFullName()
                                      : std::string("<none>"))
                              << " selection-image="
                              << (diagnosticCoreGui->getSelectionImageObject()
                                      ? diagnosticCoreGui->getSelectionImageObject()->getFullName()
                                      : std::string("<default>")) << '\n';
                boost::shared_ptr<const RBX::Instances> descendants =
                    diagnosticCoreGui->getDescendants();
                std::size_t visibleGuiCount = 0;
                bool visiblePeopleCardTooltip = false;
                bool visiblePeopleCardThumbnail = false;
                bool transparentPeopleCardThumbnail = false;
                bool reportChromeIconSeen = false;
                bool reportChromeIconTransparent = false;
                bool avatarChromeIconSeen = false;
                bool avatarChromeIconTransparent = false;
                bool musicChromeEntryMounted = false;
                bool experienceChatEnabled = false;
                bool visibleExperienceChatSurface = false;
                bool visibleExperienceChatInput = false;
                bool visibleExperienceChatMessagePrefix = false;
                bool visibleExperienceChatMessageBody = false;
                bool boundedReportMenu = false;
                bool selectedReportTab = false;
                bool visibleReportTextBox = false;
                bool visibleReportTargetPrompt = false;
                bool visibleReportReasonPrompt = false;
                bool visibleChromeDuringReport = false;
                bool boundedRespawnMenu = false;
                bool visibleRespawnPrompt = false;
                bool visibleRespawnAction = false;
                bool visibleDontRespawnAction = false;
                bool visibleChromeDuringRespawn = false;
                for (const boost::shared_ptr<RBX::Instance>& descendant : *descendants) {
                    const std::string descendantFullName = descendant->getFullName();
                    if (RBX::ScreenGui* screen =
                            RBX::Instance::fastDynamicCast<RBX::ScreenGui>(descendant.get())) {
                        const RBX::Vector2 position = screen->getAbsolutePosition();
                        const RBX::Rect2D viewport = screen->getViewport();
                        const RBX::Rect2D internalRect = screen->getRect2DFloat();
                        std::cout << "CoreGui ScreenGui=" << screen->getFullName()
                                  << " enabled=" << screen->getEnabled()
                                  << " insets=" << static_cast<int>(screen->getScreenInsets())
                                  << " ignoreInset=" << screen->getIgnoreGuiInset()
                                  << " position=" << position.x << ',' << position.y
                                  << " internal=" << internalRect.x0() << ',' << internalRect.y0()
                                  << " viewport=" << viewport.x0() << ',' << viewport.y0()
                                  << ',' << viewport.width() << ',' << viewport.height() << '\n';
                        if (screen->getFullName() == "CoreGui.ExperienceChat")
                            experienceChatEnabled = screen->getEnabled();
                    }
                    RBX::GuiObject* gui = RBX::Instance::fastDynamicCast<RBX::GuiObject>(
                        descendant.get());
                    musicChromeEntryMounted |=
                        descendantFullName.find(".music_entrypoint") != std::string::npos;
                    if (descendantFullName.starts_with("CoreGui.TopBarApp"))
                        if (RBX::TextLabel* chromeLabel =
                                RBX::Instance::fastDynamicCast<RBX::TextLabel>(
                                    descendant.get()))
                            musicChromeEntryMounted |= chromeLabel->getText() == "Music";
                    if (state->verifiesExperienceChat &&
                        descendantFullName.starts_with("CoreGui.ExperienceChat")) {
                        std::cout << "ExperienceChat node=" << descendantFullName
                                  << " class=" << descendant->getClassName().toString();
                        if (gui)
                            std::cout << " visible=" << gui->getVisible()
                                      << " current=" << gui->isCurrentlyVisible()
                                      << " position=" << gui->getAbsolutePosition().x << ','
                                      << gui->getAbsolutePosition().y
                                      << " size=" << gui->getAbsoluteSize().x << ','
                                      << gui->getAbsoluteSize().y
                                      << " authored-size=" << gui->getSize().x.scale << ','
                                      << gui->getSize().x.offset << ';'
                                      << gui->getSize().y.scale << ','
                                      << gui->getSize().y.offset
                                      << " layout-order=" << gui->getLayoutOrder()
                                      << " automatic-size="
                                      << static_cast<int>(gui->getAutomaticSize());
                        if (RBX::UIListLayout* list =
                                RBX::Instance::fastDynamicCast<RBX::UIListLayout>(
                                    descendant.get())) {
                            const RBX::UDim padding = list->getPadding();
                            const RBX::Vector2 content = list->getAbsoluteContentSize();
                            std::cout << " fill="
                                      << static_cast<int>(list->getFillDirection())
                                      << " sort="
                                      << static_cast<int>(list->getSortOrder())
                                      << " padding=" << padding.scale << ','
                                      << padding.offset
                                      << " hflex="
                                      << static_cast<int>(list->getHorizontalFlex())
                                      << " vflex="
                                      << static_cast<int>(list->getVerticalFlex())
                                      << " content=" << content.x << ',' << content.y;
                        }
                        if (RBX::TextBox* textBox =
                                RBX::Instance::fastDynamicCast<RBX::TextBox>(
                                    descendant.get())) {
                            std::cout << " text='" << textBox->getText() << "'"
                                      << " focused=" << textBox->getFocused()
                                      << " cursor=" << textBox->getCursorPosition()
                                      << " selection=" << textBox->getSelectionStart();
                        }
                        if (RBX::TextLabel* label =
                                RBX::Instance::fastDynamicCast<RBX::TextLabel>(
                                    descendant.get())) {
                            std::cout << " text='" << label->getText() << "'";
                            if (descendantFullName.ends_with(
                                    ".TextMessage.PrefixText"))
                                visibleExperienceChatMessagePrefix =
                                    label->isCurrentlyVisible() &&
                                    label->getText().find("Player:") !=
                                        std::string::npos;
                            if (descendantFullName.ends_with(
                                    ".TextMessage.BodyText"))
                                visibleExperienceChatMessageBody =
                                    label->isCurrentlyVisible() &&
                                    label->getText().ends_with(" hi");
                        }
                        std::cout << '\n';
                    }
                    if (!gui || !gui->getVisible() || gui->getAbsoluteSize().x <= 0.0f ||
                        gui->getAbsoluteSize().y <= 0.0f)
                        continue;
                    ++visibleGuiCount;
                    const std::string fullName = gui->getFullName();
                    if (state->verifiesReport) {
                        const RBX::Vector2 position = gui->getAbsolutePosition();
                        const RBX::Vector2 size = gui->getAbsoluteSize();
                        if (fullName ==
                            "CoreGui.RobloxGui.SettingsClippingShield.SettingsShield."
                            "MenuContainer")
                            boundedReportMenu = gui->isCurrentlyVisible() &&
                                position.x >= 0.0f && position.y >= 0.0f &&
                                size.x > 0.0f && size.y > 0.0f &&
                                position.x + size.x <= state->logicalWidth &&
                                position.y + size.y <= state->logicalHeight;
                        if (fullName.ends_with(".ReportAbuseTab.TabSelection"))
                            selectedReportTab = gui->isCurrentlyVisible();
                        if (fullName.ends_with(".AbuseReportsText"))
                            if (RBX::TextBox* textBox =
                                    RBX::Instance::fastDynamicCast<RBX::TextBox>(gui))
                                visibleReportTextBox = gui->isCurrentlyVisible() &&
                                    textBox->getOverlayNativeInput() &&
                                    size.x > 0.0f && size.y > 0.0f;
                        if (RBX::TextLabel* label =
                                RBX::Instance::fastDynamicCast<RBX::TextLabel>(gui)) {
                            visibleReportTargetPrompt |= gui->isCurrentlyVisible() &&
                                label->getText() == "Experience or Person?";
                            visibleReportReasonPrompt |= gui->isCurrentlyVisible() &&
                                label->getText() == "Reason for Abuse?";
                        }
                        if (fullName ==
                            "CoreGui.TopBarApp.TopBarApp.MenuIconHolder.TriggerPoint."
                            "IconHitArea.ScalingIcon")
                            visibleChromeDuringReport = gui->isCurrentlyVisible();
                    }
                    if (state->verifiesRespawn) {
                        const RBX::Vector2 position = gui->getAbsolutePosition();
                        const RBX::Vector2 size = gui->getAbsoluteSize();
                        if (fullName ==
                            "CoreGui.RobloxGui.SettingsClippingShield.SettingsShield."
                            "MenuContainer")
                            boundedRespawnMenu = gui->isCurrentlyVisible() &&
                                position.x >= 0.0f && position.y >= 0.0f &&
                                size.x > 0.0f && size.y > 0.0f &&
                                position.x + size.x <= state->logicalWidth &&
                                position.y + size.y <= state->logicalHeight;
                        if (fullName.ends_with(
                                ".ResetCharacterButtonsContainer.ResetCharacterText"))
                            if (RBX::TextLabel* label =
                                    RBX::Instance::fastDynamicCast<RBX::TextLabel>(gui))
                                visibleRespawnPrompt = gui->isCurrentlyVisible() &&
                                    label->getText() ==
                                        "Are you sure you want to respawn your character?";
                        if (fullName.ends_with(
                                ".ButtonsContainer.ResetCharacterButton"))
                            visibleRespawnAction = gui->isCurrentlyVisible() &&
                                RBX::Instance::fastDynamicCast<RBX::GuiButton>(gui) != nullptr;
                        if (fullName.ends_with(
                                ".ButtonsContainer.DontResetCharacterButton"))
                            visibleDontRespawnAction = gui->isCurrentlyVisible() &&
                                RBX::Instance::fastDynamicCast<RBX::GuiButton>(gui) != nullptr;
                        if (fullName ==
                            "CoreGui.TopBarApp.TopBarApp.MenuIconHolder.TriggerPoint."
                            "IconHitArea.ScalingIcon")
                            visibleChromeDuringRespawn = gui->isCurrentlyVisible();
                    }
                    if (state->verifiesExperienceChat &&
                        fullName.starts_with("CoreGui.ExperienceChat")) {
                        visibleExperienceChatSurface |= gui->isCurrentlyVisible();
                        visibleExperienceChatInput |= gui->isCurrentlyVisible() &&
                            RBX::Instance::fastDynamicCast<RBX::TextBox>(gui) != nullptr;
                    }
                    if (fullName.ends_with(".CardTooltip") &&
                        gui->isCurrentlyVisible())
                        visiblePeopleCardTooltip = true;
                    if (fullName.ends_with(".CardThumbnail") &&
                        gui->isCurrentlyVisible()) {
                        visiblePeopleCardThumbnail = true;
                        transparentPeopleCardThumbnail =
                            gui->getBackgroundTransparency() >= 0.99f;
                    }
                    if (fullName.ends_with(
                            ".trust_and_safety.RowLabel.IconHost."
                            "IntegrationIconFrame.IntegrationIcon")) {
                        reportChromeIconSeen = true;
                        reportChromeIconTransparent =
                            gui->getBackgroundTransparency() >= 0.99f;
                    }
                    if (fullName.ends_with(
                            ".avatar_switcher.RowLabel.IconHost."
                            "IntegrationIconFrame.IntegrationIcon")) {
                        avatarChromeIconSeen = true;
                        avatarChromeIconTransparent =
                            gui->getBackgroundTransparency() >= 0.99f;
                    }
                    if (visibleGuiCount <= 30U || fullName.find("Unibar") != std::string::npos ||
                        fullName.find("TopBar") != std::string::npos ||
                        fullName.find("MenuIcon") != std::string::npos ||
                        fullName.find("CardTooltip") != std::string::npos ||
                        fullName.find("PeoplePage") != std::string::npos ||
                        fullName.find("CapturesGallery") != std::string::npos ||
                        fullName.find("VirtualizedItem_1.Item") != std::string::npos ||
                        fullName.find("BottomButton") != std::string::npos ||
                        fullName.find("ResumeButton") != std::string::npos ||
                        fullName.find("LeaveGameButton") != std::string::npos ||
                        fullName.find("ResetCharacterButton") != std::string::npos)
                    {
                        const RBX::Vector2 position = gui->getAbsolutePosition();
                        const RBX::Vector2 size = gui->getAbsoluteSize();
                        const RBX::Rect2D internalRect = gui->getRect2DFloat();
                        std::cout << "visible CoreGui object=" << fullName
                                  << " class=" << gui->getClassName().toString()
                                  << " current=" << gui->isCurrentlyVisible()
                                  << " position=" << position.x << ',' << position.y
                                  << " internal=" << internalRect.x0() << ',' << internalRect.y0()
                                  << " size=" << size.x << ',' << size.y
                                  << " rotation=" << gui->getRotation()
                                  << " background=" << gui->getBackgroundColor3()
                                  << " background-alpha=" << gui->getBackgroundTransparency()
                                  << " border=" << gui->getBorderSizePixel();
                        if (RBX::TextLabel* label =
                                RBX::Instance::fastDynamicCast<RBX::TextLabel>(gui))
                            std::cout << " text='" << label->getText()
                                      << "' text-size=" << label->getTextSize()
                                      << " scaled=" << label->getTextScale()
                                      << " bounds=" << label->getTextBounds().x << ','
                                      << label->getTextBounds().y
                                      << " stroke-alpha=" << label->getTextStrokeTransparency()
                                      << " font=" << static_cast<int>(label->getFont())
                                      << " face=" << label->getFontFace().getFamily();
                        if (RBX::GuiButton* button =
                                RBX::Instance::fastDynamicCast<RBX::GuiButton>(gui))
                            std::cout << " button-active=" << button->getActive()
                                      << " interactable=" << button->getInteractable()
                                      << " activated-listener="
                                      << !button->activatedSignal.empty();
                        if (RBX::ImageLabel* image =
                                RBX::Instance::fastDynamicCast<RBX::ImageLabel>(gui))
                            std::cout << " image='" << image->getImage().toString()
                                      << "' image-alpha="
                                      << image->getImageTransparency();
                        if (RBX::GuiImageButton* image =
                                RBX::Instance::fastDynamicCast<RBX::GuiImageButton>(gui)) {
                            std::cout << " image='" << image->getImage().toString()
                                      << "' image-alpha="
                                      << image->getImageTransparency();
                            const std::string expectedCaptureUri =
                                "file://" + state->verificationCapturePath.string();
                            if (state->verifiesCaptureGallery &&
                                fullName.find(".CapturesGallery.") !=
                                    std::string::npos &&
                                image->getImage().toString() == expectedCaptureUri &&
                                image->getImageTransparency() < 1.0F &&
                                image->isCurrentlyVisible()) {
                                state->captureThumbnailVisible = true;
                                state->verificationCaptureThumbnailRect =
                                    image->getRect2DFloat();
                            }
                        }
                        std::cout << '\n';
                    }
                    if (fullName.find("TriggerPoint") != std::string::npos ||
                        fullName.find("IconHitArea") != std::string::npos ||
                        fullName.find("FullScreenFrame") != std::string::npos) {
                        std::cout << "Chrome input target=" << fullName
                                  << " class=" << gui->getClassName().toString()
                                  << " active=" << gui->getActive()
                                  << " interactable=" << gui->getInteractable()
                                  << " visible-now=" << gui->isCurrentlyVisible()
                                  << " z=" << gui->getZIndex();
                        if (RBX::GuiButton* button =
                                RBX::Instance::fastDynamicCast<RBX::GuiButton>(gui))
                            std::cout << " activated-listener="
                                      << !button->activatedSignal.empty();
                        std::cout << '\n';
                    }
                }
                if (state->verifiesPeoplePage && visiblePeopleCardTooltip)
                    throw std::runtime_error(
                        "People card tooltip remained visible after its genuine OK action");
                if (state->verifiesPeoplePage &&
                    (!visiblePeopleCardThumbnail || !transparentPeopleCardThumbnail))
                    throw std::runtime_error(
                        "People card thumbnail did not retain its resolved Foundation transparency");
                if ((reportChromeIconSeen && !reportChromeIconTransparent) ||
                    (avatarChromeIconSeen && !avatarChromeIconTransparent))
                    throw std::runtime_error(
                        "Chrome Report or Switch Avatar icon retained an opaque background");
                if (musicChromeEntryMounted)
                    throw std::runtime_error(
                        "Chrome mounted the product-disabled Music integration");
                if (state->verifiesExperienceChat &&
                    (!experienceChatEnabled || !visibleExperienceChatSurface ||
                     !visibleExperienceChatInput ||
                     !visibleExperienceChatMessagePrefix ||
                     !visibleExperienceChatMessageBody))
                    throw std::runtime_error(
                        "genuine ExperienceChat input did not submit and render the local player's message");
                if (state->verifiesReport &&
                    (state->reportFlowScriptError || !boundedReportMenu ||
                     !selectedReportTab || !visibleReportTextBox ||
                     !visibleReportTargetPrompt || !visibleReportReasonPrompt ||
                     !visibleChromeDuringReport))
                    throw std::runtime_error(
                        "genuine Chrome Report action did not render the complete SettingsHub report surface");
                if (state->verifiesRespawn && frameNumber == 299 &&
                    (!boundedRespawnMenu || !visibleRespawnPrompt ||
                     !visibleRespawnAction || !visibleDontRespawnAction ||
                     !visibleChromeDuringRespawn))
                    throw std::runtime_error(
                        "genuine Chrome Respawn action did not render the complete SettingsHub confirmation");
                if (state->verifiesRespawn && frameNumber == 1799) {
                    RBX::Network::Players* respawnPlayers =
                        RBX::ServiceProvider::find<RBX::Network::Players>(
                            state->dataModel.get());
                    RBX::Network::Player* respawnPlayer = respawnPlayers
                        ? respawnPlayers->getLocalPlayer() : nullptr;
                    RBX::ModelInstance* respawnedCharacter = respawnPlayer
                        ? respawnPlayer->getCharacter() : nullptr;
                    RBX::Humanoid* respawnedHumanoid = respawnedCharacter
                        ? RBX::Humanoid::modelIsCharacter(respawnedCharacter) : nullptr;
                    if (visibleRespawnPrompt || visibleRespawnAction ||
                        visibleDontRespawnAction || !visibleChromeDuringRespawn ||
                        !respawnedCharacter ||
                        respawnedCharacter == state->initialRespawnCharacter ||
                        !respawnedHumanoid || respawnedHumanoid->getHealth() <= 0.0f)
                        throw std::runtime_error(
                            "official Respawn confirmation did not close and replace the dead character");
                    RBX::Instance* respawnCameraSubject = dynamic_cast<RBX::Instance*>(
                        camera->getCameraSubject());
                    std::cout << "Respawn camera subject="
                              << (respawnCameraSubject
                                      ? respawnCameraSubject->getFullName()
                                      : std::string("<none>"))
                              << " expected=" << respawnedHumanoid->getFullName()
                              << " same-instance="
                              << (respawnCameraSubject ==
                                  static_cast<RBX::Instance*>(respawnedHumanoid))
                              << '\n';
                    if (respawnCameraSubject !=
                        static_cast<RBX::Instance*>(respawnedHumanoid))
                        throw std::runtime_error(
                            "respawned local character did not reclaim the gameplay camera");
                    std::cout << "Chrome Respawn lifecycle=confirmation,death,"
                              << "authoritative-replacement health="
                              << respawnedHumanoid->getHealth() << '\n';
                }
                RBX::Instance* mountedPlayerList =
                    diagnosticCoreGui->findFirstChildByName("PlayerList");
                RBX::ScreenGui* diagnosticRobloxGui =
                    diagnosticCoreGui->getRobloxScreenGui().get();
                RBX::Instance* mountedInspectAndBuy = diagnosticRobloxGui
                    ? diagnosticRobloxGui->findFirstChildByName("InspectAndBuy")
                    : nullptr;
                if (state->verifiesChromeLeaderboard &&
                    !state->verifiesChromeLeaderboardTouch &&
                    !state->verifiesChromeLeaderboardController &&
                    frameNumber == 349UL) {
                    bool boundedInspectContent = false;
                    bool boundedInspectContainer = false;
                    std::size_t inspectDescendantCount = 0;
                    std::cout << "InspectAndBuy mounted="
                              << (mountedInspectAndBuy != nullptr);
                    if (mountedInspectAndBuy) {
                        boost::shared_ptr<const RBX::Instances> inspectDescendants =
                            mountedInspectAndBuy->getDescendants();
                        inspectDescendantCount = inspectDescendants->size();
                        for (const boost::shared_ptr<RBX::Instance>& descendant :
                             *inspectDescendants) {
                            RBX::GuiObject* gui =
                                RBX::Instance::fastDynamicCast<RBX::GuiObject>(
                                    descendant.get());
                            if (!gui || !gui->getVisible())
                                continue;
                            const std::string fullName = descendant->getFullName();
                            const RBX::Vector2 size = gui->getAbsoluteSize();
                            if (fullName.ends_with(
                                    ".InspectAndBuyContent.Content"))
                                boundedInspectContent =
                                    size.x > 0.0f && size.y > 0.0f;
                            if (fullName.ends_with(".Content.ContainerView"))
                                boundedInspectContainer =
                                    size.x > 0.0f && size.y > 0.0f;
                        }
                    }
                    std::cout << " descendants=" << inspectDescendantCount
                              << " bounded-content=" << boundedInspectContent
                              << " bounded-container=" << boundedInspectContainer
                              << '\n';
                    if (!mountedInspectAndBuy || !boundedInspectContent ||
                        !boundedInspectContainer || inspectDescendantCount < 50U)
                        throw std::runtime_error(
                            "Chrome leaderboard Examine Avatar action did not render InspectAndBuy");
                }
                const unsigned long leaderboardClosedProofFrame =
                    state->verifiesChromeLeaderboardTouch ? 374UL
                        : state->verifiesChromeLeaderboardController ? 355UL
                        : 369UL;
                const unsigned long leaderboardFinalProofFrame =
                    state->verifiesChromeLeaderboardTouch ||
                            state->verifiesChromeLeaderboardController
                        ? 439UL : 399UL;
                if (state->verifiesChromeLeaderboard &&
                    frameNumber == leaderboardClosedProofFrame) {
                    std::cout << "InspectAndBuy closed="
                              << (mountedInspectAndBuy == nullptr) << '\n';
                    if (mountedInspectAndBuy)
                        throw std::runtime_error(
                            "Inspect And Buy overlay action did not close the menu");
                }
                RBX::Instance* reskinnedPlayerList =
                    diagnosticCoreGui->findFirstChildByName("PlayerListReskin");
                if (state->usesCurrentInExperienceUi && reskinnedPlayerList)
                    throw std::runtime_error(
                        "current Player UI mounted the explicitly rejected PlayerListReskin presentation");
                if (state->verifiesChromeLeaderboard && !mountedPlayerList)
                    throw std::runtime_error(
                        "current Player UI did not mount Chrome's normal PlayerList presentation");
                if (mountedPlayerList) {
                    boost::shared_ptr<const RBX::Instances> playerListDescendants =
                        mountedPlayerList->getDescendants();
                    bool boundedVisiblePlayerList = false;
                    bool populatedVisiblePlayerList = false;
                    bool visiblePlayerDropDownHeader = false;
                    bool visiblePlayerDropDownAvatar = false;
                    bool visibleExamineAvatar = false;
                    std::cout << "PlayerList descendants="
                              << playerListDescendants->size() << '\n';
                    for (const boost::shared_ptr<RBX::Instance>& descendant :
                         *playerListDescendants) {
                        const std::string fullName = descendant->getFullName();
                        std::cout << "PlayerList node=" << descendant->getFullName()
                                  << " class=" << descendant->getClassName().toString();
                        if (!descendant->getTagsInternal().empty()) {
                            std::cout << " tags=";
                            bool firstTag = true;
                            for (const std::string& tag : descendant->getTagsInternal()) {
                                std::cout << (firstTag ? "" : "|") << tag;
                                firstTag = false;
                            }
                        }
                        if (RBX::GuiObject* gui =
                                RBX::Instance::fastDynamicCast<RBX::GuiObject>(
                                    descendant.get())) {
                            const RBX::Vector2 position = gui->getAbsolutePosition();
                            const RBX::Vector2 size = gui->getAbsoluteSize();
                            const RBX::Vector2 anchor = gui->getAnchorPoint();
                            const RBX::UDim2 authoredPosition = gui->getPosition();
                            const RBX::Vector2 automaticContent =
                                gui->getAutomaticContentSize(RBX::Vector2(
                                    static_cast<float>(state->logicalWidth),
                                    static_cast<float>(state->logicalHeight)));
                            std::cout << " visible=" << gui->getVisible()
                                      << " current=" << gui->isCurrentlyVisible()
                                      << " automatic=" << gui->getAutomaticSize()
                                      << " authored=" << gui->getSize().x.scale << ','
                                      << gui->getSize().x.offset << ','
                                      << gui->getSize().y.scale << ','
                                      << gui->getSize().y.offset
                                      << " authored-position="
                                      << authoredPosition.x.scale << ','
                                      << authoredPosition.x.offset << ','
                                      << authoredPosition.y.scale << ','
                                      << authoredPosition.y.offset
                                      << " anchor=" << anchor.x << ',' << anchor.y
                                      << " content=" << automaticContent.x << ','
                                      << automaticContent.y
                                      << " position=" << position.x << ',' << position.y
                                      << " size=" << size.x << ',' << size.y
                                      << " background=" << gui->getBackgroundColor3()
                                      << " background-alpha="
                                      << gui->getBackgroundTransparency();
                            if (fullName == "CoreGui.PlayerList.Children.OffsetFrame")
                                boundedVisiblePlayerList = gui->isCurrentlyVisible() &&
                                    size.x > 0.0f && size.y > 0.0f &&
                                    position.x >= 0.0f && position.y >= 0.0f &&
                                    position.x + size.x <= state->logicalWidth &&
                                    position.y + size.y <= state->logicalHeight;
                            if (fullName.ends_with(
                                    ".PlayerEntryContentFrame.OverlayFrame.NameFrame.PlayerName.PlayerName"))
                                if (RBX::TextLabel* label =
                                        RBX::Instance::fastDynamicCast<RBX::TextLabel>(gui))
                                    populatedVisiblePlayerList =
                                        gui->isCurrentlyVisible() &&
                                        label->getText() == "Player";
                            if (state->verifiesChromeLeaderboardController &&
                                fullName.ends_with(
                                    ".NameFrame.BackgroundFrame.OverlayFrame.PlayerName.PlayerName"))
                                if (RBX::TextLabel* label =
                                        RBX::Instance::fastDynamicCast<RBX::TextLabel>(gui))
                                    populatedVisiblePlayerList =
                                        gui->isCurrentlyVisible() &&
                                        label->getText() == "Player";
                            const unsigned long dropDownProofFrame =
                                state->verifiesChromeLeaderboardTouch ? 355UL
                                    : state->verifiesChromeLeaderboardController ? 349UL
                                    : 329UL;
                            if (frameNumber == dropDownProofFrame &&
                                gui->isCurrentlyVisible()) {
                                if (fullName.ends_with(
                                        ".PlayerDropDown.InnerFrame.PlayerHeader.Background.TextContainerFrame.DisplayName"))
                                    if (RBX::TextLabel* label =
                                            RBX::Instance::fastDynamicCast<RBX::TextLabel>(gui))
                                        visiblePlayerDropDownHeader =
                                            label->getText() == "Player";
                                if (fullName.ends_with(
                                        ".PlayerDropDown.InnerFrame.PlayerHeader.AvatarImage"))
                                    visiblePlayerDropDownAvatar = true;
                                if (fullName.ends_with(
                                        ".PlayerDropDown.InnerFrame.InspectButton.HoverBackground.Text"))
                                    if (RBX::TextLabel* label =
                                            RBX::Instance::fastDynamicCast<RBX::TextLabel>(gui))
                                        visibleExamineAvatar =
                                            label->getText() == "Examine Avatar";
                            }
                        }
                        std::cout << '\n';
                    }
                    const unsigned long initialPlayerListProofFrame =
                        state->verifiesChromeLeaderboardTouch ? 319UL : 299UL;
                    if (state->verifiesChromeLeaderboard &&
                        (frameNumber == initialPlayerListProofFrame ||
                         (!state->verifiesChromeLeaderboardController &&
                          frameNumber == leaderboardFinalProofFrame)) &&
                        (!boundedVisiblePlayerList || !populatedVisiblePlayerList))
                        throw std::runtime_error(
                            "Chrome's normal PlayerList did not render a bounded populated panel");
                    if (state->verifiesChromeLeaderboard &&
                        frameNumber == leaderboardClosedProofFrame &&
                        boundedVisiblePlayerList)
                        throw std::runtime_error(
                            "Chrome's normal leaderboard action did not close the panel");
                    const unsigned long dropDownProofFrame =
                        state->verifiesChromeLeaderboardTouch ? 355UL
                            : state->verifiesChromeLeaderboardController ? 349UL
                            : 329UL;
                    if (state->verifiesChromeLeaderboard &&
                        !state->verifiesChromeLeaderboardController &&
                        frameNumber == dropDownProofFrame &&
                        (!visiblePlayerDropDownHeader ||
                         !visiblePlayerDropDownAvatar || !visibleExamineAvatar))
                        throw std::runtime_error(
                            "Chrome's normal PlayerList did not render the official player context menu");
                }
                std::cout << "visible CoreGui objects=" << visibleGuiCount << '\n';
                if (RBX::CoreGuiConfiguration* configuration =
                        RBX::ServiceProvider::find<RBX::CoreGuiConfiguration>(
                            state->dataModel.get())) {
                    RBX::PlayerListConfiguration* playerListConfiguration =
                        configuration->getPlayerListConfiguration();
                    std::cout << "PlayerListConfiguration enabled="
                              << (playerListConfiguration &&
                                  playerListConfiguration->getEnabled())
                              << " open="
                              << (playerListConfiguration &&
                                  playerListConfiguration->getOpen()) << '\n';
                    if (state->verifiesChromeLeaderboard &&
                        frameNumber == leaderboardClosedProofFrame &&
                        playerListConfiguration &&
                        playerListConfiguration->getOpen())
                        throw std::runtime_error(
                            "genuine PlayerList close button did not close the panel");
                    if (state->verifiesChromeLeaderboard &&
                        !state->verifiesChromeLeaderboardController &&
                        frameNumber == leaderboardFinalProofFrame &&
                        (!playerListConfiguration || !playerListConfiguration->getOpen()))
                        throw std::runtime_error(
                            "genuine Chrome leaderboard action did not reopen the panel");
                }
                if (state->verifiesChromeLeaderboardTouch &&
                    frameNumber == leaderboardFinalProofFrame) {
                    RBX::UserInputService* touchInput =
                        RBX::ServiceProvider::find<RBX::UserInputService>(
                            state->dataModel.get());
                    if (!touchInput || !touchInput->getTouchEnabled() ||
                        touchInput->getLastInputType() != RBX::InputObject::TYPE_TOUCH ||
                        touchInput->getPreferredInput() !=
                            RBX::Enums::PREFERRED_INPUT_TOUCH)
                        throw std::runtime_error(
                            "genuine Chrome leaderboard touch proof did not preserve touch input identity");
                }
                if (state->verifiesChromeLeaderboardController &&
                    frameNumber == 329UL) {
                    RBX::UserInputService* controllerInput =
                        RBX::ServiceProvider::find<RBX::UserInputService>(
                            state->dataModel.get());
                    if (!controllerInput || !controllerInput->getGamepadEnabled() ||
                        !controllerInput->getGamepadConnected(
                            RBX::InputObject::TYPE_GAMEPAD1) ||
                        controllerInput->getLastInputType() !=
                            RBX::InputObject::TYPE_GAMEPAD1 ||
                        controllerInput->getPreferredInput() !=
                            RBX::Enums::PREFERRED_INPUT_GAMEPAD)
                        throw std::runtime_error(
                            "genuine Chrome leaderboard controller proof did not preserve gamepad input identity");
                }
            }
            if (!scriptableCamera &&
                (!state->sampledCameraBeforeDrag ||
                 !state->sampledCameraAfterDrag || cameraRotation < 0.1F))
                throw std::runtime_error(
                    "headless right-drag input did not rotate the gameplay camera");
        }
        RBX::CoreGuiService* coreGui =
            RBX::ServiceProvider::find<RBX::CoreGuiService>(state->dataModel.get());
        boost::shared_ptr<RBX::ScreenGui> robloxGui =
            coreGui ? coreGui->getRobloxScreenGui() : boost::shared_ptr<RBX::ScreenGui>();
        std::cout << "CoreGui RobloxGui children="
                  << (robloxGui ? robloxGui->numChildren() : 0U);
        if (robloxGui) {
            for (std::size_t index = 0; index < robloxGui->numChildren(); ++index) {
                RBX::Instance* child = robloxGui->getChild(index);
                std::cout << (index == 0 ? " [" : ", ") << child->getName();
            }
            if (robloxGui->numChildren() != 0)
                std::cout << ']';
        }
        std::cout << '\n';
    }
}

void PlayerRuntime::writeFrameProof(const std::filesystem::path& outputPath)
{
    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(state->width) * state->height * 4U);
    RBX::Graphics::Framebuffer* source = state->verificationFramebuffer
        ? state->verificationFramebuffer.get()
        : state->device->getMainFramebuffer();
    source->download(
        pixels.data(), static_cast<unsigned int>(pixels.size()));

    bool varied = false;
    std::array<bool, 32768> colorBuckets{};
    std::size_t bucketCount = 0;
    std::size_t skyPixels = 0;
    std::size_t litSurfacePixels = 0;
    std::size_t warmPlacePixels = 0;
    std::size_t brightPlacePixels = 0;
    std::size_t viewportColoredPixels = 0;
    std::size_t viewportDarkPixels = 0;
    std::array<bool, 32768> viewportColorBuckets{};
    std::size_t viewportBucketCount = 0;
    std::array<bool, 32768> videoColorBuckets{};
    std::size_t videoBucketCount = 0;
    std::size_t videoColoredPixels = 0;
    std::array<bool, 32768> textColorBuckets{};
    std::size_t textColorBucketCount = 0;
    std::size_t textBrightPixels = 0;
    std::size_t textSelectionBluePixels = 0;
    std::size_t textEmojiColorPixels = 0;
    std::uint64_t textRegionHash = 1469598103934665603ULL;
    std::size_t chromeDarkPixels = 0;
    std::size_t chromeGlyphPixels = 0;
    std::size_t baseplateGridContrastPixels = 0;
    std::size_t spawnTextureDarkPixels = 0;
    std::array<bool, 32768> skyboxColorBuckets{};
    std::size_t skyboxColorBucketCount = 0;
    std::size_t skyboxContrastPixels = 0;
    std::size_t skyboxSamplePixels = 0;
    std::array<bool, 32768> captureThumbnailColorBuckets{};
    std::size_t captureThumbnailBucketCount = 0;
    std::size_t captureThumbnailPixelCount = 0;
    const unsigned int captureLeft = static_cast<unsigned int>(std::clamp(
        state->verificationCaptureThumbnailRect.x0() * state->width /
            state->logicalWidth,
        0.0F, static_cast<float>(state->width)));
    const unsigned int captureTop = static_cast<unsigned int>(std::clamp(
        state->verificationCaptureThumbnailRect.y0() * state->height /
            state->logicalHeight,
        0.0F, static_cast<float>(state->height)));
    const unsigned int captureRight = static_cast<unsigned int>(std::clamp(
        state->verificationCaptureThumbnailRect.x1() * state->width /
            state->logicalWidth,
        0.0F, static_cast<float>(state->width)));
    const unsigned int captureBottom = static_cast<unsigned int>(std::clamp(
        state->verificationCaptureThumbnailRect.y1() * state->height /
            state->logicalHeight,
        0.0F, static_cast<float>(state->height)));
    const unsigned int viewportLeft = 900U * state->width / state->logicalWidth;
    const unsigned int viewportTop = 440U * state->height / state->logicalHeight;
    const unsigned int viewportRight = 1220U * state->width / state->logicalWidth;
    const unsigned int viewportBottom = 680U * state->height / state->logicalHeight;
    const unsigned int videoLeft = 60U * state->width / state->logicalWidth;
    const unsigned int videoTop = 440U * state->height / state->logicalHeight;
    const unsigned int videoRight = 380U * state->width / state->logicalWidth;
    const unsigned int videoBottom = 620U * state->height / state->logicalHeight;
    const unsigned int textLeft = 360U * state->width / state->logicalWidth;
    const unsigned int textTop = 470U * state->height / state->logicalHeight;
    const unsigned int textRight = 1160U * state->width / state->logicalWidth;
    const unsigned int textBottom = 660U * state->height / state->logicalHeight;
    for (std::size_t index = 4U; index < pixels.size(); index += 4U) {
        if (pixels[index + 0U] != pixels[0] ||
            pixels[index + 1U] != pixels[1] ||
            pixels[index + 2U] != pixels[2]) {
            varied = true;
        }
        const std::uint8_t red = pixels[index + 0U];
        const std::uint8_t green = pixels[index + 1U];
        const std::uint8_t blue = pixels[index + 2U];
        const std::size_t bucket = (static_cast<std::size_t>(red >> 3U) << 10U) |
            (static_cast<std::size_t>(green >> 3U) << 5U) |
            static_cast<std::size_t>(blue >> 3U);
        if (!colorBuckets[bucket]) {
            colorBuckets[bucket] = true;
            ++bucketCount;
        }
        if (state->verifiesCaptureGallery && state->captureThumbnailVisible) {
            const std::size_t pixelIndex = index / 4U;
            const unsigned int x = static_cast<unsigned int>(pixelIndex % state->width);
            const unsigned int y = static_cast<unsigned int>(pixelIndex / state->width);
            if (x >= captureLeft && x < captureRight &&
                y >= captureTop && y < captureBottom) {
                ++captureThumbnailPixelCount;
                if (!captureThumbnailColorBuckets[bucket]) {
                    captureThumbnailColorBuckets[bucket] = true;
                    ++captureThumbnailBucketCount;
                }
            }
        }
        skyPixels += blue > red + 24U && blue > green + 16U;
        const std::uint8_t lowest = std::min({red, green, blue});
        const std::uint8_t highest = std::max({red, green, blue});
        // A real modern place can be intentionally monochromatic or strongly
        // color-graded (the selected Backrooms fixture is warm yellow). Treat
        // any non-crushed surface as lit; color variety is independently
        // guarded by the bucket count above.
        litSurfacePixels += highest > 48U &&
            static_cast<unsigned int>(red) + green + blue > 120U;
        if (state->verifiesPlaceVisual) {
            warmPlacePixels += red > blue + 18U && green > blue + 8U &&
                red > 55U;
            brightPlacePixels += lowest > 180U;
        }
        if (state->verifiesSkybox) {
            const std::size_t pixelIndex = index / 4U;
            const unsigned int x = static_cast<unsigned int>(pixelIndex % state->width);
            const unsigned int y = static_cast<unsigned int>(pixelIndex / state->width);
            if (x >= state->width / 3U && x + 1U < state->width &&
                y < state->height / 2U) {
                ++skyboxSamplePixels;
                if (!skyboxColorBuckets[bucket]) {
                    skyboxColorBuckets[bucket] = true;
                    ++skyboxColorBucketCount;
                }
                const int luminance = (red + green + blue) / 3;
                const int nextLuminance =
                    (pixels[index + 4U] + pixels[index + 5U] +
                     pixels[index + 6U]) / 3;
                skyboxContrastPixels += std::abs(luminance - nextLuminance) >= 3;
            }
        }
        if (state->verifiesSurfaceTextures) {
            const std::size_t pixelIndex = index / 4U;
            const unsigned int x = static_cast<unsigned int>(pixelIndex % state->width);
            const unsigned int y = static_cast<unsigned int>(pixelIndex / state->width);
            if (x >= state->width / 2U && x + 1U < state->width &&
                y >= state->height / 10U && y + 1U < state->height * 9U / 10U) {
                const std::uint8_t nextRed = pixels[index + 4U];
                const std::uint8_t nextGreen = pixels[index + 5U];
                const std::uint8_t nextBlue = pixels[index + 6U];
                const std::uint8_t nextLowest =
                    std::min({nextRed, nextGreen, nextBlue});
                const std::uint8_t nextHighest =
                    std::max({nextRed, nextGreen, nextBlue});
                const int luminance = (red + green + blue) / 3;
                const int nextLuminance = (nextRed + nextGreen + nextBlue) / 3;
                baseplateGridContrastPixels +=
                    highest - lowest < 20U && nextHighest - nextLowest < 20U &&
                    std::abs(luminance - nextLuminance) >= 5;
                const std::size_t below = index + state->width * 4U;
                const std::uint8_t belowRed = pixels[below];
                const std::uint8_t belowGreen = pixels[below + 1U];
                const std::uint8_t belowBlue = pixels[below + 2U];
                const std::uint8_t belowLowest =
                    std::min({belowRed, belowGreen, belowBlue});
                const std::uint8_t belowHighest =
                    std::max({belowRed, belowGreen, belowBlue});
                const int belowLuminance =
                    (belowRed + belowGreen + belowBlue) / 3;
                baseplateGridContrastPixels +=
                    highest - lowest < 20U && belowHighest - belowLowest < 20U &&
                    std::abs(luminance - belowLuminance) >= 5;
            }
            spawnTextureDarkPixels +=
                x >= state->width * 17U / 25U && x < state->width * 23U / 25U &&
                y >= state->height * 53U / 100U && y < state->height * 18U / 25U &&
                highest < 112U && highest - lowest < 30U;
        }
        if (state->usesCurrentInExperienceUi) {
            const std::size_t pixelIndex = index / 4U;
            const unsigned int x = static_cast<unsigned int>(pixelIndex % state->width);
            const unsigned int y = static_cast<unsigned int>(pixelIndex / state->width);
            const unsigned int chromeRight = 180U * state->width / state->logicalWidth;
            const unsigned int chromeBottom = 75U * state->height / state->logicalHeight;
            if (x < chromeRight && y < chromeBottom) {
                chromeDarkPixels += highest < 55U;
                chromeGlyphPixels += lowest > 205U;
            }
        }

        if (state->verifiesViewportRendering) {
            const std::size_t pixelIndex = index / 4U;
            const unsigned int x = static_cast<unsigned int>(pixelIndex % state->width);
            const unsigned int y = static_cast<unsigned int>(pixelIndex / state->width);
            if (x >= viewportLeft && x < viewportRight &&
                y >= viewportTop && y < viewportBottom) {
                viewportColoredPixels += highest - lowest > 48U && highest > 96U;
                viewportDarkPixels += highest < 64U;
                if (!viewportColorBuckets[bucket]) {
                    viewportColorBuckets[bucket] = true;
                    ++viewportBucketCount;
                }
            }
        }
        if (state->verifiesVideoRendering) {
            const std::size_t pixelIndex = index / 4U;
            const unsigned int x = static_cast<unsigned int>(pixelIndex % state->width);
            const unsigned int y = static_cast<unsigned int>(pixelIndex / state->width);
            if (x >= videoLeft && x < videoRight && y >= videoTop && y < videoBottom) {
                videoColoredPixels += highest - lowest > 40U && highest > 80U;
                if (!videoColorBuckets[bucket]) {
                    videoColorBuckets[bucket] = true;
                    ++videoBucketCount;
                }
            }
        }
        if (state->verifiesTextRendering) {
            const std::size_t pixelIndex = index / 4U;
            const unsigned int x = static_cast<unsigned int>(pixelIndex % state->width);
            const unsigned int y = static_cast<unsigned int>(pixelIndex / state->width);
            if (x >= textLeft && x < textRight && y >= textTop && y < textBottom) {
                if (!textColorBuckets[bucket]) {
                    textColorBuckets[bucket] = true;
                    ++textColorBucketCount;
                }
                textBrightPixels += lowest > 180U;
                textSelectionBluePixels += blue > red + 35U && blue > green + 8U;
                textEmojiColorPixels += red > 140U && green > 70U && red > blue + 40U;
                textRegionHash ^= red;
                textRegionHash *= 1099511628211ULL;
                textRegionHash ^= green;
                textRegionHash *= 1099511628211ULL;
                textRegionHash ^= blue;
                textRegionHash *= 1099511628211ULL;
            }
        }
    }
    if (!outputPath.parent_path().empty())
        std::filesystem::create_directories(outputPath.parent_path());
    std::ofstream output(outputPath, std::ios::binary);
    if (!output)
        throw std::runtime_error("could not open the render proof output");
    output << "P6\n" << state->width << ' ' << state->height << "\n255\n";
    for (std::size_t index = 0; index < pixels.size(); index += 4U) {
        output.put(static_cast<char>(pixels[index + 0U]));
        output.put(static_cast<char>(pixels[index + 1U]));
        output.put(static_cast<char>(pixels[index + 2U]));
    }
    if (!output)
        throw std::runtime_error("could not write the complete render proof");

    if (!varied)
        throw std::runtime_error("render proof contains only a flat clear color");
    const std::size_t pixelCount = pixels.size() / 4U;
    std::cout << "render proof pixels=" << pixelCount
              << " buckets=" << bucketCount
              << " lit=" << litSurfacePixels
              << " first-rgba=" << static_cast<unsigned int>(pixels[0]) << ','
              << static_cast<unsigned int>(pixels[1]) << ','
              << static_cast<unsigned int>(pixels[2]) << ','
              << static_cast<unsigned int>(pixels[3]) << '\n';
    if (bucketCount < 32U || litSurfacePixels < pixelCount / 10U) {
        throw std::runtime_error(
            "render proof is missing a textured, lit 3D scene");
    }
    if (state->verifiesPlaceVisual) {
        std::cout << "selected place visual warm=" << warmPlacePixels
                  << " bright=" << brightPlacePixels
                  << " buckets=" << bucketCount << '\n';
        if (warmPlacePixels < pixelCount / 8U ||
            brightPlacePixels < pixelCount / 10000U)
            throw std::runtime_error(
                "selected place is missing its warm materials or fluorescent lighting");
    }
    if (state->verifiesSurfaceTextures) {
        std::cout << "surface texture pixels Baseplate grid-contrast="
                  << baseplateGridContrastPixels << " SpawnLocation dark="
                  << spawnTextureDarkPixels << '\n';
        if (baseplateGridContrastPixels < pixelCount / 100U)
            throw std::runtime_error(
                "official Baseplate grid texture is loaded but not visibly rendered");
        if (spawnTextureDarkPixels < pixelCount / 3000U)
            throw std::runtime_error(
                "SpawnLocation texture is loaded but not visibly rendered");
    }
    if (state->verifiesPlaceVisual) {
        const RBX::Graphics::ImageInfo& wallpaperInfo =
            state->wallpaperTexture.getInfo();
        std::cout << "selected-place wallpaper status="
                  << state->wallpaperTexture.getStatus() << " size="
                  << wallpaperInfo.width << 'x' << wallpaperInfo.height << '\n';
        if (state->wallpaperTexture.getStatus() !=
                RBX::Graphics::TextureRef::Status_Loaded ||
            wallpaperInfo.width != 560 || wallpaperInfo.height != 440)
            throw std::runtime_error(
                "selected place authored wallpaper did not load from its embedded asset");
    }
    if (state->verifiesSkybox) {
        std::cout << "skybox pixels samples=" << skyboxSamplePixels
                  << " color-buckets=" << skyboxColorBucketCount
                  << " contrast=" << skyboxContrastPixels << '\n';
        if (skyboxSamplePixels < pixelCount / 10U ||
            skyboxColorBucketCount < 32U || skyboxContrastPixels < 512U)
            throw std::runtime_error(
                "loaded skybox faces are not visibly rendered in the scene");
    }
    if (state->usesCurrentInExperienceUi &&
        (chromeDarkPixels < 1000U || chromeGlyphPixels < 100U)) {
        throw std::runtime_error(
            "render proof is missing visible 2026 Chrome surfaces or BuilderIcons glyphs");
    }
    if (state->verifiesViewportRendering) {
        const std::size_t viewportPixelCount =
            static_cast<std::size_t>(viewportRight - viewportLeft) *
            (viewportBottom - viewportTop);
        if (viewportBucketCount < 8U ||
            viewportColoredPixels < viewportPixelCount / 100U ||
            viewportDarkPixels < viewportPixelCount / 2U) {
            throw std::runtime_error(
                "ViewportFrame proof is flat, blank, or missing rendered colored geometry");
        }
    }
    if (state->verifiesVideoRendering) {
        const std::size_t videoPixelCount =
            static_cast<std::size_t>(videoRight - videoLeft) * (videoBottom - videoTop);
        if (videoBucketCount < 32U || videoColoredPixels < videoPixelCount / 5U) {
            throw std::runtime_error(
                "VideoFrame proof is blank or missing decoded colored video pixels");
        }
    }
    if (state->verifiesTextRendering) {
        static constexpr std::uint64_t kTextRegionGoldenHash =
            15944835803061162936ULL;
        std::cout << "text visual golden hash=" << textRegionHash
                  << " color-buckets=" << textColorBucketCount
                  << " bright=" << textBrightPixels
                  << " selection-blue=" << textSelectionBluePixels
                  << " emoji-color=" << textEmojiColorPixels << '\n';
        if (textRegionHash != kTextRegionGoldenHash ||
            textColorBucketCount < 48U || textBrightPixels < 500U ||
            textSelectionBluePixels < 500U || textEmojiColorPixels < 24U)
            throw std::runtime_error(
                "Unicode text proof is blank, monochrome, or missing bidi selection geometry");
    }
    if (state->verifiesCaptureGallery) {
        std::cout << "CaptureService Gallery card proof visible="
                  << state->captureThumbnailVisible
                  << " rect=" << captureLeft << ',' << captureTop << '-'
                  << captureRight << ',' << captureBottom
                  << " pixels=" << captureThumbnailPixelCount
                  << " color-buckets=" << captureThumbnailBucketCount
                  << std::endl;
        if (!state->captureThumbnailVisible || captureThumbnailPixelCount < 256U ||
            captureThumbnailBucketCount < 8U)
            throw std::runtime_error(
                "supplied 2026 Gallery capture card is missing, blank, or flat");
        state->captureThumbnailPixelsVerified = true;
    }
}

void PlayerRuntime::finishVerification()
{
    if (state->verifiesPlaceVisual)
    {
        RBX::DataModel::LegacyLock lock(
            state->dataModel.get(), RBX::DataModelJob::Write);
        unsigned int fondamentoLabels = 0;
        unsigned int merriweatherLabels = 0;
        unsigned int specialEliteLabels = 0;
        const auto inspectFontFace = [&](const RBX::Font& face,
                                         RBX::TextService::Font resolvedFont) {
            const std::string& family = face.getFamily();
            if (family.find("Fondamento.json") != std::string::npos) {
                ++fondamentoLabels;
                if (resolvedFont != RBX::TextService::FONT_FONDAMENTO)
                    throw std::runtime_error(
                        "selected place Fondamento FontFace fell back to Source Sans");
            } else if (family.find("Merriweather.json") != std::string::npos) {
                ++merriweatherLabels;
                if (resolvedFont != RBX::TextService::FONT_MERRIWEATHER)
                    throw std::runtime_error(
                        "selected place Merriweather FontFace fell back to Source Sans");
            } else if (family.find("SpecialElite.json") != std::string::npos) {
                ++specialEliteLabels;
                if (resolvedFont != RBX::TextService::FONT_SPECIALELITE)
                    throw std::runtime_error(
                        "selected place Special Elite FontFace fell back to Source Sans");
            }
        };
        boost::shared_ptr<const RBX::Instances> descendants =
            state->dataModel->getDescendants();
        for (const boost::shared_ptr<RBX::Instance>& descendant : *descendants) {
            if (RBX::TextLabel* label =
                    RBX::Instance::fastDynamicCast<RBX::TextLabel>(descendant.get()))
                inspectFontFace(label->getFontFace(), label->getFont());
            else if (RBX::TextBox* textBox =
                    RBX::Instance::fastDynamicCast<RBX::TextBox>(descendant.get()))
                inspectFontFace(textBox->getFontFace(), textBox->getFont());
        }
        std::cout << "selected-place fonts Fondamento=" << fondamentoLabels
                  << " Merriweather=" << merriweatherLabels
                  << " SpecialElite=" << specialEliteLabels << '\n';
        if (fondamentoLabels < 2 || merriweatherLabels < 1 ||
            specialEliteLabels < 3)
            throw std::runtime_error(
                "selected place FontFace values did not survive network replication");
    }
    if (state->usesDurangoLauncher)
    {
        RBX::DataModel::LegacyLock lock(
            state->dataModel.get(), RBX::DataModelJob::Write);
        RBX::Network::Players* players =
            RBX::ServiceProvider::find<RBX::Network::Players>(
                state->dataModel.get());
        RBX::Network::Player* shellPlayer = players
            ? players->getLocalPlayer() : nullptr;
        RBX::ModelInstance* shellCharacter = shellPlayer
            ? shellPlayer->getCharacter() : nullptr;
        RBX::Workspace* workspace =
            RBX::ServiceProvider::find<RBX::Workspace>(state->dataModel.get());
        RBX::Camera* shellCamera = workspace ? workspace->getCamera() : nullptr;
        RBX::PlatformService* platformService =
            RBX::ServiceProvider::find<RBX::PlatformService>(
                state->dataModel.get());
        if (!state->dataModel->isAppShell() || state->serverDataModel ||
            state->localServer || state->localClient ||
            RBX::ServiceProvider::find<RBX::Network::Server>(
                state->dataModel.get()) ||
            RBX::ServiceProvider::find<RBX::Network::Client>(
                state->dataModel.get()))
            throw std::runtime_error(
                "Durango launcher incorrectly entered a network game session");
        RBX::Humanoid* shellHumanoid = shellCharacter
            ? RBX::Humanoid::modelIsCharacter(shellCharacter) : nullptr;
        RBX::PartInstance* shellHead = shellCharacter
            ? RBX::Instance::fastDynamicCast<RBX::PartInstance>(
                shellCharacter->findFirstChildByName("Head")) : nullptr;
        RBX::PartInstance* shellRoot = shellCharacter
            ? RBX::Instance::fastDynamicCast<RBX::PartInstance>(
                shellCharacter->findFirstChildByName("HumanoidRootPart")) : nullptr;
        if (!shellPlayer || shellPlayer->getUserID() != 1 ||
            !shellCharacter || !shellHumanoid || !shellHead || !shellRoot)
            throw std::runtime_error(
                "Durango launcher lost its authentic local identity/avatar");
        RBX::MeshContentProvider* shellMeshProvider =
            RBX::ServiceProvider::find<RBX::MeshContentProvider>(
                state->dataModel.get());
        std::size_t shellMeshPartCount = 0;
        std::size_t loadedShellMeshPartCount = 0;
        std::size_t loadedShellVertexCount = 0;
        std::size_t loadedShellFaceCount = 0;
        bool shellMeshesNonempty = shellMeshProvider != nullptr;
        for (std::size_t index = 0; index < shellCharacter->numChildren(); ++index) {
            RBX::MeshPart* meshPart =
                RBX::Instance::fastDynamicCast<RBX::MeshPart>(
                    shellCharacter->getChild(index));
            if (!meshPart)
                continue;
            ++shellMeshPartCount;
            const boost::shared_ptr<void> meshData = shellMeshProvider
                ? shellMeshProvider->blockingRequestContent(
                    meshPart->getMeshId(), true)
                : boost::shared_ptr<void>();
            if (!meshData) {
                shellMeshesNonempty = false;
                continue;
            }
            const boost::shared_ptr<RBX::FileMeshData> parsedMesh =
                boost::static_pointer_cast<RBX::FileMeshData>(meshData);
            if (parsedMesh->vnts.empty() || parsedMesh->faces.empty()) {
                shellMeshesNonempty = false;
                continue;
            }
            ++loadedShellMeshPartCount;
            loadedShellVertexCount += parsedMesh->vnts.size();
            loadedShellFaceCount += parsedMesh->faces.size();
        }
        if (shellHumanoid->getRigType() !=
                RBX::Humanoid::HUMANOID_RIG_TYPE_R15 ||
            shellMeshPartCount != 14 ||
            loadedShellMeshPartCount != shellMeshPartCount ||
            !shellMeshesNonempty || loadedShellVertexCount == 0 ||
            loadedShellFaceCount == 0)
            throw std::runtime_error(
                "Durango launcher did not retain its packaged R15 avatar geometry");
        std::cout << "Durango launcher R15 RigType="
                  << shellHumanoid->getRigType()
                  << " MeshParts=" << shellMeshPartCount
                  << " loaded=" << loadedShellMeshPartCount
                  << " mesh-vertices=" << loadedShellVertexCount
                  << " mesh-faces=" << loadedShellFaceCount << '\n';
        const RBX::Color3 expectedTint(20.0f / 255.0f, 43.0f / 255.0f,
            60.0f / 255.0f);
        if (!platformService ||
            platformService->getPlatformDatamodelType() != RBX::AppShellDatamodel)
            throw std::runtime_error(
                "Durango launcher did not expose PlatformService as AppShellDatamodel");
        boost::shared_ptr<const RBX::Reflection::ValueTable> versionInfo =
            platformService->getVersionIdInfo();
        boost::shared_ptr<const RBX::Reflection::ValueTable> platformUserInfo =
            platformService->getPlatformUserInfo();
        if (!state->launcherPlatform || !versionInfo || !platformUserInfo)
            throw std::runtime_error(
                "desktop AppShell capability adapter did not report the local build and user");
        const auto versionMajor = versionInfo->find("Major");
        const auto versionMinor = versionInfo->find("Minor");
        const auto versionBuild = versionInfo->find("Build");
        const auto versionRevision = versionInfo->find("Revision");
        const auto versionProduct = versionInfo->find("Product");
        const auto versionArchitecture = versionInfo->find("Architecture");
        const auto platformGamertag = platformUserInfo->find("Gamertag");
        const auto platformUserId = platformUserInfo->find("RobloxUserId");
        if (
            versionMajor == versionInfo->end() ||
            versionMinor == versionInfo->end() ||
            versionBuild == versionInfo->end() ||
            versionRevision == versionInfo->end() ||
            versionProduct == versionInfo->end() ||
            versionArchitecture == versionInfo->end() ||
            platformGamertag == platformUserInfo->end() ||
            platformUserId == platformUserInfo->end() ||
            !versionMajor->second.isType<int>() ||
            !versionMinor->second.isType<int>() ||
            !versionBuild->second.isType<int>() ||
            !versionRevision->second.isType<int>() ||
            versionMajor->second.cast<int>() !=
                rbx::core::BuildInfo::versionMajor ||
            versionMinor->second.cast<int>() !=
                rbx::core::BuildInfo::versionMinor ||
            versionBuild->second.cast<int>() !=
                rbx::core::BuildInfo::versionPatch ||
            versionRevision->second.cast<int>() !=
                rbx::core::BuildInfo::versionRevision ||
            !versionProduct->second.isType<std::string>() ||
            versionProduct->second.cast<std::string>() !=
                std::string(rbx::core::BuildInfo::productName) ||
            !versionArchitecture->second.isType<std::string>() ||
            versionArchitecture->second.cast<std::string>() !=
                std::string(rbx::core::BuildInfo::architecture) ||
            !platformGamertag->second.isType<std::string>() ||
            platformGamertag->second.cast<std::string>().empty() ||
            !platformUserId->second.isType<int>() ||
            platformUserId->second.cast<int>() != shellPlayer->getUserID())
            throw std::runtime_error(
                "desktop AppShell capability adapter did not report the local build and user");

        const unsigned int automaticCatalogRequests =
            state->launcherPlatform->getCatalogRequestCount();
        const unsigned int automaticPartyRequests =
            state->launcherPlatform->getPartyRequestCount();
        boost::shared_ptr<RBX::Reflection::ValueArray> unsupportedValues =
            boost::make_shared<RBX::Reflection::ValueArray>();
        unsupportedValues->push_back(RBX::Reflection::Variant(
            std::string("must be cleared")));
        std::string unsupportedResponse = "must be cleared";
        double unsupportedHeroValue = 1.0;
        if (automaticCatalogRequests == 0 || automaticPartyRequests == 0 ||
            state->launcherPlatform->fetchCatalogInfo(unsupportedValues) != 0 ||
            !unsupportedValues->empty())
            throw std::runtime_error(
                "desktop AppShell catalog probe did not return an empty offline result");
        unsupportedValues->push_back(RBX::Reflection::Variant(
            std::string("must be cleared")));
        unsupportedResponse = "must be cleared";
        if (state->launcherPlatform->getPlatformPartyMembers(unsupportedValues) != 0 ||
            !unsupportedValues->empty() ||
            state->launcherPlatform->fetchFriends(RBX::InputObject::TYPE_NONE,
                &unsupportedResponse) >= 0 || !unsupportedResponse.empty() ||
            state->launcherPlatform->performAuthorization(
                RBX::InputObject::TYPE_NONE, false) != RBX::AccountAuth_Error ||
            state->launcherPlatform->performHasRobloxCredentials() !=
                RBX::AccountAuth_Error ||
            state->launcherPlatform->performHasLinkedAccount() !=
                RBX::AccountAuth_Error ||
            state->launcherPlatform->startGame3(RBX::GameJoin_Normal, 0) !=
                RBX::GameStart_Weird ||
            state->launcherPlatform->netConnectionCheck() >= 0 ||
            state->launcherPlatform->popupPartyUI(
                RBX::InputObject::TYPE_NONE) >= 0 ||
            state->launcherPlatform->popupProfileUI(
                RBX::InputObject::TYPE_NONE, std::string()) >= 0 ||
            state->launcherPlatform->popupAccountPickerUI(
                RBX::InputObject::TYPE_NONE) >= 0 ||
            state->launcherPlatform->requestPurchase(std::string()) !=
                RBX::PurchaseResult_Error ||
            state->launcherPlatform->getPMPCreatorId() >= 0 ||
            state->launcherPlatform->getTitleId() >= 0 ||
            state->launcherPlatform->awardAchievement(std::string()) !=
                RBX::Award_Fail ||
            state->launcherPlatform->setHeroStat(std::string(),
                &unsupportedHeroValue) != RBX::Award_Fail ||
            state->launcherPlatform->voiceChatGetState(0) !=
                RBX::voiceChatState_UnknownUser)
            throw std::runtime_error(
                "desktop AppShell fabricated an Xbox-only platform capability");
        unsupportedResponse = "must be cleared";
        if (state->launcherPlatform->performAccountLink(
                std::string(), std::string(), &unsupportedResponse) >= 0 ||
            !unsupportedResponse.empty())
            throw std::runtime_error(
                "desktop AppShell fabricated Xbox account-link state");
        unsupportedResponse = "must be cleared";
        if (state->launcherPlatform->performUnlinkAccount(
                &unsupportedResponse) >= 0 || !unsupportedResponse.empty())
            throw std::runtime_error(
                "desktop AppShell fabricated Xbox account-unlink state");
        unsupportedResponse = "must be cleared";
        if (state->launcherPlatform->performSetRobloxCredentials(
                std::string(), std::string(), &unsupportedResponse) >= 0 ||
            !unsupportedResponse.empty())
            throw std::runtime_error(
                "desktop AppShell fabricated Xbox credential state");
        unsupportedValues->push_back(RBX::Reflection::Variant(
            std::string("must be cleared")));
        if (state->launcherPlatform->fetchInventoryInfo(unsupportedValues) != 0 ||
            !unsupportedValues->empty())
            throw std::runtime_error(
                "desktop AppShell inventory probe did not return an empty offline result");
        unsupportedValues->push_back(RBX::Reflection::Variant(
            std::string("must be cleared")));
        if (state->launcherPlatform->getInGamePlayers(unsupportedValues) != 0 ||
            !unsupportedValues->empty())
            throw std::runtime_error(
                "desktop AppShell session probe did not return an empty offline result");
        if (state->launcherPlatform->launchPlatformUri(
                "file:///not-approved") >= 0 ||
            takeExternalUriRequest().has_value())
            throw std::runtime_error(
                "desktop AppShell accepted a non-HTTP external URI");
        if (state->launcherPlatform->launchPlatformUri(
                "https://en.help.roblox.com/hc/en-us/articles/205358110") != 0)
            throw std::runtime_error(
                "desktop AppShell rejected an approved HTTPS URI");
        const std::optional<std::string> termsUri = takeExternalUriRequest();
        if (!termsUri || *termsUri !=
                "https://en.help.roblox.com/hc/en-us/articles/205358110" ||
            state->launcherPlatform->popupHelpUI() != 0)
            throw std::runtime_error(
                "desktop AppShell did not queue an approved URI on the host boundary");
        const std::optional<std::string> helpUri = takeExternalUriRequest();
        if (!helpUri || *helpUri != "https://en.help.roblox.com/")
            throw std::runtime_error(
                "desktop AppShell help action did not use its approved host URI");
        if (!state->launcherPostProcessApplied ||
            std::abs(platformService->brightness - 0.3f) > 0.0001f ||
            std::abs(platformService->contrast - 0.5f) > 0.0001f ||
            std::abs(platformService->grayscaleLevel - 1.0f) > 0.0001f ||
            std::abs(platformService->blurIntensity - 3.0f) > 0.0001f ||
            (platformService->tintColor - expectedTint).squaredLength() > 1e-8f ||
            std::abs(state->launcherPostProcessBrightness -
                platformService->brightness) > 0.0001f ||
            std::abs(state->launcherPostProcessContrast -
                platformService->contrast) > 0.0001f ||
            std::abs(state->launcherPostProcessGrayscale -
                platformService->grayscaleLevel) > 0.0001f ||
            std::abs(state->launcherPostProcessBlur -
                platformService->blurIntensity) > 0.0001f ||
            (state->launcherPostProcessTint - platformService->tintColor)
                .squaredLength() > 1e-8f)
            throw std::runtime_error(
                "Durango CameraManager post-process values did not reach SceneManager");
        if (shellPlayer->findFirstChildOfType<RBX::PlayerScripts>())
            throw std::runtime_error(
                "Durango launcher created gameplay PlayerScripts");
        if (!shellCamera ||
            shellCamera->getCameraType() != RBX::Camera::LOCKED_CAMERA)
            throw std::runtime_error(
                "Durango launcher camera is not exclusively owned by AppHome");

        bool foundReplicator = false;
        bool foundGameplayPlayerScripts = false;
        unsigned int authoredSceneryParts = 0;
        unsigned int authoredCameraPathParts = 0;
        RBX::Instance* cameraSets = workspace
            ? workspace->findFirstChildByName("Cameras") : nullptr;
        RBX::Instance* zones = workspace
            ? workspace->findFirstChildByName("Zones") : nullptr;
        boost::shared_ptr<const RBX::Instances> shellDescendants =
            state->dataModel->getDescendants();
        for (const boost::shared_ptr<RBX::Instance>& descendant :
             *shellDescendants) {
            foundReplicator = foundReplicator ||
                RBX::Instance::fastDynamicCast<RBX::Network::ClientReplicator>(
                    descendant.get()) ||
                RBX::Instance::fastDynamicCast<RBX::Network::ServerReplicator>(
                    descendant.get());
            foundGameplayPlayerScripts = foundGameplayPlayerScripts ||
                RBX::Instance::fastDynamicCast<RBX::PlayerScripts>(
                    descendant.get());
            if (workspace && RBX::Instance::fastDynamicCast<RBX::PartInstance>(
                    descendant.get()) && descendant->isDescendantOf(workspace)) {
                ++authoredSceneryParts;
                if (cameraSets && descendant->isDescendantOf(cameraSets))
                    ++authoredCameraPathParts;
            }
        }
        if (foundReplicator || foundGameplayPlayerScripts)
            throw std::runtime_error(
                "Durango launcher retained replicator or gameplay-script state");
        if (!cameraSets || !zones ||
            !cameraSets->findFirstChildByName("City") ||
            !cameraSets->findFirstChildByName("Space") ||
            !cameraSets->findFirstChildByName("Volcano") ||
            !zones->findFirstChildByName("City") ||
            !zones->findFirstChildByName("Space") ||
            !zones->findFirstChildByName("Volcano") ||
            authoredSceneryParts < 1000 || authoredCameraPathParts < 3)
            throw std::runtime_error(
                "Durango launcher did not retain the authored ScaledWorld 3D scene and camera paths");

        RBX::Instance* cityCameraPath =
            cameraSets->findFirstChildByName("City");
        RBX::Vector3 cityCameraMinimum;
        RBX::Vector3 cityCameraMaximum;
        unsigned int cityCameraPartCount = 0;
        for (std::size_t index = 0;
             cityCameraPath && index < cityCameraPath->numChildren(); ++index)
        {
            RBX::PartInstance* cameraPart =
                RBX::Instance::fastDynamicCast<RBX::PartInstance>(
                    cityCameraPath->getChild(index));
            if (!cameraPart)
                continue;
            const RBX::CoordinateFrame authoredCameraFrame =
                cameraPart->getCoordinateFrame() * RBX::CoordinateFrame(
                    RBX::Vector3(0.0f, 0.0f,
                        -cameraPart->getPartSizeXml().z * 0.5f));
            const RBX::Vector3 point = authoredCameraFrame.translation;
            if (!point.isFinite())
                throw std::runtime_error(
                    "ScaledWorld City camera path contains a non-finite authored frame");
            if (cityCameraPartCount++ == 0)
            {
                cityCameraMinimum = point;
                cityCameraMaximum = point;
            }
            else
            {
                cityCameraMinimum.x = std::min(cityCameraMinimum.x, point.x);
                cityCameraMinimum.y = std::min(cityCameraMinimum.y, point.y);
                cityCameraMinimum.z = std::min(cityCameraMinimum.z, point.z);
                cityCameraMaximum.x = std::max(cityCameraMaximum.x, point.x);
                cityCameraMaximum.y = std::max(cityCameraMaximum.y, point.y);
                cityCameraMaximum.z = std::max(cityCameraMaximum.z, point.z);
            }
        }
        if (cityCameraPartCount < 2)
            throw std::runtime_error(
                "ScaledWorld City camera path has no authored motion curve");
        const RBX::RenderStats* launcherRenderStats =
            state->visualEngine->getRenderStats();
        if (!launcherRenderStats || launcherRenderStats->passScene.batches == 0 ||
            launcherRenderStats->passScene.faces == 0 ||
            launcherRenderStats->passScene.vertices == 0)
            throw std::runtime_error(
                "Durango launcher did not render its live ScaledWorld 3D scene");

        RBX::CoreGuiService* coreGui =
            RBX::ServiceProvider::find<RBX::CoreGuiService>(
                state->dataModel.get());
        RBX::Instance* robloxGui = coreGui
            ? coreGui->findFirstChildByName("RobloxGui") : nullptr;
        RBX::Instance* appHome = robloxGui
            ? robloxGui->findFirstChildByName("AppHomeContainer") : nullptr;
        RBX::GuiObject* appHomeGui =
            RBX::Instance::fastDynamicCast<RBX::GuiObject>(appHome);
        RBX::ImageLabel* shellBackground = appHome
            ? RBX::Instance::fastDynamicCast<RBX::ImageLabel>(
                appHome->findFirstChildByName("Background"))
            : nullptr;
        RBX::ImageLabel* crossfadeBackground = shellBackground
            ? RBX::Instance::fastDynamicCast<RBX::ImageLabel>(
                shellBackground->findFirstChildByName("CrossfadeBackground"))
            : nullptr;
        RBX::Instance* engagement = appHome
            ? appHome->findFirstChildByNameRecursive("EngagementScreen") : nullptr;
        RBX::Instance* logo = engagement
            ? engagement->findFirstChildByNameRecursive("RobloxLogo") : nullptr;
        RBX::Instance* hub = appHome
            ? appHome->findFirstChildByNameRecursive("HubContainer") : nullptr;
        RBX::Instance* homePane = hub
            ? hub->findFirstChildByNameRecursive("HomePane") : nullptr;
        RBX::BindableEvent* openDocument = coreGui
            ? RBX::Instance::fastDynamicCast<RBX::BindableEvent>(
                coreGui->findFirstChildByName("OpenLocalDocument"))
            : nullptr;
        RBX::Instance* openDocumentButton = homePane
            ? homePane->findFirstChildByNameRecursive("OpenLocalDocumentButton")
            : nullptr;
        RBX::Instance* recentDocumentButton = homePane
            ? homePane->findFirstChildByNameRecursive("RecentLocalDocument1")
            : nullptr;
        RBX::TextLabel* profileName = homePane
            ? RBX::Instance::fastDynamicCast<RBX::TextLabel>(
                homePane->findFirstChildByNameRecursive("NameLabel"))
            : nullptr;
        RBX::GuiObject* profileContainer = homePane
            ? RBX::Instance::fastDynamicCast<RBX::GuiObject>(
                homePane->findFirstChildByNameRecursive("ProfileContainer"))
            : nullptr;
        RBX::GuiObject* openDocumentGui =
            RBX::Instance::fastDynamicCast<RBX::GuiObject>(openDocumentButton);
        RBX::TextLabel* openTitle = openDocumentButton
            ? RBX::Instance::fastDynamicCast<RBX::TextLabel>(
                openDocumentButton->findFirstChildByName("Title"))
            : nullptr;
        RBX::TextLabel* openHint = openDocumentButton
            ? RBX::Instance::fastDynamicCast<RBX::TextLabel>(
                openDocumentButton->findFirstChildByName("Hint"))
            : nullptr;
        RBX::GuiObject* recentDocumentGui =
            RBX::Instance::fastDynamicCast<RBX::GuiObject>(recentDocumentButton);
        RBX::TextLabel* recentTitle = recentDocumentButton
            ? RBX::Instance::fastDynamicCast<RBX::TextLabel>(
                recentDocumentButton->findFirstChildByName("Title"))
            : nullptr;
        RBX::TextLabel* recentPath = recentDocumentButton
            ? RBX::Instance::fastDynamicCast<RBX::TextLabel>(
                recentDocumentButton->findFirstChildByName("Path"))
            : nullptr;
        RBX::StringValue* recentDocument = coreGui
            ? RBX::Instance::fastDynamicCast<RBX::StringValue>(
                coreGui->findFirstChildByNameRecursive("RecentDocument1"))
            : nullptr;
        RBX::Soundscape::SoundService* soundService =
            RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(
                state->dataModel.get());
        RBX::Instance* shellSounds = soundService
            ? soundService->findFirstChildByName("AppShellSounds") : nullptr;
        RBX::Instance* backgroundLoop = shellSounds
            ? shellSounds->findFirstChildByName("BackgroundLoop") : nullptr;
        if (!appHomeGui || !shellBackground || !crossfadeBackground ||
            !engagement || !logo || !hub || !homePane ||
            !openDocument || !openDocumentButton || !recentDocumentButton ||
            !profileName || !profileContainer || !openDocumentGui ||
            !openTitle || !openHint ||
            !recentDocumentGui || !recentTitle || !recentPath ||
            !recentDocument || !shellSounds ||
            !backgroundLoop || backgroundLoop->numChildren() != 3)
            throw std::runtime_error(
                "authentic Durango launcher did not complete desktop activation into its shell");
        if (appHomeGui->getBackgroundTransparency() < 0.999f ||
            shellBackground->getBackgroundTransparency() < 0.999f ||
            shellBackground->getImageTransparency() < 0.999f ||
            !shellBackground->getImage().toString().empty() ||
            crossfadeBackground->getBackgroundTransparency() < 0.999f ||
            !crossfadeBackground->getImage().toString().empty())
            throw std::runtime_error(
                "Durango AppHomeContainer/Background/CrossfadeBackground obscured live ScaledWorld 3D");

        for (const boost::shared_ptr<RBX::Instance>& descendant :
             *shellDescendants)
        {
            RBX::GuiObject* gui =
                RBX::Instance::fastDynamicCast<RBX::GuiObject>(descendant.get());
            if (!gui || !gui->isCurrentlyVisible())
                continue;
            const RBX::Vector2 position = gui->getAbsolutePosition();
            const RBX::Vector2 size = gui->getAbsoluteSize();
            const float left = std::max(0.0f, position.x);
            const float top = std::max(0.0f, position.y);
            const float right = std::min(
                static_cast<float>(state->logicalWidth), position.x + size.x);
            const float bottom = std::min(
                static_cast<float>(state->logicalHeight), position.y + size.y);
            const float coveredArea =
                std::max(0.0f, right - left) * std::max(0.0f, bottom - top);
            const float viewportArea = static_cast<float>(
                state->logicalWidth * state->logicalHeight);
            if (coveredArea < viewportArea * 0.95f)
                continue;

            bool opaqueImage = false;
            if (RBX::ImageLabel* imageLabel =
                    RBX::Instance::fastDynamicCast<RBX::ImageLabel>(gui))
                opaqueImage = imageLabel->getImageTransparency() <= 0.01f &&
                    !imageLabel->getImage().toString().empty();
            else if (RBX::GuiImageButton* imageButton =
                         RBX::Instance::fastDynamicCast<RBX::GuiImageButton>(gui))
                opaqueImage = imageButton->getImageTransparency() <= 0.01f &&
                    !imageButton->getImage().toString().empty();
            if (gui->getBackgroundTransparency() <= 0.01f || opaqueImage)
                throw std::runtime_error(
                    "Durango launcher has an opaque fullscreen GUI cover: " +
                    gui->getFullName());
        }
        if (profileName->getText().empty() ||
            profileName->getText() == "INSTUDIONOGAMERTAG" ||
            !profileContainer->getClipping() ||
            !openDocumentGui->getClipping() ||
            openTitle->getTextTruncate() != RBX::Enums::TEXT_TRUNCATE_AT_END ||
            openHint->getText() != "PLACE OR MODEL FILE  |  COMMAND-O" ||
            openHint->getTextTruncate() != RBX::Enums::TEXT_TRUNCATE_AT_END ||
            !recentDocumentGui->getClipping() ||
            recentTitle->getTextTruncate() != RBX::Enums::TEXT_TRUNCATE_AT_END ||
            recentPath->getTextTruncate() != RBX::Enums::TEXT_TRUNCATE_AT_END)
            throw std::runtime_error(
                "Durango launcher desktop identity or bounded local-document copy regressed");
        if (recentDocument->getValue().find("ScaledWorldv4.7.rbxl") !=
                std::string::npos ||
            std::filesystem::path(recentDocument->getValue()).filename() !=
                "Baseplate.rbxl")
            throw std::runtime_error(
                "Durango launcher exposed its internal scenery as a recent game");
        if (coreGui->findFirstChildByNameRecursive("ControlFrame"))
            throw std::runtime_error(
                "Durango launcher did not let XStarterScript remove ControlFrame");
        boost::shared_ptr<RBX::Reflection::Tuple> recentArguments =
            boost::make_shared<RBX::Reflection::Tuple>();
        recentArguments->values.push_back(
            RBX::Reflection::Variant(recentDocument->getValue()));
        openDocument->fire(recentArguments);
        const auto requestedRecent = takeRecentDocumentRequest();
        if (!requestedRecent ||
            requestedRecent->string() != recentDocument->getValue())
            throw std::runtime_error(
                "Durango launcher recent-document action did not reach the Player host");
        openDocument->fire(boost::make_shared<RBX::Reflection::Tuple>());
        if (!takeOpenDocumentRequest())
            throw std::runtime_error(
                "Durango launcher local-document bridge did not reach the Player host");
        if (state->verifiesDurangoLauncher)
        {
            if (!state->launcherFirstFrameCaptured ||
                !state->launcherSecondFrameCaptured ||
                state->launcherFirstFrameNumber != kLauncherFirstSettledFrame ||
                state->launcherSecondFrameNumber != kLauncherSecondSettledFrame ||
                state->launcherSecondFrameNumber -
                    state->launcherFirstFrameNumber < 120)
                throw std::runtime_error(
                    "Durango launcher did not produce two separated settled-frame readbacks");

            const auto insideCityCameraHull =
                [&](const RBX::Vector3& point) {
                    constexpr float tolerance = 0.5f;
                    return point.isFinite() &&
                        point.x >= cityCameraMinimum.x - tolerance &&
                        point.x <= cityCameraMaximum.x + tolerance &&
                        point.y >= cityCameraMinimum.y - tolerance &&
                        point.y <= cityCameraMaximum.y + tolerance &&
                        point.z >= cityCameraMinimum.z - tolerance &&
                        point.z <= cityCameraMaximum.z + tolerance;
                };
            const RBX::Vector3 firstCameraPosition =
                state->launcherFirstCameraFrame.translation;
            const RBX::Vector3 secondCameraPosition =
                state->launcherSecondCameraFrame.translation;
            const RBX::Vector3 firstCameraLook =
                state->launcherFirstCameraFrame.lookVector();
            const RBX::Vector3 secondCameraLook =
                state->launcherSecondCameraFrame.lookVector();
            if (!insideCityCameraHull(firstCameraPosition) ||
                !insideCityCameraHull(secondCameraPosition) ||
                !firstCameraLook.isFinite() || !secondCameraLook.isFinite())
                throw std::runtime_error(
                    "Durango CameraManager samples escaped the authored ScaledWorld City path");

            const float cameraTranslation =
                (secondCameraPosition - firstCameraPosition).magnitude();
            const float cameraLookDelta =
                (secondCameraLook - firstCameraLook).magnitude();
            if (cameraTranslation < 0.05f && cameraLookDelta < 0.001f)
                throw std::runtime_error(
                    "Durango CameraManager Scriptable camera froze between settled frames");

            const LauncherPixelEvidence firstPixels = analyzeLauncherPixels(
                state->launcherFirstFramePixels, state->width, state->height);
            const LauncherPixelEvidence secondPixels = analyzeLauncherPixels(
                state->launcherSecondFramePixels, state->width, state->height);
            const LauncherTemporalPixelEvidence temporalPixels =
                compareLauncherPixels(state->launcherFirstFramePixels,
                    state->launcherSecondFramePixels,
                    state->width, state->height);
            const std::size_t pixelCount =
                static_cast<std::size_t>(state->width) * state->height;
            const auto hasLiveSceneDetail = [&](const LauncherPixelEvidence& value) {
                return value.colorBuckets >= 48 &&
                    value.maximumLuminance >= value.minimumLuminance + 24 &&
                    value.luminanceDeviation >= 4.0 &&
                    value.spatialEdges >= pixelCount / 2000U;
            };
            std::size_t changedQuadrants = 0;
            const std::size_t minimumQuadrantChanges =
                std::max<std::size_t>(16U, pixelCount / 20000U);
            for (const std::size_t changed :
                 temporalPixels.changedPixelsByQuadrant)
                changedQuadrants += changed >= minimumQuadrantChanges;
            if (!hasLiveSceneDetail(firstPixels) ||
                !hasLiveSceneDetail(secondPixels))
                throw std::runtime_error(
                    "Durango settled-frame readback is flat or lacks live 3D scene detail");
            if (temporalPixels.changedPixels <
                    std::max<std::size_t>(256U, pixelCount / 1000U) ||
                changedQuadrants < 3 || temporalPixels.meanChannelDelta < 0.1)
                throw std::runtime_error(
                    "Durango readback did not show viewport-wide motion from its authored 3D camera path");

            std::cout << "Durango live-3D temporal proof frames="
                      << state->launcherFirstFrameNumber << ','
                      << state->launcherSecondFrameNumber
                      << " CameraType=Scriptable City-path-parts="
                      << cityCameraPartCount
                      << " camera-translation=" << cameraTranslation
                      << " camera-look-delta=" << cameraLookDelta
                      << " color-buckets=" << firstPixels.colorBuckets << ','
                      << secondPixels.colorBuckets
                      << " luminance-deviation="
                      << firstPixels.luminanceDeviation << ','
                      << secondPixels.luminanceDeviation
                      << " spatial-edges=" << firstPixels.spatialEdges << ','
                      << secondPixels.spatialEdges
                      << " changed-pixels=" << temporalPixels.changedPixels
                      << " changed-quadrants=" << changedQuadrants
                      << " mean-channel-delta="
                      << temporalPixels.meanChannelDelta << '\n';
        }
        std::cout << "Durango launcher direct AppShell retained local avatar, "
                     "mounted AppHome without network/gameplay state, engagement logo, and "
                  << backgroundLoop->numChildren()
                  << " pooled background-music voices; controller activation opened HomePane "
                     "and its picker/recent-document actions reached the Player host; "
                     "PlatformService AppShell post-process="
                  << platformService->brightness << ','
                  << platformService->contrast << ','
                  << platformService->grayscaleLevel << ','
                  << platformService->blurIntensity
                  << " ScaledWorld-parts=" << authoredSceneryParts
                  << " camera-path-parts=" << authoredCameraPathParts
                  << " scene-batches=" << launcherRenderStats->passScene.batches
                  << " scene-faces=" << launcherRenderStats->passScene.faces
                  << " scene-vertices=" << launcherRenderStats->passScene.vertices
                  << "\n";
    }
    if (state->verifiesAudio) {
        RBX::DataModel::LegacyLock lock(
            state->dataModel.get(), RBX::DataModelJob::Write);
        RBX::Soundscape::SoundService* soundService =
            RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(
                state->dataModel.get());
        const double unitDuration = soundService && soundService->getSampleRate() > 0
            ? static_cast<double>(state->audioUnitSpeedFrames) / soundService->getSampleRate()
            : 0.0;
        const double halfDuration = soundService && soundService->getSampleRate() > 0
            ? static_cast<double>(state->audioHalfSpeedFrames) / soundService->getSampleRate()
            : 0.0;
        const double rms = state->audioSampleCount == 0 ? 0.0 :
            std::sqrt(state->audioSquaredSampleSum / state->audioSampleCount);
        std::cout << "Player audio OOF length="
                  << (state->verificationSound
                        ? state->verificationSound->getSoundLength() : 0.0)
                  << " mixer-rate="
                  << (soundService ? soundService->getSampleRate() : -1)
                  << " unit-duration=" << unitDuration
                  << " half-duration=" << halfDuration
                  << " loops=" << state->audioObservedLoops
                  << " mixer-time="
                  << (soundService ? soundService->getMixerTime() : 0.0)
                  << " spatial="
                  << (state->verificationSound && state->verificationSound->isSpatial())
                  << " rms=" << rms << '\n';
        if (!soundService || !soundService->enabled() ||
            !state->verificationSound ||
            !state->verificationSound->isSoundLoaded() ||
            !state->verificationSound->isSpatial() ||
            state->audioLoadedFrame == 0 ||
            unitDuration < 0.39 || unitDuration > 0.45 ||
            halfDuration < 0.78 || halfDuration > 0.90 ||
            state->audioObservedLoops < 2 ||
            soundService->getMixerTime() < 0.1 ||
            !state->verificationAudioWire ||
            !state->verificationAudioWire->getConnected() ||
            !state->verificationAudioFaderWire ||
            !state->verificationAudioFaderWire->getConnected() ||
            !state->verificationAudioDistortionWire ||
            !state->verificationAudioDistortionWire->getConnected() ||
            !state->verificationAudioTremoloWire ||
            !state->verificationAudioTremoloWire->getConnected() ||
            !state->verificationAudioChorusWire ||
            !state->verificationAudioChorusWire->getConnected() ||
            !state->verificationAudioFlangerWire ||
            !state->verificationAudioFlangerWire->getConnected() ||
            !state->verificationAudioCompressorWire ||
            !state->verificationAudioCompressorWire->getConnected() ||
            !state->verificationAudioGateWire ||
            !state->verificationAudioGateWire->getConnected() ||
            !state->verificationAudioLimiterWire ||
            !state->verificationAudioLimiterWire->getConnected() ||
            !state->verificationAudioEqualizerWire ||
            !state->verificationAudioEqualizerWire->getConnected() ||
            !state->verificationAudioFilterWire ||
            !state->verificationAudioFilterWire->getConnected() ||
            !state->verificationAudioPitchShifterWire ||
            !state->verificationAudioPitchShifterWire->getConnected() ||
            !state->verificationAudioEchoWire ||
            !state->verificationAudioEchoWire->getConnected() ||
            !state->verificationAudioReverbWire ||
            !state->verificationAudioReverbWire->getConnected() ||
            !state->verificationAudioAnalyzerWire ||
            !state->verificationAudioAnalyzerWire->getConnected() ||
            !state->verificationAudioMixerWire ||
            !state->verificationAudioMixerWire->getConnected() ||
            !state->verificationAudioSplitterWire ||
            !state->verificationAudioSplitterWire->getConnected() ||
            !state->verificationAudioMixer ||
            state->verificationAudioMixer->getLayout() !=
                RBX::Soundscape::AUDIO_CHANNEL_STEREO ||
            !state->verificationAudioSplitter ||
            state->verificationAudioSplitter->getLayout() !=
                RBX::Soundscape::AUDIO_CHANNEL_STEREO ||
            !state->verificationAudioFader ||
            state->verificationAudioFader->getVolume() != 0.5f ||
            !state->verificationAudioDistortion ||
            state->verificationAudioDistortion->getLevel() != 0.15f ||
            !state->verificationAudioTremolo ||
            state->verificationAudioTremolo->getDepth() != 0.2f ||
            state->verificationAudioTremolo->getFrequency() != 4.0f ||
            !state->verificationAudioChorus ||
            state->verificationAudioChorus->getMix() != 0.1f ||
            !state->verificationAudioFlanger ||
            state->verificationAudioFlanger->getMix() != 0.1f ||
            !state->verificationAudioCompressor ||
            state->verificationAudioCompressor->getRatio() != 2.0f ||
            !state->verificationAudioGate ||
            state->verificationAudioGate->getThreshold() !=
                RBX::NumberRange(-70.0f, -60.0f) ||
            !state->verificationAudioLimiter ||
            state->verificationAudioLimiter->getMaxLevel() != -1.0f ||
            !state->verificationAudioEqualizer ||
            state->verificationAudioEqualizer->getLowGain() != -1.0f ||
            !state->verificationAudioFilter ||
            state->verificationAudioFilter->getFilterType() !=
                RBX::Soundscape::AUDIO_FILTER_LOWPASS_24DB ||
            state->verificationAudioFilter->getFrequency() != 12000.0f ||
            !state->verificationAudioPitchShifter ||
            state->verificationAudioPitchShifter->getPitch() != 1.01f ||
            state->verificationAudioPitchShifter->getWindowSize() !=
                RBX::Soundscape::AUDIO_WINDOW_SMALL ||
            !state->verificationAudioEcho ||
            state->verificationAudioEcho->getDelayTime() != 0.05f ||
            state->verificationAudioEcho->getFeedback() != 0.1f ||
            !state->verificationAudioReverb ||
            state->verificationAudioReverb->getDecayTime() != 0.5f ||
            state->verificationAudioReverb->getWetLevel() != -18.0f ||
            !state->verificationAudioAnalyzer ||
            state->verificationAudioAnalyzer->getPeakLevel() <= 0.0f ||
            state->verificationAudioAnalyzer->getRmsLevel() <= 0.0f ||
            state->verificationAudioAnalyzer->getSpectrum()->empty() ||
            !state->verificationAudioListenerWire ||
            !state->verificationAudioListenerWire->getConnected() ||
            !state->verificationAudioListener ||
            !state->verificationAudioOutput ||
            !state->verificationAudioPlayer ||
            !state->verificationAudioPlayer->getIsReady() ||
            !state->verificationAudioPlayer->getIsPlaying() ||
            state->audioPlayerMaximumPosition < 0.05 ||
            !state->audioScheduledPlayCancelled ||
            state->audioScheduledPlayAction == 0 ||
            rms < 0.001)
            throw std::runtime_error(
                "packaged Player audio did not preserve decoded pitch/playback behavior");
        int channelsBeforeStop = -1;
        int channelsAfterStop = -1;
        soundService->getChannelsPlaying(channelsBeforeStop);
        state->verificationSound->stop();
        soundService->getChannelsPlaying(channelsAfterStop);
        if (state->verificationSound->isPlaying() || channelsBeforeStop < 1 ||
            channelsAfterStop != channelsBeforeStop - 1)
            throw std::runtime_error(
                "packaged Player audio channel did not tear down cleanly");
        state->audioLoopConnection.disconnect();
        state->verificationAudioPlayer->stop();
        if (state->verificationAudioPlayer->getIsPlaying())
            throw std::runtime_error(
                "current AudioPlayer graph did not stop cleanly");
        state->verificationAudioWire->setParent(nullptr);
        state->verificationAudioWire.reset();
        state->verificationAudioFaderWire->setParent(nullptr);
        state->verificationAudioFaderWire.reset();
        state->verificationAudioDistortionWire->setParent(nullptr);
        state->verificationAudioDistortionWire.reset();
        state->verificationAudioTremoloWire->setParent(nullptr);
        state->verificationAudioTremoloWire.reset();
        state->verificationAudioChorusWire->setParent(nullptr);
        state->verificationAudioChorusWire.reset();
        state->verificationAudioFlangerWire->setParent(nullptr);
        state->verificationAudioFlangerWire.reset();
        state->verificationAudioCompressorWire->setParent(nullptr);
        state->verificationAudioCompressorWire.reset();
        state->verificationAudioGateWire->setParent(nullptr);
        state->verificationAudioGateWire.reset();
        state->verificationAudioLimiterWire->setParent(nullptr);
        state->verificationAudioLimiterWire.reset();
        state->verificationAudioEqualizerWire->setParent(nullptr);
        state->verificationAudioEqualizerWire.reset();
        state->verificationAudioFilterWire->setParent(nullptr);
        state->verificationAudioFilterWire.reset();
        state->verificationAudioPitchShifterWire->setParent(nullptr);
        state->verificationAudioPitchShifterWire.reset();
        state->verificationAudioEchoWire->setParent(nullptr);
        state->verificationAudioEchoWire.reset();
        state->verificationAudioReverbWire->setParent(nullptr);
        state->verificationAudioReverbWire.reset();
        state->verificationAudioAnalyzerWire->setParent(nullptr);
        state->verificationAudioAnalyzerWire.reset();
        state->verificationAudioMixerWire->setParent(nullptr);
        state->verificationAudioMixerWire.reset();
        state->verificationAudioSplitterWire->setParent(nullptr);
        state->verificationAudioSplitterWire.reset();
        state->verificationAudioMixer->setParent(nullptr);
        state->verificationAudioMixer.reset();
        state->verificationAudioSplitter->setParent(nullptr);
        state->verificationAudioSplitter.reset();
        state->verificationAudioFader->setParent(nullptr);
        state->verificationAudioFader.reset();
        state->verificationAudioDistortion->setParent(nullptr);
        state->verificationAudioDistortion.reset();
        state->verificationAudioTremolo->setParent(nullptr);
        state->verificationAudioTremolo.reset();
        state->verificationAudioChorus->setParent(nullptr);
        state->verificationAudioChorus.reset();
        state->verificationAudioFlanger->setParent(nullptr);
        state->verificationAudioFlanger.reset();
        state->verificationAudioCompressor->setParent(nullptr);
        state->verificationAudioCompressor.reset();
        state->verificationAudioGate->setParent(nullptr);
        state->verificationAudioGate.reset();
        state->verificationAudioLimiter->setParent(nullptr);
        state->verificationAudioLimiter.reset();
        state->verificationAudioEqualizer->setParent(nullptr);
        state->verificationAudioEqualizer.reset();
        state->verificationAudioFilter->setParent(nullptr);
        state->verificationAudioFilter.reset();
        state->verificationAudioPitchShifter->setParent(nullptr);
        state->verificationAudioPitchShifter.reset();
        state->verificationAudioEcho->setParent(nullptr);
        state->verificationAudioEcho.reset();
        state->verificationAudioReverb->setParent(nullptr);
        state->verificationAudioReverb.reset();
        state->verificationAudioAnalyzer->setParent(nullptr);
        state->verificationAudioAnalyzer.reset();
        state->verificationAudioPlayer->setParent(nullptr);
        state->verificationAudioPlayer.reset();
        if (RBX::Soundscape::SoundService* automaticListenerService =
                RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(
                    state->dataModel.get()))
            automaticListenerService->setDefaultListenerLocation(
                RBX::Soundscape::NoDefaultListener);
        state->verificationAudioListenerWire.reset();
        state->verificationAudioOutput.reset();
        state->verificationAudioListener.reset();
        state->verificationGraphEmitter->setParent(nullptr);
        state->verificationGraphEmitter.reset();
        state->verificationSound->setParent(nullptr);
        state->verificationSound.reset();
        state->verificationAudioEmitter->setParent(nullptr);
        state->verificationAudioEmitter.reset();
    }
    if (state->verifiesPlaceAudio) {
        RBX::DataModel::LegacyLock lock(
            state->dataModel.get(), RBX::DataModelJob::Write);
        RBX::Soundscape::SoundService* soundService =
            RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(
                state->dataModel.get());
        const double rms = state->placeAudioSampleCount == 0 ? 0.0 :
            std::sqrt(state->placeAudioSquaredSampleSum /
                state->placeAudioSampleCount);
        unsigned loadedCount = 0;
        unsigned spatialCount = 0;
        unsigned advancedCount = 0;
        std::cout << "selected-place audio ids=";
        for (std::size_t index = 0; index < state->placeSounds.size(); ++index) {
            const auto& sound = state->placeSounds[index];
            loadedCount += index < state->placeSoundObservedLoaded.size() &&
                    state->placeSoundObservedLoaded[index]
                ? 1U : 0U;
            spatialCount += sound->isSpatial() ? 1U : 0U;
            advancedCount += state->placeSoundMaximumPositions[index] > 0.01
                ? 1U : 0U;
            std::cout << (index == 0 ? "" : ",")
                      << sound->getSoundId().toString()
                      << '@' << state->placeSoundMaximumPositions[index];
        }
        std::cout << " loaded=" << loadedCount
                  << " spatial=" << spatialCount
                  << " advanced=" << advancedCount
                  << " loaded-frame=" << state->placeAudioLoadedFrame
                  << " rms=" << rms << '\n';
        if (!soundService || !soundService->enabled() ||
            state->placeSounds.size() != 3 || loadedCount != 3 ||
            spatialCount != 3 || advancedCount != 3 ||
            state->placeAudioLoadedFrame == 0 || rms < 0.001)
            throw std::runtime_error(
                "selected place authored audio did not decode, play spatially, and mix");
        int channelsBeforeStop = -1;
        int channelsAfterStop = -1;
        unsigned playingCount = 0;
        for (const auto& sound : state->placeSounds)
            playingCount += sound->isPlaying() ? 1U : 0U;
        soundService->getChannelsPlaying(channelsBeforeStop);
        for (const auto& sound : state->placeSounds)
            sound->stop();
        soundService->getChannelsPlaying(channelsAfterStop);
        if (playingCount < 2 || channelsBeforeStop < static_cast<int>(playingCount) ||
            channelsAfterStop != channelsBeforeStop - static_cast<int>(playingCount))
            throw std::runtime_error(
                "selected place authored audio channels did not tear down cleanly");
        state->placeSounds.clear();
        state->placeSoundObservedLoaded.clear();
        for (const auto& emitter : state->placeAudioEmitters)
            emitter->setParent(nullptr);
        state->placeAudioEmitters.clear();
    }
    if (state->verifiesSkybox) {
        RBX::Graphics::Sky* sky =
            state->visualEngine->getSceneManager()->getSky();
        if (!sky || !sky->isReady())
            throw std::runtime_error(
                "the serialized custom skybox did not become render-ready");
        std::cout << "skybox faces";
        for (unsigned int index = 0; index < 6; ++index) {
            const RBX::Graphics::TextureRef& face = sky->getSkyBoxFace(index);
            const RBX::Graphics::ImageInfo& info = face.getInfo();
            std::cout << ' ' << index << '=' << face.getStatus() << ':'
                      << info.width << 'x' << info.height;
            if (face.getStatus() != RBX::Graphics::TextureRef::Status_Loaded ||
                info.width == 0 || info.height == 0)
                throw std::runtime_error(
                    "a serialized custom skybox face failed to load");
        }
        std::cout << '\n';
    }
    if (state->verifiesShadowMap) {
        RBX::DataModel::LegacyLock lock(
            state->dataModel.get(), RBX::DataModelJob::Read);
        RBX::Lighting* lighting =
            RBX::ServiceProvider::find<RBX::Lighting>(state->dataModel.get());
        RBX::Graphics::SceneManager* scene =
            state->visualEngine->getSceneManager();
        std::cout << "ShadowMap proof technology="
                  << (lighting ? lighting->getTechnology() : -1)
                  << " softness="
                  << (lighting ? lighting->getShadowSoftness() : -1.0f)
                  << " size=" << state->shadowMapWidth
                  << " cascades=" << state->shadowCascadeCount
                  << " splits=" << state->shadowCascadeInfo.x << ','
                  << state->shadowCascadeInfo.y
                  << " caster-batches=" << state->shadowCasterBatches
                  << " no-cast-batches=" << state->shadowNoCastBatches
                  << " tiers=" << state->shadowLowQualityVerified << ','
                  << state->shadowMediumQualityVerified
                  << " darkened-pixels=" << state->shadowDarkenedPixels << '\n';
        if (!lighting || !lighting->getGlobalShadows() ||
            !scene->isShadowMapEnabled() || state->shadowCasterBatches == 0 ||
            state->shadowNoCastBatches >= state->shadowCasterBatches ||
            state->shadowMapWidth < 256 || state->shadowCascadeCount != 3 ||
            !state->shadowLowQualityVerified ||
            !state->shadowMediumQualityVerified ||
            state->shadowDarkenedPixels < 64U)
            throw std::runtime_error(
                "ShadowMap verification did not restore a live configured pass");
    }
    if (state->verifiesSurfaceTextures) {
        const RBX::Graphics::ImageInfo& baseplateInfo =
            state->baseplateSurfaceTexture.getInfo();
        const RBX::Graphics::ImageInfo& spawnInfo = state->spawnSurfaceTexture.getInfo();
        std::cout << "surface textures Baseplate status="
                  << state->baseplateSurfaceTexture.getStatus() << " size="
                  << baseplateInfo.width << 'x' << baseplateInfo.height
                  << " SpawnLocation status=" << state->spawnSurfaceTexture.getStatus()
                  << " size=" << spawnInfo.width << 'x' << spawnInfo.height << '\n';
        if (state->baseplateSurfaceTexture.getStatus() !=
                RBX::Graphics::TextureRef::Status_Loaded ||
            baseplateInfo.width != 400 || baseplateInfo.height != 400)
            throw std::runtime_error(
                "official Baseplate surface texture did not load at its authentic dimensions");
        if (state->spawnSurfaceTexture.getStatus() !=
                RBX::Graphics::TextureRef::Status_Loaded ||
            spawnInfo.width != 64 || spawnInfo.height != 64)
            throw std::runtime_error(
                "packaged SpawnLocation surface texture did not load at its authentic dimensions");
    }
    if (!state->verifiesCaptureGallery)
        return;
    if (!state->captureVerified || !state->captureThumbnailVisible ||
        !state->captureThumbnailPixelsVerified ||
        state->verificationCapturePath.empty() ||
        !std::filesystem::is_regular_file(state->verificationCapturePath))
        throw std::runtime_error(
            "CaptureService verification capture did not survive and render through the UI proof");

    RBX::DataModel::LegacyLock lock(
        state->dataModel.get(), RBX::DataModelJob::Write);
    RBX::CaptureService* captureService =
        RBX::ServiceProvider::find<RBX::CaptureService>(state->dataModel.get());
    if (!captureService)
        throw std::runtime_error(
            "CaptureService disappeared before verification cleanup");

    boost::shared_ptr<RBX::Reflection::ValueArray> captures(
        new RBX::Reflection::ValueArray());
    captures->push_back(RBX::Reflection::Variant(
        state->verificationCapturePath.string()));
    bool deletionReturned = false;
    long long deleted = 0;
    captureService->deleteCapturesAsync(
        captures,
        [&](long long value) {
            deleted = value;
            deletionReturned = true;
        },
        [](std::string error) {
            throw std::runtime_error("capture deletion failed: " + error);
        });
    if (!deletionReturned || deleted != 1 ||
        std::filesystem::exists(state->verificationCapturePath))
        throw std::runtime_error(
            "CaptureService did not delete its rendered verification capture");

    std::cout << "CaptureService rendered gallery lifecycle="
              << static_cast<unsigned int>(state->verificationCaptureSize.x)
              << 'x'
              << static_cast<unsigned int>(state->verificationCaptureSize.y)
              << " bytes=" << state->verificationCaptureBytes
              << " visible-proof=verified delete=verified\n";
    state->verificationCapturePath.clear();
}

} // namespace rbx::player
