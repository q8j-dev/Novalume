#pragma once

#include "V8DataModel/GuiBase2d.h"
#include "V8DataModel/GuiObject.h"
#include "V8DataModel/ColorSequence.h"
#include "V8DataModel/NumberSequence.h"
#include "Script/ThreadRef.h"
#include "Util/UDim.h"
#include "Util/SteppedInstance.h"

#include <vector>

namespace RBX
{
    class GuiObject;
    class InputObject;
    class UIGradient;

    enum FillDirection
    {
        FILL_DIRECTION_HORIZONTAL,
        FILL_DIRECTION_VERTICAL
    };

    enum HorizontalAlignment
    {
        HORIZONTAL_ALIGNMENT_CENTER,
        HORIZONTAL_ALIGNMENT_LEFT,
        HORIZONTAL_ALIGNMENT_RIGHT
    };

    enum VerticalAlignment
    {
        VERTICAL_ALIGNMENT_CENTER,
        VERTICAL_ALIGNMENT_TOP,
        VERTICAL_ALIGNMENT_BOTTOM
    };

    enum SortOrder
    {
        SORT_ORDER_NAME,
        SORT_ORDER_LAYOUT_ORDER
    };

    enum StartCorner
    {
        START_CORNER_TOP_LEFT,
        START_CORNER_TOP_RIGHT,
        START_CORNER_BOTTOM_LEFT,
        START_CORNER_BOTTOM_RIGHT
    };

    enum AspectType
    {
        ASPECT_TYPE_FIT_WITHIN_MAX_SIZE,
        ASPECT_TYPE_SCALE_WITH_PARENT_SIZE
    };

    enum DominantAxis
    {
        DOMINANT_AXIS_WIDTH,
        DOMINANT_AXIS_HEIGHT
    };

    enum UIFlexAlignment
    {
        UI_FLEX_ALIGNMENT_NONE,
        UI_FLEX_ALIGNMENT_FILL,
        UI_FLEX_ALIGNMENT_SPACE_AROUND,
        UI_FLEX_ALIGNMENT_SPACE_BETWEEN,
        UI_FLEX_ALIGNMENT_SPACE_EVENLY
    };

    enum ItemLineAlignment
    {
        ITEM_LINE_ALIGNMENT_AUTOMATIC,
        ITEM_LINE_ALIGNMENT_START,
        ITEM_LINE_ALIGNMENT_CENTER,
        ITEM_LINE_ALIGNMENT_END,
        ITEM_LINE_ALIGNMENT_STRETCH
    };

    enum UIFlexMode
    {
        UI_FLEX_MODE_NONE,
        UI_FLEX_MODE_GROW,
        UI_FLEX_MODE_SHRINK,
        UI_FLEX_MODE_FILL,
        UI_FLEX_MODE_CUSTOM
    };

    enum ApplyStrokeMode
    {
        APPLY_STROKE_MODE_CONTEXTUAL,
        APPLY_STROKE_MODE_BORDER
    };

    enum LineJoinMode
    {
        LINE_JOIN_MODE_ROUND,
        LINE_JOIN_MODE_BEVEL,
        LINE_JOIN_MODE_MITER
    };

    enum BorderStrokePosition
    {
        BORDER_STROKE_POSITION_OUTER,
        BORDER_STROKE_POSITION_CENTER,
        BORDER_STROKE_POSITION_INNER
    };

    enum StrokeSizingMode
    {
        STROKE_SIZING_MODE_FIXED_SIZE,
        STROKE_SIZING_MODE_SCALED_SIZE
    };

    enum UIDragDetectorBoundingBehavior
    {
        UI_DRAG_BOUNDING_ENTIRE_OBJECT,
        UI_DRAG_BOUNDING_HIT_POINT
    };

    enum UIDragDetectorDragRelativity
    {
        UI_DRAG_RELATIVITY_ABSOLUTE,
        UI_DRAG_RELATIVITY_RELATIVE
    };

    enum UIDragDetectorDragSpace
    {
        UI_DRAG_SPACE_PARENT,
        UI_DRAG_SPACE_LAYER_COLLECTOR,
        UI_DRAG_SPACE_REFERENCE
    };

