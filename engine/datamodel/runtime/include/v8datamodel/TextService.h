#pragma once

#include "v8tree/Service.h"
#include "v8tree/Instance.h"
#include "GfxBase/Typesetter.h"
#include "util/Font.h"

namespace RBX {
	class PartInstance;

	extern const char *const sTextService ;
	extern const char *const sGetTextBoundsParams;

	class GetTextBoundsParams
		: public DescribedCreatable<GetTextBoundsParams, Instance, sGetTextBoundsParams,
			Reflection::ClassDescriptor::PERSISTENT>
	{
	private:
		std::string text;
		RBX::Font font;
		float size;
		float width;
		bool richText;
	public:
		GetTextBoundsParams();
		const std::string& getText() const { return text; }
		void setText(std::string value);
		const RBX::Font& getFont() const { return font; }
		void setFont(RBX::Font value);
		float getSize() const { return size; }
		void setSize(float value);
		float getWidth() const { return width; }
		void setWidth(float value);
		bool getRichText() const { return richText; }
		void setRichText(bool value);
	};

	class TextService 
		: public DescribedNonCreatable<TextService, Instance, sTextService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<TextService, Instance, sTextService, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
		boost::scoped_array<shared_ptr<Typesetter> > m_typesetters;
	public:
		enum XAlignment
		{
			XALIGNMENT_LEFT   = 0,
			XALIGNMENT_RIGHT  = 1,
			XALIGNMENT_CENTER = 2
		};

		enum YAlignment
		{
			YALIGNMENT_TOP    = 0,
			YALIGNMENT_CENTER = 1,
			YALIGNMENT_BOTTOM = 2
		};

		enum FontSize
		{
			SIZE_8 = 0,
			SIZE_9 = 1,
			SIZE_10 = 2,
			SIZE_11 = 3,
			SIZE_12 = 4,
			SIZE_14 = 5,
			SIZE_18 = 6,
			SIZE_24 = 7,
			SIZE_36 = 8, 
			SIZE_48 = 9,

			SIZE_28 = 10,
			SIZE_32 = 11,
			SIZE_42 = 12,
			SIZE_60 = 13,
			SIZE_96 = 14,

			// ADD NEW FONT SIZES ABOVE HERE, AND UPDATE BELOW


			SIZE_SMALLEST = SIZE_8,
			SIZE_LARGEST = SIZE_96
		};

		enum Font
		{
			FONT_LEGACY = 0,
			FONT_ARIAL = 1,
			FONT_ARIALBOLD = 2,
			FONT_SOURCESANS = 3,
			FONT_SOURCESANSBOLD = 4,
			FONT_SOURCESANSLIGHT = 5,
			FONT_SOURCESANSITALIC = 6,
			FONT_BODONI = 7, FONT_GARAMOND = 8, FONT_CARTOON = 9,
			FONT_CODE = 10, FONT_HIGHWAY = 11, FONT_SCIFI = 12,
			FONT_ARCADE = 13, FONT_FANTASY = 14, FONT_ANTIQUE = 15,
			FONT_SOURCESANSSEMIBOLD = 16, FONT_GOTHAM = 17,
			FONT_GOTHAMMEDIUM = 18, FONT_GOTHAMBOLD = 19,
			FONT_GOTHAMBLACK = 20, FONT_AMATICSC = 21,
			FONT_BANGERS = 22, FONT_CREEPSTER = 23, FONT_DENKONE = 24,
			FONT_FONDAMENTO = 25, FONT_FREDOKAONE = 26,
			FONT_GRENZEGOTISCH = 27, FONT_INDIEFLOWER = 28,
			FONT_JOSEFINSANS = 29, FONT_JURA = 30, FONT_KALAM = 31,
			FONT_LUCKIESTGUY = 32, FONT_MERRIWEATHER = 33,
			FONT_MICHROMA = 34, FONT_NUNITO = 35, FONT_OSWALD = 36,
			FONT_PATRICKHAND = 37, FONT_PERMANENTMARKER = 38,
			FONT_ROBOTO = 39, FONT_ROBOTOCONDENSED = 40,
			FONT_ROBOTOMONO = 41, FONT_SARPANCH = 42,
			FONT_SPECIALELITE = 43, FONT_TITILLIUMWEB = 44,
			FONT_UBUNTU = 45, FONT_BUILDERSANS = 46,
			FONT_BUILDERSANS_MEDIUM = 47, FONT_BUILDERSANS_BOLD = 48,
			FONT_BUILDERSANS_EXTRABOLD = 49, FONT_ARIMO = 50,
			FONT_ARIMOBOLD = 51,
			FONT_UNKNOWN = 100,
			FONT_BUILDER_ICONS_REGULAR = 101,
			FONT_BUILDER_ICONS_FILLED = 102,

			FONT_LAST = 103
		};

		static Font FromTextFont(Text::Font font);


		static Text::Font ToTextFont(Font font);
		static Text::XAlign ToTextXAlign(XAlignment xalign);
		static Text::YAlign ToTextYAlign(YAlignment xalign);
		TextService();


		void registerTypesetter(Font font, shared_ptr<RBX::Typesetter> typesetter);
		void clearTypesetters();

		Vector2 getTextSize(std::string text, int fontSize, Font font, Vector2 frameSize);
		void getTextBoundsAsync(shared_ptr<Instance> params,
			boost::function<void(Vector2)> resumeFunction,
			boost::function<void(std::string)> errorFunction);

		Typesetter* getTypesetter(Font font);
	};
}
