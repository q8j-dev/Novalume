#pragma once

#include "V8Tree/Instance.h"

namespace RBX {

extern const char* const sPostEffect;
class PostEffect : public DescribedNonCreatable<PostEffect, Instance, sPostEffect>
{
public:
    PostEffect();
    static const Reflection::PropDescriptor<PostEffect, bool> prop_Enabled;
    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);

private:
    bool enabled;
};

extern const char* const sBloomEffect;
class BloomEffect : public DescribedCreatable<BloomEffect, PostEffect, sBloomEffect>
{
public:
    BloomEffect();
    static const Reflection::PropDescriptor<BloomEffect, float> prop_Intensity;
    static const Reflection::PropDescriptor<BloomEffect, float> prop_Size;
    static const Reflection::PropDescriptor<BloomEffect, float> prop_Threshold;
    float getIntensity() const { return intensity; }
    float getSize() const { return size; }
    float getThreshold() const { return threshold; }
    void setIntensity(float value);
    void setSize(float value);
    void setThreshold(float value);

private:
    float intensity;
    float size;
    float threshold;
};

extern const char* const sSunRaysEffect;
class SunRaysEffect : public DescribedCreatable<SunRaysEffect, PostEffect, sSunRaysEffect>
{
public:
    SunRaysEffect();
    static const Reflection::PropDescriptor<SunRaysEffect, float> prop_Intensity;
    static const Reflection::PropDescriptor<SunRaysEffect, float> prop_Spread;
    float getIntensity() const { return intensity; }
    float getSpread() const { return spread; }
    void setIntensity(float value);
    void setSpread(float value);

private:
    float intensity;
    float spread;
};

extern const char* const sDepthOfFieldEffect;
class DepthOfFieldEffect : public DescribedCreatable<DepthOfFieldEffect, PostEffect, sDepthOfFieldEffect>
{
public:
    DepthOfFieldEffect();
    static const Reflection::PropDescriptor<DepthOfFieldEffect, float> prop_FarIntensity;
    static const Reflection::PropDescriptor<DepthOfFieldEffect, float> prop_FocusDistance;
    static const Reflection::PropDescriptor<DepthOfFieldEffect, float> prop_InFocusRadius;
    static const Reflection::PropDescriptor<DepthOfFieldEffect, float> prop_NearIntensity;
    float getFarIntensity() const { return farIntensity; }
    float getFocusDistance() const { return focusDistance; }
    float getInFocusRadius() const { return inFocusRadius; }
    float getNearIntensity() const { return nearIntensity; }
    void setFarIntensity(float value);
    void setFocusDistance(float value);
    void setInFocusRadius(float value);
    void setNearIntensity(float value);

private:
    float farIntensity;
    float focusDistance;
    float inFocusRadius;
    float nearIntensity;
};

extern const char* const sColorCorrectionEffect;
class ColorCorrectionEffect : public DescribedCreatable<ColorCorrectionEffect, PostEffect, sColorCorrectionEffect>
{
public:
    ColorCorrectionEffect();
    static const Reflection::PropDescriptor<ColorCorrectionEffect, float> prop_Brightness;
    static const Reflection::PropDescriptor<ColorCorrectionEffect, float> prop_Contrast;
    static const Reflection::PropDescriptor<ColorCorrectionEffect, float> prop_Saturation;
    static const Reflection::PropDescriptor<ColorCorrectionEffect, Color3> prop_TintColor;
    float getBrightness() const { return brightness; }
    float getContrast() const { return contrast; }
    float getSaturation() const { return saturation; }
    Color3 getTintColor() const { return tintColor; }
    void setBrightness(float value);
    void setContrast(float value);
    void setSaturation(float value);
    void setTintColor(Color3 value);

private:
    float brightness;
    float contrast;
    float saturation;
    Color3 tintColor;
};

extern const char* const sAtmosphere;
class Atmosphere : public DescribedCreatable<Atmosphere, Instance, sAtmosphere>
{
public:
    Atmosphere();
    static const Reflection::PropDescriptor<Atmosphere, Color3> prop_Color;
    static const Reflection::PropDescriptor<Atmosphere, Color3> prop_Decay;
    static const Reflection::PropDescriptor<Atmosphere, float> prop_Density;
    static const Reflection::PropDescriptor<Atmosphere, float> prop_Glare;
    static const Reflection::PropDescriptor<Atmosphere, float> prop_Haze;
    static const Reflection::PropDescriptor<Atmosphere, float> prop_Offset;
    Color3 getColor() const { return color; }
    Color3 getDecay() const { return decay; }
    float getDensity() const { return density; }
    float getGlare() const { return glare; }
    float getHaze() const { return haze; }
    float getOffset() const { return offset; }
    void setColor(Color3 value);
    void setDecay(Color3 value);
    void setDensity(float value);
    void setGlare(float value);
    void setHaze(float value);
    void setOffset(float value);

private:
    Color3 color;
    Color3 decay;
    float density;
    float glare;
    float haze;
    float offset;
};

}
