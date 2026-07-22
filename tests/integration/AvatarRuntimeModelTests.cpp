#include "v8datamodel/factoryregistration.h"
#include "v8datamodel/Bone.h"
#include "v8datamodel/Attachment.h"
#include "v8datamodel/MeshPart.h"
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/ModernAvatar.h"
#include "v8datamodel/JointInstance.h"
#include "v8datamodel/Value.h"
#include "humanoid/Humanoid.h"
#include "security/SecurityContext.h"
#include "v8xml/SerializerV2.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{

struct Asset
{
    std::string_view path;
    std::string_view rootClass;
    std::string_view rootName;
    std::size_t meshPartCount;
    std::size_t motorCount;
};

constexpr Asset assets[] = {
    {"content/avatar/characterR15.rbxm", "Model", "Player", 14, 15},
    {"content/avatar/characterR15DynamicHead.rbxm", "Model", "Player", 15, 15},
    {"content/avatar/characterR15DynamicHeadV2.rbxm", "Model", "Player", 15, 15},
    {"content/avatar/animations/humanoidR15AnimateChildren.rbxm",
        "Folder", "AnimationDefaultChildren", 0, 0},
    {"content/avatar/scripts/humanoidAnimateR15CharacterController.rbxm",
        "LocalScript", "Animate", 0, 0},
    {"content/avatar/scripts/humanoidAnimatePlayEmote.rbxm",
        "LocalScript", "Animate", 0, 0},
    {"content/avatar/scripts/humanoidAnimateR15Moods.rbxm",
        "LocalScript", "Animate", 0, 0},
    {"content/avatar/scripts/R15Moods.rbxm", "StringValue", "mood", 0, 0},
    {"content/avatar/scripts/R15MoodsV2.rbxm", "StringValue", "mood", 0, 0},
    {"content/avatar/scripts/humanoidHealthRegenScript.rbxmx",
        "Script", "Health", 0, 0},
    {"StudioContent/models/RigBuilder/AnthroRigs.rbxm",
        "Folder", "AnthroRigs", 30, 30},
    {"content/models/Thumbnails/Mannequins/R15-plus.rbxm",
        "Model", "Mannequin_LowPoly", 15, 0},
};

std::size_t countClass(const RBX::Instance& instance, std::string_view className)
{
    std::size_t result = instance.getClassNameStr() == className ? 1 : 0;
    for (std::size_t index = 0; index < instance.numChildren(); ++index)
        result += countClass(*instance.getChild(index), className);
    return result;
}

void collectMotors(RBX::Instance& instance,
    std::vector<boost::shared_ptr<RBX::Motor6D>>& motors)
{
    if (RBX::Motor6D* motor = RBX::Instance::fastDynamicCast<RBX::Motor6D>(&instance))
        motors.push_back(RBX::Instance::fastSharedDynamicCast<RBX::Motor6D>(
            RBX::shared_from(motor)));
    for (std::size_t index = 0; index < instance.numChildren(); ++index)
        collectMotors(*instance.getChild(index), motors);
}

void verifyMotorAttachments(const RBX::Motor6D& motor)
{
    RBX::PartInstance* part0 = motor.getPart0Dangerous();
    RBX::PartInstance* part1 = motor.getPart1Dangerous();
    const std::string attachmentName = motor.getName() + "RigAttachment";
    const RBX::Attachment* attachment0 = part0
        ? RBX::Instance::fastDynamicCast<const RBX::Attachment>(
            part0->findConstFirstChildByName(attachmentName)) : nullptr;
    const RBX::Attachment* attachment1 = part1
        ? RBX::Instance::fastDynamicCast<const RBX::Attachment>(
            part1->findConstFirstChildByName(attachmentName)) : nullptr;
    if (!part0 || !part1 || !attachment0 || !attachment1 ||
        !motor.getC0().fuzzyEq(attachment0->getFrameInPart(), 0.00001) ||
        !motor.getC1().fuzzyEq(attachment1->getFrameInPart(), 0.00001))
        throw std::runtime_error("R15 Motor6D did not preserve its RigAttachment frames: " +
            motor.getName());
}

