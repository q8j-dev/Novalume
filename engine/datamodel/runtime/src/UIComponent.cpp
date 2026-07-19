#include "V8DataModel/UIComponent.h"

#include "V8DataModel/GuiObject.h"
#include "V8DataModel/InputObject.h"
#include "V8DataModel/UserInputService.h"
#include "Script/ScriptContext.h"
#include "Util/StandardOut.h"
#include "rbx/ui/ScreenLayout.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace RBX
{
const char* const sUIComponent = "UIComponent";
const char* const sUICorner = "UICorner";
const char* const sUIPadding = "UIPadding";
const char* const sUIScale = "UIScale";
const char* const sUIFlexItem = "UIFlexItem";
const char* const sUIStroke = "UIStroke";
const char* const sUIGradient = "UIGradient";
const char* const sUIDragDetector = "UIDragDetector";
const char* const sUIListLayout = "UIListLayout";
const char* const sUIGridLayout = "UIGridLayout";
const char* const sUIPageLayout = "UIPageLayout";
const char* const sUITextSizeConstraint = "UITextSizeConstraint";
const char* const sUISizeConstraint = "UISizeConstraint";
const char* const sUIAspectRatioConstraint = "UIAspectRatioConstraint";

namespace
{
float resolve(const UDim& value, float extent)
{
    return value.scale * extent + value.offset;
}

std::vector<const GuiObject*> orderedChildren(const Instance* parent, SortOrder order)
{
    std::vector<const GuiObject*> result;
    const copy_on_write_ptr<Instances>& children = parent->getChildren();
    if (!children)
        return result;

    for (Instances::const_iterator it = children->begin(); it != children->end(); ++it)
    {
        const GuiObject* child = Instance::fastDynamicCast<GuiObject>(it->get());
        if (child && child->getVisible())
            result.push_back(child);
    }

    std::stable_sort(result.begin(), result.end(), [order](const GuiObject* left, const GuiObject* right) {
        if (order == SORT_ORDER_LAYOUT_ORDER && left->getLayoutOrder() != right->getLayoutOrder())
            return left->getLayoutOrder() < right->getLayoutOrder();
        return left->getName() < right->getName();
    });
    return result;
}

Vector2 authoredSize(const GuiObject* child, const Vector2& viewportSize)
{
    Vector2 result = child->getSize() * viewportSize;
    if (child->getAutomaticSize() == AUTOMATIC_SIZE_X ||
        child->getAutomaticSize() == AUTOMATIC_SIZE_XY)
        result.x = std::max(result.x, child->getAbsoluteSize().x);
    if (child->getAutomaticSize() == AUTOMATIC_SIZE_Y ||
        child->getAutomaticSize() == AUTOMATIC_SIZE_XY)
        result.y = std::max(result.y, child->getAbsoluteSize().y);
    // Layout owns the final child size passed to GuiObject::recalculateAbsolutePlacement.
    // Its basis must therefore include the same constraints that recalculate applies
    // after layout.  Otherwise a square aspect-ratio item authored as Size=1,1 uses
    // the automatic parent's provisional height as its list basis, even though the
    // child is ultimately constrained to the parent's width.  That stale basis feeds
    // back into AbsoluteContentSize and inflates the automatic list indefinitely.
    if (const UISizeConstraint* constraint = findUISizeConstraint(child))
        result = constraint->constrain(result);
    if (const UIAspectRatioConstraint* constraint = findUIAspectRatioConstraint(child))
        result = constraint->constrain(result, viewportSize);
    return result;
}

size_t childIndex(const std::vector<const GuiObject*>& children, const GuiObject* child)
{
    const std::vector<const GuiObject*>::const_iterator found =
        std::find(children.begin(), children.end(), child);
    return found == children.end() ? children.size() : static_cast<size_t>(found - children.begin());
}
}

UIComponent::UIComponent(const char* name)
    : DescribedNonCreatable<UIComponent, GuiBase2d, sUIComponent>(name)
{
}

bool UIComponent::askSetParent(const Instance* instance) const
{
    return Instance::fastDynamicCast<GuiObject>(instance) != NULL;
}

bool UIComponent::askAddChild(const Instance* instance) const
{
    return Instance::fastDynamicCast<const UIStroke>(this) != NULL &&
        Instance::fastDynamicCast<UIGradient>(instance) != NULL;
}

void UIComponent::invalidateParentLayout()
{
    if (GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent()))
        parent->invalidateLayout();
    else if (UIComponent* component = Instance::fastDynamicCast<UIComponent>(getParent()))
        if (GuiObject* parent = Instance::fastDynamicCast<GuiObject>(component->getParent()))
            parent->invalidateLayout();
}

UICorner::UICorner()
    : DescribedCreatable<UICorner, UIComponent, sUICorner>("UICorner")
    , cornerRadius(0.0f, 8)
{
}

static const Reflection::PropDescriptor<UICorner, UDim> prop_CornerRadius(
    "CornerRadius", category_Data, &UICorner::getCornerRadius, &UICorner::setCornerRadius);

void UICorner::setCornerRadius(UDim value)
{
    if (cornerRadius != value)
    {
        cornerRadius = value;
        raisePropertyChanged(prop_CornerRadius);
        invalidateParentLayout();
    }
}

float UICorner::getRadius(const Vector2& size) const
{
    return std::max(0.0f, std::min(resolve(cornerRadius, std::min(size.x, size.y)),
        std::min(size.x, size.y) * 0.5f));
}

UIPadding::UIPadding()
    : DescribedCreatable<UIPadding, UIComponent, sUIPadding>("UIPadding")
{
}

static const Reflection::PropDescriptor<UIPadding, UDim> prop_PaddingLeft(
    "PaddingLeft", category_Data, &UIPadding::getPaddingLeft, &UIPadding::setPaddingLeft);
static const Reflection::PropDescriptor<UIPadding, UDim> prop_PaddingRight(
    "PaddingRight", category_Data, &UIPadding::getPaddingRight, &UIPadding::setPaddingRight);
static const Reflection::PropDescriptor<UIPadding, UDim> prop_PaddingTop(
    "PaddingTop", category_Data, &UIPadding::getPaddingTop, &UIPadding::setPaddingTop);
static const Reflection::PropDescriptor<UIPadding, UDim> prop_PaddingBottom(
    "PaddingBottom", category_Data, &UIPadding::getPaddingBottom, &UIPadding::setPaddingBottom);

#define RBX_UI_PADDING_SETTER(Name, member, descriptor) \
void UIPadding::set##Name(UDim value) \
{ \
    if (member != value) \
    { \
        member = value; \
        raisePropertyChanged(descriptor); \
        invalidateParentLayout(); \
    } \
}

RBX_UI_PADDING_SETTER(PaddingLeft, paddingLeft, prop_PaddingLeft)
RBX_UI_PADDING_SETTER(PaddingRight, paddingRight, prop_PaddingRight)
RBX_UI_PADDING_SETTER(PaddingTop, paddingTop, prop_PaddingTop)
RBX_UI_PADDING_SETTER(PaddingBottom, paddingBottom, prop_PaddingBottom)
#undef RBX_UI_PADDING_SETTER

void UIPadding::getInsets(const Vector2& parentSize, float& left, float& right,
    float& top, float& bottom) const
{
    left = resolve(paddingLeft, parentSize.x);
    right = resolve(paddingRight, parentSize.x);
    top = resolve(paddingTop, parentSize.y);
    bottom = resolve(paddingBottom, parentSize.y);
}

UIScale::UIScale()
    : DescribedCreatable<UIScale, UIComponent, sUIScale>("UIScale")
    , scale(1.0f)
{
}

static const Reflection::PropDescriptor<UIScale, float> prop_Scale(
    "Scale", category_Data, &UIScale::getScale, &UIScale::setScale);

void UIScale::setScale(float value)
{
    value = std::max(0.0f, value);
    if (scale != value)
    {
        scale = value;
        raisePropertyChanged(prop_Scale);
        invalidateParentLayout();
    }
}

UIFlexItem::UIFlexItem()
    : DescribedCreatable<UIFlexItem, UIComponent, sUIFlexItem>("UIFlexItem")
    , flexMode(UI_FLEX_MODE_NONE)
    , growRatio(0.0f)
    , shrinkRatio(0.0f)
    , itemLineAlignment(ITEM_LINE_ALIGNMENT_AUTOMATIC)
{
}

void UIFlexItem::invalidateContainerLayout()
{
    GuiObject* item = Instance::fastDynamicCast<GuiObject>(getParent());
    GuiObject* container = item ? Instance::fastDynamicCast<GuiObject>(item->getParent()) : NULL;
    if (container)
        container->invalidateLayout();
    else if (item)
        item->invalidateLayout();
}

static const Reflection::EnumPropDescriptor<UIFlexItem, UIFlexMode> prop_FlexMode(
    "FlexMode", category_Data, &UIFlexItem::getFlexMode, &UIFlexItem::setFlexMode);
static const Reflection::PropDescriptor<UIFlexItem, float> prop_GrowRatio(
    "GrowRatio", category_Data, &UIFlexItem::getGrowRatio, &UIFlexItem::setGrowRatio);
static const Reflection::PropDescriptor<UIFlexItem, float> prop_ShrinkRatio(
    "ShrinkRatio", category_Data, &UIFlexItem::getShrinkRatio, &UIFlexItem::setShrinkRatio);
static const Reflection::EnumPropDescriptor<UIFlexItem, ItemLineAlignment> prop_FlexItemLineAlignment(
    "ItemLineAlignment", category_Data, &UIFlexItem::getItemLineAlignment, &UIFlexItem::setItemLineAlignment);

#define RBX_UI_FLEX_ITEM_SETTER(Type, Name, member, descriptor) \
void UIFlexItem::set##Name(Type value) \
{ \
    if (member != value) \
    { \
        member = value; \
        raisePropertyChanged(descriptor); \
        invalidateContainerLayout(); \
    } \
}

RBX_UI_FLEX_ITEM_SETTER(UIFlexMode, FlexMode, flexMode, prop_FlexMode)
RBX_UI_FLEX_ITEM_SETTER(ItemLineAlignment, ItemLineAlignment, itemLineAlignment, prop_FlexItemLineAlignment)
#undef RBX_UI_FLEX_ITEM_SETTER

void UIFlexItem::setGrowRatio(float value)
{
    value = std::max(0.0f, value);
    if (growRatio != value)
    {
        growRatio = value;
        raisePropertyChanged(prop_GrowRatio);
        invalidateContainerLayout();
    }
}

void UIFlexItem::setShrinkRatio(float value)
{
    value = std::max(0.0f, value);
    if (shrinkRatio != value)
    {
        shrinkRatio = value;
        raisePropertyChanged(prop_ShrinkRatio);
        invalidateContainerLayout();
    }
}

float UIFlexItem::effectiveGrowRatio() const
{
    switch (flexMode)
    {
    case UI_FLEX_MODE_GROW:
    case UI_FLEX_MODE_FILL:
        return 1.0f;
    case UI_FLEX_MODE_CUSTOM:
        return growRatio;
    default:
        return 0.0f;
    }
}

float UIFlexItem::effectiveShrinkRatio() const
{
    switch (flexMode)
    {
    case UI_FLEX_MODE_SHRINK:
    case UI_FLEX_MODE_FILL:
        return 1.0f;
    case UI_FLEX_MODE_CUSTOM:
        return shrinkRatio;
    default:
        return 0.0f;
    }
}

UIStroke::UIStroke()
    : DescribedCreatable<UIStroke, UIComponent, sUIStroke>("UIStroke")
    , applyStrokeMode(APPLY_STROKE_MODE_CONTEXTUAL)
    , borderOffset(0.0f, 0)
    , borderStrokePosition(BORDER_STROKE_POSITION_OUTER)
    , color(Color3::black())
    , enabled(true)
    , lineJoinMode(LINE_JOIN_MODE_ROUND)
    , strokeSizingMode(STROKE_SIZING_MODE_FIXED_SIZE)
    , thickness(1.0f)
    , transparency(0.0f)
    , zIndex(1)
{
}

static const Reflection::EnumPropDescriptor<UIStroke, ApplyStrokeMode> prop_ApplyStrokeMode(
    "ApplyStrokeMode", category_Data, &UIStroke::getApplyStrokeMode, &UIStroke::setApplyStrokeMode);
static const Reflection::PropDescriptor<UIStroke, UDim> prop_BorderOffset(
    "BorderOffset", category_Data, &UIStroke::getBorderOffset, &UIStroke::setBorderOffset);
static const Reflection::EnumPropDescriptor<UIStroke, BorderStrokePosition> prop_BorderStrokePosition(
    "BorderStrokePosition", category_Data, &UIStroke::getBorderStrokePosition, &UIStroke::setBorderStrokePosition);
static const Reflection::PropDescriptor<UIStroke, Color3> prop_StrokeColor(
    "Color", category_Data, &UIStroke::getColor, &UIStroke::setColor);
