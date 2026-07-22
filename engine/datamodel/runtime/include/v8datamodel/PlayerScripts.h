/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved  */

#pragma once

#include "v8tree/Instance.h"
#include "GfxBase/IAdornable.h"

#include "v8datamodel/InputObject.h"
#include "v8datamodel/UserInputService.h"
#include "Script/IScriptFilter.h"
#include "Gui/GuiEvent.h"
#include "v8datamodel/GameBasicSettings.h"

#include <vector>

namespace RBX {
	class StarterPlayerScripts;

	extern const char *const sPlayerScripts;
	class PlayerScripts
		: public DescribedCreatable<PlayerScripts, Instance, sPlayerScripts, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public IScriptFilter
	{
	private:
		typedef DescribedCreatable<PlayerScripts, Instance, sPlayerScripts, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

	public:
		PlayerScripts();

		void CopyStarterPlayerScripts(StarterPlayerScripts* scripts);

		void registerTouchCameraMovementMode(GameBasicSettings::TouchCameraMovementMode mode);
		void registerComputerCameraMovementMode(GameBasicSettings::ComputerCameraMovementMode mode);
		void registerTouchMovementMode(GameBasicSettings::TouchMovementMode mode);
		void registerComputerMovementMode(GameBasicSettings::ComputerMovementMode mode);

		shared_ptr<const Reflection::ValueArray> getRegisteredTouchCameraMovementModes();
		shared_ptr<const Reflection::ValueArray> getRegisteredComputerCameraMovementModes();
		shared_ptr<const Reflection::ValueArray> getRegisteredTouchMovementModes();
		shared_ptr<const Reflection::ValueArray> getRegisteredComputerMovementModes();

		void clearTouchCameraMovementModes();
		void clearComputerCameraMovementModes();
		void clearTouchMovementModes();
		void clearComputerMovementModes();

		rbx::signal<void()> touchCameraMovementModeRegisteredSignal;
		rbx::signal<void()> computerCameraMovementModeRegisteredSignal;
		rbx::signal<void()> touchMovementModeRegisteredSignal;
		rbx::signal<void()> computerMovementModeRegisteredSignal;

	protected:
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ bool askForbidParent(const Instance* instance) const;
		/*override*/ bool askAddChild(const Instance* instance) const;
		/*override*/ bool askForbidChild(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IScriptFilter
		/*override*/ bool scriptShouldRun(BaseScript* script);

	private:
		std::vector<GameBasicSettings::TouchCameraMovementMode> touchCameraMovementModes;
		std::vector<GameBasicSettings::ComputerCameraMovementMode> computerCameraMovementModes;
		std::vector<GameBasicSettings::TouchMovementMode> touchMovementModes;
		std::vector<GameBasicSettings::ComputerMovementMode> computerMovementModes;

	};


	extern const char *const sStarterPlayerScripts;
	class StarterPlayerScripts
		: public DescribedCreatable<StarterPlayerScripts, Instance, sStarterPlayerScripts, Reflection::ClassDescriptor::PERSISTENT>
	{
	private:
		typedef DescribedCreatable<StarterPlayerScripts, Instance, sStarterPlayerScripts, Reflection::ClassDescriptor::PERSISTENT> Super;

		void InitializeDefaultScripts();
		void InitializeDefaultScriptsRunService(RunTransition transition);
		rbx::signals::scoped_connection initializeDefaultScriptsConnection;

		rbx::signal<void()> defaultScriptsLoadedSignal;	

		bool defaultScriptsLoadRequested;
		bool defaultScriptsLoaded;
		bool defaultScriptsRequested;

	public:
		StarterPlayerScripts();
		bool areDefaultScriptsLoaded() 	{ return defaultScriptsLoaded; } 
		bool checkDefaultScriptsLoaded();

		void requestDefaultScripts();
		void requestDefaultScriptsServer(shared_ptr<Instance> player);
		void defaultScriptsSend(weak_ptr<RBX::Network::Player> p);
		void defaultScriptsReceived(int confirm);

		rbx::remote_signal<void(shared_ptr<Instance>)> requestDefaultScriptsSignal;
		rbx::remote_signal<void(int)> confirmDefaultScriptsSignal;

	protected:
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ bool askForbidParent(const Instance* instance) const;
		/*override*/ bool askAddChild(const Instance* instance) const;
		/*override*/ bool askForbidChild(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	};

	extern const char *const sStarterCharacterScripts;
	class StarterCharacterScripts
		: public DescribedCreatable<StarterCharacterScripts, StarterPlayerScripts, sStarterCharacterScripts, Reflection::ClassDescriptor::PERSISTENT>
	{
	private:
		typedef DescribedCreatable<StarterCharacterScripts, StarterPlayerScripts, sStarterCharacterScripts, Reflection::ClassDescriptor::PERSISTENT> Super;
	public:
		StarterCharacterScripts();


		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ bool askAddChild(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	};



}