RBX::Instances loadAssetRoots(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("could not open Studio avatar asset: " + path.string());
    RBX::Instances roots;
    SerializerV2().loadInstances(stream, roots);
    return roots;
}

void verifyR15HumanoidApis(RBX::Instance& rig, const std::filesystem::path& path)
{
    RBX::Humanoid* humanoid = rig.findFirstChildOfType<RBX::Humanoid>();
    if (!humanoid)
        throw std::runtime_error("standard R15 rig lost its Humanoid");

    const RBX::Reflection::ClassDescriptor& descriptor = humanoid->getDescriptor();
    const RBX::Reflection::FunctionDescriptor* buildRig =
        descriptor.findFunctionDescriptor("BuildRigFromAttachments");
    const RBX::Reflection::FunctionDescriptor* getBodyPart =
        descriptor.findFunctionDescriptor("GetBodyPartR15");
    const RBX::Reflection::FunctionDescriptor* replaceBodyPart =
        descriptor.findFunctionDescriptor("ReplaceBodyPartR15");
    const RBX::Reflection::FunctionDescriptor* getAccessoryScale =
        descriptor.findFunctionDescriptor("GetAccessoryHandleScale");
    if (!buildRig || !getBodyPart || !replaceBodyPart || !getAccessoryScale ||
        buildRig->security != RBX::Security::None ||
        getBodyPart->security != RBX::Security::None ||
        replaceBodyPart->security != RBX::Security::None ||
        getAccessoryScale->security != RBX::Security::RobloxScript)
        throw std::runtime_error("current R15 Humanoid function reflection contract changed");

    boost::shared_ptr<RBX::ModelInstance> runtimeContainer =
        RBX::Creatable<RBX::Instance>::create<RBX::ModelInstance>();
    rig.setParent(runtimeContainer.get());
    {
        RBX::Security::Impersonator permission(RBX::Security::Replicator_);
        humanoid->setRigType(RBX::Humanoid::HUMANOID_RIG_TYPE_R15);
    }
    humanoid->setAutomaticScalingEnabled(true);

    RBX::PartInstance* upperTorso = RBX::Instance::fastDynamicCast<RBX::PartInstance>(
        rig.findFirstChildByName("UpperTorso"));
    if (!upperTorso || humanoid->getBodyPartR15(RBX::shared_from(upperTorso)) !=
            RBX::Humanoid::BODY_PART_R15_UPPER_TORSO)
        throw std::runtime_error("GetBodyPartR15 did not identify authentic UpperTorso");

    const G3D::Vector3 handleScale = humanoid->getAccessoryHandleScale(
        RBX::shared_from(upperTorso), RBX::Humanoid::BODY_PART_R15_UPPER_TORSO);
    if ((handleScale - G3D::Vector3::one()).length() > 0.0001f)
        throw std::runtime_error("GetAccessoryHandleScale lost default classic R15 dimensions");

    std::vector<boost::shared_ptr<RBX::Motor6D>> originalMotors;
    collectMotors(rig, originalMotors);
    if (originalMotors.size() != 15)
        throw std::runtime_error("standard R15 rig did not begin with 15 Motor6Ds");
    for (const boost::shared_ptr<RBX::Motor6D>& motor : originalMotors)
        motor->setParent(nullptr);
    if (countClass(rig, "Motor6D") != 0)
        throw std::runtime_error("R15 test could not detach authored motors");
    humanoid->buildRigFromAttachments();
    std::vector<boost::shared_ptr<RBX::Motor6D>> rebuiltMotors;
    collectMotors(rig, rebuiltMotors);
    if (rebuiltMotors.size() != 15)
        throw std::runtime_error("BuildRigFromAttachments did not rebuild 15 authentic joints");
    for (const boost::shared_ptr<RBX::Motor6D>& motor : rebuiltMotors)
        verifyMotorAttachments(*motor);
    humanoid->buildRigFromAttachments();
    if (countClass(rig, "Motor6D") != 15)
        throw std::runtime_error("BuildRigFromAttachments was not idempotent");

    RBX::Instances replacementRoots = loadAssetRoots(path);
    if (replacementRoots.size() != 1)
        throw std::runtime_error("replacement R15 asset root count changed");
    RBX::PartInstance* replacementRaw = RBX::Instance::fastDynamicCast<RBX::PartInstance>(
        replacementRoots.front()->findFirstChildByName("UpperTorso"));
    if (!replacementRaw)
        throw std::runtime_error("replacement R15 asset lost UpperTorso");
    boost::shared_ptr<RBX::PartInstance> replacement =
        RBX::Instance::fastSharedDynamicCast<RBX::PartInstance>(RBX::shared_from(replacementRaw));
    std::vector<boost::shared_ptr<RBX::Motor6D>> replacementMotors;
    collectMotors(*replacement, replacementMotors);
    for (const boost::shared_ptr<RBX::Motor6D>& motor : replacementMotors)
        motor->setParent(nullptr);
    replacement->setParent(nullptr);
    if (humanoid->getBodyPartR15(replacement) != RBX::Humanoid::BODY_PART_R15_UNKNOWN)
        throw std::runtime_error("GetBodyPartR15 accepted a detached replacement");

    RBX::PartInstance* previousUpperTorso = upperTorso;
    if (!humanoid->replaceBodyPartR15(
            RBX::Humanoid::BODY_PART_R15_UPPER_TORSO, replacement) ||
        replacement->getParent() != &rig || previousUpperTorso->getParent() != nullptr ||
        humanoid->getBodyPartR15(replacement) !=
            RBX::Humanoid::BODY_PART_R15_UPPER_TORSO)
        throw std::runtime_error("ReplaceBodyPartR15 did not atomically replace UpperTorso");
    std::vector<boost::shared_ptr<RBX::Motor6D>> replacedMotors;
    collectMotors(rig, replacedMotors);
    if (replacedMotors.size() != 15)
        throw std::runtime_error("ReplaceBodyPartR15 changed the authentic joint count");
    for (const boost::shared_ptr<RBX::Motor6D>& motor : replacedMotors)
    {
        if (motor->getPart0Dangerous() == previousUpperTorso ||
            motor->getPart1Dangerous() == previousUpperTorso)
            throw std::runtime_error("ReplaceBodyPartR15 retained a joint to the old part");
        verifyMotorAttachments(*motor);
    }
    rig.setParent(nullptr);
}

