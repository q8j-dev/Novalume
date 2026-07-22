#pragma once
#include <string>
#include "util/BrickColor.h"
#include "util/TextureId.h"
#include "Gui/ProfanityFilter.h"
#include "Gui/GuiDraw.h"
#include "util/ContentFilter.h"
#include "v8datamodel/GuiObject.h"
#include "v8datamodel/UIComponent.h"

namespace RBX
{
	class GuiImageMixin
	{
	public:
		GuiImageMixin()
			: imageTransparency(0)
			, imageColor(Color3::white())
			, imageScale(GuiObject::SCALE_STRETCH)
			, resampleMode(RESAMPLER_MODE_DEFAULT)
			, sliceCenter(Rect2D())
			, sliceScale(1.0f)
		{
		}

		TextureId getImage() const { return image;} 
		
		Vector2 getImageRectOffset() const { return imageRectOffset; }
		
		Vector2 getImageRectSize() const { return imageRectSize; }

		float getImageTransparency() const { return imageTransparency; }

		Color3 getImageColor3() const { return imageColor; }

		Rect2D getSliceCenter() const { return sliceCenter; }
		float getSliceScale() const { return sliceScale; }

		GuiObject::ImageScale getImageScale() const { return imageScale; }
		ResamplerMode getResampleMode() const { return resampleMode; }

	protected:
		TextureId image;
        float imageTransparency;
		Color3 imageColor;
		Vector2 imageRectOffset;
		Vector2 imageRectSize;
		GuiDrawImage guiImageDraw;
		Rect2D sliceCenter;
		float sliceScale;
		GuiObject::ImageScale imageScale;
		ResamplerMode resampleMode;
	};

	template<class Class>
	void renderImageFitted(Class* object, Adorn* adorn, GuiDrawImage& draw, bool crop)
	{
		Vector2 textureSize;
		if (!draw.setImage(adorn, object->getImage(), GuiDrawImage::NORMAL, &textureSize, object, ".Image") || textureSize.isZero())
			return;

		Vector2 texul, texbr;
		draw.computeUV(texul, texbr, object->getImageRectOffset(), object->getImageRectSize(), textureSize);
		const Vector2 sourceSize = object->getImageRectSize().isZero() ? textureSize : object->getImageRectSize();
		Rect2D target = object->getRect2D();
		if (sourceSize.x <= 0 || sourceSize.y <= 0 || target.width() <= 0 || target.height() <= 0)
			return;

		const float sourceAspect = sourceSize.x / sourceSize.y;
		const float targetAspect = target.width() / target.height();
		if (crop)
		{
			if (sourceAspect > targetAspect)
			{
				const float keep = targetAspect / sourceAspect;
				const float inset = (texbr.x - texul.x) * (1.0f - keep) * 0.5f;
				texul.x += inset;
				texbr.x -= inset;
			}
			else
			{
				const float keep = sourceAspect / targetAspect;
				const float inset = (texbr.y - texul.y) * (1.0f - keep) * 0.5f;
				texul.y += inset;
				texbr.y -= inset;
			}
		}
		else if (sourceAspect > targetAspect)
		{
			const float height = target.width() / sourceAspect;
			target = Rect2D::xywh(Vector2(target.x0(), target.y0() + (target.height() - height) * 0.5f), Vector2(target.width(), height));
		}
		else
		{
			const float width = target.height() * sourceAspect;
			target = Rect2D::xywh(Vector2(target.x0() + (target.width() - width) * 0.5f, target.y0()), Vector2(width, target.height()));
		}

		const Color4 color = object->applyCanvasGroup(Color4(object->getImageColor3(), 1.0f - object->getImageTransparency()));
		GuiObject* clippingObject = object->firstAncestorClipping();
		if (clippingObject == NULL || !object->getAbsoluteRotation().empty())
			draw.render2d(adorn, true, target, texul, texbr, color, object->getAbsoluteRotation(), Gui::NOTHING, false);
		else
			draw.render2d(adorn, true, target, texul, texbr, color, clippingObject->getClippedRect(), Gui::NOTHING, false);
	}