static const Reflection::PropDescriptor<UIStroke, bool> prop_StrokeEnabled(
    "Enabled", category_Data, &UIStroke::getEnabled, &UIStroke::setEnabled);
static const Reflection::EnumPropDescriptor<UIStroke, LineJoinMode> prop_LineJoinMode(
    "LineJoinMode", category_Data, &UIStroke::getLineJoinMode, &UIStroke::setLineJoinMode);
static const Reflection::EnumPropDescriptor<UIStroke, StrokeSizingMode> prop_StrokeSizingMode(
    "StrokeSizingMode", category_Data, &UIStroke::getStrokeSizingMode, &UIStroke::setStrokeSizingMode);
static const Reflection::PropDescriptor<UIStroke, float> prop_StrokeThickness(
    "Thickness", category_Data, &UIStroke::getThickness, &UIStroke::setThickness);
static const Reflection::PropDescriptor<UIStroke, float> prop_StrokeTransparency(
    "Transparency", category_Data, &UIStroke::getTransparency, &UIStroke::setTransparency);
static const Reflection::PropDescriptor<UIStroke, int> prop_StrokeZIndex(
    "ZIndex", category_Data, &UIStroke::getZIndex, &UIStroke::setZIndex);

#define RBX_UI_STROKE_SETTER(Type, Name, member, descriptor) \
void UIStroke::set##Name(Type value) \
{ \
    if (member != value) \
    { \
        member = value; \
        raisePropertyChanged(descriptor); \
        invalidateParentLayout(); \
    } \
}

RBX_UI_STROKE_SETTER(ApplyStrokeMode, ApplyStrokeMode, applyStrokeMode, prop_ApplyStrokeMode)
RBX_UI_STROKE_SETTER(UDim, BorderOffset, borderOffset, prop_BorderOffset)
RBX_UI_STROKE_SETTER(BorderStrokePosition, BorderStrokePosition, borderStrokePosition, prop_BorderStrokePosition)
RBX_UI_STROKE_SETTER(Color3, Color, color, prop_StrokeColor)
RBX_UI_STROKE_SETTER(bool, Enabled, enabled, prop_StrokeEnabled)
RBX_UI_STROKE_SETTER(LineJoinMode, LineJoinMode, lineJoinMode, prop_LineJoinMode)
RBX_UI_STROKE_SETTER(StrokeSizingMode, StrokeSizingMode, strokeSizingMode, prop_StrokeSizingMode)
RBX_UI_STROKE_SETTER(int, ZIndex, zIndex, prop_StrokeZIndex)
#undef RBX_UI_STROKE_SETTER

void UIStroke::setThickness(float value)
{
    value = std::max(0.0f, value);
    if (thickness != value)
    {
        thickness = value;
        raisePropertyChanged(prop_StrokeThickness);
        invalidateParentLayout();
    }
}

void UIStroke::setTransparency(float value)
{
    value = std::clamp(value, 0.0f, 1.0f);
    if (transparency != value)
    {
        transparency = value;
        raisePropertyChanged(prop_StrokeTransparency);
        invalidateParentLayout();
    }
}

float UIStroke::resolveThickness(const Vector2& parentSize) const
{
    return strokeSizingMode == STROKE_SIZING_MODE_SCALED_SIZE
        ? thickness * std::min(parentSize.x, parentSize.y) : thickness;
}

float UIStroke::resolveBorderOffset(const Vector2& parentSize) const
{
    return resolve(borderOffset, std::min(parentSize.x, parentSize.y));
}

UIGradient::UIGradient()
    : DescribedCreatable<UIGradient, UIComponent, sUIGradient>("UIGradient")
    , color(Color3::white())
    , transparency(0.0f)
    , enabled(true)
    , offset(Vector2::zero())
    , rotation(0.0f)
{
}

static const Reflection::PropDescriptor<UIGradient, ColorSequence> prop_GradientColor(
    "Color", category_Data, &UIGradient::getColor, &UIGradient::setColor);
static const Reflection::PropDescriptor<UIGradient, NumberSequence> prop_GradientTransparency(
    "Transparency", category_Data, &UIGradient::getTransparency, &UIGradient::setTransparency);
static const Reflection::PropDescriptor<UIGradient, bool> prop_GradientEnabled(
    "Enabled", category_Data, &UIGradient::getEnabled, &UIGradient::setEnabled);
static const Reflection::PropDescriptor<UIGradient, Vector2> prop_GradientOffset(
    "Offset", category_Data, &UIGradient::getOffset, &UIGradient::setOffset);
static const Reflection::PropDescriptor<UIGradient, float> prop_GradientRotation(
    "Rotation", category_Data, &UIGradient::getRotation, &UIGradient::setRotation);

void UIGradient::setColor(const ColorSequence& value)
{
    if (!(color == value))
    {
        color = value;
        raisePropertyChanged(prop_GradientColor);
        invalidateParentLayout();
    }
}

void UIGradient::setTransparency(const NumberSequence& value)
{
    if (!(transparency == value))
    {
        transparency = NumberSequence(value, 0.0f, 1.0f);
        raisePropertyChanged(prop_GradientTransparency);
        invalidateParentLayout();
    }
}

void UIGradient::setEnabled(bool value)
{
    if (enabled != value)
    {
        enabled = value;
        raisePropertyChanged(prop_GradientEnabled);
        invalidateParentLayout();
    }
}

void UIGradient::setOffset(Vector2 value)
{
    if (offset != value)
    {
        offset = value;
        raisePropertyChanged(prop_GradientOffset);
        invalidateParentLayout();
    }
}

void UIGradient::setRotation(float value)
{
    if (!std::isfinite(value))
        value = 0.0f;
    value = std::fmod(value, 360.0f);
    if (rotation != value)
    {
        rotation = value;
        raisePropertyChanged(prop_GradientRotation);
        invalidateParentLayout();
    }
}

bool UIGradient::askSetParent(const Instance* instance) const
{
    return Instance::fastDynamicCast<GuiObject>(instance) != NULL ||
        Instance::fastDynamicCast<UIStroke>(instance) != NULL;
}

float UIGradient::unclampedParameterAt(const Vector2& point, const Rect2D& bounds) const
{
    if (bounds.width() <= 0.0f || bounds.height() <= 0.0f)
        return 0.5f;

    const float radians = rotation * Math::pi() / 180.0f;
    const Vector2 direction(std::cos(radians), std::sin(radians));
    const Vector2 normalized((point.x - bounds.center().x) / bounds.width(),
        (point.y - bounds.center().y) / bounds.height());
    const float halfProjection = 0.5f * (std::abs(direction.x) + std::abs(direction.y));
    const float projected = (normalized - offset).dot(direction);
    return 0.5f + projected / (2.0f * halfProjection);
}

float UIGradient::parameterAt(const Vector2& point, const Rect2D& bounds) const
{
    return std::clamp(unclampedParameterAt(point, bounds), 0.0f, 1.0f);
}

namespace
{
Color3 sampleColorSequence(const ColorSequence& sequence, float parameter)
{
    const std::vector<ColorSequence::Key>& points = sequence.getPoints();
    for (size_t index = 1; index < points.size(); ++index)
        if (parameter <= points[index].time)
        {
            const ColorSequence::Key& left = points[index - 1];
            const ColorSequence::Key& right = points[index];
            const float span = right.time - left.time;
            const float alpha = span > 0.0f ? (parameter - left.time) / span : 0.0f;
            return left.value.lerp(right.value, std::clamp(alpha, 0.0f, 1.0f));
        }
    return points.back().value;
}

float sampleNumberSequence(const NumberSequence& sequence, float parameter)
{
    const std::vector<NumberSequence::Key>& points = sequence.getPoints();
    for (size_t index = 1; index < points.size(); ++index)
        if (parameter <= points[index].time)
        {
            const NumberSequence::Key& left = points[index - 1];
            const NumberSequence::Key& right = points[index];
            const float span = right.time - left.time;
            const float alpha = span > 0.0f ? (parameter - left.time) / span : 0.0f;
            return G3D::lerp(left.value, right.value, std::clamp(alpha, 0.0f, 1.0f));
        }
    return points.back().value;
}
}

Color4 UIGradient::sample(float parameter, const Color4& baseColor) const
{
    if (!enabled)
        return baseColor;
    parameter = std::clamp(parameter, 0.0f, 1.0f);
    const Color3 tint = sampleColorSequence(color, parameter);
    const float alpha = 1.0f - std::clamp(sampleNumberSequence(transparency, parameter), 0.0f, 1.0f);
    return Color4(baseColor.r * tint.r, baseColor.g * tint.g, baseColor.b * tint.b,
        baseColor.a * alpha);
}

Color4 UIGradient::sampleAt(const Vector2& point, const Rect2D& bounds, const Color4& baseColor) const
{
    return sample(parameterAt(point, bounds), baseColor);
}

static const Reflection::PropDescriptor<UIDragDetector, bool> prop_DragEnabled(
    "Enabled", category_Behavior, &UIDragDetector::getEnabled, &UIDragDetector::setEnabled);
static const Reflection::EnumPropDescriptor<UIDragDetector, UIDragDetectorBoundingBehavior> prop_BoundingBehavior(
    "BoundingBehavior", category_Behavior, &UIDragDetector::getBoundingBehavior, &UIDragDetector::setBoundingBehavior);
static const Reflection::RefPropDescriptor<UIDragDetector, GuiBase2d> prop_BoundingUI(
    "BoundingUI", category_Behavior, &UIDragDetector::getBoundingUI, &UIDragDetector::setBoundingUI);
static const Reflection::PropDescriptor<UIDragDetector, Vector2> prop_DragAxis(
    "DragAxis", category_Behavior, &UIDragDetector::getDragAxis, &UIDragDetector::setDragAxis);
static const Reflection::EnumPropDescriptor<UIDragDetector, UIDragDetectorDragRelativity> prop_DragRelativity(
    "DragRelativity", category_Behavior, &UIDragDetector::getDragRelativity, &UIDragDetector::setDragRelativity);
static const Reflection::PropDescriptor<UIDragDetector, float> prop_DragRotation(
    "DragRotation", "Dragged Amount", &UIDragDetector::getDragRotation, &UIDragDetector::setDragRotation);
static const Reflection::EnumPropDescriptor<UIDragDetector, UIDragDetectorDragSpace> prop_DragSpace(
    "DragSpace", category_Behavior, &UIDragDetector::getDragSpace, &UIDragDetector::setDragSpace);
static const Reflection::EnumPropDescriptor<UIDragDetector, UIDragDetectorDragStyle> prop_DragStyle(
    "DragStyle", category_Behavior, &UIDragDetector::getDragStyle, &UIDragDetector::setDragStyle);
static const Reflection::PropDescriptor<UIDragDetector, UDim2> prop_DragUDim2(
    "DragUDim2", "Dragged Amount", &UIDragDetector::getDragUDim2, &UIDragDetector::setDragUDim2);
static const Reflection::PropDescriptor<UIDragDetector, float> prop_MaxDragAngle(
    "MaxDragAngle", "Drag Limits", &UIDragDetector::getMaxDragAngle, &UIDragDetector::setMaxDragAngle);
static const Reflection::PropDescriptor<UIDragDetector, UDim2> prop_MaxDragTranslation(
    "MaxDragTranslation", "Drag Limits", &UIDragDetector::getMaxDragTranslation, &UIDragDetector::setMaxDragTranslation);
static const Reflection::PropDescriptor<UIDragDetector, float> prop_MinDragAngle(
    "MinDragAngle", "Drag Limits", &UIDragDetector::getMinDragAngle, &UIDragDetector::setMinDragAngle);
static const Reflection::PropDescriptor<UIDragDetector, UDim2> prop_MinDragTranslation(
    "MinDragTranslation", "Drag Limits", &UIDragDetector::getMinDragTranslation, &UIDragDetector::setMinDragTranslation);
static const Reflection::RefPropDescriptor<UIDragDetector, GuiObject> prop_ReferenceUIInstance(
    "ReferenceUIInstance", category_Behavior, &UIDragDetector::getReferenceUIInstance, &UIDragDetector::setReferenceUIInstance);
static const Reflection::EnumPropDescriptor<UIDragDetector, UIDragDetectorResponseStyle> prop_ResponseStyle(
    "ResponseStyle", category_Behavior, &UIDragDetector::getResponseStyle, &UIDragDetector::setResponseStyle);
static const Reflection::BoundFuncDesc<UIDragDetector, rbx::signals::connection(int, Lua::WeakFunctionRef)> func_AddConstraintFunction(
    &UIDragDetector::addConstraintFunction, "AddConstraintFunction", "priority", "function", Security::None);
static const Reflection::BoundFuncDesc<UIDragDetector, UDim2()> func_GetReferencePosition(
    &UIDragDetector::getReferencePosition, "GetReferencePosition", Security::None);