    enum UIDragDetectorDragStyle
    {
        UI_DRAG_STYLE_TRANSLATE_PLANE,
        UI_DRAG_STYLE_TRANSLATE_LINE,
        UI_DRAG_STYLE_ROTATE,
        UI_DRAG_STYLE_SCRIPTABLE
    };

    enum UIDragDetectorResponseStyle
    {
        UI_DRAG_RESPONSE_OFFSET,
        UI_DRAG_RESPONSE_SCALE,
        UI_DRAG_RESPONSE_CUSTOM_OFFSET,
        UI_DRAG_RESPONSE_CUSTOM_SCALE
    };

    extern const char* const sUIComponent;
    class UIComponent : public DescribedNonCreatable<UIComponent, GuiBase2d, sUIComponent>
    {
    public:
        UIComponent(const char* name);

        virtual void applyLayout(const GuiObject* child, const Rect2D& viewport,
            Vector2& position, Vector2& size) const {}

    protected:
        /*override*/ bool askSetParent(const Instance* instance) const;
        /*override*/ bool askAddChild(const Instance* instance) const;
        void invalidateParentLayout();
    };

    extern const char* const sUICorner;
    class UICorner : public DescribedCreatable<UICorner, UIComponent, sUICorner>
    {
    public:
        UICorner();
        UDim getCornerRadius() const { return cornerRadius; }
        void setCornerRadius(UDim value);
        float getRadius(const Vector2& size) const;
    private:
        UDim cornerRadius;
    };

    extern const char* const sUIPadding;
    class UIPadding : public DescribedCreatable<UIPadding, UIComponent, sUIPadding>
    {
    public:
        UIPadding();
        UDim getPaddingLeft() const { return paddingLeft; }
        UDim getPaddingRight() const { return paddingRight; }
        UDim getPaddingTop() const { return paddingTop; }
        UDim getPaddingBottom() const { return paddingBottom; }
        void setPaddingLeft(UDim value);
        void setPaddingRight(UDim value);
        void setPaddingTop(UDim value);
        void setPaddingBottom(UDim value);
        void getInsets(const Vector2& parentSize, float& left, float& right,
            float& top, float& bottom) const;
    private:
        UDim paddingLeft;
        UDim paddingRight;
        UDim paddingTop;
        UDim paddingBottom;
    };

    extern const char* const sUIScale;
    class UIScale : public DescribedCreatable<UIScale, UIComponent, sUIScale>
    {
    public:
        UIScale();
        float getScale() const { return scale; }
        void setScale(float value);
    private:
        float scale;
    };

    extern const char* const sUIFlexItem;
    class UIFlexItem : public DescribedCreatable<UIFlexItem, UIComponent, sUIFlexItem>
    {
    public:
        UIFlexItem();
        UIFlexMode getFlexMode() const { return flexMode; }
        void setFlexMode(UIFlexMode value);
        float getGrowRatio() const { return growRatio; }
        void setGrowRatio(float value);
        float getShrinkRatio() const { return shrinkRatio; }
        void setShrinkRatio(float value);
        ItemLineAlignment getItemLineAlignment() const { return itemLineAlignment; }
        void setItemLineAlignment(ItemLineAlignment value);
        float effectiveGrowRatio() const;
        float effectiveShrinkRatio() const;
    private:
        void invalidateContainerLayout();
        UIFlexMode flexMode;
        float growRatio;
        float shrinkRatio;
        ItemLineAlignment itemLineAlignment;
    };

