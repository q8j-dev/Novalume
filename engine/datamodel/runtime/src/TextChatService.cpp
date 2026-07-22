#include "v8datamodel/TextChatService.h"

#include "rbx/core/EngineFeatures.h"
#include "v8datamodel/Folder.h"
#include "v8datamodel/TextChannel.h"
#include "v8datamodel/TextChatConfiguration.h"
#include "network/Player.h"
#include "network/Players.h"

namespace RBX {

const char* const sTextChatService = "TextChatService";

REFLECTION_BEGIN();
static Reflection::EnumPropDescriptor<TextChatService, TextChatService::ChatVersion>
    propChatVersion("ChatVersion", category_Data, &TextChatService::getChatVersion,
        &TextChatService::setChatVersion, Reflection::PropertyDescriptor::STANDARD,
        Security::None);
static Reflection::PropDescriptor<TextChatService, bool> propCreateDefaultCommands(
    "CreateDefaultCommands", category_Data, &TextChatService::getCreateDefaultCommands,
    &TextChatService::setCreateDefaultCommands, Reflection::PropertyDescriptor::SCRIPTING,
    Security::Plugin);
static Reflection::PropDescriptor<TextChatService, bool> propCreateDefaultTextChannels(
    "CreateDefaultTextChannels", category_Data,
    &TextChatService::getCreateDefaultTextChannels,
    &TextChatService::setCreateDefaultTextChannels,
    Reflection::PropertyDescriptor::SCRIPTING, Security::Plugin);
static Reflection::PropDescriptor<TextChatService, bool> propChatTranslationEnabled(
    "ChatTranslationEnabled", category_Data,
    &TextChatService::getChatTranslationEnabled,
    &TextChatService::setChatTranslationEnabled,
    Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<TextChatService, bool> propIsProtectedChatEnabled(
    "IsProtectedChatEnabled", category_Data,
    &TextChatService::getIsProtectedChatEnabled, NULL,
    Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<TextChatService, bool(int)> funcCanUserChatAsync(
    &TextChatService::canUserChatAsync, "CanUserChatAsync", "userId",
    Security::None);
static Reflection::EventDesc<TextChatService, void(shared_ptr<Instance>)>
    eventMessageReceived(&TextChatService::messageReceivedSignal, "MessageReceived",
        "textChatMessage", Security::None);
static Reflection::EventDesc<TextChatService, void(shared_ptr<Instance>)>
    eventSendingMessage(&TextChatService::sendingMessageSignal, "SendingMessage",
        "textChatMessage", Security::None);
static Reflection::BoundCallbackDesc<shared_ptr<const Reflection::Tuple>(
    shared_ptr<Instance>)> callbackOnIncomingMessage(
        "OnIncomingMessage", &TextChatService::onIncomingMessage, "message",
        Security::None);
static Reflection::BoundCallbackDesc<shared_ptr<const Reflection::Tuple>(
    shared_ptr<Instance>)> callbackOnChatWindowAdded(
        "OnChatWindowAdded", &TextChatService::onChatWindowAdded, "message",
        Security::None);
static Reflection::BoundCallbackDesc<shared_ptr<const Reflection::Tuple>(
    shared_ptr<Instance>, shared_ptr<Instance>)> callbackOnBubbleAdded(
        "OnBubbleAdded", &TextChatService::onBubbleAdded, "message", "adornee",
        Security::None);
REFLECTION_END();

namespace Reflection {

template<>
EnumDesc<TextChatService::ChatVersion>::EnumDesc()
    : EnumDescriptor("ChatVersion")
{
    addPair(TextChatService::LegacyChatService, "LegacyChatService");
    addPair(TextChatService::TextChatServiceVersion, "TextChatService");
}

template<>
TextChatService::ChatVersion& Variant::convert<TextChatService::ChatVersion>()
{
    return genericConvert<TextChatService::ChatVersion>();
}

} // namespace Reflection

template<>
bool StringConverter<TextChatService::ChatVersion>::convertToValue(
    const std::string& text, TextChatService::ChatVersion& value)
{
    return Reflection::EnumDesc<TextChatService::ChatVersion>::singleton()
        .convertToValue(text.c_str(), value);
}

TextChatService::TextChatService()
    : Service(true)
    , chatVersion(TextChatServiceVersion)
    , createDefaultCommands(true)
    , createDefaultTextChannels(true)
    , chatTranslationEnabled(false)
{
    setName(sTextChatService);
    setRobloxLocked(true);
}

void TextChatService::setChatVersion(ChatVersion value)
{
    if (chatVersion == value)
        return;
    chatVersion = value;
    raisePropertyChanged(propChatVersion);
}

void TextChatService::setCreateDefaultCommands(bool value)
{
    if (createDefaultCommands == value)
        return;
    createDefaultCommands = value;
    raisePropertyChanged(propCreateDefaultCommands);
}

void TextChatService::setCreateDefaultTextChannels(bool value)
{
    if (createDefaultTextChannels == value)
        return;
    createDefaultTextChannels = value;
    raisePropertyChanged(propCreateDefaultTextChannels);
    updateDefaultTextChannels();
}

void TextChatService::setChatTranslationEnabled(bool value)
{
    if (chatTranslationEnabled == value)
        return;
    chatTranslationEnabled = value;
    raisePropertyChanged(propChatTranslationEnabled);
}

bool TextChatService::getIsProtectedChatEnabled() const
{
    return EngineFeatures::isEnabled("TextChatServiceProtectedChatEnabled");
}

void TextChatService::canUserChatAsync(int userId,
    boost::function<void(bool)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    Network::Players* players =
        ServiceProvider::find<Network::Players>(this);
    if (!players)
    {
        errorFunction("TextChatService:CanUserChatAsync requires the Players service.");
        return;
    }

    shared_ptr<Instance> targetInstance = players->getPlayerInstanceByID(userId);
    Network::Player* target =
        Instance::fastDynamicCast<Network::Player>(targetInstance.get());
    if (!target)
    {
        errorFunction(
            "TextChatService:CanUserChatAsync userId is not in the current server.");
        return;
    }

    if (Network::Players::frontendProcessing(this))
    {
        Network::Player* localPlayer = Instance::fastDynamicCast<Network::Player>(
            players->getLocalPlayerDangerous());
        if (!localPlayer || localPlayer != target)
        {
            errorFunction(
                "TextChatService:CanUserChatAsync can only query the local player on a client.");
            return;
        }
    }

    resumeFunction(target->getChatAvailabilityStatus() == "Enabled");
}

void TextChatService::deliverMessage(const shared_ptr<Instance>& textChatMessage)
{
    if (!textChatMessage)
        throw std::runtime_error("TextChatService cannot deliver a nil message");
    messageReceivedSignal(textChatMessage);
}

namespace {

void applyFirstMessageOverride(const shared_ptr<const Reflection::Tuple>& values,
    TextChatMessage& message)
{
    if (!values || values->values.empty() ||
        !values->at(0).isType<shared_ptr<Instance>>())
        return;
    const shared_ptr<Instance> value = values->at(0).cast<shared_ptr<Instance>>();
    if (TextChatMessageProperties* properties =
            Instance::fastDynamicCast<TextChatMessageProperties>(value.get()))
        message.applyProperties(*properties);
}

} // namespace

void TextChatService::applyIncomingMessageCallbacks(
    const shared_ptr<Instance>& textChatMessage, TextChannel* channel)
{
    TextChatMessage* message =
        Instance::fastDynamicCast<TextChatMessage>(textChatMessage.get());
    if (!message)
        throw std::runtime_error(
            "TextChatService incoming callbacks require a TextChatMessage");
    if (onIncomingMessage)
        applyFirstMessageOverride(onIncomingMessage(textChatMessage), *message);
    if (channel && channel->onIncomingMessage)
        applyFirstMessageOverride(channel->onIncomingMessage(textChatMessage), *message);
}

void TextChatService::applyBubbleAddedCallback(
    const shared_ptr<Instance>& textChatMessage,
    const shared_ptr<Instance>& adornee)
{
    TextChatMessage* message =
        Instance::fastDynamicCast<TextChatMessage>(textChatMessage.get());
    if (!message)
        throw std::runtime_error("TextChatService bubble callback requires a TextChatMessage");
    if (!onBubbleAdded)
        return;
    const shared_ptr<const Reflection::Tuple> result =
        onBubbleAdded(textChatMessage, adornee);
    if (!result || result->values.empty())
        return;
    if (result->at(0).isVoid())
    {
        message->setBubbleChatMessageProperties(nullptr);
        return;
    }
    if (!result->at(0).isType<shared_ptr<Instance>>())
        throw std::runtime_error(
            "TextChatService.OnBubbleAdded must return BubbleChatMessageProperties or nil");
    const shared_ptr<Instance> value = result->at(0).cast<shared_ptr<Instance>>();
    if (!value)
    {
        message->setBubbleChatMessageProperties(nullptr);
        return;
    }
    BubbleChatMessageProperties* properties =
        Instance::fastDynamicCast<BubbleChatMessageProperties>(value.get());
    if (!properties)
        throw std::runtime_error(
            "TextChatService.OnBubbleAdded must return BubbleChatMessageProperties or nil");
    message->setBubbleChatMessageProperties(properties);
}

void TextChatService::applyChatWindowAddedCallback(
    const shared_ptr<Instance>& textChatMessage)
{
    TextChatMessage* message =
        Instance::fastDynamicCast<TextChatMessage>(textChatMessage.get());
    if (!message)
        throw std::runtime_error(
            "TextChatService chat-window callback requires a TextChatMessage");

    if (chatWindowConfiguration)
    {
        const shared_ptr<Instance> defaults =
            chatWindowConfiguration->deriveNewMessageProperties();
        message->setChatWindowMessageProperties(
            Instance::fastDynamicCast<ChatWindowMessageProperties>(defaults.get()));
    }
    if (!onChatWindowAdded)
        return;

    const shared_ptr<const Reflection::Tuple> result =
        onChatWindowAdded(textChatMessage);
    if (!result || result->values.empty() || result->at(0).isVoid())
        return;
    if (!result->at(0).isType<shared_ptr<Instance>>())
        throw std::runtime_error(
            "TextChatService.OnChatWindowAdded must return ChatWindowMessageProperties or nil");
    const shared_ptr<Instance> value = result->at(0).cast<shared_ptr<Instance>>();
    ChatWindowMessageProperties* properties =
        Instance::fastDynamicCast<ChatWindowMessageProperties>(value.get());
    if (!properties)
        throw std::runtime_error(
            "TextChatService.OnChatWindowAdded must return ChatWindowMessageProperties or nil");
    message->setChatWindowMessageProperties(properties);
}

void TextChatService::onServiceProvider(ServiceProvider* oldProvider,
    ServiceProvider* newProvider)
{
    Super::onServiceProvider(oldProvider, newProvider);
    updateDefaultTextChannels();
}

void TextChatService::updateDefaultTextChannels()
{
    if (!ServiceProvider::findServiceProvider(this)) {
        if (defaultTextChannels)
            defaultTextChannels->setParent(nullptr);
        defaultTextChannels.reset();
        if (chatInputBarConfiguration)
            chatInputBarConfiguration->setParent(nullptr);
        chatInputBarConfiguration.reset();
        if (chatWindowConfiguration)
            chatWindowConfiguration->setParent(nullptr);
        chatWindowConfiguration.reset();
        return;
    }
    if (!chatInputBarConfiguration) {
        chatInputBarConfiguration.reset(new ChatInputBarConfiguration());
        chatInputBarConfiguration->setParent(this);
    }
    if (!chatWindowConfiguration) {
        chatWindowConfiguration.reset(new ChatWindowConfiguration());
        chatWindowConfiguration->setParent(this);
    }
    if (!createDefaultTextChannels) {
        if (defaultTextChannels)
            defaultTextChannels->setParent(nullptr);
        defaultTextChannels.reset();
        return;
    }
    if (defaultTextChannels)
        return;

    defaultTextChannels = Creatable<Instance>::create<Folder>();
    defaultTextChannels->setName("TextChannels");
    defaultTextChannels->setParent(this);
    for (const char* name : {"RBXGeneral", "RBXSystem"}) {
        shared_ptr<TextChannel> channel = Creatable<Instance>::create<TextChannel>();
        channel->setName(name);
        channel->setParent(defaultTextChannels.get());
    }
}

} // namespace RBX
