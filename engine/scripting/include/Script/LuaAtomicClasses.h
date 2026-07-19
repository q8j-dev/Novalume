#pragma once

#include "lua/LuaBridge.h"
#include "util/G3DCore.h"
#include "g3d/Color3.h"
#include "g3d/CoordinateFrame.h"
#include "g3d/Vector3.h"
#include "g3d/Vector3int16.h"
#include "RbxG3D/RbxRay.h"
#include "Util/BrickColor.h"
#include "Util/UDim.h"
#include "Util/Region3.h"
#include "Util/Region3int16.h"
#include "Util/Faces.h"
#include "Util/Axes.h"
#include "Util/CellID.h"
#include "util/PhysicalProperties.h"
#include "util/Content.h"
#include "util/Font.h"
#include "v8datamodel/NumberSequence.h"
#include "v8datamodel/ColorSequence.h"
#include "v8datamodel/NumberRange.h"
#include "util/Path2DControlPoint.h"
#include "util/DateTime.h"
#include "v8datamodel/TweenService.h"
#include "v8datamodel/Raycast.h"

#include <random>

namespace RBX { namespace Lua {

    class ScriptRandom
    {
    public:
        explicit ScriptRandom(std::uint64_t seed = 0) : engine(seed) {}
        bool operator==(const ScriptRandom& other) const { return engine == other.engine; }
        std::mt19937_64 engine;
    };

