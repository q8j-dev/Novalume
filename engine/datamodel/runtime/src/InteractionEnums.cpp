#include "V8DataModel/InteractionEnums.h"

#include "reflection/enumconverter.h"

namespace RBX {
namespace Reflection {

template<> EnumDesc<Enums::TextDirection>::EnumDesc()
    : EnumDescriptor("TextDirection")
{
    addPair(Enums::TEXT_DIRECTION_AUTO, "Auto");
    addPair(Enums::TEXT_DIRECTION_LEFT_TO_RIGHT, "LeftToRight");
    addPair(Enums::TEXT_DIRECTION_RIGHT_TO_LEFT, "RightToLeft");
}

template<> EnumDesc<Enums::HapticEffectType>::EnumDesc()
    : EnumDescriptor("HapticEffectType")
{
    addPair(Enums::HAPTIC_EFFECT_CUSTOM, "Custom");
    addPair(Enums::HAPTIC_EFFECT_UI_HOVER, "UIHover");
    addPair(Enums::HAPTIC_EFFECT_UI_CLICK, "UIClick");
    addPair(Enums::HAPTIC_EFFECT_UI_NOTIFICATION, "UINotification");
    addPair(Enums::HAPTIC_EFFECT_GAMEPLAY_EXPLOSION, "GameplayExplosion");
    addPair(Enums::HAPTIC_EFFECT_GAMEPLAY_COLLISION, "GameplayCollision");
}

template<> EnumDesc<Enums::ElasticBehavior>::EnumDesc()
    : EnumDescriptor("ElasticBehavior")
{
    addPair(Enums::ELASTIC_BEHAVIOR_WHEN_SCROLLABLE, "WhenScrollable");
    addPair(Enums::ELASTIC_BEHAVIOR_ALWAYS, "Always");
    addPair(Enums::ELASTIC_BEHAVIOR_NEVER, "Never");
}

template<> EnumDesc<Enums::BorderMode>::EnumDesc()
    : EnumDescriptor("BorderMode")
{
    addPair(Enums::BORDER_MODE_OUTLINE, "Outline");
    addPair(Enums::BORDER_MODE_MIDDLE, "Middle");
    addPair(Enums::BORDER_MODE_INSET, "Inset");
}

template<> EnumDesc<Enums::ScrollingDirection>::EnumDesc()
    : EnumDescriptor("ScrollingDirection")
{
    addPair(Enums::SCROLLING_DIRECTION_X, "X");
    addPair(Enums::SCROLLING_DIRECTION_Y, "Y");
    addPair(Enums::SCROLLING_DIRECTION_XY, "XY");
}

template<> EnumDesc<Enums::ScrollBarInset>::EnumDesc()
    : EnumDescriptor("ScrollBarInset")
{
    addPair(Enums::SCROLL_BAR_INSET_NONE, "None");
    addPair(Enums::SCROLL_BAR_INSET_SCROLL_BAR, "ScrollBar");
    addPair(Enums::SCROLL_BAR_INSET_ALWAYS, "Always");
}

template<> EnumDesc<Enums::VerticalScrollBarPosition>::EnumDesc()
    : EnumDescriptor("VerticalScrollBarPosition")
{
    addPair(Enums::VERTICAL_SCROLL_BAR_POSITION_RIGHT, "Right");
    addPair(Enums::VERTICAL_SCROLL_BAR_POSITION_LEFT, "Left");
}

template<> EnumDesc<Enums::VoiceChatDistanceAttenuationType>::EnumDesc()
    : EnumDescriptor("VoiceChatDistanceAttenuationType")
{
    addPair(Enums::VOICE_DISTANCE_INVERSE, "Inverse");
    addPair(Enums::VOICE_DISTANCE_LEGACY, "Legacy");
}

template<> EnumDesc<Enums::RolloutState>::EnumDesc()
    : EnumDescriptor("RolloutState")
{
    addPair(Enums::ROLLOUT_DEFAULT, "Default");
    addPair(Enums::ROLLOUT_DISABLED, "Disabled");
    addPair(Enums::ROLLOUT_ENABLED, "Enabled");
}

template<> EnumDesc<Enums::AudioApiRollout>::EnumDesc()
    : EnumDescriptor("AudioApiRollout")
{
    addPair(Enums::AUDIO_API_DISABLED, "Disabled");
    addPair(Enums::AUDIO_API_AUTOMATIC, "Automatic");
    addPair(Enums::AUDIO_API_ENABLED, "Enabled");
}

template<> EnumDesc<Enums::VoiceClientLeaveReasons>::EnumDesc()
    : EnumDescriptor("VoiceClientLeaveReasons")
{
    addPair(Enums::VOICE_LEAVE_UNKNOWN, "Unknown");
    addPair(Enums::VOICE_LEAVE_CLIENT_NETWORK_DISCONNECTED, "ClientNetworkDisconnected");
    addPair(Enums::VOICE_LEAVE_PLAYER_LEFT, "PlayerLeft");
    addPair(Enums::VOICE_LEAVE_CLIENT_SHUTDOWN, "ClientShutdown");
    addPair(Enums::VOICE_LEAVE_PUBLISH_FAILED, "PublishFailed");
    addPair(Enums::VOICE_LEAVE_REJOIN_RECEIVED, "RejoinReceived");
    addPair(Enums::VOICE_LEAVE_REBOOT, "VoiceReboot");
    addPair(Enums::VOICE_LEAVE_IMGUI_DEBUG, "ImguiDebugLeave");
    addPair(Enums::VOICE_LEAVE_LUA_INITIATED, "LuaInitiated");
}

template<> EnumDesc<Enums::TextInputType>::EnumDesc()
    : EnumDescriptor("TextInputType")
{
    addPair(Enums::TEXT_INPUT_DEFAULT, "Default");
    addPair(Enums::TEXT_INPUT_NO_SUGGESTIONS, "NoSuggestions");
    addPair(Enums::TEXT_INPUT_NUMBER, "Number");
    addPair(Enums::TEXT_INPUT_EMAIL, "Email");
    addPair(Enums::TEXT_INPUT_PHONE, "Phone");
    addPair(Enums::TEXT_INPUT_PASSWORD, "Password");
    addPair(Enums::TEXT_INPUT_PASSWORD_SHOWN, "PasswordShown");
    addPair(Enums::TEXT_INPUT_USERNAME, "Username");
    addPair(Enums::TEXT_INPUT_ONE_TIME_PASSWORD, "OneTimePassword");
    addPair(Enums::TEXT_INPUT_NEW_PASSWORD, "NewPassword");
    addPair(Enums::TEXT_INPUT_NEW_PASSWORD_SHOWN, "NewPasswordShown");
}

template<> EnumDesc<Enums::ChatRestrictionStatus>::EnumDesc()
    : EnumDescriptor("ChatRestrictionStatus")
{
    addPair(Enums::CHAT_RESTRICTION_UNKNOWN, "Unknown");
    addPair(Enums::CHAT_RESTRICTION_NOT_RESTRICTED, "NotRestricted");
    addPair(Enums::CHAT_RESTRICTION_RESTRICTED, "Restricted");
}

template<> EnumDesc<Enums::ReturnKeyType>::EnumDesc()
    : EnumDescriptor("ReturnKeyType")
{
    addPair(Enums::RETURN_KEY_DEFAULT, "Default");
    addPair(Enums::RETURN_KEY_DONE, "Done");
    addPair(Enums::RETURN_KEY_GO, "Go");
    addPair(Enums::RETURN_KEY_NEXT, "Next");
    addPair(Enums::RETURN_KEY_SEARCH, "Search");
    addPair(Enums::RETURN_KEY_SEND, "Send");
}

template<> EnumDesc<Enums::AppShellActionType>::EnumDesc()
    : EnumDescriptor("AppShellActionType")
{
    addPair(Enums::APP_SHELL_ACTION_NONE, "None");
    addPair(Enums::APP_SHELL_ACTION_OPEN_APP, "OpenApp");
    addPair(Enums::APP_SHELL_ACTION_TAP_CHAT_TAB, "TapChatTab");
    addPair(Enums::APP_SHELL_ACTION_TAP_CONVERSATION_ENTRY, "TapConversationEntry");
    addPair(Enums::APP_SHELL_ACTION_TAP_AVATAR_TAB, "TapAvatarTab");
    addPair(Enums::APP_SHELL_ACTION_READ_CONVERSATION, "ReadConversation");
    addPair(Enums::APP_SHELL_ACTION_TAP_GAME_PAGE_TAB, "TapGamePageTab");
    addPair(Enums::APP_SHELL_ACTION_TAP_HOME_PAGE_TAB, "TapHomePageTab");
    addPair(Enums::APP_SHELL_ACTION_GAME_PAGE_LOADED, "GamePageLoaded");
    addPair(Enums::APP_SHELL_ACTION_HOME_PAGE_LOADED, "HomePageLoaded");
    addPair(Enums::APP_SHELL_ACTION_AVATAR_EDITOR_PAGE_LOADED, "AvatarEditorPageLoaded");
    addPair(Enums::APP_SHELL_ACTION_HOME_PAGE_INTERACTIVE, "HomePageInteractive");
}

template<> EnumDesc<Enums::AppShellFeature>::EnumDesc()
    : EnumDescriptor("AppShellFeature")
{
    addPair(Enums::APP_SHELL_FEATURE_NONE, "None");
    addPair(Enums::APP_SHELL_FEATURE_CHAT, "Chat");
    addPair(Enums::APP_SHELL_FEATURE_AVATAR_EDITOR, "AvatarEditor");
    addPair(Enums::APP_SHELL_FEATURE_GAME_PAGE, "GamePage");
    addPair(Enums::APP_SHELL_FEATURE_HOME_PAGE, "HomePage");
    addPair(Enums::APP_SHELL_FEATURE_MORE, "More");
    addPair(Enums::APP_SHELL_FEATURE_LANDING, "Landing");
    addPair(Enums::APP_SHELL_FEATURE_WATCH_PAGE, "WatchPage");
}

template<> EnumDesc<Enums::ConnectionState>::EnumDesc()
    : EnumDescriptor("ConnectionState")
{
    addPair(Enums::CONNECTION_CONNECTED, "Connected");
    addPair(Enums::CONNECTION_DISCONNECTED, "Disconnected");
}

template<> EnumDesc<Enums::AvatarChatServiceFeature>::EnumDesc()
    : EnumDescriptor("AvatarChatServiceFeature")
{
    addPair(Enums::AVATAR_CHAT_NONE, "None");
    addPair(Enums::AVATAR_CHAT_UNIVERSE_AUDIO, "UniverseAudio");
    addPair(Enums::AVATAR_CHAT_UNIVERSE_VIDEO, "UniverseVideo");
    addPair(Enums::AVATAR_CHAT_PLACE_AUDIO, "PlaceAudio");
    addPair(Enums::AVATAR_CHAT_PLACE_VIDEO, "PlaceVideo");
    addPair(Enums::AVATAR_CHAT_USER_AUDIO_ELIGIBLE, "UserAudioEligible");
    addPair(Enums::AVATAR_CHAT_USER_AUDIO, "UserAudio");
    addPair(Enums::AVATAR_CHAT_USER_VIDEO_ELIGIBLE, "UserVideoEligible");
    addPair(Enums::AVATAR_CHAT_USER_VIDEO, "UserVideo");
    addPair(Enums::AVATAR_CHAT_USER_BANNED, "UserBanned");
    addPair(Enums::AVATAR_CHAT_USER_VERIFIED_FOR_VOICE, "UserVerifiedForVoice");
}

template<> EnumDesc<Enums::DeviceFeatureType>::EnumDesc()
    : EnumDescriptor("DeviceFeatureType")
{
    addPair(Enums::DEVICE_FEATURE_CAPTURE, "DeviceCapture");
    addPair(Enums::DEVICE_FEATURE_IN_EXPERIENCE_FAE, "InExperienceFAE");
}

template<> EnumDesc<Enums::DeviceLevel>::EnumDesc()
    : EnumDescriptor("DeviceLevel")
{
    addPair(Enums::DEVICE_LEVEL_LOW, "Low");
    addPair(Enums::DEVICE_LEVEL_MEDIUM, "Medium");
    addPair(Enums::DEVICE_LEVEL_HIGH, "High");
}

template<> EnumDesc<Enums::PeoplePageLayout>::EnumDesc()
    : EnumDescriptor("PeoplePageLayout")
{
    addPair(Enums::PEOPLE_PAGE_LAYOUT_CARD, "Card");
    addPair(Enums::PEOPLE_PAGE_LAYOUT_LIST, "List");
}

template<> EnumDesc<Enums::BundleType>::EnumDesc()
    : EnumDescriptor("BundleType")
{
    addPair(Enums::BUNDLE_BODY_PARTS, "BodyParts");
    addPair(Enums::BUNDLE_ANIMATIONS, "Animations");
    addPair(Enums::BUNDLE_SHOES, "Shoes");
    addPair(Enums::BUNDLE_DYNAMIC_HEAD, "DynamicHead");
    addPair(Enums::BUNDLE_DYNAMIC_HEAD_AVATAR, "DynamicHeadAvatar");
}

template<> EnumDesc<Enums::AvatarContextMenuOption>::EnumDesc()
    : EnumDescriptor("AvatarContextMenuOption")
{
    addPair(Enums::AVATAR_CONTEXT_FRIEND, "Friend");
    addPair(Enums::AVATAR_CONTEXT_CHAT, "Chat");
    addPair(Enums::AVATAR_CONTEXT_EMOTE, "Emote");
    addPair(Enums::AVATAR_CONTEXT_INSPECT_MENU, "InspectMenu");
}

template<> EnumDesc<Enums::AvatarItemType>::EnumDesc()
    : EnumDescriptor("AvatarItemType")
{
    addPair(Enums::AVATAR_ITEM_ASSET, "Asset");
    addPair(Enums::AVATAR_ITEM_BUNDLE, "Bundle");
}

template<> EnumDesc<Enums::ContextActionResult>::EnumDesc()
    : EnumDescriptor("ContextActionResult")
{
    addPair(Enums::CONTEXT_ACTION_SINK, "Sink");
    addPair(Enums::CONTEXT_ACTION_PASS, "Pass");
}

template<> EnumDesc<Enums::HttpRequestType>::EnumDesc()
    : EnumDescriptor("HttpRequestType")
{
    addPair(Enums::HTTP_REQUEST_DEFAULT, "Default");
    addPair(Enums::HTTP_REQUEST_MARKETPLACE_SERVICE, "MarketplaceService");
    addPair(Enums::HTTP_REQUEST_PLAYERS, "Players");
    addPair(Enums::HTTP_REQUEST_CHAT, "Chat");
    addPair(Enums::HTTP_REQUEST_AVATAR, "Avatar");
    addPair(Enums::HTTP_REQUEST_ANALYTICS, "Analytics");
    addPair(Enums::HTTP_REQUEST_LOCALIZATION, "Localization");
}

template<> EnumDesc<Enums::DisplaySize>::EnumDesc()
    : EnumDescriptor("DisplaySize")
{
    addPair(Enums::DISPLAY_SIZE_SMALL, "Small");
    addPair(Enums::DISPLAY_SIZE_MEDIUM, "Medium");
    addPair(Enums::DISPLAY_SIZE_LARGE, "Large");
}

template<> EnumDesc<Enums::AvatarAssetType>::EnumDesc()
    : EnumDescriptor("AvatarAssetType")
{
    addPair(Enums::AVATAR_ASSET_TSHIRT, "TShirt"); addPair(Enums::AVATAR_ASSET_HAT, "Hat");
    addPair(Enums::AVATAR_ASSET_SHIRT, "Shirt"); addPair(Enums::AVATAR_ASSET_PANTS, "Pants");
    addPair(Enums::AVATAR_ASSET_HEAD, "Head"); addPair(Enums::AVATAR_ASSET_FACE, "Face");
    addPair(Enums::AVATAR_ASSET_GEAR, "Gear"); addPair(Enums::AVATAR_ASSET_TORSO, "Torso");
    addPair(Enums::AVATAR_ASSET_RIGHT_ARM, "RightArm"); addPair(Enums::AVATAR_ASSET_LEFT_ARM, "LeftArm");
    addPair(Enums::AVATAR_ASSET_LEFT_LEG, "LeftLeg"); addPair(Enums::AVATAR_ASSET_RIGHT_LEG, "RightLeg");
    addPair(Enums::AVATAR_ASSET_HAIR_ACCESSORY, "HairAccessory"); addPair(Enums::AVATAR_ASSET_FACE_ACCESSORY, "FaceAccessory");
    addPair(Enums::AVATAR_ASSET_NECK_ACCESSORY, "NeckAccessory"); addPair(Enums::AVATAR_ASSET_SHOULDER_ACCESSORY, "ShoulderAccessory");
    addPair(Enums::AVATAR_ASSET_FRONT_ACCESSORY, "FrontAccessory"); addPair(Enums::AVATAR_ASSET_BACK_ACCESSORY, "BackAccessory");
    addPair(Enums::AVATAR_ASSET_WAIST_ACCESSORY, "WaistAccessory"); addPair(Enums::AVATAR_ASSET_CLIMB_ANIMATION, "ClimbAnimation");
    addPair(Enums::AVATAR_ASSET_FALL_ANIMATION, "FallAnimation"); addPair(Enums::AVATAR_ASSET_IDLE_ANIMATION, "IdleAnimation");
    addPair(Enums::AVATAR_ASSET_JUMP_ANIMATION, "JumpAnimation"); addPair(Enums::AVATAR_ASSET_RUN_ANIMATION, "RunAnimation");
    addPair(Enums::AVATAR_ASSET_SWIM_ANIMATION, "SwimAnimation"); addPair(Enums::AVATAR_ASSET_WALK_ANIMATION, "WalkAnimation");
    addPair(Enums::AVATAR_ASSET_EMOTE_ANIMATION, "EmoteAnimation"); addPair(Enums::AVATAR_ASSET_TSHIRT_ACCESSORY, "TShirtAccessory");
    addPair(Enums::AVATAR_ASSET_SHIRT_ACCESSORY, "ShirtAccessory"); addPair(Enums::AVATAR_ASSET_PANTS_ACCESSORY, "PantsAccessory");
    addPair(Enums::AVATAR_ASSET_JACKET_ACCESSORY, "JacketAccessory"); addPair(Enums::AVATAR_ASSET_SWEATER_ACCESSORY, "SweaterAccessory");
    addPair(Enums::AVATAR_ASSET_SHORTS_ACCESSORY, "ShortsAccessory"); addPair(Enums::AVATAR_ASSET_LEFT_SHOE_ACCESSORY, "LeftShoeAccessory");
    addPair(Enums::AVATAR_ASSET_RIGHT_SHOE_ACCESSORY, "RightShoeAccessory"); addPair(Enums::AVATAR_ASSET_DRESS_SKIRT_ACCESSORY, "DressSkirtAccessory");
    addPair(Enums::AVATAR_ASSET_EYEBROW_ACCESSORY, "EyebrowAccessory"); addPair(Enums::AVATAR_ASSET_EYELASH_ACCESSORY, "EyelashAccessory");
    addPair(Enums::AVATAR_ASSET_MOOD_ANIMATION, "MoodAnimation"); addPair(Enums::AVATAR_ASSET_DYNAMIC_HEAD, "DynamicHead");
    addPair(Enums::AVATAR_ASSET_FACE_MAKEUP, "FaceMakeup"); addPair(Enums::AVATAR_ASSET_LIP_MAKEUP, "LipMakeup");
    addPair(Enums::AVATAR_ASSET_EYE_MAKEUP, "EyeMakeup"); addPair(Enums::AVATAR_ASSET_AVATAR_BACKGROUND, "AvatarBackground");
}

template<> EnumDesc<Enums::MakeupType>::EnumDesc()
    : EnumDescriptor("MakeupType")
{
    addPair(Enums::MAKEUP_TYPE_FACE, "Face");
    addPair(Enums::MAKEUP_TYPE_LIP, "Lip");
    addPair(Enums::MAKEUP_TYPE_EYE, "Eye");
}

template<> EnumDesc<Enums::MarketplaceItemPurchaseStatus>::EnumDesc()
    : EnumDescriptor("MarketplaceItemPurchaseStatus")
{
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_SUCCESS, "Success");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_SYSTEM_ERROR, "SystemError");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_ALREADY_OWNED, "AlreadyOwned");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_INSUFFICIENT_ROBUX, "InsufficientRobux");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_QUANTITY_LIMIT_EXCEEDED, "QuantityLimitExceeded");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_QUOTA_EXCEEDED, "QuotaExceeded");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_NOT_FOR_SALE, "NotForSale");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_NOT_AVAILABLE_FOR_PURCHASER, "NotAvailableForPurchaser");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_PRICE_MISMATCH, "PriceMismatch");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_SOLD_OUT, "SoldOut");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_PURCHASER_IS_SELLER, "PurchaserIsSeller");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_INSUFFICIENT_MEMBERSHIP, "InsufficientMembership");
    addPair(Enums::MARKETPLACE_ITEM_PURCHASE_PLACE_INVALID, "PlaceInvalid");
}

