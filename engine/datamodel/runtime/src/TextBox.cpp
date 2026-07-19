
#include "V8DataModel/TextBox.h"
#include "V8Datamodel/UserInputService.h"
#include "v8datamodel/ScrollingFrame.h"
#include "v8datamodel/GuiService.h"
#include "Util/UserInputBase.h"
#include "Util/RunStateOwner.h"
#include "GfxBase/Typesetter.h"
#include "Humanoid/Humanoid.h"
#include "Network/Players.h"
#include "v8datamodel/Workspace.h"

DYNAMIC_FASTFLAGVARIABLE(DisplayTextBoxTextWhileTypingMobile, false)
DYNAMIC_FASTFLAGVARIABLE(PasteWithCapsLockOn, false)
DYNAMIC_FASTFLAGVARIABLE(TextBoxIsFocusedEnabled, false)

#define CURSOR_BLINK_RATE_SECONDS 0.3f

//todo: when SDL is on, remove all repeat key junk

namespace RBX
{

    REFLECTION_BEGIN();
	const char* const sTextBox = "TextBox";
	static const Reflection::PropDescriptor<TextBox, bool> prop_MultiLine("MultiLine", category_Data, &TextBox::getMultiLine, &TextBox::setMultiLine);
	static const Reflection::PropDescriptor<TextBox, bool> prop_ClearTextOnFocus("ClearTextOnFocus", category_Data, &TextBox::getClearTextOnFocus, &TextBox::setClearTextOnFocus);
	static const Reflection::PropDescriptor<TextBox, bool> prop_OverlayNativeInput(
		"OverlayNativeInput", category_Data,
		&TextBox::getOverlayNativeInput, &TextBox::setOverlayNativeInput,
		Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
	static const Reflection::EnumPropDescriptor<TextBox, Enums::ReturnKeyType> prop_ReturnKeyType(
		"ReturnKeyType", category_Data, &TextBox::getReturnKeyType, &TextBox::setReturnKeyType,
		Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
	static const Reflection::PropDescriptor<TextBox, std::string> prop_PlaceholderText(
		"PlaceholderText", category_Text, &TextBox::getPlaceholderText, &TextBox::setPlaceholderText);
	static const Reflection::PropDescriptor<TextBox, Color3> prop_PlaceholderColor3(
		"PlaceholderColor3", category_Text, &TextBox::getPlaceholderColor3, &TextBox::setPlaceholderColor3);
	static const Reflection::PropDescriptor<TextBox, int> prop_CursorPosition(
		"CursorPosition", category_Text, &TextBox::getCursorPosition, &TextBox::setCursorPosition);
	static const Reflection::PropDescriptor<TextBox, int> prop_SelectionStart(
		"SelectionStart", category_Text, &TextBox::getSelectionStart, &TextBox::setSelectionStart);
    
	static const Reflection::BoundFuncDesc<TextBox, void()> func_CaptureFocus(&TextBox::captureFocus, "CaptureFocus", Security::None);
	static const Reflection::BoundFuncDesc<TextBox, void()> func_ReleaseFocus(&TextBox::releaseFocusLua, "ReleaseFocus", Security::None);
	static const Reflection::BoundFuncDesc<TextBox, bool()> func_GetFocus(&TextBox::getFocused, "IsFocused", Security::None);

	static const Reflection::EventDesc<TextBox, void(bool, const shared_ptr<Instance>)> event_FocusLost(&TextBox::focusLostSignal, "FocusLost", "enterPressed", "inputThatCausedFocusLoss");
	static const Reflection::EventDesc<TextBox, void()> event_FocusGained(&TextBox::focusGainedSignal, "Focused");
	static const Reflection::EventDesc<TextBox, void()> event_ReturnPressedFromOnScreenKeyboard(
		&TextBox::returnPressedFromOnScreenKeyboardSignal,
		"ReturnPressedFromOnScreenKeyboard");
	
	IMPLEMENT_GUI_TEXT_MIXIN(TextBox);
    REFLECTION_END();
    
	TextBox::TextBox()
    : DescribedCreatable<TextBox,GuiObject,sTextBox>("TextBox", true)
    , GuiTextMixin("TextBox",BrickColor::brickBlack().color3())
    , iAmFocus(false)
    , showingCursor(false)
    , multiLine(false)
    , shouldCaptureFocus(false)
    , cursorPos(0)
	, selectionStartPos(-1)
    , clearTextOnFocus(true)
	, overlayNativeInput(false)
	, shouldFocusFromInput(true)
	, returnKeyType(Enums::RETURN_KEY_DEFAULT)
	, placeholderText()
	, placeholderColor3(178.0f / 255.0f, 178.0f / 255.0f, 178.0f / 255.0f)
	{
		selectable = true;
		setGuiQueue(GUIQUEUE_TEXT);
	}
	void TextBox::setPlaceholderText(const std::string& value)
	{
		std::string bounded = value;
		if (bounded.size() > ContentFilter::MAX_CONTENT_FILTER_SIZE)
			bounded.resize(ContentFilter::MAX_CONTENT_FILTER_SIZE);

		if (placeholderText != bounded &&
			(!ProfanityFilter::ContainsProfanity(bounded) || getRobloxLocked()))
		{
			placeholderText = bounded;
			raisePropertyChanged(prop_PlaceholderText);
		}
	}
	void TextBox::setPlaceholderColor3(Color3 value)
	{
		if (placeholderColor3 != value)
		{
			placeholderColor3 = value;
			raisePropertyChanged(prop_PlaceholderColor3);
		}
	}

	namespace
	{
		bool isUtf8Continuation(unsigned char value)
		{
			return (value & 0xc0U) == 0x80U;
		}

		int utf8PositionFromByteOffset(const std::string& text, int byteOffset)
		{
			const int bounded = std::clamp(byteOffset, 0, static_cast<int>(text.size()));
			int position = 1;
			for (int index = 0; index < bounded; ++index)
				if (!isUtf8Continuation(static_cast<unsigned char>(text[index])))
					++position;
			return position;
		}

		int byteOffsetFromUtf8Position(const std::string& text, int position)
		{
			if (position <= 1)
				return 0;
			const int targetCharacterCount = position - 1;
			int characterCount = 0;
			for (int index = 0; index < static_cast<int>(text.size()); ++index)
				if (!isUtf8Continuation(static_cast<unsigned char>(text[index])))
				{
					if (characterCount == targetCharacterCount)
						return index;
					++characterCount;
				}
			return static_cast<int>(text.size());
		}

		int previousUtf8Boundary(const std::string& text, int byteOffset)
		{
			int result = std::max(0, byteOffset - 1);
			while (result > 0 &&
				isUtf8Continuation(static_cast<unsigned char>(text[result])))
				--result;
			return result;
		}

		int nextUtf8Boundary(const std::string& text, int byteOffset)
		{
			int result = std::min(static_cast<int>(text.size()), byteOffset + 1);
			while (result < static_cast<int>(text.size()) &&
				isUtf8Continuation(static_cast<unsigned char>(text[result])))
				++result;
			return result;
		}
	}

	int TextBox::getCursorPosition() const
	{
		return iAmFocus ? utf8PositionFromByteOffset(bufferedText, cursorPos) : -1;
	}

	void TextBox::setCursorPosition(int value)
	{
		if (!iAmFocus)
			return;
		const int previous = getCursorPosition();
		cursorPos = byteOffsetFromUtf8Position(bufferedText, std::max(1, value));
		if (previous != getCursorPosition())
			raisePropertyChanged(prop_CursorPosition);
	}

	int TextBox::getSelectionStart() const
	{
		return iAmFocus && selectionStartPos >= 0
			? utf8PositionFromByteOffset(bufferedText, selectionStartPos) : -1;
	}

	void TextBox::setSelectionStart(int value)
	{
		if (!iAmFocus)
			return;
		const int previous = getSelectionStart();
		selectionStartPos = value < 0 ? -1
			: byteOffsetFromUtf8Position(bufferedText, std::max(1, value));
		if (previous != getSelectionStart())
			raisePropertyChanged(prop_SelectionStart);
	}
	void TextBox::setReturnKeyType(Enums::ReturnKeyType value)
	{
		if (returnKeyType != value)
		{
			returnKeyType = value;
			raisePropertyChanged(prop_ReturnKeyType);
		}
	}
	void TextBox::setMultiLine(bool value)
	{
		if(value != multiLine)
		{
			multiLine = value;
			raisePropertyChanged(prop_MultiLine);
		}
	}
	void TextBox::setClearTextOnFocus(bool value)
	{
		if(value != clearTextOnFocus)
		{
			clearTextOnFocus = value;
			raisePropertyChanged(prop_ClearTextOnFocus);
		}
	}
	void TextBox::setOverlayNativeInput(bool value)
	{
		if (value != overlayNativeInput)
		{
			overlayNativeInput = value;
			raisePropertyChanged(prop_OverlayNativeInput);
		}
	}
	void TextBox::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
	{
        if(oldProvider)
		{
			bufferedText = "";
            textBoxFinishedEditingConnection.disconnect();
		}
            
		Super::onServiceProvider(oldProvider, newProvider);
		onServiceProviderHeartbeatInstance(oldProvider, newProvider);
        
        if(newProvider)
		{
			bufferedText = getText();

            if( RBX::UserInputService* userInputService = RBX::ServiceProvider::create<RBX::UserInputService>(newProvider) )
                textBoxFinishedEditingConnection = userInputService->textBoxFinishedEditing.connect(bind(&TextBox::externalReleaseFocus,this,_1,_2, _3));
		}
	}
	
	void TextBox::onAncestorChanged(const AncestorChanged& event)
	{
		Super::onAncestorChanged(event);
		releaseFocus(false, shared_ptr<InputObject>(), event.oldParent );
	}

    GuiResponse TextBox::process(const shared_ptr<InputObject>& event)
    {
		if( this->isDescendantOf( ServiceProvider::find<Workspace>(this) ) ) // textbox on SurfaceGUIs under workspace won't accept user input
			return GuiResponse();

		GuiResponse answer = Super::process(event);
        
        if (event->getUserInputType() == InputObject::TYPE_FOCUS &&
            event->getUserInputState() == InputObject::INPUT_STATE_END)
        {
            releaseFocus(false, event);
        }
        
        return answer;
    }

	GuiResponse TextBox::preProcessMouseEvent(const shared_ptr<InputObject>& event)
	{
		Rect rect(getClippedRect());
		bool mouseOver = rect.pointInRect(event->get2DPosition());
		if (event->isLeftMouseDownEvent() && shouldFocusFromInput && mouseOver)
		{
			gainFocus(event);
		}
		else if (event->isLeftMouseDownEvent() && !mouseOver && iAmFocus)
		{
			releaseFocus(false, event);
			return GuiResponse::notSunk();
		}

		GuiResponse answer = Super::processMouseEventInternal(event, iAmFocus);			//We need to call our classParent to deal with the state table
        
		return iAmFocus ? GuiResponse::sunkWithTarget(this) : answer;
	}

	GuiResponse TextBox::preProcess(const shared_ptr<InputObject>& event)
    {
		if( this->isDescendantOf( ServiceProvider::find<Workspace>(this) ) ) // textbox on SurfaceGUIs under workspace won't accept user input
			return GuiResponse();

		if (event->getUserInputType() == InputObject::TYPE_FOCUS &&
            event->getUserInputState() == InputObject::INPUT_STATE_END)
        {
            releaseFocus(false, event);
        }

		if (event->isMouseEvent())
		{
			return preProcessMouseEvent(event);
		}
		else if (event->isKeyEvent()) 
		{
			return processKeyEvent(event);
		}
		else if (event->isTextInputEvent())
		{
			return processTextInputEvent(event);
		}
		else if (event->isTouchEvent())
		{
			return processTouchEvent(event);
		}
		else if (event->isGamepadEvent())
		{
			return processGamepadEvent(event);
		}

		return GuiResponse::notSunk();
    }
    
	GuiResponse TextBox::processMouseEvent(const shared_ptr<InputObject>& event)
	{
		Rect rect(getClippedRect());
		bool mouseOver = rect.pointInRect(event->get2DPosition());
		if (event->isLeftMouseDownEvent() && shouldFocusFromInput && mouseOver)
		{
			gainFocus(event);
		}
		else if (event->isLeftMouseDownEvent() && !mouseOver && iAmFocus)
		{
			releaseFocus(false, event);
			return GuiResponse::notSunk();
		}

		GuiResponse answer = Super::processMouseEvent(event);			//We need to call our classParent to deal with the state table
        
		// used(this) will set a target and sink this event at the PlayerGui processing level - no other GuiObjects will see it.
		return iAmFocus ? GuiResponse::sunkWithTarget(this) : answer;
	}

	GuiResponse TextBox::processTouchEvent(const shared_ptr<InputObject>& event)
	{
		shouldFocusFromInput = true;

		if ( event->isTouchEvent() && mouseIsOver(event->get2DPosition()) && getActive() )
		{
			if( RBX::ScrollingFrame* scrollFrame = findFirstAncestorOfType<RBX::ScrollingFrame>())
			{
				bool processedInput = scrollFrame->processInputFromDescendant(event);
				shouldFocusFromInput = !scrollFrame->isTouchScrolling();

				if (processedInput)
				{
					return GuiResponse::sunk();
				}
			}
		}

		return Super::processTouchEvent(event);
	}

	void TextBox::selectionToggled(const shared_ptr<InputObject>& event)
	{
		iAmFocus ? releaseFocus(false, event) : captureFocus();
	}

	GuiResponse TextBox::processGamepadEvent(const shared_ptr<InputObject>& event)
	{
		// todo: don't hard code selection button? maybe this is ok
		if (event->getKeyCode() == SDLK_GAMEPAD_BUTTONA && event->getUserInputState() == InputObject::INPUT_STATE_BEGIN)
		{
			if (isSelectedObject())
			{
				selectionToggled(event);
				return GuiResponse::sunk();
			}
		}

		return Super::processGamepadEvent(event);
	}

    
	int TextBox::getCursorPos(RBX::Vector2 mousePos)
	{
		int pos = getPosInString(mousePos);
		if(pos < 0) // cursor did not click on any text, lets determine to put at end or beginning of text field
		{
			Rect2D rect = getRect2D();
			if(mousePos.x < rect.center().x)
				pos = 0;
			else
				pos = bufferedText.length();
		}
		return pos;
	}

	void TextBox::setBufferedText(std::string value, int newCursorPosition)
	{
		if (DFFlag::DisplayTextBoxTextWhileTypingMobile)
		{
			bufferedText = value;
			cursorPos = newCursorPosition;
			setText(bufferedText);
		}
	}
    
	void TextBox::gainFocus(const shared_ptr<InputObject>& event)
	{
		cursorPos = bufferedText.length();
		selectionStartPos = -1;
		iAmFocus = true;
		showingCursor = false;
        
		if(clearTextOnFocus)
		{
			bufferedText = "";
			cursorPos = 0;
		}
		else if (event && event->isLeftMouseUpEvent())
		{
			setText(bufferedText);
			cursorPos = getCursorPos(event->get2DPosition());
		}
        
		lastSwap = Time::nowFast();
		repeatKeyState.state = RepeatKeyState::STATE_NONE;
        
		ModelInstance* localCharacter = Network::Players::findLocalCharacter(this);
		Humanoid* localHumanoid = Humanoid::modelIsCharacter(localCharacter);
		if (localHumanoid)
			localHumanoid->setTyping(true);
        
		if(DataModel* dm = DataModel::get(this))
		{
			dm->setGuiTargetInstance(shared_from(this));
			dm->setSuppressNavKeys(true); // suppress the nav keys
		}

		focusGainedSignal();
		raisePropertyChanged(prop_CursorPosition);
		raisePropertyChanged(prop_SelectionStart);
        
        if( RBX::UserInputService* userInputService = RBX::ServiceProvider::create<RBX::UserInputService>(this) )
            userInputService->textBoxGainFocus(shared_from(this));
	}
    
	void TextBox::captureFocus()
	{
		cursorPos = bufferedText.length();
		selectionStartPos = -1;
		iAmFocus = true;
		showingCursor = false;

		if(clearTextOnFocus)
		{
			bufferedText = "";
			cursorPos = 0;
		}
		
		shouldCaptureFocus = true;
        
		gainFocus(shared_ptr<RBX::InputObject>());
	}


	void TextBox::onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
	{
		if (descriptor == prop_Text && strcmp(getText().c_str(), bufferedText.c_str()))
		{
            bufferedText = getText();
		}

		Super::onPropertyChanged(descriptor);
	}


    void TextBox::externalReleaseFocus(const char* externalReleaseText, bool enterPressed, const shared_ptr<InputObject>& inputThatCausedFocusLoss)
    {
        if(!iAmFocus)
            return;
        
        bufferedText = std::string(externalReleaseText);
        setText(bufferedText);
        
        ModelInstance* localCharacter = Network::Players::findLocalCharacter(this);
		Humanoid* localHumanoid = Humanoid::modelIsCharacter(localCharacter);
		if (localHumanoid)
			localHumanoid->setTyping(false);
        
        iAmFocus = false;
		selectionStartPos = -1;
		showingCursor = false;
		repeatKeyState.state = RepeatKeyState::STATE_NONE;

		// externalReleaseFocus is the completion path used by the platform's
		// on-screen keyboard. A submitted return has a distinct event from
		// FocusLost in the current engine API; physical keyboard submission
		// continues to use only FocusLost.
		if (enterPressed)
			returnPressedFromOnScreenKeyboardSignal();
        
        setFocusLost(enterPressed, inputThatCausedFocusLoss);
		raisePropertyChanged(prop_CursorPosition);
		raisePropertyChanged(prop_SelectionStart);
	}	
	
	void TextBox::releaseFocusLua()
	{
		releaseFocus(false, shared_ptr<InputObject>());
	}

	bool TextBox::getFocused()
	{
		if (!DFFlag::TextBoxIsFocusedEnabled)
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR, "TextBox:IsFocused() is not yet enabled!");
			return false;
		}
		return iAmFocus;
	}
    
