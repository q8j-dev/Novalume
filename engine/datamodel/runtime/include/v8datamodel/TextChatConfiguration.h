#pragma once

#include "util/Font.h"
#include "util/G3DCore.h"
#include "util/KeyCode.h"
#include "v8datamodel/UIComponent.h"
#include "v8tree/Instance.h"

namespace RBX {

extern const char* const sTextChatConfigurations;
extern const char* const sChatInputBarConfiguration;
extern const char* const sChatWindowConfiguration;
extern const char* const sChatWindowMessageProperties;

class TextChatMessageProperties;
class TextBox;
class TextChannel;

class ChatWindowMessageProperties;

class TextChatConfigurations
    : public DescribedNonCreatable<TextChatConfigurations, Instance,
          sTextChatConfigurations>
{
public:
    TextChatConfigurations();
};

class ChatInputBarConfiguration
    : public DescribedNonCreatable<ChatInputBarConfiguration,
          TextChatConfigurations, sChatInputBarConfiguration>
{
public:
    ChatInputBarConfiguration();

    Vector2 getAbsolutePosition() const { return absolutePosition; }
    void setAbsolutePositionWrite(Vector2 value);
    Vector2 getAbsoluteSize() const { return absoluteSize; }
    void setAbsoluteSizeWrite(Vector2 value);
    bool getAutocompleteEnabled() const { return autocompleteEnabled; }
    void setAutocompleteEnabled(bool value);
    Color3 getBackgroundColor3() const { return backgroundColor3; }
    void setBackgroundColor3(Color3 value);
    double getBackgroundTransparency() const { return backgroundTransparency; }
    void setBackgroundTransparency(double value);
    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);
    Font getFontFace() const { return fontFace; }
    void setFontFace(Font value);
    bool getIsFocused() const { return isFocused; }
    void setIsFocusedWrite(bool value);
    KeyCode getKeyboardKeyCode() const { return keyboardKeyCode; }
    void setKeyboardKeyCode(KeyCode value);
    Color3 getPlaceholderColor3() const { return placeholderColor3; }
    void setPlaceholderColor3(Color3 value);
    TextChannel* getTargetTextChannel() const;
    void setTargetTextChannel(TextChannel* value);
    TextBox* getTextBox() const;
    void setTextBox(TextBox* value);
    Color3 getTextColor3() const { return textColor3; }
    void setTextColor3(Color3 value);
    long long getTextSize() const { return textSize; }
    void setTextSize(long long value);
    Color3 getTextStrokeColor3() const { return textStrokeColor3; }
    void setTextStrokeColor3(Color3 value);
    double getTextStrokeTransparency() const { return textStrokeTransparency; }
    void setTextStrokeTransparency(double value);

private:
    Vector2 absolutePosition;
    Vector2 absoluteSize;
    bool autocompleteEnabled;
    Color3 backgroundColor3;
    double backgroundTransparency;
    bool enabled;
    Font fontFace;
    bool isFocused;
    KeyCode keyboardKeyCode;
    Color3 placeholderColor3;
    weak_ptr<TextChannel> targetTextChannel;
    weak_ptr<TextBox> textBox;
    Color3 textColor3;
    long long textSize;
    Color3 textStrokeColor3;
    double textStrokeTransparency;
};

class ChatWindowConfiguration
    : public DescribedNonCreatable<ChatWindowConfiguration,
          TextChatConfigurations, sChatWindowConfiguration>
{
public:
    ChatWindowConfiguration();

    Vector2 getAbsolutePosition() const { return absolutePosition; }
    void setAbsolutePositionWrite(Vector2 value);
    Vector2 getAbsoluteSize() const { return absoluteSize; }
    void setAbsoluteSizeWrite(Vector2 value);
    Color3 getBackgroundColor3() const { return backgroundColor3; }
    void setBackgroundColor3(Color3 value);
    double getBackgroundTransparency() const { return backgroundTransparency; }
    void setBackgroundTransparency(double value);
    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);
    Font getFontFace() const { return fontFace; }
    void setFontFace(Font value);
    float getHeightScale() const { return heightScale; }
    void setHeightScale(float value);
    HorizontalAlignment getHorizontalAlignment() const { return horizontalAlignment; }
    void setHorizontalAlignment(HorizontalAlignment value);
    Color3 getTextColor3() const { return textColor3; }
    void setTextColor3(Color3 value);
    long long getTextSize() const { return textSize; }
    void setTextSize(long long value);
    Color3 getTextStrokeColor3() const { return textStrokeColor3; }
    void setTextStrokeColor3(Color3 value);
    double getTextStrokeTransparency() const { return textStrokeTransparency; }
    void setTextStrokeTransparency(double value);
    VerticalAlignment getVerticalAlignment() const { return verticalAlignment; }
    void setVerticalAlignment(VerticalAlignment value);
    float getWidthScale() const { return widthScale; }
    void setWidthScale(float value);

    shared_ptr<Instance> deriveNewMessageProperties();

private:
    Vector2 absolutePosition;
    Vector2 absoluteSize;
    Color3 backgroundColor3;
    double backgroundTransparency;
    bool enabled;
    Font fontFace;
    float heightScale;
    HorizontalAlignment horizontalAlignment;
    Color3 textColor3;
    long long textSize;
    Color3 textStrokeColor3;
    double textStrokeTransparency;
    VerticalAlignment verticalAlignment;
    float widthScale;
};

} // namespace RBX
