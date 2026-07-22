#pragma once

#include "v8datamodel/GuiObject.h"

namespace RBX
{
    extern const char* const sCanvasGroup;

    class CanvasGroup : public DescribedCreatable<CanvasGroup, GuiObject, sCanvasGroup>
    {
    public:
        CanvasGroup();

        float getGroupTransparency() const { return groupTransparency; }
        void setGroupTransparency(float value);
        Color3 getGroupColor3() const { return groupColor; }
        void setGroupColor3(Color3 value);

        /*override*/ void render2d(Adorn* adorn);

    private:
        float groupTransparency;
        Color3 groupColor;
    };
}