    extern const char* const sUIStroke;
    class UIStroke : public DescribedCreatable<UIStroke, UIComponent, sUIStroke>
    {
    public:
        UIStroke();
        ApplyStrokeMode getApplyStrokeMode() const { return applyStrokeMode; }
        void setApplyStrokeMode(ApplyStrokeMode value);
        UDim getBorderOffset() const { return borderOffset; }
        void setBorderOffset(UDim value);
        BorderStrokePosition getBorderStrokePosition() const { return borderStrokePosition; }
        void setBorderStrokePosition(BorderStrokePosition value);
        Color3 getColor() const { return color; }
        void setColor(Color3 value);
        bool getEnabled() const { return enabled; }
        void setEnabled(bool value);
        LineJoinMode getLineJoinMode() const { return lineJoinMode; }
        void setLineJoinMode(LineJoinMode value);
        StrokeSizingMode getStrokeSizingMode() const { return strokeSizingMode; }
        void setStrokeSizingMode(StrokeSizingMode value);
        float getThickness() const { return thickness; }
        void setThickness(float value);
        float getTransparency() const { return transparency; }
        void setTransparency(float value);
        int getZIndex() const { return zIndex; }
        void setZIndex(int value);
        float resolveThickness(const Vector2& parentSize) const;
        float resolveBorderOffset(const Vector2& parentSize) const;
    private:
        ApplyStrokeMode applyStrokeMode;
        UDim borderOffset;
        BorderStrokePosition borderStrokePosition;
        Color3 color;
        bool enabled;
        LineJoinMode lineJoinMode;
        StrokeSizingMode strokeSizingMode;
        float thickness;
        float transparency;
        int zIndex;
    };

    extern const char* const sUIGradient;
    class UIGradient : public DescribedCreatable<UIGradient, UIComponent, sUIGradient>
    {
    public:
        UIGradient();
        const ColorSequence& getColor() const { return color; }
        void setColor(const ColorSequence& value);
        const NumberSequence& getTransparency() const { return transparency; }
        void setTransparency(const NumberSequence& value);
        bool getEnabled() const { return enabled; }
        void setEnabled(bool value);
        Vector2 getOffset() const { return offset; }
        void setOffset(Vector2 value);
        float getRotation() const { return rotation; }
        void setRotation(float value);

        float unclampedParameterAt(const Vector2& point, const Rect2D& bounds) const;
        float parameterAt(const Vector2& point, const Rect2D& bounds) const;
        Color4 sample(float parameter, const Color4& baseColor) const;
        Color4 sampleAt(const Vector2& point, const Rect2D& bounds, const Color4& baseColor) const;

    protected:
        /*override*/ bool askSetParent(const Instance* instance) const;

    private:
        ColorSequence color;
        NumberSequence transparency;
        bool enabled;
        Vector2 offset;
        float rotation;
    };

    extern const char* const sUIDragDetector;
    class UIDragDetector : public DescribedCreatable<UIDragDetector, UIComponent, sUIDragDetector>
    {
    public:
        UIDragDetector();

        bool getEnabled() const { return enabled; }
        void setEnabled(bool value);
        UIDragDetectorBoundingBehavior getBoundingBehavior() const { return boundingBehavior; }
        void setBoundingBehavior(UIDragDetectorBoundingBehavior value);
        GuiBase2d* getBoundingUI() const;
        void setBoundingUI(GuiBase2d* value);
        Vector2 getDragAxis() const { return dragAxis; }
        void setDragAxis(Vector2 value);
        UIDragDetectorDragRelativity getDragRelativity() const { return dragRelativity; }
        void setDragRelativity(UIDragDetectorDragRelativity value);
        float getDragRotation() const { return dragRotation; }
        void setDragRotation(float value);
        UIDragDetectorDragSpace getDragSpace() const { return dragSpace; }
        void setDragSpace(UIDragDetectorDragSpace value);
        UIDragDetectorDragStyle getDragStyle() const { return dragStyle; }
        void setDragStyle(UIDragDetectorDragStyle value);
        UDim2 getDragUDim2() const { return dragUDim2; }
        void setDragUDim2(UDim2 value);
        float getMaxDragAngle() const { return maxDragAngle; }
        void setMaxDragAngle(float value);
        UDim2 getMaxDragTranslation() const { return maxDragTranslation; }
        void setMaxDragTranslation(UDim2 value);
        float getMinDragAngle() const { return minDragAngle; }
        void setMinDragAngle(float value);
        UDim2 getMinDragTranslation() const { return minDragTranslation; }
        void setMinDragTranslation(UDim2 value);
        GuiObject* getReferenceUIInstance() const;
        void setReferenceUIInstance(GuiObject* value);
        UIDragDetectorResponseStyle getResponseStyle() const { return responseStyle; }
        void setResponseStyle(UIDragDetectorResponseStyle value);

