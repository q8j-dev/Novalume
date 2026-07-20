#include "PlayerRuntime.h"
#include "rbx/core/BuildInfo.h"
#include "rbx/assets/AssetMountTable.h"
#include "rbx/assets/PlacePackage.h"
#include "rbx/platform/Host.h"
#include "GfxCore/Device.h"
#include "V8DataModel/LocalStorageService.h"
#include "util/Http.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#ifndef RBX_LEGACY_UI_TEST
#define RBX_LEGACY_UI_TEST 0
#endif

int main(int argc, char** argv) {
    try {
        struct HttpShutdownGuard {
            ~HttpShutdownGuard() { RBX::Http::shutdown(); }
        } httpShutdownGuard;
        bool useCurrentInExperienceUi = RBX_LEGACY_UI_TEST == 0;
        bool headlessVerify = false;
        bool verifyPlayerList = false;
        bool verifyChromeInteraction = false;
        bool verifyChromeLeaderboard = false;
        bool verifyReport = false;
        bool verifyRespawn = false;
        bool verifySwitchAvatar = false;
        bool verifyPeoplePage = false;
        bool verifyExperienceChat = false;
        bool verifyCaptureGallery = false;
        bool verifyViewportRendering = false;
        bool verifySurfaceTextures = false;
        bool verifyShadowMap = false;
        bool verifySkybox = false;
        bool verifyAudio = false;
        bool verifyPlaceAudio = false;
        bool verifyTextRendering = false;
        bool verifyLauncher = false;
        bool verifyPlaceVisual = false;
        rbx::player::AvatarRigVariant avatarRig =
            rbx::player::AvatarRigVariant::R15;
        std::optional<std::filesystem::path> videoVerificationPath;
        std::optional<std::filesystem::path> placePath;
        std::optional<std::filesystem::path> renderProofPath;
        std::optional<int> requestedFrameLimit;
        for (int index = 1; index < argc; ++index) {
            const std::string_view argument(argv[index]);
            headlessVerify |= argument == "--headless-verify";
            verifyPlayerList |= argument == "--verify-player-list";
            verifyChromeInteraction |= argument == "--verify-chrome-interaction";
            verifyChromeLeaderboard |= argument == "--verify-chrome-leaderboard";
            verifyReport |= argument == "--verify-report";
            verifyRespawn |= argument == "--verify-respawn";
            verifySwitchAvatar |= argument == "--verify-switch-avatar";
            verifyPeoplePage |= argument == "--verify-people-page";
            verifyExperienceChat |= argument == "--verify-experience-chat";
            verifyCaptureGallery |= argument == "--verify-capture-gallery";
            verifyViewportRendering |= argument == "--verify-viewport-rendering";
            verifySurfaceTextures |= argument == "--verify-surface-textures";
            verifyShadowMap |= argument == "--verify-shadow-map";
            verifySkybox |= argument == "--verify-skybox";
            verifyAudio |= argument == "--verify-audio";
            verifyPlaceAudio |= argument == "--verify-place-audio";
            verifyTextRendering |= argument == "--verify-text-rendering";
            verifyLauncher |= argument == "--verify-launcher";
            verifyPlaceVisual |= argument == "--verify-place-visual";
            if (argument == "--r15")
                avatarRig = rbx::player::AvatarRigVariant::R15;
			else if (argument == "--r15-plus")
				avatarRig = rbx::player::AvatarRigVariant::R15Plus;
            else if (argument == "--rthro-normal")
                avatarRig = rbx::player::AvatarRigVariant::RthroNormal;
            else if (argument == "--rthro-slender")
                avatarRig = rbx::player::AvatarRigVariant::RthroSlender;
            if (argument == "--place" && index + 1 < argc) {
                placePath = argv[++index];
            } else if (argument == "--render-proof" && index + 1 < argc) {
                renderProofPath = argv[++index];
            } else if (argument == "--frame-limit" && index + 1 < argc) {
                requestedFrameLimit = std::stoi(argv[++index]);
                if (*requestedFrameLimit <= 0)
                    throw std::runtime_error("--frame-limit requires a positive integer");
            } else if (argument == "--verify-video-rendering" && index + 1 < argc) {
                videoVerificationPath = argv[++index];
            } else if (!argument.starts_with('-')) {
                placePath = argv[index];
            }
        }
        const bool useDurangoLauncher = verifyLauncher ||
            (!headlessVerify && !placePath.has_value());
        useCurrentInExperienceUi = useCurrentInExperienceUi &&
            !useDurangoLauncher;
        verifyChromeInteraction |= verifyChromeLeaderboard;
        verifyChromeInteraction |= verifyReport;
        verifyChromeInteraction |= verifyRespawn;
        verifyChromeInteraction |= verifySwitchAvatar;
        verifyChromeInteraction |= verifyExperienceChat;
        if (verifyCaptureGallery && !renderProofPath)
            throw std::runtime_error(
                "--verify-capture-gallery requires --render-proof for pixel verification");
        if (placePath) {
            const auto extension = placePath->extension().string();
            if (!std::filesystem::is_regular_file(*placePath) ||
                (extension != ".rbxl" && extension != ".rbxlx" &&
                 extension != ".rbxm" && extension != ".rbxmx" &&
                 extension != ".rbxlp")) {
                throw std::runtime_error(
                    "--place requires an existing Roblox place/model or RBXLP package");
            }
        }
        if (verifyPlaceAudio && !placePath)
            throw std::runtime_error("--verify-place-audio requires --place");

        auto host = rbx::platform::createHost(1280, 720, !headlessVerify);
        RBX::LocalStorageService::setStorageRoot(
            host->writableDataRoot() / "RobloxPlayer");
        std::optional<RBX::Assets::MaterializedPlacePackage> materializedPlace;
        std::optional<std::filesystem::path> resolvedPlacePath = placePath;
        if (placePath && RBX::Assets::isPlacePackage(*placePath)) {
            materializedPlace = RBX::Assets::materializePlacePackage(
                *placePath, host->writableDataRoot() / "RobloxPlayer" /
                    "PlacePackages");
            resolvedPlacePath = materializedPlace->place;
        }
        const auto resources = host->resourceRoot();
        const auto manifest = resources / "resource-manifest.json";
        const auto uiOverlayManifest = resources / "overlays" / "player-2026" /
                                       "overlay-manifest.json";
        const auto metalUiShader = resources / "shaders" / "metal" /
                                   "vs_player_ui.sc.bin";
        if (!std::filesystem::is_regular_file(manifest) ||
            std::filesystem::file_size(manifest) == 0 ||
            (useCurrentInExperienceUi &&
             (!std::filesystem::is_regular_file(uiOverlayManifest) ||
              std::filesystem::file_size(uiOverlayManifest) == 0)) ||
            !std::filesystem::is_regular_file(metalUiShader)) {
            throw std::runtime_error("packaged resource, in-game UI overlay, or Metal shader manifest is missing");
        }

        auto& assetMounts = RBX::Assets::assetMountTable();
        assetMounts.clear();
        assetMounts.addMount("content", resources / "content", 0);
        const auto avatarRuntime =
            resources / "overlays" / "avatar-runtime" / "content";
        if (std::filesystem::is_directory(avatarRuntime))
            assetMounts.addMount("avatar-runtime", avatarRuntime, 120);
        if (useCurrentInExperienceUi) {
            assetMounts.addMount("player-core",
                resources / "overlays" / "player-2026" / "content", 100);
            assetMounts.addMount("player-extra",
                resources / "overlays" / "player-2026" / "ExtraContent", 110);
        }
        assetMounts.addMount("platform", resources / "PlatformContent" / "pc", 200);
        if (materializedPlace)
            assetMounts.addMount("embedded-place", materializedPlace->root, 1000);

        const auto surface = host->nativeSurface();
        const auto playerListImage = assetMounts.resolve(
            "rbxasset://textures/ui/PlayerList/ViewAvatar.png", surface.pixelDensity);
        if (useCurrentInExperienceUi &&
            (!playerListImage || playerListImage->mountName == "content")) {
            throw std::runtime_error("packaged 2026 Player UI overlay did not win rbxasset precedence");
        }
        if (playerListImage) {
            std::cout << "player UI asset=" << playerListImage->logicalPath
                      << " mount=" << playerListImage->mountName
                      << " density=" << playerListImage->densityScale << '\n';
        }
        const RBX::Graphics::DeviceWindow deviceWindow{
            .windowHandle = reinterpret_cast<void*>(surface.window),
            .displayHandle = reinterpret_cast<void*>(surface.display),
            .graphicsContext = reinterpret_cast<void*>(surface.graphicsContext),
            .width = surface.width,
            .height = surface.height,
            .pixelDensity = surface.pixelDensity,
            .renderer = RBX::Graphics::DeviceWindow::Renderer::Metal};
        std::unique_ptr<RBX::Graphics::Device> device(
            RBX::Graphics::Device::create(RBX::Graphics::Device::API_Bgfx, deviceWindow));
        if (!device->validate() || device->getFeatureLevel() != "Metal") {
            throw std::runtime_error("RobloxPlayer requires bgfx Metal on macOS");
        }
        std::cout << rbx::core::BuildInfo::productName << " renderer="
                  << device->getAPIName() << '/' << device->getFeatureLevel() << '\n';

        const std::filesystem::path runtimePlace = resolvedPlacePath
            ? *resolvedPlacePath
            : resources / "places" / "Baseplate.rbxl";
        if (!std::filesystem::is_regular_file(runtimePlace))
            throw std::runtime_error("packaged default Baseplate.rbxl is missing: " +
                                     runtimePlace.string());
        std::cout << "place=" << runtimePlace << '\n';
        const auto runtimeLoadStart = std::chrono::steady_clock::now();
        rbx::player::PlayerRuntime runtime(device.get(), resources,
            host->existingClientSettingsRoot(), runtimePlace,
            surface.width, surface.height, surface.logicalWidth,
            surface.logicalHeight, headlessVerify, useCurrentInExperienceUi,
            useDurangoLauncher,
            avatarRig, verifyViewportRendering,
            videoVerificationPath.value_or(std::filesystem::path()),
            verifyPeoplePage, verifyExperienceChat, verifyCaptureGallery,
            verifyChromeLeaderboard, verifyReport, verifyRespawn,
            verifySwitchAvatar, verifySurfaceTextures, verifyShadowMap,
            verifySkybox, verifyAudio, verifyPlaceAudio, verifyTextRendering,
            verifyPlaceVisual);
        const double runtimeLoadMilliseconds =
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - runtimeLoadStart).count();

        const int frameLimit = requestedFrameLimit.value_or(headlessVerify ? 300 : -1);
        int frame = 0;
        std::vector<double> headlessFrameMilliseconds;
        if (headlessVerify && frameLimit > 60)
            headlessFrameMilliseconds.reserve(static_cast<std::size_t>(
                std::min(frameLimit, 245) - 60));
        while (host->pumpEvents() && (frameLimit < 0 || frame < frameLimit)) {
            const auto frameStart = std::chrono::steady_clock::now();
            for (const auto& event : host->takeInputEvents())
                runtime.handleInput(event);
            host->setPointerLock(runtime.wantsPointerLock());
            if (headlessVerify && verifyLauncher && frame == 100) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::enter});
            } else if (headlessVerify && verifyLauncher && frame == 101) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyUp,
                    .key = rbx::platform::InputEvent::Key::enter});
            } else if (headlessVerify && !verifyLauncher && !verifySurfaceTextures && !verifyShadowMap &&
                !verifySkybox && !verifyTextRendering && !verifyPlaceVisual &&
                (frame == 60 || frame == 70 || frame == 80 || frame == 90)) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::w,
                    .text = 'w'});
            } else if (headlessVerify && !verifyLauncher && !verifySurfaceTextures && !verifyShadowMap &&
                       !verifySkybox && !verifyTextRendering && !verifyPlaceVisual && frame == 100) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyUp,
                    .key = rbx::platform::InputEvent::Key::w,
                    .text = 'w'});
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::d,
                    .text = 'd'});
            } else if (headlessVerify && !verifyLauncher && !verifySurfaceTextures && !verifyShadowMap &&
                       !verifySkybox && !verifyTextRendering && !verifyPlaceVisual &&
                       (frame == 110 || frame == 120 || frame == 130)) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::d,
                    .text = 'd'});
            } else if (headlessVerify && !verifyLauncher && !verifySurfaceTextures && !verifyShadowMap &&
                       !verifySkybox && !verifyTextRendering && !verifyPlaceVisual && frame == 140) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyUp,
                    .key = rbx::platform::InputEvent::Key::d,
                    .text = 'd'});
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::s,
                    .text = 's'});
            } else if (headlessVerify && !verifyLauncher && !verifySurfaceTextures && !verifyShadowMap &&
                       !verifySkybox && !verifyTextRendering && !verifyPlaceVisual &&
                       (frame == 150 || frame == 160 || frame == 170 ||
                        frame == 181 || frame == 201 || frame == 221)) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::s,
                    .text = 's'});
            } else if (headlessVerify && !verifyLauncher && !verifySurfaceTextures && !verifyShadowMap &&
                       !verifySkybox && !verifyTextRendering && !verifyPlaceVisual && frame == 230) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyUp,
                    .key = rbx::platform::InputEvent::Key::s,
                    .text = 's'});
            } else if (headlessVerify && verifyExperienceChat && frame == 100) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 138.0F, .y = 34.0F});
            } else if (headlessVerify && verifyExperienceChat && frame == 101) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 138.0F, .y = 34.0F});
            } else if (headlessVerify && verifyExperienceChat && frame == 140) {
                // Exercise both authentic Chrome toggle directions. ExperienceChat
                // starts visible when ChatVisible is enabled, so the first action
                // closes it and this second action must reopen it.
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 138.0F, .y = 34.0F});
            } else if (headlessVerify && verifyExperienceChat && frame == 141) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 138.0F, .y = 34.0F});
            } else if (headlessVerify && verifyExperienceChat && frame == 160) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 100.0F, .y = 263.0F});
            } else if (headlessVerify && verifyExperienceChat && frame == 161) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 100.0F, .y = 263.0F});
            } else if (headlessVerify && verifyExperienceChat && frame == 165) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::h, .text = 'h'});
            } else if (headlessVerify && verifyExperienceChat && frame == 166) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyUp,
                    .key = rbx::platform::InputEvent::Key::h, .text = 'h'});
            } else if (headlessVerify && verifyExperienceChat && frame == 167) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::i, .text = 'i'});
            } else if (headlessVerify && verifyExperienceChat && frame == 168) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyUp,
                    .key = rbx::platform::InputEvent::Key::i, .text = 'i'});
            } else if (headlessVerify && verifyExperienceChat && frame == 170) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::enter});
            } else if (headlessVerify && verifyExperienceChat && frame == 171) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyUp,
                    .key = rbx::platform::InputEvent::Key::enter});
            } else if (headlessVerify && verifyChromeInteraction && frame == 100) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 94.0F, .y = 34.0F});
            } else if (headlessVerify && verifyChromeInteraction && frame == 101) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 94.0F, .y = 34.0F});
            } else if (headlessVerify && verifyChromeLeaderboard && frame == 260) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 170.0F, .y = 264.0F});
            } else if (headlessVerify && verifyChromeLeaderboard && frame == 261) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 170.0F, .y = 264.0F});
            } else if (headlessVerify && verifyReport && frame == 260) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 170.0F, .y = 92.0F});
            } else if (headlessVerify && verifyReport && frame == 261) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 170.0F, .y = 92.0F});
            } else if (headlessVerify && verifyRespawn && frame == 260) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 170.0F, .y = 376.0F});
            } else if (headlessVerify && verifyRespawn && frame == 261) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 170.0F, .y = 376.0F});
            } else if (headlessVerify && verifySwitchAvatar && frame == 260) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 170.0F, .y = 136.0F});
            } else if (headlessVerify && verifySwitchAvatar && frame == 261) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 170.0F, .y = 136.0F});
            } else if (headlessVerify && verifyRespawn && frame == 320) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 530.0F, .y = 462.0F});
            } else if (headlessVerify && verifyRespawn && frame == 321) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 530.0F, .y = 462.0F});
            } else if (headlessVerify && verifyChromeLeaderboard && frame == 330) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 1125.0F, .y = 72.0F});
            } else if (headlessVerify && verifyChromeLeaderboard && frame == 331) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 1125.0F, .y = 72.0F});
            } else if (headlessVerify && verifyChromeLeaderboard && frame == 350) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 94.0F, .y = 34.0F});
            } else if (headlessVerify && verifyChromeLeaderboard && frame == 351) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 94.0F, .y = 34.0F});
            } else if (headlessVerify && verifyChromeLeaderboard && frame == 370) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 170.0F, .y = 264.0F});
            } else if (headlessVerify && verifyChromeLeaderboard && frame == 371) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 170.0F, .y = 264.0F});
            } else if (headlessVerify && !verifyLauncher && frame == 185) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::secondary,
                    .x = 640.0F, .y = 360.0F});
            } else if (headlessVerify && !verifyLauncher && frame > 185 && frame <= 235) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerMove,
                    // Preserve both halves of the desktop pointer contract:
                    // an absolute cursor position that actually advances and
                    // its matching relative delta.  Camera capture can become
                    // active a frame after the button-down, so a stationary
                    // absolute position made this proof scheduler-dependent.
                    .x = 640.0F + static_cast<float>(frame - 185) * 8.0F,
                    .y = 360.0F,
                    .deltaX = 8.0F, .deltaY = 0.0F});
            } else if (headlessVerify && !verifyLauncher && frame == 236) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::secondary,
                    .x = 640.0F, .y = 360.0F});
            } else if (headlessVerify && !verifyLauncher && !verifyPlayerList &&
                       !verifyChromeInteraction && !verifyPlaceVisual &&
                       !placePath && frame == 245) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::escape});
            } else if (headlessVerify && !verifyLauncher && !verifyPlayerList &&
                       !verifyChromeInteraction && !verifyPlaceVisual &&
                       !placePath && frame == 246) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyUp,
                    .key = rbx::platform::InputEvent::Key::escape});
            } else if (headlessVerify && !verifyLauncher && !verifyPlayerList &&
                       !verifyChromeInteraction && !verifyPeoplePage &&
                       !verifyPlaceVisual && !placePath && frame == 275) {
                // Select the genuine Settings tab after its opening tween.
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 340.0F, .y = 105.0F});
            } else if (headlessVerify && !verifyLauncher && !verifyPlayerList &&
                       !verifyChromeInteraction && !verifyPeoplePage &&
                       !verifyPlaceVisual && !placePath && frame == 276) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 340.0F, .y = 105.0F});
            } else if (headlessVerify && verifyPlayerList && frame == 250) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyDown,
                    .key = rbx::platform::InputEvent::Key::tab});
            } else if (headlessVerify && verifyPlayerList && frame == 251) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::keyUp,
                    .key = rbx::platform::InputEvent::Key::tab});
            } else if (headlessVerify && verifyPeoplePage && frame == 260) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerDown,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 536.0F, .y = 407.0F});
            } else if (headlessVerify && verifyPeoplePage && frame == 261) {
                runtime.handleInput(rbx::platform::InputEvent{
                    .kind = rbx::platform::InputEvent::Kind::pointerUp,
                    .button = rbx::platform::InputEvent::PointerButton::primary,
                    .x = 536.0F, .y = 407.0F});
            }
            try {
                runtime.renderFrame(static_cast<unsigned long>(frame));
            } catch (...) {
                // Preserve the last completed GPU frame for diagnosing a failed
                // headless verification instead of losing the visual evidence.
                if (renderProofPath)
                    runtime.writeFrameProof(*renderProofPath);
                throw;
            }
            ++frame;
            // Keep the steady gameplay window separate from the intentionally
            // blocking final semantic inspection and post-245 menu/emote
            // interactions. Those remain correctness gates, not frame-pacing
            // samples.
            if (headlessVerify && frame > 60 && frame <= 245)
                headlessFrameMilliseconds.push_back(
                    std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - frameStart).count());
            std::this_thread::sleep_for(
                std::chrono::milliseconds(headlessVerify ? 5 : 1));
        }
        const RBX::Graphics::DeviceStats rendererStats = device->getStatistics();
        std::cout << "rendered frames=" << frame
                  << " last-frame draws=" << rendererStats.drawCalls << '\n';
        if (!headlessFrameMilliseconds.empty()) {
            std::sort(headlessFrameMilliseconds.begin(),
                headlessFrameMilliseconds.end());
            const auto percentile = [&headlessFrameMilliseconds](double value) {
                const std::size_t index = static_cast<std::size_t>(
                    value * static_cast<double>(headlessFrameMilliseconds.size() - 1));
                return headlessFrameMilliseconds[index];
            };
            const std::size_t stutters16 = static_cast<std::size_t>(std::count_if(
                headlessFrameMilliseconds.begin(), headlessFrameMilliseconds.end(),
                [](double milliseconds) { return milliseconds > 16.6667; }));
            const std::size_t stutters33 = static_cast<std::size_t>(std::count_if(
                headlessFrameMilliseconds.begin(), headlessFrameMilliseconds.end(),
                [](double milliseconds) { return milliseconds > 33.3333; }));
            std::cout << "headless performance warmup=60 samples="
                      << headlessFrameMilliseconds.size()
                      << " load-ms=" << runtimeLoadMilliseconds
                      << " frame-p50-ms=" << percentile(0.50)
                      << " frame-p95-ms=" << percentile(0.95)
                      << " frame-p99-ms=" << percentile(0.99)
                      << " over-16.67-ms=" << stutters16
                      << " over-33.33-ms=" << stutters33
                      << " resolution=" << surface.width << 'x' << surface.height
                      << " logical=" << surface.logicalWidth << 'x'
                      << surface.logicalHeight << " renderer=Metal\n";
        }
        if (renderProofPath) {
            runtime.writeFrameProof(*renderProofPath);
            std::cout << "render proof=" << *renderProofPath << '\n';
        }
        runtime.finishVerification();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "RobloxPlayer: " << error.what() << '\n';
        return 1;
    }
}
