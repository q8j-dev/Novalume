
#include "v8datamodel/ScrollingFrame.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/UserInputService.h"
#include "v8datamodel/GamepadService.h"
#include "v8datamodel/TextBox.h"
#include "rbx/ui/ScreenLayout.h"

#define category_Scrolling "Scrolling"

#define WHEEL_SCROLL_DELTA 10
#define MAX_SCROLL_WHEEL_SCROLL_PERCENT 0.10f

#define MIN_SCROLLBAR_SIZE 1.0f

#define REALLY_BIG_SCROLL_DELTA 10000000.0f
#define MAX_MULTIPLY_TIME_MSEC 250.0f

#define DECELERATION_RATE 0.9f
#define TOUCH_INERTIA_RESET_TIME_MSEC 500.0f

#define GAMEPAD_THUMBSTICK_WAIT_PERIOD_MSEC 300.0f
#define GAMEPAD_SCROLL_CONSTANT 80.0f

DYNAMIC_FASTFLAGVARIABLE(LimitScrollWheelMaxToHalfWindowSize, false)
DYNAMIC_FASTFLAGVARIABLE(FixRotatedHorizontalScrollBar, false)

namespace RBX
{
	const char* const sScrollingFrame = "ScrollingFrame";
    
    const Reflection::PropDescriptor<ScrollingFrame, bool> prop_scrollingEnabled("ScrollingEnabled", category_Scrolling, &ScrollingFrame::getScrollingEnabled, &ScrollingFrame::setScrollingEnabled);
    const Reflection::EnumPropDescriptor<ScrollingFrame, Enums::ScrollingDirection> prop_scrollingDirection("ScrollingDirection", category_Scrolling, &ScrollingFrame::getScrollingDirection, &ScrollingFrame::setScrollingDirection);
    const Reflection::EnumPropDescriptor<ScrollingFrame, Enums::ScrollBarInset> prop_horizontalScrollBarInset("HorizontalScrollBarInset", category_Scrolling, &ScrollingFrame::getHorizontalScrollBarInset, &ScrollingFrame::setHorizontalScrollBarInset);
    const Reflection::EnumPropDescriptor<ScrollingFrame, Enums::ScrollBarInset> prop_verticalScrollBarInset("VerticalScrollBarInset", category_Scrolling, &ScrollingFrame::getVerticalScrollBarInset, &ScrollingFrame::setVerticalScrollBarInset);
    const Reflection::EnumPropDescriptor<ScrollingFrame, Enums::VerticalScrollBarPosition> prop_verticalScrollBarPosition("VerticalScrollBarPosition", category_Scrolling, &ScrollingFrame::getVerticalScrollBarPosition, &ScrollingFrame::setVerticalScrollBarPosition);

	const Reflection::PropDescriptor<ScrollingFrame, UDim2> prop_canvasSize("CanvasSize", category_Scrolling, &ScrollingFrame::getCanvasSize, &ScrollingFrame::setCanvasSize);
	const Reflection::EnumPropDescriptor<ScrollingFrame, AutomaticSize> prop_automaticCanvasSize("AutomaticCanvasSize", category_Scrolling, &ScrollingFrame::getAutomaticCanvasSize, &ScrollingFrame::setAutomaticCanvasSize);
	const Reflection::PropDescriptor<ScrollingFrame, Vector2> prop_absoluteCanvasSize("AbsoluteCanvasSize", category_Scrolling, &ScrollingFrame::getAbsoluteCanvasSize, NULL);
	const Reflection::PropDescriptor<ScrollingFrame, Vector2> prop_canvasPosition("CanvasPosition", category_Scrolling, &ScrollingFrame::getCanvasPosition, &ScrollingFrame::luaSetCanvasPosition);

	const Reflection::PropDescriptor<ScrollingFrame, Vector2> prop_absWindowSize("AbsoluteWindowSize", category_Scrolling, &ScrollingFrame::getAbsoluteWindowSize, NULL);

	const Reflection::PropDescriptor<ScrollingFrame, int> prop_scrollBarThickness("ScrollBarThickness", category_Scrolling, &ScrollingFrame::getScrollBarThickness, &ScrollingFrame::setScrollBarThickness);
	const Reflection::PropDescriptor<ScrollingFrame, Color3> prop_scrollBarImageColor("ScrollBarImageColor3", category_Scrolling, &ScrollingFrame::getScrollBarImageColor3, &ScrollingFrame::setScrollBarImageColor3);
	const Reflection::PropDescriptor<ScrollingFrame, float> prop_scrollBarImageTransparency("ScrollBarImageTransparency", category_Scrolling, &ScrollingFrame::getScrollBarImageTransparency, &ScrollingFrame::setScrollBarImageTransparency);

	const Reflection::PropDescriptor<ScrollingFrame, TextureId> prop_topImage("TopImage", category_Scrolling, &ScrollingFrame::getTopImage, &ScrollingFrame::setTopImage);
	const Reflection::PropDescriptor<ScrollingFrame, TextureId> prop_midImage("MidImage", category_Scrolling, &ScrollingFrame::getMidImage, &ScrollingFrame::setMidImage);
	const Reflection::PropDescriptor<ScrollingFrame, TextureId> prop_bottomImage("BottomImage", category_Scrolling, &ScrollingFrame::getBottomImage, &ScrollingFrame::setBottomImage);
    
    ScrollingFrame::ScrollingFrame()
        : DescribedCreatable<ScrollingFrame, GuiObject, sScrollingFrame>(sScrollingFrame, false)
		, IStepped(StepType_Render)
        , scrollDragging(false)
        , verticalBarSize(Rect2D())
        , horizontalBarSize(Rect2D())
        , scrollingDirection(Y)
        , allowedScrollingDirection(Enums::SCROLLING_DIRECTION_XY)
        , horizontalScrollBarInset(Enums::SCROLL_BAR_INSET_NONE)
        , verticalScrollBarInset(Enums::SCROLL_BAR_INSET_NONE)
        , verticalScrollBarPosition(Enums::VERTICAL_SCROLL_BAR_POSITION_RIGHT)
        , topImage(TextureId("rbxasset://textures/ui/Scroll/scroll-top.png"))
        , midImage(TextureId("rbxasset://textures/ui/Scroll/scroll-middle.png"))
        , bottomImage(TextureId("rbxasset://textures/ui/Scroll/scroll-bottom.png"))
        , scrollingEnabled(true)
		, canvasSize(0,0,2,0)
		, automaticCanvasSize(AUTOMATIC_SIZE_NONE)
		, canvasPosition(0,0)
		, scrollBarThickness(12)
		, scrollBarImageColor(Color3::white())
		, scrollBarImageTransparency(0.0f)
		, needsRecalculate(true)
		, scrollDeltaMultiplier(1)
		, isLeftMouseDown(false)
        , touchInputPositions(4)
        , sampledVelocity(0,0)
		, lastGamepadScrollDelta(0,0)
	{
		selectable = true;
        clipping = true;
    }