	void TextBox::releaseFocus(bool enterPressed, const shared_ptr<InputObject>& inputThatCausedFocusLoss, Instance* contextLocalCharacter)
	{
		if(!iAmFocus)
			return;
        
		ModelInstance* localCharacter = Network::Players::findLocalCharacter( contextLocalCharacter ? contextLocalCharacter : this);
		Humanoid* localHumanoid = Humanoid::modelIsCharacter(localCharacter);
		if (localHumanoid)
			localHumanoid->setTyping(false);
        
		setText(bufferedText);
		iAmFocus = false;
		selectionStartPos = -1;
		showingCursor = false;
		repeatKeyState.state = RepeatKeyState::STATE_NONE;
        
		if(DataModel* dm = DataModel::get(this))
			dm->setSuppressNavKeys(false); // give nav back
        
        setFocusLost(enterPressed, inputThatCausedFocusLoss);
		raisePropertyChanged(prop_CursorPosition);
		raisePropertyChanged(prop_SelectionStart);
	}
    
    void TextBox::setFocusLost(bool enterPressed, const shared_ptr<InputObject>& inputThatCausedFocusLoss)
    {
        focusLostSignal(enterPressed, inputThatCausedFocusLoss);
        if( RBX::UserInputService* userInputService = RBX::ServiceProvider::create<RBX::UserInputService>(this) )
            userInputService->textBoxReleaseFocus(shared_from(this));
    }
    
