
#include "v8datamodel/GuiObject.h"
#include "v8datamodel/ScreenGui.h"
#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/GuiService.h"
#include "GfxBase/Adorn.h"
#include "FastLog.h"

#include <algorithm>

namespace RBX {
	const char* const  sScreenGui = "ScreenGui";

	namespace Reflection {
		template<>
		EnumDesc<ScreenInsetsType>::EnumDesc()
			: EnumDescriptor("ScreenInsets")
		{
			addPair(SCREEN_INSETS_NONE, "None");
			addPair(SCREEN_INSETS_DEVICE_SAFE, "DeviceSafeInsets");
			addPair(SCREEN_INSETS_CORE_UI_SAFE, "CoreUISafeInsets");
			addPair(SCREEN_INSETS_TOPBAR_SAFE, "TopbarSafeInsets");
		}

		template<>
		ScreenInsetsType& Variant::convert<ScreenInsetsType>()
		{
			return genericConvert<ScreenInsetsType>();
		}

		template<>
		EnumDesc<SafeAreaCompatMode>::EnumDesc()
			: EnumDescriptor("SafeAreaCompatibility")
		{
			addPair(SAFE_AREA_COMPAT_NONE, "None");
			addPair(SAFE_AREA_COMPAT_FULLSCREEN_EXTENSION, "FullscreenExtension");
		}
	}

	template<>
	bool StringConverter<ScreenInsetsType>::convertToValue(
		const std::string& text, ScreenInsetsType& value)
	{
		return Reflection::EnumDesc<ScreenInsetsType>::singleton().convertToValue(
			text.c_str(), value);
	}

