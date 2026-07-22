#pragma once

#include "v8tree/Service.h"

namespace RBX {

class Folder;
class TextChannel;
class ChatInputBarConfiguration;
class ChatWindowConfiguration;

extern const char* const sTextChatService;

class TextChatService
    : public DescribedNonCreatable<TextChatService, Instance, sTextChatService>
    , public Service
{
public:
    using Super = DescribedNonCreatable<TextChatService, Instance, sTextChatService>;

    enum ChatVersion
    {
        LegacyChatService = 0,
        TextChatServiceVersion = 1
    };

    TextChatService();

    rbx::signal<void(shared_ptr<Instance>)> messageReceivedSignal;
    rbx::signal<void(shared_ptr<Instance>)> sendingMessageSignal;
    void deliverMessage(const shared_ptr<Instance>& textChatMessage);
    void applyIncomingMessageCallbacks(const shared_ptr<Instance>& textChatMessage,
        TextChannel* channel);
    void applyBubbleAddedCallback(const shared_ptr<Instance>& textChatMessage,
        const shared_ptr<Instance>& adornee);
    void applyChatWindowAddedCallback(const shared_ptr<Instance>& textChatMessage);

    boost::function<shared_ptr<const Reflection::Tuple>(shared_ptr<Instance>)>
        onIncomingMessage;
    boost::function<shared_ptr<const Reflection::Tuple>(shared_ptr<Instance>)>
        onChatWindowAdded;
    boost::function<shared_ptr<const Reflection::Tuple>(shared_ptr<Instance>,
        shared_ptr<Instance>)> onBubbleAdded;

    ChatVersion getChatVersion() const { return chatVersion; }
    void setChatVersion(ChatVersion value);
    bool getCreateDefaultCommands() const { return createDefaultCommands; }
    void setCreateDefaultCommands(bool value);
    bool getCreateDefaultTextChannels() const { return createDefaultTextChannels; }
    void setCreateDefaultTextChannels(bool value);
    bool getChatTranslationEnabled() const { return chatTranslationEnabled; }
    void setChatTranslationEnabled(bool value);
    bool getIsProtectedChatEnabled() const;
    void canUserChatAsync(int userId, boost::function<void(bool)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

protected:
    void onServiceProvider(ServiceProvider* oldProvider,
        ServiceProvider* newProvider) override;

private:
    void updateDefaultTextChannels();

    ChatVersion chatVersion;
    bool createDefaultCommands;
    bool createDefaultTextChannels;
    bool chatTranslationEnabled;
    shared_ptr<Folder> defaultTextChannels;
    shared_ptr<ChatInputBarConfiguration> chatInputBarConfiguration;
    shared_ptr<ChatWindowConfiguration> chatWindowConfiguration;
};

} // namespace RBX
