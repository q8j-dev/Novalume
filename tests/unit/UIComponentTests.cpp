#include "V8DataModel/CanvasGroup.h"
#include "V8DataModel/CoreGuiConfiguration.h"
#include "V8DataModel/ContextActionService.h"
#include "V8DataModel/Frame.h"
#include "V8DataModel/UIComponent.h"
#include "V8DataModel/TextService.h"
#include "V8DataModel/TextLabel.h"
#include "V8DataModel/TextBox.h"
#include "V8DataModel/TextChatService.h"
#include "V8DataModel/TextChannel.h"
#include "V8DataModel/TextChatConfiguration.h"
#include "V8DataModel/ChatService.h"
#include "V8DataModel/RbxAnalyticsService.h"
#include "Network/Player.h"
#include "V8DataModel/StyleSheet.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/BillboardGui.h"
#include "V8DataModel/PlayerScripts.h"
#include "V8DataModel/WorldModel.h"
#include "V8DataModel/ViewportFrame.h"
#include "V8DataModel/VideoFrame.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/CaptureService.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/ExperienceNotificationService.h"
#include "V8DataModel/Animator.h"
#include "Humanoid/Humanoid.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PhysicsService.h"
#include "V8Xml/Serializer.h"
#include "Script/ScriptContext.h"
#include "Script/Script.h"
#include "Util/ProtectedString.h"
#include "Util/Content.h"
#include "Util/Font.h"
#include "Util/RunStateOwner.h"
#include "V8World/Primitive.h"
#include "GfxRender/GeometryGenerator.h"
#include "rbx/ui/ScreenLayout.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool near(float left, float right)
{
    return std::abs(left - right) < 0.01f;
}
}