	static const Reflection::EnumPropDescriptor<ScreenGui, ScreenInsetsType> prop_ScreenInsets(
		"ScreenInsets", category_Data, &ScreenGui::getScreenInsets, &ScreenGui::setScreenInsets);
	static const Reflection::EnumPropDescriptor<ScreenGui, SafeAreaCompatMode> prop_SafeAreaCompatibility(
		"SafeAreaCompatibility", category_Data, &ScreenGui::getSafeAreaCompatibility,
		&ScreenGui::setSafeAreaCompatibility);
	static const Reflection::PropDescriptor<ScreenGui, bool> prop_ClipToDeviceSafeArea(
		"ClipToDeviceSafeArea", category_Data, &ScreenGui::getClipToDeviceSafeArea,
		&ScreenGui::setClipToDeviceSafeArea);
	static const Reflection::PropDescriptor<ScreenGui, int> propDisplayOrder(
		"DisplayOrder", category_Data, &ScreenGui::getDisplayOrder,
		&ScreenGui::setDisplayOrder);
	static const Reflection::PropDescriptor<ScreenGui, bool> propIgnoreGuiInset(
		"IgnoreGuiInset", category_Data, &ScreenGui::getIgnoreGuiInset,
		&ScreenGui::setIgnoreGuiInset, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
	static const Reflection::PropDescriptor<ScreenGui, bool> propOnTopOfCoreBlur(
		"OnTopOfCoreBlur", "Behavior", &ScreenGui::getOnTopOfCoreBlur,
		&ScreenGui::setOnTopOfCoreBlur, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING,
		Security::RobloxScript);

    REFLECTION_BEGIN();
	static const Reflection::PropDescriptor<ScreenGui, Vector2int16> prop_ReplicateAbsoluteSize("ReplicatingAbsoluteSize", category_Data, &ScreenGui::getAbsoluteSize, &ScreenGui::setReplicatingAbsoluteSize,  Reflection::PropertyDescriptor::REPLICATE_ONLY);
	static const Reflection::PropDescriptor<ScreenGui, Vector2int16> prop_ReplicateAbsolutePosition("ReplicatingAbsolutePosition", category_Data, &ScreenGui::getAbsolutePosition, &ScreenGui::setReplicatingAbsolutePosition, Reflection::PropertyDescriptor::REPLICATE_ONLY);
    REFLECTION_END();

	ScreenGui::ScreenGui()
		:DescribedCreatable<ScreenGui, GuiLayerCollector, sScreenGui>(sScreenGui)
		,renderable(false)
		,bufferedViewport(Rect2D::xywh(0.0f, 0.0f, 800.0f, 600.0f))
		,screenInsets(SCREEN_INSETS_CORE_UI_SAFE)
		,safeAreaCompatibility(SAFE_AREA_COMPAT_FULLSCREEN_EXTENSION)
		,clipToDeviceSafeArea(true)
		,displayOrder(0)
		,onTopOfCoreBlur(false)
	{
	}

	ScreenGui::ScreenGui(const char* name)
		:DescribedCreatable<ScreenGui, GuiLayerCollector, sScreenGui>(name)
		,renderable(false)
		,bufferedViewport(Rect2D::xywh(0.0f, 0.0f, 800.0f, 600.0f))
		,screenInsets(SCREEN_INSETS_CORE_UI_SAFE)
		,safeAreaCompatibility(SAFE_AREA_COMPAT_FULLSCREEN_EXTENSION)
		,clipToDeviceSafeArea(true)
		,displayOrder(0)
		,onTopOfCoreBlur(false)
	{
	}
    
    ScreenGui::~ScreenGui()
    {
        modalGuiObjects.clear();
        connections.clear();
    }

	void ScreenGui::setReplicatingAbsoluteSize(Vector2int16 value)
	{
		// We do need to handle a resize since it came in over the replication engine.
		handleResize(getRect2D(), false);
	}
	void ScreenGui::setReplicatingAbsolutePosition(Vector2int16 value)
	{
		// We do need to handle a resize since it came in over the replication engine.
		handleResize(getRect2D(), false);
	}
    
    void ScreenGui::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
    {
        if (oldProvider)
        {
            modalGuiObjects.clear();
            connections.clear();
        }
        
        
        Super::onServiceProvider(oldProvider, newProvider);
        
        if (newProvider)
        {
            // if we have core gui set up, get it's absolute size and use that (instead of 800x600)
            if(CoreGuiService* coreGui = ServiceProvider::create<CoreGuiService>(newProvider))
            {
                if (shared_ptr<RBX::ScreenGui> coreScreenGui = coreGui->getRobloxScreenGui())
                {
                    if (coreScreenGui.get() != this)
                    {
                        setBufferedViewport(coreScreenGui->getViewport());
                    }
                }
            }
        }
    }
    
	void ScreenGui::onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
	{
		Super::onPropertyChanged(descriptor);

		if(descriptor == Super::prop_AbsoluteSize){
			handleResize(bufferedViewport, false);
			raisePropertyChanged(prop_ReplicateAbsoluteSize);
		}
		else if(descriptor == Super::prop_AbsolutePosition){
			handleResize(bufferedViewport, false);
			raisePropertyChanged(prop_ReplicateAbsolutePosition);
		}
	}

	//The main windows parent can be any kind of non-GUI instance
	bool ScreenGui::askSetParent(const Instance* instance) const
	{
		return (Instance::fastDynamicCast<GuiBase2d>(instance) == NULL);
	}

	bool ScreenGui::isAncestorRenderableGui() const
	{
		const Instance* ancestor = getParent();
		while(ancestor != NULL)
		{
			if( ancestor->fastDynamicCast<BasePlayerGui>() )
				return true;
			ancestor = ancestor->getParent();
		}
		return false;
	}

	bool ScreenGui::canProcessMeAndDescendants() const
	{
        // Each ScreenGui is processed individually so we have to stop the hierarchy traversal
        // to eliminate duplicate processing of elements in nested ScreenGuis
        return false;
	}

	void ScreenGui::onAncestorChanged(const AncestorChanged& event)
	{
		Super::onAncestorChanged(event);

		bool newRenderable = isAncestorRenderableGui();

		if (newRenderable != renderable)
		{
			renderable = newRenderable;
			shouldRenderSetDirty();
		}

		handleResize(bufferedViewport, false);
	}

	void ScreenGui::render2d(Adorn* adorn)
	{
		render2dContext(adorn, NULL);
	}

	void ScreenGui::render2dContext(Adorn* adorn, const Instance* context)
	{
		const Rect2D full = adorn->getViewport();
		const Rect2D device = adorn->getDeviceGuiRect();
		const Rect2D core = adorn->getUserGuiRect();
		auto insets = [&full](const Rect2D& inner) {
			return UI::ScreenInsets{
				.left = std::max(0.0f, inner.x0() - full.x0()),
				.top = std::max(0.0f, inner.y0() - full.y0()),
				.right = std::max(0.0f, full.x1() - inner.x1()),
				.bottom = std::max(0.0f, full.y1() - inner.y1()),
			};
		};
		const UI::ScreenRect resolved = UI::resolveScreenRect(
			{full.x0(), full.y0(), full.x1(), full.y1()}, insets(device), insets(core),
			screenInsets, clipToDeviceSafeArea);
		setBufferedViewport(Rect2D::xyxy(resolved.left, resolved.top, resolved.right, resolved.bottom));

		Super::render2dContext(adorn, context);
	}

	void ScreenGui::setScreenInsets(ScreenInsetsType value)
	{
		if (screenInsets != value)
		{
			screenInsets = value;
			raisePropertyChanged(prop_ScreenInsets);
		}
	}

	void ScreenGui::setSafeAreaCompatibility(SafeAreaCompatMode value)
	{
		if (safeAreaCompatibility != value)
		{
			safeAreaCompatibility = value;
			raisePropertyChanged(prop_SafeAreaCompatibility);
		}
	}

	void ScreenGui::setClipToDeviceSafeArea(bool value)
	{
		if (clipToDeviceSafeArea != value)
		{
			clipToDeviceSafeArea = value;
			raisePropertyChanged(prop_ClipToDeviceSafeArea);
		}
	}

	void ScreenGui::setDisplayOrder(int value)
	{
		if (displayOrder == value)
			return;
		displayOrder = value;
		raisePropertyChanged(propDisplayOrder);
		shouldRenderSetDirty();
	}

	void ScreenGui::setIgnoreGuiInset(bool value)
	{
		if (getIgnoreGuiInset() == value)
			return;
		setScreenInsets(value ? SCREEN_INSETS_NONE : SCREEN_INSETS_CORE_UI_SAFE);
		raisePropertyChanged(propIgnoreGuiInset);
	}

	void ScreenGui::setOnTopOfCoreBlur(bool value)
	{
		if (onTopOfCoreBlur == value)
			return;
		onTopOfCoreBlur = value;
		raisePropertyChanged(propOnTopOfCoreBlur);
		shouldRenderSetDirty();
	}
    
    void ScreenGui::setBufferedViewport(Rect2D newViewport)
    {
        if (newViewport != bufferedViewport)
        {
			bufferedViewport = newViewport;
            handleResize(bufferedViewport, false);
		}
    }
    
    Vector2 ScreenGui::getAbsolutePosition() const
    {
		// A ScreenGui that ignores the core inset is already laid out against the
		// full viewport. Applying the legacy global-inset translation a second time
		// moves current CoreGui (notably Chrome/Unibar) above the physical screen.
		if (screenInsets == SCREEN_INSETS_NONE)
			return absolutePosition;
        if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
        {
            Vector4 guiInset = guiService->getGlobalGuiInset();
            return Vector2(absolutePosition.x - guiInset.x, absolutePosition.y - guiInset.y);
        }
        return absolutePosition;
    }

	GuiResponse ScreenGui::process(const shared_ptr<InputObject>& event)
	{
		// N.B.: HUD mouse message processing blowing us up on Mac; hack around it for now
		return Super::process(event);
	}
    
    GuiResponse ScreenGui::processGesture(const UserInputService::Gesture gesture, shared_ptr<const RBX::Reflection::ValueArray> touchPositions, shared_ptr<const Reflection::Tuple> args)
    {
        return Super::processGesture(gesture,touchPositions,args);
    }

	bool ScreenGui::removeModalButton(RBX::GuiButton* guiButton)
	{
		for(std::vector<GuiButton*>::iterator iter = modalGuiObjects.begin(); iter != modalGuiObjects.end(); ++iter)
		{
			if( (*iter) == guiButton )
			{
				modalGuiObjects.erase(iter);
				return true;
			}
		}
		return false;
	}

	bool ScreenGui::insertModalButton(RBX::GuiButton* guiButton)
	{
		for(std::vector<GuiButton*>::iterator iter = modalGuiObjects.begin(); iter != modalGuiObjects.end(); ++iter)
			if( (*iter) == guiButton )
				return false;

		modalGuiObjects.push_back(guiButton);
		return true;
	}

	void ScreenGui::onModalButtonChanged(const RBX::Reflection::PropertyDescriptor* desc, RBX::GuiButton* guiButton)
	{
		if(guiButton->getModal())
			insertModalButton(guiButton);
		else
			removeModalButton(guiButton);
	}

	void ScreenGui::onDescendantAdded(Instance* instance)
	{
        Super::onDescendantAdded(instance);
        
		if(RBX::GuiButton* guiButton = Instance::fastDynamicCast<RBX::GuiButton>(instance))
		{
            connections[instance] = guiButton->propertyChangedSignal.connect(boost::bind(&ScreenGui::onModalButtonChanged,this,_1,guiButton));
            
			if(guiButton->getModal())
				insertModalButton(guiButton);
		}
	}
	void ScreenGui::onDescendantRemoving(const shared_ptr<Instance>& instance)
	{
        Super::onDescendantRemoving(instance);
        
		if(RBX::GuiButton* guiButton = Instance::fastDynamicCast<RBX::GuiButton>(instance.get()))
		{
			removeModalButton(guiButton);
            connections.erase(instance.get());
		}
	}

	bool ScreenGui::hasModalDialog()
	{
		for(std::vector<GuiButton*>::iterator iter = modalGuiObjects.begin(); iter != modalGuiObjects.end(); ++iter)
		{
			if( (*iter)->isCurrentlyVisible() )
				return true;
		}
		return false;
	}

	const char* const  sGuiMain = "GuiMain";
	GuiMain::GuiMain()
		:DescribedCreatable<GuiMain, ScreenGui, sGuiMain>(sGuiMain)
	{}

}