static const Reflection::BoundFuncDesc<UIDragDetector, float()> func_GetReferenceRotation(
    &UIDragDetector::getReferenceRotation, "GetReferenceRotation", Security::None);
static const Reflection::BoundFuncDesc<UIDragDetector, void(Lua::WeakFunctionRef)> func_SetDragStyleFunction(
    &UIDragDetector::setDragStyleFunction, "SetDragStyleFunction", "function", Security::None);
static const Reflection::EventDesc<UIDragDetector, void(Vector2)> event_DragStart(
    &UIDragDetector::dragStartSignal, "DragStart", "inputPosition", Security::None);
static const Reflection::EventDesc<UIDragDetector, void(Vector2)> event_DragContinue(
    &UIDragDetector::dragContinueSignal, "DragContinue", "inputPosition", Security::None);
static const Reflection::EventDesc<UIDragDetector, void(Vector2)> event_DragEnd(
    &UIDragDetector::dragEndSignal, "DragEnd", "inputPosition", Security::None);

namespace
{
G3D::int16 dragOffset(float value)
{
    return static_cast<G3D::int16>(std::clamp(std::lround(value),
        static_cast<long>(std::numeric_limits<G3D::int16>::min()),
        static_cast<long>(std::numeric_limits<G3D::int16>::max())));
}

template<typename T> T* lockedPointer(const weak_ptr<T>& value)
{
    const shared_ptr<T> locked = value.lock();
    return locked.get();
}

float clampComponent(float value, float minimum, float maximum)
{
    return minimum <= maximum ? std::clamp(value, minimum, maximum) : value;
}

void keepConstraintConnectionAlive()
{
}
}

UIDragDetector::UIDragDetector()
    : DescribedCreatable<UIDragDetector, UIComponent, sUIDragDetector>("UIDragDetector")
    , enabled(true)
    , boundingBehavior(UI_DRAG_BOUNDING_ENTIRE_OBJECT)
    , dragAxis(1.0f, 0.0f)
    , dragRelativity(UI_DRAG_RELATIVITY_RELATIVE)
    , dragRotation(0.0f)
    , dragSpace(UI_DRAG_SPACE_PARENT)
    , dragStyle(UI_DRAG_STYLE_TRANSLATE_PLANE)
    , maxDragAngle(std::numeric_limits<float>::infinity())
    , maxDragTranslation(1.0e6f, std::numeric_limits<G3D::int16>::max(),
        1.0e6f, std::numeric_limits<G3D::int16>::max())
    , minDragAngle(-std::numeric_limits<float>::infinity())
    , minDragTranslation(-1.0e6f, std::numeric_limits<G3D::int16>::min(),
        -1.0e6f, std::numeric_limits<G3D::int16>::min())
    , responseStyle(UI_DRAG_RESPONSE_OFFSET)
    , dragging(false)
    , initialParentRotation(0.0f)
{
}

#define RBX_UI_DRAG_SETTER(Type, Name, field, descriptor) \
void UIDragDetector::set##Name(Type value) \
{ \
    if (field != value) \
    { \
        field = value; \
        raisePropertyChanged(descriptor); \
    } \
}

RBX_UI_DRAG_SETTER(bool, Enabled, enabled, prop_DragEnabled)
RBX_UI_DRAG_SETTER(UIDragDetectorBoundingBehavior, BoundingBehavior, boundingBehavior, prop_BoundingBehavior)
RBX_UI_DRAG_SETTER(Vector2, DragAxis, dragAxis, prop_DragAxis)
RBX_UI_DRAG_SETTER(UIDragDetectorDragRelativity, DragRelativity, dragRelativity, prop_DragRelativity)
RBX_UI_DRAG_SETTER(float, DragRotation, dragRotation, prop_DragRotation)
RBX_UI_DRAG_SETTER(UIDragDetectorDragSpace, DragSpace, dragSpace, prop_DragSpace)
RBX_UI_DRAG_SETTER(UIDragDetectorDragStyle, DragStyle, dragStyle, prop_DragStyle)
RBX_UI_DRAG_SETTER(UDim2, DragUDim2, dragUDim2, prop_DragUDim2)
RBX_UI_DRAG_SETTER(float, MaxDragAngle, maxDragAngle, prop_MaxDragAngle)
RBX_UI_DRAG_SETTER(UDim2, MaxDragTranslation, maxDragTranslation, prop_MaxDragTranslation)
RBX_UI_DRAG_SETTER(float, MinDragAngle, minDragAngle, prop_MinDragAngle)
RBX_UI_DRAG_SETTER(UDim2, MinDragTranslation, minDragTranslation, prop_MinDragTranslation)
RBX_UI_DRAG_SETTER(UIDragDetectorResponseStyle, ResponseStyle, responseStyle, prop_ResponseStyle)

#undef RBX_UI_DRAG_SETTER

GuiBase2d* UIDragDetector::getBoundingUI() const
{
    return lockedPointer(boundingUI);
}

void UIDragDetector::setBoundingUI(GuiBase2d* value)
{
    if (value != getBoundingUI())
    {
        boundingUI = weak_from(value);
        raisePropertyChanged(prop_BoundingUI);
    }
}

GuiObject* UIDragDetector::getReferenceUIInstance() const
{
    return lockedPointer(referenceUIInstance);
}

void UIDragDetector::setReferenceUIInstance(GuiObject* value)
{
    if (value != getReferenceUIInstance())
    {
        referenceUIInstance = weak_from(value);
        raisePropertyChanged(prop_ReferenceUIInstance);
    }
}

rbx::signals::connection UIDragDetector::addConstraintFunction(int priority, Lua::WeakFunctionRef function)
{
    rbx::signals::connection lifetime = constraintLifetimeSignal.connect(&keepConstraintConnectionAlive);
    ConstraintFunction entry = { priority, function, lifetime };
    constraintFunctions.push_back(entry);
    std::stable_sort(constraintFunctions.begin(), constraintFunctions.end(),
        [](const ConstraintFunction& left, const ConstraintFunction& right) {
            return left.priority < right.priority;
        });
    return lifetime;
}

UDim2 UIDragDetector::getReferencePosition()
{
    if (GuiObject* reference = getReferenceUIInstance())
        return reference->getPosition();
    if (GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent()))
        if (GuiObject* grandparent = Instance::fastDynamicCast<GuiObject>(parent->getParent()))
            return grandparent->getPosition();
        else
            return parent->getPosition();
    return UDim2();
}

float UIDragDetector::getReferenceRotation()
{
    if (GuiObject* reference = getReferenceUIInstance())
        return reference->getRotation();
    if (GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent()))
        if (GuiObject* grandparent = Instance::fastDynamicCast<GuiObject>(parent->getParent()))
            return grandparent->getRotation();
        else
            return parent->getRotation();
    return 0.0f;
}

void UIDragDetector::setDragStyleFunction(Lua::WeakFunctionRef function)
{
    dragStyleFunction = function;
}

void UIDragDetector::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    parentInputBeganConnection.disconnect();
    globalInputChangedConnection.disconnect();
    globalInputEndedConnection.disconnect();
    Super::onServiceProvider(oldProvider, newProvider);
    if (newProvider)
        connectInput();
}

void UIDragDetector::connectInput()
{
    if (GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent()))
        parentInputBeganConnection = parent->inputBeganEvent.connect(
            boost::bind(&UIDragDetector::parentInputBegan, this, _1));
    if (UserInputService* input = ServiceProvider::find<UserInputService>(this))
    {
        globalInputChangedConnection = input->coreInputUpdatedEvent.connect(
            boost::bind(&UIDragDetector::globalInputChanged, this, _1));
        globalInputEndedConnection = input->coreInputEndedEvent.connect(
            boost::bind(&UIDragDetector::globalInputEnded, this, _1));
    }
}

void UIDragDetector::parentInputBegan(shared_ptr<Instance> event)
{
    InputObject* input = Instance::fastDynamicCast<InputObject>(event.get());
    if (!enabled || !input || (!input->isLeftMouseDownEvent() && !input->isTouchEvent()))
        return;
    activeInput = shared_from(input);
    beginDrag(input->get2DPosition());
}

void UIDragDetector::globalInputChanged(shared_ptr<Instance> event)
{
    InputObject* input = Instance::fastDynamicCast<InputObject>(event.get());
    if (!dragging || !input)
        return;
    if (activeInput && activeInput->isTouchEvent() && activeInput.get() != input)
        return;
    if (input->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT || input->isTouchEvent())
        continueDrag(input->get2DPosition());
}

void UIDragDetector::globalInputEnded(shared_ptr<Instance> event)
{
    InputObject* input = Instance::fastDynamicCast<InputObject>(event.get());
    if (!dragging || !input)
        return;
    const bool matchingTouch = activeInput && activeInput->isTouchEvent() && activeInput.get() == input;
    if (matchingTouch || input->isLeftMouseUpEvent())
        endDrag(input->get2DPosition());
}

void UIDragDetector::beginDrag(const Vector2& inputPosition)
{
    GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!enabled || !parent || dragging)
        return;
    dragging = true;
    initialInputPosition = inputPosition;
    initialParentPosition = parent->getPosition();
    initialParentRotation = parent->getRotation();
    setDragUDim2(UDim2());
    setDragRotation(0.0f);
    dragStartSignal(inputPosition);
}

bool UIDragDetector::invokeDragStyleFunction(const Vector2& inputPosition, UDim2& translation,
    float& rotation, UIDragDetectorDragRelativity& relativity, UIDragDetectorDragSpace& space)
{
    if (!dragStyleFunction.lock())
        return false;
    ScriptContext* context = ServiceProvider::create<ScriptContext>(this);
    if (!context)
        return false;
    Reflection::Tuple arguments;
    arguments.values.push_back(inputPosition);
    try
    {
        Reflection::Tuple result = context->callInNewThread(dragStyleFunction, arguments);
        if (result.values.empty() || result.values[0].isVoid() || !result.values[0].isType<UDim2>())
            return false;
        translation = result.values[0].cast<UDim2>();
        if (result.values.size() > 1 && result.values[1].isType<float>())
            rotation = result.values[1].cast<float>();
        if (result.values.size() > 2 && result.values[2].isType<UIDragDetectorDragRelativity>())
            relativity = result.values[2].cast<UIDragDetectorDragRelativity>();
        if (result.values.size() > 3 && result.values[3].isType<UIDragDetectorDragSpace>())
            space = result.values[3].cast<UIDragDetectorDragSpace>();
        return true;
    }
    catch (const base_exception& error)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR,
            "UIDragDetector drag function failed: %s", error.what());
        return false;
    }
}

bool UIDragDetector::invokeConstraint(Lua::WeakFunctionRef& function, UDim2& translation,
    float& rotation, UIDragDetectorDragRelativity& relativity, UIDragDetectorDragSpace& space)
{
    if (!function.lock())
        return false;
    ScriptContext* context = ServiceProvider::create<ScriptContext>(this);
    if (!context)
        return false;
    Reflection::Tuple arguments;
    arguments.values.push_back(translation);
    arguments.values.push_back(rotation);
    try
    {
        Reflection::Tuple result = context->callInNewThread(function, arguments);
        if (result.values.empty() || !result.values[0].isType<UDim2>())
            return false;
        translation = result.values[0].cast<UDim2>();
        if (result.values.size() > 1 && result.values[1].isType<float>())
            rotation = result.values[1].cast<float>();
        if (result.values.size() > 2 && result.values[2].isType<UIDragDetectorDragRelativity>())
            relativity = result.values[2].cast<UIDragDetectorDragRelativity>();
        if (result.values.size() > 3 && result.values[3].isType<UIDragDetectorDragSpace>())
            space = result.values[3].cast<UIDragDetectorDragSpace>();
        return true;
    }
    catch (const base_exception& error)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR,
            "UIDragDetector constraint failed: %s", error.what());
        return false;
    }
}