	void ScrollingFrame::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
	{
        if (oldProvider)
        {
            inputEndedConnection.disconnect();
			guiServicePropertyChangedConnection.disconnect();
			selectionGainedConnection.disconnect();
			selectionLostConnection.disconnect();
        }
        
		Super::onServiceProvider(oldProvider, newProvider);
		onServiceProviderIStepped(oldProvider, newProvider);

        if (newProvider)
        {
            wheelDownTimer.reset();
            wheelUpTimer.reset();
            if(UserInputService* inputService = RBX::ServiceProvider::find<UserInputService>(newProvider))
            {
                inputEndedConnection = inputService->coreInputEndedEvent.connect(boost::bind(&ScrollingFrame::globalInputEnded,this,_1));
            }

			if(GuiService* guiService = RBX::ServiceProvider::find<GuiService>(newProvider))
			{
				guiServicePropertyChangedConnection = guiService->propertyChangedSignal.connect(boost::bind(&ScrollingFrame::selectedGuiObjectChanged, this, _1));
			}

			selectionGainedConnection = selectionGainedEvent.connect(boost::bind(&ScrollingFrame::selectionGained, this));
			selectionLostConnection = selectionLostEvent.connect(boost::bind(&ScrollingFrame::selectionLost, this));
        }
	}

	void ScrollingFrame::selectionLost()
	{
		lastGamepadScrollDelta = Vector2::zero();
	}

	void ScrollingFrame::selectionGained()
	{
		selectionTimer.reset();
	}

	void ScrollingFrame::selectedGuiObjectChanged(const RBX::Reflection::PropertyDescriptor* desc)
	{
		GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this);
		if (!guiService)
		{
			return;
		}

		if (!scrollingEnabled)
		{
			return;
		}

		RBX::GuiObject* selectedObject = NULL;
		if (*desc == GuiService::prop_selectedGuiObject)
		{
			selectedObject = guiService->getSelectedGuiObjectLua();
		}
		else if(*desc == GuiService::prop_selectedCoreGuiObject)
		{
			selectedObject = guiService->getSelectedCoreGuiObjectLua();
		}

		if (!selectedObject)
		{
			return;
		}

		if (!selectedObject->isDescendantOf(this))
		{
			return;
		}

		// make sure selected object is visible in some capacity

		const Rect2D selectedObjectRect = selectedObject->getRect2D();
		const Rect2D visibleRect = getVisibleRect2D();

		Vector2 scrollDelta = Vector2::zero();

		if (selectedObjectRect.height() <= visibleRect.height())
		{
			if (selectedObjectRect.y1() > visibleRect.y1())
			{
				scrollDelta.y = selectedObjectRect.y1() - visibleRect.y1();
			}
			else if (selectedObjectRect.y0() < visibleRect.y0())
			{
				scrollDelta.y = selectedObjectRect.y0() - visibleRect.y0();
			}
		}
		else
		{
			if (selectedObjectRect.y0() >= visibleRect.y1() )
			{
				scrollDelta.y = selectedObjectRect.height();
			}
			else if (selectedObjectRect.y1() <= visibleRect.y0())
			{
				scrollDelta.y = -selectedObjectRect.height();
			}
		}

		if (selectedObjectRect.width() <= visibleRect.width())
		{
			if (selectedObjectRect.x1() > visibleRect.x1())
			{
				scrollDelta.x = selectedObjectRect.x1() - visibleRect.x1();
			}
			else if (selectedObjectRect.x0() < visibleRect.x0())
			{
				scrollDelta.x = selectedObjectRect.x0() - visibleRect.x0();
			}
		}
		else
		{
			if (selectedObjectRect.x0() >= visibleRect.x1() )
			{
				scrollDelta.x = selectedObjectRect.width();
			}
			else if (selectedObjectRect.x1() <= visibleRect.x0())
			{
				scrollDelta.x = -selectedObjectRect.width();
			}
		}