template<> EnumDesc<Enums::RaycastFilterType>::EnumDesc()
    : EnumDescriptor("RaycastFilterType")
{
    addPair(Enums::RAYCAST_FILTER_EXCLUDE, "Exclude");
    addLegacyName("Blacklist", Enums::RAYCAST_FILTER_EXCLUDE);
    addPair(Enums::RAYCAST_FILTER_INCLUDE, "Include");
    addLegacyName("Whitelist", Enums::RAYCAST_FILTER_INCLUDE);
}

template<> EnumDesc<Enums::PromptPublishAssetResult>::EnumDesc()
    : EnumDescriptor("PromptPublishAssetResult")
{
    addPair(Enums::PROMPT_PUBLISH_ASSET_SUCCESS, "Success");
    addPair(Enums::PROMPT_PUBLISH_ASSET_PERMISSION_DENIED, "PermissionDenied");
    addPair(Enums::PROMPT_PUBLISH_ASSET_TIMEOUT, "Timeout");
    addPair(Enums::PROMPT_PUBLISH_ASSET_UPLOAD_FAILED, "UploadFailed");
    addPair(Enums::PROMPT_PUBLISH_ASSET_NO_USER_INPUT, "NoUserInput");
    addPair(Enums::PROMPT_PUBLISH_ASSET_UNKNOWN_FAILURE, "UnknownFailure");
}