void verifyMeshParts(const RBX::Instance& instance)
{
    if (const RBX::MeshPart* meshPart =
            RBX::Instance::fastDynamicCast<RBX::MeshPart>(&instance))
    {
        if (meshPart->getMeshId().isNull() ||
            meshPart->getInitialSize().min() <= 0.0f)
            throw std::runtime_error("R15 MeshPart lost its mesh or authored initial size");
    }
    for (std::size_t index = 0; index < instance.numChildren(); ++index)
        verifyMeshParts(*instance.getChild(index));
}

void verifyAnthroRig(RBX::Instance& folder, std::string_view name)
{
    RBX::Instance* rig = folder.findFirstChildByName(std::string(name));
    if (!rig || rig->getClassNameStr() != "Model" ||
        countClass(*rig, "MeshPart") != 15 ||
        countClass(*rig, "Motor6D") != 15 ||
        countClass(*rig, "Attachment") != 48 ||
        countClass(*rig, "Vector3Value") != 64 ||
        countClass(*rig, "StringValue") != 15)
        throw std::runtime_error("Studio Rthro rig hierarchy changed: " +
            std::string(name));

    RBX::Humanoid* humanoid = rig->findFirstChildOfType<RBX::Humanoid>();
    RBX::Instance* bodyType = humanoid
        ? humanoid->findFirstChildByName("BodyTypeScale") : nullptr;
    RBX::Instance* bodyProportion = humanoid
        ? humanoid->findFirstChildByName("BodyProportionScale") : nullptr;
    if (!humanoid || humanoid->getRootPartDangerous() == nullptr ||
        humanoid->getRootPartDangerous()->getName() != "HumanoidRootPart" ||
        !RBX::Instance::fastDynamicCast<RBX::DoubleValue>(bodyType) ||
        !RBX::Instance::fastDynamicCast<RBX::DoubleValue>(bodyProportion))
        throw std::runtime_error("Studio Rthro Humanoid scale contract changed: " +
            std::string(name));

    RBX::DoubleValue* bodyTypeValue =
        RBX::Instance::fastDynamicCast<RBX::DoubleValue>(bodyType);
    RBX::DoubleValue* bodyProportionValue =
        RBX::Instance::fastDynamicCast<RBX::DoubleValue>(bodyProportion);
    RBX::PartInstance* upperTorso = RBX::Instance::fastDynamicCast<RBX::PartInstance>(
        rig->findFirstChildByName("UpperTorso"));
    RBX::Vector3Value* originalSize = upperTorso
        ? RBX::Instance::fastDynamicCast<RBX::Vector3Value>(
            upperTorso->findFirstChildByName("OriginalSize")) : nullptr;
    if (!bodyTypeValue || !bodyProportionValue || !upperTorso || !originalSize)
        throw std::runtime_error("Studio Rthro scaling metadata changed: " +
            std::string(name));

    {
        RBX::Security::Impersonator permission(RBX::Security::Replicator_);
        humanoid->setRigType(RBX::Humanoid::HUMANOID_RIG_TYPE_R15);
    }
    humanoid->setAutomaticScalingEnabled(true);
    const G3D::Vector3 classic(2.0f, 1.60003f, 1.0f);
    const G3D::Vector3 normal(1.35398f, 2.08418f, 1.20334f);
    const G3D::Vector3 slender(1.61288f, 2.75854f, 1.17362f);
    const bool isNormal = name == "AnthroNormal";
    const G3D::Vector3 authored = isNormal ? normal : slender;
    bodyProportionValue->setValue(isNormal ? 1.0 : 0.0);
    const G3D::Vector3 opposite = isNormal ? slender : normal;
    const G3D::Vector3 expectedOpposite = originalSize->getValue() *
        G3D::Vector3(opposite.x / authored.x, opposite.y / authored.y,
            opposite.z / authored.z);
    if ((upperTorso->getPartSizeXml() - expectedOpposite).length() > 0.0001f)
        throw std::runtime_error("Studio Rthro normal/slender interpolation failed: " +
            std::string(name));

    bodyTypeValue->setValue(0.0);
    const G3D::Vector3 expectedClassic = originalSize->getValue() *
        G3D::Vector3(classic.x / authored.x, classic.y / authored.y,
            classic.z / authored.z);
    if ((upperTorso->getPartSizeXml() - expectedClassic).length() > 0.0001f)
        throw std::runtime_error("Studio Rthro body-type interpolation failed: " +
            std::string(name));

    bodyProportionValue->setValue(isNormal ? 0.0 : 1.0);
    bodyTypeValue->setValue(1.0);

    RBX::PartInstance* root = humanoid->getRootPartDangerous();
    RBX::Attachment* rootAttachment = root
        ? RBX::Instance::fastDynamicCast<RBX::Attachment>(
            root->findFirstChildByName("RootRigAttachment")) : nullptr;
    const float expectedRootOffset = isNormal ? -0.987f : -0.917431f;
    if (!rootAttachment || std::abs(
            rootAttachment->getFrameInPart().translation.y - expectedRootOffset) >
            0.0001f)
        throw std::runtime_error(
            "Studio Rthro RootRigAttachment scaling failed: " +
            std::string(name));

    verifyMeshParts(*rig);
}

