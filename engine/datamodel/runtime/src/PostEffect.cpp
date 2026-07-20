#include "V8DataModel/PostEffect.h"

#include <cmath>

using namespace RBX;

namespace {
float finiteClamp(float value, float low, float high)
{
    return std::isfinite(value) ? G3D::clamp(value, low, high) : low;
}
Color3 colorClamp(Color3 value)
{
    return Color3(finiteClamp(value.r, 0, 1), finiteClamp(value.g, 0, 1),
        finiteClamp(value.b, 0, 1));
}
}

const char* const RBX::sPostEffect = "PostEffect";
const char* const RBX::sBloomEffect = "BloomEffect";
const char* const RBX::sSunRaysEffect = "SunRaysEffect";
const char* const RBX::sDepthOfFieldEffect = "DepthOfFieldEffect";
const char* const RBX::sColorCorrectionEffect = "ColorCorrectionEffect";
const char* const RBX::sAtmosphere = "Atmosphere";

#define FLOAT_PROPERTY(Class, Name) \
    const Reflection::PropDescriptor<Class, float> Class::prop_##Name( \
        #Name, category_Appearance, &Class::get##Name, &Class::set##Name)

const Reflection::PropDescriptor<PostEffect, bool> PostEffect::prop_Enabled(
    "Enabled", category_Appearance, &PostEffect::getEnabled, &PostEffect::setEnabled);
FLOAT_PROPERTY(BloomEffect, Intensity);
FLOAT_PROPERTY(BloomEffect, Size);
FLOAT_PROPERTY(BloomEffect, Threshold);
FLOAT_PROPERTY(SunRaysEffect, Intensity);
FLOAT_PROPERTY(SunRaysEffect, Spread);
FLOAT_PROPERTY(DepthOfFieldEffect, FarIntensity);
FLOAT_PROPERTY(DepthOfFieldEffect, FocusDistance);
FLOAT_PROPERTY(DepthOfFieldEffect, InFocusRadius);
FLOAT_PROPERTY(DepthOfFieldEffect, NearIntensity);
FLOAT_PROPERTY(ColorCorrectionEffect, Brightness);
FLOAT_PROPERTY(ColorCorrectionEffect, Contrast);
FLOAT_PROPERTY(ColorCorrectionEffect, Saturation);
const Reflection::PropDescriptor<ColorCorrectionEffect, Color3>
    ColorCorrectionEffect::prop_TintColor("TintColor", category_Appearance,
        &ColorCorrectionEffect::getTintColor, &ColorCorrectionEffect::setTintColor);
const Reflection::PropDescriptor<Atmosphere, Color3> Atmosphere::prop_Color(
    "Color", category_Appearance, &Atmosphere::getColor, &Atmosphere::setColor);
const Reflection::PropDescriptor<Atmosphere, Color3> Atmosphere::prop_Decay(
    "Decay", category_Appearance, &Atmosphere::getDecay, &Atmosphere::setDecay);
FLOAT_PROPERTY(Atmosphere, Density);
FLOAT_PROPERTY(Atmosphere, Glare);
FLOAT_PROPERTY(Atmosphere, Haze);
FLOAT_PROPERTY(Atmosphere, Offset);
#undef FLOAT_PROPERTY

PostEffect::PostEffect() : enabled(true) { setName(sPostEffect); }
void PostEffect::setEnabled(bool value) { if (enabled != value) { enabled = value; raisePropertyChanged(prop_Enabled); } }

BloomEffect::BloomEffect() : intensity(1), size(24), threshold(0.95f) { setName(sBloomEffect); }
void BloomEffect::setIntensity(float value) { value = finiteClamp(value, 0, 10); if (intensity != value) { intensity = value; raisePropertyChanged(prop_Intensity); } }
void BloomEffect::setSize(float value) { value = finiteClamp(value, 0, 56); if (size != value) { size = value; raisePropertyChanged(prop_Size); } }
void BloomEffect::setThreshold(float value) { value = finiteClamp(value, 0, 100); if (threshold != value) { threshold = value; raisePropertyChanged(prop_Threshold); } }