template<> EnumDesc<Enums::MarketplaceBulkPurchasePromptStatus>::EnumDesc()
    : EnumDescriptor("MarketplaceBulkPurchasePromptStatus")
{
    addPair(Enums::MARKETPLACE_BULK_PURCHASE_COMPLETED, "Completed");
    addPair(Enums::MARKETPLACE_BULK_PURCHASE_ABORTED, "Aborted");
    addPair(Enums::MARKETPLACE_BULK_PURCHASE_ERROR, "Error");
}

template<> EnumDesc<Enums::AppLifecycleManagerState>::EnumDesc()
    : EnumDescriptor("AppLifecycleManagerState")
{
    addPair(Enums::APP_LIFECYCLE_DETACHED, "Detached");
    addPair(Enums::APP_LIFECYCLE_ACTIVE, "Active");
    addPair(Enums::APP_LIFECYCLE_INACTIVE, "Inactive");
    addPair(Enums::APP_LIFECYCLE_HIDDEN, "Hidden");
}

template<> EnumDesc<Enums::CaptureType>::EnumDesc()
    : EnumDescriptor("CaptureType")
{
    addPair(Enums::CAPTURE_TYPE_SCREENSHOT, "Screenshot");
    addPair(Enums::CAPTURE_TYPE_VIDEO, "Video");
}