int main()
{
    using namespace RBX;

    boost::shared_ptr<Animator> currentAnimator = Creatable<Instance>::create<Animator>();
    require(currentAnimator->findFunctionDescriptor("GetPlayingAnimationTracks") != nullptr &&
            currentAnimator->findFunctionDescriptor("GetPlayingAnimationTracksCoreScript") != nullptr,
        "Animator must expose both current public and CoreScript playing-track queries");
    require(currentAnimator->getDescriptor().findEventDescriptor("AnimationPlayed") != nullptr &&
            currentAnimator->getDescriptor().findEventDescriptor("AnimationPlayedCoreScript") != nullptr &&
            currentAnimator->getDescriptor().findEventDescriptor("AnimationStreamTrackPlayed") != nullptr,
        "Animator must expose the current public and CoreScript playback events");

    boost::shared_ptr<Humanoid> currentHumanoid = Creatable<Instance>::create<Humanoid>();
    require(currentHumanoid->findPropertyDescriptor("AutomaticScalingEnabled") != nullptr &&
            currentHumanoid->getAutomaticScalingEnabled(),
        "Humanoid must expose the enabled-by-default current avatar scaling contract");
    currentHumanoid->setAutomaticScalingEnabled(false);
    require(!currentHumanoid->getAutomaticScalingEnabled(),
        "Humanoid.AutomaticScalingEnabled must retain its selected state");
    int publicAnimationPlayed = 0;
    int coreAnimationPlayed = 0;
    shared_ptr<Instance> playedTrack = Creatable<Instance>::create<PartInstance>();
    rbx::signals::scoped_connection publicAnimationConnection =
        currentAnimator->animationPlayedSignal.connect(
            [&publicAnimationPlayed, &playedTrack](shared_ptr<Instance> track) {
                require(track == playedTrack,
                    "Animator.AnimationPlayed must preserve the exact played track");
                ++publicAnimationPlayed;
            });
    rbx::signals::scoped_connection coreAnimationConnection =
        currentAnimator->animationPlayedCoreScriptSignal.connect(
            [&coreAnimationPlayed, &playedTrack](shared_ptr<Instance> track) {
                require(track == playedTrack,
                    "Animator.AnimationPlayedCoreScript must preserve the exact played track");
                ++coreAnimationPlayed;
            });
    currentAnimator->tellParentAnimationPlayed(playedTrack);
    require(publicAnimationPlayed == 1 && coreAnimationPlayed == 1,
        "one real Animator playback transition must notify both reflected event surfaces once");
    currentAnimator->tellParentAnimationPlayed(shared_ptr<Instance>());
    require(publicAnimationPlayed == 1 && coreAnimationPlayed == 1,
        "Animator must not publish a playback event without a real track");

    boost::shared_ptr<UIListLayout> defaultListLayout =
        Creatable<Instance>::create<UIListLayout>();
    require(defaultListLayout->getFillDirection() == FILL_DIRECTION_VERTICAL,
        "UIListLayout must use the current engine's vertical default fill direction");

    boost::shared_ptr<TextBox> cursorTextBox = Creatable<Instance>::create<TextBox>();
    cursorTextBox->setClearTextOnFocus(false);
    cursorTextBox->setText("a\xc3\xa9");
    require(cursorTextBox->getCursorPosition() == -1 &&
            cursorTextBox->getSelectionStart() == -1,
        "an unfocused TextBox must expose the current engine's inactive cursor state");
    cursorTextBox->captureFocus();
    require(cursorTextBox->getCursorPosition() == 3,
        "TextBox CursorPosition must be one-based and count UTF-8 characters");
    cursorTextBox->setSelectionStart(1);
    cursorTextBox->setCursorPosition(2);
    require(cursorTextBox->getSelectionStart() == 1 &&
            cursorTextBox->getCursorPosition() == 2,
        "TextBox cursor and selection endpoints must retain current one-based positions");
    cursorTextBox->releaseFocusLua();
    require(cursorTextBox->getCursorPosition() == -1 &&
            cursorTextBox->getSelectionStart() == -1,
        "TextBox must clear its public cursor and selection endpoints on focus loss");

    boost::shared_ptr<BillboardGui> bubbleBillboard =
        Creatable<Instance>::create<BillboardGui>();
    require(bubbleBillboard->getStudsOffsetWorldSpace() == Vector3::zero() &&
            bubbleBillboard->findPropertyDescriptor("StudsOffsetWorldSpace") != nullptr,
        "BillboardGui.StudsOffsetWorldSpace must expose the current zero-default reflected contract");
    bubbleBillboard->setStudsOffsetWorldSpace(Vector3(1.0f, 2.0f, 3.0f));
    require(bubbleBillboard->getStudsOffsetWorldSpace() == Vector3(1.0f, 2.0f, 3.0f),
        "BillboardGui.StudsOffsetWorldSpace must retain the global-axis render offset");

    boost::shared_ptr<ModelInstance> emptyBoundsModel =
        Creatable<Instance>::create<ModelInstance>();
    shared_ptr<const Reflection::Tuple> emptyBounds = emptyBoundsModel->getBoundingBox();
    require(emptyBounds->values.size() == 2 &&
            emptyBounds->at(0).get<CoordinateFrame>() == CoordinateFrame() &&
            emptyBounds->at(1).get<Vector3>() == Vector3::zero(),
        "an empty Model:GetBoundingBox must return the identity frame and zero size");

    boost::shared_ptr<ModelInstance> worldBoundsModel =
        Creatable<Instance>::create<ModelInstance>();
    boost::shared_ptr<PartInstance> worldBoundsPart =
        Creatable<Instance>::create<PartInstance>();
    worldBoundsPart->setPartSizeXml(Vector3(2.0f, 4.0f, 6.0f));
    worldBoundsPart->setCoordinateFrame(CoordinateFrame(Vector3(10.0f, 0.0f, 0.0f)));
    worldBoundsPart->setParent(worldBoundsModel.get());
    boost::shared_ptr<ModelInstance> nestedBoundsModel =
        Creatable<Instance>::create<ModelInstance>();
    nestedBoundsModel->setParent(worldBoundsModel.get());
    boost::shared_ptr<PartInstance> nestedBoundsPart =
        Creatable<Instance>::create<PartInstance>();
    nestedBoundsPart->setPartSizeXml(Vector3(4.0f, 2.0f, 2.0f));
    nestedBoundsPart->setCoordinateFrame(CoordinateFrame(Vector3(14.0f, 2.0f, 0.0f)));
    nestedBoundsPart->setParent(nestedBoundsModel.get());
    shared_ptr<const Reflection::Tuple> worldBounds = worldBoundsModel->getBoundingBox();
    const CoordinateFrame worldBoundsFrame = worldBounds->at(0).get<CoordinateFrame>();
    const Vector3 worldBoundsSize = worldBounds->at(1).get<Vector3>();
    require(worldBoundsFrame.rotation == Matrix3::identity() &&
            worldBoundsFrame.translation.fuzzyEq(Vector3(12.5f, 0.5f, 0.0f)) &&
            worldBoundsSize.fuzzyEq(Vector3(7.0f, 5.0f, 6.0f)),
        "Model:GetBoundingBox without a PrimaryPart must use world axes and every descendant BasePart");

    boost::shared_ptr<ModelInstance> orientedBoundsModel =
        Creatable<Instance>::create<ModelInstance>();
    const Matrix3 quarterTurn = Matrix3::fromAxisAngleFast(Vector3::unitY(), G3D::pi() * 0.5f);
    const CoordinateFrame primaryFrame(quarterTurn, Vector3(10.0f, 5.0f, -2.0f));
    boost::shared_ptr<PartInstance> orientedPrimary =
        Creatable<Instance>::create<PartInstance>();
    orientedPrimary->setPartSizeXml(Vector3(2.0f, 4.0f, 6.0f));
    orientedPrimary->setCoordinateFrame(primaryFrame);
    orientedPrimary->setParent(orientedBoundsModel.get());
    boost::shared_ptr<PartInstance> orientedChild =
        Creatable<Instance>::create<PartInstance>();
    orientedChild->setPartSizeXml(Vector3(4.0f, 2.0f, 2.0f));
    CoordinateFrame childFrame = primaryFrame;
    childFrame.translation = primaryFrame.pointToWorldSpace(Vector3(4.0f, 1.0f, 0.0f));
    orientedChild->setCoordinateFrame(childFrame);
    orientedChild->setParent(orientedBoundsModel.get());
    orientedBoundsModel->setPrimaryPartSetByUser(orientedPrimary.get());
    shared_ptr<const Reflection::Tuple> orientedBounds = orientedBoundsModel->getBoundingBox();
    const CoordinateFrame orientedBoundsFrame = orientedBounds->at(0).get<CoordinateFrame>();
    const Vector3 orientedBoundsSize = orientedBounds->at(1).get<Vector3>();
    require(orientedBoundsFrame.rotation.fuzzyEq(quarterTurn) &&
            orientedBoundsFrame.translation.fuzzyEq(
                primaryFrame.pointToWorldSpace(Vector3(2.5f, 0.0f, 0.0f))) &&
            orientedBoundsSize.fuzzyEq(Vector3(7.0f, 4.0f, 6.0f)),
        "Model:GetBoundingBox must align its frame to PrimaryPart and bound rotated descendants in that frame");

    boost::shared_ptr<Decal> currentDecal = Creatable<Instance>::create<Decal>();
    require(currentDecal->getColor3() == Color3::white(),
        "Decal.Color3 must retain the current white default");
    currentDecal->setColor3(Color3(0.125f, 0.5f, 0.875f));
    require(near(currentDecal->getColor3().r, 0.125f) &&
            near(currentDecal->getColor3().g, 0.5f) &&
            near(currentDecal->getColor3().b, 0.875f),
        "Decal.Color3 must retain the current texture tint");
    require(currentDecal->findPropertyDescriptor("Color3") != nullptr,
        "Decal.Color3 must be reflected for current RBXL/RBXLX serialization");

    boost::shared_ptr<DecalTexture> currentTexture =
        Creatable<Instance>::create<DecalTexture>();
    require(near(currentTexture->getOffsetStudsU(), 0.0f) &&
            near(currentTexture->getOffsetStudsV(), 0.0f),
        "Texture offsets must retain the current zero defaults");
    require(currentTexture->findPropertyDescriptor("OffsetStudsU") != nullptr &&
            currentTexture->findPropertyDescriptor("OffsetStudsV") != nullptr,
        "Texture offsets must be reflected for current RBXL/RBXLX serialization");

    boost::shared_ptr<PartInstance> texturedBlock =
        Creatable<Instance>::create<PartInstance>();
    texturedBlock->setPartSizeXml(Vector3(8.0f, 2.0f, 6.0f));
    currentTexture->setFace(NORM_Y);
    currentTexture->setStudsPerTileU(2.0f);
    currentTexture->setStudsPerTileV(2.0f);

    const auto generateTextureVertices = [&]() {
        Graphics::GeometryGenerator counter;
        Graphics::GeometryGenerator::Options options;
        Graphics::GeometryGenerator::Resources resources;
        counter.addInstance(texturedBlock.get(), currentTexture.get(), options, resources);
        std::vector<Graphics::GeometryGenerator::Vertex> vertices(counter.getVertexCount());
        std::vector<unsigned short> indices(counter.getIndexCount());
        Graphics::GeometryGenerator generator(vertices.data(), indices.data());
        generator.addInstance(texturedBlock.get(), currentTexture.get(), options, resources);
        return vertices;
    };

    const std::vector<Graphics::GeometryGenerator::Vertex> unshiftedTextureVertices =
        generateTextureVertices();
    currentTexture->setOffsetStudsU(0.5f);
    currentTexture->setOffsetStudsV(0.5f);
    const std::vector<Graphics::GeometryGenerator::Vertex> shiftedTextureVertices =
        generateTextureVertices();
    require(unshiftedTextureVertices.size() == 4 && shiftedTextureVertices.size() == 4,
        "a block Texture must generate one complete surface quad");
    for (size_t i = 0; i < shiftedTextureVertices.size(); ++i)
    {
        require(near(shiftedTextureVertices[i].uv.x - unshiftedTextureVertices[i].uv.x, -0.25f) &&
                near(shiftedTextureVertices[i].uv.y - unshiftedTextureVertices[i].uv.y, 0.25f),
            "positive Texture offsets must move the repeated image right/up using current normalized UV semantics");
    }

    GameBasicSettings playerSettings;
    require(!playerSettings.getPlayerListVisible(),
        "current PlayerList visibility must default closed until explicitly opened");
    require(playerSettings.findPropertyDescriptor("PlayerListVisible") != nullptr,
        "UserGameSettings must reflect the current PlayerListVisible contract");
    playerSettings.setPlayerListVisible(true);
    require(playerSettings.getPlayerListVisible(),
        "UserGameSettings must persist the PlayerList visibility requested by CoreScripts");

    boost::shared_ptr<Frame> styledRoot = Creatable<Instance>::create<Frame>();
    boost::shared_ptr<StyleSheet> styleSheet = Creatable<Instance>::create<StyleSheet>();
    boost::shared_ptr<StyleRule> styleRule = Creatable<Instance>::create<StyleRule>();
    styleRule->setSelector(".transparent-default");
    styleRule->setProperty("BackgroundTransparency", Reflection::Variant(1.0));
    styleRule->setParent(styleSheet.get());
    boost::shared_ptr<StyleLink> styleLink = Creatable<Instance>::create<StyleLink>();
    styleLink->setStyleSheet(styleSheet.get());
    styleLink->setParent(styledRoot.get());
    boost::shared_ptr<Frame> styledFrame = Creatable<Instance>::create<Frame>();
    styledFrame->setParent(styledRoot.get());
    styledFrame->addTagInternal("transparent-default");
    applyResolvedStyles(styledFrame.get());
    require(near(styledFrame->getBackgroundTransparency(), 1.0f),
        "stylesheet doubles must coerce to native float UI properties");
    styledFrame->setBackgroundTransparency(0.0f);
    require(near(styledFrame->getBackgroundTransparency(), 1.0f),
        "tagged GUI defaults assigned after mounting must re-resolve styles");

    boost::shared_ptr<StyleRule> styledBackgroundRule =
        Creatable<Instance>::create<StyleRule>();
    styledBackgroundRule->setSelector(".transparent-default");
    styledBackgroundRule->setPriority(1);
    styledBackgroundRule->setProperty(
        "BackgroundTransparency", Reflection::Variant(0.08));
    styledBackgroundRule->setParent(styleSheet.get());
    require(near(styledFrame->getBackgroundTransparency(), 0.08f),
        "a later higher-priority Foundation rule must replace an earlier styled default");
    styledFrame->setBackgroundTransparency(0.5f);
    require(near(styledFrame->getBackgroundTransparency(), 0.5f),
        "explicit non-default GUI properties must retain precedence over styles");

    boost::shared_ptr<StyleSheet> derivedBaseSheet = Creatable<Instance>::create<StyleSheet>();
    boost::shared_ptr<StyleRule> derivedBaseRule = Creatable<Instance>::create<StyleRule>();
    derivedBaseRule->setSelector(".derived-only");
    derivedBaseRule->setProperty("BackgroundTransparency", Reflection::Variant(0.75));
    derivedBaseRule->setParent(derivedBaseSheet.get());
    boost::shared_ptr<StyleSheet> derivedLocalSheet = Creatable<Instance>::create<StyleSheet>();
    shared_ptr<Instances> derivedSheets(new Instances());
    derivedSheets->push_back(derivedBaseSheet);
    derivedLocalSheet->setDerives(derivedSheets);
    boost::shared_ptr<Frame> derivedRoot = Creatable<Instance>::create<Frame>();
    boost::shared_ptr<StyleLink> derivedLink = Creatable<Instance>::create<StyleLink>();
    derivedLink->setStyleSheet(derivedLocalSheet.get());
    derivedLink->setParent(derivedRoot.get());
    boost::shared_ptr<Frame> derivedFrame = Creatable<Instance>::create<Frame>();
    derivedFrame->addTagInternal("derived-only");
    derivedFrame->setParent(derivedRoot.get());
    require(near(derivedFrame->getBackgroundTransparency(), 0.75f),
        "a linked StyleSheet must inherit rules from its SetDerives cascade");
    derivedBaseRule->setProperty("BackgroundTransparency", Reflection::Variant(0.6));
    require(near(derivedFrame->getBackgroundTransparency(), 0.6f),
        "changes in a derived StyleSheet must invalidate the linked GUI subtree");
    boost::shared_ptr<StyleRule> derivedLocalRule = Creatable<Instance>::create<StyleRule>();
    derivedLocalRule->setSelector(".derived-only");
    derivedLocalRule->setProperty("BackgroundTransparency", Reflection::Variant(0.2));
    derivedLocalRule->setParent(derivedLocalSheet.get());
    require(near(derivedFrame->getBackgroundTransparency(), 0.2f),
        "a local StyleSheet rule must override an equal-priority derived rule");
    shared_ptr<Instances> cyclicDerives(new Instances());
    cyclicDerives->push_back(derivedLocalSheet);
    derivedBaseSheet->setDerives(cyclicDerives);
    require(near(derivedFrame->getBackgroundTransparency(), 0.2f),
        "cyclic StyleSheet derivation must terminate while preserving the local cascade");

    // Foundation creates its StyleLink before its generated rules have all
    // been parented/configured. Existing tagged descendants must be resolved
    // again when the linked sheet changes; otherwise PlayerListContainer's
    // authentic `auto-xy` tag remains at a zero size and is positioned beyond
    // the right edge of the viewport.
    boost::shared_ptr<Frame> lateStyledRoot = Creatable<Instance>::create<Frame>();
    boost::shared_ptr<StyleSheet> lateStyleSheet = Creatable<Instance>::create<StyleSheet>();
    boost::shared_ptr<StyleLink> lateStyleLink = Creatable<Instance>::create<StyleLink>();
    lateStyleLink->setStyleSheet(lateStyleSheet.get());
    lateStyleLink->setParent(lateStyledRoot.get());
    boost::shared_ptr<Frame> lateStyledFrame = Creatable<Instance>::create<Frame>();
    lateStyledFrame->addTagInternal("late-auto-xy");
    lateStyledFrame->setParent(lateStyledRoot.get());
    require(lateStyledFrame->getAutomaticSize() == AUTOMATIC_SIZE_NONE,
        "a tagged frame must remain at its native default before its rule exists");
    boost::shared_ptr<StyleRule> lateStyleRule = Creatable<Instance>::create<StyleRule>();
    lateStyleRule->setSelector(".late-auto-xy");
    lateStyleRule->setProperty("AutomaticSize", Reflection::Variant(AUTOMATIC_SIZE_XY));
    lateStyleRule->setParent(lateStyleSheet.get());
    require(lateStyledFrame->getAutomaticSize() == AUTOMATIC_SIZE_XY,
        "StyleLink must re-resolve existing tagged descendants when rules are added later");
    lateStyledRoot->setSize(UDim2(0.0f, 500, 0.0f, 500));
    lateStyledRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 500.0f, 500.0f), true);
    boost::shared_ptr<Frame> lateMountedPanel = Creatable<Instance>::create<Frame>();
    lateMountedPanel->setSize(UDim2(0.0f, 143, 0.0f, 360));
    lateMountedPanel->setParent(lateStyledFrame.get());
    require(near(lateStyledFrame->getAbsoluteSize().x, 143.0f) &&
            near(lateStyledFrame->getAbsoluteSize().y, 360.0f),
        "an AutomaticSize container must resize when its fixed panel mounts later");
    lateMountedPanel->setVisible(false);
    require(near(lateStyledFrame->getAbsoluteSize().x, 0.0f) &&
            near(lateStyledFrame->getAbsoluteSize().y, 0.0f),
        "an AutomaticSize container must shrink when its content is hidden");
    lateMountedPanel->setVisible(true);
    require(near(lateStyledFrame->getAbsoluteSize().x, 143.0f) &&
            near(lateStyledFrame->getAbsoluteSize().y, 360.0f),
        "an AutomaticSize container must grow when its content becomes visible");

    boost::shared_ptr<PlayerScripts> playerScripts = Creatable<Instance>::create<PlayerScripts>();
    int registeredCameraModes = 0;
    playerScripts->computerCameraMovementModeRegisteredSignal.connect([&]() { ++registeredCameraModes; });
    playerScripts->registerComputerCameraMovementMode(GameBasicSettings::COMPUTER_CAMERA_MOVEMENT_MODE_DEFAULT);
    playerScripts->registerComputerCameraMovementMode(GameBasicSettings::COMPUTER_CAMERA_MOVEMENT_MODE_CAMERA_TOGGLE);
    playerScripts->registerComputerCameraMovementMode(GameBasicSettings::COMPUTER_CAMERA_MOVEMENT_MODE_CAMERA_TOGGLE);
    shared_ptr<const Reflection::ValueArray> cameraModes = playerScripts->getRegisteredComputerCameraMovementModes();
    require(cameraModes->size() == 2 && registeredCameraModes == 2,
        "PlayerScripts movement-mode registries must preserve insertion order and ignore duplicates");
    require((*cameraModes)[1].get<GameBasicSettings::ComputerCameraMovementMode>() ==
            GameBasicSettings::COMPUTER_CAMERA_MOVEMENT_MODE_CAMERA_TOGGLE,
        "PlayerScripts must return registered movement modes as their reflected enum values");
    playerScripts->clearComputerCameraMovementModes();
    require(playerScripts->getRegisteredComputerCameraMovementModes()->empty() && registeredCameraModes == 3,
        "clearing a PlayerScripts movement-mode registry must notify the settings observer");

    boost::shared_ptr<Frame> parent = Creatable<Instance>::create<Frame>();
    parent->setSize(UDim2(0.0f, 300, 0.0f, 120));

    boost::shared_ptr<UIPadding> padding = Creatable<Instance>::create<UIPadding>();
    padding->setPaddingLeft(UDim(0.0f, 10));
    padding->setPaddingRight(UDim(0.0f, 20));
    padding->setPaddingTop(UDim(0.0f, 5));
    padding->setParent(parent.get());

    boost::shared_ptr<UIListLayout> list = Creatable<Instance>::create<UIListLayout>();
    list->setFillDirection(FILL_DIRECTION_HORIZONTAL);
    list->setSortOrder(SORT_ORDER_LAYOUT_ORDER);
    list->setPadding(UDim(0.0f, 6));
    list->setParent(parent.get());

    boost::shared_ptr<Frame> second = Creatable<Instance>::create<Frame>();
    second->setName("second");
    second->setSize(UDim2(0.0f, 40, 0.0f, 30));
    second->setLayoutOrder(2);
    second->setParent(parent.get());

    boost::shared_ptr<Frame> first = Creatable<Instance>::create<Frame>();
    first->setName("first");
    first->setSize(UDim2(0.0f, 50, 0.0f, 20));
    first->setLayoutOrder(1);
    first->setParent(parent.get());

    parent->handleResize(Rect2D::xywh(0.0f, 0.0f, 300.0f, 120.0f), true);
    require(near(first->getAbsolutePosition().x, 10.0f) && near(first->getAbsolutePosition().y, 5.0f),
        "UIPadding and UIListLayout must place the first ordered child in the inset content box");
    require(near(second->getAbsolutePosition().x, 66.0f),
        "UIListLayout must advance by child size and resolved padding");
    require(near(list->getAbsoluteContentSize().x, 96.0f) && near(list->getAbsoluteContentSize().y, 30.0f),
        "UIListLayout AbsoluteContentSize must describe the laid-out children");

    boost::shared_ptr<Frame> mountedListHost = Creatable<Instance>::create<Frame>();
    mountedListHost->setSize(UDim2(0.0f, 200, 0.0f, 100));
    mountedListHost->handleResize(Rect2D::xywh(0.0f, 0.0f, 200.0f, 100.0f), true);
    boost::shared_ptr<UIListLayout> mountedList = Creatable<Instance>::create<UIListLayout>();
    mountedList->setSortOrder(SORT_ORDER_LAYOUT_ORDER);
    mountedList->setParent(mountedListHost.get());
    boost::shared_ptr<Frame> mountedLast = Creatable<Instance>::create<Frame>();
    mountedLast->setSize(UDim2(1.0f, 0, 0.0f, 30));
    mountedLast->setLayoutOrder(3);
    mountedLast->setParent(mountedListHost.get());
    boost::shared_ptr<Frame> mountedFirst = Creatable<Instance>::create<Frame>();
    mountedFirst->setSize(UDim2(1.0f, 0, 0.0f, 20));
    mountedFirst->setLayoutOrder(1);
    mountedFirst->setParent(mountedListHost.get());
    boost::shared_ptr<Frame> mountedMiddle = Creatable<Instance>::create<Frame>();
    mountedMiddle->setSize(UDim2(1.0f, 0, 0.0f, 25));
    mountedMiddle->setLayoutOrder(2);
    mountedMiddle->setParent(mountedListHost.get());
    require(near(mountedFirst->getAbsolutePosition().y, 0.0f) &&
            near(mountedMiddle->getAbsolutePosition().y, 20.0f) &&
            near(mountedLast->getAbsolutePosition().y, 45.0f),
        "UIListLayout must reflow already-mounted siblings when React adds later children");
    mountedMiddle->setVisible(false);
    require(near(mountedLast->getAbsolutePosition().y, 20.0f),
        "UIListLayout must reflow siblings when a mounted child changes visibility");
    mountedFirst->setSize(UDim2(1.0f, 0, 0.0f, 35));
    require(near(mountedLast->getAbsolutePosition().y, 35.0f),
        "UIListLayout must reflow siblings when a mounted child's authored size changes");

    boost::shared_ptr<Frame> automaticRoot = Creatable<Instance>::create<Frame>();
    automaticRoot->setSize(UDim2(0.0f, 200, 0.0f, 200));
    boost::shared_ptr<Frame> automaticOuter = Creatable<Instance>::create<Frame>();
    automaticOuter->setSize(UDim2(1.0f, 0, 0.0f, 0));
    automaticOuter->setAutomaticSize(AUTOMATIC_SIZE_Y);
    automaticOuter->setParent(automaticRoot.get());
    boost::shared_ptr<UIListLayout> outerLayout = Creatable<Instance>::create<UIListLayout>();
    outerLayout->setFillDirection(FILL_DIRECTION_VERTICAL);
    outerLayout->setParent(automaticOuter.get());
    boost::shared_ptr<Frame> automaticInner = Creatable<Instance>::create<Frame>();
    automaticInner->setSize(UDim2(1.0f, 0, 0.0f, 0));
    automaticInner->setAutomaticSize(AUTOMATIC_SIZE_Y);
    automaticInner->setParent(automaticOuter.get());
    boost::shared_ptr<UIListLayout> innerLayout = Creatable<Instance>::create<UIListLayout>();
    innerLayout->setFillDirection(FILL_DIRECTION_VERTICAL);
    innerLayout->setParent(automaticInner.get());
    boost::shared_ptr<Frame> automaticRow = Creatable<Instance>::create<Frame>();
    automaticRow->setSize(UDim2(1.0f, 0, 0.0f, 40));
    automaticRow->setParent(automaticInner.get());
    automaticRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 200.0f, 200.0f), true);
    require(near(automaticInner->getAbsoluteSize().y, 40.0f) &&
            near(automaticOuter->getAbsoluteSize().y, 40.0f),
        "nested AutomaticSize frames must propagate resolved descendant content height");

    boost::shared_ptr<Frame> fillHost = Creatable<Instance>::create<Frame>();
    fillHost->setSize(UDim2(1.0f, 0, 0.0f, 0));
    fillHost->setAutomaticSize(AUTOMATIC_SIZE_Y);
    fillHost->setParent(automaticRoot.get());
    boost::shared_ptr<Frame> fillWrapper = Creatable<Instance>::create<Frame>();
    fillWrapper->setSize(UDim2(1.0f, 0, 1.0f, 0));
    fillWrapper->setParent(fillHost.get());
    boost::shared_ptr<Frame> fillCanvas = Creatable<Instance>::create<Frame>();
    fillCanvas->setSize(UDim2(1.0f, 0, 1.0f, 0));
    fillCanvas->setParent(fillWrapper.get());
    boost::shared_ptr<UIListLayout> fillLayout = Creatable<Instance>::create<UIListLayout>();
    fillLayout->setFillDirection(FILL_DIRECTION_VERTICAL);
    fillLayout->setParent(fillCanvas.get());
    for (int index = 0; index < 3; ++index)
    {
        boost::shared_ptr<Frame> row = Creatable<Instance>::create<Frame>();
        row->setSize(UDim2(1.0f, 0, 0.0f, 20));
        row->setParent(fillCanvas.get());
    }
    automaticRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 200.0f, 200.0f), true);
    require(near(fillLayout->getAbsoluteContentSize().y, 60.0f) &&
            near(fillHost->getAbsoluteSize().y, 60.0f) &&
            near(fillWrapper->getAbsoluteSize().y, 60.0f),
        "AutomaticSize must consume UIListLayout content through zero-offset fill wrappers");

    boost::shared_ptr<Frame> tooltipHost = Creatable<Instance>::create<Frame>();
    tooltipHost->setSize(UDim2(0.0f, 254, 0.0f, 0));
    tooltipHost->setAutomaticSize(AUTOMATIC_SIZE_Y);
    tooltipHost->setParent(automaticRoot.get());
    boost::shared_ptr<Frame> tooltipContent = Creatable<Instance>::create<Frame>();
    tooltipContent->setSize(UDim2(1.0f, 0, 0.0f, 132));
    tooltipContent->setParent(tooltipHost.get());
    boost::shared_ptr<Frame> tooltipArrow = Creatable<Instance>::create<Frame>();
    tooltipArrow->setPosition(UDim2(0.0f, 1, 0.5f, 0));
    tooltipArrow->setSize(UDim2(0.0f, 8, 0.0f, 8));
    tooltipArrow->setAnchorPoint(Vector2(0.5f, 0.5f));
    tooltipArrow->setParent(tooltipHost.get());
    automaticRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 200.0f, 200.0f), true);
    require(near(tooltipHost->getAbsoluteSize().y, 132.0f),
        "percentage-positioned decorations must not inflate an automatic host from its parent height");

    boost::shared_ptr<Frame> centeredHost = Creatable<Instance>::create<Frame>();
    centeredHost->setSize(UDim2(0.0f, 0, 0.0f, 40));
    centeredHost->setAutomaticSize(AUTOMATIC_SIZE_X);
    centeredHost->setParent(automaticRoot.get());
    boost::shared_ptr<Frame> centeredContent = Creatable<Instance>::create<Frame>();
    centeredContent->setPosition(UDim2(0.5f, 0, 0.0f, 0));
    centeredContent->setSize(UDim2(0.0f, 80, 1.0f, 0));
    centeredContent->setAnchorPoint(Vector2(0.5f, 0.0f));
    centeredContent->setParent(centeredHost.get());
    automaticRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 200.0f, 200.0f), true);
    require(near(centeredHost->getAbsoluteSize().x, 80.0f),
        "centered fixed children must determine their automatic host through the anchor fixed point");

    boost::shared_ptr<Frame> aspectListHost = Creatable<Instance>::create<Frame>();
    aspectListHost->setSize(UDim2(0.0f, 145, 0.0f, 0));
    aspectListHost->setAutomaticSize(AUTOMATIC_SIZE_Y);
    aspectListHost->setParent(automaticRoot.get());
    boost::shared_ptr<UIListLayout> aspectList = Creatable<Instance>::create<UIListLayout>();
    aspectList->setFillDirection(FILL_DIRECTION_VERTICAL);
    aspectList->setParent(aspectListHost.get());
    boost::shared_ptr<Frame> squareListItem = Creatable<Instance>::create<Frame>();
    squareListItem->setSize(UDim2(1.0f, 0, 1.0f, 0));
    squareListItem->setParent(aspectListHost.get());
    boost::shared_ptr<UIAspectRatioConstraint> squareConstraint =
        Creatable<Instance>::create<UIAspectRatioConstraint>();
    squareConstraint->setAspectRatio(1.0f);
    squareConstraint->setDominantAxis(DOMINANT_AXIS_WIDTH);
    squareConstraint->setParent(squareListItem.get());
    boost::shared_ptr<Frame> fixedListItem = Creatable<Instance>::create<Frame>();
    fixedListItem->setSize(UDim2(1.0f, 0, 0.0f, 56));
    fixedListItem->setParent(aspectListHost.get());
    automaticRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 200.0f, 200.0f), true);
    require(near(aspectListHost->getAbsoluteSize().y, 201.0f) &&
            near(aspectList->getAbsoluteContentSize().y, 201.0f),
        "list content must use the constrained size of aspect-ratio children in automatic hosts");

    list->setPadding(UDim(0.0f, 12));
    require(near(second->getAbsolutePosition().x, 72.0f),
        "changing UIListLayout properties after parenting must immediately invalidate layout");
    padding->setPaddingLeft(UDim(0.0f, 18));
    require(near(first->getAbsolutePosition().x, 18.0f) && near(second->getAbsolutePosition().x, 80.0f),
        "changing UIPadding properties after parenting must immediately invalidate layout");

    boost::shared_ptr<Frame> anchorRoot = Creatable<Instance>::create<Frame>();
    anchorRoot->setSize(UDim2(0.0f, 300, 0.0f, 120));
    boost::shared_ptr<Frame> anchored = Creatable<Instance>::create<Frame>();
    anchored->setSize(UDim2(0.0f, 40, 0.0f, 20));
    anchored->setPosition(UDim2(0.5f, 0, 0.5f, 0));
    anchored->setAnchorPoint(Vector2(0.5f, 0.5f));
    anchored->setParent(anchorRoot.get());
    anchorRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 300.0f, 120.0f), true);
    require(near(anchored->getAbsolutePosition().x, 130.0f) &&
            near(anchored->getAbsolutePosition().y, 50.0f),
        "GuiObject AnchorPoint must place the authored Position at the selected point within the object");
    anchored->setAnchorPoint(Vector2(2.0f, -1.0f));
    require(anchored->getAnchorPoint() == Vector2(1.0f, 0.0f),
        "GuiObject AnchorPoint must remain inside the supported unit square");
    anchored->setParent(NULL);

    boost::shared_ptr<Frame> pageRoot = Creatable<Instance>::create<Frame>();
    pageRoot->setSize(UDim2(0.0f, 300, 0.0f, 100));
    boost::shared_ptr<UIPageLayout> pageLayout = Creatable<Instance>::create<UIPageLayout>();
    pageLayout->setPadding(UDim(0.0f, 5));
    pageLayout->setCircular(true);
    pageLayout->setTweenTime(0.25f);
    pageLayout->setParent(pageRoot.get());
    boost::shared_ptr<Frame> pageOne = Creatable<Instance>::create<Frame>();
    boost::shared_ptr<Frame> pageTwo = Creatable<Instance>::create<Frame>();
    boost::shared_ptr<Frame> pageThree = Creatable<Instance>::create<Frame>();
    pageOne->setName("one");
    pageTwo->setName("two");
    pageThree->setName("three");
    pageOne->setLayoutOrder(1);
    pageTwo->setLayoutOrder(2);
    pageThree->setLayoutOrder(3);
    pageOne->setSize(UDim2(0.0f, 100, 0.0f, 40));
    pageTwo->setSize(UDim2(0.0f, 100, 0.0f, 40));
    pageThree->setSize(UDim2(0.0f, 100, 0.0f, 40));
    pageOne->setParent(pageRoot.get());
    pageTwo->setParent(pageRoot.get());
    pageThree->setParent(pageRoot.get());
    pageRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 300.0f, 100.0f), true);
    require(pageLayout->getCurrentPage() == pageOne.get(),
        "UIPageLayout must select the first sorted page when content becomes available");
    require(near(pageOne->getAbsolutePosition().x, 100.0f) &&
            near(pageOne->getAbsolutePosition().y, 30.0f) &&
            near(pageTwo->getAbsolutePosition().x, 205.0f),
        "UIPageLayout must center the current page and place ordered siblings using resolved padding");

    int enteredPages = 0;
    int leftPages = 0;
    int stoppedPages = 0;
    pageLayout->pageEnterSignal.connect([&](shared_ptr<Instance>) { ++enteredPages; });
    pageLayout->pageLeaveSignal.connect([&](shared_ptr<Instance>) { ++leftPages; });
    pageLayout->stoppedSignal.connect([&](shared_ptr<Instance>) { ++stoppedPages; });
    pageLayout->next();
    require(pageLayout->getCurrentPage() == pageTwo.get() && pageLayout->getAnimated(),
        "UIPageLayout Next must update CurrentPage and begin the recovered easing transition");
    pageLayout->stepAnimation(0.125);
    require(near(pageTwo->getAbsolutePosition().x, 126.0f),
        "UIPageLayout must animate page positions with the configured Out/Quad easing curve");
    pageLayout->stepAnimation(0.125);
    require(!pageLayout->getAnimated() && near(pageTwo->getAbsolutePosition().x, 100.0f),
        "UIPageLayout must settle exactly on the target page after TweenTime");
    require(enteredPages == 1 && leftPages == 1 && stoppedPages == 1,
        "UIPageLayout must report page entry, departure, and completed motion exactly once");
    pageLayout->jumpTo(pageThree);
    pageLayout->stepAnimation(0.25);
    pageLayout->next();
    pageLayout->stepAnimation(0.25);
    require(pageLayout->getCurrentPage() == pageOne.get() &&
            near(pageOne->getAbsolutePosition().x, 100.0f),
        "circular UIPageLayout navigation must cross the final-to-first seam in the forward direction");
    pageLayout->setTweenTime(0.0f);
    pageLayout->previous();
    require(pageLayout->getCurrentPage() == pageThree.get() && !pageLayout->getAnimated(),
        "zero TweenTime UIPageLayout navigation must switch pages synchronously");

    boost::shared_ptr<UICorner> corner = Creatable<Instance>::create<UICorner>();
    corner->setCornerRadius(UDim(0.5f, 0));
    corner->setParent(first.get());
    require(near(corner->getRadius(Vector2(50.0f, 20.0f)), 10.0f),
        "UICorner must resolve and clamp radius against the shortest edge");

    boost::shared_ptr<CanvasGroup> group = Creatable<Instance>::create<CanvasGroup>();
    group->setGroupColor3(Color3(0.5f, 0.25f, 1.0f));
    group->setGroupTransparency(0.25f);
    boost::shared_ptr<Frame> tinted = Creatable<Instance>::create<Frame>();
    tinted->setParent(group.get());
    const Color4 composited = tinted->applyCanvasGroup(Color4(1.0f, 0.8f, 0.6f, 0.8f));
    require(near(composited.r, 0.5f) && near(composited.g, 0.2f) &&
            near(composited.b, 0.6f) && near(composited.a, 0.6f),
        "CanvasGroup must apply group tint and transparency to descendant rendering");

    boost::shared_ptr<UITextSizeConstraint> textConstraint =
        Creatable<Instance>::create<UITextSizeConstraint>();
    textConstraint->setMinTextSize(12);
    textConstraint->setMaxTextSize(18);
    require(textConstraint->getMinTextSize() == 12 && textConstraint->getMaxTextSize() == 18,
        "UITextSizeConstraint must preserve a valid text-size interval");

    boost::shared_ptr<Frame> constraintRoot = Creatable<Instance>::create<Frame>();
    constraintRoot->setSize(UDim2(0.0f, 1280, 0.0f, 720));
    boost::shared_ptr<Frame> constrained = Creatable<Instance>::create<Frame>();
    constrained->setSize(UDim2(1.0f, 0, 1.0f, 0));
    constrained->setParent(constraintRoot.get());
    boost::shared_ptr<UISizeConstraint> sizeConstraint =
        Creatable<Instance>::create<UISizeConstraint>();
    sizeConstraint->setMinSize(Vector2(120.0f, 80.0f));
    sizeConstraint->setMaxSize(Vector2(475.0f, 275.0f));
    sizeConstraint->setParent(constrained.get());
    constraintRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 1280.0f, 720.0f), true);
    require(near(constrained->getAbsoluteSize().x, 475.0f) &&
            near(constrained->getAbsoluteSize().y, 275.0f),
        "UISizeConstraint must clamp responsive GUI dimensions to the recovered chat maximum");
    sizeConstraint->setMaxSize(Vector2(400.0f, 240.0f));
    require(near(constrained->getAbsoluteSize().x, 400.0f) &&
            near(constrained->getAbsoluteSize().y, 240.0f),
        "changing UISizeConstraint after parenting must immediately invalidate layout");

    boost::shared_ptr<Frame> aspectChild = Creatable<Instance>::create<Frame>();
    aspectChild->setSize(UDim2(0.0f, 400, 0.0f, 400));
    aspectChild->setParent(constraintRoot.get());
    boost::shared_ptr<UIAspectRatioConstraint> aspectConstraint =
        Creatable<Instance>::create<UIAspectRatioConstraint>();
    aspectConstraint->setAspectRatio(2.0f);
    aspectConstraint->setParent(aspectChild.get());
    constraintRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 800.0f, 600.0f), true);
    require(near(aspectChild->getAbsoluteSize().x, 400.0f) &&
            near(aspectChild->getAbsoluteSize().y, 200.0f),
        "UIAspectRatioConstraint must fit the authored size while preserving its ratio");
    aspectConstraint->setAspectType(ASPECT_TYPE_SCALE_WITH_PARENT_SIZE);
    require(near(aspectChild->getAbsoluteSize().x, 1280.0f) &&
            near(aspectChild->getAbsoluteSize().y, 640.0f),
        "ScaleWithParentSize must use the parent bounds while preserving its ratio");
    aspectConstraint->setDominantAxis(DOMINANT_AXIS_HEIGHT);
    require(near(aspectChild->getAbsoluteSize().x, 1280.0f) &&
            near(aspectChild->getAbsoluteSize().y, 640.0f),
        "DominantAxis changes must immediately recompute the constrained size");

    boost::shared_ptr<Frame> flexRoot = Creatable<Instance>::create<Frame>();
    flexRoot->setSize(UDim2(0.0f, 300, 0.0f, 100));
    boost::shared_ptr<UIListLayout> flexLayout = Creatable<Instance>::create<UIListLayout>();
    flexLayout->setFillDirection(FILL_DIRECTION_HORIZONTAL);
    flexLayout->setSortOrder(SORT_ORDER_LAYOUT_ORDER);
    flexLayout->setHorizontalFlex(UI_FLEX_ALIGNMENT_FILL);
    flexLayout->setParent(flexRoot.get());
    boost::shared_ptr<Frame> flexFirst = Creatable<Instance>::create<Frame>();
    flexFirst->setLayoutOrder(1);
    flexFirst->setSize(UDim2(0.0f, 50, 0.0f, 20));
    flexFirst->setParent(flexRoot.get());
    boost::shared_ptr<Frame> flexSecond = Creatable<Instance>::create<Frame>();
    flexSecond->setLayoutOrder(2);
    flexSecond->setSize(UDim2(0.0f, 50, 0.0f, 30));
    flexSecond->setParent(flexRoot.get());
    flexRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 300.0f, 100.0f), true);
    require(near(flexFirst->getAbsoluteSize().x, 150.0f) &&
            near(flexSecond->getAbsoluteSize().x, 150.0f),
        "UIListLayout Fill flex must distribute free space across its line");

    boost::shared_ptr<Frame> intrinsicFlexRoot = Creatable<Instance>::create<Frame>();
    intrinsicFlexRoot->setSize(UDim2(0.0f, 300, 0.0f, 60));
    boost::shared_ptr<UIListLayout> intrinsicFlexLayout =
        Creatable<Instance>::create<UIListLayout>();
    intrinsicFlexLayout->setFillDirection(FILL_DIRECTION_HORIZONTAL);
    intrinsicFlexLayout->setVerticalAlignment(VERTICAL_ALIGNMENT_CENTER);
    intrinsicFlexLayout->setParent(intrinsicFlexRoot.get());
    for (int index = 0; index < 3; ++index)
    {
        boost::shared_ptr<Frame> container = Creatable<Instance>::create<Frame>();
        container->setName("intrinsic-flex-container");
        container->setSize(UDim2(0.0f, 0, 0.0f, 0));
        boost::shared_ptr<UIFlexItem> fill = Creatable<Instance>::create<UIFlexItem>();
        fill->setFlexMode(UI_FLEX_MODE_FILL);
        fill->setParent(container.get());
        boost::shared_ptr<Frame> content = Creatable<Instance>::create<Frame>();
        content->setSize(UDim2(0.0f, 80, 0.0f, 48));
        content->setParent(container.get());
        container->setParent(intrinsicFlexRoot.get());
    }
    intrinsicFlexRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 300.0f, 60.0f), true);
    for (std::size_t index = 0; index < intrinsicFlexRoot->numChildren(); ++index)
        if (Frame* container = Instance::fastDynamicCast<Frame>(
                intrinsicFlexRoot->getChild(index)))
            require(near(container->getAbsoluteSize().x, 100.0f) &&
                    near(container->getAbsoluteSize().y, 48.0f) &&
                    near(container->getAbsolutePosition().y, 6.0f),
                "zero-cross-axis Fill items must use intrinsic content size and centered alignment");

    boost::shared_ptr<Frame> automaticCrossRoot = Creatable<Instance>::create<Frame>();
    automaticCrossRoot->setSize(UDim2(0.0f, 200, 0.0f, 60));
    boost::shared_ptr<UIListLayout> automaticCrossLayout =
        Creatable<Instance>::create<UIListLayout>();
    automaticCrossLayout->setFillDirection(FILL_DIRECTION_HORIZONTAL);
    automaticCrossLayout->setVerticalAlignment(VERTICAL_ALIGNMENT_CENTER);
    automaticCrossLayout->setParent(automaticCrossRoot.get());
    boost::shared_ptr<Frame> automaticCrossItem = Creatable<Instance>::create<Frame>();
    automaticCrossItem->setSize(UDim2(0.0f, 0, 0.0f, 0));
    automaticCrossItem->setAutomaticSize(AUTOMATIC_SIZE_X);
    boost::shared_ptr<Frame> automaticCrossContent = Creatable<Instance>::create<Frame>();
    automaticCrossContent->setSize(UDim2(0.0f, 70, 0.0f, 30));
    automaticCrossContent->setParent(automaticCrossItem.get());
    automaticCrossItem->setParent(automaticCrossRoot.get());
    automaticCrossRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 200.0f, 60.0f), true);
    require(near(automaticCrossItem->getAbsoluteSize().x, 70.0f) &&
            near(automaticCrossItem->getAbsoluteSize().y, 30.0f) &&
            near(automaticCrossItem->getAbsolutePosition().y, 15.0f),
        "single-axis AutomaticSize list items must retain intrinsic cross-axis content size");

    boost::shared_ptr<UIFlexItem> firstFlex = Creatable<Instance>::create<UIFlexItem>();
    firstFlex->setFlexMode(UI_FLEX_MODE_CUSTOM);
    firstFlex->setGrowRatio(1.0f);
    firstFlex->setShrinkRatio(1.0f);
    firstFlex->setParent(flexFirst.get());
    boost::shared_ptr<UIFlexItem> secondFlex = Creatable<Instance>::create<UIFlexItem>();
    secondFlex->setFlexMode(UI_FLEX_MODE_CUSTOM);
    secondFlex->setGrowRatio(3.0f);
    secondFlex->setShrinkRatio(1.0f);
    secondFlex->setParent(flexSecond.get());
    flexRoot->invalidateLayout();
    require(near(flexFirst->getAbsoluteSize().x, 100.0f) &&
            near(flexSecond->getAbsoluteSize().x, 200.0f),
        "UIFlexItem custom grow ratios must override the container flex ratio");

    flexRoot->setSize(UDim2(0.0f, 80, 0.0f, 100));
    flexRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 80.0f, 100.0f), true);
    require(near(flexFirst->getAbsoluteSize().x, 40.0f) &&
            near(flexSecond->getAbsoluteSize().x, 40.0f),
        "UIFlexItem shrink ratios must resolve line overflow without negative sizes");

    boost::shared_ptr<Frame> wrapRoot = Creatable<Instance>::create<Frame>();
    wrapRoot->setSize(UDim2(0.0f, 120, 0.0f, 100));
    boost::shared_ptr<UIListLayout> wrapLayout = Creatable<Instance>::create<UIListLayout>();
    wrapLayout->setFillDirection(FILL_DIRECTION_HORIZONTAL);
    wrapLayout->setSortOrder(SORT_ORDER_LAYOUT_ORDER);
    wrapLayout->setWraps(true);
    wrapLayout->setItemLineAlignment(ITEM_LINE_ALIGNMENT_STRETCH);
    wrapLayout->setParent(wrapRoot.get());
    boost::shared_ptr<Frame> wrapItems[3];
    for (int index = 0; index < 3; ++index)
    {
        wrapItems[index] = Creatable<Instance>::create<Frame>();
        wrapItems[index]->setLayoutOrder(index);
        wrapItems[index]->setSize(UDim2(0.0f, 60, 0.0f, index == 1 ? 30 : 20));
        wrapItems[index]->setParent(wrapRoot.get());
    }
    wrapRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 120.0f, 100.0f), true);
    require(near(wrapItems[0]->getAbsoluteSize().y, 30.0f) &&
            near(wrapItems[2]->getAbsolutePosition().y, 30.0f),
        "wrapped flex lines must stretch within each line and advance on the cross axis");

    boost::shared_ptr<UIStroke> outerStroke = Creatable<Instance>::create<UIStroke>();
    outerStroke->setApplyStrokeMode(APPLY_STROKE_MODE_BORDER);
    outerStroke->setBorderStrokePosition(BORDER_STROKE_POSITION_CENTER);
    outerStroke->setBorderOffset(UDim(0.1f, 2));
    outerStroke->setStrokeSizingMode(STROKE_SIZING_MODE_SCALED_SIZE);
    outerStroke->setThickness(0.05f);
    outerStroke->setTransparency(0.25f);
    outerStroke->setZIndex(4);
    outerStroke->setParent(wrapRoot.get());
    boost::shared_ptr<UIStroke> innerStroke = Creatable<Instance>::create<UIStroke>();
    innerStroke->setZIndex(2);
    innerStroke->setParent(wrapRoot.get());
    const std::vector<const UIStroke*> strokes = findUIStrokes(wrapRoot.get());
    require(strokes.size() == 2 && strokes[0] == innerStroke.get() && strokes[1] == outerStroke.get(),
        "sibling UIStroke instances must render in stable ZIndex order");
    require(near(outerStroke->resolveThickness(Vector2(120.0f, 100.0f)), 5.0f) &&
            near(outerStroke->resolveBorderOffset(Vector2(120.0f, 100.0f)), 12.0f),
        "UIStroke must resolve scaled thickness and UDim border offsets against the shortest edge");

    std::vector<ColorSequence::Key> colorKeys;
    colorKeys.push_back(ColorSequence::Key(0.0f, Color3(1.0f, 0.0f, 0.0f), 0.0f));
    colorKeys.push_back(ColorSequence::Key(0.25f, Color3(0.0f, 1.0f, 0.0f), 0.0f));
    colorKeys.push_back(ColorSequence::Key(1.0f, Color3(0.0f, 0.0f, 1.0f), 0.0f));
    std::vector<NumberSequence::Key> transparencyKeys;
    transparencyKeys.push_back(NumberSequence::Key(0.0f, 0.0f, 0.0f));
    transparencyKeys.push_back(NumberSequence::Key(0.5f, 0.5f, 0.0f));
    transparencyKeys.push_back(NumberSequence::Key(1.0f, 1.0f, 0.0f));
    boost::shared_ptr<UIGradient> gradient = Creatable<Instance>::create<UIGradient>();
    gradient->setColor(ColorSequence(colorKeys, true));
    gradient->setTransparency(NumberSequence(transparencyKeys, true));
    gradient->setParent(outerStroke.get());
    require(findUIGradient(outerStroke.get()) == gradient.get(),
        "UIGradient must parent directly to UIStroke for gradient border rendering");
    const Color4 quarter = gradient->sample(0.25f, Color4(0.5f, 0.5f, 0.5f, 0.8f));
    require(near(quarter.r, 0.0f) && near(quarter.g, 0.5f) && near(quarter.b, 0.0f) && near(quarter.a, 0.6f),
        "UIGradient must interpolate all sequence keypoints and multiply the authored color and alpha");
    gradient->setRotation(90.0f);
    const Rect2D gradientBounds = Rect2D::xywh(10.0f, 20.0f, 100.0f, 200.0f);
    require(near(gradient->parameterAt(Vector2(60.0f, 20.0f), gradientBounds), 0.0f) &&
            near(gradient->parameterAt(Vector2(60.0f, 220.0f), gradientBounds), 1.0f),
        "UIGradient rotation must project the full object bounds onto its color axis");
    gradient->setOffset(Vector2(0.0f, 0.25f));
    require(near(gradient->parameterAt(Vector2(60.0f, 120.0f), gradientBounds), 0.25f),
        "UIGradient offset must translate the gradient in normalized object space");

    boost::shared_ptr<Frame> dragRoot = Creatable<Instance>::create<Frame>();
    dragRoot->setSize(UDim2(0.0f, 400, 0.0f, 200));
    boost::shared_ptr<Frame> dragTarget = Creatable<Instance>::create<Frame>();
    dragTarget->setPosition(UDim2(0.0f, 20, 0.0f, 30));
    dragTarget->setSize(UDim2(0.0f, 40, 0.0f, 20));
    dragTarget->setParent(dragRoot.get());
    boost::shared_ptr<UIDragDetector> dragDetector = Creatable<Instance>::create<UIDragDetector>();
    dragDetector->setParent(dragTarget.get());
    dragRoot->handleResize(Rect2D::xywh(0.0f, 0.0f, 400.0f, 200.0f), true);
    int dragStarts = 0;
    int dragContinues = 0;
    int dragEnds = 0;
    rbx::signals::scoped_connection dragStartConnection = dragDetector->dragStartSignal.connect(
        [&dragStarts](Vector2) { ++dragStarts; });
    rbx::signals::scoped_connection dragContinueConnection = dragDetector->dragContinueSignal.connect(
        [&dragContinues](Vector2) { ++dragContinues; });
    rbx::signals::scoped_connection dragEndConnection = dragDetector->dragEndSignal.connect(
        [&dragEnds](Vector2) { ++dragEnds; });
    dragDetector->beginDrag(Vector2(25.0f, 35.0f));
    dragDetector->continueDrag(Vector2(105.0f, 75.0f));
    require(dragDetector->getDragUDim2() == UDim2(0.0f, 80, 0.0f, 40) &&
            dragTarget->getPosition() == UDim2(0.0f, 100, 0.0f, 70),
        "UIDragDetector Offset response must translate its parent and expose the performed motion");
    dragDetector->endDrag(Vector2(105.0f, 75.0f));
    require(dragStarts == 1 && dragContinues == 1 && dragEnds == 1,
        "UIDragDetector must emit one start, continuation, and end event for a drag stream");

    dragDetector->setResponseStyle(UI_DRAG_RESPONSE_CUSTOM_SCALE);
    dragDetector->beginDrag(Vector2::zero());
    dragDetector->continueDrag(Vector2(100.0f, 50.0f));
    require(near(dragDetector->getDragUDim2().x.scale, 0.25f) &&
            near(dragDetector->getDragUDim2().y.scale, 0.25f) &&
            dragTarget->getPosition() == UDim2(0.0f, 100, 0.0f, 70),
        "UIDragDetector CustomScale response must report normalized motion without moving the parent");
    dragDetector->endDrag(Vector2(100.0f, 50.0f));

    dragDetector->setResponseStyle(UI_DRAG_RESPONSE_CUSTOM_OFFSET);
    dragDetector->setDragStyle(UI_DRAG_STYLE_TRANSLATE_LINE);
    dragDetector->setDragAxis(Vector2(1.0f, 0.0f));
    dragDetector->beginDrag(Vector2::zero());
    dragDetector->continueDrag(Vector2(60.0f, 45.0f));
    require(dragDetector->getDragUDim2() == UDim2(0.0f, 60, 0.0f, 0),
        "UIDragDetector TranslateLine must project input motion onto DragAxis");
    dragDetector->endDrag(Vector2(60.0f, 45.0f));

    require(TextService::FromTextFont(Text::FONT_BUILDER_ICONS_REGULAR) ==
            TextService::FONT_BUILDER_ICONS_REGULAR &&
            TextService::ToTextFont(TextService::FONT_BUILDER_ICONS_FILLED) ==
            Text::FONT_BUILDER_ICONS_FILLED,
        "Builder Icons regular and filled faces must remain distinct through the UI font bridge");
    require(TextService::FromTextFont(Text::FONT_BUILDERSANS_BOLD) ==
            TextService::FONT_BUILDERSANS_BOLD &&
            TextService::ToTextFont(TextService::FONT_BUILDERSANS_MEDIUM) ==
            Text::FONT_BUILDERSANS_MEDIUM,
        "Builder Sans weights must remain distinct through the UI font bridge");

    boost::shared_ptr<WorldModel> firstWorld = Creatable<Instance>::create<WorldModel>();
    boost::shared_ptr<WorldModel> secondWorld = Creatable<Instance>::create<WorldModel>();
    boost::shared_ptr<PartInstance> worldPart = Creatable<Instance>::create<PartInstance>();
    worldPart->setParent(firstWorld.get());
    require(worldPart->getPartPrimitive()->getWorld() == firstWorld->getWorld(),
        "parts parented to WorldModel must enter that model's isolated physics world");
    require(firstWorld->getWorld() != secondWorld->getWorld(),
        "each WorldModel must own a distinct physics world");
    worldPart->setParent(secondWorld.get());
    require(worldPart->getPartPrimitive()->getWorld() == secondWorld->getWorld(),
        "reparenting between WorldModels must transfer the primitive between isolated worlds");
    worldPart->setParent(NULL);
    require(worldPart->getPartPrimitive()->getWorld() == NULL,
        "removing a part from WorldModel must remove its primitive from the isolated world");

    boost::shared_ptr<ViewportFrame> viewport = Creatable<Instance>::create<ViewportFrame>();
    firstWorld->setParent(viewport.get());
    boost::shared_ptr<Camera> viewportCamera = Creatable<Instance>::create<Camera>();
    viewportCamera->setParent(viewport.get());
    viewport->setCurrentCamera(viewportCamera.get());
    viewport->setImageTransparency(2.0f);
    viewport->setCameraFieldOfView(200.0f);
    viewport->setIsMirrored(true);
    require(viewport->getCurrentCamera() == viewportCamera.get() &&
            viewport->getImageTransparency() == 1.0f &&
            viewport->getCameraFieldOfView() == 120.0f && viewport->getIsMirrored(),
        "ViewportFrame must preserve the recovered camera, image, field-of-view, and mirror contracts");

    boost::shared_ptr<VideoFrame> videoFrame = Creatable<Instance>::create<VideoFrame>();
    require(!videoFrame->getPlaying() && !videoFrame->getLooped() &&
            near(videoFrame->getVolume(), 1.0f) && !videoFrame->getIsLoaded() &&
            videoFrame->getResolution() == Vector2::zero(),
        "VideoFrame must expose the recovered unloaded playback defaults");
    int playedVideos = 0;
    int pausedVideos = 0;
    videoFrame->playedSignal.connect([&playedVideos]() { ++playedVideos; });
    videoFrame->pausedSignal.connect([&pausedVideos]() { ++pausedVideos; });
    videoFrame->play();
    videoFrame->pause();
    videoFrame->setVolume(-1.0f);
    videoFrame->setTimePosition(-2.0);
    videoFrame->setMaximumResolution(Vector2(-10.0f, 720.0f));
    videoFrame->setInternalVideoUsage(INTERNAL_VIDEO_USAGE_WATCH_PAGE);
    require(playedVideos == 1 && pausedVideos == 1 && !videoFrame->getPlaying() &&
            near(videoFrame->getVolume(), 0.0f) && videoFrame->getTimePosition() == 0.0 &&
            videoFrame->getMaximumResolution() == Vector2(0.0f, 720.0f) &&
            videoFrame->getInternalVideoUsage() == INTERNAL_VIDEO_USAGE_WATCH_PAGE,
        "VideoFrame must expose real playback controls, state events, and bounded media properties");

    const Content none;
    const Content uri = Content::fromUri("rbxassetid://123");
    const Content asset = Content::fromAssetId(456);
    require(none.getSourceType() == CONTENT_SOURCE_NONE &&
            uri.getSourceType() == CONTENT_SOURCE_URI && uri.getUri() == "rbxassetid://123" &&
            asset.getUri() == "rbxassetid://456",
        "Content must preserve the recovered none, URI, and numeric asset source contracts");

    boost::shared_ptr<DataModel> contentDataModel = DataModel::createDataModel(false, NULL, false);
    DataModel::LegacyLock contentDataModelLock(contentDataModel, DataModelJob::Write);
    std::stringstream currentDecalPlace;
    currentDecalPlace
        << "<roblox version=\"4\">"
        << "<Item class=\"Workspace\" referent=\"RBX1\"><Properties/>"
        << "<Item class=\"Part\" referent=\"RBX2\"><Properties><string name=\"Name\">TintedPart</string></Properties>"
        << "<Item class=\"Texture\" referent=\"RBX3\"><Properties>"
        << "<Color3 name=\"Color3\"><R>0.125</R><G>0.5</G><B>0.875</B></Color3>"
        << "<Content name=\"Texture\"><url>rbxasset://textures/SpawnLocation.png</url></Content>"
        << "<float name=\"OffsetStudsU\">1.25</float>"
        << "<float name=\"OffsetStudsV\">-2.5</float>"
        << "<float name=\"StudsPerTileU\">8</float>"
        << "<float name=\"StudsPerTileV\">4</float>"
        << "<token name=\"Face\">1</token>"
        << "</Properties></Item></Item></Item></roblox>";
    Serializer().load(currentDecalPlace, contentDataModel.get());
    Workspace* decalWorkspace = contentDataModel->getWorkspace();
    require(decalWorkspace->numChildren() == 1,
        "current decal XML must load its parent Part through the normal serializer");
    PartInstance* tintedPart = Instance::fastDynamicCast<PartInstance>(decalWorkspace->getChild(0));
    require(tintedPart && tintedPart->numChildren() == 1,
        "current decal XML must preserve the Decal child");
    Decal* serializedDecal = Instance::fastDynamicCast<Decal>(tintedPart->getChild(0));
    require(serializedDecal && near(serializedDecal->getColor3().r, 0.125f) &&
            near(serializedDecal->getColor3().g, 0.5f) &&
            near(serializedDecal->getColor3().b, 0.875f),
        "normal current RBXLX loading must apply Decal.Color3 instead of discarding it");
    DecalTexture* serializedTexture = Instance::fastDynamicCast<DecalTexture>(serializedDecal);
    require(serializedTexture && near(serializedTexture->getOffsetStudsU(), 1.25f) &&
            near(serializedTexture->getOffsetStudsV(), -2.5f) &&
            near(serializedTexture->getStudsPerTileU(), 8.0f) &&
            near(serializedTexture->getStudsPerTileV(), 4.0f),
        "normal current RBXLX loading must apply Texture scale and offsets instead of discarding them");
    tintedPart->setParent(NULL);
    const Content object = Content::fromObject(contentDataModel);
    require(object.getSourceType() == CONTENT_SOURCE_OBJECT &&
            object.getObject() == contentDataModel,
        "Content.fromObject must hold strong ownership of the referenced engine object");
    ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(contentDataModel.get());
    scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "local value = Instance.new('NumberValue')\n"
            "value.Name = '__renderStepDelta'\n"
            "value.Value = -1\n"
            "value.Parent = workspace\n"
            "game:GetService('RunService'):BindToRenderStep('CurrentCameraTest', 200, function(dt)\n"
            "    value.Value = dt\n"
            "end)"),
        "BindToRenderStepDeltaSetup", Reflection::Tuple());
    ServiceProvider::create<RunService>(contentDataModel.get())->renderStepped(0.125, false);
    std::auto_ptr<Reflection::Tuple> renderStepDeltaResult =
        scriptContext->executeInNewThread(
            Security::GameScript_, ProtectedString::fromTrustedSource(
                "game:GetService('RunService'):UnbindFromRenderStep('CurrentCameraTest')\n"
                "local value = workspace:FindFirstChild('__renderStepDelta')\n"
                "local result = value.Value\n"
                "value:Destroy()\n"
                "return result"),
            "BindToRenderStepDeltaResult", Reflection::Tuple());
    require(renderStepDeltaResult->values.size() == 1 &&
            std::abs(renderStepDeltaResult->at(0).convert<double>() - 0.125) < 1e-9,
        "RunService:BindToRenderStep callbacks must receive the current frame delta");
    std::auto_ptr<Reflection::Tuple> unicodeNormalizationResult =
        scriptContext->executeInNewThread(
            Security::GameScript_, ProtectedString::fromTrustedSource(
                "return utf8.nfcnormalize('e\\204\\129') == '\\195\\169', "
                "utf8.nfcnormalize('\\225\\132\\128\\225\\133\\161') == '\\234\\176\\128'"),
            "UnicodeNfcNormalization", Reflection::Tuple());
    require(unicodeNormalizationResult->values.size() == 2 &&
            unicodeNormalizationResult->at(0).convert<bool>() &&
            unicodeNormalizationResult->at(1).convert<bool>(),
        "utf8.nfcnormalize must perform canonical composition with complete Unicode data");
    std::auto_ptr<Reflection::Tuple> coroutineOwnershipResult =
        scriptContext->executeInNewThread(
            Security::GameScript_, ProtectedString::fromTrustedSource(
                "local thread = task.spawn(function() coroutine.yield() end)\n"
                "local suspendedBeforeOwnerResume = coroutine.status(thread) == 'suspended'\n"
                "local resumed = coroutine.resume(thread)\n"
                "return suspendedBeforeOwnerResume, resumed, coroutine.status(thread) == 'dead'"),
            "LuauCoroutineOwnership", Reflection::Tuple());
    require(coroutineOwnershipResult->values.size() == 3 &&
            coroutineOwnershipResult->at(0).convert<bool>() &&
            coroutineOwnershipResult->at(1).convert<bool>() &&
            coroutineOwnershipResult->at(2).convert<bool>(),
        "a plain Luau coroutine.yield must remain suspended until its owning coroutine resumes it");
    std::auto_ptr<Reflection::Tuple> yieldablePcallResult =
        scriptContext->executeInNewThread(
            Security::GameScript_, ProtectedString::fromTrustedSource(
                "local thread = coroutine.create(function()\n"
                "  local ok, value = pcall(function() coroutine.yield('paused'); return 42 end)\n"
                "  return ok, value\n"
                "end)\n"
                "local started, paused = coroutine.resume(thread)\n"
                "local resumed, protected, value = coroutine.resume(thread)\n"
                "return started, paused == 'paused', resumed, protected, value == 42, ypcall == pcall"),
            "LuauYieldableProtectedCall", Reflection::Tuple());
    require(yieldablePcallResult->values.size() == 6 &&
            yieldablePcallResult->at(0).convert<bool>() &&
            yieldablePcallResult->at(1).convert<bool>() &&
            yieldablePcallResult->at(2).convert<bool>() &&
            yieldablePcallResult->at(3).convert<bool>() &&
            yieldablePcallResult->at(4).convert<bool>() &&
            yieldablePcallResult->at(5).convert<bool>(),
        "current Luau pcall/ypcall must preserve protected results across a coroutine yield");
    std::auto_ptr<Reflection::Tuple> taskCancellationResult =
        scriptContext->executeInNewThread(
            Security::GameScript_, ProtectedString::fromTrustedSource(
                "local fired = false\n"
                "local delayed = task.delay(10, function(value) fired = value end, true)\n"
                "local isThread = type(delayed) == 'thread'\n"
                "local wasSuspended = coroutine.status(delayed) == 'suspended'\n"
                "task.cancel(delayed)\n"
                "return isThread, wasSuspended, coroutine.status(delayed) == 'dead', not fired"),
            "LuauTaskCancellation", Reflection::Tuple());
    require(taskCancellationResult->values.size() == 4 &&
            taskCancellationResult->at(0).convert<bool>() &&
            taskCancellationResult->at(1).convert<bool>() &&
            taskCancellationResult->at(2).convert<bool>() &&
            taskCancellationResult->at(3).convert<bool>(),
        "task.delay must return a cancellable thread and task.cancel must prevent its callback");
    std::auto_ptr<Reflection::Tuple> contextActionPassResult =
        scriptContext->executeInNewThread(
            Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
                "game:GetService('ContextActionService'):BindCoreAction("
                "'PassKeyboardInput', function() return Enum.ContextActionResult.Pass end, "
                "false, Enum.UserInputType.Keyboard)\n"
                "return true"),
            "ContextActionResultPass", Reflection::Tuple());
    require(contextActionPassResult->at(0).convert<bool>(),
        "ContextActionService pass-through callback must bind from current CoreScript Luau");
    ContextActionService* contextActionService =
        ServiceProvider::create<ContextActionService>(contentDataModel.get());
    boost::shared_ptr<InputObject> escapeInput =
        Creatable<Instance>::create<InputObject>(
            InputObject::TYPE_KEYBOARD, InputObject::INPUT_STATE_BEGIN,
            SDLK_ESCAPE, KMOD_NONE, 0, contentDataModel.get());
    require(!contextActionService->processCoreBindings(escapeInput).wasSunk(),
        "Enum.ContextActionResult.Pass must allow lower-priority current CoreScript actions to receive keyboard input");
    scriptContext->executeInNewThread(
        Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
            "local service = game:GetService('ContextActionService')\n"
            "service:UnbindCoreAction('PassKeyboardInput')\n"
            "service:BindCoreAction('SinkEscapeInput', function() "
            "return Enum.ContextActionResult.Sink end, false, Enum.KeyCode.Escape)"),
        "ContextActionResultSink", Reflection::Tuple());
    require(contextActionService->processCoreBindings(escapeInput).wasSunk(),
        "Enum.ContextActionResult.Sink must consume its matched keyboard input");
    contextActionService->unbindCoreAction("SinkEscapeInput");
    std::auto_ptr<Reflection::Tuple> contentResult = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "local uri = Content.fromUri('rbxassetid://123')\n"
            "local asset = Content.fromAssetId(456)\n"
            "local object = Content.fromObject(game)\n"
            "return Content.none.SourceType == Enum.ContentSourceType.None, "
            "uri.SourceType == Enum.ContentSourceType.Uri, uri.Uri == 'rbxassetid://123', "
            "uri.Object == nil, asset.Uri == 'rbxassetid://456', "
            "object.SourceType == Enum.ContentSourceType.Object, object.Object == game"),
        "ContentContract", Reflection::Tuple());
    static const char* contentChecks[] = {
        "Content.none must expose ContentSourceType.None",
        "Content.fromUri must expose ContentSourceType.Uri",
        "Content.fromUri must preserve its URI",
        "URI Content must not expose an Object",
        "Content.fromAssetId must form the canonical URI",
        "Content.fromObject must expose ContentSourceType.Object",
        "Content.fromObject must preserve engine object identity"
    };
    require(contentResult->values.size() == 7,
        "Luau Content contract must return all seven validation fields");
    for (std::size_t index = 0; index < 7; ++index)
        require(contentResult->at(index).convert<bool>(), contentChecks[index]);

    const std::filesystem::path promptSource =
        std::filesystem::temp_directory_path() / "rbx-capture-prompt-contract.png";
    {
        // A complete 1x1 RGBA PNG. Prompt validation and gallery persistence
        // exercise the same file-backed Content path used by screenshot capture.
        static const unsigned char png[] = {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
            0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
            0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4,
            0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41,
            0x54, 0x08, 0xd7, 0x63, 0xf8, 0xcf, 0xc0, 0xf0,
            0x1f, 0x00, 0x05, 0x00, 0x01, 0xff, 0x89, 0x99,
            0x3d, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
            0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
        };
        std::ofstream output(promptSource, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(png), sizeof(png));
    }
    require(std::filesystem::is_regular_file(promptSource),
        "capture prompt fixture must be written before exercising the native service");
    CaptureService* captureService =
        ServiceProvider::create<CaptureService>(contentDataModel.get());
    require(captureService != NULL,
        "CaptureService must attach to the test DataModel before the script prompt lifecycle");
    const std::string promptUri = "file://" + promptSource.string();
    long long savePromptId = 0;
    shared_ptr<const Reflection::ValueArray> openedCaptures;
    rbx::signals::scoped_connection savePromptConnection =
        captureService->openSaveCapturesPromptSignal.connect(
            [&savePromptId, &openedCaptures](long long promptId,
                shared_ptr<const Reflection::ValueArray> captures) {
                savePromptId = promptId;
                openedCaptures = captures;
            });
    const std::string beginSavePromptScript =
        "local service = game:GetService('CaptureService')\n"
        "local content = Content.fromUri('" + promptUri + "')\n"
        "service:PromptSaveCapturesToGallery({content}, function(results)\n"
        "  game:SetAttribute('CaptureSaveCallback', results[content] == true)\n"
        "end)\n"
        "return true\n";
    scriptContext->executeInNewThread(
        Security::RobloxGameScript_,
        ProtectedString::fromTrustedSource(beginSavePromptScript),
        "BeginCaptureSavePrompt", Reflection::Tuple());
    require(savePromptId > 0 && openedCaptures && openedCaptures->size() == 1 &&
            openedCaptures->at(0).isType<Content>() &&
            openedCaptures->at(0).cast<Content>() == Content::fromUri(promptUri),
        "PromptSaveCapturesToGallery must allocate an ID and publish the original Content array");
    const std::string finishSavePromptScript =
        "local service = game:GetService('CaptureService')\n"
        "local content = Content.fromUri('" + promptUri + "')\n"
        "service:OnSavePromptFinished(" + std::to_string(savePromptId) +
            ", {[content] = true})\n"
        "local savedPath\n"
        "for _, capture in service:RetrieveCaptures() do\n"
        "  if string.find(capture.filePath, 'rbx%-capture%-prompt%-contract') then savedPath = capture.filePath end\n"
        "end\n"
        "assert(savedPath, 'accepted prompt did not persist the capture')\n"
        "return game:GetAttribute('CaptureSaveCallback'), savedPath\n";
    std::auto_ptr<Reflection::Tuple> savePromptResult =
        scriptContext->executeInNewThread(
            Security::RobloxGameScript_,
            ProtectedString::fromTrustedSource(finishSavePromptScript),
            "FinishCaptureSavePrompt", Reflection::Tuple());
    require(savePromptResult->values.size() == 2 &&
            savePromptResult->at(0).convert<bool>(),
        "OnSavePromptFinished must preserve keyed Content results and invoke the registered callback");

    const std::string savedPath = savePromptResult->at(1).convert<std::string>();
    require(ContentProvider::resolveLocalContent(
                ContentId("file://" + savedPath)) ==
            std::filesystem::canonical(savedPath).string(),
        "ContentProvider must resolve an engine-owned Gallery capture for the shared renderer");
    require(ContentProvider::resolveLocalContent(ContentId(promptUri)).empty(),
        "ContentProvider must reject arbitrary file URLs outside the Gallery sandbox");

    ExperienceNotificationService* experienceNotifications =
        ServiceProvider::create<ExperienceNotificationService>(contentDataModel.get());
    bool uninitializedEligibilityErrored = false;
    experienceNotifications->canPromptOptInAsync(
        [](bool) {},
        [&uninitializedEligibilityErrored](std::string) {
            uninitializedEligibilityErrored = true;
        });
    require(uninitializedEligibilityErrored,
        "ExperienceNotificationService must reject eligibility queries before host initialization");
    experienceNotifications->initializePromptEligibility(false);
    bool offlineCanPrompt = true;
    experienceNotifications->canPromptOptInAsync(
        [&offlineCanPrompt](bool value) { offlineCanPrompt = value; },
        [](std::string) {});
    require(!offlineCanPrompt,
        "ExperienceNotificationService must expose the offline host's ineligible state");
    bool ineligiblePromptRejected = false;
    try
    {
        experienceNotifications->promptOptIn();
    }
    catch (const std::runtime_error&)
    {
        ineligiblePromptRejected = true;
    }
    require(ineligiblePromptRejected,
        "ExperienceNotificationService must reject PromptOptIn when CanPromptOptInAsync is false");
    int promptRequests = 0;
    int promptClosures = 0;
    rbx::signals::scoped_connection notificationPromptConnection =
        experienceNotifications->promptOptInRequestedSignal.connect(
            [&promptRequests]() { ++promptRequests; });
    rbx::signals::scoped_connection notificationClosedConnection =
        experienceNotifications->optInPromptClosedSignal.connect(
            [&promptClosures]() { ++promptClosures; });
    experienceNotifications->initializePromptEligibility(true);
    experienceNotifications->promptOptIn();
    experienceNotifications->promptOptIn();
    require(promptRequests == 1 && experienceNotifications->getPromptPending(),
        "ExperienceNotificationService must open exactly one pending opt-in prompt");
    experienceNotifications->invokeOptInPromptClosed();
    require(promptClosures == 1 && !experienceNotifications->getPromptPending(),
        "ExperienceNotificationService must close and retire the pending prompt exactly once");
    long long sharePromptId = 0;
    Content openedShareContent;
    std::string openedLaunchData;
    rbx::signals::scoped_connection sharePromptConnection =
        captureService->openShareCapturePromptSignal.connect(
            [&sharePromptId, &openedShareContent, &openedLaunchData](
                long long promptId, Content content, std::string launchData) {
                sharePromptId = promptId;
                openedShareContent = content;
                openedLaunchData = launchData;
            });
    const std::string savedUri = "file://" + savedPath;
    const std::string beginSharePromptScript =
        "local service = game:GetService('CaptureService')\n"
        "service:PromptShareCapture(Content.fromUri('" + savedUri +
            "'), 'unit-launch', function() game:SetAttribute('CaptureShareAccepted', true) end, function() game:SetAttribute('CaptureShareDenied', true) end)\n"
        "return true\n";
    scriptContext->executeInNewThread(
        Security::RobloxGameScript_,
        ProtectedString::fromTrustedSource(beginSharePromptScript),
        "BeginCaptureSharePrompt", Reflection::Tuple());
    require(sharePromptId > savePromptId &&
            openedShareContent == Content::fromUri(savedUri) &&
            openedLaunchData == "unit-launch",
        "PromptShareCapture must publish its tracked ID, exact Content, and launch data");
    const std::string finishSharePromptScript =
        "local service = game:GetService('CaptureService')\n"
        "service:OnSharePromptFinished(" + std::to_string(sharePromptId) + ", true)\n"
        "service:DeleteCapture('" + savedPath + "')\n"
        "return game:GetAttribute('CaptureShareAccepted'), game:GetAttribute('CaptureShareDenied') == nil\n";
    std::auto_ptr<Reflection::Tuple> sharePromptResult =
        scriptContext->executeInNewThread(
            Security::RobloxGameScript_,
            ProtectedString::fromTrustedSource(finishSharePromptScript),
            "FinishCaptureSharePrompt", Reflection::Tuple());
    require(sharePromptResult->values.size() == 2 &&
            sharePromptResult->at(0).convert<bool>() &&
            sharePromptResult->at(1).convert<bool>(),
        "OnSharePromptFinished must invoke only the callback matching the user's decision");
    require(!std::filesystem::exists(savedPath),
        "capture prompt test must delete its persisted gallery capture");
    std::filesystem::remove(promptSource);

    std::auto_ptr<Reflection::Tuple> boundingBoxResult =
        scriptContext->executeInNewThread(
            Security::GameScript_, ProtectedString::fromTrustedSource(
                "local model = Instance.new('Model')\n"
                "local first = Instance.new('Part')\n"
                "first.Size = Vector3.new(2, 4, 6)\n"
                "first.CFrame = CFrame.new(10, 0, 0)\n"
                "first.Parent = model\n"
                "local nested = Instance.new('Model')\n"
                "nested.Parent = model\n"
                "local second = Instance.new('Part')\n"
                "second.Size = Vector3.new(4, 2, 2)\n"
                "second.CFrame = CFrame.new(14, 2, 0)\n"
                "second.Parent = nested\n"
                "local frame, size = model:GetBoundingBox()\n"
                "return typeof(frame) == 'CFrame', typeof(size) == 'Vector3', "
                "(frame.Position - Vector3.new(12.5, 0.5, 0)).Magnitude < 1e-5, "
                "(size - Vector3.new(7, 5, 6)).Magnitude < 1e-5"),
            "ModelBoundingBoxContract", Reflection::Tuple());
    require(boundingBoxResult->values.size() == 4 &&
            boundingBoxResult->at(0).convert<bool>() &&
            boundingBoxResult->at(1).convert<bool>() &&
            boundingBoxResult->at(2).convert<bool>() &&
            boundingBoxResult->at(3).convert<bool>(),
        "Model:GetBoundingBox must expose its complete CFrame/Vector3 tuple to experience Luau");

    std::auto_ptr<Reflection::Tuple> currentCFrameResult =
        scriptContext->executeInNewThread(
            Security::GameScript_, ProtectedString::fromTrustedSource(
                "local frame = CFrame.new(10, 5, -2) * CFrame.Angles(0, math.pi / 2, 0)\n"
                "local localPoint = frame:PointToObjectSpace(frame:PointToWorldSpace(Vector3.new(2, 3, 4)))\n"
                "local rx, ry, rz = frame:ToOrientation()\n"
                "local axis, angle = frame:ToAxisAngle()\n"
                "local orientation = CFrame.fromOrientation(0.25, -0.5, 0.75)\n"
                "local camera = Instance.new('Camera')\n"
                "return frame.Position == Vector3.new(10, 5, -2), "
                "frame.Rotation.Position == Vector3.zero, "
                "frame.RightVector == frame.XVector and frame.UpVector == frame.YVector and frame.rightVector == frame.RightVector and frame.upVector == frame.UpVector, "
                "frame.LookVector == -frame.ZVector, "
                "(localPoint - Vector3.new(2, 3, 4)).Magnitude < 1e-5, "
                "frame:FuzzyEq(frame:Orthonormalize()), "
                "math.abs(frame:AngleBetween(CFrame.new())) > 1.5, "
                "typeof(rx) == 'number' and typeof(ry) == 'number' and typeof(rz) == 'number', "
                "typeof(axis) == 'Vector3' and math.abs(angle - math.pi / 2) < 1e-5, "
                "orientation:FuzzyEq(CFrame.fromEulerAnglesYXZ(0.25, -0.5, 0.75)), "
                "camera.NearPlaneZ == -0.5"),
            "CurrentCFrameMembers", Reflection::Tuple());
    require(currentCFrameResult->values.size() == 11,
        "current CFrame/camera contract must return all eleven validation fields");
    for (std::size_t index = 0; index < 11; ++index)
        require(currentCFrameResult->at(index).convert<bool>(),
            "current CFrame properties and transformation methods must preserve their numeric semantics");

    std::auto_ptr<Reflection::Tuple> vector2Result = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "local value = Vector2.new(3, 4)\n"
            "local midpoint = value:Lerp(Vector2.new(5, 6), 0.5)\n"
            "return value.Magnitude == 5, math.abs(value.Unit.X - 0.6) < 1e-6, "
            "math.abs(value.Unit.Y - 0.8) < 1e-6, midpoint == Vector2.new(4, 5), "
            "value.magnitude == value.Magnitude, value.unit == value.Unit"),
        "Vector2CurrentMembers", Reflection::Tuple());
    static const char* vector2Checks[] = {
        "Vector2.Magnitude must return the Euclidean length",
        "Vector2.Unit.X must preserve normalized X precision",
        "Vector2.Unit.Y must preserve normalized Y precision",
        "Vector2:Lerp must interpolate both components",
        "Vector2.magnitude must preserve the legacy Magnitude alias",
        "Vector2.unit must preserve the legacy Unit alias"
    };
    require(vector2Result->values.size() == 6,
        "Luau Vector2 contract must return all six validation fields");
    for (std::size_t index = 0; index < 6; ++index)
        require(vector2Result->at(index).convert<bool>(), vector2Checks[index]);

    std::auto_ptr<Reflection::Tuple> udim2Result = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "local x = UDim.new(0.5, -12)\n"
            "local y = UDim.new(0, 44)\n"
            "local value = UDim2.new(x, y)\n"
            "return value.X == x and value.Y == y and value == UDim2.new(0.5, -12, 0, 44)"),
        "UDim2CurrentConstructor", Reflection::Tuple());
    require(udim2Result->at(0).convert<bool>(),
        "UDim2.new must preserve the current two-UDim constructor used by Foundation components");

    std::auto_ptr<Reflection::Tuple> memberAliasResult = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "local layout = Instance.new('UIGridLayout')\n"
            "layout.Name = 'ButtonLayout'\n"
            "return layout.name == layout.Name and layout.name == 'ButtonLayout'"),
        "InstanceLowerCamelMemberAlias", Reflection::Tuple());
    require(memberAliasResult->at(0).convert<bool>(),
        "Instance lower-camel aliases must resolve against the concrete member rather than the global name table");

    std::auto_ptr<Reflection::Tuple> cframeMatrixResult = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "local position = Vector3.new(7, 8, 9)\n"
            "local right = Vector3.new(0, 0, -1)\n"
            "local up = Vector3.new(0, 1, 0)\n"
            "local inferred = CFrame.fromMatrix(position, right, up)\n"
            "local explicit = CFrame.fromMatrix(position, right, up, Vector3.new(1, 0, 0))\n"
            "local ix, iy, iz, i00, i01, i02, i10, i11, i12, i20, i21, i22 = inferred:GetComponents()\n"
            "local ex, ey, ez, e00, e01, e02, e10, e11, e12, e20, e21, e22 = explicit:GetComponents()\n"
            "return ix == 7 and iy == 8 and iz == 9 "
            "and i00 == 0 and i01 == 0 and i02 == 1 "
            "and i10 == 0 and i11 == 1 and i12 == 0 "
            "and i20 == -1 and i21 == 0 and i22 == 0 "
            "and ex == 7 and ey == 8 and ez == 9 "
            "and e00 == 0 and e01 == 0 and e02 == 1 "
            "and e10 == 0 and e11 == 1 and e12 == 0 "
            "and e20 == -1 and e21 == 0 and e22 == 0"),
        "CFrameFromMatrix", Reflection::Tuple());
    require(cframeMatrixResult->at(0).convert<bool>(),
        "CFrame.fromMatrix must preserve current right/up/back column semantics and infer back from right cross up");

    std::auto_ptr<Reflection::Tuple> recoveredEnumsResult = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "return Enum.MakeupType.Face.Value == 0 "
            "and Enum.MakeupType.Lip.Value == 1 and Enum.MakeupType.Eye.Value == 2 "
            "and Enum.MarketplaceItemPurchaseStatus.Success.Value == 1 "
            "and Enum.MarketplaceItemPurchaseStatus.PlaceInvalid.Value == 13 "
            "and Enum.RaycastFilterType.Exclude.Value == 0 "
            "and Enum.RaycastFilterType.Include.Value == 1 "
            "and Enum.RaycastFilterType.Blacklist == Enum.RaycastFilterType.Exclude "
            "and Enum.RaycastFilterType.Whitelist == Enum.RaycastFilterType.Include"),
        "Recovered2026Enums", Reflection::Tuple());
    require(recoveredEnumsResult->at(0).convert<bool>(),
        "recovered 2026 enums must retain the exact client names, values, and legacy ray-filter aliases");

    std::auto_ptr<Reflection::Tuple> runContextResult = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "local serverScript = Instance.new('Script')\n"
            "local localScript = Instance.new('LocalScript')\n"
            "local defaultsLegacy = serverScript.RunContext == Enum.RunContext.Legacy "
            "and localScript.RunContext == Enum.RunContext.Legacy\n"
            "serverScript.RunContext = Enum.RunContext.Client\n"
            "localScript.RunContext = Enum.RunContext.Server\n"
            "return defaultsLegacy, serverScript.RunContext == Enum.RunContext.Client, "
            "localScript.RunContext == Enum.RunContext.Legacy, "
            "Enum.RunContext.Legacy.Value == 0 and Enum.RunContext.Server.Value == 1 "
            "and Enum.RunContext.Client.Value == 2 and Enum.RunContext.Plugin.Value == 3"),
        "ScriptRunContextContract", Reflection::Tuple());
    require(runContextResult->values.size() == 4 &&
            runContextResult->at(0).convert<bool>() &&
            runContextResult->at(1).convert<bool>() &&
            runContextResult->at(2).convert<bool>() &&
            runContextResult->at(3).convert<bool>(),
        "Script.RunContext must preserve exact current enum values and LocalScript's fixed Legacy contract");

    std::auto_ptr<Reflection::Tuple> publishResultEnum = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "return Enum.PromptPublishAssetResult.Success.Value == 1 "
            "and Enum.PromptPublishAssetResult.PermissionDenied.Value == 2 "
            "and Enum.PromptPublishAssetResult.Timeout.Value == 3 "
            "and Enum.PromptPublishAssetResult.UploadFailed.Value == 4 "
            "and Enum.PromptPublishAssetResult.NoUserInput.Value == 5 "
            "and Enum.PromptPublishAssetResult.UnknownFailure.Value == 6"),
        "PromptPublishAssetResultEnum", Reflection::Tuple());
    require(publishResultEnum->at(0).convert<bool>(),
        "PromptPublishAssetResult must match the exact Studio deployment enum contract");

    std::auto_ptr<Reflection::Tuple> currentServiceEvents = scriptContext->executeInNewThread(
        Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
            "local marketplace = game:GetService('MarketplaceService')\n"
            "local social = game:GetService('SocialService')\n"
            "local bulk = marketplace.PromptBulkPurchaseRequestedV2:Connect(function() end)\n"
            "local share = social.OpenShareSheetWithLink:Connect(function() end)\n"
            "local valid = Enum.MarketplaceBulkPurchasePromptStatus.Completed.Value == 1 "
            "and Enum.MarketplaceBulkPurchasePromptStatus.Aborted.Value == 2 "
            "and Enum.MarketplaceBulkPurchasePromptStatus.Error.Value == 3\n"
            "bulk:Disconnect()\n"
            "share:Disconnect()\n"
            "return valid"),
        "CurrentMarketplaceAndSocialEvents", Reflection::Tuple());
    require(currentServiceEvents->at(0).convert<bool>(),
        "current bulk-purchase and share-sheet CoreScripts must bind to their typed engine events");

    std::auto_ptr<Reflection::Tuple> appUiSizeResult =
        scriptContext->executeInNewThread(
            Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
                "local input = game:GetService('UserInputService')\n"
                "assert(input.StatusBarSize == Vector2.zero and input.NavBarSize == Vector2.zero)\n"
                "assert(input.BottomBarSize == Vector2.zero and input.RightBarSize == Vector2.zero)\n"
                "local changes = {StatusBarSize = 0, NavBarSize = 0, BottomBarSize = 0, RightBarSize = 0}\n"
                "local connections = {}\n"
                "for property in pairs(changes) do\n"
                "  connections[property] = input:GetPropertyChangedSignal(property):Connect(function()\n"
                "    changes[property] = changes[property] + 1\n"
                "  end)\n"
                "end\n"
                "input:SendAppUISizes(Vector2.new(8, 9), Vector2.new(6, 7), Vector2.new(4, 5), Vector2.new(2, 3))\n"
                "assert(input.RightBarSize == Vector2.new(8, 9))\n"
                "assert(input.BottomBarSize == Vector2.new(6, 7))\n"
                "assert(input.NavBarSize == Vector2.new(4, 5))\n"
                "assert(input.StatusBarSize == Vector2.new(2, 3))\n"
                "for property, count in pairs(changes) do assert(count == 1, property .. ' changed ' .. count .. ' times') end\n"
                "input:SendAppUISizes(Vector2.new(8, 9), Vector2.new(6, 7), Vector2.new(4, 5), Vector2.new(2, 3))\n"
                "for property, count in pairs(changes) do assert(count == 1, property .. ' repeated unchanged notification') end\n"
                "for _, connection in pairs(connections) do connection:Disconnect() end\n"
                "return true"),
            "UserInputServiceAppUISizes", Reflection::Tuple());
    require(appUiSizeResult->at(0).convert<bool>(),
        "UserInputService app UI sizes must preserve all four insets and notify only changed properties");

    std::auto_ptr<Reflection::Tuple> raycastResult = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "local part = Instance.new('Part')\n"
            "part.Name = 'RaycastTarget'\n"
            "part.Anchored = true\n"
            "part.Size = Vector3.new(4, 4, 4)\n"
            "part.CFrame = CFrame.new(0, 0, 0)\n"
            "part.Parent = workspace\n"
            "local params = RaycastParams.new()\n"
            "assert(typeof(params) == 'RaycastParams')\n"
            "assert(params.FilterType == Enum.RaycastFilterType.Exclude)\n"
            "assert(params.IgnoreWater == false and params.CollisionGroup == 'Default')\n"
            "assert(params.RespectCanCollide == false and params.BruteForceAllSlow == false)\n"
            "local hit = workspace:Raycast(Vector3.new(0, 10, 0), Vector3.new(0, -20, 0), params)\n"
            "assert(hit ~= nil, 'Workspace:Raycast returned nil for an intersecting Part')\n"
            "assert(typeof(hit) == 'RaycastResult', 'Workspace:Raycast returned ' .. typeof(hit))\n"
            "assert(hit.Instance == part, 'RaycastResult.Instance did not preserve Part identity')\n"
            "assert(hit.Position == Vector3.new(0, 2, 0) and hit.Normal == Vector3.new(0, 1, 0))\n"
            "assert(hit.Material == Enum.Material.Plastic and hit.Distance == 8)\n"
            "params.FilterDescendantsInstances = {part}\n"
            "assert(params.FilterDescendantsInstances[1] == part)\n"
            "assert(workspace:Raycast(Vector3.new(0, 10, 0), Vector3.new(0, -20, 0), params) == nil)\n"
            "params.FilterType = Enum.RaycastFilterType.Include\n"
            "assert(workspace:Raycast(Vector3.new(0, 10, 0), Vector3.new(0, -20, 0), params).Instance == part)\n"
            "part.CanCollide = false\n"
            "params.RespectCanCollide = true\n"
            "assert(workspace:Raycast(Vector3.new(0, 10, 0), Vector3.new(0, -20, 0), params) == nil)\n"
            "part.CanCollide = true\n"
            "params.RespectCanCollide = false\n"
            "params.FilterType = Enum.RaycastFilterType.Exclude\n"
            "local farPart = Instance.new('Part')\n"
            "farPart.Anchored = true\n"
            "farPart.Size = Vector3.new(4, 4, 4)\n"
            "farPart.CFrame = CFrame.new(0, -6000, 0)\n"
            "farPart.Parent = workspace\n"
            "params.FilterDescendantsInstances = {part}\n"
            "local longHit = workspace:Raycast(Vector3.new(0, 10, 0), Vector3.new(0, -7000, 0), params)\n"
            "assert(longHit.Instance == farPart and math.abs(longHit.Distance - 6008) < 0.01)\n"
            "params.BruteForceAllSlow = true\n"
            "local slowHit = workspace:Raycast(Vector3.new(0, 10, 0), Vector3.new(0, -7000, 0), params)\n"
            "assert(slowHit.Instance == farPart and math.abs(slowHit.Distance - longHit.Distance) < 0.01)\n"
            "params.BruteForceAllSlow = false\n"
            "farPart:Destroy()\n"
            "params.FilterDescendantsInstances = {}\n"
            "local overlap = OverlapParams.new()\n"
            "assert(typeof(overlap) == 'OverlapParams' and overlap.MaxParts == 0)\n"
            "assert(overlap.FilterType == Enum.RaycastFilterType.Exclude and overlap.CollisionGroup == 'Default')\n"
            "local boxParts = workspace:GetPartBoundsInBox(CFrame.fromOrientation(0, math.pi / 4, 0), Vector3.new(3, 5, 3), overlap)\n"
            "assert(#boxParts == 1 and boxParts[1] == part)\n"
            "local radiusParts = workspace:GetPartBoundsInRadius(Vector3.new(3, 0, 0), 1.01, overlap)\n"
            "assert(#radiusParts == 1 and radiusParts[1] == part)\n"
            "overlap.FilterDescendantsInstances = {part}\n"
            "assert(#workspace:GetPartBoundsInBox(CFrame.new(), Vector3.new(5, 5, 5), overlap) == 0)\n"
            "overlap.FilterType = Enum.RaycastFilterType.Include\n"
            "overlap.MaxParts = 1\n"
            "assert(workspace:GetPartBoundsInBox(CFrame.new(), Vector3.new(5, 5, 5), overlap)[1] == part)\n"
            "overlap.FilterType = Enum.RaycastFilterType.Exclude\n"
            "overlap.FilterDescendantsInstances = {}\n"
            "local physics = game:GetService('PhysicsService')\n"
            "physics:RegisterCollisionGroup('Ghosts')\n"
            "assert(physics:GetMaxCollisionGroups() == 32)\n"
            "local registered, foundGhosts = physics:GetRegisteredCollisionGroups(), false\n"
            "for _, group in ipairs(registered) do\n"
            "    if group.name == 'Ghosts' then foundGhosts = type(group.mask) == 'number' end\n"
            "end\n"
            "assert(foundGhosts)\n"
            "part.CollisionGroup = 'Ghosts'\n"
            "assert(part.CollisionGroup == 'Ghosts' and part.CollisionGroupId == physics:GetCollisionGroupId('Ghosts'))\n"
            "physics:CollisionGroupSetCollidable('Default', 'Ghosts', false)\n"
            "assert(physics:CollisionGroupsAreCollidable('Default', 'Ghosts') == false)\n"
            "params.CollisionGroup = 'Default'\n"
            "assert(workspace:Raycast(Vector3.new(0, 10, 0), Vector3.new(0, -20, 0), params) == nil)\n"
            "params.CollisionGroup = 'Ghosts'\n"
            "assert(workspace:Raycast(Vector3.new(0, 10, 0), Vector3.new(0, -20, 0), params).Instance == part)\n"
            "physics:CollisionGroupSetCollidable('Default', 'Ghosts', true)\n"
            "params.CollisionGroup = 'Default'\n"
            "assert(workspace:Raycast(Vector3.new(0, 10, 0), Vector3.new(0, -20, 0), params).Instance == part)\n"
            "physics:RenameCollisionGroup('Ghosts', 'Spectres')\n"
            "assert(part.CollisionGroup == 'Spectres' and physics:IsCollisionGroupRegistered('Spectres'))\n"
            "physics:UnregisterCollisionGroup('Spectres')\n"
            "assert(part.CollisionGroup == 'Default' and part.CollisionGroupId == 0)\n"
            "part:Destroy()\n"
            "return true"),
        "WorkspaceRaycastContract", Reflection::Tuple());
    require(raycastResult->at(0).convert<bool>(),
        "Workspace ray and bounds queries must execute through physics with typed results and current filter semantics");

    static const unsigned char collisionGroupDataBytes[] = {
        0x01, 0x03,
        0x00, 0x04, 0xfb, 0xff, 0xff, 0xff, 0x07,
        'D', 'e', 'f', 'a', 'u', 'l', 't',
        0x01, 0x04, 0xfd, 0xff, 0xff, 0xff, 0x09,
        'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r',
        0x02, 0x04, 0xfe, 0xff, 0xff, 0xff, 0x19,
        'P', 'l', 'u', 'g', 'i', 'n', '_', 'U', 'n', 's', 'e', 'l', 'e', 'c', 't', 'a', 'b', 'l', 'e', '_', 'G', 'r', 'o', 'u', 'p'
    };
    const std::string collisionGroupData(
        reinterpret_cast<const char*>(collisionGroupDataBytes),
        sizeof(collisionGroupDataBytes));
    Workspace* workspace = contentDataModel->getWorkspace();
    const double firstServerTime = workspace->getServerTimeNow();
    const double secondServerTime = workspace->getServerTimeNow();
    require(firstServerTime > 1700000000.0 && secondServerTime >= firstServerTime,
        "Workspace:GetServerTimeNow must provide an advancing offline Unix clock");
    std::auto_ptr<Reflection::Tuple> serverTimeResult = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "local first = workspace:GetServerTimeNow()\n"
            "local second = workspace:GetServerTimeNow()\n"
            "return first > 1700000000 and second >= first"),
        "ServerTimeContract", Reflection::Tuple());
    require(serverTimeResult->at(0).convert<bool>(),
        "Workspace:GetServerTimeNow must be callable from ordinary experience scripts");
    boost::shared_ptr<Network::Player> availabilityPlayer;
    {
        Security::Impersonator permission(Security::Replicator_);
        availabilityPlayer = Creatable<Instance>::create<Network::Player>();
    }
    require(availabilityPlayer->getChatAvailabilityStatus() == "Unknown",
        "players must start with the current pending chat availability state");
    int availabilityChangeCount = 0;
    rbx::signals::scoped_connection availabilityConnection =
        availabilityPlayer->propertyChangedSignal.connect(
            [&availabilityChangeCount](const Reflection::PropertyDescriptor* property) {
                if (property && property->name == "ChatAvailabilityStatus")
                    ++availabilityChangeCount;
            });
    availabilityPlayer->setChatAvailabilityStatus("Enabled");
    availabilityPlayer->setChatAvailabilityStatus("Enabled");
    require(availabilityPlayer->getChatAvailabilityStatus() == "Enabled" &&
            availabilityChangeCount == 1,
        "Player chat availability must retain and signal a single Unknown-to-Enabled resolution");
    require(workspace->findPropertyDescriptor("CollisionGroupData") != NULL,
        "Workspace must reflect the current serialized CollisionGroupData property");
    std::stringstream collisionGroupPlace;
    collisionGroupPlace
        << "<roblox version=\"4\">"
        << "<Item class=\"Workspace\" referent=\"RBX1\"><Properties>"
        << "<BinaryString name=\"CollisionGroupData\">"
        << "AQMABPv///8HRGVmYXVsdAEE/f///wlDaGFyYWN0ZXICBP7///8ZUGx1Z2luX1Vuc2VsZWN0YWJsZV9Hcm91cA=="
        << "</BinaryString></Properties></Item></roblox>";
    Serializer().load(collisionGroupPlace, contentDataModel.get());
    require(workspace->getCollisionGroupData().value() == collisionGroupData,
        "the normal XML place loader must consume and round-trip current Studio CollisionGroupData bytes");
    PhysicsService* physicsService = ServiceProvider::find<PhysicsService>(workspace);
    require(physicsService && physicsService->getCollisionGroupId("Character") == 1 &&
            physicsService->getCollisionGroupId("Plugin_Unselectable_Group") == 2 &&
            !physicsService->collisionGroupsAreCollidable("Default", "Plugin_Unselectable_Group") &&
            physicsService->collisionGroupsAreCollidable("Default", "Character") &&
            !physicsService->collisionGroupsAreCollidable("Character", "Character"),
        "CollisionGroupData must install exact group IDs and the symmetric collision matrix");

    boost::shared_ptr<PartInstance> serializedGroupTarget =
        Creatable<Instance>::create<PartInstance>();
    serializedGroupTarget->setAnchored(true);
    serializedGroupTarget->setPartSizeXml(G3D::Vector3(4.0f, 4.0f, 4.0f));
    serializedGroupTarget->setCoordinateFrame(G3D::CoordinateFrame(G3D::Vector3::zero()));
    serializedGroupTarget->setCollisionGroup("Plugin_Unselectable_Group");
    serializedGroupTarget->setParent(workspace);
    RaycastParams serializedParams;
    serializedParams.collisionGroup = "Default";
    require(workspace->raycast(G3D::Vector3(0.0f, 10.0f, 0.0f),
                G3D::Vector3(0.0f, -20.0f, 0.0f), serializedParams).isVoid(),
        "imported collision masks must filter Workspace:Raycast queries from Default");
    serializedParams.collisionGroup = "Plugin_Unselectable_Group";
    require(!workspace->raycast(G3D::Vector3(0.0f, 10.0f, 0.0f),
                G3D::Vector3(0.0f, -20.0f, 0.0f), serializedParams).isVoid(),
        "imported collision masks must permit same-group Workspace:Raycast queries");
    serializedGroupTarget->setParent(NULL);

    std::string asymmetricData = collisionGroupData;
    asymmetricData[4] = static_cast<char>(0xff);
    bool rejectedAsymmetricData = false;
    try
    {
        workspace->setCollisionGroupData(BinaryString(asymmetricData));
    }
    catch (const std::exception&)
    {
        rejectedAsymmetricData = true;
    }
    require(rejectedAsymmetricData && workspace->getCollisionGroupData().value() == collisionGroupData,
        "invalid CollisionGroupData must be rejected atomically without changing the active matrix");

    boost::shared_ptr<TextLabel> fontLabel = Creatable<Instance>::create<TextLabel>();
    fontLabel->setFontFace(Font("rbxasset://fonts/families/BuilderSans.json", FONT_WEIGHT_BOLD));
    require(fontLabel->getFont() == TextService::FONT_BUILDERSANS_BOLD &&
            fontLabel->getFontFace().getWeight() == FONT_WEIGHT_BOLD,
        "FontFace must select the matching shipped Builder Sans rendering face");
    std::auto_ptr<Reflection::Tuple> fontResult = scriptContext->executeInNewThread(
        Security::GameScript_, ProtectedString::fromTrustedSource(
            "local face = Font.new('rbxasset://fonts/families/BuilderSans.json', "
            "Enum.FontWeight.SemiBold, Enum.FontStyle.Normal)\n"
            "local inherited = Font.fromEnum(Enum.Font.BuilderSansBold)\n"
            "local label = Instance.new('TextLabel')\n"
            "label.FontFace = face\n"
            "return face.Family == 'rbxasset://fonts/families/BuilderSans.json' "
            "and face.Weight == Enum.FontWeight.SemiBold and face.Style == Enum.FontStyle.Normal "
            "and not face.Bold and inherited.Bold and label.FontFace == face"),
        "FontContract", Reflection::Tuple());
    require(fontResult->at(0).convert<bool>(),
        "Luau Font constructors, enums, immutable accessors, and FontFace assignment must work");

    TextChatService* textChatService =
        ServiceProvider::create<TextChatService>(contentDataModel.get());
    RbxAnalyticsService* analyticsService =
        ServiceProvider::create<RbxAnalyticsService>(contentDataModel.get());
    std::auto_ptr<Reflection::Tuple> analyticsResult = scriptContext->executeInNewThread(
        Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
            "local analytics = game:GetService('RbxAnalyticsService')\n"
            "analytics:ReportCounter('ExperienceChatMount', 2)\n"
            "analytics:ReportCounter('ExperienceChatMount', 3)\n"
            "analytics:ReportStats('ExperienceChatLoadTime', 0.25)\n"
            "return true"),
        "OfflineAnalyticsContract", Reflection::Tuple());
    require(analyticsResult->at(0).convert<bool>() &&
            analyticsService->getReportedCounter("ExperienceChatMount") == 5,
        "offline analytics must retain counters locally without requiring a telemetry URL");
    ChatService* chatService =
        ServiceProvider::create<ChatService>(contentDataModel.get());
    require(!chatService->getBubbleChatEnabled() && chatService->getLoadDefaultChat(),
        "Chat must expose its recovered bubble-chat and default-chat state");
    bool bubbleSettingsChanged = false;
    rbx::signals::scoped_connection bubbleSettingsConnection =
        chatService->bubbleChatSettingsChangedSignal.connect(
            [&](Reflection::Variant settings) {
                bubbleSettingsChanged =
                    settings.isType<shared_ptr<const Reflection::ValueTable> >();
            });
    std::auto_ptr<Reflection::Tuple> bubbleChatResult = scriptContext->executeInNewThread(
        Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
            "local chat = game:GetService('Chat')\n"
            "local propertyChanged = false\n"
            "local connection = chat:GetPropertyChangedSignal('BubbleChatEnabled'):Connect(function() propertyChanged = true end)\n"
            "chat:SetBubbleChatSettings({ TextSize = 20 })\n"
            "connection:Disconnect()\n"
            "return chat.BubbleChatEnabled == false and chat.LoadDefaultChat == true and not propertyChanged"),
        "BubbleChatContract", Reflection::Tuple());
    require(bubbleChatResult->at(0).convert<bool>() && bubbleSettingsChanged,
        "Chat bubble settings must use the reflected property and settings-change pipeline");
    Instance* textChannels = textChatService->findFirstChildByName("TextChannels");
    TextChannel* generalChannel = textChannels
        ? Instance::fastDynamicCast<TextChannel>(
              textChannels->findFirstChildByName("RBXGeneral"))
        : nullptr;
    TextChannel* systemChannel = textChannels
        ? Instance::fastDynamicCast<TextChannel>(
              textChannels->findFirstChildByName("RBXSystem"))
        : nullptr;
    require(generalChannel && systemChannel,
        "TextChatService must create the current TextChannels/RBXGeneral/RBXSystem graph");
    bool channelReceived = false;
    bool serviceReceived = false;
    rbx::signals::scoped_connection channelConnection =
        systemChannel->messageReceivedSignal.connect(
            [&](shared_ptr<Instance>) { channelReceived = true; });
    rbx::signals::scoped_connection serviceConnection =
        textChatService->messageReceivedSignal.connect(
            [&](shared_ptr<Instance>) { serviceReceived = true; });
    shared_ptr<Instance> systemMessage =
        systemChannel->displaySystemMessage("Offline system message", "system");
    TextChatMessage* typedSystemMessage =
        Instance::fastDynamicCast<TextChatMessage>(systemMessage.get());
    require(channelReceived && serviceReceived && typedSystemMessage &&
            typedSystemMessage->getText() == "Offline system message" &&
            typedSystemMessage->getMetadata() == "system" &&
            typedSystemMessage->getStatus() == TextChatMessage::Success &&
            typedSystemMessage->getTextChannel() == systemChannel &&
            typedSystemMessage->getTextSource() == nullptr &&
            typedSystemMessage->getChatWindowMessageProperties() &&
            typedSystemMessage->getChatWindowMessageProperties()->getPrefixTextProperties(),
        "DisplaySystemMessage must produce and deliver a typed successful system message");
    std::auto_ptr<Reflection::Tuple> textChatResult = scriptContext->executeInNewThread(
        Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
            "local stage = 'start'\n"
            "local ok, result = pcall(function()\n"
            "local service = game:GetService('TextChatService')\n"
            "local channels = service:FindFirstChild('TextChannels')\n"
            "local general = channels:FindFirstChild('RBXGeneral')\n"
            "local system = channels:FindFirstChild('RBXSystem')\n"
            "assert(general:IsA('TextChannel') and system:IsA('TextChannel'))\n"
            "local callbackOrder = {}\n"
            "stage = 'assign service callback'\n"
            "service.OnIncomingMessage = function(message)\n"
            "  stage = 'run service callback'\n"
            "  table.insert(callbackOrder, 'service')\n"
            "  assert(message.Text == 'Hello' and message.PrefixText == '')\n"
            "  local properties = Instance.new('TextChatMessageProperties')\n"
            "  properties.Text = 'Filtered Hello'\n"
            "  properties.PrefixText = '[System]'\n"
            "  properties.Translation = 'Translated Hello'\n"
            "  return properties\n"
            "end\n"
            "stage = 'assign channel callback'\n"
            "system.OnIncomingMessage = function(message)\n"
            "  stage = 'run channel callback'\n"
            "  table.insert(callbackOrder, 'channel')\n"
            "  assert(message.Text == 'Filtered Hello' and message.PrefixText == '[System]')\n"
            "  local properties = Instance.new('TextChatMessageProperties')\n"
            "  properties.PrefixText = ''\n"
            "  return properties\n"
            "end\n"
            "local received, serviceReceived\n"
            "local connection = system.MessageReceived:Connect(function(message) received = message end)\n"
            "local serviceConnection = service.MessageReceived:Connect(function(message) serviceReceived = message end)\n"
            "stage = 'display system message'\n"
            "local message = system:DisplaySystemMessage('Hello', 'test')\n"
            "stage = 'validate result'\n"
            "connection:Disconnect()\n"
            "serviceConnection:Disconnect()\n"
            "service.OnIncomingMessage = nil\n"
            "system.OnIncomingMessage = nil\n"
            "return received == message and serviceReceived == message "
            "and callbackOrder[1] == 'service' and callbackOrder[2] == 'channel' "
            "and message.Text == 'Filtered Hello' and message.PrefixText == '[System]' "
            "and message.Translation == 'Translated Hello' and message.Metadata == 'test' "
            "and message.Status == Enum.TextChatMessageStatus.Success "
            "and message.TextChannel == system and message.TextSource == nil "
            "and typeof(message.Timestamp) == 'DateTime'\n"
            "end)\n"
            "if not ok then return false, stage .. ': ' .. tostring(result) end\n"
            "return true, result"),
        "TextChatContract", Reflection::Tuple());
    if (!textChatResult->at(0).convert<bool>())
        std::cerr << "TextChat callback contract failed: "
                  << textChatResult->at(1).convert<std::string>() << '\n';
    require(textChatResult->at(0).convert<bool>() &&
            textChatResult->at(1).convert<bool>(),
        "current TextChatService channel and system-message semantics must be available to Luau");

    std::auto_ptr<Reflection::Tuple> chatWindowPropertiesResult =
        scriptContext->executeInNewThread(
            Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
                "local service = game:GetService('TextChatService')\n"
                "local configuration = service:FindFirstChildOfClass('ChatWindowConfiguration')\n"
                "assert(configuration and configuration.Enabled)\n"
                "configuration.TextSize = 17\n"
                "configuration.TextStrokeTransparency = 0.4\n"
                "service.OnChatWindowAdded = function(message)\n"
                "  local properties = configuration:DeriveNewMessageProperties()\n"
                "  properties.TextColor3 = Color3.fromRGB(12, 34, 56)\n"
                "  properties.PrefixTextProperties.TextColor3 = Color3.fromRGB(90, 80, 70)\n"
                "  return properties\n"
                "end\n"
                "local message = service.TextChannels.RBXSystem:DisplaySystemMessage('Window', 'window')\n"
                "service.OnChatWindowAdded = nil\n"
                "local properties = message.ChatWindowMessageProperties\n"
                "return properties ~= nil and properties:IsA('ChatWindowMessageProperties') "
                "and properties.TextSize == 17 and properties.TextStrokeTransparency == 0.4 "
                "and properties.TextColor3 == Color3.fromRGB(12, 34, 56) "
                "and properties.PrefixTextProperties ~= nil "
                "and properties.PrefixTextProperties.TextColor3 == Color3.fromRGB(90, 80, 70)"),
            "ChatWindowMessagePropertiesContract", Reflection::Tuple());
    require(chatWindowPropertiesResult->at(0).convert<bool>(),
        "ChatWindowConfiguration derivation and OnChatWindowAdded must attach complete current message appearance data");

    std::auto_ptr<Reflection::Tuple> bubblePropertiesResult =
        scriptContext->executeInNewThread(
            Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
                "local service = game:GetService('TextChatService')\n"
                "service.OnBubbleAdded = function(message, adornee)\n"
                "  assert(message.Text == 'Offline system message' and adornee == nil)\n"
                "  local properties = Instance.new('BubbleChatMessageProperties')\n"
                "  properties.BackgroundColor3 = Color3.fromRGB(10, 20, 30)\n"
                "  properties.BackgroundTransparency = 0.35\n"
                "  properties.FontFace = Font.new('rbxasset://fonts/families/BuilderSans.json', Enum.FontWeight.Bold)\n"
                "  properties.TailVisible = false\n"
                "  properties.TextColor3 = Color3.fromRGB(230, 220, 210)\n"
                "  properties.TextSize = 19\n"
                "  return properties\n"
                "end\n"
                "return true"),
            "BubbleChatMessagePropertiesContract", Reflection::Tuple());
    require(bubblePropertiesResult->at(0).convert<bool>(),
        "BubbleChatMessageProperties callback setup must execute through Luau");
    textChatService->applyBubbleAddedCallback(systemMessage, shared_ptr<Instance>());
    BubbleChatMessageProperties* bubbleProperties =
        typedSystemMessage->getBubbleChatMessageProperties();
    require(bubbleProperties &&
            bubbleProperties->getBackgroundColor3() == Color3(10.0f / 255.0f,
                20.0f / 255.0f, 30.0f / 255.0f) &&
            std::abs(bubbleProperties->getBackgroundTransparency() - 0.35) < 1e-9 &&
            bubbleProperties->getFontFace().getWeight() == FONT_WEIGHT_BOLD &&
            !bubbleProperties->getTailVisible() &&
            bubbleProperties->getTextColor3() == Color3(230.0f / 255.0f,
                220.0f / 255.0f, 210.0f / 255.0f) &&
            bubbleProperties->getTextSize() == 19,
        "OnBubbleAdded must attach every current BubbleChatMessageProperties override to the message");
    std::auto_ptr<Reflection::Tuple> clearBubblePropertiesResult =
        scriptContext->executeInNewThread(
            Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
                "game:GetService('TextChatService').OnBubbleAdded = function() return nil end\n"
                "return true"),
            "BubbleChatMessagePropertiesNilContract", Reflection::Tuple());
    require(clearBubblePropertiesResult->at(0).convert<bool>(),
        "nil bubble-property callback setup must execute through Luau");
    textChatService->applyBubbleAddedCallback(systemMessage, shared_ptr<Instance>());
    require(typedSystemMessage->getBubbleChatMessageProperties() == nullptr,
        "an explicit nil OnBubbleAdded result must clear message bubble overrides");
    std::auto_ptr<Reflection::Tuple> invalidBubblePropertiesResult =
        scriptContext->executeInNewThread(
            Security::RobloxGameScript_, ProtectedString::fromTrustedSource(
                "game:GetService('TextChatService').OnBubbleAdded = function() return true end\n"
                "return true"),
            "BubbleChatMessagePropertiesInvalidContract", Reflection::Tuple());
    require(invalidBubblePropertiesResult->at(0).convert<bool>(),
        "invalid bubble-property callback setup must execute through Luau");
    bool rejectedInvalidBubbleProperties = false;
    try
    {
        textChatService->applyBubbleAddedCallback(systemMessage, shared_ptr<Instance>());
    }
    catch (const std::exception&)
    {
        rejectedInvalidBubbleProperties = true;
    }
    require(rejectedInvalidBubbleProperties,
        "OnBubbleAdded must reject callback values other than BubbleChatMessageProperties or nil");

    CoreGuiConfiguration* coreConfiguration =
        ServiceProvider::create<CoreGuiConfiguration>(contentDataModel.get());
    require(coreConfiguration->getPlayerListConfiguration() &&
            coreConfiguration->getCapturesViewConfiguration() &&
            coreConfiguration->getSelfViewConfiguration() &&
            coreConfiguration->getPlayerListConfiguration()->getEnabled() &&
            !coreConfiguration->getPlayerListConfiguration()->getOpen(),
        "CoreGuiConfiguration must own the recovered enabled/closed feature configurations");
    coreConfiguration->getPlayerListConfiguration()->setOpen(true);
    require(coreConfiguration->getPlayerListConfiguration()->getOpen(),
        "PlayerListConfiguration.Open must preserve the native Player UI state contract");

    return 0;
}