        rbx::signals::connection addConstraintFunction(int priority, Lua::WeakFunctionRef function);
        UDim2 getReferencePosition();
        float getReferenceRotation();
        void setDragStyleFunction(Lua::WeakFunctionRef function);

        void beginDrag(const Vector2& inputPosition);
        void continueDrag(const Vector2& inputPosition);
        void endDrag(const Vector2& inputPosition);
        bool isDragging() const { return dragging; }

        rbx::signal<void(Vector2)> dragStartSignal;
        rbx::signal<void(Vector2)> dragContinueSignal;
        rbx::signal<void(Vector2)> dragEndSignal;

    protected:
        /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    private:
        typedef DescribedCreatable<UIDragDetector, UIComponent, sUIDragDetector> Super;
        struct ConstraintFunction
        {
            int priority;
            Lua::WeakFunctionRef function;
            rbx::signals::connection lifetime;
        };

        void parentInputBegan(shared_ptr<Instance> event);
        void globalInputChanged(shared_ptr<Instance> event);
        void globalInputEnded(shared_ptr<Instance> event);
        void connectInput();
        bool invokeDragStyleFunction(const Vector2& inputPosition, UDim2& translation,
            float& rotation, UIDragDetectorDragRelativity& relativity,
            UIDragDetectorDragSpace& space);
        bool invokeConstraint(Lua::WeakFunctionRef& function, UDim2& translation,
            float& rotation, UIDragDetectorDragRelativity& relativity,
            UIDragDetectorDragSpace& space);
        void applyMotion(UDim2 translation, float rotation,
            UIDragDetectorDragRelativity relativity, UIDragDetectorDragSpace space);

        bool enabled;
        UIDragDetectorBoundingBehavior boundingBehavior;
        weak_ptr<GuiBase2d> boundingUI;
        Vector2 dragAxis;
        UIDragDetectorDragRelativity dragRelativity;
        float dragRotation;
        UIDragDetectorDragSpace dragSpace;
        UIDragDetectorDragStyle dragStyle;
        UDim2 dragUDim2;
        float maxDragAngle;
        UDim2 maxDragTranslation;
        float minDragAngle;
        UDim2 minDragTranslation;
        weak_ptr<GuiObject> referenceUIInstance;
        UIDragDetectorResponseStyle responseStyle;
        Lua::WeakFunctionRef dragStyleFunction;
        std::vector<ConstraintFunction> constraintFunctions;
        rbx::signal<void()> constraintLifetimeSignal;

        bool dragging;
        Vector2 initialInputPosition;
        UDim2 initialParentPosition;
        float initialParentRotation;
        shared_ptr<InputObject> activeInput;
        rbx::signals::scoped_connection parentInputBeganConnection;
        rbx::signals::scoped_connection globalInputChangedConnection;
        rbx::signals::scoped_connection globalInputEndedConnection;
    };

    extern const char* const sUIListLayout;
    class UIListLayout : public DescribedCreatable<UIListLayout, UIComponent, sUIListLayout>
    {
    public:
        UIListLayout();
        FillDirection getFillDirection() const { return fillDirection; }
        void setFillDirection(FillDirection value);
        HorizontalAlignment getHorizontalAlignment() const { return horizontalAlignment; }
        void setHorizontalAlignment(HorizontalAlignment value);
        VerticalAlignment getVerticalAlignment() const { return verticalAlignment; }
        void setVerticalAlignment(VerticalAlignment value);
        SortOrder getSortOrder() const { return sortOrder; }
        void setSortOrder(SortOrder value);
        UDim getPadding() const { return padding; }
        void setPadding(UDim value);
        UIFlexAlignment getHorizontalFlex() const { return horizontalFlex; }
        void setHorizontalFlex(UIFlexAlignment value);
        UIFlexAlignment getVerticalFlex() const { return verticalFlex; }
        void setVerticalFlex(UIFlexAlignment value);
        ItemLineAlignment getItemLineAlignment() const { return itemLineAlignment; }
        void setItemLineAlignment(ItemLineAlignment value);
        bool getWraps() const { return wraps; }
        void setWraps(bool value);
        Vector2 getAbsoluteContentSize() const;
        /*override*/ void applyLayout(const GuiObject* child, const Rect2D& viewport,
            Vector2& position, Vector2& size) const;
    private:
        FillDirection fillDirection;
        HorizontalAlignment horizontalAlignment;
        VerticalAlignment verticalAlignment;
        SortOrder sortOrder;
        UDim padding;
        UIFlexAlignment horizontalFlex;
        UIFlexAlignment verticalFlex;
        ItemLineAlignment itemLineAlignment;
        bool wraps;
    };