void verifyR15PlusInstance(RBX::Instance& instance,
    std::size_t& constraintCount, std::size_t& boneCount,
	std::size_t& wrapDataCount)
{
    if (instance.getClassNameStr() == "AnimationConstraint")
    {
        RBX::AnimationConstraint* constraint =
            RBX::Instance::fastDynamicCast<RBX::AnimationConstraint>(&instance);
        if (!constraint || !constraint->getAttachment0() ||
            !constraint->getAttachment1() || !constraint->getPart0() ||
            !constraint->getPart1() ||
            constraint->getPart0() == constraint->getPart1())
            throw std::runtime_error(
                "R15-plus AnimationConstraint did not bind its real attachments and parts: " +
                instance.getName() + " attachment0=" +
                (constraint && constraint->getAttachment0() ? constraint->getAttachment0()->getName() : "nil") +
                " attachment1=" +
                (constraint && constraint->getAttachment1() ? constraint->getAttachment1()->getName() : "nil") +
                " part0=" +
                (constraint && constraint->getPart0() ? constraint->getPart0()->getName() : "nil") +
                " part1=" +
                (constraint && constraint->getPart1() ? constraint->getPart1()->getName() : "nil"));
        ++constraintCount;
    }
    else if (instance.getClassNameStr() == "Bone")
    {
        RBX::Bone* bone = RBX::Instance::fastDynamicCast<RBX::Bone>(&instance);
        if (!bone || (!RBX::Instance::fastDynamicCast<RBX::Bone>(bone->getParent()) &&
                !RBX::Instance::fastDynamicCast<RBX::PartInstance>(bone->getParent())))
            throw std::runtime_error("R15-plus Bone lost its authentic hierarchy");
        ++boneCount;
    }
	else if (instance.getClassNameStr() == "WrapTarget")
	{
		RBX::WrapTarget* wrap =
			RBX::Instance::fastDynamicCast<RBX::WrapTarget>(&instance);
		if (!wrap || wrap->getHSRData().value().empty())
			throw std::runtime_error("R15-plus WrapTarget lost its shared HSR data");
		++wrapDataCount;
	}

    for (std::size_t index = 0; index < instance.numChildren(); ++index)
        verifyR15PlusInstance(*instance.getChild(index), constraintCount,
			boneCount, wrapDataCount);
}

