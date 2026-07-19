
#include "V8DataModel/Folder.h"
#include "V8DataModel/GuiBase2d.h"
#include "V8DataModel/GuiObject.h"
#include "v8datamodel/LocalizationService.h"
#include "v8datamodel/LocalizationTable.h"
#include "v8datamodel/StyleSheet.h"
#include "rbx/ui/ScreenLayout.h"

#include <cmath>

namespace RBX{

const char* const  sGuiBase2d = "GuiBase2d";

//Read only properties for Absolute position/Absolute size
Reflection::PropDescriptor<GuiBase2d, Vector2> GuiBase2d::prop_AbsoluteSize("AbsoluteSize", category_Data, &GuiBase2d::getAbsoluteSize, NULL, Reflection::PropertyDescriptor::UI);
Reflection::PropDescriptor<GuiBase2d, Vector2> GuiBase2d::prop_AbsolutePosition("AbsolutePosition", category_Data, &GuiBase2d::getAbsolutePosition, NULL, Reflection::PropertyDescriptor::UI);
Reflection::PropDescriptor<GuiBase2d, bool> GuiBase2d::prop_SelectionGroup(
	"SelectionGroup", "Selection", &GuiBase2d::getSelectionGroup, &GuiBase2d::setSelectionGroup);
static Reflection::PropDescriptor<GuiBase2d, bool> prop_AutoLocalize(
	"AutoLocalize", "Localization", &GuiBase2d::getAutoLocalize, &GuiBase2d::setAutoLocalize);
static Reflection::PropDescriptor<GuiBase2d, bool> prop_Localize(
	"Localize", "Localization", &GuiBase2d::getAutoLocalize, &GuiBase2d::setAutoLocalize,
	Reflection::PropertyDescriptor::Attributes::deprecated(prop_AutoLocalize));
static Reflection::RefPropDescriptor<GuiBase2d, LocalizationTable> prop_RootLocalizationTable(
	"RootLocalizationTable", "Localization", &GuiBase2d::getRootLocalizationTable,
	&GuiBase2d::setRootLocalizationTable);
Reflection::EnumPropDescriptor<GuiBase2d, Enums::SelectionBehavior> GuiBase2d::prop_SelectionBehaviorUp(
	"SelectionBehaviorUp", "Selection", &GuiBase2d::getSelectionBehaviorUp, &GuiBase2d::setSelectionBehaviorUp);
Reflection::EnumPropDescriptor<GuiBase2d, Enums::SelectionBehavior> GuiBase2d::prop_SelectionBehaviorDown(
	"SelectionBehaviorDown", "Selection", &GuiBase2d::getSelectionBehaviorDown, &GuiBase2d::setSelectionBehaviorDown);
Reflection::EnumPropDescriptor<GuiBase2d, Enums::SelectionBehavior> GuiBase2d::prop_SelectionBehaviorLeft(
	"SelectionBehaviorLeft", "Selection", &GuiBase2d::getSelectionBehaviorLeft, &GuiBase2d::setSelectionBehaviorLeft);
Reflection::EnumPropDescriptor<GuiBase2d, Enums::SelectionBehavior> GuiBase2d::prop_SelectionBehaviorRight(
	"SelectionBehaviorRight", "Selection", &GuiBase2d::getSelectionBehaviorRight, &GuiBase2d::setSelectionBehaviorRight);
static Reflection::EventDesc<GuiBase2d, void(bool, shared_ptr<Instance>, shared_ptr<Instance>)>
	event_SelectionChanged(&GuiBase2d::selectionChangedSignal, "SelectionChanged", "amISelected",
		"previousSelection", "newSelection", Security::None);

namespace Reflection {
template<>
EnumDesc<Enums::SelectionBehavior>::EnumDesc()
	: EnumDescriptor("SelectionBehavior")
{
	addPair(Enums::SELECTION_BEHAVIOR_ESCAPE, "Escape");
	addPair(Enums::SELECTION_BEHAVIOR_STOP, "Stop");
}
} // namespace Reflection


GuiBase2d::GuiBase2d(const char* name)
: Super(name)
, zIndex(GuiBase::minZIndex2d())
, guiQueue(GUIQUEUE_GENERAL)
, selectionGroup(false)
, autoLocalize(true)
, selectionBehaviorUp(Enums::SELECTION_BEHAVIOR_ESCAPE)
, selectionBehaviorDown(Enums::SELECTION_BEHAVIOR_ESCAPE)
, selectionBehaviorLeft(Enums::SELECTION_BEHAVIOR_ESCAPE)
, selectionBehaviorRight(Enums::SELECTION_BEHAVIOR_ESCAPE)
{}

void GuiBase2d::onPropertyChanged(
	const Reflection::PropertyDescriptor& descriptor)
{
	Super::onPropertyChanged(descriptor);
	// React may assign a native class default after Tag has already attached a
	// StyleRule. Re-resolve authored GUI properties so the stylesheet retains
	// its precedence over defaults. Read-only layout outputs are excluded to
	// keep absolute-position/size propagation out of the style invalidation
	// path. applyResolvedStyles owns the recursion guard for descriptor writes
	// performed by the resolver itself.
	if (!descriptor.isReadOnly() && !getTagsInternal().empty())
		applyResolvedStyles(this);
}

void GuiBase2d::setAutoLocalize(bool value)
{
	if (autoLocalize != value)
	{
		autoLocalize = value;
		raisePropertyChanged(prop_AutoLocalize);
		raisePropertyChanged(prop_Localize);
	}
}

LocalizationTable* GuiBase2d::getRootLocalizationTable() const
{
	return rootLocalizationTable.lock().get();
}

void GuiBase2d::setRootLocalizationTable(LocalizationTable* value)
{
	if (getRootLocalizationTable() == value)
		return;
	rootLocalizationTable = value ? weak_from(value) : weak_ptr<LocalizationTable>();
	raisePropertyChanged(prop_RootLocalizationTable);
}

std::string GuiBase2d::localizeText(const std::string& source) const
{
	if (!autoLocalize)
		return source;
	shared_ptr<LocalizationTable> table = rootLocalizationTable.lock();
	LocalizationService* service = ServiceProvider::find<LocalizationService>(this);
	if (!table && service)
		table = service->findCoreLocalizationTable();
	if (!table || !service)
		return source;
	try
	{
		return table->translate(service->getRobloxLocaleId(), source);
	}
	catch (const std::exception&)
	{
		return source;
	}
}

void GuiBase2d::setSelectionGroup(bool value)
{
	if (selectionGroup != value)
	{
		selectionGroup = value;
		raisePropertyChanged(prop_SelectionGroup);
	}
}

#define RBX_DEFINE_SELECTION_BEHAVIOR(Direction, member) \
void GuiBase2d::setSelectionBehavior##Direction(Enums::SelectionBehavior value) \
{ \
	if (member != value) \
	{ \
		member = value; \
		raisePropertyChanged(prop_SelectionBehavior##Direction); \
	} \
}

RBX_DEFINE_SELECTION_BEHAVIOR(Up, selectionBehaviorUp)
RBX_DEFINE_SELECTION_BEHAVIOR(Down, selectionBehaviorDown)
RBX_DEFINE_SELECTION_BEHAVIOR(Left, selectionBehaviorLeft)
RBX_DEFINE_SELECTION_BEHAVIOR(Right, selectionBehaviorRight)
#undef RBX_DEFINE_SELECTION_BEHAVIOR

Enums::SelectionBehavior GuiBase2d::getSelectionBehavior(const Vector2& direction) const
{
	if (std::abs(direction.x) > std::abs(direction.y))
		return direction.x < 0 ? selectionBehaviorLeft : selectionBehaviorRight;
	return direction.y < 0 ? selectionBehaviorUp : selectionBehaviorDown;
}

bool GuiBase2d::recalculateAbsolutePlacement(const Rect2D& viewport)
{
	bool result = false;
	result = setAbsolutePosition(viewport.x0y0());
	result = setAbsoluteSize(viewport.wh()) || result;
	return result;
}

void GuiBase2d::RecursiveRenderChildren(shared_ptr<RBX::Instance> instance, Adorn* adorn)
{
	if(RBX::GuiBase2d* guiBase = Instance::fastDynamicCast<RBX::GuiBase2d>(instance.get())){
		guiBase->recursiveRender2d(adorn);
	}
}

void GuiBase2d::recursiveRender2d(Adorn* adorn)
{
	render2d(adorn);

	visitChildren(boost::bind(&GuiBase2d::RecursiveRenderChildren, _1, adorn));		
}

static void ResizeChildren(shared_ptr<RBX::Instance> instance, const Rect2D& viewport, bool force)
{
	if(RBX::GuiBase2d* guiBase = Instance::fastDynamicCast<RBX::GuiBase2d>(instance.get())){
		guiBase->handleResize(viewport, force);
	} else if (RBX::Folder* f = Instance::fastDynamicCast<RBX::Folder>(instance.get())) {
		f->visitChildren(boost::bind(&ResizeChildren, _1, viewport, force));
	}
}
void GuiBase2d::handleResize(const Rect2D& viewport, bool force)
{
	applyResolvedStyles(this);
	//First we handle our own resize
	const bool resized = recalculateAbsolutePlacement(viewport);
	if(resized || force){
		//If our absolute size/position changed in any way, we have to resize our children too
		visitChildren(boost::bind(&ResizeChildren, _1, getChildRect2D(), force));
	}

	// Automatic descendants establish their intrinsic dimensions during the
	// top-down child pass above. Reconcile this object's content size once on
	// the way back up, then update its children against the resolved viewport.
	// Keeping this bounded inside the layout pass avoids synchronous resize
	// re-entry while React is attaching nested automatic frames.
	if (GuiObject* object = Instance::fastDynamicCast<GuiObject>(this))
		if (object->getAutomaticSize() != AUTOMATIC_SIZE_NONE &&
			recalculateAbsolutePlacement(viewport))
			visitChildren(boost::bind(&ResizeChildren, _1, getChildRect2D(), true));

}

bool GuiBase2d::setAbsolutePosition(const Vector2& value, bool fireChangedEvent)
{
	if(absolutePositionFloat != value){
		absolutePositionFloat = value;
		absolutePosition = Math::roundVector2(value);
		if (fireChangedEvent)
		{
			raisePropertyChanged(prop_AbsolutePosition);
		}
		return true;
	}
	return false;
}
bool GuiBase2d::setAbsoluteSize(const Vector2& value, bool fireChangedEvent)
{
	if(absoluteSizeFloat != value){
		absoluteSizeFloat = value;
		absoluteSize = Math::roundVector2(value);
		if (fireChangedEvent)
		{
			raisePropertyChanged(prop_AbsoluteSize);
		}
		return true;
	}
	return false;
}

//Only Gui items can descend from Gui items... no funny business
bool GuiBase2d::askAddChild(const Instance* instance) const
{
	return (Instance::fastDynamicCast<GuiBase2d>(instance) != NULL);
}

Rect2D GuiBase2d::getRect2D() const		// four integers - maybe we should store size, position as a Rect2d (g3d version) or Rect (our version)?
{
	return Rect2D::xywh(absolutePosition, absoluteSize);
}

Rect2D GuiBase2d::getRect2DFloat() const
{
	return Rect2D::xywh(absolutePositionFloat, absoluteSizeFloat);
}

} // Namespace