	void TextBox::onHeartbeat(const Heartbeat& event)
	{
		if(iAmFocus)
		{
			switch(repeatKeyState.state)
			{
				case RepeatKeyState::STATE_NONE:
					break;
				case RepeatKeyState::STATE_DEPRESSED:
					if(repeatKeyState.stateWallTime + 0.5 < event.wallTime && !UserInputService::IsUsingNewKeyboardEvents())
					{
						//We've waited half a second, start repeating
						doKey(repeatKeyState.keyType, repeatKeyState.character);
						repeatKeyState.state = RepeatKeyState::STATE_REPEATING;
						repeatKeyState.stateWallTime = event.wallTime;
					}
					break;
				case RepeatKeyState::STATE_REPEATING:
					if (!UserInputService::IsUsingNewKeyboardEvents())
					{
						while(repeatKeyState.stateWallTime + (1/20.0) < event.wallTime)
						{
							doKey(repeatKeyState.keyType, repeatKeyState.character);
							repeatKeyState.stateWallTime += 1/20.0;
						}
					}
					break;
			}
		}
	}

	void TextBox::doKey(RepeatKeyState::KeyType keyType, char key)
	{
		doKey(keyType, std::string(1, key));
	}

	bool TextBox::deleteSelection()
	{
		if (selectionStartPos < 0 || selectionStartPos == cursorPos)
		{
			selectionStartPos = -1;
			return false;
		}
		const int begin = std::min(selectionStartPos, cursorPos);
		const int end = std::max(selectionStartPos, cursorPos);
		bufferedText.erase(begin, end - begin);
		cursorPos = begin;
		selectionStartPos = -1;
		return true;
	}