	template<class Class>
	void renderImageTiled(Class* object, Adorn* adorn, GuiDrawImage& draw)
	{
		Vector2 textureSize;
		if (!draw.setImage(adorn, object->getImage(), GuiDrawImage::NORMAL, &textureSize, object, ".Image"))
			return;

		Vector2 texul, texbr;
		draw.computeUV(texul, texbr, object->getImageRectOffset(), object->getImageRectSize(), textureSize);
		const Vector2 tileSize = object->getImageRectSize().isZero() ? textureSize : object->getImageRectSize();
		const Rect2D target = object->getRect2D();
		if (tileSize.x <= 0 || tileSize.y <= 0)
			return;

		const Color4 color = object->applyCanvasGroup(Color4(object->getImageColor3(), 1.0f - object->getImageTransparency()));
		GuiObject* clippingObject = object->firstAncestorClipping();
		for (float y = target.y0(); y < target.y1(); y += tileSize.y)
			for (float x = target.x0(); x < target.x1(); x += tileSize.x)
			{
				const float width = std::min(tileSize.x, target.x1() - x);
				const float height = std::min(tileSize.y, target.y1() - y);
				const Rect2D tile = Rect2D::xywh(x, y, width, height);
				const Vector2 tilebr(texul.x + (texbr.x - texul.x) * width / tileSize.x,
					texul.y + (texbr.y - texul.y) * height / tileSize.y);
				if (clippingObject == NULL || !object->getAbsoluteRotation().empty())
					draw.render2d(adorn, true, tile, texul, tilebr, color, object->getAbsoluteRotation(), Gui::NOTHING, false);
				else
					draw.render2d(adorn, true, tile, texul, tilebr, color, clippingObject->getClippedRect(), Gui::NOTHING, false);
			}
	}

