#pragma once

#include "GuiBase.h"
#include "rbx/ui/SelectionBehavior.h"

namespace RBX {
	class LocalizationTable;

	extern const char* const sGuiBase2d;

	//A set of base functionality used by all Lua bound Gui objects
	class GuiBase2d	: public DescribedNonCreatable<GuiBase2d, GuiBase, sGuiBase2d>
	{
	public:
		GuiBase2d(const char* name);

		virtual bool isGuiLeaf() const { return false; }

		virtual Vector2 getAbsolutePosition() const { return absolutePosition; }
		bool setAbsolutePosition(const Vector2& value, bool fireChangedEvent = true);

		Vector2 getAbsoluteSize() const { return absoluteSize; }
		bool setAbsoluteSize(const Vector2& value, bool fireChangedEvent = true);

		Rect2D getRect2D() const;		// four integers - maybe we should store size, position as a Rect2d (g3d version) or Rect (our version)?
		Rect2D getRect2DFloat() const;
		virtual Rect2D getChildRect2D() const { return getRect2DFloat(); }
		virtual Rect2D getCanvasRect() const { return getChildRect2D(); }


		virtual void handleResize(const Rect2D& viewport, bool force);

		virtual bool recalculateAbsolutePlacement(const Rect2D& viewport);

		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ bool askAddChild(const Instance* instance) const;

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiBase
		/*implement*/ virtual bool canProcessMeAndDescendants() const { return true; }
		/*implement*/ virtual int getZIndex() const { return zIndex; }
		/*implement*/ virtual GuiQueue getGuiQueue() const { return guiQueue; }

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ bool shouldRender2d() const { return false; } // explicit render traversal by ScreenGui or other.

		/*override*/ bool isVisible(const Rect2D& rect) const
		{
			const Rect2D objectRect = getRect2DFloat();
			return objectRect.width() > 0.0f && objectRect.height() > 0.0f &&
				rect.intersects(objectRect);
		}

		static Reflection::PropDescriptor<GuiBase2d, Vector2> prop_AbsoluteSize;
		static Reflection::PropDescriptor<GuiBase2d, Vector2> prop_AbsolutePosition;
		static Reflection::PropDescriptor<GuiBase2d, bool> prop_SelectionGroup;
		static Reflection::EnumPropDescriptor<GuiBase2d, Enums::SelectionBehavior> prop_SelectionBehaviorUp;
		static Reflection::EnumPropDescriptor<GuiBase2d, Enums::SelectionBehavior> prop_SelectionBehaviorDown;
		static Reflection::EnumPropDescriptor<GuiBase2d, Enums::SelectionBehavior> prop_SelectionBehaviorLeft;
		static Reflection::EnumPropDescriptor<GuiBase2d, Enums::SelectionBehavior> prop_SelectionBehaviorRight;

		bool getSelectionGroup() const { return selectionGroup; }
		void setSelectionGroup(bool value);
		Enums::SelectionBehavior getSelectionBehaviorUp() const { return selectionBehaviorUp; }
		Enums::SelectionBehavior getSelectionBehaviorDown() const { return selectionBehaviorDown; }
		Enums::SelectionBehavior getSelectionBehaviorLeft() const { return selectionBehaviorLeft; }
		Enums::SelectionBehavior getSelectionBehaviorRight() const { return selectionBehaviorRight; }
		void setSelectionBehaviorUp(Enums::SelectionBehavior value);
		void setSelectionBehaviorDown(Enums::SelectionBehavior value);
		void setSelectionBehaviorLeft(Enums::SelectionBehavior value);
		void setSelectionBehaviorRight(Enums::SelectionBehavior value);
		Enums::SelectionBehavior getSelectionBehavior(const Vector2& direction) const;
		bool getAutoLocalize() const { return autoLocalize; }
		void setAutoLocalize(bool value);
		LocalizationTable* getRootLocalizationTable() const;
		void setRootLocalizationTable(LocalizationTable* value);
		std::string localizeText(const std::string& source) const;

		rbx::signal<void(bool, shared_ptr<Instance>, shared_ptr<Instance>)> selectionChangedSignal;


	protected:
		void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor) override;
		void recursiveRender2d(Adorn* adorn);
		void setGuiQueue(GuiQueue queue) { guiQueue = queue; }

		Vector2 absolutePosition;
		Vector2 absolutePositionFloat;
		Vector2 absoluteSize;
		Vector2 absoluteSizeFloat;
		int zIndex;
		GuiQueue guiQueue;
		bool selectionGroup;
		bool autoLocalize;
		weak_ptr<LocalizationTable> rootLocalizationTable;
		Enums::SelectionBehavior selectionBehaviorUp;
		Enums::SelectionBehavior selectionBehaviorDown;
		Enums::SelectionBehavior selectionBehaviorLeft;
		Enums::SelectionBehavior selectionBehaviorRight;

	private:
		typedef DescribedNonCreatable<GuiBase2d, GuiBase, sGuiBase2d> Super;

		static void RecursiveRenderChildren(shared_ptr<RBX::Instance> instance, Adorn* adorn);
	};
}
