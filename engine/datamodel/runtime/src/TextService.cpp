
#include "V8DataModel/TextService.h"
#include "rbx/ui/TextTruncate.h"

FASTFLAGVARIABLE(TypesettersReleaseResources, true);
FASTFLAGVARIABLE(UseDynamicTypesetterUTF8, false)

namespace RBX
{
const char* const sTextService = "TextService";
const char* const sGetTextBoundsParams = "GetTextBoundsParams";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<GetTextBoundsParams, std::string> propText("Text", category_Data, &GetTextBoundsParams::getText, &GetTextBoundsParams::setText);
static Reflection::PropDescriptor<GetTextBoundsParams, RBX::Font> propFont("Font", category_Data, &GetTextBoundsParams::getFont, &GetTextBoundsParams::setFont);
static Reflection::PropDescriptor<GetTextBoundsParams, float> propSize("Size", category_Data, &GetTextBoundsParams::getSize, &GetTextBoundsParams::setSize);
static Reflection::PropDescriptor<GetTextBoundsParams, float> propWidth("Width", category_Data, &GetTextBoundsParams::getWidth, &GetTextBoundsParams::setWidth);
static Reflection::PropDescriptor<GetTextBoundsParams, bool> propRichText("RichText", category_Data, &GetTextBoundsParams::getRichText, &GetTextBoundsParams::setRichText);
REFLECTION_END();

namespace Reflection {
template<>
EnumDesc<Enums::TextTruncate>::EnumDesc()
:EnumDescriptor("TextTruncate")
{
	addPair(Enums::TEXT_TRUNCATE_NONE, "None");
	addPair(Enums::TEXT_TRUNCATE_AT_END, "AtEnd");
	addPair(Enums::TEXT_TRUNCATE_SPLIT_WORD, "SplitWord");
}

template<>
EnumDesc<TextService::FontSize>::EnumDesc()
:EnumDescriptor("FontSize")
{
	addPair(TextService::SIZE_8 , "Size8");
	addPair(TextService::SIZE_9 , "Size9");
	addPair(TextService::SIZE_10, "Size10");
	addPair(TextService::SIZE_11, "Size11");
	addPair(TextService::SIZE_12, "Size12");
	addPair(TextService::SIZE_14, "Size14");
	addPair(TextService::SIZE_18, "Size18");
	addPair(TextService::SIZE_24, "Size24");
	addPair(TextService::SIZE_36, "Size36");
	addPair(TextService::SIZE_48, "Size48");
	addPair(TextService::SIZE_28, "Size28");
	addPair(TextService::SIZE_32, "Size32");
	addPair(TextService::SIZE_42, "Size42");
	addPair(TextService::SIZE_60, "Size60");
	addPair(TextService::SIZE_96, "Size96");
}


template<>
EnumDesc<TextService::Font>::EnumDesc()
:EnumDescriptor("Font")
{
	addPair(TextService::FONT_LEGACY, "Legacy");
	addPair(TextService::FONT_ARIAL, "Arial");
	addPair(TextService::FONT_ARIALBOLD, "ArialBold");
	addPair(TextService::FONT_SOURCESANS, "SourceSans");
	addPair(TextService::FONT_SOURCESANSBOLD, "SourceSansBold");
	addPair(TextService::FONT_SOURCESANSLIGHT, "SourceSansLight");
	addPair(TextService::FONT_SOURCESANSITALIC, "SourceSansItalic");
	addPair(TextService::FONT_BODONI, "Bodoni");
	addPair(TextService::FONT_GARAMOND, "Garamond");
	addPair(TextService::FONT_CARTOON, "Cartoon");
	addPair(TextService::FONT_CODE, "Code");
	addPair(TextService::FONT_HIGHWAY, "Highway");
	addPair(TextService::FONT_SCIFI, "SciFi");
	addPair(TextService::FONT_ARCADE, "Arcade");
	addPair(TextService::FONT_FANTASY, "Fantasy");
	addPair(TextService::FONT_ANTIQUE, "Antique");
	addPair(TextService::FONT_SOURCESANSSEMIBOLD, "SourceSansSemibold");
	addPair(TextService::FONT_GOTHAM, "Gotham");
	addLegacyName("Montserrat", TextService::FONT_GOTHAM);
	addPair(TextService::FONT_GOTHAMMEDIUM, "GothamMedium");
	addLegacyName("GothamSemibold", TextService::FONT_GOTHAMMEDIUM);
	addLegacyName("MontserratMedium", TextService::FONT_GOTHAMMEDIUM);
	addPair(TextService::FONT_GOTHAMBOLD, "GothamBold");
	addLegacyName("MontserratBold", TextService::FONT_GOTHAMBOLD);
	addPair(TextService::FONT_GOTHAMBLACK, "GothamBlack");
	addLegacyName("MontserratBlack", TextService::FONT_GOTHAMBLACK);
	addPair(TextService::FONT_AMATICSC, "AmaticSC");
	addPair(TextService::FONT_BANGERS, "Bangers");
	addPair(TextService::FONT_CREEPSTER, "Creepster");
	addPair(TextService::FONT_DENKONE, "DenkOne");
	addPair(TextService::FONT_FONDAMENTO, "Fondamento");
	addPair(TextService::FONT_FREDOKAONE, "FredokaOne");
	addPair(TextService::FONT_GRENZEGOTISCH, "GrenzeGotisch");
	addPair(TextService::FONT_INDIEFLOWER, "IndieFlower");
	addPair(TextService::FONT_JOSEFINSANS, "JosefinSans");
	addPair(TextService::FONT_JURA, "Jura");
	addPair(TextService::FONT_KALAM, "Kalam");
	addPair(TextService::FONT_LUCKIESTGUY, "LuckiestGuy");
	addPair(TextService::FONT_MERRIWEATHER, "Merriweather");
	addPair(TextService::FONT_MICHROMA, "Michroma");
	addPair(TextService::FONT_NUNITO, "Nunito");
	addPair(TextService::FONT_OSWALD, "Oswald");
	addPair(TextService::FONT_PATRICKHAND, "PatrickHand");
	addPair(TextService::FONT_PERMANENTMARKER, "PermanentMarker");
	addPair(TextService::FONT_ROBOTO, "Roboto");
	addPair(TextService::FONT_ROBOTOCONDENSED, "RobotoCondensed");
	addPair(TextService::FONT_ROBOTOMONO, "RobotoMono");
	addPair(TextService::FONT_SARPANCH, "Sarpanch");
	addPair(TextService::FONT_SPECIALELITE, "SpecialElite");
	addPair(TextService::FONT_TITILLIUMWEB, "TitilliumWeb");
	addPair(TextService::FONT_UBUNTU, "Ubuntu");
	addPair(TextService::FONT_BUILDERSANS, "BuilderSans");
	addPair(TextService::FONT_BUILDERSANS_MEDIUM, "BuilderSansMedium");
	addPair(TextService::FONT_BUILDERSANS_BOLD, "BuilderSansBold");
	addPair(TextService::FONT_BUILDERSANS_EXTRABOLD, "BuilderSansExtraBold");
	addPair(TextService::FONT_ARIMO, "Arimo");
	addPair(TextService::FONT_ARIMOBOLD, "ArimoBold");
	addPair(TextService::FONT_UNKNOWN, "Unknown");
}

template<>
TextService::Font& Variant::convert<TextService::Font>(void)
{
	return genericConvert<TextService::Font>();
}

template<>
EnumDesc<TextService::XAlignment>::EnumDesc()
:EnumDescriptor("TextXAlignment")
{
	addPair(TextService::XALIGNMENT_LEFT,   "Left");
	addPair(TextService::XALIGNMENT_CENTER, "Center");
	addPair(TextService::XALIGNMENT_RIGHT,  "Right");
}

template<>
EnumDesc<TextService::YAlignment>::EnumDesc()
:EnumDescriptor("TextYAlignment")
{
	addPair(TextService::YALIGNMENT_TOP,	"Top");
	addPair(TextService::YALIGNMENT_CENTER,"Center");
	addPair(TextService::YALIGNMENT_BOTTOM,"Bottom");
}
}//namespace Reflection

template<>
bool StringConverter<TextService::Font>::convertToValue(const std::string& text, TextService::Font& value)
{
	return Reflection::EnumDesc<TextService::Font>::singleton().convertToValue(text.c_str(),value);
}

static Reflection::BoundFuncDesc<TextService, Vector2(std::string, int, TextService::Font, Vector2)> func_getTextSize(&TextService::getTextSize, "GetTextSize", "string", "fontSize", "font", "frameSize", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<TextService, Vector2(shared_ptr<Instance>)> funcGetTextBoundsAsync(&TextService::getTextBoundsAsync, "GetTextBoundsAsync", "params", Security::None);

GetTextBoundsParams::GetTextBoundsParams()
	: size(14.0f)
	, width(0.0f)
	, richText(false)
{
	setName(sGetTextBoundsParams);
}

void GetTextBoundsParams::setText(std::string value) { if (text != value) { text = value; raisePropertyChanged(propText); } }
void GetTextBoundsParams::setFont(RBX::Font value) { if (font != value) { font = value; raisePropertyChanged(propFont); } }
void GetTextBoundsParams::setSize(float value) { value = std::max(0.0f, value); if (size != value) { size = value; raisePropertyChanged(propSize); } }
void GetTextBoundsParams::setWidth(float value) { value = std::max(0.0f, value); if (width != value) { width = value; raisePropertyChanged(propWidth); } }
void GetTextBoundsParams::setRichText(bool value) { if (richText != value) { richText = value; raisePropertyChanged(propRichText); } }

TextService::Font TextService::FromTextFont(Text::Font font)
{
	RBXASSERT(Text::isValidFont(font));
	return static_cast<Font>(font);
}
Text::Font TextService::ToTextFont(Font font)
{
	const Text::Font result = static_cast<Text::Font>(font);
	RBXASSERT(Text::isValidFont(result));
	return result;
}

Text::XAlign TextService::ToTextXAlign(XAlignment xalign)
{

	switch(xalign){
		case TextService::XALIGNMENT_LEFT:
			return Text::XALIGN_LEFT;		
		case TextService::XALIGNMENT_RIGHT:
			return Text::XALIGN_RIGHT;	
		case TextService::XALIGNMENT_CENTER:
			return Text::XALIGN_CENTER;	
		default:
			RBXASSERT(0);
			return Text::XALIGN_LEFT;
	}
}
Text::YAlign TextService::ToTextYAlign(YAlignment yalign)
{
	switch(yalign){
		case TextService::YALIGNMENT_TOP:
			return Text::YALIGN_TOP;
		case TextService::YALIGNMENT_CENTER:
			return Text::YALIGN_CENTER;	
		case TextService::YALIGNMENT_BOTTOM:
			return Text::YALIGN_BOTTOM;	
		default:
			RBXASSERT(0);
			return Text::YALIGN_TOP;
	}
}

TextService::TextService()
	:Super()
{
	this->setName(sTextService);

	clearTypesetters();
}

void TextService::clearTypesetters()
{
    if (FFlag::TypesettersReleaseResources)
    {
        if (m_typesetters.get())
        {
            for (size_t i = 0; i < FONT_LAST; ++i)
            {
				if (m_typesetters[i])
					m_typesetters[i]->releaseResources();
            }
        }
        else
        {
            m_typesetters.reset(new shared_ptr<Typesetter>[FONT_LAST]);
        }
    }
    else
    {
        m_typesetters.reset(new shared_ptr<Typesetter>[FONT_LAST]);
    }
}
void TextService::registerTypesetter(Font font, shared_ptr<RBX::Typesetter> typesetter)
{
	RBXASSERT(font < FONT_LAST);
	m_typesetters[font] = typesetter;
}

Typesetter* TextService::getTypesetter(Font font)
{
	RBXASSERT(font < FONT_LAST);
	return m_typesetters[font].get();
}

Vector2 TextService::getTextSize(std::string text, int fontSize, Font font, Vector2 frameSize)
{
	if (font >= FONT_LAST || font < FONT_LEGACY)
	{
		return Vector2::zero();
	}

	if (Typesetter* typesetter = getTypesetter(font))
	{
		return typesetter->measure(text, (float) fontSize, frameSize);
	}

	return Vector2::zero();
}

void TextService::getTextBoundsAsync(shared_ptr<Instance> instance,
	boost::function<void(Vector2)> resumeFunction,
	boost::function<void(std::string)> errorFunction)
{
	shared_ptr<GetTextBoundsParams> params = shared_dynamic_cast<GetTextBoundsParams>(instance);
	if (!params)
	{
		errorFunction("GetTextBoundsAsync requires GetTextBoundsParams");
		return;
	}

	Font selected = FONT_SOURCESANS;
	for (int value = FONT_LEGACY; value < FONT_UNKNOWN; ++value)
	{
		if (RBX::Font::fromEnum(value) == params->getFont())
		{
			selected = static_cast<Font>(value);
			break;
		}
	}
	const float availableWidth = params->getWidth() > 0.0f ? params->getWidth() : 1000000.0f;
	resumeFunction(getTextSize(params->getText(), static_cast<int>(std::ceil(params->getSize())), selected,
		Vector2(availableWidth, 1000000.0f)));
}

}
