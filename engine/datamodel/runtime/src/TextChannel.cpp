#include "v8datamodel/TextChannel.h"

#include "v8datamodel/TextChatService.h"
#include "network/Player.h"
#include "network/Players.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <sstream>

namespace RBX {

const char* const sTextChannel = "TextChannel";
const char* const sTextChatMessage = "TextChatMessage";
const char* const sTextChatMessageProperties = "TextChatMessageProperties";
const char* const sBubbleChatMessageProperties = "BubbleChatMessageProperties";
const char* const sChatWindowMessageProperties = "ChatWindowMessageProperties";
const char* const sTextSource = "TextSource";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<TextChatMessageProperties, std::string>
    propOverridePrefixText("PrefixText", category_Data,
        &TextChatMessageProperties::getPrefixText,
        &TextChatMessageProperties::setPrefixText,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<BubbleChatMessageProperties, Color3>
    propBubbleBackgroundColor3("BackgroundColor3", category_Appearance,
        &BubbleChatMessageProperties::getBackgroundColor3,
        &BubbleChatMessageProperties::setBackgroundColor3,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<BubbleChatMessageProperties, double>
    propBubbleBackgroundTransparency("BackgroundTransparency", category_Appearance,
        &BubbleChatMessageProperties::getBackgroundTransparency,
        &BubbleChatMessageProperties::setBackgroundTransparency,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<BubbleChatMessageProperties, Font>
    propBubbleFontFace("FontFace", category_Appearance,
        &BubbleChatMessageProperties::getFontFace,
        &BubbleChatMessageProperties::setFontFace,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<BubbleChatMessageProperties, bool>
    propBubbleTailVisible("TailVisible", category_Appearance,
        &BubbleChatMessageProperties::getTailVisible,
        &BubbleChatMessageProperties::setTailVisible,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<BubbleChatMessageProperties, Color3>
    propBubbleTextColor3("TextColor3", category_Appearance,
        &BubbleChatMessageProperties::getTextColor3,
        &BubbleChatMessageProperties::setTextColor3,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<BubbleChatMessageProperties, long long>
    propBubbleTextSize("TextSize", category_Appearance,
        &BubbleChatMessageProperties::getTextSize,
        &BubbleChatMessageProperties::setTextSize,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<ChatWindowMessageProperties, Font>
    propWindowMessageFontFace("FontFace", category_Appearance,
        &ChatWindowMessageProperties::getFontFace,
        &ChatWindowMessageProperties::setFontFace,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::RefPropDescriptor<ChatWindowMessageProperties,
    ChatWindowMessageProperties> propPrefixTextProperties(
        "PrefixTextProperties", category_Appearance,
        &ChatWindowMessageProperties::getPrefixTextProperties,
        &ChatWindowMessageProperties::setPrefixTextProperties,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<ChatWindowMessageProperties, Color3>
    propWindowMessageTextColor3("TextColor3", category_Appearance,
        &ChatWindowMessageProperties::getTextColor3,
        &ChatWindowMessageProperties::setTextColor3,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<ChatWindowMessageProperties, int>
    propWindowMessageTextSize("TextSize", category_Appearance,
        &ChatWindowMessageProperties::getTextSize,
        &ChatWindowMessageProperties::setTextSize,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<ChatWindowMessageProperties, Color3>
    propWindowMessageTextStrokeColor3("TextStrokeColor3", category_Appearance,
        &ChatWindowMessageProperties::getTextStrokeColor3,
        &ChatWindowMessageProperties::setTextStrokeColor3,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<ChatWindowMessageProperties, double>
    propWindowMessageTextStrokeTransparency("TextStrokeTransparency",
        category_Appearance,
        &ChatWindowMessageProperties::getTextStrokeTransparency,
        &ChatWindowMessageProperties::setTextStrokeTransparency,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<TextChatMessageProperties, std::string>
    propOverrideText("Text", category_Data,
        &TextChatMessageProperties::getText,
        &TextChatMessageProperties::setText,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<TextChatMessageProperties, std::string>
    propOverrideTranslation("Translation", category_Data,
        &TextChatMessageProperties::getTranslation,
        &TextChatMessageProperties::setTranslation,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<TextChatMessage, std::string> propMessageId(
    "MessageId", category_Data, &TextChatMessage::getMessageId, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<TextChatMessage, std::string> propMetadata(
    "Metadata", category_Data, &TextChatMessage::getMetadata, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<TextChatMessage, std::string> propPrefixText(
    "PrefixText", category_Data, &TextChatMessage::getPrefixText, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::EnumPropDescriptor<TextChatMessage, TextChatMessage::Status> propStatus(
    "Status", category_Data, &TextChatMessage::getStatus, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<TextChatMessage, std::string> propText(
    "Text", category_Data, &TextChatMessage::getText, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::RefPropDescriptor<TextChatMessage, TextChannel> propTextChannel(
    "TextChannel", category_Data, &TextChatMessage::getTextChannel, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::RefPropDescriptor<TextChatMessage, Instance> propTextSource(
    "TextSource", category_Data, &TextChatMessage::getTextSource, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<TextChatMessage, DateTime> propTimestamp(
    "Timestamp", category_Data, &TextChatMessage::getTimestamp, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<TextChatMessage, std::string> propTranslation(
    "Translation", category_Data, &TextChatMessage::getTranslation, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<TextSource, int> propTextSourceUserId(
    "UserId", category_Data, &TextSource::getUserId, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<TextSource, bool> propTextSourceCanSend(
    "CanSend", category_Data, &TextSource::getCanSend, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::RefPropDescriptor<TextChatMessage, BubbleChatMessageProperties>
    propBubbleChatMessageProperties("BubbleChatMessageProperties", category_Data,
        &TextChatMessage::getBubbleChatMessageProperties,
        &TextChatMessage::setBubbleChatMessageProperties,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::RefPropDescriptor<TextChatMessage, ChatWindowMessageProperties>
    propChatWindowMessageProperties("ChatWindowMessageProperties", category_Data,
        &TextChatMessage::getChatWindowMessageProperties,
        &TextChatMessage::setChatWindowMessageProperties,
        Reflection::PropertyDescriptor::SCRIPTING);

static Reflection::BoundFuncDesc<TextChannel, shared_ptr<Instance>(std::string, std::string)>
    funcDisplaySystemMessage(&TextChannel::displaySystemMessage,
        "DisplaySystemMessage", "systemMessage", "metadata", std::string(),
        Security::None);
static Reflection::BoundYieldFuncDesc<TextChannel,
    shared_ptr<Instance>(std::string, std::string)> funcSendAsync(
        &TextChannel::sendAsync, "SendAsync", "message", "metadata",
        std::string(), Security::None);
static Reflection::EventDesc<TextChannel, void(shared_ptr<Instance>)>
    eventMessageReceived(&TextChannel::messageReceivedSignal, "MessageReceived",
        "incomingMessage", Security::None);
static Reflection::BoundCallbackDesc<shared_ptr<const Reflection::Tuple>(
    shared_ptr<Instance>)> callbackOnIncomingMessage(
        "OnIncomingMessage", &TextChannel::onIncomingMessage, "message",
        Security::None);
REFLECTION_END();

namespace Reflection {

template<>
EnumDesc<TextChatMessage::Status>::EnumDesc()
    : EnumDescriptor("TextChatMessageStatus")
{
    addPair(TextChatMessage::Unknown, "Unknown");
    addPair(TextChatMessage::Success, "Success");
    addPair(TextChatMessage::Sending, "Sending");
    addPair(TextChatMessage::TextFilterFailed, "TextFilterFailed");
    addPair(TextChatMessage::Floodchecked, "Floodchecked");
    addPair(TextChatMessage::InvalidPrivacySettings, "InvalidPrivacySettings");
    addPair(TextChatMessage::InvalidTextChannelPermissions,
        "InvalidTextChannelPermissions");
    addPair(TextChatMessage::MessageTooLong, "MessageTooLong");
    addPair(TextChatMessage::ModerationTimeout, "ModerationTimeout");
}

template<>
TextChatMessage::Status& Variant::convert<TextChatMessage::Status>()
{
    return genericConvert<TextChatMessage::Status>();
}

} // namespace Reflection

template<>
bool StringConverter<TextChatMessage::Status>::convertToValue(
    const std::string& text, TextChatMessage::Status& value)
{
    return Reflection::EnumDesc<TextChatMessage::Status>::singleton()
        .convertToValue(text.c_str(), value);
}

TextChatMessage::TextChatMessage(std::string messageId, std::string text,
    std::string metadata, TextChannel* channel, Status status,
    DateTime timestamp, TextSource* source, std::string prefix)
    : messageId(std::move(messageId))
    , metadata(std::move(metadata))
    , status(status)
    , text(std::move(text))
    , textChannel(channel)
    , textSource(source)
    , timestamp(timestamp)
{
    prefixText = std::move(prefix);
    setName(sTextChatMessage);
    setRobloxLocked(true);
}

TextSource::TextSource(int userId, bool canSend)
    : userId(userId)
    , canSend(canSend)
{
    setName(sTextSource);
    setRobloxLocked(true);
}

TextChatMessageProperties::TextChatMessageProperties()
{
    setName(sTextChatMessageProperties);
}

BubbleChatMessageProperties::BubbleChatMessageProperties()
    : backgroundColor3(Color3::white())
    , backgroundTransparency(0.1)
    , fontFace(Font("rbxasset://fonts/families/BuilderSans.json",
          FONT_WEIGHT_MEDIUM))
    , tailVisible(true)
    , textColor3(Color3(57.0f / 255.0f, 59.0f / 255.0f, 61.0f / 255.0f))
    , textSize(16)
{
    setName(sBubbleChatMessageProperties);
}

ChatWindowMessageProperties::ChatWindowMessageProperties()
    : fontFace(Font("rbxasset://fonts/families/BuilderSans.json",
          FONT_WEIGHT_MEDIUM))
    , textColor3(Color3::white())
    , textSize(18)
    , textStrokeColor3(Color3::black())
    , textStrokeTransparency(0.5)
{
    setName(sChatWindowMessageProperties);
    setRobloxLocked(true);
}

void BubbleChatMessageProperties::setBackgroundColor3(Color3 value)
{ backgroundColor3 = value; raisePropertyChanged(propBubbleBackgroundColor3); }
void BubbleChatMessageProperties::setBackgroundTransparency(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    backgroundTransparency = value;
    raisePropertyChanged(propBubbleBackgroundTransparency);
}
void BubbleChatMessageProperties::setFontFace(Font value)
{ fontFace = std::move(value); raisePropertyChanged(propBubbleFontFace); }
void BubbleChatMessageProperties::setTailVisible(bool value)
{ tailVisible = value; raisePropertyChanged(propBubbleTailVisible); }
void BubbleChatMessageProperties::setTextColor3(Color3 value)
{ textColor3 = value; raisePropertyChanged(propBubbleTextColor3); }
void BubbleChatMessageProperties::setTextSize(long long value)
{
    value = std::clamp(value, 1LL, 100LL);
    textSize = value;
    raisePropertyChanged(propBubbleTextSize);
}

void ChatWindowMessageProperties::setFontFace(Font value)
{ fontFace = std::move(value); raisePropertyChanged(propWindowMessageFontFace); }
void ChatWindowMessageProperties::setPrefixTextProperties(
    ChatWindowMessageProperties* value)
{
    shared_ptr<ChatWindowMessageProperties> next = value
        ? shared_dynamic_cast<ChatWindowMessageProperties>(shared_from(value))
        : shared_ptr<ChatWindowMessageProperties>();
    if (prefixTextProperties == next)
        return;
    prefixTextProperties = next;
    raisePropertyChanged(propPrefixTextProperties);
}
void ChatWindowMessageProperties::setTextColor3(Color3 value)
{ textColor3 = value; raisePropertyChanged(propWindowMessageTextColor3); }
void ChatWindowMessageProperties::setTextSize(int value)
{
    value = std::clamp(value, 1, 100);
    textSize = value;
    raisePropertyChanged(propWindowMessageTextSize);
}
void ChatWindowMessageProperties::setTextStrokeColor3(Color3 value)
{
    textStrokeColor3 = value;
    raisePropertyChanged(propWindowMessageTextStrokeColor3);
}
void ChatWindowMessageProperties::setTextStrokeTransparency(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    textStrokeTransparency = value;
    raisePropertyChanged(propWindowMessageTextStrokeTransparency);
}

void TextChatMessageProperties::setPrefixText(const std::string& value)
{
    prefixText = value;
    prefixTextSet = true;
    raisePropertyChanged(propOverridePrefixText);
}

void TextChatMessageProperties::setText(const std::string& value)
{
    text = value;
    textSet = true;
    raisePropertyChanged(propOverrideText);
}

void TextChatMessageProperties::setTranslation(const std::string& value)
{
    translation = value;
    translationSet = true;
    raisePropertyChanged(propOverrideTranslation);
}

void TextChatMessage::applyProperties(const TextChatMessageProperties& properties)
{
    if (properties.hasPrefixText() && !properties.getPrefixText().empty())
        prefixText = properties.getPrefixText();
    if (properties.hasText() && !properties.getText().empty())
        text = properties.getText();
    if (properties.hasTranslation() && !properties.getTranslation().empty())
        translation = properties.getTranslation();
}

void TextChatMessage::setBubbleChatMessageProperties(
    BubbleChatMessageProperties* value)
{
    shared_ptr<BubbleChatMessageProperties> next = value
        ? shared_dynamic_cast<BubbleChatMessageProperties>(shared_from(value))
        : shared_ptr<BubbleChatMessageProperties>();
    if (bubbleChatMessageProperties == next)
        return;
    bubbleChatMessageProperties = next;
    raisePropertyChanged(propBubbleChatMessageProperties);
}

void TextChatMessage::setChatWindowMessageProperties(
    ChatWindowMessageProperties* value)
{
    shared_ptr<ChatWindowMessageProperties> next = value
        ? shared_dynamic_cast<ChatWindowMessageProperties>(shared_from(value))
        : shared_ptr<ChatWindowMessageProperties>();
    if (chatWindowMessageProperties == next)
        return;
    chatWindowMessageProperties = next;
    raisePropertyChanged(propChatWindowMessageProperties);
}

TextChannel::TextChannel()
{
    setName(sTextChannel);
}

shared_ptr<TextSource> TextChannel::findOrCreateTextSource(int userId)
{
    boost::shared_ptr<const Instances> children(getChildren().read());
    if (children)
        for (const shared_ptr<Instance>& child : *children)
            if (TextSource* source = Instance::fastDynamicCast<TextSource>(child.get()))
                if (source->getUserId() == userId)
                    return shared_dynamic_cast<TextSource>(child);

    shared_ptr<TextSource> source(new TextSource(userId, true));
    source->setParent(this);
    return source;
}

void TextChannel::sendAsync(std::string message, std::string metadata,
    boost::function<void(shared_ptr<Instance>)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    Network::Players* players = ServiceProvider::find<Network::Players>(this);
    Network::Player* localPlayer = players
        ? Instance::fastDynamicCast<Network::Player>(
              players->getLocalPlayerDangerous())
        : nullptr;
    TextChatService* service = ServiceProvider::find<TextChatService>(this);
    if (!service || !localPlayer)
    {
        errorFunction("TextChannel:SendAsync requires a local player and TextChatService.");
        return;
    }

    if (message.find_first_not_of(" \t\r\n") == std::string::npos)
    {
        errorFunction("TextChannel:SendAsync message must contain non-whitespace text.");
        return;
    }

    static std::atomic<unsigned long long> sequence{0};
    const unsigned long long id =
        sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    std::ostringstream messageId;
    messageId << "local-" << milliseconds << '-' << id;

    shared_ptr<TextSource> source = findOrCreateTextSource(localPlayer->getUserID());
    source->setName(localPlayer->getName());
    const std::string prefix = localPlayer->getDisplayName() + ":";

    shared_ptr<TextChatMessage> sending(new TextChatMessage(
        messageId.str(), message, metadata, this, TextChatMessage::Sending,
        DateTime(milliseconds), source.get(), prefix));
    service->sendingMessageSignal(sending);

    shared_ptr<TextChatMessage> delivered(new TextChatMessage(
        messageId.str(), std::move(message), std::move(metadata), this,
        TextChatMessage::Success, DateTime(milliseconds), source.get(), prefix));
    service->applyIncomingMessageCallbacks(delivered, this);
    service->applyChatWindowAddedCallback(delivered);
    service->deliverMessage(delivered);
    messageReceivedSignal(delivered);
    resumeFunction(delivered);
}

shared_ptr<Instance> TextChannel::displaySystemMessage(
    std::string systemMessage, std::string metadata)
{
    static std::atomic<unsigned long long> sequence{0};
    const unsigned long long id = sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    std::ostringstream messageId;
    messageId << "system-" << milliseconds << '-' << id;

    shared_ptr<TextChatMessage> message(new TextChatMessage(
        messageId.str(), std::move(systemMessage), std::move(metadata), this,
        TextChatMessage::Success, DateTime(milliseconds)));
    if (TextChatService* service = ServiceProvider::find<TextChatService>(this)) {
        service->applyIncomingMessageCallbacks(message, this);
        service->applyChatWindowAddedCallback(message);
        service->deliverMessage(message);
    } else if (onIncomingMessage) {
        const shared_ptr<const Reflection::Tuple> overrides = onIncomingMessage(message);
        if (overrides && !overrides->values.empty())
            if (TextChatMessageProperties* properties =
                    Instance::fastDynamicCast<TextChatMessageProperties>(
                        overrides->at(0).cast<shared_ptr<Instance>>().get()))
                message->applyProperties(*properties);
    }
    messageReceivedSignal(message);
    return message;
}

} // namespace RBX
