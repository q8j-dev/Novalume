//
//  NotificationService.h
//  Copyright ROBLOX Corp 2014
//
//  Created by Ganesh Agrawal on 5/15/14.
//
//

#pragma once

#include "v8tree/Instance.h"
#include "v8tree/Service.h"
#include "v8datamodel/InteractionEnums.h"

#include <set>

namespace RBX
{
	extern const char* const sNotificationService;
	class NotificationService
    : public DescribedNonCreatable<NotificationService, Instance, sNotificationService, Reflection::ClassDescriptor::INTERNAL>
    , public Service
	{
	private:
		typedef DescribedNonCreatable<NotificationService, Instance, sNotificationService, Reflection::ClassDescriptor::INTERNAL> Super;
        
		bool canUseService();
		bool connected;
		bool luaChatEnabled;
		bool luaGameDetailsEnabled;
		std::string selectedTheme;
		std::set<std::string> rccNamespaces;
        
	public:
		NotificationService();
        
        rbx::signal<void(long long, int, std::string, int)> scheduleNotificationSignal;
        rbx::signal<void(long long, int)> cancelNotificationSignal;
        rbx::signal<void(long long)> cancelAllNotificationSignal;
        rbx::signal<void(long long, boost::function<void(shared_ptr<const Reflection::ValueArray>)>, boost::function<void(std::string)>)> getScheduledNotificationsSignal;
        rbx::signal<void(Enums::AppShellActionType, bool)> appShellActionSignal;
        rbx::signal<void(Enums::AppShellFeature)> appShellFeatureSignal;
        rbx::signal<void(std::string)> subscribeToRccNamespaceSignal;

        rbx::signal<void(std::string, Enums::ConnectionState, std::string, shared_ptr<const Reflection::ValueTable>)> rccConnectionChangedSignal;
        rbx::signal<void(shared_ptr<const Reflection::ValueTable>, long long)> rccEventReceivedSignal;
        rbx::signal<void(std::string, Enums::ConnectionState, std::string)> roblox17sConnectionChangedSignal;
        rbx::signal<void(shared_ptr<const Reflection::ValueTable>)> roblox17sEventReceivedSignal;
        rbx::signal<void(std::string, Enums::ConnectionState, std::string, std::string)> robloxConnectionChangedSignal;
        rbx::signal<void(shared_ptr<const Reflection::ValueTable>)> robloxEventReceivedSignal;

        bool getIsConnected() const { return connected; }
        bool getIsLuaChatEnabled() const { return luaChatEnabled; }
        bool getIsLuaGameDetailsEnabled() const { return luaGameDetailsEnabled; }
        std::string getSelectedTheme() const { return selectedTheme; }
        void setSelectedTheme(std::string value);

        void actionEnabled(Enums::AppShellActionType actionType);
        void actionTaken(Enums::AppShellActionType actionType);
        void switchedToAppShellFeature(Enums::AppShellFeature feature);
        void subscribeToRccEventNamespace(std::string eventNamespace);
        void scheduleNotification(long long userId, int alertId, std::string alerMsg, int minutesToFire);
        void cancelNotification(long long userId, int alertId);
        void cancelAllNotification(long long userId);
        void getScheduledNotifications(long long userId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)>	errorFunction);
	};
}