    void TextBox::doKey(RepeatKeyState::KeyType keyType, std::string key)
	{
		std::string bufferedTextBefore = bufferedText;
		const int cursorPositionBefore = getCursorPosition();
		const int selectionStartBefore = getSelectionStart();

		switch(keyType)
		{
		case RepeatKeyState::TYPE_BACKSPACE:
			{
				if (deleteSelection())
					break;
				if(!key.empty() && key[0] != 0)
				{
					//Delete word
					int i = std::min<int>(bufferedText.size() - 1, cursorPos);

					//Move past any whitespace
					while( i >= 0 ) 
					{
						if(Typesetter::isCharWhitespace(bufferedText[i]) || bufferedText[i] == '\n')
							--i;
						else
							break;
					}

					//Move until we hit our first whitespace
					while( i >= 0 )
					{
						if(Typesetter::isCharWhitespace(bufferedText[i]))
							break;
						else
							--i;
					}

					if( i < 0 )
					{
						bufferedText = bufferedText.substr(cursorPos);
						cursorPos = 0;
					}
					else if (i != cursorPos)
					{
						bufferedText = bufferedText.erase(i, (cursorPos - i));
						cursorPos = i;
					}
				}
				else if ( (bufferedText.size() > 0) && (cursorPos > 0) && (unsigned(cursorPos) <= bufferedText.length()) ) // delete key
				{
					const int previous = previousUtf8Boundary(bufferedText, cursorPos);
					bufferedText.erase(previous, cursorPos - previous);
					cursorPos = previous;
				}
				break;
			}
		case RepeatKeyState::TYPE_DELETE:
			if (!deleteSelection() && (bufferedText.size() > 0) &&
				(cursorPos >= 0) && (unsigned(cursorPos) < bufferedText.length()))
				bufferedText.erase(cursorPos,
					nextUtf8Boundary(bufferedText, cursorPos) - cursorPos);
			break;
		case RepeatKeyState::TYPE_CHARACTER:
			if(Typesetter::isStringSupported(key))
			{
				deleteSelection();
				if( (cursorPos >= 0) && (cursorPos >= 0) && (unsigned(cursorPos) <= bufferedText.length()) )
					bufferedText.insert(cursorPos,key);
				cursorPos += key.length();
			}
			break;
		case RepeatKeyState::TYPE_LEFTARROW:
			if (selectionStartPos >= 0)
			{
				cursorPos = std::min(selectionStartPos, cursorPos);
				selectionStartPos = -1;
			}
			else if(cursorPos > 0)
				cursorPos = previousUtf8Boundary(bufferedText, cursorPos);
			break;
		case RepeatKeyState::TYPE_RIGHTARROW:
			if (selectionStartPos >= 0)
			{
				cursorPos = std::max(selectionStartPos, cursorPos);
				selectionStartPos = -1;
			}
			else if(static_cast<unsigned>(cursorPos) < bufferedText.length())
				cursorPos = nextUtf8Boundary(bufferedText, cursorPos);
			break;
		case RepeatKeyState::TYPE_PASTE:
			if(repeatKeyState.state == RepeatKeyState::STATE_NONE)
			{
				std::string pasteText = "";
				if(RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(this))
					pasteText = inputService->getPasteText();

				if( (pasteText.length() > 0) && (cursorPos >= 0) && (unsigned(cursorPos) <= bufferedText.length()) )
				{
					deleteSelection();
					bufferedText.insert(cursorPos,pasteText);
					cursorPos += pasteText.length();
				}
			}
			break;
		}

		if (bufferedTextBefore != bufferedText)
			setText(bufferedText);
		if (cursorPositionBefore != getCursorPosition())
			raisePropertyChanged(prop_CursorPosition);
		if (selectionStartBefore != getSelectionStart())
			raisePropertyChanged(prop_SelectionStart);
	}

