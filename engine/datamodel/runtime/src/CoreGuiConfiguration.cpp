#include "v8datamodel/CoreGuiConfiguration.h"

namespace RBX {

const char* const sBaseCoreGuiConfiguration = "BaseCoreGuiConfiguration";
const char* const sCapturesViewConfiguration = "CapturesViewConfiguration";
const char* const sPlayerListConfiguration = "PlayerListConfiguration";
const char* const sSelfViewConfiguration = "SelfViewConfiguration";
const char* const sCoreGuiConfiguration = "CoreGuiConfiguration";

static const Reflection::PropDescriptor<BaseCoreGuiConfiguration, bool> prop_Enabled(
    "Enabled", category_Behavior, &BaseCoreGuiConfiguration::getEnabled,
    &BaseCoreGuiConfiguration::setEnabled, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE,
    Security::RobloxScript);
static const Reflection::PropDescriptor<CapturesViewConfiguration, bool> prop_CapturesOpen(
    "Open", category_Behavior, &CapturesViewConfiguration::getOpen,
    &CapturesViewConfiguration::setOpen, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE,
    Security::RobloxScript);
static const Reflection::PropDescriptor<PlayerListConfiguration, bool> prop_PlayerListOpen(
    "Open", category_Behavior, &PlayerListConfiguration::getOpen,
    &PlayerListConfiguration::setOpen, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE,
    Security::RobloxScript);
static const Reflection::PropDescriptor<SelfViewConfiguration, bool> prop_SelfViewOpen(
    "Open", category_Behavior, &SelfViewConfiguration::getOpen,
    &SelfViewConfiguration::setOpen, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE,
    Security::RobloxScript);

static const Reflection::RefPropDescriptor<CoreGuiConfiguration, CapturesViewConfiguration>
    prop_CapturesViewConfiguration(
        "CapturesViewConfiguration", category_Data,
        &CoreGuiConfiguration::getCapturesViewConfiguration, NULL,
        Reflection::PropertyDescriptor::UI, Security::RobloxScript);
static const Reflection::RefPropDescriptor<CoreGuiConfiguration, PlayerListConfiguration>
    prop_PlayerListConfiguration(
        "PlayerListConfiguration", category_Data,
        &CoreGuiConfiguration::getPlayerListConfiguration, NULL,
        Reflection::PropertyDescriptor::UI, Security::RobloxScript);
static const Reflection::RefPropDescriptor<CoreGuiConfiguration, SelfViewConfiguration>
    prop_SelfViewConfiguration(
        "SelfViewConfiguration", category_Data,
        &CoreGuiConfiguration::getSelfViewConfiguration, NULL,
        Reflection::PropertyDescriptor::UI, Security::RobloxScript);

BaseCoreGuiConfiguration::BaseCoreGuiConfiguration(const char* name)
    : DescribedNonCreatable<BaseCoreGuiConfiguration, Instance, sBaseCoreGuiConfiguration>(
          name)
    , enabled(true)
{
}

void BaseCoreGuiConfiguration::setEnabled(bool value)
{
    if (enabled != value)
    {
        enabled = value;
        raisePropertyChanged(prop_Enabled);
    }
}

CapturesViewConfiguration::CapturesViewConfiguration()
    : DescribedCreatable<CapturesViewConfiguration, BaseCoreGuiConfiguration,
          sCapturesViewConfiguration>("CapturesViewConfiguration")
    , open(false)
{
}

void CapturesViewConfiguration::setOpen(bool value)
{
    if (open != value)
    {
        open = value;
        raisePropertyChanged(prop_CapturesOpen);
    }
}

PlayerListConfiguration::PlayerListConfiguration()
    : DescribedCreatable<PlayerListConfiguration, BaseCoreGuiConfiguration,
          sPlayerListConfiguration>("PlayerListConfiguration")
    , open(false)
{
}

void PlayerListConfiguration::setOpen(bool value)
{
    if (open != value)
    {
        open = value;
        raisePropertyChanged(prop_PlayerListOpen);
    }
}

SelfViewConfiguration::SelfViewConfiguration()
    : DescribedCreatable<SelfViewConfiguration, BaseCoreGuiConfiguration,
          sSelfViewConfiguration>("SelfViewConfiguration")
    , open(false)
{
}

void SelfViewConfiguration::setOpen(bool value)
{
    if (open != value)
    {
        open = value;
        raisePropertyChanged(prop_SelfViewOpen);
    }
}

CoreGuiConfiguration::CoreGuiConfiguration()
    : DescribedNonCreatable<CoreGuiConfiguration, Instance, sCoreGuiConfiguration>(
          "CoreGuiConfiguration")
    , capturesViewConfiguration(
          Creatable<Instance>::create<CapturesViewConfiguration>())
    , playerListConfiguration(
          Creatable<Instance>::create<PlayerListConfiguration>())
    , selfViewConfiguration(
          Creatable<Instance>::create<SelfViewConfiguration>())
{
    capturesViewConfiguration->setParent(this);
    playerListConfiguration->setParent(this);
    selfViewConfiguration->setParent(this);
}

CapturesViewConfiguration* CoreGuiConfiguration::getCapturesViewConfiguration() const
{
    return capturesViewConfiguration.get();
}

PlayerListConfiguration* CoreGuiConfiguration::getPlayerListConfiguration() const
{
    return playerListConfiguration.get();
}

SelfViewConfiguration* CoreGuiConfiguration::getSelfViewConfiguration() const
{
    return selfViewConfiguration.get();
}

} // namespace RBX