void UIDragDetector::continueDrag(const Vector2& inputPosition)
{
    GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!dragging || !enabled || !parent)
        return;

    UIDragDetectorDragRelativity motionRelativity = UI_DRAG_RELATIVITY_RELATIVE;
    UIDragDetectorDragSpace motionSpace = dragSpace;
    UDim2 translation;
    float rotation = 0.0f;

    if (dragStyle == UI_DRAG_STYLE_SCRIPTABLE)
    {
        motionRelativity = dragRelativity;
        if (!invokeDragStyleFunction(inputPosition, translation, rotation, motionRelativity, motionSpace))
        {
            dragContinueSignal(inputPosition);
            return;
        }
    }
    else
    {
        Vector2 delta = inputPosition - initialInputPosition;
        const GuiObject* reference = getReferenceUIInstance();
        const float referenceRotation = reference ? reference->getAbsoluteRotation().getAngle().getValue() :
            (motionSpace == UI_DRAG_SPACE_LAYER_COLLECTOR ? 0.0f : getReferenceRotation());
        if (referenceRotation != 0.0f)
            delta = Rotation2D(RotationAngle(-referenceRotation), Vector2::zero()).rotate(delta);

        if (dragStyle == UI_DRAG_STYLE_TRANSLATE_LINE)
        {
            Vector2 axis = dragAxis;
            if (axis.squaredLength() > 0.0f)
                axis /= axis.length();
            else
                axis = Vector2::zero();
            delta = axis * delta.dot(axis);
        }
        else if (dragStyle == UI_DRAG_STYLE_ROTATE)
        {
            const GuiObject* originObject = reference ? reference : parent;
            const Vector2 center = originObject->getRect2DFloat().center();
            const Vector2 start = initialInputPosition - center;
            const Vector2 current = inputPosition - center;
            if (start.squaredLength() > 0.0f && current.squaredLength() > 0.0f)
                rotation = std::atan2(start.x * current.y - start.y * current.x,
                    start.dot(current)) * 180.0f / Math::pi();
            rotation = clampComponent(rotation, minDragAngle, maxDragAngle);
        }

        if (dragStyle != UI_DRAG_STYLE_ROTATE)
        {
            Vector2 scaleExtent(1.0f, 1.0f);
            if (GuiBase2d* layoutParent = Instance::fastDynamicCast<GuiBase2d>(parent->getParent()))
                scaleExtent = layoutParent->getCanvasRect().wh().max(Vector2(1.0f, 1.0f));
            const bool useScale = responseStyle == UI_DRAG_RESPONSE_SCALE ||
                responseStyle == UI_DRAG_RESPONSE_CUSTOM_SCALE;
            translation = useScale
                ? UDim2(delta.x / scaleExtent.x, 0, delta.y / scaleExtent.y, 0)
                : UDim2(0, dragOffset(delta.x), 0, dragOffset(delta.y));
            translation.x.scale = clampComponent(translation.x.scale,
                minDragTranslation.x.scale, maxDragTranslation.x.scale);
            translation.y.scale = clampComponent(translation.y.scale,
                minDragTranslation.y.scale, maxDragTranslation.y.scale);
            translation.x.offset = static_cast<G3D::int16>(clampComponent(translation.x.offset,
                minDragTranslation.x.offset, maxDragTranslation.x.offset));
            translation.y.offset = static_cast<G3D::int16>(clampComponent(translation.y.offset,
                minDragTranslation.y.offset, maxDragTranslation.y.offset));
        }
    }

    constraintFunctions.erase(std::remove_if(constraintFunctions.begin(), constraintFunctions.end(),
        [](const ConstraintFunction& value) { return !value.lifetime.connected(); }), constraintFunctions.end());
    for (ConstraintFunction& constraint : constraintFunctions)
        invokeConstraint(constraint.function, translation, rotation, motionRelativity, motionSpace);

    applyMotion(translation, rotation, motionRelativity, motionSpace);
    dragContinueSignal(inputPosition);
}

void UIDragDetector::applyMotion(UDim2 translation, float rotation,
    UIDragDetectorDragRelativity relativity, UIDragDetectorDragSpace space)
{
    GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return;

    setDragUDim2(translation);
    setDragRotation(rotation);
    if (responseStyle == UI_DRAG_RESPONSE_CUSTOM_OFFSET || responseStyle == UI_DRAG_RESPONSE_CUSTOM_SCALE)
        return;

    UDim2 target = relativity == UI_DRAG_RELATIVITY_RELATIVE
        ? initialParentPosition + translation : translation;

    if (GuiBase2d* bounds = getBoundingUI())
    {
        const Rect2D boundsRect = bounds->getRect2DFloat();
        const Rect2D objectRect = parent->getRect2DFloat();
        Vector2 pixelDelta = translation * (Instance::fastDynamicCast<GuiBase2d>(parent->getParent())
            ? Instance::fastDynamicCast<GuiBase2d>(parent->getParent())->getCanvasRect().wh()
            : Vector2::zero());
        Rect2D moved = objectRect + pixelDelta;
        Vector2 correction = Vector2::zero();
        if (boundingBehavior == UI_DRAG_BOUNDING_ENTIRE_OBJECT)
        {
            if (moved.x0() < boundsRect.x0()) correction.x += boundsRect.x0() - moved.x0();
            if (moved.x1() > boundsRect.x1()) correction.x += boundsRect.x1() - moved.x1();
            if (moved.y0() < boundsRect.y0()) correction.y += boundsRect.y0() - moved.y0();
            if (moved.y1() > boundsRect.y1()) correction.y += boundsRect.y1() - moved.y1();
        }
        else
        {
            const Vector2 point = moved.center();
            correction.x = std::clamp(point.x, boundsRect.x0(), boundsRect.x1()) - point.x;
            correction.y = std::clamp(point.y, boundsRect.y0(), boundsRect.y1()) - point.y;
        }
        target.x.offset = dragOffset(target.x.offset + correction.x);
        target.y.offset = dragOffset(target.y.offset + correction.y);
    }

    parent->setPosition(target);
    parent->setRotation(relativity == UI_DRAG_RELATIVITY_RELATIVE
        ? initialParentRotation + rotation : rotation);
}

void UIDragDetector::endDrag(const Vector2& inputPosition)
{
    if (!dragging)
        return;
    dragging = false;
    activeInput.reset();
    dragEndSignal(inputPosition);
}

UIListLayout::UIListLayout()
    : DescribedCreatable<UIListLayout, UIComponent, sUIListLayout>("UIListLayout")
    , fillDirection(FILL_DIRECTION_VERTICAL)
    , horizontalAlignment(HORIZONTAL_ALIGNMENT_LEFT)
    , verticalAlignment(VERTICAL_ALIGNMENT_TOP)
    , sortOrder(SORT_ORDER_NAME)
    , horizontalFlex(UI_FLEX_ALIGNMENT_NONE)
    , verticalFlex(UI_FLEX_ALIGNMENT_NONE)
    , itemLineAlignment(ITEM_LINE_ALIGNMENT_AUTOMATIC)
    , wraps(false)
{
}

static const Reflection::EnumPropDescriptor<UIListLayout, FillDirection> prop_ListFillDirection(
    "FillDirection", category_Data, &UIListLayout::getFillDirection, &UIListLayout::setFillDirection);
static const Reflection::EnumPropDescriptor<UIListLayout, HorizontalAlignment> prop_ListHorizontalAlignment(
    "HorizontalAlignment", category_Data, &UIListLayout::getHorizontalAlignment, &UIListLayout::setHorizontalAlignment);
static const Reflection::EnumPropDescriptor<UIListLayout, VerticalAlignment> prop_ListVerticalAlignment(
    "VerticalAlignment", category_Data, &UIListLayout::getVerticalAlignment, &UIListLayout::setVerticalAlignment);
static const Reflection::EnumPropDescriptor<UIListLayout, SortOrder> prop_ListSortOrder(
    "SortOrder", category_Data, &UIListLayout::getSortOrder, &UIListLayout::setSortOrder);
static const Reflection::PropDescriptor<UIListLayout, UDim> prop_ListPadding(
    "Padding", category_Data, &UIListLayout::getPadding, &UIListLayout::setPadding);
static const Reflection::EnumPropDescriptor<UIListLayout, UIFlexAlignment> prop_HorizontalFlex(
    "HorizontalFlex", category_Data, &UIListLayout::getHorizontalFlex, &UIListLayout::setHorizontalFlex);
static const Reflection::EnumPropDescriptor<UIListLayout, UIFlexAlignment> prop_VerticalFlex(
    "VerticalFlex", category_Data, &UIListLayout::getVerticalFlex, &UIListLayout::setVerticalFlex);
static const Reflection::EnumPropDescriptor<UIListLayout, ItemLineAlignment> prop_ListItemLineAlignment(
    "ItemLineAlignment", category_Data, &UIListLayout::getItemLineAlignment, &UIListLayout::setItemLineAlignment);
static const Reflection::PropDescriptor<UIListLayout, bool> prop_Wraps(
    "Wraps", category_Data, &UIListLayout::getWraps, &UIListLayout::setWraps);
static const Reflection::PropDescriptor<UIListLayout, Vector2> prop_ListAbsoluteContentSize(
    "AbsoluteContentSize", category_Data, &UIListLayout::getAbsoluteContentSize, NULL,
    Reflection::PropertyDescriptor::UI);

#define RBX_UI_LIST_SETTER(Type, Name, member, descriptor) \
void UIListLayout::set##Name(Type value) \
{ \
    if (member != value) \
    { \
        member = value; \
        raisePropertyChanged(descriptor); \
        invalidateParentLayout(); \
        raisePropertyChanged(prop_ListAbsoluteContentSize); \
    } \
}

RBX_UI_LIST_SETTER(FillDirection, FillDirection, fillDirection, prop_ListFillDirection)
RBX_UI_LIST_SETTER(HorizontalAlignment, HorizontalAlignment, horizontalAlignment, prop_ListHorizontalAlignment)
RBX_UI_LIST_SETTER(VerticalAlignment, VerticalAlignment, verticalAlignment, prop_ListVerticalAlignment)
RBX_UI_LIST_SETTER(SortOrder, SortOrder, sortOrder, prop_ListSortOrder)
RBX_UI_LIST_SETTER(UDim, Padding, padding, prop_ListPadding)
RBX_UI_LIST_SETTER(UIFlexAlignment, HorizontalFlex, horizontalFlex, prop_HorizontalFlex)
RBX_UI_LIST_SETTER(UIFlexAlignment, VerticalFlex, verticalFlex, prop_VerticalFlex)
RBX_UI_LIST_SETTER(ItemLineAlignment, ItemLineAlignment, itemLineAlignment, prop_ListItemLineAlignment)
RBX_UI_LIST_SETTER(bool, Wraps, wraps, prop_Wraps)
#undef RBX_UI_LIST_SETTER