	void TextBox::keyUp(RepeatKeyState::KeyType keyType, RBX::KeyCode keyCode, char key)
	{
		if( repeatKeyState.state == RepeatKeyState::STATE_DEPRESSED || (repeatKeyState.state == RepeatKeyState::STATE_REPEATING && !UserInputService::IsUsingNewKeyboardEvents()) )
			if(keyType == repeatKeyState.keyType && keyCode == repeatKeyState.keyCode) //if last key pressed was ours, stop it from repeating further
				repeatKeyState.state = RepeatKeyState::STATE_NONE;
	}

	void TextBox::textInput(RepeatKeyState::KeyType keyType, std::string textInput)
	{
		if (UserInputService::IsUsingNewKeyboardEvents() && iAmFocus)
		{
			doKey(keyType, textInput);
		}
	}

	void TextBox::keyDown(RepeatKeyState::KeyType keyType, RBX::KeyCode keyCode, char key)
	{
		if(repeatKeyState.state == RepeatKeyState::STATE_NONE || repeatKeyState.state == RepeatKeyState::STATE_DEPRESSED)
		{
			doKey(keyType, key);
            
			repeatKeyState.keyCode = keyCode;
			repeatKeyState.keyType = keyType;
			repeatKeyState.character = key;
			if(keyType != RepeatKeyState::TYPE_PASTE) // we don't repeat paste commands
				repeatKeyState.state = RepeatKeyState::STATE_DEPRESSED;
			if(RunService* runService = ServiceProvider::find<RunService>(this))
				repeatKeyState.stateWallTime = runService->wallTime();
		}
		else
		{
			//We are repeating, so ignore the new key press
		}
	}
	GuiResponse TextBox::processTextInputEvent(const shared_ptr<InputObject>& event)
	{
		RBXASSERT(event->isTextInputEvent());

		textInput(RepeatKeyState::TYPE_CHARACTER, event->getInputText());

		return iAmFocus ? GuiResponse::sunkWithTarget(this) : GuiResponse::notSunk();
	}
	GuiResponse TextBox::processKeyEvent(const shared_ptr<InputObject>& event)
	{
		RBXASSERT(event->isKeyEvent());

		if(shouldCaptureFocus)
		{
			gainFocus(event);
			shouldCaptureFocus = false;
		}
        
		std::string bufferedTextBefore = bufferedText;
		bool wasFocusAtStartOfProcessKey = iAmFocus;
        
        
		GuiResponse answer = Super::processKeyEvent(event);
		if (iAmFocus) 
		{
			if (event->isKeyDownEvent())
			{
				if( event->isEscapeKey() )
					releaseFocus(false, event);
				else if(event->isCarriageReturnKey())
				{
					if (isSelectedObject())
						releaseFocus(false, event);
					else if(getMultiLine())
						keyDown(RepeatKeyState::TYPE_CHARACTER, event->getKeyCode(), '\n');
					else
						releaseFocus(true, event);
				}
				else if (event->isLeftArrowKey())
					keyDown(RepeatKeyState::TYPE_LEFTARROW, event->getKeyCode(), 'l');
				else if (event->isRightArrowKey())
					keyDown(RepeatKeyState::TYPE_RIGHTARROW, event->getKeyCode(), 'r');
				else if (event->isClearKey())
					bufferedText = "";
				else if (event->isDeleteKey())
					keyDown(RepeatKeyState::TYPE_DELETE, event->getKeyCode(), '\0');
				else if (event->isBackspaceKey())
                {
                    RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(this);
                    RBXASSERT(inputService);
					keyDown(RepeatKeyState::TYPE_BACKSPACE, event->getKeyCode(), inputService->isCtrlDown() ? '\1' : '\0');
                }
				else if (event->isTextCharacterKey())
				{
                    if(isPasteCommand(event))
						keyDown(RepeatKeyState::TYPE_PASTE, event->getKeyCode(),'v');
					else
                        keyDown(RepeatKeyState::TYPE_CHARACTER, event->getKeyCode(), event->modifiedKey);
				}
			}
			else if(event->isKeyUpEvent()) // key up
			{
				if(event->isCarriageReturnKey())
				{
					if(getMultiLine())
						keyUp(RepeatKeyState::TYPE_CHARACTER, event->getKeyCode(), '\n');
				}
				else if(event->isDeleteKey())
					keyUp(RepeatKeyState::TYPE_DELETE, event->getKeyCode(), '\0');
				else if(event->isBackspaceKey())
					keyUp(RepeatKeyState::TYPE_BACKSPACE, event->getKeyCode(), '\0');
				else if (event->isLeftArrowKey())
					keyUp(RepeatKeyState::TYPE_LEFTARROW, event->getKeyCode(), 'l');
				else if (event->isRightArrowKey())
					keyUp(RepeatKeyState::TYPE_RIGHTARROW, event->getKeyCode(), 'r');
				else if(event->isTextCharacterKey())
				{
					char found = event->modifiedKey;
					keyUp(RepeatKeyState::TYPE_CHARACTER, event->getKeyCode(), found);
				}
			}
			//Turn off repeating characters if shift is pressed or unpressed
			if((event->getKeyCode() == SDLK_LSHIFT || event->getKeyCode() == SDLK_RSHIFT) &&
			   (repeatKeyState.state == RepeatKeyState::STATE_DEPRESSED || (repeatKeyState.state == RepeatKeyState::STATE_REPEATING && !UserInputService::IsUsingNewKeyboardEvents())))
				repeatKeyState.state = RepeatKeyState::STATE_NONE;
		}
		else if (!iAmFocus && event->isKeyDownEvent() && isSelectedObject() && event->getKeyCode() == SDLK_RETURN)
			captureFocus();

		if (bufferedTextBefore != bufferedText)
			setText(bufferedText);
        
		return wasFocusAtStartOfProcessKey ? GuiResponse::sunkWithTarget(this) : answer;
	}
        
