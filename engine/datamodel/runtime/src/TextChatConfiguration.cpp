#include "V8DataModel/TextChatConfiguration.h"

#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/TextBox.h"
#include "V8DataModel/TextChannel.h"

#include <algorithm>

namespace RBX {

const char* const sTextChatConfigurations = "TextChatConfigurations";
const char* const sChatInputBarConfiguration = "ChatInputBarConfiguration";
const char* const sChatWindowConfiguration = "ChatWindowConfiguration";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<ChatInputBarConfiguration, Vector2>
    propInputAbsolutePosition("AbsolutePosition", category_Data,
        &ChatInputBarConfiguration::getAbsolutePosition, NULL,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<ChatInputBarConfiguration, Vector2>
    propInputAbsolutePositionWrite("AbsolutePositionWrite", category_Data,
        &ChatInputBarConfiguration::getAbsolutePosition,
        &ChatInputBarConfiguration::setAbsolutePositionWrite,
        Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<ChatInputBarConfiguration, Vector2>
    propInputAbsoluteSize("AbsoluteSize", category_Data,
        &ChatInputBarConfiguration::getAbsoluteSize, NULL,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<ChatInputBarConfiguration, Vector2>
    propInputAbsoluteSizeWrite("AbsoluteSizeWrite", category_Data,
        &ChatInputBarConfiguration::getAbsoluteSize,
        &ChatInputBarConfiguration::setAbsoluteSizeWrite,
        Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<ChatInputBarConfiguration, bool>
    propInputAutocompleteEnabled("AutocompleteEnabled", category_Behavior,
        &ChatInputBarConfiguration::getAutocompleteEnabled,
        &ChatInputBarConfiguration::setAutocompleteEnabled);
static Reflection::PropDescriptor<ChatInputBarConfiguration, Color3>
    propInputBackgroundColor3("BackgroundColor3", category_Appearance,
        &ChatInputBarConfiguration::getBackgroundColor3,
        &ChatInputBarConfiguration::setBackgroundColor3);
static Reflection::PropDescriptor<ChatInputBarConfiguration, double>
    propInputBackgroundTransparency("BackgroundTransparency", category_Appearance,
        &ChatInputBarConfiguration::getBackgroundTransparency,
        &ChatInputBarConfiguration::setBackgroundTransparency);
static Reflection::PropDescriptor<ChatInputBarConfiguration, bool>
    propInputEnabled("Enabled", category_Behavior,
        &ChatInputBarConfiguration::getEnabled,
        &ChatInputBarConfiguration::setEnabled);
static Reflection::PropDescriptor<ChatInputBarConfiguration, Font>
    propInputFontFace("FontFace", category_Appearance,
        &ChatInputBarConfiguration::getFontFace,
        &ChatInputBarConfiguration::setFontFace);
static Reflection::PropDescriptor<ChatInputBarConfiguration, bool>
    propInputIsFocused("IsFocused", category_Data,
        &ChatInputBarConfiguration::getIsFocused, NULL,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<ChatInputBarConfiguration, bool>
    propInputIsFocusedWrite("IsFocusedWrite", category_Data,
        &ChatInputBarConfiguration::getIsFocused,
        &ChatInputBarConfiguration::setIsFocusedWrite,
        Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::EnumPropDescriptor<ChatInputBarConfiguration, KeyCode>
    propInputKeyboardKeyCode("KeyboardKeyCode", category_Behavior,
        &ChatInputBarConfiguration::getKeyboardKeyCode,
        &ChatInputBarConfiguration::setKeyboardKeyCode);
static Reflection::PropDescriptor<ChatInputBarConfiguration, Color3>
    propInputPlaceholderColor3("PlaceholderColor3", category_Appearance,
        &ChatInputBarConfiguration::getPlaceholderColor3,
        &ChatInputBarConfiguration::setPlaceholderColor3);
static Reflection::RefPropDescriptor<ChatInputBarConfiguration, TextChannel>
    propInputTargetTextChannel("TargetTextChannel", category_Data,
        &ChatInputBarConfiguration::getTargetTextChannel,
        &ChatInputBarConfiguration::setTargetTextChannel);
static Reflection::RefPropDescriptor<ChatInputBarConfiguration, TextBox>
    propInputTextBox("TextBox", category_Data,
        &ChatInputBarConfiguration::getTextBox,
        &ChatInputBarConfiguration::setTextBox);
static Reflection::PropDescriptor<ChatInputBarConfiguration, Color3>
    propInputTextColor3("TextColor3", category_Appearance,
        &ChatInputBarConfiguration::getTextColor3,
        &ChatInputBarConfiguration::setTextColor3);
static Reflection::PropDescriptor<ChatInputBarConfiguration, long long>
    propInputTextSize("TextSize", category_Appearance,
        &ChatInputBarConfiguration::getTextSize,
        &ChatInputBarConfiguration::setTextSize);
static Reflection::PropDescriptor<ChatInputBarConfiguration, Color3>
    propInputTextStrokeColor3("TextStrokeColor3", category_Appearance,
        &ChatInputBarConfiguration::getTextStrokeColor3,
        &ChatInputBarConfiguration::setTextStrokeColor3);
static Reflection::PropDescriptor<ChatInputBarConfiguration, double>
    propInputTextStrokeTransparency("TextStrokeTransparency", category_Appearance,
        &ChatInputBarConfiguration::getTextStrokeTransparency,
        &ChatInputBarConfiguration::setTextStrokeTransparency);
static Reflection::PropDescriptor<ChatWindowConfiguration, Vector2>
    propAbsolutePosition("AbsolutePosition", category_Data,
        &ChatWindowConfiguration::getAbsolutePosition, NULL,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<ChatWindowConfiguration, Vector2>
    propAbsolutePositionWrite("AbsolutePositionWrite", category_Data,
        &ChatWindowConfiguration::getAbsolutePosition,
        &ChatWindowConfiguration::setAbsolutePositionWrite,
        Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<ChatWindowConfiguration, Vector2>
    propAbsoluteSize("AbsoluteSize", category_Data,
        &ChatWindowConfiguration::getAbsoluteSize, NULL,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<ChatWindowConfiguration, Vector2>
    propAbsoluteSizeWrite("AbsoluteSizeWrite", category_Data,
        &ChatWindowConfiguration::getAbsoluteSize,
        &ChatWindowConfiguration::setAbsoluteSizeWrite,
        Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<ChatWindowConfiguration, Color3>
    propWindowBackgroundColor3("BackgroundColor3", category_Appearance,
        &ChatWindowConfiguration::getBackgroundColor3,
        &ChatWindowConfiguration::setBackgroundColor3);
static Reflection::PropDescriptor<ChatWindowConfiguration, double>
    propWindowBackgroundTransparency("BackgroundTransparency", category_Appearance,
        &ChatWindowConfiguration::getBackgroundTransparency,
        &ChatWindowConfiguration::setBackgroundTransparency);
static Reflection::PropDescriptor<ChatWindowConfiguration, bool>
    propWindowEnabled("Enabled", category_Behavior,
        &ChatWindowConfiguration::getEnabled,
        &ChatWindowConfiguration::setEnabled);
static Reflection::PropDescriptor<ChatWindowConfiguration, Font>
    propWindowFontFace("FontFace", category_Appearance,
        &ChatWindowConfiguration::getFontFace,
        &ChatWindowConfiguration::setFontFace);
static Reflection::PropDescriptor<ChatWindowConfiguration, float>
    propWindowHeightScale("HeightScale", category_Behavior,
        &ChatWindowConfiguration::getHeightScale,
        &ChatWindowConfiguration::setHeightScale);
static Reflection::EnumPropDescriptor<ChatWindowConfiguration, HorizontalAlignment>
    propWindowHorizontalAlignment("HorizontalAlignment", category_Data,
        &ChatWindowConfiguration::getHorizontalAlignment,
        &ChatWindowConfiguration::setHorizontalAlignment);
static Reflection::PropDescriptor<ChatWindowConfiguration, Color3>
    propWindowTextColor3("TextColor3", category_Appearance,
        &ChatWindowConfiguration::getTextColor3,
        &ChatWindowConfiguration::setTextColor3);
static Reflection::PropDescriptor<ChatWindowConfiguration, long long>
    propWindowTextSize("TextSize", category_Appearance,
        &ChatWindowConfiguration::getTextSize,
        &ChatWindowConfiguration::setTextSize);
static Reflection::PropDescriptor<ChatWindowConfiguration, Color3>
    propWindowTextStrokeColor3("TextStrokeColor3", category_Appearance,
        &ChatWindowConfiguration::getTextStrokeColor3,
        &ChatWindowConfiguration::setTextStrokeColor3);
static Reflection::PropDescriptor<ChatWindowConfiguration, double>
    propWindowTextStrokeTransparency("TextStrokeTransparency", category_Appearance,
        &ChatWindowConfiguration::getTextStrokeTransparency,
        &ChatWindowConfiguration::setTextStrokeTransparency);
static Reflection::EnumPropDescriptor<ChatWindowConfiguration, VerticalAlignment>
    propWindowVerticalAlignment("VerticalAlignment", category_Data,
        &ChatWindowConfiguration::getVerticalAlignment,
        &ChatWindowConfiguration::setVerticalAlignment);
static Reflection::PropDescriptor<ChatWindowConfiguration, float>
    propWindowWidthScale("WidthScale", category_Behavior,
        &ChatWindowConfiguration::getWidthScale,
        &ChatWindowConfiguration::setWidthScale);
static Reflection::BoundFuncDesc<ChatWindowConfiguration, shared_ptr<Instance>()>
    funcDeriveNewMessageProperties(
        &ChatWindowConfiguration::deriveNewMessageProperties,
        "DeriveNewMessageProperties", Security::None);
REFLECTION_END();

TextChatConfigurations::TextChatConfigurations()
{
    setName(sTextChatConfigurations);
    setRobloxLocked(true);
}

ChatInputBarConfiguration::ChatInputBarConfiguration()
    : absolutePosition(Vector2::zero())
    , absoluteSize(Vector2::zero())
    , autocompleteEnabled(true)
    , backgroundColor3(Color3(25.0f / 255.0f, 27.0f / 255.0f,
          29.0f / 255.0f))
    , backgroundTransparency(0.2)
    , enabled(true)
    , fontFace(Font("rbxasset://fonts/families/BuilderSans.json",
          FONT_WEIGHT_MEDIUM))
    , isFocused(false)
    , keyboardKeyCode(SDLK_SLASH)
    , placeholderColor3(Color3(178.0f / 255.0f, 178.0f / 255.0f,
          178.0f / 255.0f))
    , textColor3(Color3::white())
    , textSize(18)
    , textStrokeColor3(Color3::black())
    , textStrokeTransparency(1.0)
{
    setName(sChatInputBarConfiguration);
    setRobloxLocked(true);
}

#define RBX_CHAT_INPUT_SETTER(Type, Name, member, descriptor) \
    void ChatInputBarConfiguration::set##Name(Type value) \
    { \
        if (member == value) \
            return; \
        member = value; \
        raisePropertyChanged(descriptor); \
    }

RBX_CHAT_INPUT_SETTER(Vector2, AbsolutePositionWrite, absolutePosition,
    propInputAbsolutePosition)
RBX_CHAT_INPUT_SETTER(Vector2, AbsoluteSizeWrite, absoluteSize,
    propInputAbsoluteSize)
RBX_CHAT_INPUT_SETTER(bool, AutocompleteEnabled, autocompleteEnabled,
    propInputAutocompleteEnabled)
RBX_CHAT_INPUT_SETTER(Color3, BackgroundColor3, backgroundColor3,
    propInputBackgroundColor3)
RBX_CHAT_INPUT_SETTER(bool, Enabled, enabled, propInputEnabled)
RBX_CHAT_INPUT_SETTER(Font, FontFace, fontFace, propInputFontFace)
RBX_CHAT_INPUT_SETTER(bool, IsFocusedWrite, isFocused, propInputIsFocused)
RBX_CHAT_INPUT_SETTER(KeyCode, KeyboardKeyCode, keyboardKeyCode,
    propInputKeyboardKeyCode)
RBX_CHAT_INPUT_SETTER(Color3, PlaceholderColor3, placeholderColor3,
    propInputPlaceholderColor3)
RBX_CHAT_INPUT_SETTER(Color3, TextColor3, textColor3, propInputTextColor3)
RBX_CHAT_INPUT_SETTER(Color3, TextStrokeColor3, textStrokeColor3,
    propInputTextStrokeColor3)

#undef RBX_CHAT_INPUT_SETTER

void ChatInputBarConfiguration::setBackgroundTransparency(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    if (backgroundTransparency == value)
        return;
    backgroundTransparency = value;
    raisePropertyChanged(propInputBackgroundTransparency);
}

void ChatInputBarConfiguration::setTextSize(long long value)
{
    value = std::clamp(value, 1LL, 100LL);
    if (textSize == value)
        return;
    textSize = value;
    raisePropertyChanged(propInputTextSize);
}

void ChatInputBarConfiguration::setTextStrokeTransparency(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    if (textStrokeTransparency == value)
        return;
    textStrokeTransparency = value;
    raisePropertyChanged(propInputTextStrokeTransparency);
}

TextChannel* ChatInputBarConfiguration::getTargetTextChannel() const
{
    return targetTextChannel.lock().get();
}

void ChatInputBarConfiguration::setTargetTextChannel(TextChannel* value)
{
    if (getTargetTextChannel() == value)
        return;
    targetTextChannel = shared_from(value);
    raisePropertyChanged(propInputTargetTextChannel);
}

TextBox* ChatInputBarConfiguration::getTextBox() const
{
    return textBox.lock().get();
}

void ChatInputBarConfiguration::setTextBox(TextBox* value)
{
    if (getTextBox() == value)
        return;
    // PlayerGui in this API contract denotes a live player-GUI root. CoreGui
    // and the per-player PlayerGui both derive from BasePlayerGui; the supplied
    // ExperienceChat package owns its internal TextBox under CoreGui.
    if (value && !value->findFirstAncestorOfType<BasePlayerGui>())
        throw std::runtime_error(
            "ChatInputBarConfiguration.TextBox should be a descendant of PlayerGui.");
    textBox = shared_from(value);
    raisePropertyChanged(propInputTextBox);
}

ChatWindowConfiguration::ChatWindowConfiguration()
    : absolutePosition(Vector2::zero())
    , absoluteSize(Vector2::zero())
    , backgroundColor3(Color3(25.0f / 255.0f, 27.0f / 255.0f,
          29.0f / 255.0f))
    , backgroundTransparency(0.3)
    , enabled(true)
    , fontFace(Font("rbxasset://fonts/families/BuilderSans.json",
          FONT_WEIGHT_MEDIUM))
    , heightScale(1.0f)
    , horizontalAlignment(HORIZONTAL_ALIGNMENT_LEFT)
    , textColor3(Color3::white())
    , textSize(14)
    , textStrokeColor3(Color3::black())
    , textStrokeTransparency(0.5)
    , verticalAlignment(VERTICAL_ALIGNMENT_TOP)
    , widthScale(1.0f)
{
    setName(sChatWindowConfiguration);
    setRobloxLocked(true);
}

#define RBX_CHAT_WINDOW_SETTER(Type, Name, member, descriptor) \
    void ChatWindowConfiguration::set##Name(Type value) \
    { \
        if (member == value) \
            return; \
        member = value; \
        raisePropertyChanged(descriptor); \
    }

RBX_CHAT_WINDOW_SETTER(Vector2, AbsolutePositionWrite, absolutePosition,
    propAbsolutePosition)
RBX_CHAT_WINDOW_SETTER(Vector2, AbsoluteSizeWrite, absoluteSize, propAbsoluteSize)
RBX_CHAT_WINDOW_SETTER(Color3, BackgroundColor3, backgroundColor3,
    propWindowBackgroundColor3)
RBX_CHAT_WINDOW_SETTER(bool, Enabled, enabled, propWindowEnabled)
RBX_CHAT_WINDOW_SETTER(Font, FontFace, fontFace, propWindowFontFace)
RBX_CHAT_WINDOW_SETTER(HorizontalAlignment, HorizontalAlignment,
    horizontalAlignment, propWindowHorizontalAlignment)
RBX_CHAT_WINDOW_SETTER(Color3, TextColor3, textColor3, propWindowTextColor3)
RBX_CHAT_WINDOW_SETTER(Color3, TextStrokeColor3, textStrokeColor3,
    propWindowTextStrokeColor3)
RBX_CHAT_WINDOW_SETTER(VerticalAlignment, VerticalAlignment,
    verticalAlignment, propWindowVerticalAlignment)

#undef RBX_CHAT_WINDOW_SETTER

void ChatWindowConfiguration::setBackgroundTransparency(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    if (backgroundTransparency == value)
        return;
    backgroundTransparency = value;
    raisePropertyChanged(propWindowBackgroundTransparency);
}

void ChatWindowConfiguration::setHeightScale(float value)
{
    value = std::clamp(value, 0.5f, 2.0f);
    if (heightScale == value)
        return;
    heightScale = value;
    raisePropertyChanged(propWindowHeightScale);
}

void ChatWindowConfiguration::setTextSize(long long value)
{
    value = std::clamp(value, 1LL, 100LL);
    if (textSize == value)
        return;
    textSize = value;
    raisePropertyChanged(propWindowTextSize);
}

void ChatWindowConfiguration::setTextStrokeTransparency(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    if (textStrokeTransparency == value)
        return;
    textStrokeTransparency = value;
    raisePropertyChanged(propWindowTextStrokeTransparency);
}

void ChatWindowConfiguration::setWidthScale(float value)
{
    value = std::clamp(value, 0.5f, 2.0f);
    if (widthScale == value)
        return;
    widthScale = value;
    raisePropertyChanged(propWindowWidthScale);
}

shared_ptr<Instance> ChatWindowConfiguration::deriveNewMessageProperties()
{
    shared_ptr<ChatWindowMessageProperties> properties(
        new ChatWindowMessageProperties());
    properties->setFontFace(fontFace);
    properties->setTextColor3(textColor3);
    properties->setTextSize(static_cast<int>(textSize));
    properties->setTextStrokeColor3(textStrokeColor3);
    properties->setTextStrokeTransparency(textStrokeTransparency);

    shared_ptr<ChatWindowMessageProperties> prefixProperties(
        new ChatWindowMessageProperties());
    prefixProperties->setFontFace(fontFace);
    prefixProperties->setTextColor3(textColor3);
    prefixProperties->setTextSize(static_cast<int>(textSize));
    prefixProperties->setTextStrokeColor3(textStrokeColor3);
    prefixProperties->setTextStrokeTransparency(textStrokeTransparency);
    properties->setPrefixTextProperties(prefixProperties.get());
    return properties;
}

} // namespace RBX