namespace
{
struct ListPlacement
{
    Vector2 position;
    Vector2 size;
};

struct FlexLine
{
    size_t begin;
    size_t end;
    float crossSize;
    float crossPosition;
};

float mainValue(const Vector2& value, bool horizontal) { return horizontal ? value.x : value.y; }
float crossValue(const Vector2& value, bool horizontal) { return horizontal ? value.y : value.x; }
void setMainValue(Vector2& value, bool horizontal, float amount) { horizontal ? value.x = amount : value.y = amount; }
void setCrossValue(Vector2& value, bool horizontal, float amount) { horizontal ? value.y = amount : value.x = amount; }

void distributeSpacing(UIFlexAlignment alignment, float freeSpace, size_t count,
    float& leading, float& between)
{
    leading = 0.0f;
    between = 0.0f;
    if (freeSpace <= 0.0f || count == 0)
        return;
    switch (alignment)
    {
    case UI_FLEX_ALIGNMENT_SPACE_AROUND:
        between = freeSpace / count;
        leading = between * 0.5f;
        break;
    case UI_FLEX_ALIGNMENT_SPACE_BETWEEN:
        between = count > 1 ? freeSpace / (count - 1) : 0.0f;
        break;
    case UI_FLEX_ALIGNMENT_SPACE_EVENLY:
        between = freeSpace / (count + 1);
        leading = between;
        break;
    default:
        break;
    }
}

std::vector<ListPlacement> calculateListPlacements(const UIListLayout* layout,
    const GuiObject* parent, const Rect2D& viewport, const std::vector<const GuiObject*>& children)
{
    const bool horizontal = layout->getFillDirection() == FILL_DIRECTION_HORIZONTAL;
    const Vector2 viewportSize = viewport.wh();
    const float mainExtent = mainValue(viewportSize, horizontal);
    const float crossExtent = crossValue(viewportSize, horizontal);
    const float authoredGap = resolve(layout->getPadding(), mainExtent);
    const UIFlexAlignment mainFlex = horizontal ? layout->getHorizontalFlex() : layout->getVerticalFlex();
    const UIFlexAlignment crossFlex = horizontal ? layout->getVerticalFlex() : layout->getHorizontalFlex();

    std::vector<ListPlacement> result(children.size());
    for (size_t index = 0; index < children.size(); ++index)
    {
        result[index].size = authoredSize(children[index], viewportSize);
        // Foundation uses zero-sized children as intrinsic list items both
        // for explicit flex containers and for single-axis AutomaticSize
        // content (for example, an auto-X text label in a horizontal row).
        // Preserve the authored zero main-axis basis for flex distribution,
        // but let the cross axis contribute its intrinsic content extent.
        if ((findUIFlexItem(children[index]) ||
                children[index]->getAutomaticSize() != AUTOMATIC_SIZE_NONE) &&
            crossValue(result[index].size, horizontal) <= 0.0f)
        {
            const Vector2 intrinsic =
                children[index]->getAutomaticContentSize(viewportSize);
            setCrossValue(result[index].size, horizontal,
                std::max(0.0f, crossValue(intrinsic, horizontal)));
        }
    }

    std::vector<FlexLine> lines;
    size_t begin = 0;
    float used = 0.0f;
    float lineCross = 0.0f;
    for (size_t index = 0; index < children.size(); ++index)
    {
        const float itemMain = mainValue(result[index].size, horizontal);
        const float candidate = used + (index > begin ? authoredGap : 0.0f) + itemMain;
        if (layout->getWraps() && index > begin && candidate > mainExtent)
        {
            lines.push_back({begin, index, lineCross, 0.0f});
            begin = index;
            used = itemMain;
            lineCross = crossValue(result[index].size, horizontal);
        }
        else
        {
            used = candidate;
            lineCross = std::max(lineCross, crossValue(result[index].size, horizontal));
        }
    }
    if (begin < children.size())
        lines.push_back({begin, children.size(), lineCross, 0.0f});

    float totalCross = 0.0f;
    for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
    {
        FlexLine& line = lines[lineIndex];
        const size_t count = line.end - line.begin;
        float occupied = authoredGap * (count > 0 ? count - 1 : 0);
        float growTotal = 0.0f;
        float shrinkTotal = 0.0f;
        for (size_t index = line.begin; index < line.end; ++index)
        {
            occupied += mainValue(result[index].size, horizontal);
            const UIFlexItem* item = findUIFlexItem(children[index]);
            growTotal += item ? item->effectiveGrowRatio() : (mainFlex == UI_FLEX_ALIGNMENT_FILL ? 1.0f : 0.0f);
            shrinkTotal += item ? item->effectiveShrinkRatio() : (mainFlex == UI_FLEX_ALIGNMENT_FILL ? 1.0f : 0.0f);
        }

        const float freeBeforeFlex = mainExtent - occupied;
        if (freeBeforeFlex > 0.0f && growTotal > 0.0f)
        {
            for (size_t index = line.begin; index < line.end; ++index)
            {
                const UIFlexItem* item = findUIFlexItem(children[index]);
                const float ratio = item ? item->effectiveGrowRatio() : (mainFlex == UI_FLEX_ALIGNMENT_FILL ? 1.0f : 0.0f);
                setMainValue(result[index].size, horizontal,
                    mainValue(result[index].size, horizontal) + freeBeforeFlex * ratio / growTotal);
            }
        }
        else if (freeBeforeFlex < 0.0f && shrinkTotal > 0.0f)
        {
            float remaining = -freeBeforeFlex;
            std::vector<bool> active(count, true);
            while (remaining > 0.001f)
            {
                float activeRatio = 0.0f;
                for (size_t local = 0; local < count; ++local)
                    if (active[local])
                    {
                        const UIFlexItem* item = findUIFlexItem(children[line.begin + local]);
                        activeRatio += item ? item->effectiveShrinkRatio() : (mainFlex == UI_FLEX_ALIGNMENT_FILL ? 1.0f : 0.0f);
                    }
                if (activeRatio <= 0.0f)
                    break;
                float consumed = 0.0f;
                for (size_t local = 0; local < count; ++local)
                    if (active[local])
                    {
                        const size_t index = line.begin + local;
                        const UIFlexItem* item = findUIFlexItem(children[index]);
                        const float ratio = item ? item->effectiveShrinkRatio() : (mainFlex == UI_FLEX_ALIGNMENT_FILL ? 1.0f : 0.0f);
                        const float current = mainValue(result[index].size, horizontal);
                        const float reduction = std::min(current, remaining * ratio / activeRatio);
                        setMainValue(result[index].size, horizontal, current - reduction);
                        consumed += reduction;
                        if (reduction >= current - 0.001f)
                            active[local] = false;
                    }
                if (consumed <= 0.001f)
                    break;
                remaining -= consumed;
            }
        }

        float finalMain = authoredGap * (count > 0 ? count - 1 : 0);
        for (size_t index = line.begin; index < line.end; ++index)
            finalMain += mainValue(result[index].size, horizontal);
        float leading = 0.0f;
        float extraBetween = 0.0f;
        distributeSpacing(mainFlex, mainExtent - finalMain, count, leading, extraBetween);
        if (mainFlex == UI_FLEX_ALIGNMENT_NONE)
        {
            if (horizontal && layout->getHorizontalAlignment() == HORIZONTAL_ALIGNMENT_CENTER)
                leading = (mainExtent - finalMain) * 0.5f;
            else if (horizontal && layout->getHorizontalAlignment() == HORIZONTAL_ALIGNMENT_RIGHT)
                leading = mainExtent - finalMain;
            else if (!horizontal && layout->getVerticalAlignment() == VERTICAL_ALIGNMENT_CENTER)
                leading = (mainExtent - finalMain) * 0.5f;
            else if (!horizontal && layout->getVerticalAlignment() == VERTICAL_ALIGNMENT_BOTTOM)
                leading = mainExtent - finalMain;
        }

        float cursor = leading;
        for (size_t index = line.begin; index < line.end; ++index)
        {
            setMainValue(result[index].position, horizontal, cursor);
            cursor += mainValue(result[index].size, horizontal) + authoredGap + extraBetween;
        }
        totalCross += line.crossSize;
    }

    if (lines.size() > 1)
        totalCross += authoredGap * (lines.size() - 1);
    float crossLeading = 0.0f;
    float crossBetween = 0.0f;
    float crossFree = crossExtent - totalCross;
    if (crossFlex == UI_FLEX_ALIGNMENT_FILL && crossFree > 0.0f && !lines.empty())
    {
        const float growth = crossFree / lines.size();
        for (size_t index = 0; index < lines.size(); ++index)
            lines[index].crossSize += growth;
        crossFree = 0.0f;
    }
    else
        distributeSpacing(crossFlex, crossFree, lines.size(), crossLeading, crossBetween);
    if (crossFlex == UI_FLEX_ALIGNMENT_NONE)
    {
        if (horizontal && layout->getVerticalAlignment() == VERTICAL_ALIGNMENT_CENTER)
            crossLeading = crossFree * 0.5f;
        else if (horizontal && layout->getVerticalAlignment() == VERTICAL_ALIGNMENT_BOTTOM)
            crossLeading = crossFree;
        else if (!horizontal && layout->getHorizontalAlignment() == HORIZONTAL_ALIGNMENT_CENTER)
            crossLeading = crossFree * 0.5f;
        else if (!horizontal && layout->getHorizontalAlignment() == HORIZONTAL_ALIGNMENT_RIGHT)
            crossLeading = crossFree;
    }

    float crossCursor = crossLeading;
    for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
    {
        FlexLine& line = lines[lineIndex];
        line.crossPosition = crossCursor;
        for (size_t index = line.begin; index < line.end; ++index)
        {
            const UIFlexItem* item = findUIFlexItem(children[index]);
            ItemLineAlignment alignment = item && item->getItemLineAlignment() != ITEM_LINE_ALIGNMENT_AUTOMATIC
                ? item->getItemLineAlignment() : layout->getItemLineAlignment();
            if (alignment == ITEM_LINE_ALIGNMENT_AUTOMATIC)
            {
                if (crossFlex != UI_FLEX_ALIGNMENT_NONE)
                    alignment = ITEM_LINE_ALIGNMENT_STRETCH;
                else if (horizontal)
                    alignment = layout->getVerticalAlignment() == VERTICAL_ALIGNMENT_CENTER ? ITEM_LINE_ALIGNMENT_CENTER
                        : layout->getVerticalAlignment() == VERTICAL_ALIGNMENT_BOTTOM ? ITEM_LINE_ALIGNMENT_END : ITEM_LINE_ALIGNMENT_START;
                else
                    alignment = layout->getHorizontalAlignment() == HORIZONTAL_ALIGNMENT_CENTER ? ITEM_LINE_ALIGNMENT_CENTER
                        : layout->getHorizontalAlignment() == HORIZONTAL_ALIGNMENT_RIGHT ? ITEM_LINE_ALIGNMENT_END : ITEM_LINE_ALIGNMENT_START;
            }
            const float itemCross = crossValue(result[index].size, horizontal);
            float offset = 0.0f;
            if (alignment == ITEM_LINE_ALIGNMENT_CENTER)
                offset = (line.crossSize - itemCross) * 0.5f;
            else if (alignment == ITEM_LINE_ALIGNMENT_END)
                offset = line.crossSize - itemCross;
            else if (alignment == ITEM_LINE_ALIGNMENT_STRETCH)
                setCrossValue(result[index].size, horizontal, line.crossSize);
            setCrossValue(result[index].position, horizontal, crossCursor + offset);
            result[index].position += viewport.x0y0();
        }
        crossCursor += line.crossSize + authoredGap + crossBetween;
    }
    return result;
}
}

Vector2 UIListLayout::getAbsoluteContentSize() const
{
    const GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return Vector2::zero();
    const Rect2D viewport = Rect2D::xywh(Vector2::zero(), parent->getAbsoluteSize());
    const std::vector<const GuiObject*> children = orderedChildren(parent, sortOrder);
    const std::vector<ListPlacement> placements = calculateListPlacements(this, parent, viewport, children);
    Vector2 content = Vector2::zero();
    for (size_t index = 0; index < placements.size(); ++index)
        content = content.max(placements[index].position + placements[index].size);
    return content;
}

void UIListLayout::applyLayout(const GuiObject* child, const Rect2D& viewport,
    Vector2& position, Vector2& size) const
{
    const GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return;
    const std::vector<const GuiObject*> children = orderedChildren(parent, sortOrder);
    const size_t target = childIndex(children, child);
    if (target == children.size())
        return;
    const std::vector<ListPlacement> placements = calculateListPlacements(this, parent, viewport, children);
    position = placements[target].position;
    size = placements[target].size;
}

UIGridLayout::UIGridLayout()
    : DescribedCreatable<UIGridLayout, UIComponent, sUIGridLayout>("UIGridLayout")
    , cellPadding(0.0f, 5, 0.0f, 5)
    , cellSize(0.0f, 100, 0.0f, 100)
    , fillDirectionMaxCells(0)
    , fillDirection(FILL_DIRECTION_HORIZONTAL)
    , horizontalAlignment(HORIZONTAL_ALIGNMENT_LEFT)
    , verticalAlignment(VERTICAL_ALIGNMENT_TOP)
    , sortOrder(SORT_ORDER_NAME)
    , startCorner(START_CORNER_TOP_LEFT)
{
}

static const Reflection::PropDescriptor<UIGridLayout, UDim2> prop_CellPadding(
    "CellPadding", category_Data, &UIGridLayout::getCellPadding, &UIGridLayout::setCellPadding);
static const Reflection::PropDescriptor<UIGridLayout, UDim2> prop_CellSize(
    "CellSize", category_Data, &UIGridLayout::getCellSize, &UIGridLayout::setCellSize);
static const Reflection::PropDescriptor<UIGridLayout, int> prop_FillDirectionMaxCells(
    "FillDirectionMaxCells", category_Data, &UIGridLayout::getFillDirectionMaxCells, &UIGridLayout::setFillDirectionMaxCells);
static const Reflection::EnumPropDescriptor<UIGridLayout, FillDirection> prop_GridFillDirection(
    "FillDirection", category_Data, &UIGridLayout::getFillDirection, &UIGridLayout::setFillDirection);
static const Reflection::EnumPropDescriptor<UIGridLayout, HorizontalAlignment> prop_GridHorizontalAlignment(
    "HorizontalAlignment", category_Data, &UIGridLayout::getHorizontalAlignment, &UIGridLayout::setHorizontalAlignment);
static const Reflection::EnumPropDescriptor<UIGridLayout, VerticalAlignment> prop_GridVerticalAlignment(
    "VerticalAlignment", category_Data, &UIGridLayout::getVerticalAlignment, &UIGridLayout::setVerticalAlignment);
static const Reflection::EnumPropDescriptor<UIGridLayout, SortOrder> prop_GridSortOrder(
    "SortOrder", category_Data, &UIGridLayout::getSortOrder, &UIGridLayout::setSortOrder);
static const Reflection::EnumPropDescriptor<UIGridLayout, StartCorner> prop_GridStartCorner(
    "StartCorner", category_Data, &UIGridLayout::getStartCorner, &UIGridLayout::setStartCorner);
