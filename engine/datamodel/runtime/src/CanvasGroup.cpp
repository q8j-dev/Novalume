#include "V8DataModel/CanvasGroup.h"

namespace RBX
{
const char* const sCanvasGroup = "CanvasGroup";

static const Reflection::PropDescriptor<CanvasGroup, float> prop_GroupTransparency(
    "GroupTransparency", category_Data, &CanvasGroup::getGroupTransparency,
    &CanvasGroup::setGroupTransparency);
static const Reflection::PropDescriptor<CanvasGroup, Color3> prop_GroupColor3(
    "GroupColor3", category_Data, &CanvasGroup::getGroupColor3,
    &CanvasGroup::setGroupColor3);

CanvasGroup::CanvasGroup()
    : DescribedCreatable<CanvasGroup, GuiObject, sCanvasGroup>("CanvasGroup", false)
    , groupTransparency(0.0f)
    , groupColor(Color3::white())
{
}

void CanvasGroup::setGroupTransparency(float value)
{
    value = G3D::clamp(value, 0.0f, 1.0f);
    if (groupTransparency != value)
    {
        groupTransparency = value;
        raisePropertyChanged(prop_GroupTransparency);
    }
}

void CanvasGroup::setGroupColor3(Color3 value)
{
    if (groupColor != value)
    {
        groupColor = value;
        raisePropertyChanged(prop_GroupColor3);
    }
}

void CanvasGroup::render2d(Adorn* adorn)
{
    GuiObject::render2d(adorn);
}
}
