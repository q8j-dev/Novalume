#pragma once

#include "v8tree/Service.h"
#include "v8tree/Instance.h"

namespace RBX {
	namespace Network { class Player; }
	class PartInstance;

	extern const char *const sChatService ;

	class ChatService 
		: public DescribedCreatable<ChatService, Instance, sChatService, Reflection::ClassDescriptor::INTERNAL>
		, public Service
	{
	private:
		void gotFilteredStringSuccess(std::string response, Network::Player* player, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void gotFilterStringError(std::string error, boost::function<void(std::string)> errorFunction);
	public:
		enum ChatColor
		{
			CHAT_BLUE,
			CHAT_GREEN,
			CHAT_RED
		};

		ChatService();
		bool getBubbleChatEnabled() const { return bubbleChatEnabled; }
		void setBubbleChatEnabled(bool value);
		bool getLoadDefaultChat() const { return loadDefaultChat; }
		bool getIsAutoMigrated() const { return isAutoMigrated; }
		void setIsAutoMigrated(bool value);
		void setBubbleChatSettings(Reflection::Variant settings);

		void chat(shared_ptr<Instance> instance, std::string message, ChatService::ChatColor chatColor);
		void timeoutChatAttempt(bool isPermanentTimeout, long long endTime);

		void filterStringForPlayer(std::string stringToFilter, shared_ptr<Instance> playerToFilterFor, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void canUserChatAsync(int userId, boost::function<void(bool)> resumeFunction,
			boost::function<void(std::string)> errorFunction);

		rbx::remote_signal<void(shared_ptr<Instance>, std::string, ChatService::ChatColor)> chattedSignal;
		rbx::remote_signal<void(bool, long long)> timeoutChatAttemptSignal;
		rbx::signal<void(Reflection::Variant)> bubbleChatSettingsChangedSignal;
	private:
		bool bubbleChatEnabled;
		bool loadDefaultChat;
		bool isAutoMigrated;
	};
}