	#define DECLARE_GUI_IMAGE_MIXIN(Class)		    \
		void setImage(TextureId value);		        \
		void setImageRectOffset(Vector2 value);		\
		void setImageRectSize(Vector2 value);       \
		void setImageTransparency(float value);		\
		void setImageColor3(Color3 value);			\
		void setSliceCenter(Rect2D value);			\
		void setSliceScale(float value);			\
		void setImageScale(ImageScale value);		\
		void setResampleMode(ResamplerMode value);	\
		void renderStretched(Adorn* adorn);			\
		void renderSliced(Adorn* adorn);			\
		void renderImage(Adorn* adorn);

	
	#define IMPLEMENT_GUI_IMAGE_MIXIN(Class) \
		static const Reflection::PropDescriptor<Class, TextureId> prop_Image("Image", category_Image, &Class::getImage, &Class::setImage); \
		static const Reflection::PropDescriptor<Class, Vector2> prop_ImageRectOffset("ImageRectOffset", category_Image, &Class::getImageRectOffset, &Class::setImageRectOffset); \
		static const Reflection::PropDescriptor<Class, Vector2> prop_ImageRectSize("ImageRectSize", category_Image, &Class::getImageRectSize, &Class::setImageRectSize); \
		static const Reflection::PropDescriptor<Class, float> prop_ImageTransparency("ImageTransparency", category_Image, &Class::getImageTransparency, &Class::setImageTransparency); \
		static const Reflection::PropDescriptor<Class, Color3> prop_ImageColor3("ImageColor3", category_Image, &Class::getImageColor3, &Class::setImageColor3);		\
		static const Reflection::PropDescriptor<Class, Rect2D> prop_SliceCenter("SliceCenter", category_Image, &Class::getSliceCenter, &Class::setSliceCenter); \
		static const Reflection::PropDescriptor<Class, float> prop_SliceScale("SliceScale", category_Image, &Class::getSliceScale, &Class::setSliceScale); \
		static const Reflection::EnumPropDescriptor<Class, GuiObject::ImageScale> prop_ImageScale("ScaleType", category_Image, &Class::getImageScale, &Class::setImageScale); \
		static const Reflection::EnumPropDescriptor<Class, ResamplerMode> prop_ResampleMode("ResampleMode", category_Image, &Class::getResampleMode, &Class::setResampleMode); \
		void Class::setImage(TextureId value)				\
		{													\
			if(image != value){								\
				image = value;								\
				raisePropertyChanged(prop_Image);			\
			}												\
		}													\
		void Class::setImageRectOffset(Vector2 value)		\
		{													\
			if(imageRectOffset != value){					\
				Rect2D offsetSliceCenter = Rect2D::xyxy(sliceCenter.x0y0() + value, sliceCenter.x1y1() + value);									\
				Rect2D imageRect = Rect2D::xywh(value, imageRectSize);																				\
				if (sliceCenter != Rect2D::xywh(0,0,0,0) && !imageRect.contains(offsetSliceCenter))													\
				{																																	\
					RBX::StandardOut::singleton()->printf(MESSAGE_WARNING,"SliceCenter ((%f,%f), (%f,%f)) is outside the bounds of imageOffset ((%f,%f), (%f,%f)).", offsetSliceCenter.x0(),offsetSliceCenter.y0(),offsetSliceCenter.x1(), offsetSliceCenter.y1(), imageRect.x0(),imageRect.y0(),imageRect.x1(), imageRect.y1()); \
				}																																	\
				imageRectOffset = value;					\
				raisePropertyChanged(prop_ImageRectOffset);	\
			}												\
		}													\
		void Class::setImageRectSize(Vector2 value)			\
		{													\
			if(imageRectSize != value){						\
				Rect2D offsetSliceCenter = Rect2D::xyxy(sliceCenter.x0y0() + imageRectOffset, sliceCenter.x1y1() + imageRectOffset);				\
				Rect2D imageRect = Rect2D::xywh(imageRectOffset, value);																				\
				if (sliceCenter != Rect2D::xywh(0,0,0,0) && !imageRect.contains(offsetSliceCenter))													\
				{																																	\
					RBX::StandardOut::singleton()->printf(MESSAGE_WARNING,"SliceCenter ((%f,%f), (%f,%f)) is outside the bounds of imageOffset ((%f,%f), (%f,%f))", offsetSliceCenter.x0(),offsetSliceCenter.y0(),offsetSliceCenter.x1(), offsetSliceCenter.y1(), imageRect.x0(),imageRect.y0(),imageRect.x1(), imageRect.y1()); \
					return;																															\
				}																																	\
				imageRectSize = value;						\
				raisePropertyChanged(prop_ImageRectSize);	\
			}												\
		}                                                   \
		void Class::setImageTransparency(float value)		\
		{													\
		    value = G3D::clamp(value, 0, 1);                \
		                                                    \
			if(imageTransparency != value){					\
				imageTransparency = value;					\
				raisePropertyChanged(prop_ImageTransparency); 	\
			}												\
		}													\
		void Class::setImageColor3(Color3 value)			\
		{													\
			if(imageColor != value){						\
				imageColor = value;							\
				raisePropertyChanged(prop_ImageColor3);		\
			}												\
		}													\
		void Class::setSliceCenter(Rect2D value)			\
		{													\
			if(sliceCenter != value)						\
			{												\
				Rect2D offsetSliceCenter = Rect2D::xyxy(value.x0y0() + imageRectOffset, value.x1y1() + imageRectOffset);							\
				Rect2D imageRect = Rect2D::xywh(imageRectOffset, imageRectSize);																	\
				if (imageRect != Rect2D::xywh(0,0,0,0) && !imageRect.contains(offsetSliceCenter))													\
				{																																	\
					RBX::StandardOut::singleton()->printf(MESSAGE_WARNING,"SliceCenter ((%f,%f), (%f,%f)) is outside the bounds of imageOffset ((%f,%f), (%f,%f))", offsetSliceCenter.x0(),offsetSliceCenter.y0(),offsetSliceCenter.x1(), offsetSliceCenter.y1(), imageRect.x0(),imageRect.y0(),imageRect.x1(), imageRect.y1()); \
				}																																	\
				sliceCenter = value;						\
				raisePropertyChanged(prop_SliceCenter);		\
			}												\
		}													\
		void Class::setSliceScale(float value)			\
		{													\
			value = std::max(0.0f, value);				\
			if (sliceScale != value) {					\
				sliceScale = value;						\
				raisePropertyChanged(prop_SliceScale);		\
			}											\
		}													\
		void Class::setImageScale(ImageScale value)			\
		{													\
			if(imageScale != value){						\
				imageScale = value;							\
				raisePropertyChanged(prop_ImageScale);		\
			}												\
		}													\
															\
		void Class::setResampleMode(ResamplerMode value)	\
		{													\
			if (resampleMode != value) {					\
				resampleMode = value;						\
				raisePropertyChanged(prop_ResampleMode);		\
			}												\
		}													\
															\
		void Class::renderStretched(Adorn* adorn)			\
		{													\
			Vector2 imageSize;								\
			if (guiImageDraw.setImage(adorn, image, GuiDrawImage::NORMAL, &imageSize, this, ".Image"))	\
			{																			\
				Vector2 texul, texbr;													\
				guiImageDraw.computeUV(texul, texbr, imageRectOffset, imageRectSize, imageSize);	\
																									\
				Color4 color = applyCanvasGroup(Color4(getImageColor3(), 1 - imageTransparency));	\
																									\
				GuiObject* clippingObject = firstAncestorClipping();								\
				if (const UIGradient* gradient = findUIGradient(this))							\
				{																		\
					adorn->setTexture(0, guiImageDraw.getImageTexture());						\
					Rect2D clippedRect;													\
					const Rect2D* clip = NULL;											\
					if (clippingObject != NULL && absoluteRotation.empty())					\
					{																	\
						clippedRect = clippingObject->getClippedRect();						\
						clip = &clippedRect;											\
					}																	\
					render2dGradientTexture(adorn, getRect2D(), texul, texbr, color, clip);\
					adorn->setTexture(0, TextureProxyBaseRef());							\
				}																		\
				else if( clippingObject == NULL || !absoluteRotation.empty())					\
					guiImageDraw.render2d(adorn, true, getRect2D(), texul, texbr, color, absoluteRotation, Gui::NOTHING, false);	\
				else																												\
					guiImageDraw.render2d(adorn, true, getRect2D(), texul, texbr, color, clippingObject->getClippedRect(), Gui::NOTHING, false);	\
			}																																		\
		}																																			\
																																					\
		void Class::renderSliced(Adorn* adorn)																										\
		{																																			\
			TextureId selectImage = getImage();																										\
																																					\
			if(guiImageDraw.setImage(adorn, selectImage, GuiDrawImage::NORMAL, NULL, this, ".Image"))																			\
			{																																		\
                Color4 rectColor = applyCanvasGroup(Color4(getImageColor3(), 1.0 - getImageTransparency()));                                       \
				Rect2D imageRectTextureOffset = Rect2D::xywh(imageRectOffset, imageRectSize);														\
				if (imageRectTextureOffset.width() > 0.0f && imageRectTextureOffset.height() > 0.0f)												\
				{																																											\
					render2dScale9Impl2(adorn, selectImage, guiImageDraw, sliceCenter, firstAncestorClipping(), rectColor, NULL, &imageRectTextureOffset, sliceScale); \
				}																																											\
				else																																										\
				{																																											\
					render2dScale9Impl2(adorn, selectImage, guiImageDraw, sliceCenter, firstAncestorClipping(), rectColor, NULL, NULL, sliceScale); \
				}																																											\
			}																																					\
		}																																						\
																																								\
		void Class::renderImage(Adorn* adorn)																													\
		{																																						\
			switch (imageScale)																																	\
			{																																					\
				case GuiObject::SCALE_STRETCH:																													\
					{																																			\
						renderStretched(adorn);																													\
						break;																																	\
					}																																			\
				case GuiObject::SCALE_SLICED:																													\
					{																																			\
						renderSliced(adorn);																													\
						break;																																	\
					}																																			\
				case GuiObject::SCALE_TILE: renderImageTiled(this, adorn, guiImageDraw); break; \
				case GuiObject::SCALE_FIT: renderImageFitted(this, adorn, guiImageDraw, false); break; \
				case GuiObject::SCALE_CROP: renderImageFitted(this, adorn, guiImageDraw, true); break; \
				default:																																		\
					break;																																		\
			}																																					\
																																								\
			renderStudioSelectionBox(adorn);																													\
		}


}	// namespace RBX												
	
