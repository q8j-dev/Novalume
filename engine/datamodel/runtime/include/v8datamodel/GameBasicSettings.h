#pragma once

#include "v8datamodel/GlobalSettings.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/InteractionEnums.h"
#include "rbx/ui/PreferredTextSize.h"

namespace RBX
{
	extern const char *const sGameBasicSettings;
	class GameBasicSettings
		: public GlobalBasicSettingsItem<GameBasicSettings, sGameBasicSettings>
	{
		typedef GlobalBasicSettingsItem<GameBasicSettings, sGameBasicSettings> Super;
	public:
		enum ControlMode {CONTROL_CLASSIC = 0, CONTROL_MOUSELOCK = 1, CONTROL_HYBRID = 2, CONTROL_CAMLOCK = 3, CONTROL_MOUSEPAN = 4};
		enum RenderQualitySetting {QUALITY_AUTO = 0, QUALITY_1 = 1, QUALITY_2 = 2, QUALITY_3 = 3, QUALITY_4 = 4, QUALITY_5 = 5, QUALITY_6 = 6, QUALITY_7 = 7, QUALITY_8 = 8, QUALITY_9 = 9, QUALITY_10 = 10}; 
		enum CameraMode {CAMERA_MODE_DEFAULT = 0, CAMERA_MODE_CLASSIC = 1, CAMERA_MODE_FOLLOW = 2};
		enum TouchCameraMovementMode {
			TOUCH_CAMERA_MOVEMENT_MODE_DEFAULT = 0, 
			TOUCH_CAMERA_MOVEMENT_MODE_CLASSIC = 1, 
			TOUCH_CAMERA_MOVEMENT_MODE_FOLLOW = 2,
			TOUCH_CAMERA_MOVEMENT_MODE_ORBITAL = 3 };
		enum ComputerCameraMovementMode {
			COMPUTER_CAMERA_MOVEMENT_MODE_DEFAULT = 0, 
			COMPUTER_CAMERA_MOVEMENT_MODE_CLASSIC = 1, 
			COMPUTER_CAMERA_MOVEMENT_MODE_FOLLOW = 2,
			COMPUTER_CAMERA_MOVEMENT_MODE_ORBITAL = 3,
			COMPUTER_CAMERA_MOVEMENT_MODE_CAMERA_TOGGLE = 4};
		enum TouchMovementMode { 
			TOUCH_MOVEMENT_MODE_DEFAULT = 0, 
			TOUCH_MOVEMENT_MODE_THUMBSTICK = 1, 
			TOUCH_MOVEMENT_MODE_DPAD = 2, 
			TOUCH_MOVEMENT_MODE_THUMBPAD = 3, 
			TOUCH_MOVEMENT_MODE_CLICK_TO_MOVE = 4,
			TOUCH_MOVEMENT_MODE_DYNAMIC_THUMBSTICK = 5 };
		enum ComputerMovementMode { 
			COMPUTER_MOVEMENT_MODE_DEFAULT = 0, 
			COMPUTER_MOVEMENT_MODE_KBD_MOUSE = 1, 
			COMPUTER_MOVEMENT_MODE_CLICK_TO_MOVE = 2};
		enum RotationType {
			ROTATION_TYPE_MOVEMENT_RELATIVE = 0,
			ROTATION_TYPE_CAMERA_RELATIVE = 1};
		enum VRSafetyBubbleMode {
			VR_SAFETY_BUBBLE_NO_ONE = 0,
			VR_SAFETY_BUBBLE_ONLY_FRIENDS = 1,
			VR_SAFETY_BUBBLE_ANYONE = 2};

		static Reflection::PropDescriptor<GameBasicSettings, float> prop_masterVolume;
		static Reflection::EnumPropDescriptor<GameBasicSettings, Enums::PreferredTextSize> prop_preferredTextSize;

		GameBasicSettings();

		ControlMode getControlMode() const { return controlMode; }
		void setControlMode(ControlMode setting);

		CameraMode getCameraMode() const { return cameraMode; }
		GameBasicSettings::CameraMode getCameraModeWithDefault() const;
		void setCameraMode(CameraMode setting);

		TouchCameraMovementMode getTouchCameraMovementMode() const { return touchCameraMovementMode; }
		void setTouchCameraMovementMode(TouchCameraMovementMode setting);
		bool getTouchCameraMovementModeModified() const { return touchCameraMovementModeModified; }
		void setTouchCameraMovementModeModified(bool setting);

		ComputerCameraMovementMode getComputerCameraMovementMode() const { return computerCameraMovementMode; }
		void setComputerCameraMovementMode(ComputerCameraMovementMode setting);
		bool getComputerCameraMovementModeModified() const { return computerCameraMovementModeModified; }
		void setComputerCameraMovementModeModified(bool setting);

		TouchMovementMode getTouchMovementMode() const { return touchMoveMode; }
		void setTouchMovementMode(TouchMovementMode setting);
		bool getTouchMovementModeModified() const { return touchMoveModeModeModified; }
		void setTouchMovementModeModified(bool setting);

		ComputerMovementMode getComputerMovementMode() const { return computerMoveMode; }
		void setComputerMovementMode(ComputerMovementMode setting);
		bool getComputerMovementModeModified() const { return computerMoveModeModeModified; }
		void setComputerMovementModeModified(bool setting);

		RotationType getRotationType() const;
		void setRotationType(RotationType setting);

		void setMouseLock(bool isLocked);
		bool isMouseLocked() const { return mouseLocked; }

		void setCanMousePan(bool canPan) { canMousePan = canPan; }
		bool getCanMousePan() { return canMousePan; }

		void setFreeLook(bool canLook) { freeLook = canLook; }
		bool getFreeLook() { return freeLook; }

		bool inClassicMode()	{ return controlMode == CONTROL_CLASSIC; }
		bool inMouseLockMode()	{ return controlMode == CONTROL_MOUSELOCK; }
		bool inHybridMode()		{ return controlMode == CONTROL_HYBRID; }
		bool inCamlockMode()	{ return controlMode == CONTROL_CAMLOCK; }
		bool inMousepanMode()	{ return controlMode == CONTROL_MOUSEPAN; }
		
		bool mouseLockedInMouseLockMode()	{ return inMouseLockMode() && isMouseLocked(); }
		bool camLockedInCamLockMode()		{ return inCamlockMode() && !getFreeLook(); }

		bool getTutorialState(std::string tutorialId);
		void setTutorialState(std::string tutorialId, bool value);

		std::string getCompletedTutorials() const;
		void setCompletedTutorials(std::string value);

		GameSettings::UploadSetting getUploadVideoSetting() const { return uploadVideos; }
		void setUploadVideoSetting(GameSettings::UploadSetting setting);

		GameSettings::UploadSetting getPostImageSetting() const { return uploadScreenshots; }
		void setPostImageSetting(GameSettings::UploadSetting setting);

		RenderQualitySetting getRenderQuality() const { return renderQualitySetting; }
		void setRenderQuality(RenderQualitySetting value);

		bool getAllTutorialsDisabled() const { return allTutorialsDisabled; }
		void setAllTutorialsDisabled(bool value);

		bool getFullScreenConst() const { return fullscreen; }
		bool getFullScreen() { return fullscreen; }
		void setFullScreen(bool value) 
		{ 
			if(value != fullscreen)
			{
				fullscreen = value; 
				fullscreenChangedSignal(value);
			}
		}

		Vector2 getStartScreenPos() const { return startScreenPos; }
		void setStartScreenPos(Vector2 value);

		Vector2 getStartScreenSize() const { return startScreenSize; }
		void setStartScreenSize(Vector2 value);

		bool getStartMaximized() const { return startMaximized; }
		void setStartMaximized(bool value);

		float getMasterVolume() const { return masterVolume; }
		void setMasterVolume(float value);

		Enums::PreferredTextSize getPreferredTextSize() const { return preferredTextSize; }
		void setPreferredTextSize(Enums::PreferredTextSize value);

		float getPreferredTransparency() const { return preferredTransparency; }
		void setPreferredTransparency(float value);

		bool getReducedMotion() const { return reducedMotion; }
		void setReducedMotion(bool value);

		Enums::PeoplePageLayout getPeoplePageLayout() const { return peoplePageLayout; }
		void setPeoplePageLayout(Enums::PeoplePageLayout value);

		bool getUiNavigationKeyBindEnabled() const { return uiNavigationKeyBindEnabled; }
		void setUiNavigationKeyBindEnabled(bool value);
		bool getChatVisible() const { return chatVisible; }
		void setChatVisible(bool value);
		bool getPlayerListVisible() const { return playerListVisible; }
		void setPlayerListVisible(bool value);

		bool getPerformanceStatsVisible() const { return performanceStatsVisible; }
		void setPerformanceStatsVisible(bool value);
		bool getOnScreenProfilerEnabled() const { return onScreenProfilerEnabled; }
		void setOnScreenProfilerEnabled(bool value);
		bool getMicroProfilerWebServerEnabled() const { return microProfilerWebServerEnabled; }
		void setMicroProfilerWebServerEnabled(bool value);
		std::string getMicroProfilerWebServerIP() const;
		int getMicroProfilerWebServerPort() const;

		float getMouseSensitivity() const;
		void setMouseSensitivity(float value);
		Vector2 getMouseSensitivityFirstPerson() const { return mouseSensitivityFirstPerson; }
		void setMouseSensitivityFirstPerson(Vector2 value);
		Vector2 getMouseSensitivityThirdPerson() const { return mouseSensitivityThirdPerson; }
		void setMouseSensitivityThirdPerson(Vector2 value);
		float getGamepadCameraSensitivity() const { return gamepadCameraSensitivity; }
		void setGamepadCameraSensitivity(float value);
		bool getIsUsingGamepadCameraSensitivity() const { return isUsingGamepadCameraSensitivity; }
		void setGamepadCameraSensitivityVisible();
		bool getCameraYInverted() const { return cameraYInverted; }
		void setCameraYInverted(bool value);
		bool getIsUsingCameraYInverted() const { return isUsingCameraYInverted; }
		void setCameraYInvertVisible();
		float getCameraYInvertValue() { return cameraYInverted ? -1.0f : 1.0f; }
		float getHapticStrength() const { return hapticStrength; }
		void setHapticStrength(float value);
		VRSafetyBubbleMode getVRSafetyBubbleMode() const { return vrSafetyBubbleMode; }
		void setVRSafetyBubbleMode(VRSafetyBubbleMode value);

		bool inStudioMode() { return studio; }
		void setStudioMode(bool value) 
		{
			if(value != studio)
			{
				studioModeChangedSignal(value);
				studio = value; 
			}
		}

		bool getUsedHideHudShortcut() const { return usedHideHudShortcut; }
		void setUsedHideHudShortcut(bool value) { usedHideHudShortcut = value; }

		bool getHasEverUsedVR() const { return hasEverUsedVR; }
		void setHasEverUsedVR(bool value);
		bool getVREnabled() const { return vrEnabled; }
		void setVREnabled(bool value);

		std::string getGoogleAnalyticsClientId() const;
		void setGoogleAnalyticsClientId(const std::string& id);
		
		/*override*/ void reset();
		/*override*/ void verifySetParent(const Instance* instance) const;

		void recordSettingsInGA(bool touchEnabled) const;

		rbx::signal<void(bool)> fullscreenChangedSignal;
		rbx::signal<void(bool)> studioModeChangedSignal;
		rbx::signal<void(Enums::PreferredTextSize)> preferredTextSizeChangedSignal;
		rbx::signal<void(bool)> performanceStatsVisibleChangedSignal;

	private:	
		ControlMode controlMode;
		RenderQualitySetting renderQualitySetting;
		CameraMode cameraMode;
		TouchCameraMovementMode touchCameraMovementMode;
		bool touchCameraMovementModeModified;
		ComputerCameraMovementMode computerCameraMovementMode;
		bool computerCameraMovementModeModified;
		TouchMovementMode touchMoveMode;
		bool touchMoveModeModeModified;
		ComputerMovementMode computerMoveMode;
		bool computerMoveModeModeModified;
		RotationType rotationType;

		bool mouseLocked;
		bool canMousePan;
		bool freeLook;
		GameSettings::UploadSetting uploadVideos;
		GameSettings::UploadSetting uploadScreenshots;
		bool usedHideHudShortcut;
		bool hasEverUsedVR;
		bool vrEnabled;

		bool fullscreen;
		bool studio;

		Vector2 startScreenPos;
		Vector2 startScreenSize;

		bool startMaximized;

		float masterVolume;
		Enums::PreferredTextSize preferredTextSize;
		float preferredTransparency;
		bool reducedMotion;
		Enums::PeoplePageLayout peoplePageLayout;
		bool uiNavigationKeyBindEnabled;
		bool chatVisible;
		bool playerListVisible;
		bool performanceStatsVisible;
		bool onScreenProfilerEnabled;
		bool microProfilerWebServerEnabled;
		float mouseSensitivity;
		Vector2 mouseSensitivityFirstPerson;
		Vector2 mouseSensitivityThirdPerson;
		float gamepadCameraSensitivity;
		bool isUsingGamepadCameraSensitivity;
		bool cameraYInverted;
		bool isUsingCameraYInverted;
		float hapticStrength;
		VRSafetyBubbleMode vrSafetyBubbleMode;

		std::map<std::string, bool> tutorialState;
		bool allTutorialsDisabled;

		std::string googleAnalyticsClientId;
	};

}