static const Reflection::PropDescriptor<UIGridLayout, Vector2> prop_GridAbsoluteContentSize(
    "AbsoluteContentSize", category_Data, &UIGridLayout::getAbsoluteContentSize, NULL,
    Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<UIGridLayout, Vector2> prop_GridAbsoluteCellSize(
    "AbsoluteCellSize", category_Data, &UIGridLayout::getAbsoluteCellSize, NULL,
    Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<UIGridLayout, Vector2> prop_GridAbsoluteCellCount(
    "AbsoluteCellCount", category_Data, &UIGridLayout::getAbsoluteCellCount, NULL,
    Reflection::PropertyDescriptor::UI);

#define RBX_UI_GRID_SETTER(Type, Name, member, descriptor) \
void UIGridLayout::set##Name(Type value) \
{ \
    if (member != value) \
    { \
        member = value; \
        raisePropertyChanged(descriptor); \
        invalidateParentLayout(); \
        raisePropertyChanged(prop_GridAbsoluteContentSize); \
    } \
}

RBX_UI_GRID_SETTER(UDim2, CellPadding, cellPadding, prop_CellPadding)
RBX_UI_GRID_SETTER(UDim2, CellSize, cellSize, prop_CellSize)
RBX_UI_GRID_SETTER(FillDirection, FillDirection, fillDirection, prop_GridFillDirection)
RBX_UI_GRID_SETTER(HorizontalAlignment, HorizontalAlignment, horizontalAlignment, prop_GridHorizontalAlignment)
RBX_UI_GRID_SETTER(VerticalAlignment, VerticalAlignment, verticalAlignment, prop_GridVerticalAlignment)
RBX_UI_GRID_SETTER(SortOrder, SortOrder, sortOrder, prop_GridSortOrder)
RBX_UI_GRID_SETTER(StartCorner, StartCorner, startCorner, prop_GridStartCorner)
#undef RBX_UI_GRID_SETTER

void UIGridLayout::setFillDirectionMaxCells(int value)
{
    value = std::max(0, value);
    if (fillDirectionMaxCells != value)
    {
        fillDirectionMaxCells = value;
        raisePropertyChanged(prop_FillDirectionMaxCells);
        invalidateParentLayout();
        raisePropertyChanged(prop_GridAbsoluteContentSize);
    }
}

Vector2 UIGridLayout::getAbsoluteContentSize() const
{
    const GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return Vector2::zero();
    const Vector2 viewportSize = parent->getAbsoluteSize();
    const Vector2 size = cellSize * viewportSize;
    const Vector2 gap = cellPadding * viewportSize;
    const size_t count = orderedChildren(parent, sortOrder).size();
    if (count == 0)
        return Vector2::zero();
    size_t major = fillDirectionMaxCells > 0 ? std::min(count, static_cast<size_t>(fillDirectionMaxCells)) : count;
    size_t minor = (count + major - 1) / major;
    return fillDirection == FILL_DIRECTION_HORIZONTAL
        ? Vector2(major * size.x + (major - 1) * gap.x, minor * size.y + (minor - 1) * gap.y)
        : Vector2(minor * size.x + (minor - 1) * gap.x, major * size.y + (major - 1) * gap.y);
}

Vector2 UIGridLayout::getAbsoluteCellSize() const
{
    const GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    return parent ? cellSize * parent->getAbsoluteSize() : Vector2::zero();
}

Vector2 UIGridLayout::getAbsoluteCellCount() const
{
    const GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return Vector2::zero();
    const size_t count = orderedChildren(parent, sortOrder).size();
    if (count == 0)
        return Vector2::zero();
    const size_t major = fillDirectionMaxCells > 0
        ? std::min(count, static_cast<size_t>(fillDirectionMaxCells)) : count;
    const size_t minor = (count + major - 1) / major;
    return fillDirection == FILL_DIRECTION_HORIZONTAL
        ? Vector2(static_cast<float>(major), static_cast<float>(minor))
        : Vector2(static_cast<float>(minor), static_cast<float>(major));
}

void UIGridLayout::applyLayout(const GuiObject* child, const Rect2D& viewport,
    Vector2& position, Vector2& size) const
{
    const GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return;
    const std::vector<const GuiObject*> children = orderedChildren(parent, sortOrder);
    const size_t index = childIndex(children, child);
    if (index == children.size())
        return;
    const Vector2 viewportSize = viewport.wh();
    size = cellSize * viewportSize;
    const Vector2 gap = cellPadding * viewportSize;
    const size_t major = fillDirectionMaxCells > 0
        ? std::min(children.size(), static_cast<size_t>(fillDirectionMaxCells)) : children.size();
    size_t row = fillDirection == FILL_DIRECTION_HORIZONTAL ? index / major : index % major;
    size_t column = fillDirection == FILL_DIRECTION_HORIZONTAL ? index % major : index / major;
    const Vector2 counts = getAbsoluteCellCount();
    const size_t columns = static_cast<size_t>(counts.x);
    const size_t rows = static_cast<size_t>(counts.y);
    if (startCorner == START_CORNER_TOP_RIGHT || startCorner == START_CORNER_BOTTOM_RIGHT)
        column = columns - 1 - column;
    if (startCorner == START_CORNER_BOTTOM_LEFT || startCorner == START_CORNER_BOTTOM_RIGHT)
        row = rows - 1 - row;
    const Vector2 content = getAbsoluteContentSize();
    Vector2 origin = viewport.x0y0();
    if (horizontalAlignment == HORIZONTAL_ALIGNMENT_CENTER)
        origin.x += (viewportSize.x - content.x) * 0.5f;
    else if (horizontalAlignment == HORIZONTAL_ALIGNMENT_RIGHT)
        origin.x += viewportSize.x - content.x;
    if (verticalAlignment == VERTICAL_ALIGNMENT_CENTER)
        origin.y += (viewportSize.y - content.y) * 0.5f;
    else if (verticalAlignment == VERTICAL_ALIGNMENT_BOTTOM)
        origin.y += viewportSize.y - content.y;
    position = origin + Vector2(column * (size.x + gap.x), row * (size.y + gap.y));
}

UIPageLayout::UIPageLayout()
    : DescribedCreatable<UIPageLayout, UIComponent, sUIPageLayout>("UIPageLayout")
    , IStepped(StepType_Render)
    , circular(false)
    , easingDirection(GuiObject::EASING_DIRECTION_OUT)
    , easingStyle(GuiObject::EASING_STYLE_QUAD)
    , gamepadInputEnabled(true)
    , horizontalAlignment(HORIZONTAL_ALIGNMENT_CENTER)
    , padding(0.0f, 0)
    , scrollWheelInputEnabled(true)
    , sortOrder(SORT_ORDER_LAYOUT_ORDER)
    , touchInputEnabled(true)
    , tweenTime(1.0f)
    , verticalAlignment(VERTICAL_ALIGNMENT_CENTER)
    , animating(false)
    , animationElapsed(0.0)
    , animationStartLogical(0.0)
    , animationEndLogical(0.0)
{
}

static const Reflection::PropDescriptor<UIPageLayout, bool> prop_PageAnimated(
    "Animated", category_Data, &UIPageLayout::getAnimated, NULL,
    Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<UIPageLayout, bool> prop_PageCircular(
    "Circular", category_Data, &UIPageLayout::getCircular, &UIPageLayout::setCircular);
static const Reflection::RefPropDescriptor<UIPageLayout, GuiObject> prop_PageCurrentPage(
    "CurrentPage", category_Data, &UIPageLayout::getCurrentPage, NULL,
    Reflection::PropertyDescriptor::UI);
static const Reflection::EnumPropDescriptor<UIPageLayout, GuiObject::TweenEasingDirection> prop_PageEasingDirection(
    "EasingDirection", category_Data, &UIPageLayout::getEasingDirection, &UIPageLayout::setEasingDirection);
static const Reflection::EnumPropDescriptor<UIPageLayout, GuiObject::TweenEasingStyle> prop_PageEasingStyle(
    "EasingStyle", category_Data, &UIPageLayout::getEasingStyle, &UIPageLayout::setEasingStyle);
static const Reflection::PropDescriptor<UIPageLayout, bool> prop_PageGamepadInputEnabled(
    "GamepadInputEnabled", category_Behavior, &UIPageLayout::getGamepadInputEnabled, &UIPageLayout::setGamepadInputEnabled);
static const Reflection::EnumPropDescriptor<UIPageLayout, HorizontalAlignment> prop_PageHorizontalAlignment(
    "HorizontalAlignment", category_Data, &UIPageLayout::getHorizontalAlignment, &UIPageLayout::setHorizontalAlignment);
static const Reflection::PropDescriptor<UIPageLayout, UDim> prop_PagePadding(
    "Padding", category_Data, &UIPageLayout::getPadding, &UIPageLayout::setPadding);
static const Reflection::PropDescriptor<UIPageLayout, bool> prop_PageScrollWheelInputEnabled(
    "ScrollWheelInputEnabled", category_Behavior, &UIPageLayout::getScrollWheelInputEnabled, &UIPageLayout::setScrollWheelInputEnabled);
static const Reflection::EnumPropDescriptor<UIPageLayout, SortOrder> prop_PageSortOrder(
    "SortOrder", category_Data, &UIPageLayout::getSortOrder, &UIPageLayout::setSortOrder);
static const Reflection::PropDescriptor<UIPageLayout, bool> prop_PageTouchInputEnabled(
    "TouchInputEnabled", category_Behavior, &UIPageLayout::getTouchInputEnabled, &UIPageLayout::setTouchInputEnabled);
static const Reflection::PropDescriptor<UIPageLayout, float> prop_PageTweenTime(
    "TweenTime", category_Data, &UIPageLayout::getTweenTime, &UIPageLayout::setTweenTime);
static const Reflection::EnumPropDescriptor<UIPageLayout, VerticalAlignment> prop_PageVerticalAlignment(
    "VerticalAlignment", category_Data, &UIPageLayout::getVerticalAlignment, &UIPageLayout::setVerticalAlignment);

static const Reflection::BoundFuncDesc<UIPageLayout, void()> func_PageApplyLayout(
    &UIPageLayout::applyLayoutNow, "ApplyLayout", Security::None);
static const Reflection::BoundFuncDesc<UIPageLayout, void(shared_ptr<Instance>)> func_PageJumpTo(
    &UIPageLayout::jumpTo, "JumpTo", "page", Security::None);
static const Reflection::BoundFuncDesc<UIPageLayout, void(int)> func_PageJumpToIndex(
    &UIPageLayout::jumpToIndex, "JumpToIndex", "index", Security::None);
static const Reflection::BoundFuncDesc<UIPageLayout, void()> func_PageNext(
    &UIPageLayout::next, "Next", Security::None);
static const Reflection::BoundFuncDesc<UIPageLayout, void()> func_PagePrevious(
    &UIPageLayout::previous, "Previous", Security::None);
static const Reflection::EventDesc<UIPageLayout, void(shared_ptr<Instance>)> event_PageEnter(
    &UIPageLayout::pageEnterSignal, "PageEnter", "page", Security::None);
static const Reflection::EventDesc<UIPageLayout, void(shared_ptr<Instance>)> event_PageLeave(
    &UIPageLayout::pageLeaveSignal, "PageLeave", "page", Security::None);
static const Reflection::EventDesc<UIPageLayout, void(shared_ptr<Instance>)> event_PageStopped(
    &UIPageLayout::stoppedSignal, "Stopped", "currentPage", Security::None);

#define RBX_UI_PAGE_SETTER(Type, Name, member, descriptor) \
void UIPageLayout::set##Name(Type value) \
{ \
    if (member != value) \
    { \
        member = value; \
        raisePropertyChanged(descriptor); \
        invalidateParentLayout(); \
    } \
}

RBX_UI_PAGE_SETTER(bool, Circular, circular, prop_PageCircular)
RBX_UI_PAGE_SETTER(GuiObject::TweenEasingDirection, EasingDirection, easingDirection, prop_PageEasingDirection)
RBX_UI_PAGE_SETTER(GuiObject::TweenEasingStyle, EasingStyle, easingStyle, prop_PageEasingStyle)
RBX_UI_PAGE_SETTER(bool, GamepadInputEnabled, gamepadInputEnabled, prop_PageGamepadInputEnabled)
RBX_UI_PAGE_SETTER(HorizontalAlignment, HorizontalAlignment, horizontalAlignment, prop_PageHorizontalAlignment)
RBX_UI_PAGE_SETTER(UDim, Padding, padding, prop_PagePadding)
RBX_UI_PAGE_SETTER(bool, ScrollWheelInputEnabled, scrollWheelInputEnabled, prop_PageScrollWheelInputEnabled)
RBX_UI_PAGE_SETTER(SortOrder, SortOrder, sortOrder, prop_PageSortOrder)
RBX_UI_PAGE_SETTER(bool, TouchInputEnabled, touchInputEnabled, prop_PageTouchInputEnabled)
RBX_UI_PAGE_SETTER(VerticalAlignment, VerticalAlignment, verticalAlignment, prop_PageVerticalAlignment)
#undef RBX_UI_PAGE_SETTER

void UIPageLayout::setTweenTime(float value)
{
    value = std::max(0.0f, value);
    if (tweenTime != value)
    {
        tweenTime = value;
        raisePropertyChanged(prop_PageTweenTime);
        if (animating && tweenTime == 0.0f)
            finishAnimation();
    }
}

GuiObject* UIPageLayout::getCurrentPage() const
{
    return currentPage.lock().get();
}

void UIPageLayout::reconcilePages(const std::vector<const GuiObject*>& children)
{
    GuiObject* selected = getCurrentPage();
    if (selected && childIndex(children, selected) != children.size())
        return;

    if (animating)
    {
        animating = false;
        raisePropertyChanged(prop_PageAnimated);
    }

    shared_ptr<Instance> oldPage;
    if (shared_ptr<GuiObject> old = currentPage.lock())
        oldPage = old;
    currentPage.reset();

    if (!children.empty())
    {
        size_t index = static_cast<size_t>(std::max(0.0, std::min(
            static_cast<double>(children.size() - 1), std::floor(animationEndLogical + 0.5))));
        currentPage = weak_from(const_cast<GuiObject*>(children[index]));
        animationStartLogical = animationEndLogical = static_cast<double>(index);
    }
    else
        animationStartLogical = animationEndLogical = 0.0;

    raisePropertyChanged(prop_PageCurrentPage);
    if (oldPage)
        pageLeaveSignal(oldPage);
    if (shared_ptr<GuiObject> page = currentPage.lock())
        pageEnterSignal(page);
}

double UIPageLayout::visualLogicalPosition() const
{
    const double progress = easedProgress();
    return animationStartLogical + (animationEndLogical - animationStartLogical) * progress;
}

float UIPageLayout::easedProgress() const
{
    if (!animating || tweenTime <= 0.0f)
        return 1.0f;
    const float elapsed = std::min(static_cast<float>(animationElapsed), tweenTime);
    return GuiObject::TweenInterpolate(easingDirection, easingStyle, elapsed, tweenTime,
        UDim2(0.0f, 0, 0.0f, 0), UDim2(1.0f, 0, 0.0f, 0)).x.scale;
}

void UIPageLayout::selectPage(size_t index, const std::vector<const GuiObject*>& children)
{
    if (children.empty() || index >= children.size())
        return;

    reconcilePages(children);
    GuiObject* oldPage = getCurrentPage();
    const size_t oldIndex = childIndex(children, oldPage);
    if (oldIndex == children.size())
        return;
    if (oldIndex == index && !animating)
        return;

    const double start = visualLogicalPosition();
    double delta = static_cast<double>(index) - static_cast<double>(oldIndex);
    if (circular && children.size() > 1)
    {
        const double count = static_cast<double>(children.size());
        if (delta > count * 0.5)
            delta -= count;
        else if (delta < -count * 0.5)
            delta += count;
    }
    const double target = animationEndLogical + delta;

    shared_ptr<Instance> oldInstance = shared_from(oldPage);
    shared_ptr<GuiObject> newPage = shared_from(const_cast<GuiObject*>(children[index]));
    currentPage = newPage;
    raisePropertyChanged(prop_PageCurrentPage);
    pageLeaveSignal(oldInstance);
    pageEnterSignal(newPage);

    animationStartLogical = start;
    animationEndLogical = target;
    animationElapsed = 0.0;
    if (tweenTime <= 0.0f || animationStartLogical == animationEndLogical)
    {
        animating = false;
        animationStartLogical = animationEndLogical;
        invalidateParentLayout();
        stoppedSignal(newPage);
        return;
    }

    if (!animating)
    {
        animating = true;
        raisePropertyChanged(prop_PageAnimated);
    }
    invalidateParentLayout();
}

void UIPageLayout::applyLayoutNow()
{
    GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return;
    reconcilePages(orderedChildren(parent, sortOrder));
    parent->invalidateLayout();
}

void UIPageLayout::jumpTo(shared_ptr<Instance> page)
{
    GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    GuiObject* target = Instance::fastDynamicCast<GuiObject>(page.get());
    if (!parent || !target || target->getParent() != parent)
        return;
    const std::vector<const GuiObject*> children = orderedChildren(parent, sortOrder);
    const size_t index = childIndex(children, target);
    if (index != children.size())
        selectPage(index, children);
}

void UIPageLayout::jumpToIndex(int index)
{
    GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return;
    const std::vector<const GuiObject*> children = orderedChildren(parent, sortOrder);
    if (index >= 0 && static_cast<size_t>(index) < children.size())
        selectPage(static_cast<size_t>(index), children);
}

void UIPageLayout::next()
{
    GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return;
    const std::vector<const GuiObject*> children = orderedChildren(parent, sortOrder);
    reconcilePages(children);
    const size_t current = childIndex(children, getCurrentPage());
    if (current == children.size() || children.empty())
        return;
    if (current + 1 < children.size())
        selectPage(current + 1, children);
    else if (circular)
        selectPage(0, children);
}

void UIPageLayout::previous()
{
    GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return;
    const std::vector<const GuiObject*> children = orderedChildren(parent, sortOrder);
    reconcilePages(children);
    const size_t current = childIndex(children, getCurrentPage());
    if (current == children.size() || children.empty())
        return;
    if (current > 0)
        selectPage(current - 1, children);
    else if (circular)
        selectPage(children.size() - 1, children);
}

void UIPageLayout::finishAnimation()
{
    if (!animating)
        return;
    animating = false;
    animationElapsed = tweenTime;
    animationStartLogical = animationEndLogical;
    raisePropertyChanged(prop_PageAnimated);
    invalidateParentLayout();
    if (shared_ptr<GuiObject> page = currentPage.lock())
        stoppedSignal(page);
}

void UIPageLayout::stepAnimation(double timeStep)
{
    if (!animating)
        return;
    animationElapsed += std::max(0.0, timeStep);
    if (animationElapsed >= tweenTime)
        finishAnimation();
    else
        invalidateParentLayout();
}

void UIPageLayout::onStepped(const Stepped& event)
{
    stepAnimation(event.gameStep);
}

void UIPageLayout::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    Super::onServiceProvider(oldProvider, newProvider);
    onServiceProviderIStepped(oldProvider, newProvider);
}

void UIPageLayout::applyLayout(const GuiObject* child, const Rect2D& viewport,
    Vector2& position, Vector2& size) const
{
    const GuiObject* parent = Instance::fastDynamicCast<GuiObject>(getParent());
    if (!parent)
        return;
    const std::vector<const GuiObject*> children = orderedChildren(parent, sortOrder);
    UIPageLayout* mutableThis = const_cast<UIPageLayout*>(this);
    mutableThis->reconcilePages(children);
    const size_t target = childIndex(children, child);
    const size_t current = childIndex(children, getCurrentPage());
    if (target == children.size() || current == children.size())
        return;

    const Vector2 viewportSize = viewport.wh();
    const float gap = resolve(padding, viewportSize.x);
    std::vector<float> widths(children.size());
    std::vector<float> centers(children.size());
    for (size_t index = 0; index < children.size(); ++index)
    {
        widths[index] = authoredSize(children[index], viewportSize).x;
        if (index == 0)
            centers[index] = widths[index] * 0.5f;
        else
            centers[index] = centers[index - 1] + widths[index - 1] * 0.5f + gap + widths[index] * 0.5f;
    }

    const double logical = visualLogicalPosition();
    const long logicalFloor = static_cast<long>(std::floor(logical));
    const float logicalFraction = static_cast<float>(logical - logicalFloor);
    const long count = static_cast<long>(children.size());
    const float cycle = centers.back() + widths.back() * 0.5f + gap + widths.front() * 0.5f;

    const boost::function<float(long)> centerAt = [&, count, cycle](long logicalIndex) {
        if (!circular)
        {
            const long clamped = std::max(0L, std::min(count - 1, logicalIndex));
            return centers[static_cast<size_t>(clamped)];
        }
        long cycleIndex = logicalIndex / count;
        long wrapped = logicalIndex % count;
        if (wrapped < 0)
        {
            wrapped += count;
            --cycleIndex;
        }
        return centers[static_cast<size_t>(wrapped)] + static_cast<float>(cycleIndex) * cycle;
    };
    const boost::function<float(long)> widthAt = [&, count](long logicalIndex) {
        long wrapped = logicalIndex % count;
        if (wrapped < 0)
            wrapped += count;
        if (!circular)
            wrapped = std::max(0L, std::min(count - 1, logicalIndex));
        return widths[static_cast<size_t>(wrapped)];
    };

    const float scrollCenter = centerAt(logicalFloor) * (1.0f - logicalFraction) +
        centerAt(logicalFloor + 1) * logicalFraction;
    const float displayedWidth = widthAt(logicalFloor) * (1.0f - logicalFraction) +
        widthAt(logicalFloor + 1) * logicalFraction;
    float targetCenter = displayedWidth * 0.5f;
    if (horizontalAlignment == HORIZONTAL_ALIGNMENT_CENTER)
        targetCenter = viewportSize.x * 0.5f;
    else if (horizontalAlignment == HORIZONTAL_ALIGNMENT_RIGHT)
        targetCenter = viewportSize.x - displayedWidth * 0.5f;

    float childCenter = centers[target];
    if (circular && cycle > 0.0f)
        childCenter += std::floor((scrollCenter - childCenter) / cycle + 0.5f) * cycle;
    const float x = targetCenter + childCenter - scrollCenter - widths[target] * 0.5f;

    const float childHeight = size.y;
    float y = 0.0f;
    if (verticalAlignment == VERTICAL_ALIGNMENT_CENTER)
        y = (viewportSize.y - childHeight) * 0.5f;
    else if (verticalAlignment == VERTICAL_ALIGNMENT_BOTTOM)
        y = viewportSize.y - childHeight;
    position = viewport.x0y0() + Vector2(x, y);
}

UITextSizeConstraint::UITextSizeConstraint()
    : DescribedCreatable<UITextSizeConstraint, UIComponent, sUITextSizeConstraint>("UITextSizeConstraint")
    , minTextSize(1)
    , maxTextSize(100)
{
}

static const Reflection::PropDescriptor<UITextSizeConstraint, int> prop_MinTextSize(
    "MinTextSize", category_Data, &UITextSizeConstraint::getMinTextSize, &UITextSizeConstraint::setMinTextSize);
static const Reflection::PropDescriptor<UITextSizeConstraint, int> prop_MaxTextSize(
    "MaxTextSize", category_Data, &UITextSizeConstraint::getMaxTextSize, &UITextSizeConstraint::setMaxTextSize);

void UITextSizeConstraint::setMinTextSize(int value)
{
    value = std::max(1, value);
    if (minTextSize != value)
    {
        minTextSize = value;
        if (maxTextSize < minTextSize)
            maxTextSize = minTextSize;
        raisePropertyChanged(prop_MinTextSize);
        invalidateParentLayout();
    }
}

void UITextSizeConstraint::setMaxTextSize(int value)
{
    value = std::max(1, value);
    if (maxTextSize != value)
    {
        maxTextSize = value;
        if (minTextSize > maxTextSize)
            minTextSize = maxTextSize;
        raisePropertyChanged(prop_MaxTextSize);
        invalidateParentLayout();
    }
}

UISizeConstraint::UISizeConstraint()
    : DescribedCreatable<UISizeConstraint, UIComponent, sUISizeConstraint>("UISizeConstraint")
    , minSize(Vector2::zero())
    , maxSize(std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
{
}

static const Reflection::PropDescriptor<UISizeConstraint, Vector2> prop_MinSize(
    "MinSize", category_Data, &UISizeConstraint::getMinSize, &UISizeConstraint::setMinSize);
static const Reflection::PropDescriptor<UISizeConstraint, Vector2> prop_MaxSize(
    "MaxSize", category_Data, &UISizeConstraint::getMaxSize, &UISizeConstraint::setMaxSize);

void UISizeConstraint::setMinSize(Vector2 value)
{
    value.x = std::max(0.0f, value.x);
    value.y = std::max(0.0f, value.y);
    if (minSize != value)
    {
        minSize = value;
        maxSize.x = std::max(maxSize.x, minSize.x);
        maxSize.y = std::max(maxSize.y, minSize.y);
        raisePropertyChanged(prop_MinSize);
        invalidateParentLayout();
    }
}

void UISizeConstraint::setMaxSize(Vector2 value)
{
    value.x = std::max(0.0f, value.x);
    value.y = std::max(0.0f, value.y);
    if (maxSize != value)
    {
        maxSize = value;
        minSize.x = std::min(minSize.x, maxSize.x);
        minSize.y = std::min(minSize.y, maxSize.y);
        raisePropertyChanged(prop_MaxSize);
        invalidateParentLayout();
    }
}

Vector2 UISizeConstraint::constrain(const Vector2& size) const
{
    return Vector2(
        std::clamp(size.x, minSize.x, maxSize.x),
        std::clamp(size.y, minSize.y, maxSize.y));
}

UIAspectRatioConstraint::UIAspectRatioConstraint()
    : DescribedCreatable<UIAspectRatioConstraint, UIComponent, sUIAspectRatioConstraint>("UIAspectRatioConstraint")
    , aspectRatio(1.0f)
    , aspectType(ASPECT_TYPE_FIT_WITHIN_MAX_SIZE)
    , dominantAxis(DOMINANT_AXIS_WIDTH)
{
}

static const Reflection::PropDescriptor<UIAspectRatioConstraint, float> prop_AspectRatio(
    "AspectRatio", category_Data, &UIAspectRatioConstraint::getAspectRatio, &UIAspectRatioConstraint::setAspectRatio);
static const Reflection::EnumPropDescriptor<UIAspectRatioConstraint, AspectType> prop_AspectType(
    "AspectType", category_Data, &UIAspectRatioConstraint::getAspectType, &UIAspectRatioConstraint::setAspectType);
static const Reflection::EnumPropDescriptor<UIAspectRatioConstraint, DominantAxis> prop_DominantAxis(
    "DominantAxis", category_Data, &UIAspectRatioConstraint::getDominantAxis, &UIAspectRatioConstraint::setDominantAxis);

void UIAspectRatioConstraint::setAspectRatio(float value)
{
    value = std::max(value, std::numeric_limits<float>::epsilon());
    if (aspectRatio != value)
    {
        aspectRatio = value;
        raisePropertyChanged(prop_AspectRatio);
        invalidateParentLayout();
    }
}

void UIAspectRatioConstraint::setAspectType(AspectType value)
{
    if (aspectType != value)
    {
        aspectType = value;
        raisePropertyChanged(prop_AspectType);
        invalidateParentLayout();
    }
}

void UIAspectRatioConstraint::setDominantAxis(DominantAxis value)
{
    if (dominantAxis != value)
    {
        dominantAxis = value;
        raisePropertyChanged(prop_DominantAxis);
        invalidateParentLayout();
    }
}

Vector2 UIAspectRatioConstraint::constrain(const Vector2& size, const Vector2& parentSize) const
{
    const Vector2 bounds = aspectType == ASPECT_TYPE_SCALE_WITH_PARENT_SIZE ? parentSize : size;
    Vector2 result = bounds;
    if (dominantAxis == DOMINANT_AXIS_WIDTH)
    {
        result.y = result.x / aspectRatio;
        if (result.y > bounds.y)
        {
            result.y = bounds.y;
            result.x = result.y * aspectRatio;
        }
    }
    else
    {
        result.x = result.y * aspectRatio;
        if (result.x > bounds.x)
        {
            result.x = bounds.x;
            result.y = result.x / aspectRatio;
        }
    }
    return Vector2(std::max(0.0f, result.x), std::max(0.0f, result.y));
}

template<class Component>
const Component* findComponent(const GuiObject* parent)
{
    const copy_on_write_ptr<Instances>& children = parent->getChildren();
    if (!children)
        return NULL;
    for (Instances::const_iterator it = children->begin(); it != children->end(); ++it)
        if (const Component* component = Instance::fastDynamicCast<Component>(it->get()))
            return component;
    return NULL;
}

const UIPadding* findUIPadding(const GuiObject* parent) { return findComponent<UIPadding>(parent); }
const UIScale* findUIScale(const GuiObject* parent) { return findComponent<UIScale>(parent); }
const UICorner* findUICorner(const GuiObject* parent) { return findComponent<UICorner>(parent); }
const UITextSizeConstraint* findUITextSizeConstraint(const GuiObject* parent) { return findComponent<UITextSizeConstraint>(parent); }
const UISizeConstraint* findUISizeConstraint(const GuiObject* parent) { return findComponent<UISizeConstraint>(parent); }
const UIAspectRatioConstraint* findUIAspectRatioConstraint(const GuiObject* parent) { return findComponent<UIAspectRatioConstraint>(parent); }
const UIFlexItem* findUIFlexItem(const GuiObject* parent) { return findComponent<UIFlexItem>(parent); }
const UIGradient* findUIGradient(const Instance* parent)
{
    const copy_on_write_ptr<Instances>& children = parent->getChildren();
    if (!children)
        return NULL;
    for (Instances::const_iterator it = children->begin(); it != children->end(); ++it)
        if (const UIGradient* gradient = Instance::fastDynamicCast<UIGradient>(it->get()))
            return gradient;
    return NULL;
}

std::vector<const UIStroke*> findUIStrokes(const GuiObject* parent)
{
    std::vector<const UIStroke*> result;
    const copy_on_write_ptr<Instances>& children = parent->getChildren();
    if (children)
        for (Instances::const_iterator it = children->begin(); it != children->end(); ++it)
            if (const UIStroke* stroke = Instance::fastDynamicCast<UIStroke>(it->get()))
                result.push_back(stroke);
    std::stable_sort(result.begin(), result.end(), [](const UIStroke* left, const UIStroke* right) {
        return left->getZIndex() < right->getZIndex();
    });
    return result;
}

const UIComponent* findUILayout(const GuiObject* parent)
{
    if (const UIListLayout* list = findComponent<UIListLayout>(parent))
        return list;
    if (const UIGridLayout* grid = findComponent<UIGridLayout>(parent))
        return grid;
    return findComponent<UIPageLayout>(parent);
}

namespace Reflection
{
template<> EnumDesc<RBX::FillDirection>::EnumDesc() : EnumDescriptor("FillDirection")
{
    addPair(RBX::FILL_DIRECTION_HORIZONTAL, "Horizontal");
    addPair(RBX::FILL_DIRECTION_VERTICAL, "Vertical");
}
template<> EnumDesc<RBX::HorizontalAlignment>::EnumDesc() : EnumDescriptor("HorizontalAlignment")
{
    addPair(RBX::HORIZONTAL_ALIGNMENT_CENTER, "Center");
    addPair(RBX::HORIZONTAL_ALIGNMENT_LEFT, "Left");
    addPair(RBX::HORIZONTAL_ALIGNMENT_RIGHT, "Right");
}
template<> EnumDesc<RBX::VerticalAlignment>::EnumDesc() : EnumDescriptor("VerticalAlignment")
{
    addPair(RBX::VERTICAL_ALIGNMENT_CENTER, "Center");
    addPair(RBX::VERTICAL_ALIGNMENT_TOP, "Top");
    addPair(RBX::VERTICAL_ALIGNMENT_BOTTOM, "Bottom");
}
template<> EnumDesc<RBX::SortOrder>::EnumDesc() : EnumDescriptor("SortOrder")
{
    addPair(RBX::SORT_ORDER_NAME, "Name");
    addPair(RBX::SORT_ORDER_LAYOUT_ORDER, "LayoutOrder");
}
template<> EnumDesc<RBX::StartCorner>::EnumDesc() : EnumDescriptor("StartCorner")
{
    addPair(RBX::START_CORNER_TOP_LEFT, "TopLeft");
    addPair(RBX::START_CORNER_TOP_RIGHT, "TopRight");
    addPair(RBX::START_CORNER_BOTTOM_LEFT, "BottomLeft");
    addPair(RBX::START_CORNER_BOTTOM_RIGHT, "BottomRight");
}
template<> EnumDesc<RBX::AspectType>::EnumDesc() : EnumDescriptor("AspectType")
{
    addPair(RBX::ASPECT_TYPE_FIT_WITHIN_MAX_SIZE, "FitWithinMaxSize");
    addPair(RBX::ASPECT_TYPE_SCALE_WITH_PARENT_SIZE, "ScaleWithParentSize");
}
template<> EnumDesc<RBX::DominantAxis>::EnumDesc() : EnumDescriptor("DominantAxis")
{
    addPair(RBX::DOMINANT_AXIS_WIDTH, "Width");
    addPair(RBX::DOMINANT_AXIS_HEIGHT, "Height");
}
template<> EnumDesc<RBX::UIFlexAlignment>::EnumDesc() : EnumDescriptor("UIFlexAlignment")
{
    addPair(RBX::UI_FLEX_ALIGNMENT_NONE, "None");
    addPair(RBX::UI_FLEX_ALIGNMENT_FILL, "Fill");
    addPair(RBX::UI_FLEX_ALIGNMENT_SPACE_AROUND, "SpaceAround");
    addPair(RBX::UI_FLEX_ALIGNMENT_SPACE_BETWEEN, "SpaceBetween");
    addPair(RBX::UI_FLEX_ALIGNMENT_SPACE_EVENLY, "SpaceEvenly");
}
template<> EnumDesc<RBX::ItemLineAlignment>::EnumDesc() : EnumDescriptor("ItemLineAlignment")
{
    addPair(RBX::ITEM_LINE_ALIGNMENT_AUTOMATIC, "Automatic");
    addPair(RBX::ITEM_LINE_ALIGNMENT_START, "Start");
    addPair(RBX::ITEM_LINE_ALIGNMENT_CENTER, "Center");
    addPair(RBX::ITEM_LINE_ALIGNMENT_END, "End");
    addPair(RBX::ITEM_LINE_ALIGNMENT_STRETCH, "Stretch");
}
template<> EnumDesc<RBX::UIFlexMode>::EnumDesc() : EnumDescriptor("UIFlexMode")
{
    addPair(RBX::UI_FLEX_MODE_NONE, "None");
    addPair(RBX::UI_FLEX_MODE_GROW, "Grow");
    addPair(RBX::UI_FLEX_MODE_SHRINK, "Shrink");
    addPair(RBX::UI_FLEX_MODE_FILL, "Fill");
    addPair(RBX::UI_FLEX_MODE_CUSTOM, "Custom");
}
template<> EnumDesc<RBX::ApplyStrokeMode>::EnumDesc() : EnumDescriptor("ApplyStrokeMode")
{
    addPair(RBX::APPLY_STROKE_MODE_CONTEXTUAL, "Contextual");
    addPair(RBX::APPLY_STROKE_MODE_BORDER, "Border");
}
template<> EnumDesc<RBX::LineJoinMode>::EnumDesc() : EnumDescriptor("LineJoinMode")
{
    addPair(RBX::LINE_JOIN_MODE_ROUND, "Round");
    addPair(RBX::LINE_JOIN_MODE_BEVEL, "Bevel");
    addPair(RBX::LINE_JOIN_MODE_MITER, "Miter");
}
template<> EnumDesc<RBX::BorderStrokePosition>::EnumDesc() : EnumDescriptor("BorderStrokePosition")
{
    addPair(RBX::BORDER_STROKE_POSITION_OUTER, "Outer");
    addPair(RBX::BORDER_STROKE_POSITION_CENTER, "Center");
    addPair(RBX::BORDER_STROKE_POSITION_INNER, "Inner");
}
template<> EnumDesc<RBX::StrokeSizingMode>::EnumDesc() : EnumDescriptor("StrokeSizingMode")
{
    addPair(RBX::STROKE_SIZING_MODE_FIXED_SIZE, "FixedSize");
    addPair(RBX::STROKE_SIZING_MODE_SCALED_SIZE, "ScaledSize");
}
template<> EnumDesc<RBX::UIDragDetectorBoundingBehavior>::EnumDesc() : EnumDescriptor("UIDragDetectorBoundingBehavior")
{
    addPair(RBX::UI_DRAG_BOUNDING_ENTIRE_OBJECT, "EntireObject");
    addPair(RBX::UI_DRAG_BOUNDING_HIT_POINT, "HitPoint");
}
template<> EnumDesc<RBX::UIDragDetectorDragRelativity>::EnumDesc() : EnumDescriptor("UIDragDetectorDragRelativity")
{
    addPair(RBX::UI_DRAG_RELATIVITY_ABSOLUTE, "Absolute");
    addPair(RBX::UI_DRAG_RELATIVITY_RELATIVE, "Relative");
}
template<> EnumDesc<RBX::UIDragDetectorDragSpace>::EnumDesc() : EnumDescriptor("UIDragDetectorDragSpace")
{
    addPair(RBX::UI_DRAG_SPACE_PARENT, "Parent");
    addPair(RBX::UI_DRAG_SPACE_LAYER_COLLECTOR, "LayerCollector");
    addPair(RBX::UI_DRAG_SPACE_REFERENCE, "Reference");
}
template<> EnumDesc<RBX::UIDragDetectorDragStyle>::EnumDesc() : EnumDescriptor("UIDragDetectorDragStyle")
{
    addPair(RBX::UI_DRAG_STYLE_TRANSLATE_PLANE, "TranslatePlane");
    addPair(RBX::UI_DRAG_STYLE_TRANSLATE_LINE, "TranslateLine");
    addPair(RBX::UI_DRAG_STYLE_ROTATE, "Rotate");
    addPair(RBX::UI_DRAG_STYLE_SCRIPTABLE, "Scriptable");
}
template<> EnumDesc<RBX::UIDragDetectorResponseStyle>::EnumDesc() : EnumDescriptor("UIDragDetectorResponseStyle")
{
    addPair(RBX::UI_DRAG_RESPONSE_OFFSET, "Offset");
    addPair(RBX::UI_DRAG_RESPONSE_SCALE, "Scale");
    addPair(RBX::UI_DRAG_RESPONSE_CUSTOM_OFFSET, "CustomOffset");
    addPair(RBX::UI_DRAG_RESPONSE_CUSTOM_SCALE, "CustomScale");
}
}
}
