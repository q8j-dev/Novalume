#pragma once

#include "util/DateTime.h"
#include "util/Font.h"
#include "v8tree/Instance.h"

#include <string>

namespace RBX {

extern const char* const sTextChannel;
extern const char* const sTextChatMessage;
extern const char* const sTextChatMessageProperties;
extern const char* const sBubbleChatMessageProperties;
extern const char* const sChatWindowMessageProperties;
extern const char* const sTextSource;

class TextChannel;

class TextSource
    : public DescribedNonCreatable<TextSource, Instance, sTextSource>
{
public:
    TextSource(int userId, bool canSend);

    int getUserId() const { return userId; }
    bool getCanSend() const { return canSend; }

private:
    int userId;
    bool canSend;
};

class TextChatMessageProperties
    : public DescribedCreatable<TextChatMessageProperties, Instance,
          sTextChatMessageProperties>
{
public:
    TextChatMessageProperties();

    const std::string& getPrefixText() const { return prefixText; }
    bool hasPrefixText() const { return prefixTextSet; }
    void setPrefixText(const std::string& value);
    const std::string& getText() const { return text; }
    bool hasText() const { return textSet; }
    void setText(const std::string& value);
    const std::string& getTranslation() const { return translation; }
    bool hasTranslation() const { return translationSet; }
    void setTranslation(const std::string& value);

private:
    std::string prefixText;
    std::string text;
    std::string translation;
    bool prefixTextSet = false;
    bool textSet = false;
    bool translationSet = false;
};

class BubbleChatMessageProperties
    : public DescribedCreatable<BubbleChatMessageProperties,
          TextChatMessageProperties, sBubbleChatMessageProperties>
{
public:
    BubbleChatMessageProperties();

    Color3 getBackgroundColor3() const { return backgroundColor3; }
    void setBackgroundColor3(Color3 value);
    double getBackgroundTransparency() const { return backgroundTransparency; }
    void setBackgroundTransparency(double value);
    Font getFontFace() const { return fontFace; }
    void setFontFace(Font value);
    bool getTailVisible() const { return tailVisible; }
    void setTailVisible(bool value);
    Color3 getTextColor3() const { return textColor3; }
    void setTextColor3(Color3 value);
    long long getTextSize() const { return textSize; }
    void setTextSize(long long value);

private:
    Color3 backgroundColor3;
    double backgroundTransparency;
    Font fontFace;
    bool tailVisible;
    Color3 textColor3;
    long long textSize;
};

class ChatWindowMessageProperties
    : public DescribedNonCreatable<ChatWindowMessageProperties,
          TextChatMessageProperties, sChatWindowMessageProperties>
{
public:
    ChatWindowMessageProperties();

    Font getFontFace() const { return fontFace; }
    void setFontFace(Font value);
    ChatWindowMessageProperties* getPrefixTextProperties() const
    { return prefixTextProperties.get(); }
    void setPrefixTextProperties(ChatWindowMessageProperties* value);
    Color3 getTextColor3() const { return textColor3; }
    void setTextColor3(Color3 value);
    int getTextSize() const { return textSize; }
    void setTextSize(int value);
    Color3 getTextStrokeColor3() const { return textStrokeColor3; }
    void setTextStrokeColor3(Color3 value);
    double getTextStrokeTransparency() const { return textStrokeTransparency; }
    void setTextStrokeTransparency(double value);

private:
    Font fontFace;
    shared_ptr<ChatWindowMessageProperties> prefixTextProperties;
    Color3 textColor3;
    int textSize;
    Color3 textStrokeColor3;
    double textStrokeTransparency;
};

class TextChatMessage
    : public DescribedNonCreatable<TextChatMessage, Instance, sTextChatMessage>
{
public:
    enum Status
    {
        Unknown = 1,
        Success = 2,
        Sending = 3,
        TextFilterFailed = 4,
        Floodchecked = 5,
        InvalidPrivacySettings = 6,
        InvalidTextChannelPermissions = 7,
        MessageTooLong = 8,
        ModerationTimeout = 9
    };

    TextChatMessage(std::string messageId, std::string text,
        std::string metadata, TextChannel* channel, Status status,
        DateTime timestamp, TextSource* source = nullptr,
        std::string prefixText = std::string());

    const std::string& getMessageId() const { return messageId; }
    const std::string& getMetadata() const { return metadata; }
    const std::string& getPrefixText() const { return prefixText; }
    Status getStatus() const { return status; }
    const std::string& getText() const { return text; }
    TextChannel* getTextChannel() const { return textChannel; }
    Instance* getTextSource() const { return textSource; }
    DateTime getTimestamp() const { return timestamp; }
    const std::string& getTranslation() const { return translation; }
    BubbleChatMessageProperties* getBubbleChatMessageProperties() const
    { return bubbleChatMessageProperties.get(); }
    void setBubbleChatMessageProperties(BubbleChatMessageProperties* value);
    ChatWindowMessageProperties* getChatWindowMessageProperties() const
    { return chatWindowMessageProperties.get(); }
    void setChatWindowMessageProperties(ChatWindowMessageProperties* value);
    void applyProperties(const TextChatMessageProperties& properties);

private:
    std::string messageId;
    std::string metadata;
    std::string prefixText;
    Status status;
    std::string text;
    TextChannel* textChannel;
    Instance* textSource;
    DateTime timestamp;
    std::string translation;
    shared_ptr<BubbleChatMessageProperties> bubbleChatMessageProperties;
    shared_ptr<ChatWindowMessageProperties> chatWindowMessageProperties;
};

class TextChannel
    : public DescribedCreatable<TextChannel, Instance, sTextChannel>
{
public:
    TextChannel();

    shared_ptr<Instance> displaySystemMessage(
        std::string systemMessage, std::string metadata);
    void sendAsync(std::string message, std::string metadata,
        boost::function<void(shared_ptr<Instance>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

    rbx::signal<void(shared_ptr<Instance>)> messageReceivedSignal;
    boost::function<shared_ptr<const Reflection::Tuple>(shared_ptr<Instance>)>
        onIncomingMessage;

private:
    shared_ptr<TextSource> findOrCreateTextSource(int userId);
};

} // namespace RBX