template<> EnumDesc<Enums::CaptureGalleryPermission>::EnumDesc()
    : EnumDescriptor("CaptureGalleryPermission")
{
    addPair(Enums::CAPTURE_GALLERY_READ_AND_UPLOAD, "ReadAndUpload");
}

template<> EnumDesc<Enums::ReadCapturesFromGalleryResult>::EnumDesc()
    : EnumDescriptor("ReadCapturesFromGalleryResult")
{
    addPair(Enums::READ_CAPTURES_SUCCESS, "Success");
    addPair(Enums::READ_CAPTURES_NEED_PERMISSION, "NeedPermission");
}

template<> EnumDesc<Enums::VideoCaptureStartedResult>::EnumDesc()
    : EnumDescriptor("VideoCaptureStartedResult")
{
    addPair(Enums::VIDEO_CAPTURE_STARTED_SUCCESS, "Success");
    addPair(Enums::VIDEO_CAPTURE_STARTED_OTHER_ERROR, "OtherError");
    addPair(Enums::VIDEO_CAPTURE_STARTED_ALREADY, "CapturingAlready");
    addPair(Enums::VIDEO_CAPTURE_STARTED_NO_DEVICE_SUPPORT, "NoDeviceSupport");
    addPair(Enums::VIDEO_CAPTURE_STARTED_NO_SPACE, "NoSpaceOnDevice");
}

template<> EnumDesc<Enums::VideoCaptureResult>::EnumDesc()
    : EnumDescriptor("VideoCaptureResult")
{
    addPair(Enums::VIDEO_CAPTURE_SUCCESS, "Success");
    addPair(Enums::VIDEO_CAPTURE_OTHER_ERROR, "OtherError");
    addPair(Enums::VIDEO_CAPTURE_SCREEN_SIZE_CHANGED, "ScreenSizeChanged");
    addPair(Enums::VIDEO_CAPTURE_TIME_LIMIT_REACHED, "TimeLimitReached");
}