void verifyR15PlusRig(RBX::Instance& rig)
{
    struct ClassCount
    {
        std::string_view name;
        std::size_t expected;
    };
    constexpr ClassCount expected[] = {
        {"AnimationConstraint", 15}, {"Attachment", 51},
        {"BallSocketConstraint", 14}, {"Bone", 37}, {"CFrameValue", 165},
        {"FaceControls", 1}, {"Folder", 1}, {"Humanoid", 1},
        {"MeshPart", 15}, {"Model", 1}, {"NoCollisionConstraint", 19},
        {"NumberValue", 6}, {"Part", 1}, {"StringValue", 15},
        {"Vector3Value", 104}, {"WrapTarget", 15},
    };
    for (const ClassCount& item : expected)
        if (countClass(rig, item.name) != item.expected)
            throw std::runtime_error("Studio R15-plus class count changed: " +
                std::string(item.name));

    RBX::Humanoid* humanoid = rig.findFirstChildOfType<RBX::Humanoid>();
    if (!humanoid || !humanoid->getRootPartDangerous() ||
        humanoid->getRootPartDangerous()->getName() != "HumanoidRootPart")
        throw std::runtime_error("Studio R15-plus Humanoid root did not resolve");

    // Serializer roots are intentionally detached. Parenting the authentic rig
    // exercises the normal runtime ancestry transition that binds each
    // AnimationConstraint after all cross-instance references have resolved.
    boost::shared_ptr<RBX::ModelInstance> runtimeContainer =
        RBX::Creatable<RBX::Instance>::create<RBX::ModelInstance>();
    rig.setParent(runtimeContainer.get());

    RBX::PartInstance* upperTorso = RBX::Instance::fastDynamicCast<RBX::PartInstance>(
        rig.findFirstChildByName("UpperTorso"));
    RBX::DoubleValue* widthScale = RBX::Instance::fastDynamicCast<RBX::DoubleValue>(
        humanoid->findFirstChildByName("BodyWidthScale"));
    RBX::DoubleValue* heightScale = RBX::Instance::fastDynamicCast<RBX::DoubleValue>(
        humanoid->findFirstChildByName("BodyHeightScale"));
    RBX::DoubleValue* depthScale = RBX::Instance::fastDynamicCast<RBX::DoubleValue>(
        humanoid->findFirstChildByName("BodyDepthScale"));
    RBX::Vector3Value* originalSize = upperTorso
        ? RBX::Instance::fastDynamicCast<RBX::Vector3Value>(
            upperTorso->findFirstChildByName("OriginalSize")) : nullptr;
    RBX::Attachment* waist = upperTorso
        ? RBX::Instance::fastDynamicCast<RBX::Attachment>(
            upperTorso->findFirstChildByName("WaistRigAttachment")) : nullptr;
    RBX::Vector3Value* originalWaist = waist
        ? RBX::Instance::fastDynamicCast<RBX::Vector3Value>(
            waist->findFirstChildByName("OriginalPosition")) : nullptr;
    if (!upperTorso || !widthScale || !heightScale || !depthScale ||
        !originalSize || !waist || !originalWaist)
        throw std::runtime_error("R15-plus scaling metadata did not deserialize");

    {
        RBX::Security::Impersonator permission(RBX::Security::Replicator_);
        humanoid->setRigType(RBX::Humanoid::HUMANOID_RIG_TYPE_R15);
    }
    humanoid->setAutomaticScalingEnabled(true);
    widthScale->setValue(1.25);
    heightScale->setValue(1.1);
    depthScale->setValue(0.8);
    const G3D::Vector3 expectedScale(1.25f, 1.1f, 0.8f);
    const G3D::Vector3 expectedSize = originalSize->getValue() * expectedScale;
    const G3D::Vector3 expectedWaist = originalWaist->getValue() * expectedScale;
    if ((upperTorso->getPartSizeXml() - expectedSize).length() > 0.0001f ||
        (waist->getFrameInPart().translation - expectedWaist).length() > 0.0001f)
        throw std::runtime_error(
            "R15-plus Humanoid scaling did not update size and attachment frames: size error=" +
            std::to_string((upperTorso->getPartSizeXml() - expectedSize).length()) +
            " attachment error=" +
            std::to_string((waist->getFrameInPart().translation - expectedWaist).length()) +
            " auto=" + std::to_string(humanoid->getAutomaticScalingEnabled()) +
            " rig=" + std::to_string(static_cast<int>(humanoid->getRigType())));

    std::size_t constraintCount = 0;
    std::size_t boneCount = 0;
    std::size_t wrapDataCount = 0;
    verifyR15PlusInstance(rig, constraintCount, boneCount, wrapDataCount);
    if (constraintCount != 15 || boneCount != 37 || wrapDataCount != 15)
        throw std::runtime_error("Studio R15-plus concrete class registration changed");
    verifyMeshParts(rig);
    rig.setParent(nullptr);
}