		if (scrollDelta != Vector2::zero())
		{
			Vector2 scrollPercent = scrollDelta/getCanvasRect().wh();
			setCanvasPosition( Vector2(canvasPosition.x + (getCanvasRect().width() * scrollPercent.x), canvasPosition.y + (getCanvasRect().height() * scrollPercent.y)), false);
		}
	}
    
	void ScrollingFrame::onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
	{
		Super::onPropertyChanged(descriptor);

		if(descriptor == GuiBase2d::prop_AbsoluteSize 
			|| descriptor == GuiBase2d::prop_AbsolutePosition
			|| descriptor == prop_canvasSize
			|| descriptor == prop_canvasPosition
			|| descriptor == prop_scrollBarThickness)
		{
			resizeChildren();
			needsRecalculate = true;
		}
	}

	Vector2 ScrollingFrame::getAbsoluteWindowSize() const
	{
		return getVisibleRect2D().wh();
	}

	Rect2D ScrollingFrame::getVisibleRect2D() const
	{
		Rect2D rect = getRect2D();
		const bool insetHorizontal = horizontalScrollBarInset == Enums::SCROLL_BAR_INSET_ALWAYS ||
			(horizontalScrollBarInset == Enums::SCROLL_BAR_INSET_SCROLL_BAR && canScrollHorizontal());
		const bool insetVertical = verticalScrollBarInset == Enums::SCROLL_BAR_INSET_ALWAYS ||
			(verticalScrollBarInset == Enums::SCROLL_BAR_INSET_SCROLL_BAR && canScrollVertical());

		if (insetHorizontal)
		{
			rect = Rect2D::xywh(rect.x0y0(), Vector2(rect.width(), std::max(0.0f, rect.height() - scrollBarThickness)));
		}
		if (insetVertical)
		{
			const float width = std::max(0.0f, rect.width() - scrollBarThickness);
			const float x = verticalScrollBarPosition == Enums::VERTICAL_SCROLL_BAR_POSITION_LEFT
				? rect.x0() + scrollBarThickness
				: rect.x0();
			rect = Rect2D::xywh(Vector2(x, rect.y0()), Vector2(width, rect.height()));
		}

		return rect;
	}

	void ScrollingFrame::setScrollingEnabled(bool value)
    {
        if (value != scrollingEnabled)
        {
            scrollingEnabled = value;
            raisePropertyChanged(prop_scrollingEnabled);
        }
    }

	void ScrollingFrame::setCanvasSize(UDim2 value)
	{
		if (value != canvasSize)
		{
			canvasSize = value;
			raisePropertyChanged(prop_canvasSize);
		}
	}

	void ScrollingFrame::setAutomaticCanvasSize(AutomaticSize value)
	{
		if (automaticCanvasSize != value)
		{
			automaticCanvasSize = value;
			raisePropertyChanged(prop_automaticCanvasSize);
			raisePropertyChanged(prop_absoluteCanvasSize);
			resizeChildren();
			needsRecalculate = true;
		}
	}

	Vector2 ScrollingFrame::getAbsoluteCanvasSize() const
	{
		const Rect2D viewport = getRect2D();
		Vector2 result(viewport.width() * canvasSize.x.scale + canvasSize.x.offset,
			viewport.height() * canvasSize.y.scale + canvasSize.y.offset);
		result.x = std::max(viewport.width(), result.x);
		result.y = std::max(viewport.height(), result.y);

		if (automaticCanvasSize != AUTOMATIC_SIZE_NONE)
		{
			Vector2 content(viewport.width(), viewport.height());
			for (size_t index = 0; index < numChildren(); ++index)
			{
				if (const GuiBase2d* child = Instance::fastDynamicCast<GuiBase2d>(getChild(index)))
				{
					const Rect2D rect = child->getRect2D();
					content.x = std::max(content.x, rect.x1() - viewport.x0() + canvasPosition.x);
					content.y = std::max(content.y, rect.y1() - viewport.y0() + canvasPosition.y);
				}
			}
			if (automaticCanvasSize == AUTOMATIC_SIZE_X || automaticCanvasSize == AUTOMATIC_SIZE_XY) result.x = content.x;
			if (automaticCanvasSize == AUTOMATIC_SIZE_Y || automaticCanvasSize == AUTOMATIC_SIZE_XY) result.y = content.y;
		}
		return result;
	}

	void ScrollingFrame::setScrollingDirection(Enums::ScrollingDirection value)
	{
		if (allowedScrollingDirection == value)
			return;
		allowedScrollingDirection = value;
		if (value == Enums::SCROLLING_DIRECTION_X)
			setCanvasPosition(Vector2(canvasPosition.x, 0), false);
		else if (value == Enums::SCROLLING_DIRECTION_Y)
			setCanvasPosition(Vector2(0, canvasPosition.y), false);
		needsRecalculate = true;
		raisePropertyChanged(prop_scrollingDirection);
	}

	void ScrollingFrame::setHorizontalScrollBarInset(Enums::ScrollBarInset value)
	{
		if (horizontalScrollBarInset == value) return;
		horizontalScrollBarInset = value;
		resizeChildren();
		needsRecalculate = true;
		raisePropertyChanged(prop_horizontalScrollBarInset);
	}

	void ScrollingFrame::setVerticalScrollBarInset(Enums::ScrollBarInset value)
	{
		if (verticalScrollBarInset == value) return;
		verticalScrollBarInset = value;
		resizeChildren();
		needsRecalculate = true;
		raisePropertyChanged(prop_verticalScrollBarInset);
	}

	void ScrollingFrame::setVerticalScrollBarPosition(Enums::VerticalScrollBarPosition value)
	{
		if (verticalScrollBarPosition == value) return;
		verticalScrollBarPosition = value;
		resizeChildren();
		needsRecalculate = true;
		raisePropertyChanged(prop_verticalScrollBarPosition);
	}

	void ScrollingFrame::luaSetCanvasPosition(Vector2 value)
	{
		setCanvasPosition(value);
	}

	Vector2 ScrollingFrame::getMaxSize() const
	{
		return getCanvasScrollingSize();
	}

    Vector2 ScrollingFrame::calculateCanvasPositionClamped(Vector2 desiredValue) const
    {
        // make sure we aren't setting anything to NaN or some other invalid value
        if (RBX::Math::isNanInf(desiredValue.x))
        {
            desiredValue.x = canvasPosition.x;
        }
        if (RBX::Math::isNanInf(desiredValue.y))
        {
            desiredValue.y = canvasPosition.y;
        }
        
		Vector2 maxSize = getCanvasScrollingSize();
        
        return G3D::clamp(desiredValue, Vector2::zero(), maxSize.max(Vector2::zero()));
    }
    
	void ScrollingFrame::setCanvasPosition(Vector2 value, bool printWarnings)
	{
		if (value == canvasPosition)
		{
			return;
		}
        
        Vector2 newValue = calculateCanvasPositionClamped(value);

		if (newValue != value && printWarnings)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "ScrollingFrame.CanvasPosition set to invalid position (%f, %f), clamping to position (%f, %f).", value.x,
																																											value.y,
																																											newValue.x,
																																											newValue.y);
		}

		if (newValue != canvasPosition)
		{
			canvasPosition = newValue;
			raisePropertyChanged(prop_canvasPosition);
		}
	}

	void ScrollingFrame::setScrollBarThickness(int value)
	{
		int newValue = std::max(0, value);
		if (newValue != value)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "ScrollingFrame.ScrollBarThickness set to negative value, clamping value to 0.");
		}
		if (newValue != scrollBarThickness)
		{
			scrollBarThickness = newValue;
			raisePropertyChanged(prop_scrollBarThickness);
		}
	}

	void ScrollingFrame::setScrollBarImageColor3(Color3 value)
	{
		if (scrollBarImageColor != value) { scrollBarImageColor = value; raisePropertyChanged(prop_scrollBarImageColor); }
	}

	void ScrollingFrame::setScrollBarImageTransparency(float value)
	{
		value = G3D::clamp(value, 0.0f, 1.0f);
		if (scrollBarImageTransparency != value) { scrollBarImageTransparency = value; raisePropertyChanged(prop_scrollBarImageTransparency); }
	}

	void ScrollingFrame::setTopImage(TextureId value)
	{
		if (value != topImage)
		{
			topImage = value;
			raisePropertyChanged(prop_topImage);
		}
	}

	void ScrollingFrame::setMidImage(TextureId value)
	{
		if (value != midImage)
		{
			midImage = value;
			raisePropertyChanged(prop_midImage);
		}
	}

	void ScrollingFrame::setBottomImage(TextureId value)
	{
		if (value != bottomImage)
		{
			bottomImage = value;
			raisePropertyChanged(prop_bottomImage);
		}
	}

	Rect2D ScrollingFrame::getCanvasRectLocal() const
	{
		Rect2D myScrollRect;

		if(const GuiBase2d* guiParent = Instance::fastDynamicCast<GuiBase2d>(getParent()))
		{
			myScrollRect = guiParent->getRect2D();
		}
		else
		{
			myScrollRect = getRect2D();
		}

		const Vector2 absoluteCanvasSize = getAbsoluteCanvasSize();
		float width = absoluteCanvasSize.x;
		float height = absoluteCanvasSize.y;

		// make sure there is a valid size for these
		width = std::max(getRect2D().width(), width);
		height = std::max(getRect2D().height(), height);

		const Vector2 rectPos = getRect2D().x0y0();

		return Rect2D::xywh(rectPos.x - canvasPosition.x,
							rectPos.y - canvasPosition.y,
							width,
							height);
	}

	Rect2D ScrollingFrame::getCanvasRect() const
	{
		Rect2D canvasLocalRect = getCanvasRectLocal();
		Vector2 localPos = canvasLocalRect.x0y0();
		return Rect2D::xywh(localPos, canvasLocalRect.wh());
	}


	void ScrollingFrame::handleResize(const Rect2D& viewport, bool force)
	{
        Super::handleResize(viewport,force);
		resizeChildren();
	}
    
    void ScrollingFrame::stepScrollInertia(const Stepped& event)
    {
        if(sampledVelocity != Vector2::zero() && touchInput == shared_ptr<InputObject>())
        {
            Vector2 movement = sampledVelocity * event.gameStep;
            handleInputDrag(lastInputPosition + movement);
            sampledVelocity *= DECELERATION_RATE;
            
            if (sampledVelocity.length() <= 1.0f)
            {
                sampledVelocity = Vector2::zero();
                scrollDragging = false;
            }
        }

    }

	void ScrollingFrame::stepGamepadScroll(const Stepped& event)
	{
		Vector2 scrollDelta = lastGamepadScrollDelta * event.gameStep * GAMEPAD_SCROLL_CONSTANT;
		if (scrollDelta != Vector2::zero())
		{
			if (!scrollByPosition(scrollDelta))
			{
				if (GamepadService* gamepadService = RBX::ServiceProvider::create<GamepadService>(this))
				{
					gamepadService->trySelectGuiObject(-lastGamepadScrollDelta.direction());
					lastGamepadScrollDelta = Vector2::zero();
				}
			}
		}
	}
    
    void ScrollingFrame::onStepped(const Stepped& event)
    {
        stepScrollInertia(event);
		stepGamepadScroll(event);
        recalculateScroll();
    }

	Rect2D ScrollingFrame::getClippedRect()
	{
		return Super::getClippedRect().intersect(getVisibleRect2D());
	}

	Vector2 ScrollingFrame::getCanvasScrollingSize() const
	{
		Vector2 scrollingSize = getCanvasRectLocal().wh() - getVisibleRect2D().wh();
		scrollingSize = scrollingSize.clamp(Vector2::zero(),Vector2::inf());

		return scrollingSize;
	}

	Vector2 ScrollingFrame::getFrameScrollingSize() const
	{
		const bool noCorner = (!canScrollHorizontal() || !canScrollVertical());
		return (getRect2D().wh() - getVerticalScrollBarSize() - getHorizontalScrollBarSize() + (noCorner ? Vector2(scrollBarThickness,scrollBarThickness) : Vector2::zero()));
	}

	Vector2 ScrollingFrame::getVerticalScrollBarSize() const
	{
		const float minScrollBarLength = getMinScrollBarSize();
		const bool hasCorner = (canScrollHorizontal() && canScrollVertical());
		const float rectHeight = getRect2D().height();

		float verticalSize = (hasCorner ? (-scrollBarThickness+1) : 0) + (rectHeight / getCanvasRectLocal().height()) * rectHeight;
		verticalSize = std::max(minScrollBarLength, verticalSize);

		return Vector2(scrollBarThickness, verticalSize);
	}

	Vector2 ScrollingFrame::getHorizontalScrollBarSize() const
	{
		const float minScrollBarLength = getMinScrollBarSize();
		const bool hasCorner = (canScrollHorizontal() && canScrollVertical());
		const float rectWidth = getRect2D().width();

		float horizontalSize = (hasCorner ? (-scrollBarThickness+1) : 0) + (rectWidth / getCanvasRectLocal().width()) * rectWidth;
		horizontalSize = std::max(minScrollBarLength, horizontalSize);

		return Vector2(horizontalSize, scrollBarThickness);
	}

	float ScrollingFrame::getMinScrollBarSize() const
	{
		return MIN_SCROLLBAR_SIZE + (scrollBarThickness * 2);
	}

	void ScrollingFrame::recalculateScroll()
	{
		if (!needsRecalculate)
		{
			return;
		}
        
		needsRecalculate = false;

		const Rect2D myRect = getRect2D();

		const Vector2 scrollSize = getFrameScrollingSize();

		const Vector2 canvasSize = getCanvasScrollingSize();
        const Vector2 canvasPos = calculateCanvasPositionClamped(getCanvasPosition());
        
        setCanvasPosition(canvasPos, false);

		////////////////////////////////////////////////////////////////////////////////////////////
		// Vertical Calculation

		const float yPercentageLocation = (canvasSize.y > 0.0f) ? canvasPos.y/canvasSize.y : 0.0f;
		const float yPositionVertical =  myRect.y0() + (scrollSize.y * yPercentageLocation);

		const Vector2 sizeVertical = getVerticalScrollBarSize();

		const float verticalX = verticalScrollBarPosition == Enums::VERTICAL_SCROLL_BAR_POSITION_LEFT
			? myRect.x0()
			: myRect.x1() - sizeVertical.x;
		const Vector2 positionVertical = Vector2(verticalX, yPositionVertical);
		verticalBarSize = Rect2D::xywh(positionVertical, sizeVertical);

		//////////////////////////////////////////////////////////////////////////////////////////
		// Horizontal Calculation

		const float xPercentageLocation = (canvasSize.x > 0.0f) ? canvasPos.x/canvasSize.x : 0.0f;
		const float xPositionHorizontal =  myRect.x0() + (scrollSize.x * xPercentageLocation);

		const Vector2 sizeHorizontal = getHorizontalScrollBarSize();

		const Vector2 positionHorizontal = Vector2(xPositionHorizontal, myRect.y1() - sizeHorizontal.y);
		horizontalBarSize = Rect2D::xywh(positionHorizontal, sizeHorizontal);
	}

	bool ScrollingFrame::scrollByPosition(const Vector2& inputDelta, bool printWarnings)
	{
		if (!scrollingEnabled)
		{
			return false;
		}

		const bool verticalScroll = canScrollVertical();
		const bool horizontalScroll = canScrollHorizontal();
		if (!verticalScroll && !horizontalScroll)
		{
			return false;
		}

		const bool bothScrollDir = (verticalScroll && horizontalScroll);
		const Rect2D myRect = getRect2D();
		const Vector2 scrollingFrameSize = getFrameScrollingSize();

		Vector2 oldCanvasPosition = canvasPosition;

		if ( verticalScroll )
		{
			const Rect2D currentVerticalBar = getVerticalBarRect();

			Vector2 newVerticalPos = currentVerticalBar.x0y0() - inputDelta;
				const float verticalX = verticalScrollBarPosition == Enums::VERTICAL_SCROLL_BAR_POSITION_LEFT
					? myRect.x0()
					: myRect.x1() - scrollBarThickness;
				newVerticalPos = newVerticalPos.clamp(Vector2(verticalX, myRect.y0()),
					Vector2(verticalX, myRect.y1() - currentVerticalBar.height() - (bothScrollDir ? scrollBarThickness : 0)));

			const float verticalPercent = (scrollingFrameSize.y > 0.0f) ? (newVerticalPos.y - myRect.y0())/scrollingFrameSize.y : 0.0f;
			setCanvasPosition( Vector2(canvasPosition.x, getCanvasScrollingSize().y * verticalPercent), printWarnings );
		}

		if (horizontalScroll)
		{
			const Rect2D currentHorizontalBar = getHorizontalBarRect();

			Vector2 newHorizontalPos = currentHorizontalBar.x0y0() - inputDelta;
			newHorizontalPos = newHorizontalPos.clamp(Vector2(myRect.x0(), myRect.y1() - scrollBarThickness), 
				Vector2(myRect.x1() - currentHorizontalBar.width() - (bothScrollDir ? scrollBarThickness : 0), myRect.y1() - scrollBarThickness));

			float horizontalPercent = (scrollingFrameSize.x > 0.0f) ? (newHorizontalPos.x - myRect.x0())/scrollingFrameSize.x : 0.0f;
			horizontalPercent = G3D::clamp(horizontalPercent,0.0f,1.0f);
			setCanvasPosition( Vector2(getCanvasScrollingSize().x * horizontalPercent, canvasPosition.y), printWarnings );
		}

		return oldCanvasPosition != canvasPosition;
	}
    
	void ScrollingFrame::resizeChildren()
	{
        if (getChildren())
        {
            for (Instances::const_iterator child_itr = getChildren()->begin(); child_itr != getChildren()->end(); ++child_itr)
            {
                if (GuiObject* child = Instance::fastDynamicCast<GuiObject>(child_itr->get()))
                {
                    child->checkForResize();
                }
            }
        }
	}

	Vector2 ScrollingFrame::getScrollWheelDelta(bool scrollingDownValue)
	{
		Vector2 scrollDelta(0, scrollingDownValue ? WHEEL_SCROLL_DELTA : -WHEEL_SCROLL_DELTA);
		double lastScrollDelta;

		if (scrollingDownValue)
		{
			lastScrollDelta = wheelDownTimer.delta().msec();
			wheelDownTimer.reset();
		}
		else
		{
			lastScrollDelta = wheelUpTimer.delta().msec();
			wheelUpTimer.reset();
		}

		if (lastScrollDelta <= MAX_MULTIPLY_TIME_MSEC)
		{
			scrollDelta *= Vector2(0,scrollDeltaMultiplier);

			if (scrollDeltaMultiplier == 1)
			{
				scrollDeltaMultiplier++;
			}
			else
			{
				scrollDeltaMultiplier *= 4;
			}

			if (DFFlag::LimitScrollWheelMaxToHalfWindowSize)
			{
				scrollDeltaMultiplier = std::min(float(scrollDeltaMultiplier), 
					std::min((getVisibleRect2D().height() * 0.5f)/WHEEL_SCROLL_DELTA, (getCanvasRectLocal().height() * MAX_SCROLL_WHEEL_SCROLL_PERCENT)/WHEEL_SCROLL_DELTA));
			}
			else
			{
				scrollDeltaMultiplier = std::min(1.0f * scrollDeltaMultiplier, (getCanvasRectLocal().height() * MAX_SCROLL_WHEEL_SCROLL_PERCENT)/WHEEL_SCROLL_DELTA);
			}
		}
		else
		{
			scrollDeltaMultiplier = 1;
		}

		return scrollDelta;
	}
    
    void ScrollingFrame::scrollDownWheel()
    {
		if (canScrollVertical())
		{
			Vector2 scrollDelta = getScrollWheelDelta(true);
			setCanvasPosition( canvasPosition + scrollDelta, false);
			lastInputPosition += scrollDelta;
		}
    }
    
    void ScrollingFrame::scrollUpWheel()
    {
		if (canScrollVertical())
		{
			Vector2 scrollDelta = getScrollWheelDelta(false);
			setCanvasPosition( canvasPosition + scrollDelta, false);
			lastInputPosition += scrollDelta;
		}
    }

	bool ScrollingFrame::canScrollVertical() const
	{
		return (allowedScrollingDirection == Enums::SCROLLING_DIRECTION_Y || allowedScrollingDirection == Enums::SCROLLING_DIRECTION_XY) &&
			( getCanvasRectLocal().height() > getRect2D().height() );
	}

	bool ScrollingFrame::canScrollHorizontal() const
	{
		return (allowedScrollingDirection == Enums::SCROLLING_DIRECTION_X || allowedScrollingDirection == Enums::SCROLLING_DIRECTION_XY) &&
			( getCanvasRectLocal().width() > getRect2D().width() );
	}
    
    bool ScrollingFrame::isInsideDraggingRect(const Vector2& inputPosition)
    {
        if (isTouchDevice())
        {
            return true;
        }

        if (scrollingDirection == X)
        {
			const Rect2D visibleRect = getVisibleRect2D();

            if ( (inputPosition.x < visibleRect.x0()) )
            {
				scrollByPosition(Vector2(REALLY_BIG_SCROLL_DELTA,0));
                return false;
            }
            if( (inputPosition.x > (visibleRect.x0() + visibleRect.width())) )
            {
				scrollByPosition(Vector2(-REALLY_BIG_SCROLL_DELTA,0));
                return false;
            }
        }
        else if (scrollingDirection == Y)
        {
			const Rect2D visibleRect = getVisibleRect2D();

            if ( (inputPosition.y < visibleRect.y0()) )
            {
				scrollByPosition(Vector2(0,REALLY_BIG_SCROLL_DELTA));
                return false;
            }
            if ( inputPosition.y > (visibleRect.y0() + visibleRect.height()) )
            {
				scrollByPosition(Vector2(0,-REALLY_BIG_SCROLL_DELTA), false);
                return false;
            }
        }
        
        return true;
    }
    
    void ScrollingFrame::handleInputDrag(const Vector2& inputPosition, const shared_ptr<InputObject>& event)
    {
        if (scrollDragging && inputPosition != lastInputPosition)
        {
			if (event && event->isGamepadEvent())
			{
				// don't do dragging rect comparison for gamepad
			}
            else if (!isInsideDraggingRect(inputPosition))
            {
                return;
            }
            
            Vector2 dragCoefficient;
            if (scrollingDirection == XY)
            {
                dragCoefficient = Vector2(1,1);
            }
            else
            {
                dragCoefficient = Vector2( (scrollingDirection == X) ? 1.0f : 0.0f, (scrollingDirection == Y) ? 1.0f : 0.0f);
            }

			if (event && event->isGamepadEvent())
			{
				lastGamepadScrollDelta = inputPosition * dragCoefficient;
				scrollByPosition(lastGamepadScrollDelta);
			}
			else
			{
				Vector2 inputDelta = inputPosition - lastInputPosition;
				Vector2 dragDelta = dragCoefficient * inputDelta;

				if (isTouchDevice()) // scroll by canvasPos if using finger
				{
					setCanvasPosition( canvasPosition - dragDelta, false);
				}
				else
				{
					scrollByPosition(-dragDelta);
				}
			}


            lastInputPosition = inputPosition;
        }
    }
    
    void ScrollingFrame::handleInputBegin(const Vector2& inputPosition, const ScrollingDirection& dragDirection)
    {
        scrollingDirection = dragDirection;
        lastInputPosition = inputPosition;
        scrollDragging = true;
    }

    bool ScrollingFrame::hasMouse() const
    {
        if (RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(this))
            return userInputService->getMouseEnabled();
        
        return false;
    }
    
    bool ScrollingFrame::isTouchDevice() const
    {
        if (RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(this))
			return userInputService->getTouchEnabled();
        
        return false;
    }
    
    void ScrollingFrame::setScrollingInertia()
    {
        sampledVelocity = Vector2::zero();
        
        if (touchDeltaTimer.delta().msec() < TOUCH_INERTIA_RESET_TIME_MSEC)
        {
            for (unsigned int i = 0; i < touchInputPositions.size(); ++i)
            {
                std::pair<Vector2, float> currPair = touchInputPositions[i];
                sampledVelocity += currPair.first/currPair.second;
            }
            sampledVelocity /= touchInputPositions.size();
            sampledVelocity = sampledVelocity.clamp(-getCanvasRectLocal().wh(), getCanvasRectLocal().wh());
        }
    }

    void ScrollingFrame::globalInputEnded(const shared_ptr<Instance>& event)
    {
        if (InputObject* inputObject = Instance::fastDynamicCast<InputObject>(event.get()))
        {
            if(touchInput.get() == inputObject)
            {
                touchInput.reset();
                touchScrolling = false;
                
                // input has ended, on touch give some extra scrolling inertia
                setScrollingInertia();
            }
			if (leftMouseDownInput.get() == inputObject)
			{
				leftMouseDownInput.reset();
				scrollDragging = false;
			}
        } 
        
    }

	bool ScrollingFrame::processInputFromDescendant(const shared_ptr<InputObject>& event)
	{
		GuiResponse response = process(event);
		return response.wasSunk();
	}

	bool ScrollingFrame::doProcessMouseEvent(const shared_ptr<InputObject>& event, const bool inputOver)
	{
		const bool mouseWheelForward = event->isMouseWheelForward();
		const bool mouseWheelBackward = event->isMouseWheelBackward();
		const bool eventLeftButtonUp = event->isLeftMouseUpEvent();
		const bool eventLeftButtonDown = event->isLeftMouseDownEvent();
		const bool wasDownOver = (guiState == Gui::DOWN_OVER);
		bool sinkEvent = (scrollDragging && eventLeftButtonUp && wasDownOver);

		if (eventLeftButtonDown)
		{
			isLeftMouseDown = true;
		}
		if (eventLeftButtonUp)
		{
			isLeftMouseDown = false;
		}

		if(eventLeftButtonDown)
		{
			if (verticalBarSize.contains(Vector2(event->get2DPosition())))
			{
				if (leftMouseDownInput == shared_ptr<InputObject>() && event->getUserInputState() == InputObject::INPUT_STATE_BEGIN)
				{
					leftMouseDownInput = shared_from(event.get());
				}
				handleInputBegin(event->get2DPosition(), Y);
			}
			else if (horizontalBarSize.contains(Vector2(event->get2DPosition())))
			{
				if (leftMouseDownInput == shared_ptr<InputObject>() && event->getUserInputState() == InputObject::INPUT_STATE_BEGIN)
				{
					leftMouseDownInput = shared_from(event.get());
				}
				handleInputBegin(event->get2DPosition(), X);
			}

			sinkEvent = scrollDragging;
		}

		if(inputOver)
		{
			if (eventLeftButtonUp && scrollDragging)          {sinkEvent = true;}
			if (mouseWheelBackward)         {sinkEvent = true; scrollDownWheel(); }
			if (mouseWheelForward)          {sinkEvent = true; scrollUpWheel();   }
		}

		if (isLeftMouseDown && event->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT)
			handleInputDrag(event->get2DPosition());
		if (eventLeftButtonUp)
			scrollDragging = false;

		return sinkEvent;
	}
    
	bool ScrollingFrame::doProcessTouchEvent(const shared_ptr<InputObject>& event, const bool inputOver)
	{
		bool sinkEvent = false;

		if (inputOver && touchInput == shared_ptr<InputObject>() && event->getUserInputState() == InputObject::INPUT_STATE_BEGIN)
		{
			touchInput = shared_from(event.get());
			handleInputBegin(event->get2DPosition(), XY);
            touchInputPositions.clear();
            touchDeltaTimer.reset();
            
            sinkEvent = true;
		}
		else if(touchInput.get() == event.get())
		{
			if (touchInput->getUserInputState() == InputObject::INPUT_STATE_CHANGE)
			{
                Vector2 delta = (event->get2DPosition() - lastInputPosition);
                if ( delta.length() > 0.0f && (canScrollVertical() || canScrollHorizontal()) )
                {
                    touchScrolling = true;
                
					std::pair<Vector2, float> positionDeltaTimePair(delta, (float)touchDeltaTimer.delta().seconds());
					touchInputPositions.push_back(positionDeltaTimePair);
                
					handleInputDrag(event->get2DPosition());
					touchDeltaTimer.reset();
				}
			}

			sinkEvent = true;
		}

		return sinkEvent;
	}

	bool ScrollingFrame::canScrollInDirection(Vector2 direction)
	{
		Vector2 maxSize = getMaxSize(); 
		return  canvasPosition != G3D::clamp(canvasPosition + direction, Vector2::zero(), maxSize.max(Vector2::zero()));
	}

	bool ScrollingFrame::doProcessGamepadEvent(const shared_ptr<InputObject>& event, const bool inputOver)
	{
		bool sinkEvent = false;

		GamepadService* gamepadService = RBX::ServiceProvider::find<GamepadService>(this);

		if (!gamepadService)
		{
			return sinkEvent;
		}

		if ( !gamepadService->isNavigationGamepad(event->getUserInputType()) )
		{
			return sinkEvent;
		}

		GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this);
		
		if (!guiService)
		{
			return sinkEvent;
		}

		GuiObject* selectedObject = guiService->getSelectedGuiObject();

		if (!selectedObject || selectedObject != this)
		{
			return sinkEvent;
		}

		if (event->getKeyCode() == SDLK_GAMEPAD_THUMBSTICK1 || event->isDPadEvent())
		{
			if (Instance::fastDynamicCast<GuiButton>(selectedObject) && selectedObject->isDescendantOf(this))
			{
				return sinkEvent;
			}

			sinkEvent = true;

			Vector2 scrollingDelta = event->get2DPosition();
			if (event->isDPadEvent() && event->getUserInputState() == InputObject::INPUT_STATE_BEGIN)
			{
				lastInputPosition = Vector2::zero();

				switch (event->getKeyCode())
				{
				case SDLK_GAMEPAD_DPADDOWN: scrollingDelta = Vector2(0,-1.0f); break;
				case SDLK_GAMEPAD_DPADUP:	scrollingDelta = Vector2(0,1.0f); break;
				case SDLK_GAMEPAD_DPADLEFT:	scrollingDelta = Vector2(-1.0f,0); break;
				case SDLK_GAMEPAD_DPADRIGHT:scrollingDelta = Vector2(1.0f,0); break;
				default:
					break;
				}
			}
			else if (event->getKeyCode() == SDLK_GAMEPAD_THUMBSTICK1)
			{
				fabs(scrollingDelta.x) > fabs(scrollingDelta.y) ? scrollingDelta.y = 0 : scrollingDelta.x = 0;
				if ( !canScrollInDirection(Vector2(scrollingDelta.x, -scrollingDelta.y)) )
				{
					lastGamepadScrollDelta = Vector2::zero();
					return selectionTimer.delta().msec() <= GAMEPAD_THUMBSTICK_WAIT_PERIOD_MSEC;
				}
			}

			Vector2 maxSize = getMaxSize();
			if (maxSize.x == 0.0f)
			{
				if (scrollingDelta.x != 0.0f)
				{
					sinkEvent = false;
					scrollingDelta.x = 0.0f;
				}
			}
			if (maxSize.y == 0.0f)
			{
				if (scrollingDelta.y != 0.0f)
				{
					sinkEvent = false;
					scrollingDelta.y = 0.0f;
				}
			}

			scrollDragging = scrollingDelta.length() > 0.3f;
			if (scrollDragging)
			{
				scrollingDirection = XY;
				handleInputDrag(Vector2(-scrollingDelta.x,scrollingDelta.y), event);
			}
			else
			{
				lastGamepadScrollDelta = Vector2::zero();
			}
		}

		return sinkEvent;
	}
    
    GuiResponse ScrollingFrame::process(const shared_ptr<InputObject>& event)
    {
        if (!scrollingEnabled || event->getUserInputState() == InputObject::INPUT_STATE_CANCEL)
        {
            return GuiResponse::notSunk();
		}

		Rect rect(getRect2D());
        
        if( GuiObject* clippingObject = firstAncestorClipping() )
        {
            RBX::Rect2D clippedRect = getRect2D().intersect(clippingObject->getClippedRect());
            rect = Rect(clippedRect);
        }
        
        const bool inputOver = rect.pointInRect(event->get2DPosition());
        bool sinkEvent = false;
        
        if (isTouchDevice() && event->isTouchEvent())
        {
			sinkEvent = doProcessTouchEvent(event, inputOver);
        }
		else if (event->isGamepadEvent())
		{
			sinkEvent = doProcessGamepadEvent(event, inputOver);
		}
		else if (hasMouse() && event->isMouseEvent())
		{
			sinkEvent = doProcessMouseEvent(event, inputOver);
		}
        
        if (sinkEvent)
        {
            return GuiResponse::sunk();
        }
        else
        {
            return Super::process(event);
        }
    }

	void ScrollingFrame::renderScrollbarLines(Adorn* adorn, const bool scrollVertical, const bool scrollHorizontal)
	{
		Rect2D rect = getRect2D();
		Rect2D clippedRect = rect;
		if( GuiObject* clippingObject = firstAncestorClipping())
		{
			const RBX::Rect2D clipRect = clippingObject->getClippedRect();
			clippedRect = clipRect.intersect(rect);
		}

		const Rotation2D rotation = getAbsoluteRotation();
		const int borderSize = getBorderSizePixel();

		// vertical line
		const float xPosForVerticalLine = rect.x1() - scrollBarThickness;
		if (scrollVertical && scrollBarThickness > 0 && xPosForVerticalLine < clippedRect.x1())
		{
			const float yBeginPos = std::max(rect.y0(), clippedRect.y0());
			const float yEndPos =  std::min(rect.y1(), clippedRect.y1());

			const Vector2 beginPos = Vector2(xPosForVerticalLine, yBeginPos);
			const Vector2 endPos = Vector2(xPosForVerticalLine, yEndPos);

			adorn->rect2d(Rect2D::xyxy(beginPos.x - borderSize, beginPos.y - borderSize, beginPos.x, endPos.y), borderColor, rotation);
		}

		// horizontal line
		const float yPosForHorizonalLine = rect.y1() - scrollBarThickness;
		if (scrollHorizontal && scrollBarThickness > 0 && yPosForHorizonalLine < clippedRect.y1())
		{
			const float xBeginPos = std::max(rect.x0(), clippedRect.x0());
			const float xEndPos =  std::min(rect.x1(), clippedRect.x1());

			const Vector2 beginPos = Vector2(xBeginPos, yPosForHorizonalLine);
			const Vector2 endPos = Vector2(xEndPos, yPosForHorizonalLine);

			adorn->rect2d(Rect2D::xyxy(beginPos.x, beginPos.y - borderSize, endPos.x + borderSize, beginPos.y), borderColor, rotation);
		}
	}

	void ScrollingFrame::renderScrollbarSection(Adorn* adorn, GuiDrawImage& drawImage, const TextureId& image, const char* context, const Rect2D& imageRect, const Rotation2D& rotation)
	{
		if (drawImage.setImage(adorn, image, GuiDrawImage::NORMAL, NULL, this, context))
		{
			Vector2 lowerUV(0,0);
			Vector2 upperUV(1,1);

			if( GuiObject* clippingObject = firstAncestorClipping())
			{
				const RBX::Rect2D clipRect = clippingObject->getClippedRect();
				RBX::Rect2D clippedRect = clipRect.intersect(imageRect);

				if (rotation != Rotation2D())
				{
					if (DFFlag::FixRotatedHorizontalScrollBar)
					{
						Rotation2D reverseRotation(rotation.getAngle().inverse(),imageRect.center());
						Vector2 clippedOriginRotated = reverseRotation.rotate(clipRect.x0y0());
						Vector2 clippedx1y1Rotated = reverseRotation.rotate(clipRect.x1y1());
						RBX::Rect2D clipRectRotated = Rect2D::xyxy(clippedOriginRotated, clippedx1y1Rotated);
						clippedRect = clipRectRotated.intersect(imageRect);
					}
					else
					{
						Rotation2D reverseRotation(rotation.getAngle().inverse(),imageRect.center());
						Vector2 clippedOriginRotated = reverseRotation.rotate(clippedRect.x0y0());
						Vector2 clippedx1y1Rotated = reverseRotation.rotate(clippedRect.x1y1());

						clippedRect = Rect2D::xyxy(clippedOriginRotated, clippedx1y1Rotated);
					}
				}

				if(clippedRect.width() != 0)
				{
					float uvwidth = upperUV.x - lowerUV.x;

					lowerUV.x += ( uvwidth * ( clippedRect.x0() - imageRect.x0() ) / imageRect.width() );
					upperUV.x += ( uvwidth * ( clippedRect.x1() - imageRect.x1() ) / imageRect.width() );
				}

				if(clippedRect.height() != 0)
				{
					float uvheight = upperUV.y - lowerUV.y;

					lowerUV.y += ( uvheight * ( clippedRect.y0() - imageRect.y0() ) / imageRect.height() );
					upperUV.y += ( uvheight * ( clippedRect.y1() - imageRect.y1() ) / imageRect.height() );
				}

				if (clippedRect != RBX::Rect2D())
				{
					drawImage.render2d(adorn, 
						true, 
						clippedRect,
						lowerUV, upperUV,
						applyCanvasGroup(Color4(scrollBarImageColor, 1.0f - scrollBarImageTransparency)), 
						rotation,
						Gui::NOTHING, 
						false);
				}
			}
			else
			{
				Rect2D rotatedImageRect = imageRect;

				if (!rotation.getAngle().empty() && !getAbsoluteRotation().getAngle().empty())
				{
					Vector2 x0y0 = rotation.rotate(imageRect.x0y0());
					Vector2 x1y1 = rotation.rotate(imageRect.x1y1());
					rotatedImageRect = Rect2D::xyxy(x0y0,x1y1);
				}

				drawImage.render2d(adorn, 
					true, 
					rotatedImageRect,
					lowerUV, upperUV,
					applyCanvasGroup(Color4(scrollBarImageColor, 1.0f - scrollBarImageTransparency)), 
					getAbsoluteRotation().getAngle().empty() ? rotation : getAbsoluteRotation(),
					Gui::NOTHING, 
					false);
			}

		}
	}

	void ScrollingFrame::renderHorizontalScrollBar(Adorn* adorn)
	{
		Rect2D lastHorizontalBarSize = getHorizontalBarRect();

		//////////////////////////////////////////////////////////////////////////
		// Top Image
		Rect2D topImageRect = Rect2D::xywh(lastHorizontalBarSize.x0y0(),
											Vector2(scrollBarThickness, lastHorizontalBarSize.height()));

		renderScrollbarSection(adorn,
                                hTopImage,
								topImage,
								".TopImage",
								topImageRect,
								Rotation2D(RotationAngle(-90.0f),topImageRect.center()) );

		//////////////////////////////////////////////////////////////////////////
		// Mid Image
		Vector2 midImageRectSize = Vector2(lastHorizontalBarSize.height(), std::max(MIN_SCROLLBAR_SIZE, lastHorizontalBarSize.width() - (scrollBarThickness*2)));
		Vector2 midImagePosition = lastHorizontalBarSize.center() - Vector2(scrollBarThickness/2, midImageRectSize.y/2);
		Rect2D midImageRect = Rect2D::xywh(midImagePosition, midImageRectSize);

		renderScrollbarSection(adorn,
                                hMidImage,
								midImage,
								".MidImage",
								midImageRect,
                                Rotation2D(RotationAngle(-90.0f),midImageRect.center()) );

		//////////////////////////////////////////////////////////////////////////
		// Bottom Image
		Rect2D bottomImageRect = Rect2D::xywh(lastHorizontalBarSize.x0y0() + Vector2(lastHorizontalBarSize.width() - scrollBarThickness, 0),
												Vector2(scrollBarThickness, lastHorizontalBarSize.height()));

		renderScrollbarSection(adorn,
                                hBottomImage,
								bottomImage,
								".BottomImage",
								bottomImageRect,
                                Rotation2D(RotationAngle(-90.0f),bottomImageRect.center()) );
	}

	void ScrollingFrame::renderVerticalScrollBar(Adorn* adorn)
	{
		Rect2D lastVerticalBarSize = getVerticalBarRect();
        
		Rect2D topRect = Rect2D::xywh(lastVerticalBarSize.x0y0(), Vector2(scrollBarThickness, scrollBarThickness));
		renderScrollbarSection(adorn, 
								vTopImage,
								topImage,
								".TopImage",
								topRect,
								Rotation2D());

		Rect2D midRect = Rect2D::xywh(topRect.x0y0() + Vector2(0, topRect.height()), 
										Vector2(scrollBarThickness, std::max(MIN_SCROLLBAR_SIZE, lastVerticalBarSize.height() - (scrollBarThickness*2))));
		renderScrollbarSection(adorn, 
								vMidImage,
								midImage,
								".MidImage",
								midRect,
								Rotation2D());

		Rect2D bottomRect = Rect2D::xywh(midRect.x0y0() + Vector2(0, midRect.height()), Vector2(scrollBarThickness, scrollBarThickness));
		renderScrollbarSection(adorn, 
								vBottomImage,
								bottomImage,
								".BottomImage",
								bottomRect,
								Rotation2D());
	}
    
    void ScrollingFrame::render2d(Adorn* adorn)
    {
        Super::render2d(adorn);

		const bool scrollVertical = canScrollVertical();
		const bool scrollHorizontal = canScrollHorizontal();

		renderScrollbarLines(adorn, scrollVertical, scrollHorizontal);
        
        // render vertical scrollbar
        if (scrollVertical && scrollBarThickness > 0)
        {
			renderVerticalScrollBar(adorn);
        }
        
        // render horizontal scrollbar
        if (scrollHorizontal && scrollBarThickness > 0)
        {
			renderHorizontalScrollBar(adorn);
        }
    }

	bool checkDescendantsForInteractable(Instance* instance) 
	{
		if (instance->getChildren())
		{
			Instances::const_iterator end = instance->getChildren()->end();
			for (Instances::const_iterator iter = instance->getChildren()->begin(); iter != end; ++iter)
			{
				if (Instance::fastDynamicCast<GuiButton>((*iter).get()) || Instance::fastDynamicCast<TextBox>((*iter).get()))
				{
					if (Instance::fastDynamicCast<GuiObject>((*iter).get()))
					{
						return true;
					}
					else
					{
						return checkDescendantsForInteractable((*iter).get());
					}
				}
			}
		}

		return false;
	}

	bool ScrollingFrame::hasInteractableDescendants()
	{
		return checkDescendantsForInteractable(this);
	}
} // namespace RBX