template<> EnumDesc<Enums::AssetType>::EnumDesc() : EnumDescriptor("AssetType")
{
#define RBX_ASSET(value, name) addPair(Enums::value, name)
    RBX_ASSET(ASSET_IMAGE, "Image"); RBX_ASSET(ASSET_TSHIRT, "TShirt");
    RBX_ASSET(ASSET_AUDIO, "Audio"); RBX_ASSET(ASSET_MESH, "Mesh"); RBX_ASSET(ASSET_LUA, "Lua");
    RBX_ASSET(ASSET_HAT, "Hat"); RBX_ASSET(ASSET_PLACE, "Place"); RBX_ASSET(ASSET_MODEL, "Model");
    RBX_ASSET(ASSET_SHIRT, "Shirt"); RBX_ASSET(ASSET_PANTS, "Pants"); RBX_ASSET(ASSET_DECAL, "Decal");
    RBX_ASSET(ASSET_HEAD, "Head"); RBX_ASSET(ASSET_FACE, "Face"); RBX_ASSET(ASSET_GEAR, "Gear");
    RBX_ASSET(ASSET_BADGE, "Badge"); RBX_ASSET(ASSET_ANIMATION, "Animation"); RBX_ASSET(ASSET_TORSO, "Torso");
    RBX_ASSET(ASSET_RIGHT_ARM, "RightArm"); RBX_ASSET(ASSET_LEFT_ARM, "LeftArm");
    RBX_ASSET(ASSET_LEFT_LEG, "LeftLeg"); RBX_ASSET(ASSET_RIGHT_LEG, "RightLeg");
    RBX_ASSET(ASSET_PACKAGE, "Package"); RBX_ASSET(ASSET_GAME_PASS, "GamePass");
    RBX_ASSET(ASSET_PLUGIN, "Plugin"); RBX_ASSET(ASSET_MESH_PART, "MeshPart");
    RBX_ASSET(ASSET_HAIR_ACCESSORY, "HairAccessory"); RBX_ASSET(ASSET_FACE_ACCESSORY, "FaceAccessory");
    RBX_ASSET(ASSET_NECK_ACCESSORY, "NeckAccessory"); RBX_ASSET(ASSET_SHOULDER_ACCESSORY, "ShoulderAccessory");
    RBX_ASSET(ASSET_FRONT_ACCESSORY, "FrontAccessory"); RBX_ASSET(ASSET_BACK_ACCESSORY, "BackAccessory");
    RBX_ASSET(ASSET_WAIST_ACCESSORY, "WaistAccessory"); RBX_ASSET(ASSET_CLIMB_ANIMATION, "ClimbAnimation");
    RBX_ASSET(ASSET_DEATH_ANIMATION, "DeathAnimation"); RBX_ASSET(ASSET_FALL_ANIMATION, "FallAnimation");
    RBX_ASSET(ASSET_IDLE_ANIMATION, "IdleAnimation"); RBX_ASSET(ASSET_JUMP_ANIMATION, "JumpAnimation");
    RBX_ASSET(ASSET_RUN_ANIMATION, "RunAnimation"); RBX_ASSET(ASSET_SWIM_ANIMATION, "SwimAnimation");
    RBX_ASSET(ASSET_WALK_ANIMATION, "WalkAnimation"); RBX_ASSET(ASSET_POSE_ANIMATION, "PoseAnimation");
    RBX_ASSET(ASSET_EAR_ACCESSORY, "EarAccessory"); RBX_ASSET(ASSET_EYE_ACCESSORY, "EyeAccessory");
    RBX_ASSET(ASSET_EMOTE_ANIMATION, "EmoteAnimation"); RBX_ASSET(ASSET_VIDEO, "Video");
    RBX_ASSET(ASSET_TSHIRT_ACCESSORY, "TShirtAccessory"); RBX_ASSET(ASSET_SHIRT_ACCESSORY, "ShirtAccessory");
    RBX_ASSET(ASSET_PANTS_ACCESSORY, "PantsAccessory"); RBX_ASSET(ASSET_JACKET_ACCESSORY, "JacketAccessory");
    RBX_ASSET(ASSET_SWEATER_ACCESSORY, "SweaterAccessory"); RBX_ASSET(ASSET_SHORTS_ACCESSORY, "ShortsAccessory");
    RBX_ASSET(ASSET_LEFT_SHOE_ACCESSORY, "LeftShoeAccessory"); RBX_ASSET(ASSET_RIGHT_SHOE_ACCESSORY, "RightShoeAccessory");
    RBX_ASSET(ASSET_DRESS_SKIRT_ACCESSORY, "DressSkirtAccessory"); RBX_ASSET(ASSET_FONT_FAMILY, "FontFamily");
    RBX_ASSET(ASSET_EYEBROW_ACCESSORY, "EyebrowAccessory"); RBX_ASSET(ASSET_EYELASH_ACCESSORY, "EyelashAccessory");
    RBX_ASSET(ASSET_MOOD_ANIMATION, "MoodAnimation"); RBX_ASSET(ASSET_DYNAMIC_HEAD, "DynamicHead");
    RBX_ASSET(ASSET_FACE_MAKEUP, "FaceMakeup"); RBX_ASSET(ASSET_LIP_MAKEUP, "LipMakeup");
    RBX_ASSET(ASSET_EYE_MAKEUP, "EyeMakeup"); RBX_ASSET(ASSET_VOXEL_FRAGMENT, "VoxelFragment");
    RBX_ASSET(ASSET_AVATAR_BACKGROUND, "AvatarBackground"); RBX_ASSET(ASSET_TEXT_DOCUMENT, "TextDocument");
#undef RBX_ASSET
}

template<> EnumDesc<Enums::AccessoryType>::EnumDesc() : EnumDescriptor("AccessoryType")
{
    addPair(Enums::ACCESSORY_UNKNOWN, "Unknown"); addPair(Enums::ACCESSORY_HAT, "Hat");
    addPair(Enums::ACCESSORY_HAIR, "Hair"); addPair(Enums::ACCESSORY_FACE, "Face");
    addPair(Enums::ACCESSORY_NECK, "Neck"); addPair(Enums::ACCESSORY_SHOULDER, "Shoulder");
    addPair(Enums::ACCESSORY_FRONT, "Front"); addPair(Enums::ACCESSORY_BACK, "Back");
    addPair(Enums::ACCESSORY_WAIST, "Waist"); addPair(Enums::ACCESSORY_TSHIRT, "TShirt");
    addPair(Enums::ACCESSORY_SHIRT, "Shirt"); addPair(Enums::ACCESSORY_PANTS, "Pants");
    addPair(Enums::ACCESSORY_JACKET, "Jacket"); addPair(Enums::ACCESSORY_SWEATER, "Sweater");
    addPair(Enums::ACCESSORY_SHORTS, "Shorts"); addPair(Enums::ACCESSORY_LEFT_SHOE, "LeftShoe");
    addPair(Enums::ACCESSORY_RIGHT_SHOE, "RightShoe"); addPair(Enums::ACCESSORY_DRESS_SKIRT, "DressSkirt");
    addPair(Enums::ACCESSORY_EYEBROW, "Eyebrow"); addPair(Enums::ACCESSORY_EYELASH, "Eyelash");
}