void printTree(const RBX::Instance& instance, std::size_t depth)
{
    std::cout << std::string(depth * 2, ' ') << instance.getClassNameStr()
              << ':' << instance.getName();
    if (const RBX::PartInstance* part =
            RBX::Instance::fastDynamicCast<const RBX::PartInstance>(&instance))
        std::cout << " size=" << part->getPartSizeXml().x << ','
                  << part->getPartSizeXml().y << ',' << part->getPartSizeXml().z
                  << " position=" << part->getCoordinateFrame().translation
                  << " canCollide=" << part->getCanCollide();
    if (const RBX::MeshPart* mesh =
            RBX::Instance::fastDynamicCast<const RBX::MeshPart>(&instance))
        std::cout << " mesh=" << mesh->getMeshId().toString()
                  << " texture=" << mesh->getTextureId().toString();
    if (const RBX::Humanoid* humanoid =
            RBX::Instance::fastDynamicCast<const RBX::Humanoid>(&instance))
        std::cout << " rigType=" << humanoid->getRigType()
                  << " hipHeight=" << humanoid->getHipHeight();
    if (const RBX::StringValue* value =
            RBX::Instance::fastDynamicCast<const RBX::StringValue>(&instance))
        std::cout << " value=" << value->getValue();
    if (const RBX::DoubleValue* value =
            RBX::Instance::fastDynamicCast<const RBX::DoubleValue>(&instance))
        std::cout << " value=" << value->getValue();
    if (const RBX::Vector3Value* value =
            RBX::Instance::fastDynamicCast<const RBX::Vector3Value>(&instance))
        std::cout << " value=" << value->getValue();
    if (const RBX::Attachment* attachment =
            RBX::Instance::fastDynamicCast<const RBX::Attachment>(&instance))
        std::cout << " frame=" << attachment->getFrameInPart().translation;
    std::cout << '\n';
    for (std::size_t index = 0; index < instance.numChildren(); ++index)
        printTree(*instance.getChild(index), depth + 1);
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2 && argc != 3)
        throw std::runtime_error("expected the supplied Studio root");

    static std::once_flag registration;
    std::call_once(registration, [] { static RBX::FactoryRegistrator registrator; });

    const std::filesystem::path root(argv[1]);
    if (argc == 3)
    {
        const std::filesystem::path path = root / argv[2];
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
            throw std::runtime_error("could not open Studio avatar asset: " + path.string());
        RBX::Instances roots;
        SerializerV2().loadInstances(stream, roots);
        std::cout << "roots=" << roots.size() << '\n';
        for (const auto& assetRoot : roots)
            printTree(*assetRoot, 0);
        return 0;
    }
    for (const Asset& asset : assets)
    {
        const std::filesystem::path path = root / asset.path;
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
            throw std::runtime_error("could not open Studio avatar asset: " + path.string());

        RBX::Instances roots;
        SerializerV2().loadInstances(stream, roots);
        if (roots.size() != 1 || roots.front()->getClassNameStr() != asset.rootClass ||
            roots.front()->getName() != asset.rootName ||
            countClass(*roots.front(), "MeshPart") != asset.meshPartCount ||
            countClass(*roots.front(), "Motor6D") != asset.motorCount)
            throw std::runtime_error("Studio avatar asset hierarchy changed: " + path.string());

        verifyMeshParts(*roots.front());

        if (asset.rootName == "Player")
        {
            RBX::Humanoid* humanoid =
                roots.front()->findFirstChildOfType<RBX::Humanoid>();
            if (!humanoid || !humanoid->getRootPartDangerous() ||
                humanoid->getRootPartDangerous()->getName() != "HumanoidRootPart")
                throw std::runtime_error(
                    "Studio R15 Humanoid.RootPart did not resolve HumanoidRootPart");
            if (asset.path == "content/avatar/characterR15.rbxm")
                verifyR15HumanoidApis(*roots.front(), path);
        }

        if (asset.rootName == "Animate" &&
            (!roots.front()->findFirstChildByName("idle") ||
             !roots.front()->findFirstChildByName("PlayEmote")))
            throw std::runtime_error("Studio Animate model lacks its animation/emote children");

        if (asset.rootName == "AnthroRigs")
        {
            if (roots.front()->numChildren() != 2)
                throw std::runtime_error("Studio AnthroRigs must contain exactly two rigs");
            verifyAnthroRig(*roots.front(), "AnthroNormal");
            verifyAnthroRig(*roots.front(), "AnthroSlender");
        }

        if (asset.rootName == "Mannequin_LowPoly")
            verifyR15PlusRig(*roots.front());
    }
    return 0;
}