    class RandomBridge : public Bridge<ScriptRandom>
    {
        friend class Bridge<ScriptRandom>;
    public:
        static void registerClassLibrary(lua_State* L);
    private:
        static int newRandom(lua_State* L);
        static int clone(lua_State* L);
        static int nextInteger(lua_State* L);
        static int nextNumber(lua_State* L);
        static int nextUnitVector(lua_State* L);
        static int shuffle(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    template<> int Bridge<ScriptRandom>::on_index(
        const ScriptRandom& value, const char* name, lua_State* L);
    template<> void Bridge<ScriptRandom>::on_newindex(
        ScriptRandom& value, const char* name, lua_State* L);
    template<> int Bridge<ScriptRandom>::on_tostring(
        const ScriptRandom& value, lua_State* L);

    class TweenInfoBridge : public Bridge<TweenInfo>
    {
        friend class Bridge<TweenInfo>;
    public:
        static void registerClassLibrary(lua_State* L);
        static void pushTweenInfo(lua_State* L, const TweenInfo& value)
        {
            pushNewObject(L, value);
        }
    private:
        static int newTweenInfo(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    template<> int Bridge<TweenInfo>::on_index(
        const TweenInfo& value, const char* name, lua_State* L);
    template<> void Bridge<TweenInfo>::on_newindex(
        TweenInfo& value, const char* name, lua_State* L);
    template<> int Bridge<TweenInfo>::on_tostring(
        const TweenInfo& value, lua_State* L);

    class RaycastParamsBridge : public Bridge<RaycastParams>
    {
        friend class Bridge<RaycastParams>;
    public:
        static void registerClassLibrary(lua_State* L);
    private:
        static int newRaycastParams(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    class RaycastResultBridge : public Bridge<RaycastResult>
    {
        friend class Bridge<RaycastResult>;
    };

    class OverlapParamsBridge : public Bridge<OverlapParams>
    {
        friend class Bridge<OverlapParams>;
    public:
        static void registerClassLibrary(lua_State* L);
    private:
        static int newOverlapParams(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    template<> int Bridge<RaycastParams>::on_index(
        const RaycastParams& value, const char* name, lua_State* L);
    template<> void Bridge<RaycastParams>::on_newindex(
        RaycastParams& value, const char* name, lua_State* L);
    template<> int Bridge<RaycastParams>::on_tostring(
        const RaycastParams& value, lua_State* L);
    template<> int Bridge<OverlapParams>::on_index(
        const OverlapParams& value, const char* name, lua_State* L);
    template<> void Bridge<OverlapParams>::on_newindex(
        OverlapParams& value, const char* name, lua_State* L);
    template<> int Bridge<OverlapParams>::on_tostring(
        const OverlapParams& value, lua_State* L);
    template<> int Bridge<RaycastResult>::on_index(
        const RaycastResult& value, const char* name, lua_State* L);
    template<> void Bridge<RaycastResult>::on_newindex(
        RaycastResult& value, const char* name, lua_State* L);
    template<> int Bridge<RaycastResult>::on_tostring(
        const RaycastResult& value, lua_State* L);

	class CoordinateFrameBridge : public Bridge<G3D::CoordinateFrame>
	{
		friend class Bridge< G3D::CoordinateFrame >;
	public:
		static void registerClassLibrary (lua_State *L);
		static void pushCoordinateFrame(lua_State *L, const G3D::CoordinateFrame& CF)
		{
			pushNewObject(L, CF);
		}
	private:
		static int newCoordinateFrame(lua_State *L);
		static int fromEulerAnglesXYZ(lua_State *L);
		static int fromEulerAnglesYXZ(lua_State *L);
		static int fromMatrix(lua_State *L);
		static int lookAt(lua_State *L);
		static int fromAxisAngle(lua_State *L);
		static int on_add(lua_State *L);
		static int on_sub(lua_State *L);
		static int on_mul(lua_State *L);
		static int on_inverse(lua_State *L);
		static int on_lerp(lua_State *L);
		static int on_orthonormalize(lua_State *L);

		// Implementation of G3D::CoordinateFrame help functions
		static int on_toWorldSpace(lua_State *L);
		static int on_toObjectSpace(lua_State *L);
		static int on_pointToWorldSpace(lua_State *L);
		static int on_pointToObjectSpace(lua_State *L);
		static int on_vectorToWorldSpace(lua_State *L);
		static int on_vectorToObjectSpace(lua_State *L);
		static int on_toEulerAnglesXYZ(lua_State *L);
		static int on_toEulerAnglesYXZ(lua_State *L);
		static int on_toAxisAngle(lua_State *L);
		static int on_fuzzyEq(lua_State *L);
		static int on_angleBetween(lua_State *L);
		static int on_components(lua_State *L);

		static const luaL_reg classLibrary[];
	};

	class PhysicalPropertiesBridge : public Bridge<PhysicalProperties>
	{
		friend class Bridge<PhysicalProperties>;
	public:
		static void registerClassLibrary (lua_State *L);

		static void pushPhysicalProperties(lua_State *L, const PhysicalProperties& v)
		{
			if (v.getCustomEnabled() == true)
			{
				pushNewObject(L, v);
			}
			else
			{
				lua_pushnil(L);
			}
		}
	private:
		static int newPhysicalProperties(lua_State *L);
		static const luaL_reg classLibrary[];
	};

    class Rect2DBridge : public Bridge<G3D::Rect2D>
    {
        friend class Bridge< G3D::Rect2D >;
	public:
		static void registerClassLibrary (lua_State *L);
        
		static void pushRect2D(lua_State *L, const G3D::Rect2D& v)
		{
			pushNewObject(L, v);
		}
	private:
		static int newRect2D(lua_State *L);
		static const luaL_reg classLibrary[];
    };
    
	class Region3Bridge : public Bridge<RBX::Region3>
	{
		friend class Bridge< RBX::Region3 >;
	public:
		static void registerClassLibrary (lua_State *L) ;

		static void pushRegion3(lua_State *L, const RBX::Region3& v)
		{
			pushNewObject(L, v);
		}
	private:
		static int newRegion3(lua_State *L);
		static int expandToGrid(lua_State *L);
		static const luaL_reg classLibrary[];
	};

	class Region3int16Bridge : public Bridge<RBX::Region3int16>
	{
		friend class Bridge< RBX::Region3int16 >;
	public:
		static void registerClassLibrary (lua_State *L);

		static void pushRegion3int16(lua_State *L, const RBX::Region3int16& v)
		{
			pushNewObject(L, v);
		}
	private:
		static int newRegion3int16(lua_State *L);
		static const luaL_reg classLibrary[];
	};

	class Vector3Bridge : public Bridge<G3D::Vector3>
	{
		friend class Bridge< G3D::Vector3 >;
	public:
		static void registerClassLibrary (lua_State *L);
		using Bridge<G3D::Vector3>::getValue;
		static G3D::Vector3& getObject(lua_State* L, unsigned int index);
		static bool getValue(lua_State* L, unsigned int index, G3D::Vector3& value);

		static void pushVector3(lua_State *L, const G3D::Vector3& v)
		{
			pushNewObject(L, v);
		}
	private:
		static int newVector3(lua_State *L);
		static int newVector3FromNormalId(lua_State *L);
		static int newVector3FromAxis(lua_State *L);
		static int on_add(lua_State *L);
		static int on_sub(lua_State *L);
		static int on_mul(lua_State *L);
		static int on_div(lua_State *L);
		static int on_unm(lua_State *L);
		static const luaL_reg classLibrary[];
	};

	class Vector3int16Bridge : public Bridge<G3D::Vector3int16>
	{
		friend class Bridge< G3D::Vector3int16 >;
	public:
		static void registerClassLibrary (lua_State *L); 

		static void pushVector3int16(lua_State *L, const G3D::Vector3int16& v)
		{
			pushNewObject(L, v);
		}
	private:
		static int newVector3int16(lua_State *L);
		static int on_add(lua_State *L);
		static int on_sub(lua_State *L);
		static int on_mul(lua_State *L);
		static int on_div(lua_State *L);
		static int on_unm(lua_State *L);
		static const luaL_reg classLibrary[];
	};

	class RbxRayBridge : public Bridge<RBX::RbxRay>
	{
		friend class Bridge< RBX::RbxRay >;
	public:
		static void registerClassLibrary (lua_State *L);
			
		static void pushRay(lua_State *L, const RBX::RbxRay& v)
		{
			pushNewObject(L, v);
		}
	private:
		static int newRbxRay(lua_State *L);
		//static int on_add(lua_State *L);
		//static int on_sub(lua_State *L);
		//static int on_mul(lua_State *L);
		//static int on_div(lua_State *L);
		//static int on_unm(lua_State *L);
		static const luaL_reg classLibrary[];
	};


	class Vector2Bridge : public Bridge<RBX::Vector2>
	{
		friend class Bridge< RBX::Vector2 >;
	public:
		static void registerClassLibrary (lua_State *L);

		static void pushVector2(lua_State *L, const RBX::Vector2& v)
		{
			pushNewObject(L, v);
		}
	private:
		static int newVector2(lua_State *L);
		static int on_add(lua_State *L);
		static int on_sub(lua_State *L);
		static int on_mul(lua_State *L);
		static int on_div(lua_State *L);
		static int on_unm(lua_State *L);
		static const luaL_reg classLibrary[];
	};

	class Vector2int16Bridge : public Bridge<RBX::Vector2int16>
	{
		friend class Bridge< RBX::Vector2int16 >;
	public:
		static void registerClassLibrary (lua_State *L);

		static void pushVector2int16(lua_State *L, const RBX::Vector2int16& v)
		{
			pushNewObject(L, v);
		}
	private:
		static int newVector2int16(lua_State *L);
		static int on_add(lua_State *L);
		static int on_sub(lua_State *L);
		static int on_mul(lua_State *L);
		static int on_div(lua_State *L);
		static int on_unm(lua_State *L);
		static const luaL_reg classLibrary[];
	};

	class Color3Bridge : public Bridge<G3D::Color3>
	{
		friend class Bridge< G3D::Color3 >;
	public:
		static void registerClassLibrary (lua_State *L);
		static void pushColor3(lua_State *L, const G3D::Color3& color);

	private:
		static int newColor3(lua_State *L);
		static int newRGBColor3(lua_State *L);
		static int newHSVColor3(lua_State* L);
		static int newHexColor3(lua_State* L);
		static int lerp(lua_State* L);
		static int toHSV(lua_State* L);
		static int toHex(lua_State* L);
		static const luaL_reg classLibrary[];
	};

	class ContentBridge : public Bridge<RBX::Content>
	{
		friend class Bridge<RBX::Content>;
	public:
		static void registerClassLibrary(lua_State* L);
		static void pushContent(lua_State* L, const RBX::Content& content)
		{
			pushNewObject(L, content);
		}
	private:
		static int fromUri(lua_State* L);
		static int fromAssetId(lua_State* L);
		static int fromObject(lua_State* L);
		static const luaL_reg classLibrary[];
	};

	template<> int Bridge<RBX::Content>::on_index(
		const RBX::Content& content, const char* name, lua_State* L);
	template<> void Bridge<RBX::Content>::on_newindex(
		RBX::Content& content, const char* name, lua_State* L);
	template<> int Bridge<RBX::Content>::on_tostring(
		const RBX::Content& content, lua_State* L);

	class FontBridge : public Bridge<RBX::Font>
	{
		friend class Bridge<RBX::Font>;
	public:
		static void registerClassLibrary(lua_State* L);
		static void pushFont(lua_State* L, const RBX::Font& font)
		{
			pushNewObject(L, font);
		}
	private:
		static int newFont(lua_State* L);
		static int fromEnum(lua_State* L);
		static const luaL_reg classLibrary[];
	};

	template<> int Bridge<RBX::Font>::on_index(
		const RBX::Font& font, const char* name, lua_State* L);
	template<> void Bridge<RBX::Font>::on_newindex(
		RBX::Font& font, const char* name, lua_State* L);
	template<> int Bridge<RBX::Font>::on_tostring(
		const RBX::Font& font, lua_State* L);

	class UDimBridge : public Bridge<RBX::UDim>
	{
		friend class Bridge< RBX::UDim>;
	public:
		static void registerClassLibrary (lua_State *L);

		static void pushUDim(lua_State *L, const RBX::UDim& v)
		{
			pushNewObject(L, v);
		}
	private:
		static int newUDim(lua_State *L);
		static int on_add(lua_State *L);
		static int on_sub(lua_State *L);
		static int on_unm(lua_State *L);
		static int lerp(lua_State *L);
		static const luaL_reg classLibrary[];
	};

	class UDim2Bridge : public Bridge<RBX::UDim2>
	{
		friend class Bridge< RBX::UDim2>;
	public:
		static void registerClassLibrary (lua_State *L);

	private:
		static int newUDim2 (lua_State *L);
		static int fromScale(lua_State* L);
		static int fromOffset(lua_State* L);
		static int on_add(lua_State *L);
		static int on_sub(lua_State *L);
		static int on_unm(lua_State *L);
		static int lerp(lua_State *L);

		static const luaL_reg classLibrary[];
	};

	class FacesBridge : public Bridge<RBX::Faces>
	{
		friend class Bridge< RBX::Faces>;
	public:
		static void registerClassLibrary (lua_State *L);

	private:
		static int newFaces (lua_State *L);
		static const luaL_reg classLibrary[];
	};

	class AxesBridge : public Bridge<RBX::Axes>
	{
		friend class Bridge< RBX::Axes>;
	public:
		static void registerClassLibrary (lua_State *L);

	private:
		static int newAxes(lua_State *L);
		static const luaL_reg classLibrary[];
	};
	
	class BrickColorBridge : public Bridge<RBX::BrickColor>
	{
		friend class Bridge< RBX::BrickColor >;
	public:
		static void registerClassLibrary (lua_State *L) ;

	private:
		static int newBrickColor(lua_State *L);
		static int randomBrickColor(lua_State *L);
		static int paletteBrickColor(lua_State *L);
		static const luaL_reg classLibrary[];
	};

	// CellID bridge for cluster access	
	class CellIDBridge : public Bridge<CellID>
	{
		friend class Bridge< CellID >;
	public:
		static void registerClassLibrary (lua_State *L) ;

		static void pushCellID(lua_State *L, const CellID& v)
		{
			pushNewObject(L, v);
		}
	private:
		static int newCellID(lua_State *L);
		static const luaL_reg classLibrary[];
	};

    // Number sequence for particle props
    class NumberSequenceBridge : public Bridge<NumberSequence>
    {
        friend class Bridge< NumberSequence >;
    public:
        static void registerClassLibrary(lua_State* L);
        static void pushNumberSequence(lua_State* L, const NumberSequence& v) { pushNewObject(L, v); }
    private:
        static int newNumberSequence(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    // Number sequence for particle props
    class ColorSequenceBridge : public Bridge<ColorSequence>
    {
        friend class Bridge< ColorSequence >;
    public:
        static void registerClassLibrary(lua_State* L);
        static void pushColorSequence(lua_State* L, const ColorSequence& v) { pushNewObject(L, v); }
    private:
        static int newColorSequence(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    class NumberSequenceKeypointBridge : public Bridge<NumberSequenceKeypoint>
    {
        friend class Bridge< NumberSequenceKeypoint >;
    public:
        static void registerClassLibrary(lua_State* L);
        static void pushNumberSequenceKeypoint(lua_State* L, const NumberSequenceKeypoint& v) { pushNewObject(L, v); }
    private:
        static int newNumberSequenceKeypoint(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    class ColorSequenceKeypointBridge : public Bridge<ColorSequenceKeypoint>
    {
        friend class Bridge< ColorSequenceKeypoint >;
    public:
        static void registerClassLibrary(lua_State* L);
        static void pushColorSequenceKeypoint(lua_State* L, const ColorSequenceKeypoint& v) { pushNewObject(L, v); }
    private:
        static int newColorSequenceKeypoint(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    class NumberRangeBridge : public Bridge<NumberRange>
    {
        friend class Bridge< NumberRange >;
    public:
        static void registerClassLibrary(lua_State* L);
        static void pushNumberRange(lua_State* L, const NumberRange& v) { pushNewObject(L, v); }
    private:
        static int newNumberRange(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    class Path2DControlPointBridge : public Bridge<Path2DControlPoint>
    {
        friend class Bridge<Path2DControlPoint>;
    public:
        static void registerClassLibrary(lua_State* L);
        static void pushPath2DControlPoint(lua_State* L, const Path2DControlPoint& value)
        {
            pushNewObject(L, value);
        }
    private:
        static int newPath2DControlPoint(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    class DateTimeBridge : public Bridge<DateTime>
    {
        friend class Bridge<DateTime>;
    public:
        static void registerClassLibrary(lua_State* L);
        static void pushDateTime(lua_State* L, const DateTime& value)
        {
            pushNewObject(L, value);
        }
    private:
        static int now(lua_State* L);
        static int fromUnixTimestamp(lua_State* L);
        static int fromUnixTimestampMillis(lua_State* L);
        static int fromUniversalTime(lua_State* L);
        static int fromLocalTime(lua_State* L);
        static int fromIsoDate(lua_State* L);
        static int toUniversalTime(lua_State* L);
        static int toLocalTime(lua_State* L);
        static int toIsoDate(lua_State* L);
        static int formatUniversalTime(lua_State* L);
        static int formatLocalTime(lua_State* L);
        static const luaL_reg classLibrary[];
    };

    template<> int Bridge<RBX::DateTime>::on_index(
        const RBX::DateTime& value, const char* name, lua_State* L);
    template<> void Bridge<RBX::DateTime>::on_newindex(
        RBX::DateTime& value, const char* name, lua_State* L);
    template<> int Bridge<RBX::DateTime>::on_tostring(
        const RBX::DateTime& value, lua_State* L);

// Specialization to implement arithmatic operators
template<>
void Bridge<G3D::Vector3int16>::registerClass (lua_State *L);

template<>
void Bridge<G3D::Vector3>::registerClass (lua_State *L);

template<>
void Bridge<RBX::Vector2>::registerClass (lua_State *L); 

template<>
void Bridge<G3D::CoordinateFrame>::registerClass (lua_State *L);

// Specialization to implement arithmatic operators
template<>
void Bridge<RBX::UDim>::registerClass (lua_State *L);

// Specialization to implement arithmatic operators
template<>
void Bridge<RBX::UDim2>::registerClass (lua_State *L);

} }