template<> EnumDesc<Enums::TrackerMode>::EnumDesc() : EnumDescriptor("TrackerMode") { addPair(Enums::TRACKER_MODE_NONE, "None"); addPair(Enums::TRACKER_MODE_AUDIO, "Audio"); addPair(Enums::TRACKER_MODE_VIDEO, "Video"); addPair(Enums::TRACKER_MODE_AUDIO_VIDEO, "AudioVideo"); }
template<> EnumDesc<Enums::TrackerError>::EnumDesc() : EnumDescriptor("TrackerError") { addPair(Enums::TRACKER_ERROR_OK, "Ok"); addPair(Enums::TRACKER_ERROR_NO_SERVICE, "NoService"); addPair(Enums::TRACKER_ERROR_INIT_FAILED, "InitFailed"); addPair(Enums::TRACKER_ERROR_NO_VIDEO, "NoVideo"); addPair(Enums::TRACKER_ERROR_VIDEO, "VideoError"); addPair(Enums::TRACKER_ERROR_VIDEO_NO_PERMISSION, "VideoNoPermission"); addPair(Enums::TRACKER_ERROR_VIDEO_UNSUPPORTED, "VideoUnsupported"); addPair(Enums::TRACKER_ERROR_NO_AUDIO, "NoAudio"); addPair(Enums::TRACKER_ERROR_AUDIO, "AudioError"); addPair(Enums::TRACKER_ERROR_AUDIO_NO_PERMISSION, "AudioNoPermission"); addPair(Enums::TRACKER_ERROR_UNSUPPORTED_DEVICE, "UnsupportedDevice"); }
template<> EnumDesc<Enums::TrackerFaceTrackingStatus>::EnumDesc() : EnumDescriptor("TrackerFaceTrackingStatus") { addPair(Enums::TRACKER_FACE_SUCCESS, "FaceTrackingSuccess"); addPair(Enums::TRACKER_FACE_NO_FACE_FOUND, "FaceTrackingNoFaceFound"); addPair(Enums::TRACKER_FACE_UNKNOWN, "FaceTrackingUnknown"); addPair(Enums::TRACKER_FACE_LOST, "FaceTrackingLost"); addPair(Enums::TRACKER_FACE_HAS_TRACKING_ERROR, "FaceTrackingHasTrackingError"); addPair(Enums::TRACKER_FACE_OCCLUDED, "FaceTrackingIsOccluded"); addPair(Enums::TRACKER_FACE_UNINITIALIZED, "FaceTrackingUninitialized"); }
template<> EnumDesc<Enums::TrackerPromptEvent>::EnumDesc() : EnumDescriptor("TrackerPromptEvent") { addPair(Enums::TRACKER_PROMPT_LOD_CAMERA_RECOMMEND_DISABLE, "LODCameraRecommendDisable"); }
template<> EnumDesc<Enums::TrackerExtrapolationFlagMode>::EnumDesc() : EnumDescriptor("TrackerExtrapolationFlagMode") { addPair(Enums::TRACKER_EXTRAPOLATION_FORCE_DISABLED, "ForceDisabled"); addPair(Enums::TRACKER_EXTRAPOLATION_FACS_AND_POSE, "ExtrapolateFacsAndPose"); addPair(Enums::TRACKER_EXTRAPOLATION_FACS_ONLY, "ExtrapolateFacsOnly"); addPair(Enums::TRACKER_EXTRAPOLATION_AUTO, "Auto"); }
template<> EnumDesc<Enums::TrackerLodFlagMode>::EnumDesc() : EnumDescriptor("TrackerLodFlagMode") { addPair(Enums::TRACKER_LOD_FLAG_FORCE_FALSE, "ForceFalse"); addPair(Enums::TRACKER_LOD_FLAG_FORCE_TRUE, "ForceTrue"); addPair(Enums::TRACKER_LOD_FLAG_AUTO, "Auto"); }
template<> EnumDesc<Enums::TrackerLodValueMode>::EnumDesc() : EnumDescriptor("TrackerLodValueMode") { addPair(Enums::TRACKER_LOD_VALUE_FORCE_0, "Force0"); addPair(Enums::TRACKER_LOD_VALUE_FORCE_1, "Force1"); addPair(Enums::TRACKER_LOD_VALUE_AUTO, "Auto"); }
template<> EnumDesc<Enums::HttpError>::EnumDesc() : EnumDescriptor("HttpError") { addPair(Enums::HTTP_ERROR_OK, "OK"); addPair(Enums::HTTP_ERROR_INVALID_URL, "InvalidUrl"); addPair(Enums::HTTP_ERROR_DNS_RESOLVE, "DnsResolve"); addPair(Enums::HTTP_ERROR_CONNECT_FAIL, "ConnectFail"); addPair(Enums::HTTP_ERROR_OUT_OF_MEMORY, "OutOfMemory"); addPair(Enums::HTTP_ERROR_TIMED_OUT, "TimedOut"); addPair(Enums::HTTP_ERROR_TOO_MANY_REDIRECTS, "TooManyRedirects"); addPair(Enums::HTTP_ERROR_INVALID_REDIRECT, "InvalidRedirect"); addPair(Enums::HTTP_ERROR_NET_FAIL, "NetFail"); addPair(Enums::HTTP_ERROR_ABORTED, "Aborted"); addPair(Enums::HTTP_ERROR_SSL_CONNECT_FAIL, "SslConnectFail"); addPair(Enums::HTTP_ERROR_SSL_VERIFICATION_FAIL, "SslVerificationFail"); addPair(Enums::HTTP_ERROR_UNKNOWN, "Unknown"); addPair(Enums::HTTP_ERROR_CONNECTION_CLOSED, "ConnectionClosed"); addPair(Enums::HTTP_ERROR_SERVER_PROTOCOL, "ServerProtocolError"); addPair(Enums::HTTP_ERROR_CREATOR_ENVIRONMENTS_UNSUPPORTED, "CreatorEnvironmentsNotSupportedByService"); addPair(Enums::HTTP_ERROR_INACTIVITY_TIMEOUT, "InactivityTimeout"); addPair(Enums::HTTP_ERROR_TOO_MANY_OUTSTANDING_REQUESTS, "TooManyOutstandingRequests"); addPair(Enums::HTTP_ERROR_INVALID_RANGE_RESPONSE, "InvalidRangeResponse"); }
template<> EnumDesc<Enums::ExperienceAuthScope>::EnumDesc() : EnumDescriptor("ExperienceAuthScope") { addPair(Enums::EXPERIENCE_AUTH_SCOPE_DEFAULT, "DefaultScope"); addPair(Enums::EXPERIENCE_AUTH_SCOPE_CREATOR_ASSETS_CREATE, "CreatorAssetsCreate"); }
template<> EnumDesc<Enums::ScopeCheckResult>::EnumDesc() : EnumDescriptor("ScopeCheckResult") { addPair(Enums::SCOPE_CHECK_CONSENT_ACCEPTED, "ConsentAccepted"); addPair(Enums::SCOPE_CHECK_INVALID_SCOPES, "InvalidScopes"); addPair(Enums::SCOPE_CHECK_TIMEOUT, "Timeout"); addPair(Enums::SCOPE_CHECK_NO_USER_INPUT, "NoUserInput"); addPair(Enums::SCOPE_CHECK_BACKEND_ERROR, "BackendError"); addPair(Enums::SCOPE_CHECK_UNEXPECTED_ERROR, "UnexpectedError"); addPair(Enums::SCOPE_CHECK_INVALID_ARGUMENT, "InvalidArgument"); addPair(Enums::SCOPE_CHECK_CONSENT_DENIED, "ConsentDenied"); }

