//
//  NotificationService.cpp
//  App
//
//  Created by Ganesh Agrawal on 5/15/14.
//
//

#include "v8datamodel/NotificationService.h"
#include "v8datamodel/UserInputService.h"
#include "network/Players.h"



FASTFLAGVARIABLE(NotificationServiceEnabledForEveryone, false);

namespace RBX
{
	const char* const sNotificationService = "NotificationService";
    
    
    REFLECTION_BEGIN();
    static Reflection::PropDescriptor<NotificationService, bool> propIsConnected(
        "IsConnected", category_Data, &NotificationService::getIsConnected, NULL,
        Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
    static Reflection::PropDescriptor<NotificationService, bool> propIsLuaChatEnabled(
        "IsLuaChatEnabled", category_Behavior, &NotificationService::getIsLuaChatEnabled, NULL,
        Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
    static Reflection::PropDescriptor<NotificationService, bool> propIsLuaGameDetailsEnabled(
        "IsLuaGameDetailsEnabled", category_Behavior, &NotificationService::getIsLuaGameDetailsEnabled, NULL,
        Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
    static Reflection::PropDescriptor<NotificationService, std::string> propSelectedTheme(
        "SelectedTheme", category_Behavior, &NotificationService::getSelectedTheme,
        &NotificationService::setSelectedTheme, Reflection::PropertyDescriptor::STANDARD,
        Security::RobloxScript);

    static Reflection::BoundFuncDesc<NotificationService, void(Enums::AppShellActionType)>
    funcActionEnabled(&NotificationService::actionEnabled, "ActionEnabled", "actionType", Security::RobloxScript);
    static Reflection::BoundFuncDesc<NotificationService, void(Enums::AppShellActionType)>
    funcActionTaken(&NotificationService::actionTaken, "ActionTaken", "actionType", Security::RobloxScript);
    static Reflection::BoundFuncDesc<NotificationService, void(Enums::AppShellFeature)>
    funcSwitchedToAppShellFeature(&NotificationService::switchedToAppShellFeature, "SwitchedToAppShellFeature", "appShellFeature", Security::RobloxScript);
    static Reflection::BoundFuncDesc<NotificationService, void(std::string)>
    funcSubscribeToRccEventNamespace(&NotificationService::subscribeToRccEventNamespace, "SubscribeToRccEventNamespace", "eventNamespace", Security::RobloxScript);

    static Reflection::BoundFuncDesc<NotificationService, void(long long, int, std::string, int) >
    func_ScheduleNotification( &NotificationService::scheduleNotification, "ScheduleNotification", "userId", "alertId", "alertMsg", "minutesToFire", Security::LocalUser);
    
    static Reflection::BoundFuncDesc<NotificationService, void(long long, int) >
    func_CancelNotification( &NotificationService::cancelNotification, "CancelNotification", "userId", "alertId", Security::LocalUser);

    static Reflection::BoundFuncDesc<NotificationService, void(long long) >
    func_CancelAllNotification( &NotificationService::cancelAllNotification, "CancelAllNotification", "userId", Security::LocalUser);
    
    static Reflection::BoundYieldFuncDesc<NotificationService, shared_ptr<const Reflection::ValueArray>(long long)> func_GetScheduledNotifications( &NotificationService::getScheduledNotifications, "GetScheduledNotifications", "userId", Security::LocalUser);

    static Reflection::EventDesc<NotificationService, void(std::string, Enums::ConnectionState, std::string, shared_ptr<const Reflection::ValueTable>)> eventRccConnectionChanged(&NotificationService::rccConnectionChangedSignal, "RccConnectionChanged", "connectionName", "connectionState", "rccSequenceNumber", "userIdToNamespaceSequenceNumbers", Security::RobloxScript);
    static Reflection::EventDesc<NotificationService, void(shared_ptr<const Reflection::ValueTable>, long long)> eventRccEventReceived(&NotificationService::rccEventReceivedSignal, "RccEventReceived", "eventData", "userId", Security::RobloxScript);
    static Reflection::EventDesc<NotificationService, void(std::string, Enums::ConnectionState, std::string)> eventRoblox17sConnectionChanged(&NotificationService::roblox17sConnectionChangedSignal, "Roblox17sConnectionChanged", "connectionName", "connectionState", "namespaceSequenceNumbers", Security::None);
    static Reflection::EventDesc<NotificationService, void(shared_ptr<const Reflection::ValueTable>)> eventRoblox17sEventReceived(&NotificationService::roblox17sEventReceivedSignal, "Roblox17sEventReceived", "eventData", Security::None);
    static Reflection::EventDesc<NotificationService, void(std::string, Enums::ConnectionState, std::string, std::string)> eventRobloxConnectionChanged(&NotificationService::robloxConnectionChangedSignal, "RobloxConnectionChanged", "connectionName", "connectionState", "sequenceNumber", "namespaceSequenceNumbers", Security::RobloxScript);
    static Reflection::EventDesc<NotificationService, void(shared_ptr<const Reflection::ValueTable>)> eventRobloxEventReceived(&NotificationService::robloxEventReceivedSignal, "RobloxEventReceived", "eventData", Security::RobloxScript);
    REFLECTION_END();
    
   
    NotificationService::NotificationService()
        : connected(false)
        , luaChatEnabled(true)
        , luaGameDetailsEnabled(true)
        , selectedTheme("Default")
    {
        setName(sNotificationService);
    }
    
    bool NotificationService::canUseService()
    {
        bool retVal = true;
        
        if (!FFlag::NotificationServiceEnabledForEveryone)
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING, "Sorry, NotificationService is currently off.");
            retVal = false;
        }
        
        if (RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(this))
        {
            if (!inputService->getTouchEnabled())
            {
                StandardOut::singleton()->printf(MESSAGE_WARNING, "Sorry, NotificationService only works on touch devices currently.");
                retVal = false;
            }
        }
        
        if (!Network::Players::frontendProcessing(this))
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING, "NotificationService:ScheduleNotification must be called from a local script!");
            retVal = false;
        }
        return retVal;
    }
    
    void NotificationService::setSelectedTheme(std::string value)
    {
        if (selectedTheme != value) { selectedTheme = value; raisePropertyChanged(propSelectedTheme); }
    }

    void NotificationService::actionEnabled(Enums::AppShellActionType actionType)
    { appShellActionSignal(actionType, true); }
    void NotificationService::actionTaken(Enums::AppShellActionType actionType)
    { appShellActionSignal(actionType, false); }
    void NotificationService::switchedToAppShellFeature(Enums::AppShellFeature feature)
    { appShellFeatureSignal(feature); }
    void NotificationService::subscribeToRccEventNamespace(std::string eventNamespace)
    {
        if (!eventNamespace.empty() && rccNamespaces.insert(eventNamespace).second)
            subscribeToRccNamespaceSignal(eventNamespace);
    }

    void NotificationService::scheduleNotification(long long userId, int alertId, std::string alerMsg, int minutesToFire)
    {
        if (canUseService())
            scheduleNotificationSignal(userId, alertId, alerMsg, minutesToFire);
    }
    
    void NotificationService::cancelNotification(long long userId, int alertId)
    {
        if (canUseService())
            cancelNotificationSignal(userId, alertId);
    }
    
    void NotificationService::cancelAllNotification(long long userId)
    {
        if (canUseService())
            cancelAllNotificationSignal(userId);
    }
    

    
    void NotificationService::getScheduledNotifications(long long userId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)>	errorFunction)
    {
        if (canUseService())
            getScheduledNotificationsSignal(userId, resumeFunction,	errorFunction);
        else
            errorFunction("Notification Service Not Available");
    }

}// namespace RBX