    bool TextBox::isPasteCommand(const shared_ptr<InputObject>& event)
    {
        if(event->getKeyCode() == RBX::SDLK_v)
        {
            if(RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(this))
            {
                std::vector<RBX::ModCode> modCodes = inputService->getCommandModCodes();
                for(std::vector<RBX::ModCode>::iterator it = modCodes.begin(); it != modCodes.end(); ++it)
                {
                    if(DFFlag::PasteWithCapsLockOn ? event->mod & *(it) : event->mod == *(it))
                        return true;
                }
			}
		}
        
        return false;
	}
    
	std::string TextBox::getTextWithCursor()
	{
		RBXASSERT(cursorPos >= 0);
        
		// make sure we are in range
		cursorPos = std::min<int>(std::max<int>(0, cursorPos), bufferedText.length());
        
		std::string textWithCursor = bufferedText;
		
        textWithCursor.insert(cursorPos,"\1");
        
		return textWithCursor;
	}
	std::string TextBox::getTextWithBlankCursor()
	{
		RBXASSERT(cursorPos >= 0);
        
		// make sure we are in range
		cursorPos = std::min<int>(std::max<int>(0, cursorPos), bufferedText.length());
        
		std::string textWithCursor = bufferedText;
        
		return textWithCursor;
	}
	