    extern const char* const sUIGridLayout;
    class UIGridLayout : public DescribedCreatable<UIGridLayout, UIComponent, sUIGridLayout>
    {
    public:
        UIGridLayout();
        UDim2 getCellPadding() const { return cellPadding; }
        void setCellPadding(UDim2 value);
        UDim2 getCellSize() const { return cellSize; }
        void setCellSize(UDim2 value);
        int getFillDirectionMaxCells() const { return fillDirectionMaxCells; }
        void setFillDirectionMaxCells(int value);
        FillDirection getFillDirection() const { return fillDirection; }
        void setFillDirection(FillDirection value);
        HorizontalAlignment getHorizontalAlignment() const { return horizontalAlignment; }
        void setHorizontalAlignment(HorizontalAlignment value);
        VerticalAlignment getVerticalAlignment() const { return verticalAlignment; }
        void setVerticalAlignment(VerticalAlignment value);
        SortOrder getSortOrder() const { return sortOrder; }
        void setSortOrder(SortOrder value);
        StartCorner getStartCorner() const { return startCorner; }
        void setStartCorner(StartCorner value);
        Vector2 getAbsoluteContentSize() const;
        Vector2 getAbsoluteCellSize() const;
        Vector2 getAbsoluteCellCount() const;
        /*override*/ void applyLayout(const GuiObject* child, const Rect2D& viewport,
            Vector2& position, Vector2& size) const;
    private:
        UDim2 cellPadding;
        UDim2 cellSize;
        int fillDirectionMaxCells;
        FillDirection fillDirection;
        HorizontalAlignment horizontalAlignment;
        VerticalAlignment verticalAlignment;
        SortOrder sortOrder;
        StartCorner startCorner;
    };

    extern const char* const sUIPageLayout;
    class UIPageLayout
        : public DescribedCreatable<UIPageLayout, UIComponent, sUIPageLayout>
        , public IStepped
    {
    public:
        UIPageLayout();

        bool getAnimated() const { return animating; }
        bool getCircular() const { return circular; }
        void setCircular(bool value);
        GuiObject* getCurrentPage() const;
        GuiObject::TweenEasingDirection getEasingDirection() const { return easingDirection; }
        void setEasingDirection(GuiObject::TweenEasingDirection value);
        GuiObject::TweenEasingStyle getEasingStyle() const { return easingStyle; }
        void setEasingStyle(GuiObject::TweenEasingStyle value);
        bool getGamepadInputEnabled() const { return gamepadInputEnabled; }
        void setGamepadInputEnabled(bool value);
        HorizontalAlignment getHorizontalAlignment() const { return horizontalAlignment; }
        void setHorizontalAlignment(HorizontalAlignment value);
        UDim getPadding() const { return padding; }
        void setPadding(UDim value);
        bool getScrollWheelInputEnabled() const { return scrollWheelInputEnabled; }
        void setScrollWheelInputEnabled(bool value);
        SortOrder getSortOrder() const { return sortOrder; }
        void setSortOrder(SortOrder value);
        bool getTouchInputEnabled() const { return touchInputEnabled; }
        void setTouchInputEnabled(bool value);
        float getTweenTime() const { return tweenTime; }
        void setTweenTime(float value);
        VerticalAlignment getVerticalAlignment() const { return verticalAlignment; }
        void setVerticalAlignment(VerticalAlignment value);

        void applyLayoutNow();
        void jumpTo(shared_ptr<Instance> page);
        void jumpToIndex(int index);
        void next();
        void previous();
        void stepAnimation(double timeStep);

        rbx::signal<void(shared_ptr<Instance>)> pageEnterSignal;
        rbx::signal<void(shared_ptr<Instance>)> pageLeaveSignal;
        rbx::signal<void(shared_ptr<Instance>)> stoppedSignal;

        /*override*/ void applyLayout(const GuiObject* child, const Rect2D& viewport,
            Vector2& position, Vector2& size) const;

    protected:
        /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
        /*override*/ void onStepped(const Stepped& event);

    private:
        typedef DescribedCreatable<UIPageLayout, UIComponent, sUIPageLayout> Super;

        void selectPage(size_t index, const std::vector<const GuiObject*>& children);
        void reconcilePages(const std::vector<const GuiObject*>& children);
        double visualLogicalPosition() const;
        float easedProgress() const;
        void finishAnimation();

        bool circular;
        weak_ptr<GuiObject> currentPage;
        GuiObject::TweenEasingDirection easingDirection;
        GuiObject::TweenEasingStyle easingStyle;
        bool gamepadInputEnabled;
        HorizontalAlignment horizontalAlignment;
        UDim padding;
        bool scrollWheelInputEnabled;
        SortOrder sortOrder;
        bool touchInputEnabled;
        float tweenTime;
        VerticalAlignment verticalAlignment;

        bool animating;
        double animationElapsed;
        double animationStartLogical;
        double animationEndLogical;
    };