template<> Enums::TextDirection& Variant::convert<Enums::TextDirection>()
{
    return genericConvert<Enums::TextDirection>();
}

template<> Enums::HapticEffectType& Variant::convert<Enums::HapticEffectType>()
{
    return genericConvert<Enums::HapticEffectType>();
}

template<> Enums::ElasticBehavior& Variant::convert<Enums::ElasticBehavior>()
{
    return genericConvert<Enums::ElasticBehavior>();
}

template<> Enums::BorderMode& Variant::convert<Enums::BorderMode>()
{
    return genericConvert<Enums::BorderMode>();
}

template<> Enums::ScrollingDirection& Variant::convert<Enums::ScrollingDirection>()
{
    return genericConvert<Enums::ScrollingDirection>();
}
template<> Enums::ScrollBarInset& Variant::convert<Enums::ScrollBarInset>()
{ return genericConvert<Enums::ScrollBarInset>(); }
template<> Enums::VerticalScrollBarPosition& Variant::convert<Enums::VerticalScrollBarPosition>()
{ return genericConvert<Enums::VerticalScrollBarPosition>(); }

template<> Enums::VoiceChatDistanceAttenuationType& Variant::convert<Enums::VoiceChatDistanceAttenuationType>()
{ return genericConvert<Enums::VoiceChatDistanceAttenuationType>(); }
template<> Enums::RolloutState& Variant::convert<Enums::RolloutState>()
{ return genericConvert<Enums::RolloutState>(); }
template<> Enums::AudioApiRollout& Variant::convert<Enums::AudioApiRollout>()
{ return genericConvert<Enums::AudioApiRollout>(); }
template<> Enums::VoiceClientLeaveReasons& Variant::convert<Enums::VoiceClientLeaveReasons>()
{ return genericConvert<Enums::VoiceClientLeaveReasons>(); }
template<> Enums::TextInputType& Variant::convert<Enums::TextInputType>()
{ return genericConvert<Enums::TextInputType>(); }
template<> Enums::ChatRestrictionStatus& Variant::convert<Enums::ChatRestrictionStatus>()
{ return genericConvert<Enums::ChatRestrictionStatus>(); }
template<> Enums::ReturnKeyType& Variant::convert<Enums::ReturnKeyType>()
{ return genericConvert<Enums::ReturnKeyType>(); }
template<> Enums::AppShellActionType& Variant::convert<Enums::AppShellActionType>()
{ return genericConvert<Enums::AppShellActionType>(); }
template<> Enums::AppShellFeature& Variant::convert<Enums::AppShellFeature>()
{ return genericConvert<Enums::AppShellFeature>(); }
template<> Enums::ConnectionState& Variant::convert<Enums::ConnectionState>()
{ return genericConvert<Enums::ConnectionState>(); }
template<> Enums::AvatarChatServiceFeature& Variant::convert<Enums::AvatarChatServiceFeature>()
{ return genericConvert<Enums::AvatarChatServiceFeature>(); }
template<> Enums::DeviceFeatureType& Variant::convert<Enums::DeviceFeatureType>()
{ return genericConvert<Enums::DeviceFeatureType>(); }
template<> Enums::DeviceLevel& Variant::convert<Enums::DeviceLevel>()
{ return genericConvert<Enums::DeviceLevel>(); }
template<> Enums::AppLifecycleManagerState& Variant::convert<Enums::AppLifecycleManagerState>()
{ return genericConvert<Enums::AppLifecycleManagerState>(); }
template<> Enums::CaptureType& Variant::convert<Enums::CaptureType>()
{ return genericConvert<Enums::CaptureType>(); }
template<> Enums::CaptureGalleryPermission& Variant::convert<Enums::CaptureGalleryPermission>()
{ return genericConvert<Enums::CaptureGalleryPermission>(); }
template<> Enums::ReadCapturesFromGalleryResult& Variant::convert<Enums::ReadCapturesFromGalleryResult>()
{ return genericConvert<Enums::ReadCapturesFromGalleryResult>(); }
template<> Enums::VideoCaptureStartedResult& Variant::convert<Enums::VideoCaptureStartedResult>()
{ return genericConvert<Enums::VideoCaptureStartedResult>(); }
template<> Enums::VideoCaptureResult& Variant::convert<Enums::VideoCaptureResult>()
{ return genericConvert<Enums::VideoCaptureResult>(); }
template<> Enums::AssetType& Variant::convert<Enums::AssetType>() { return genericConvert<Enums::AssetType>(); }
template<> Enums::AccessoryType& Variant::convert<Enums::AccessoryType>() { return genericConvert<Enums::AccessoryType>(); }
#define RBX_TRACKER_VARIANT(Type) template<> Enums::Type& Variant::convert<Enums::Type>() { return genericConvert<Enums::Type>(); }
RBX_TRACKER_VARIANT(TrackerMode)
RBX_TRACKER_VARIANT(TrackerError)
RBX_TRACKER_VARIANT(TrackerFaceTrackingStatus)
RBX_TRACKER_VARIANT(TrackerPromptEvent)
RBX_TRACKER_VARIANT(TrackerExtrapolationFlagMode)
RBX_TRACKER_VARIANT(TrackerLodFlagMode)
RBX_TRACKER_VARIANT(TrackerLodValueMode)
RBX_TRACKER_VARIANT(HttpError)
RBX_TRACKER_VARIANT(ExperienceAuthScope)
RBX_TRACKER_VARIANT(ScopeCheckResult)
RBX_TRACKER_VARIANT(DisplaySize)
#undef RBX_TRACKER_VARIANT

} // namespace Reflection

template<> bool StringConverter<Enums::TextDirection>::convertToValue(
    const std::string& text, Enums::TextDirection& value)
{
    return Reflection::EnumDesc<Enums::TextDirection>::singleton()
        .convertToValue(text.c_str(), value);
}

template<> bool StringConverter<Enums::HapticEffectType>::convertToValue(
    const std::string& text, Enums::HapticEffectType& value)
{
    return Reflection::EnumDesc<Enums::HapticEffectType>::singleton()
        .convertToValue(text.c_str(), value);
}

