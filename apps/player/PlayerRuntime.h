#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "rbx/platform/Host.h"

namespace RBX::Graphics {
class Device;
}

namespace rbx::player {

enum class AvatarRigVariant
{
    R6,
    R15,
	R15Plus,
    RthroNormal,
    RthroSlender,
};

class PlayerRuntime final {
public:
    PlayerRuntime(RBX::Graphics::Device* device,
                  const std::filesystem::path& resourceRoot,
                  const std::filesystem::path& clientSettingsRoot,
                  const std::filesystem::path& placePath,
                  unsigned int renderWidth, unsigned int renderHeight,
                  unsigned int logicalWidth, unsigned int logicalHeight,
                  bool disableAudioOutput, bool useCurrentInExperienceUi,
                  bool useDurangoLauncher = false,
                  AvatarRigVariant avatarRig = AvatarRigVariant::R15,
                  bool verifyViewportRendering = false,
                  const std::filesystem::path& videoVerificationPath = {},
                  bool verifyPeoplePage = false,
                  bool verifyExperienceChat = false,
                  bool verifyCaptureGallery = false,
                  bool verifyChromeLeaderboard = false,
                  bool verifyReport = false,
                  bool verifyRespawn = false,
                  bool verifySwitchAvatar = false,
                  bool verifySurfaceTextures = false,
                  bool verifyShadowMap = false,
                  bool verifySkybox = false,
                  bool verifyAudio = false,
                  bool verifyPlaceAudio = false,
                  bool verifyTextRendering = false,
                  bool verifyPlaceVisual = false,
                  const std::vector<std::filesystem::path>& recentDocuments = {});
    ~PlayerRuntime();

    PlayerRuntime(const PlayerRuntime&) = delete;
    PlayerRuntime& operator=(const PlayerRuntime&) = delete;

    void resize(unsigned int renderWidth, unsigned int renderHeight,
                unsigned int logicalWidth, unsigned int logicalHeight,
                float pixelDensity);
    void renderFrame(unsigned long frameNumber);
    void handleInput(const rbx::platform::InputEvent& event);
    [[nodiscard]] bool wantsPointerLock() const;
    [[nodiscard]] bool takeOpenDocumentRequest();
    [[nodiscard]] std::optional<std::filesystem::path>
        takeRecentDocumentRequest();
    void writeFrameProof(const std::filesystem::path& outputPath);
    void finishVerification();

private:
    struct State;
    std::unique_ptr<State> state;
};

} // namespace rbx::player