	void TextBox::render2d(Adorn* adorn)
	{
		const bool renderPlaceholder = bufferedText.empty() && !placeholderText.empty();
		const Color4 placeholderColor = applyCanvasGroup(
			Color4(placeholderColor3, getRenderTextAlpha(getTextTransparency())));

		if (iAmFocus)
		{
			Time now = Time::nowFast();
			double delta = (now - lastSwap).seconds();
			if (delta > CURSOR_BLINK_RATE_SECONDS)
			{
				showingCursor = !showingCursor;
				lastSwap = now;
			}
            
			// The placeholder remains visible while an empty box is focused; the
			// insertion caret alternates on top of it using the existing control
			// character understood by the text renderer.
			if (renderPlaceholder)
			{
				render2dTextImpl(adorn, getRenderBackgroundColor4(), localizeText(placeholderText), getFont(), getTextSize(), placeholderColor, applyCanvasGroup(getRenderTextStrokeColor4()), getTextWrap(), getTextScale(), getXAlignment(), getYAlignment(), getTextTruncate());
				if (showingCursor)
					render2dTextImpl(adorn, getRenderBackgroundColor4(), "\1", getFont(), getTextSize(), applyCanvasGroup(getRenderTextColor4()), applyCanvasGroup(getRenderTextStrokeColor4()), getTextWrap(), getTextScale(), getXAlignment(), getYAlignment(), getTextTruncate());
			}
			else
				render2dTextImpl(adorn, getRenderBackgroundColor4(), showingCursor ? getTextWithCursor() : getTextWithBlankCursor(), getFont(), getTextSize(), applyCanvasGroup(getRenderTextColor4()), applyCanvasGroup(getRenderTextStrokeColor4()), getTextWrap(), getTextScale(), getXAlignment(), getYAlignment(), getTextTruncate());
		}
		else
			render2dTextImpl(adorn, getRenderBackgroundColor4(), localizeText(renderPlaceholder ? placeholderText : getContentText()), getFont(), getTextSize(), renderPlaceholder ? placeholderColor : applyCanvasGroup(getRenderTextColor4()), applyCanvasGroup(getRenderTextStrokeColor4()), getTextWrap(), getTextScale(), getXAlignment(), getYAlignment(), getTextTruncate());
        
		renderStudioSelectionBox(adorn);
	}
}