template<> bool StringConverter<Enums::ElasticBehavior>::convertToValue(
    const std::string& text, Enums::ElasticBehavior& value)
{
    return Reflection::EnumDesc<Enums::ElasticBehavior>::singleton()
        .convertToValue(text.c_str(), value);
}

template<> bool StringConverter<Enums::BorderMode>::convertToValue(
    const std::string& text, Enums::BorderMode& value)
{
    return Reflection::EnumDesc<Enums::BorderMode>::singleton()
        .convertToValue(text.c_str(), value);
}

template<> bool StringConverter<Enums::ScrollingDirection>::convertToValue(
    const std::string& text, Enums::ScrollingDirection& value)
{
    return Reflection::EnumDesc<Enums::ScrollingDirection>::singleton()
        .convertToValue(text.c_str(), value);
}
template<> bool StringConverter<Enums::ScrollBarInset>::convertToValue(const std::string& text, Enums::ScrollBarInset& value)
{ return Reflection::EnumDesc<Enums::ScrollBarInset>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::VerticalScrollBarPosition>::convertToValue(const std::string& text, Enums::VerticalScrollBarPosition& value)
{ return Reflection::EnumDesc<Enums::VerticalScrollBarPosition>::singleton().convertToValue(text.c_str(), value); }

template<> bool StringConverter<Enums::VoiceChatDistanceAttenuationType>::convertToValue(
    const std::string& text, Enums::VoiceChatDistanceAttenuationType& value)
{ return Reflection::EnumDesc<Enums::VoiceChatDistanceAttenuationType>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::RolloutState>::convertToValue(
    const std::string& text, Enums::RolloutState& value)
{ return Reflection::EnumDesc<Enums::RolloutState>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::AudioApiRollout>::convertToValue(
    const std::string& text, Enums::AudioApiRollout& value)
{ return Reflection::EnumDesc<Enums::AudioApiRollout>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::VoiceClientLeaveReasons>::convertToValue(
    const std::string& text, Enums::VoiceClientLeaveReasons& value)
{ return Reflection::EnumDesc<Enums::VoiceClientLeaveReasons>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::TextInputType>::convertToValue(
    const std::string& text, Enums::TextInputType& value)
{ return Reflection::EnumDesc<Enums::TextInputType>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::ChatRestrictionStatus>::convertToValue(
    const std::string& text, Enums::ChatRestrictionStatus& value)
{ return Reflection::EnumDesc<Enums::ChatRestrictionStatus>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::ReturnKeyType>::convertToValue(
    const std::string& text, Enums::ReturnKeyType& value)
{ return Reflection::EnumDesc<Enums::ReturnKeyType>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::AppShellActionType>::convertToValue(
    const std::string& text, Enums::AppShellActionType& value)
{ return Reflection::EnumDesc<Enums::AppShellActionType>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::AppShellFeature>::convertToValue(
    const std::string& text, Enums::AppShellFeature& value)
{ return Reflection::EnumDesc<Enums::AppShellFeature>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::ConnectionState>::convertToValue(
    const std::string& text, Enums::ConnectionState& value)
{ return Reflection::EnumDesc<Enums::ConnectionState>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::AvatarChatServiceFeature>::convertToValue(
    const std::string& text, Enums::AvatarChatServiceFeature& value)
{ return Reflection::EnumDesc<Enums::AvatarChatServiceFeature>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::DeviceFeatureType>::convertToValue(
    const std::string& text, Enums::DeviceFeatureType& value)
{ return Reflection::EnumDesc<Enums::DeviceFeatureType>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::DeviceLevel>::convertToValue(
    const std::string& text, Enums::DeviceLevel& value)
{ return Reflection::EnumDesc<Enums::DeviceLevel>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::AppLifecycleManagerState>::convertToValue(
    const std::string& text, Enums::AppLifecycleManagerState& value)
{ return Reflection::EnumDesc<Enums::AppLifecycleManagerState>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::CaptureType>::convertToValue(const std::string& text, Enums::CaptureType& value)
{ return Reflection::EnumDesc<Enums::CaptureType>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::CaptureGalleryPermission>::convertToValue(const std::string& text, Enums::CaptureGalleryPermission& value)
{ return Reflection::EnumDesc<Enums::CaptureGalleryPermission>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::ReadCapturesFromGalleryResult>::convertToValue(const std::string& text, Enums::ReadCapturesFromGalleryResult& value)
{ return Reflection::EnumDesc<Enums::ReadCapturesFromGalleryResult>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::VideoCaptureStartedResult>::convertToValue(const std::string& text, Enums::VideoCaptureStartedResult& value)
{ return Reflection::EnumDesc<Enums::VideoCaptureStartedResult>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::VideoCaptureResult>::convertToValue(const std::string& text, Enums::VideoCaptureResult& value)
{ return Reflection::EnumDesc<Enums::VideoCaptureResult>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::AssetType>::convertToValue(const std::string& text, Enums::AssetType& value)
{ return Reflection::EnumDesc<Enums::AssetType>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::AccessoryType>::convertToValue(const std::string& text, Enums::AccessoryType& value)
{ return Reflection::EnumDesc<Enums::AccessoryType>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::BundleType>::convertToValue(const std::string& text, Enums::BundleType& value)
{ return Reflection::EnumDesc<Enums::BundleType>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::AvatarContextMenuOption>::convertToValue(const std::string& text, Enums::AvatarContextMenuOption& value)
{ return Reflection::EnumDesc<Enums::AvatarContextMenuOption>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<Enums::AvatarAssetType>::convertToValue(const std::string& text, Enums::AvatarAssetType& value)
{ return Reflection::EnumDesc<Enums::AvatarAssetType>::singleton().convertToValue(text.c_str(), value); }
#define RBX_TRACKER_STRING(Type) template<> bool StringConverter<Enums::Type>::convertToValue(const std::string& text, Enums::Type& value) { return Reflection::EnumDesc<Enums::Type>::singleton().convertToValue(text.c_str(), value); }
RBX_TRACKER_STRING(TrackerMode)
RBX_TRACKER_STRING(TrackerError)
RBX_TRACKER_STRING(TrackerFaceTrackingStatus)
RBX_TRACKER_STRING(TrackerPromptEvent)
RBX_TRACKER_STRING(TrackerExtrapolationFlagMode)
RBX_TRACKER_STRING(TrackerLodFlagMode)
RBX_TRACKER_STRING(TrackerLodValueMode)
RBX_TRACKER_STRING(HttpError)
RBX_TRACKER_STRING(ExperienceAuthScope)
RBX_TRACKER_STRING(ScopeCheckResult)
RBX_TRACKER_STRING(DisplaySize)
#undef RBX_TRACKER_STRING

} // namespace RBX