    extern const char* const sUITextSizeConstraint;
    class UITextSizeConstraint : public DescribedCreatable<UITextSizeConstraint, UIComponent, sUITextSizeConstraint>
    {
    public:
        UITextSizeConstraint();
        int getMinTextSize() const { return minTextSize; }
        void setMinTextSize(int value);
        int getMaxTextSize() const { return maxTextSize; }
        void setMaxTextSize(int value);
    private:
        int minTextSize;
        int maxTextSize;
    };

    extern const char* const sUISizeConstraint;
    class UISizeConstraint : public DescribedCreatable<UISizeConstraint, UIComponent, sUISizeConstraint>
    {
    public:
        UISizeConstraint();
        Vector2 getMinSize() const { return minSize; }
        void setMinSize(Vector2 value);
        Vector2 getMaxSize() const { return maxSize; }
        void setMaxSize(Vector2 value);
        Vector2 constrain(const Vector2& size) const;
    private:
        Vector2 minSize;
        Vector2 maxSize;
    };

    extern const char* const sUIAspectRatioConstraint;
    class UIAspectRatioConstraint : public DescribedCreatable<UIAspectRatioConstraint, UIComponent, sUIAspectRatioConstraint>
    {
    public:
        UIAspectRatioConstraint();
        float getAspectRatio() const { return aspectRatio; }
        void setAspectRatio(float value);
        AspectType getAspectType() const { return aspectType; }
        void setAspectType(AspectType value);
        DominantAxis getDominantAxis() const { return dominantAxis; }
        void setDominantAxis(DominantAxis value);
        Vector2 constrain(const Vector2& size, const Vector2& parentSize) const;
    private:
        float aspectRatio;
        AspectType aspectType;
        DominantAxis dominantAxis;
    };

    const UIPadding* findUIPadding(const GuiObject* parent);
    const UIScale* findUIScale(const GuiObject* parent);
    const UIComponent* findUILayout(const GuiObject* parent);
    const UICorner* findUICorner(const GuiObject* parent);
    const UITextSizeConstraint* findUITextSizeConstraint(const GuiObject* parent);
    const UISizeConstraint* findUISizeConstraint(const GuiObject* parent);
    const UIAspectRatioConstraint* findUIAspectRatioConstraint(const GuiObject* parent);
    const UIFlexItem* findUIFlexItem(const GuiObject* parent);
    const UIGradient* findUIGradient(const Instance* parent);
    std::vector<const UIStroke*> findUIStrokes(const GuiObject* parent);
}
