#pragma once

#include "GfxBase/Typesetter.h"

#include <boost/scoped_array.hpp>

#include "TextureRef.h"

namespace RBX
{
	namespace Graphics
	{
		class VisualEngine;
		class GlyphProvider;
        class TextureAtlas;

		class TypesetterDynamic: public Typesetter
		{
		public:
			TypesetterDynamic(TextureAtlas* textureAtlas, RBX::Graphics::TextureManager* textureManager, const std::string& fontPath, float legacyHeightScale, unsigned fontId, bool retina,
				const std::vector<std::string>& fallbackPaths = std::vector<std::string>());
            
            ~TypesetterDynamic();

			virtual Vector2 draw(
				Adorn*				adorn,
				const std::string&  s,
				const Vector2&      position,
				float               size,
				bool                autoScale,
				const Color4&       color,
				const Color4&       outline,
				RBX::Text::XAlign   xalign,
				RBX::Text::YAlign   yalign,
				const Vector2&		availableSpace,
				const Rect2D&		clippingRect,
				const Rotation2D&   rotation) const;

			virtual int getCursorPositionInText(
				const std::string&      s,
				const RBX::Vector2&     pos2D,
				float                   size,
				RBX::Text::XAlign       xalign,
				RBX::Text::YAlign       yalign,
				const RBX::Vector2&		availableSpace,
				const Rotation2D&       rotation,
				RBX::Vector2			cursorPos) const;

			virtual std::vector<Rect2D> getSelectionRects(
				const std::string& s,
				const RBX::Vector2& pos2D,
				float size,
				RBX::Text::XAlign xalign,
				RBX::Text::YAlign yalign,
				const RBX::Vector2& availableSpace,
				unsigned selectionStart,
				unsigned selectionEnd,
				bool autoScale = false) const;

			virtual Vector2 measure(
				const std::string&  s,
				float size,
				const Vector2& availableSpace,
				bool* textFits
				) const;

			virtual const std::shared_ptr<Texture>& getTexture() const;
            virtual void loadResources(RBX::Graphics::TextureManager* textureManager, RBX::Graphics::TextureAtlas* glyphAtlas);
            virtual void releaseResources();
			unsigned getColorLayerCount(unsigned character) const;

			struct Glyph
			{
				short xoffset;
				short yoffset;
				short xadvance;
				short width;
				short height;
			};

		private:

            GlyphProvider* glyphProvider;
            std::shared_ptr<Texture> fallbackWhiteTexture;

			struct GlyphLine
			{
				std::vector<unsigned> data;
				size_t length;

				int yoffset;
				int width;
				unsigned sourceStart;
				unsigned sourceEnd;
			};

			float legacyHeightScale;
			float retinaScale;
			bool namedGlyphs;

            void drawCursor(Adorn* adorn, bool useClipping, const Rect2D& clippingRect, const Rotation2D& rotation, const Rect2D& glyphRect, const Color4& color) const;
			Vector2int16 layoutRatio(const std::vector<unsigned>& stringUnicode, std::vector<GlyphLine>* lines, int size, const Vector2& availableSpace) const;
			Vector2int16 layout(const std::vector<unsigned>& stringUnicode, std::vector<GlyphLine>* lines, int size, const Vector2int16& availableSpace, bool useAvailableSpace, bool onlyProperFit,  bool* textFits) const;

			Vector2 measureLining(std::vector<GlyphLine>& lines, unsigned size) const;

            void decodeUTF8(const std::string& string, std::vector<unsigned>* out) const;
			void reorderLines(const std::vector<unsigned>& stringUnicode,
				std::vector<GlyphLine>* lines) const;
			void reshapeLines(const std::vector<unsigned>& stringUnicode, unsigned size,
				std::vector<GlyphLine>* lines) const;

			Vector2 drawScaledImpl(
				Adorn*				adorn,
				const std::vector<unsigned>& stringUnicode,
				const Vector2&      position,
				const Color4&       color,
				const Color4&       outline,
				RBX::Text::XAlign   xalign,
				RBX::Text::YAlign   yalign,
				const Vector2&		availableSpace,
				const Rect2D&		clippingRect,
				const Rotation2D&   rotation) const;

			Vector2 drawImpl(
				Adorn*				adorn,
				const std::vector<unsigned>& stringUnicode,
				const Vector2&      position,
				float               size,
				const Color4&       color,
				const Color4&       outline,
				RBX::Text::XAlign   xalign,
				RBX::Text::YAlign   yalign,
				const Vector2&		availableSpace,
				const Rect2D&		clippingRect,
				const Rotation2D&   rotation) const;

            void render(Adorn* adorn, 
                const Color4& color, const Color4& outline, float alpha, RBX::Text::XAlign xalign, std::vector<GlyphLine>& lines, float scale, 
                unsigned measureSize, unsigned renderSize, unsigned ascender,
                float startx, float starty, float height, 
                const Rect2D& clippingRect, const Rotation2D& rotation) const;
		};
	}
}