SunRaysEffect::SunRaysEffect() : intensity(0.25f), spread(1) { setName(sSunRaysEffect); }
void SunRaysEffect::setIntensity(float value) { value = finiteClamp(value, 0, 1); if (intensity != value) { intensity = value; raisePropertyChanged(prop_Intensity); } }
void SunRaysEffect::setSpread(float value) { value = finiteClamp(value, 0, 1); if (spread != value) { spread = value; raisePropertyChanged(prop_Spread); } }

DepthOfFieldEffect::DepthOfFieldEffect() : farIntensity(0.75f), focusDistance(10), inFocusRadius(10), nearIntensity(0.75f) { setName(sDepthOfFieldEffect); }
void DepthOfFieldEffect::setFarIntensity(float value) { value = finiteClamp(value, 0, 1); if (farIntensity != value) { farIntensity = value; raisePropertyChanged(prop_FarIntensity); } }
void DepthOfFieldEffect::setFocusDistance(float value) { value = finiteClamp(value, 0, 100000); if (focusDistance != value) { focusDistance = value; raisePropertyChanged(prop_FocusDistance); } }
void DepthOfFieldEffect::setInFocusRadius(float value) { value = finiteClamp(value, 0, 100000); if (inFocusRadius != value) { inFocusRadius = value; raisePropertyChanged(prop_InFocusRadius); } }
void DepthOfFieldEffect::setNearIntensity(float value) { value = finiteClamp(value, 0, 1); if (nearIntensity != value) { nearIntensity = value; raisePropertyChanged(prop_NearIntensity); } }

ColorCorrectionEffect::ColorCorrectionEffect() : brightness(0), contrast(0), saturation(0), tintColor(Color3::white()) { setName(sColorCorrectionEffect); }
void ColorCorrectionEffect::setBrightness(float value) { value = finiteClamp(value, -1, 1); if (brightness != value) { brightness = value; raisePropertyChanged(prop_Brightness); } }
void ColorCorrectionEffect::setContrast(float value) { value = finiteClamp(value, -1, 1); if (contrast != value) { contrast = value; raisePropertyChanged(prop_Contrast); } }
void ColorCorrectionEffect::setSaturation(float value) { value = finiteClamp(value, -1, 1); if (saturation != value) { saturation = value; raisePropertyChanged(prop_Saturation); } }
void ColorCorrectionEffect::setTintColor(Color3 value) { value = colorClamp(value); if (tintColor != value) { tintColor = value; raisePropertyChanged(prop_TintColor); } }

Atmosphere::Atmosphere() : color(Color3::white()), decay(0.4f, 0.4f, 0.4f), density(0.3f), glare(0), haze(0), offset(0.25f) { setName(sAtmosphere); }
void Atmosphere::setColor(Color3 value) { value = colorClamp(value); if (color != value) { color = value; raisePropertyChanged(prop_Color); } }
void Atmosphere::setDecay(Color3 value) { value = colorClamp(value); if (decay != value) { decay = value; raisePropertyChanged(prop_Decay); } }
void Atmosphere::setDensity(float value) { value = finiteClamp(value, 0, 1); if (density != value) { density = value; raisePropertyChanged(prop_Density); } }
void Atmosphere::setGlare(float value) { value = finiteClamp(value, 0, 10); if (glare != value) { glare = value; raisePropertyChanged(prop_Glare); } }
void Atmosphere::setHaze(float value) { value = finiteClamp(value, 0, 10); if (haze != value) { haze = value; raisePropertyChanged(prop_Haze); } }
void Atmosphere::setOffset(float value) { value = finiteClamp(value, -1, 1); if (offset != value) { offset = value; raisePropertyChanged(prop_Offset); } }
