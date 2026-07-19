
#include "script/LuaAtomicClasses.h"
#include "script/LuaEnum.h"
#include "util/PartMaterial.h"
#include "v8world/MaterialProperties.h"
#include "G3D/Quat.h"
#include "rbxformat.h"
#include "script/LuaInstanceBridge.h"
#include "util/Content.h"
#include "util/Font.h"
#include "v8datamodel/TextService.h"
#include "rbx/assets/AssetMountTable.h"

#include <rapidjson/document.h>

#include <array>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <unordered_map>

#if !defined(RBX_LUAU_VM)
#include "lstate.h"
#include "lfunc.h"
#endif

FASTFLAGVARIABLE(PhysPropConstructFromMaterial, false)

namespace RBX { namespace Lua {

template<> const char* Bridge<ScriptRandom>::className("Random");
template<> const char* Bridge<TweenInfo>::className("TweenInfo");
template<> const char* Bridge<RaycastParams>::className("RaycastParams");
template<> const char* Bridge<OverlapParams>::className("OverlapParams");
template<> const char* Bridge<RaycastResult>::className("RaycastResult");

const luaL_reg RandomBridge::classLibrary[] = {
    {"new", newRandom},
    {NULL, NULL}
};

void RandomBridge::registerClassLibrary(lua_State* L)
{
    luaL_register(L, className, classLibrary);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int RandomBridge::newRandom(lua_State* L)
{
    std::uint64_t seed;
    if (lua_isnoneornil(L, 1))
    {
        std::random_device source;
        seed = (static_cast<std::uint64_t>(source()) << 32) ^ source();
    }
    else
    {
        const double requested = luaL_checknumber(L, 1);
        if (!std::isfinite(requested))
            throw RBX::runtime_error("Random.new seed must be finite");
        seed = static_cast<std::uint64_t>(static_cast<std::int64_t>(requested));
    }
    pushNewObject(L, ScriptRandom(seed));
    return 1;
}

int RandomBridge::clone(lua_State* L)
{
    pushNewObject(L, getObject(L, 1));
    return 1;
}

int RandomBridge::nextInteger(lua_State* L)
{
    ScriptRandom& value = getObject(L, 1);
    const std::int64_t minimum = static_cast<std::int64_t>(luaL_checknumber(L, 2));
    const std::int64_t maximum = static_cast<std::int64_t>(luaL_checknumber(L, 3));
    if (minimum > maximum)
        throw RBX::runtime_error("Random:NextInteger minimum must not exceed maximum");
    std::uniform_int_distribution<std::int64_t> distribution(minimum, maximum);
    lua_pushnumber(L, static_cast<double>(distribution(value.engine)));
    return 1;
}

int RandomBridge::nextNumber(lua_State* L)
{
    ScriptRandom& value = getObject(L, 1);
    const double minimum = lua_isnoneornil(L, 2) ? 0.0 : luaL_checknumber(L, 2);
    const double maximum = lua_isnoneornil(L, 3) ? 1.0 : luaL_checknumber(L, 3);
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum)
        throw RBX::runtime_error("Random:NextNumber bounds must be finite and ordered");
    std::uniform_real_distribution<double> distribution(minimum, maximum);
    lua_pushnumber(L, distribution(value.engine));
    return 1;
}

int RandomBridge::nextUnitVector(lua_State* L)
{
    ScriptRandom& value = getObject(L, 1);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const double z = unit(value.engine) * 2.0 - 1.0;
    const double angle = unit(value.engine) * 2.0 * G3D::pi();
    const double radius = std::sqrt(std::max(0.0, 1.0 - z * z));
    Vector3Bridge::pushVector3(L, G3D::Vector3(
        static_cast<float>(radius * std::cos(angle)),
        static_cast<float>(radius * std::sin(angle)), static_cast<float>(z)));
    return 1;
}

int RandomBridge::shuffle(lua_State* L)
{
    ScriptRandom& value = getObject(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    const int length = static_cast<int>(lua_objlen(L, 2));
    for (int index = length; index > 1; --index)
    {
        std::uniform_int_distribution<int> distribution(1, index);
        const int other = distribution(value.engine);
        lua_rawgeti(L, 2, index);
        lua_rawgeti(L, 2, other);
        lua_rawseti(L, 2, index);
        lua_rawseti(L, 2, other);
    }
    return 0;
}

template<>
int Bridge<ScriptRandom>::on_index(
    const ScriptRandom&, const char* name, lua_State* L)
{
    if (strcmp(name, "Clone") == 0)
        return lua_pushcfunction(L, RandomBridge::clone), 1;
    if (strcmp(name, "NextInteger") == 0)
        return lua_pushcfunction(L, RandomBridge::nextInteger), 1;
    if (strcmp(name, "NextNumber") == 0)
        return lua_pushcfunction(L, RandomBridge::nextNumber), 1;
    if (strcmp(name, "NextUnitVector") == 0)
        return lua_pushcfunction(L, RandomBridge::nextUnitVector), 1;
    if (strcmp(name, "Shuffle") == 0)
        return lua_pushcfunction(L, RandomBridge::shuffle), 1;
    throw RBX::runtime_error("'%s' is not a valid member of Random", name);
}

template<>
void Bridge<ScriptRandom>::on_newindex(
    ScriptRandom&, const char* name, lua_State*)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}

const luaL_reg TweenInfoBridge::classLibrary[] = {
    {"new", newTweenInfo},
    {NULL, NULL}
};

void TweenInfoBridge::registerClassLibrary(lua_State* L)
{
    luaL_register(L, className, classLibrary);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

namespace {
template<typename Enum>
Enum tweenInfoEnum(lua_State* L, int index, Enum defaultValue, const char* name)
{
    if (lua_isnoneornil(L, index))
        return defaultValue;
    EnumDescriptorItemPtr item;
    if (!EnumItem::getItem(L, index, item) || !item->owner.isType<Enum>())
        throw RBX::runtime_error("TweenInfo.new expects %s for argument %d", name, index);
    return static_cast<Enum>(item->value);
}
}

int TweenInfoBridge::newTweenInfo(lua_State* L)
{
    const float time = lua_isnoneornil(L, 1) ? 1.0f : luaL_checknumber(L, 1);
    const GuiObject::TweenEasingStyle style = tweenInfoEnum(
        L, 2, GuiObject::EASING_STYLE_QUAD, "Enum.EasingStyle");
    const GuiObject::TweenEasingDirection direction = tweenInfoEnum(
        L, 3, GuiObject::EASING_DIRECTION_OUT, "Enum.EasingDirection");
    const int repeatCount = lua_isnoneornil(L, 4) ? 0 : luaL_checkinteger(L, 4);
    const bool reverses = !lua_isnoneornil(L, 5) && lua_toboolean(L, 5);
    const float delayTime = lua_isnoneornil(L, 6) ? 0.0f : luaL_checknumber(L, 6);
    if (!std::isfinite(time) || time < 0 || !std::isfinite(delayTime) || delayTime < 0 ||
        repeatCount < -1)
        throw RBX::runtime_error("TweenInfo.new received an invalid duration or repeat count");
    pushNewObject(L, TweenInfo(time, style, direction, repeatCount, reverses, delayTime));
    return 1;
}

template<>
int Bridge<TweenInfo>::on_index(
    const TweenInfo& value, const char* name, lua_State* L)
{
    if (strcmp(name, "Time") == 0)
        return lua_pushnumber(L, value.time), 1;
    if (strcmp(name, "EasingStyle") == 0)
    {
        EnumItem::push(L, Reflection::EnumDesc<GuiObject::TweenEasingStyle>::singleton()
            .convertToItem(value.easingStyle));
        return 1;
    }
    if (strcmp(name, "EasingDirection") == 0)
    {
        EnumItem::push(L, Reflection::EnumDesc<GuiObject::TweenEasingDirection>::singleton()
            .convertToItem(value.easingDirection));
        return 1;
    }
    if (strcmp(name, "RepeatCount") == 0)
        return lua_pushinteger(L, value.repeatCount), 1;
    if (strcmp(name, "Reverses") == 0)
        return lua_pushboolean(L, value.reverses), 1;
    if (strcmp(name, "DelayTime") == 0)
        return lua_pushnumber(L, value.delayTime), 1;
    throw RBX::runtime_error("'%s' is not a valid member of TweenInfo", name);
}

template<>
void Bridge<TweenInfo>::on_newindex(TweenInfo&, const char* name, lua_State*)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
int Bridge<TweenInfo>::on_tostring(const TweenInfo& value, lua_State* L)
{
    lua_pushfstring(L, "TweenInfo(%f, %d, %d, %d, %s, %f)", value.time,
        static_cast<int>(value.easingStyle), static_cast<int>(value.easingDirection),
        value.repeatCount, value.reverses ? "true" : "false", value.delayTime);
    return 1;
}

const luaL_reg RaycastParamsBridge::classLibrary[] = {
    {"new", newRaycastParams},
    {NULL, NULL}
};

void RaycastParamsBridge::registerClassLibrary(lua_State* L)
{
    luaL_register(L, className, classLibrary);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int RaycastParamsBridge::newRaycastParams(lua_State* L)
{
    if (lua_gettop(L) != 0)
        throw RBX::runtime_error("RaycastParams.new expects no arguments");
    pushNewObject(L, RaycastParams());
    return 1;
}

template<>
int Bridge<RaycastParams>::on_index(
    const RaycastParams& value, const char* name, lua_State* L)
{
    if (strcmp(name, "FilterDescendantsInstances") == 0)
    {
        const Instances empty;
        const Instances& instances = value.filterDescendantsInstances
            ? *value.filterDescendantsInstances : empty;
        lua_createtable(L, static_cast<int>(instances.size()), 0);
        int index = 1;
        for (Instances::const_iterator it = instances.begin(); it != instances.end(); ++it)
        {
            ObjectBridge::push(L, *it);
            lua_rawseti(L, -2, index++);
        }
        return 1;
    }
    if (strcmp(name, "FilterType") == 0)
    {
        EnumItem::push(L, Reflection::EnumDesc<::RBX::Enums::RaycastFilterType>::singleton()
            .convertToItem(value.filterType));
        return 1;
    }
    if (strcmp(name, "IgnoreWater") == 0)
        return lua_pushboolean(L, value.ignoreWater), 1;
    if (strcmp(name, "CollisionGroup") == 0)
        return lua_pushlstring(L, value.collisionGroup.data(), value.collisionGroup.size()), 1;
    if (strcmp(name, "RespectCanCollide") == 0)
        return lua_pushboolean(L, value.respectCanCollide), 1;
    if (strcmp(name, "BruteForceAllSlow") == 0)
        return lua_pushboolean(L, value.bruteForceAllSlow), 1;
    throw RBX::runtime_error("'%s' is not a valid member of RaycastParams", name);
}

namespace {

bool raycastBoolean(lua_State* L, int index, const char* property)
{
    if (!lua_isboolean(L, index))
        throw RBX::runtime_error("RaycastParams.%s must be a boolean", property);
    return lua_toboolean(L, index) != 0;
}

} // namespace

template<>
void Bridge<RaycastParams>::on_newindex(
    RaycastParams& value, const char* name, lua_State* L)
{
    if (strcmp(name, "FilterDescendantsInstances") == 0)
    {
        luaL_checktype(L, 3, LUA_TTABLE);
        shared_ptr<Instances> instances(new Instances());
        const int count = static_cast<int>(lua_objlen(L, 3));
        instances->reserve(count);
        for (int index = 1; index <= count; ++index)
        {
            lua_rawgeti(L, 3, index);
            shared_ptr<Instance> instance;
            const bool valid = ObjectBridge::getPtr(L, lua_gettop(L), instance);
            lua_pop(L, 1);
            if (!valid || !instance)
                throw RBX::runtime_error(
                    "RaycastParams.FilterDescendantsInstances[%d] must be an Instance", index);
            instances->push_back(instance);
        }
        value.filterDescendantsInstances = instances;
        return;
    }
    if (strcmp(name, "FilterType") == 0)
    {
        EnumDescriptorItemPtr item;
        if (!EnumItem::getItem(L, 3, item) ||
            !item->owner.isType<::RBX::Enums::RaycastFilterType>())
            throw RBX::runtime_error(
                "RaycastParams.FilterType must be an Enum.RaycastFilterType");
        value.filterType = static_cast<::RBX::Enums::RaycastFilterType>(item->value);
        return;
    }
    if (strcmp(name, "IgnoreWater") == 0)
        value.ignoreWater = raycastBoolean(L, 3, name);
    else if (strcmp(name, "CollisionGroup") == 0)
        value.collisionGroup = luaL_checkstring(L, 3);
    else if (strcmp(name, "RespectCanCollide") == 0)
        value.respectCanCollide = raycastBoolean(L, 3, name);
    else if (strcmp(name, "BruteForceAllSlow") == 0)
        value.bruteForceAllSlow = raycastBoolean(L, 3, name);
    else
        throw RBX::runtime_error("'%s' is not a valid member of RaycastParams", name);
}

template<>
int Bridge<RaycastParams>::on_tostring(const RaycastParams&, lua_State* L)
{
    lua_pushstring(L, "RaycastParams");
    return 1;
}

const luaL_reg OverlapParamsBridge::classLibrary[] = {
    {"new", newOverlapParams},
    {NULL, NULL}
};

void OverlapParamsBridge::registerClassLibrary(lua_State* L)
{
    luaL_register(L, className, classLibrary);
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

int OverlapParamsBridge::newOverlapParams(lua_State* L)
{
    if (lua_gettop(L) != 0)
        throw RBX::runtime_error("OverlapParams.new expects no arguments");
    pushNewObject(L, OverlapParams());
    return 1;
}

template<>
int Bridge<OverlapParams>::on_index(
    const OverlapParams& value, const char* name, lua_State* L)
{
    if (strcmp(name, "FilterDescendantsInstances") == 0)
    {
        const Instances empty;
        const Instances& instances = value.filterDescendantsInstances
            ? *value.filterDescendantsInstances : empty;
        lua_createtable(L, static_cast<int>(instances.size()), 0);
        int index = 1;
        for (Instances::const_iterator it = instances.begin(); it != instances.end(); ++it)
        {
            ObjectBridge::push(L, *it);
            lua_rawseti(L, -2, index++);
        }
        return 1;
    }
    if (strcmp(name, "FilterType") == 0)
    {
        EnumItem::push(L, Reflection::EnumDesc<::RBX::Enums::RaycastFilterType>::singleton()
            .convertToItem(value.filterType));
        return 1;
    }
    if (strcmp(name, "MaxParts") == 0)
        return lua_pushinteger(L, value.maxParts), 1;
    if (strcmp(name, "CollisionGroup") == 0)
        return lua_pushlstring(L, value.collisionGroup.data(), value.collisionGroup.size()), 1;
    if (strcmp(name, "RespectCanCollide") == 0)
        return lua_pushboolean(L, value.respectCanCollide), 1;
    if (strcmp(name, "BruteForceAllSlow") == 0)
        return lua_pushboolean(L, value.bruteForceAllSlow), 1;
    throw RBX::runtime_error("'%s' is not a valid member of OverlapParams", name);
}

template<>
void Bridge<OverlapParams>::on_newindex(
    OverlapParams& value, const char* name, lua_State* L)
{
    if (strcmp(name, "FilterDescendantsInstances") == 0)
    {
        luaL_checktype(L, 3, LUA_TTABLE);
        shared_ptr<Instances> instances(new Instances());
        const int count = static_cast<int>(lua_objlen(L, 3));
        instances->reserve(count);
        for (int index = 1; index <= count; ++index)
        {
            lua_rawgeti(L, 3, index);
            shared_ptr<Instance> instance;
            const bool valid = ObjectBridge::getPtr(L, lua_gettop(L), instance);
            lua_pop(L, 1);
            if (!valid || !instance)
                throw RBX::runtime_error(
                    "OverlapParams.FilterDescendantsInstances[%d] must be an Instance", index);
            instances->push_back(instance);
        }
        value.filterDescendantsInstances = instances;
        return;
    }
    if (strcmp(name, "FilterType") == 0)
    {
        EnumDescriptorItemPtr item;
        if (!EnumItem::getItem(L, 3, item) ||
            !item->owner.isType<::RBX::Enums::RaycastFilterType>())
            throw RBX::runtime_error(
                "OverlapParams.FilterType must be an Enum.RaycastFilterType");
        value.filterType = static_cast<::RBX::Enums::RaycastFilterType>(item->value);
        return;
    }
    if (strcmp(name, "MaxParts") == 0)
    {
        const int maxParts = static_cast<int>(luaL_checkinteger(L, 3));
        if (maxParts < 0)
            throw RBX::runtime_error("OverlapParams.MaxParts must be non-negative");
        value.maxParts = maxParts;
    }
    else if (strcmp(name, "CollisionGroup") == 0)
        value.collisionGroup = luaL_checkstring(L, 3);
    else if (strcmp(name, "RespectCanCollide") == 0)
        value.respectCanCollide = raycastBoolean(L, 3, name);
    else if (strcmp(name, "BruteForceAllSlow") == 0)
        value.bruteForceAllSlow = raycastBoolean(L, 3, name);
    else
        throw RBX::runtime_error("'%s' is not a valid member of OverlapParams", name);
}

template<>
int Bridge<OverlapParams>::on_tostring(const OverlapParams&, lua_State* L)
{
    lua_pushstring(L, "OverlapParams");
    return 1;
}

template<>
int Bridge<RaycastResult>::on_index(
    const RaycastResult& value, const char* name, lua_State* L)
{
    if (strcmp(name, "Instance") == 0)
        return ObjectBridge::push(L, value.instance), 1;
    if (strcmp(name, "Position") == 0)
        return Vector3Bridge::pushVector3(L, value.position), 1;
    if (strcmp(name, "Normal") == 0)
        return Vector3Bridge::pushVector3(L, value.normal), 1;
    if (strcmp(name, "Material") == 0)
    {
        EnumItem::push(L, Reflection::EnumDesc<PartMaterial>::singleton()
            .convertToItem(value.material));
        return 1;
    }
    if (strcmp(name, "Distance") == 0)
        return lua_pushnumber(L, value.distance), 1;
    throw RBX::runtime_error("'%s' is not a valid member of RaycastResult", name);
}

template<>
void Bridge<RaycastResult>::on_newindex(
    RaycastResult&, const char* name, lua_State*)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
int Bridge<RaycastResult>::on_tostring(const RaycastResult&, lua_State* L)
{
    lua_pushstring(L, "RaycastResult");
    return 1;
}

template<>
int Bridge<ScriptRandom>::on_tostring(
    const ScriptRandom&, lua_State* L)
{
    lua_pushstring(L, "Random");
    return 1;
}

template<>
const char* Bridge<RBX::Content>::className("Content");
template<>
const char* Bridge<RBX::Font>::className("Font");
template<>
const char* Bridge<RBX::Path2DControlPoint>::className("Path2DControlPoint");
template<>
const char* Bridge<RBX::DateTime>::className("DateTime");

namespace {

struct CalendarFields
{
	int year = 1970;
	int month = 1;
	int day = 1;
	int hour = 0;
	int minute = 0;
	int second = 0;
	int millisecond = 0;
	int weekday = 4;
};

struct DateTimeLocale
{
	std::array<std::string, 12> months;
	std::array<std::string, 12> monthsShort;
	std::array<std::string, 7> weekdays;
	std::array<std::string, 7> weekdaysShort;
	std::array<std::string, 7> weekdaysMin;
	std::unordered_map<std::string, std::string> longDateFormat;
	struct Meridiem
	{
		int startFrom;
		std::string lowerCase;
		std::string upperCase;
	};
	std::vector<Meridiem> meridiem;
};

std::int64_t floorDivide(std::int64_t value, std::int64_t divisor)
{
	std::int64_t quotient = value / divisor;
	if (value % divisor < 0)
		--quotient;
	return quotient;
}

std::int64_t daysFromCivil(int year, unsigned int month, unsigned int day)
{
	year -= month <= 2;
	const int era = (year >= 0 ? year : year - 399) / 400;
	const unsigned int yearOfEra = static_cast<unsigned int>(year - era * 400);
	const unsigned int dayOfYear =
		(153 * (month + (month > 2 ? static_cast<unsigned int>(-3) : 9)) + 2) / 5 + day - 1;
	const unsigned int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
	return static_cast<std::int64_t>(era) * 146097 + dayOfEra - 719468;
}

CalendarFields civilFromMilliseconds(std::int64_t milliseconds)
{
	CalendarFields result;
	const std::int64_t seconds = floorDivide(milliseconds, 1000);
	const std::int64_t daysSinceEpoch = floorDivide(seconds, 86400);
	std::int64_t daySeconds = seconds - daysSinceEpoch * 86400;
	std::int64_t days = daysSinceEpoch + 719468;
	const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
	const unsigned int dayOfEra = static_cast<unsigned int>(days - era * 146097);
	const unsigned int yearOfEra =
		(dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
	int year = static_cast<int>(yearOfEra) + static_cast<int>(era) * 400;
	const unsigned int dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
	const unsigned int monthPrime = (5 * dayOfYear + 2) / 153;
	const unsigned int day = dayOfYear - (153 * monthPrime + 2) / 5 + 1;
	const unsigned int month = monthPrime + (monthPrime < 10 ? 3 : static_cast<unsigned int>(-9));
	year += month <= 2;

	result.year = year;
	result.month = static_cast<int>(month);
	result.day = static_cast<int>(day);
	result.hour = static_cast<int>(daySeconds / 3600);
	result.minute = static_cast<int>((daySeconds % 3600) / 60);
	result.second = static_cast<int>(daySeconds % 60);
	result.millisecond = static_cast<int>(milliseconds - seconds * 1000);
	result.weekday = static_cast<int>((daysSinceEpoch + 4) % 7);
	if (result.weekday < 0)
		result.weekday += 7;
	return result;
}

bool isLeapYear(int year)
{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int daysInMonth(int year, int month)
{
	static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	return month == 2 && isLeapYear(year) ? 29 : days[month - 1];
}

void validateCalendarFields(const CalendarFields& value)
{
	if (value.year < 1 || value.year > 9999 || value.month < 1 || value.month > 12 ||
		value.day < 1 || value.day > daysInMonth(value.year, value.month) ||
		value.hour < 0 || value.hour > 23 || value.minute < 0 || value.minute > 59 ||
		value.second < 0 || value.second > 59 || value.millisecond < 0 || value.millisecond > 999)
		throw RBX::runtime_error("DateTime fields are out of range");
}

std::int64_t universalMilliseconds(const CalendarFields& value)
{
	validateCalendarFields(value);
	return daysFromCivil(value.year, static_cast<unsigned int>(value.month),
		static_cast<unsigned int>(value.day)) * 86400000LL + value.hour * 3600000LL +
		value.minute * 60000LL + value.second * 1000LL + value.millisecond;
}

void validateTimestamp(std::int64_t milliseconds)
{
	if (milliseconds < DateTime::MinimumUnixTimestampMillis ||
		milliseconds > DateTime::MaximumUnixTimestampMillis)
		throw RBX::runtime_error("UnixTimestampMillis is outside the supported DateTime range");
}

CalendarFields localFields(std::int64_t milliseconds)
{
	validateTimestamp(milliseconds);
	const std::time_t seconds = static_cast<std::time_t>(floorDivide(milliseconds, 1000));
	std::tm local = {};
#if defined(_WIN32)
	if (localtime_s(&local, &seconds) != 0)
#else
	if (!localtime_r(&seconds, &local))
#endif
		throw RBX::runtime_error("DateTime cannot be represented in local time");
	CalendarFields result;
	result.year = local.tm_year + 1900;
	result.month = local.tm_mon + 1;
	result.day = local.tm_mday;
	result.hour = local.tm_hour;
	result.minute = local.tm_min;
	result.second = local.tm_sec;
	result.millisecond = static_cast<int>(milliseconds - floorDivide(milliseconds, 1000) * 1000);
	result.weekday = local.tm_wday;
	return result;
}

std::int64_t localMilliseconds(const CalendarFields& value)
{
	validateCalendarFields(value);
	std::tm local = {};
	local.tm_year = value.year - 1900;
	local.tm_mon = value.month - 1;
	local.tm_mday = value.day;
	local.tm_hour = value.hour;
	local.tm_min = value.minute;
	local.tm_sec = value.second;
	local.tm_isdst = -1;
	const std::time_t result = std::mktime(&local);
	if (result == static_cast<std::time_t>(-1) || local.tm_year != value.year - 1900 ||
		local.tm_mon != value.month - 1 || local.tm_mday != value.day ||
		local.tm_hour != value.hour || local.tm_min != value.minute || local.tm_sec != value.second)
		throw RBX::runtime_error("DateTime cannot be represented in local time");
	const std::int64_t milliseconds = static_cast<std::int64_t>(result) * 1000 + value.millisecond;
	validateTimestamp(milliseconds);
	return milliseconds;
}

int optionalInteger(lua_State* L, int index, int defaultValue)
{
	return lua_isnoneornil(L, index) ? defaultValue : static_cast<int>(luaL_checkinteger(L, index));
}

CalendarFields fieldsFromArguments(lua_State* L)
{
	CalendarFields result;
	result.year = optionalInteger(L, 1, 1970);
	result.month = optionalInteger(L, 2, 1);
	result.day = optionalInteger(L, 3, 1);
	result.hour = optionalInteger(L, 4, 0);
	result.minute = optionalInteger(L, 5, 0);
	result.second = optionalInteger(L, 6, 0);
	result.millisecond = optionalInteger(L, 7, 0);
	return result;
}

std::string normalizeLocale(std::string locale)
{
	std::transform(locale.begin(), locale.end(), locale.begin(), [](unsigned char character) {
		return character == '_' ? '-' : static_cast<char>(std::tolower(character));
	});
	return locale.empty() ? "en-us" : locale;
}

std::shared_ptr<const DateTimeLocale> loadLocale(const std::string& requestedLocale)
{
	static std::mutex mutex;
	static std::unordered_map<std::string, std::shared_ptr<const DateTimeLocale> > cache;
	const std::string locale = normalizeLocale(requestedLocale);
	std::lock_guard<std::mutex> lock(mutex);
	const auto existing = cache.find(locale);
	if (existing != cache.end())
		return existing->second;

	auto read = [](const std::string& name) -> std::shared_ptr<DateTimeLocale> {
		const std::optional<Assets::ResolvedAsset> asset = Assets::assetMountTable().resolve(
			"rbxasset://configs/DateTimeLocaleConfigs/" + name + ".json");
		if (!asset)
			return std::shared_ptr<DateTimeLocale>();
		std::ifstream input(asset->path, std::ios::binary);
		if (!input)
			return std::shared_ptr<DateTimeLocale>();
		const std::string json((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
		rapidjson::Document document;
		document.Parse<rapidjson::kParseDefaultFlags>(json.c_str());
		if (document.HasParseError() || !document.IsObject())
			throw RBX::runtime_error("Invalid DateTime locale config for %s", name.c_str());
		std::shared_ptr<DateTimeLocale> result(new DateTimeLocale());
		auto copyArray = [&](const char* key, auto& destination) {
			if (!document.HasMember(key) || !document[key].IsArray() ||
				document[key].Size() != destination.size())
				throw RBX::runtime_error("DateTime locale %s has invalid %s", name.c_str(), key);
			const rapidjson::Value& values = document[key];
			for (rapidjson::SizeType index = 0; index < values.Size(); ++index)
			{
				if (!values[index].IsString())
					throw RBX::runtime_error("DateTime locale %s has non-string %s", name.c_str(), key);
				destination[index].assign(values[index].GetString(), values[index].GetStringLength());
			}
		};
		copyArray("months", result->months);
		copyArray("monthsShort", result->monthsShort);
		copyArray("weekdays", result->weekdays);
		copyArray("weekdaysShort", result->weekdaysShort);
		copyArray("weekdaysMin", result->weekdaysMin);
		if (!document.HasMember("longDateFormat") || !document["longDateFormat"].IsObject())
			throw RBX::runtime_error("DateTime locale %s has no longDateFormat", name.c_str());
		const rapidjson::Value& formats = document["longDateFormat"];
		for (rapidjson::Value::ConstMemberIterator member = formats.MemberBegin(); member != formats.MemberEnd(); ++member)
			if (member->name.IsString() && member->value.IsString())
				result->longDateFormat.emplace(
					std::string(member->name.GetString(), member->name.GetStringLength()),
					std::string(member->value.GetString(), member->value.GetStringLength()));
		if (document.HasMember("meridiem") && document["meridiem"].IsArray())
		{
			const rapidjson::Value& meridiem = document["meridiem"];
			for (rapidjson::SizeType index = 0; index < meridiem.Size(); ++index)
			{
				const rapidjson::Value& item = meridiem[index];
				if (item.IsObject() && item.HasMember("startFrom") && item["startFrom"].IsInt() &&
					item.HasMember("lowerCase") && item["lowerCase"].IsString() &&
					item.HasMember("upperCase") && item["upperCase"].IsString())
					result->meridiem.push_back({ item["startFrom"].GetInt(),
						item["lowerCase"].GetString(), item["upperCase"].GetString() });
			}
		}
		return result;
	};

	std::shared_ptr<DateTimeLocale> loaded;
	try
	{
		loaded = read(locale);
	}
	catch (const std::exception&)
	{
		if (locale == "en-us")
			throw;
	}
	if (!loaded && locale.find('-') != std::string::npos)
	{
		const std::string language = locale.substr(0, locale.find('-'));
		static const std::unordered_map<std::string, std::string> defaults = {
			{ "ar", "ar-001" }, { "de", "de-de" }, { "en", "en-us" },
			{ "es", "es-es" }, { "fr", "fr-fr" }, { "it", "it-it" },
			{ "ja", "ja-jp" }, { "ko", "ko-kr" }, { "pt", "pt-br" },
			{ "ru", "ru-ru" }, { "zh", "zh-cn" }
		};
		const auto mapped = defaults.find(language);
		if (mapped != defaults.end())
			loaded = read(mapped->second);
	}
	if (!loaded)
		loaded = read("en-us");
	if (!loaded)
		throw RBX::runtime_error("The en-us DateTime locale config is unavailable");
	cache[locale] = loaded;
	return loaded;
}

std::string padded(int value, int width)
{
	std::ostringstream stream;
	stream << std::setfill('0') << std::setw(width) << value;
	return stream.str();
}

std::string meridiemFor(const DateTimeLocale& locale, const CalendarFields& fields, bool upper)
{
	const int time = fields.hour * 100 + fields.minute;
	const DateTimeLocale::Meridiem* selected = nullptr;
	for (const DateTimeLocale::Meridiem& candidate : locale.meridiem)
		if (candidate.startFrom <= time && (!selected || candidate.startFrom >= selected->startFrom))
			selected = &candidate;
	if (!selected)
		return upper ? (fields.hour < 12 ? "AM" : "PM") : (fields.hour < 12 ? "am" : "pm");
	return upper ? selected->upperCase : selected->lowerCase;
}

std::string formatDateTime(const CalendarFields& fields, std::string format,
	const DateTimeLocale& locale, std::int64_t milliseconds)
{
	const auto composite = locale.longDateFormat.find(format);
	if (composite != locale.longDateFormat.end())
		format = composite->second;
	static const std::array<const char*, 25> tokens = { "YYYY", "MMMM", "dddd", "MMM", "ddd",
		"SSS", "YY", "MM", "DD", "HH", "hh", "mm", "ss", "dd", "M", "D", "H", "h",
		"m", "s", "d", "A", "a", "X", "x" };
	std::string result;
	for (std::size_t index = 0; index < format.size();)
	{
		if (format[index] == '[')
		{
			const std::size_t end = format.find(']', index + 1);
			if (end == std::string::npos)
				throw RBX::runtime_error("Unterminated DateTime format literal");
			result.append(format, index + 1, end - index - 1);
			index = end + 1;
			continue;
		}
		const char* matched = nullptr;
		for (const char* token : tokens)
			if (format.compare(index, std::strlen(token), token) == 0)
			{
				matched = token;
				break;
			}
		if (!matched)
		{
			result.push_back(format[index++]);
			continue;
		}
		const std::string token(matched);
		if (token == "YYYY") result += padded(fields.year, 4);
		else if (token == "YY") result += padded(fields.year % 100, 2);
		else if (token == "MMMM") result += locale.months[fields.month - 1];
		else if (token == "MMM") result += locale.monthsShort[fields.month - 1];
		else if (token == "MM") result += padded(fields.month, 2);
		else if (token == "M") result += std::to_string(fields.month);
		else if (token == "DD") result += padded(fields.day, 2);
		else if (token == "D") result += std::to_string(fields.day);
		else if (token == "dddd") result += locale.weekdays[fields.weekday];
		else if (token == "ddd") result += locale.weekdaysShort[fields.weekday];
		else if (token == "dd") result += locale.weekdaysMin[fields.weekday];
		else if (token == "d") result += std::to_string(fields.weekday);
		else if (token == "HH") result += padded(fields.hour, 2);
		else if (token == "H") result += std::to_string(fields.hour);
		else if (token == "hh") result += padded(fields.hour % 12 == 0 ? 12 : fields.hour % 12, 2);
		else if (token == "h") result += std::to_string(fields.hour % 12 == 0 ? 12 : fields.hour % 12);
		else if (token == "mm") result += padded(fields.minute, 2);
		else if (token == "m") result += std::to_string(fields.minute);
		else if (token == "ss") result += padded(fields.second, 2);
		else if (token == "s") result += std::to_string(fields.second);
		else if (token == "SSS") result += padded(fields.millisecond, 3);
		else if (token == "A") result += meridiemFor(locale, fields, true);
		else if (token == "a") result += meridiemFor(locale, fields, false);
		else if (token == "X") result += std::to_string(floorDivide(milliseconds, 1000));
		else if (token == "x") result += std::to_string(milliseconds);
		index += token.size();
	}
	return result;
}

std::string isoDate(std::int64_t milliseconds)
{
	const CalendarFields fields = civilFromMilliseconds(milliseconds);
	return padded(fields.year, 4) + "-" + padded(fields.month, 2) + "-" +
		padded(fields.day, 2) + "T" + padded(fields.hour, 2) + ":" +
		padded(fields.minute, 2) + ":" + padded(fields.second, 2) + "." +
		padded(fields.millisecond, 3) + "Z";
}

void pushCalendarFields(lua_State* L, const CalendarFields& fields)
{
	lua_createtable(L, 0, 7);
	auto set = [&](const char* name, int value) {
		lua_pushinteger(L, value);
		lua_setfield(L, -2, name);
	};
	set("Year", fields.year);
	set("Month", fields.month);
	set("Day", fields.day);
	set("Hour", fields.hour);
	set("Minute", fields.minute);
	set("Second", fields.second);
	set("Millisecond", fields.millisecond);
}

} // namespace

const luaL_reg DateTimeBridge::classLibrary[] = {
	{ "now", now },
	{ "fromUnixTimestamp", fromUnixTimestamp },
	{ "fromUnixTimestampMillis", fromUnixTimestampMillis },
	{ "fromUniversalTime", fromUniversalTime },
	{ "fromLocalTime", fromLocalTime },
	{ "fromIsoDate", fromIsoDate },
	{ NULL, NULL }
};

void DateTimeBridge::registerClassLibrary(lua_State* L)
{
	luaL_register(L, className, classLibrary);
	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

int DateTimeBridge::now(lua_State* L)
{
	const auto value = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
	pushNewObject(L, DateTime(value));
	return 1;
}

int DateTimeBridge::fromUnixTimestamp(lua_State* L)
{
	const double seconds = luaL_checknumber(L, 1);
	if (!std::isfinite(seconds))
		throw RBX::runtime_error("UnixTimestamp must be finite");
	const long double milliseconds = static_cast<long double>(seconds) * 1000.0L;
	if (milliseconds < DateTime::MinimumUnixTimestampMillis ||
		milliseconds > DateTime::MaximumUnixTimestampMillis)
		throw RBX::runtime_error("UnixTimestamp is outside the supported DateTime range");
	pushNewObject(L, DateTime(static_cast<std::int64_t>(std::floor(milliseconds))));
	return 1;
}

int DateTimeBridge::fromUnixTimestampMillis(lua_State* L)
{
	const double input = luaL_checknumber(L, 1);
	if (!std::isfinite(input) || input < DateTime::MinimumUnixTimestampMillis ||
		input > DateTime::MaximumUnixTimestampMillis)
		throw RBX::runtime_error("UnixTimestampMillis is outside the supported DateTime range");
	const std::int64_t value = static_cast<std::int64_t>(std::floor(input));
	validateTimestamp(value);
	pushNewObject(L, DateTime(value));
	return 1;
}

int DateTimeBridge::fromUniversalTime(lua_State* L)
{
	const std::int64_t value = universalMilliseconds(fieldsFromArguments(L));
	validateTimestamp(value);
	pushNewObject(L, DateTime(value));
	return 1;
}

int DateTimeBridge::fromLocalTime(lua_State* L)
{
	pushNewObject(L, DateTime(localMilliseconds(fieldsFromArguments(L))));
	return 1;
}

int DateTimeBridge::fromIsoDate(lua_State* L)
{
	const std::string text = luaL_checkstring(L, 1);
	static const std::regex pattern(
		R"(^(\d{4})-(\d{2})-(\d{2})[Tt](\d{2}):(\d{2}):(\d{2})(?:\.(\d{1,9}))?([Zz]|[+-]\d{2}:?\d{2})$)");
	std::smatch match;
	if (!std::regex_match(text, match, pattern))
	{
		lua_pushnil(L);
		return 1;
	}
	CalendarFields fields;
	fields.year = std::stoi(match[1].str());
	fields.month = std::stoi(match[2].str());
	fields.day = std::stoi(match[3].str());
	fields.hour = std::stoi(match[4].str());
	fields.minute = std::stoi(match[5].str());
	fields.second = std::stoi(match[6].str());
	if (match[7].matched)
	{
		std::string fraction = match[7].str();
		fraction.resize(3, '0');
		fields.millisecond = std::stoi(fraction.substr(0, 3));
	}
	std::int64_t value;
	try
	{
		value = universalMilliseconds(fields);
	}
	catch (const std::exception&)
	{
		lua_pushnil(L);
		return 1;
	}
	const std::string zone = match[8].str();
	if (zone != "Z" && zone != "z")
	{
		const int hours = std::stoi(zone.substr(1, 2));
		const int minutes = std::stoi(zone.substr(zone.size() - 2));
		if (hours > 23 || minutes > 59)
		{
			lua_pushnil(L);
			return 1;
		}
		const std::int64_t offset = (hours * 60LL + minutes) * 60000LL;
		value += zone[0] == '+' ? -offset : offset;
	}
	if (value < DateTime::MinimumUnixTimestampMillis || value > DateTime::MaximumUnixTimestampMillis)
	{
		lua_pushnil(L);
		return 1;
	}
	pushNewObject(L, DateTime(value));
	return 1;
}

int DateTimeBridge::toUniversalTime(lua_State* L)
{
	pushCalendarFields(L, civilFromMilliseconds(getObject(L, 1).getUnixTimestampMillis()));
	return 1;
}

int DateTimeBridge::toLocalTime(lua_State* L)
{
	pushCalendarFields(L, localFields(getObject(L, 1).getUnixTimestampMillis()));
	return 1;
}

int DateTimeBridge::toIsoDate(lua_State* L)
{
	const std::string value = isoDate(getObject(L, 1).getUnixTimestampMillis());
	lua_pushlstring(L, value.data(), value.size());
	return 1;
}

int DateTimeBridge::formatUniversalTime(lua_State* L)
{
	const DateTime& value = getObject(L, 1);
	const std::string format = luaL_checkstring(L, 2);
	const std::string locale = luaL_checkstring(L, 3);
	const std::string result = formatDateTime(civilFromMilliseconds(value.getUnixTimestampMillis()),
		format, *loadLocale(locale), value.getUnixTimestampMillis());
	lua_pushlstring(L, result.data(), result.size());
	return 1;
}

int DateTimeBridge::formatLocalTime(lua_State* L)
{
	const DateTime& value = getObject(L, 1);
	const std::string format = luaL_checkstring(L, 2);
	const std::string locale = luaL_checkstring(L, 3);
	const std::string result = formatDateTime(localFields(value.getUnixTimestampMillis()),
		format, *loadLocale(locale), value.getUnixTimestampMillis());
	lua_pushlstring(L, result.data(), result.size());
	return 1;
}

template<>
int Bridge<RBX::DateTime>::on_index(const RBX::DateTime& value, const char* name, lua_State* L)
{
	if (strcmp(name, "UnixTimestamp") == 0)
		return lua_pushnumber(L, static_cast<double>(floorDivide(value.getUnixTimestampMillis(), 1000))), 1;
	if (strcmp(name, "UnixTimestampMillis") == 0)
		return lua_pushnumber(L, static_cast<double>(value.getUnixTimestampMillis())), 1;
	if (strcmp(name, "ToUniversalTime") == 0)
		return lua_pushcfunction(L, DateTimeBridge::toUniversalTime), 1;
	if (strcmp(name, "ToLocalTime") == 0)
		return lua_pushcfunction(L, DateTimeBridge::toLocalTime), 1;
	if (strcmp(name, "ToIsoDate") == 0)
		return lua_pushcfunction(L, DateTimeBridge::toIsoDate), 1;
	if (strcmp(name, "FormatUniversalTime") == 0)
		return lua_pushcfunction(L, DateTimeBridge::formatUniversalTime), 1;
	if (strcmp(name, "FormatLocalTime") == 0)
		return lua_pushcfunction(L, DateTimeBridge::formatLocalTime), 1;
	throw RBX::runtime_error("'%s' is not a valid member of DateTime", name);
}

template<>
void Bridge<RBX::DateTime>::on_newindex(RBX::DateTime&, const char* name, lua_State*)
{
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
int Bridge<RBX::DateTime>::on_tostring(const RBX::DateTime& value, lua_State* L)
{
	const std::string result = isoDate(value.getUnixTimestampMillis());
	lua_pushlstring(L, result.data(), result.size());
	return 1;
}

const luaL_reg Path2DControlPointBridge::classLibrary[] = {
	{"new", newPath2DControlPoint},
	{NULL, NULL}
};

void Path2DControlPointBridge::registerClassLibrary(lua_State* L)
{
	luaL_register(L, className, classLibrary);
	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

int Path2DControlPointBridge::newPath2DControlPoint(lua_State* L)
{
	const int count = lua_gettop(L);
	if (count == 0)
	{
		pushNewObject(L, Path2DControlPoint());
		return 1;
	}
	if (count == 1)
	{
		pushNewObject(L, Path2DControlPoint(UDim2Bridge::getObject(L, 1)));
		return 1;
	}
	if (count == 3)
	{
		pushNewObject(L, Path2DControlPoint(UDim2Bridge::getObject(L, 1),
			UDim2Bridge::getObject(L, 2), UDim2Bridge::getObject(L, 3)));
		return 1;
	}
	throw RBX::runtime_error("Trying to create a Path2DControlPoint with wrong number of arguments");
}

template<>
int Bridge<RBX::Path2DControlPoint>::on_index(
	const RBX::Path2DControlPoint& point, const char* name, lua_State* L)
{
	if (strcmp(name, "Position") == 0)
		return UDim2Bridge::pushNewObject(L, point.position), 1;
	if (strcmp(name, "LeftTangent") == 0)
		return UDim2Bridge::pushNewObject(L, point.leftTangent), 1;
	if (strcmp(name, "RightTangent") == 0)
		return UDim2Bridge::pushNewObject(L, point.rightTangent), 1;
	throw RBX::runtime_error("'%s' is not a valid member of Path2DControlPoint", name);
}

template<>
void Bridge<RBX::Path2DControlPoint>::on_newindex(
	RBX::Path2DControlPoint&, const char* name, lua_State*)
{
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
int Bridge<RBX::Path2DControlPoint>::on_tostring(
	const RBX::Path2DControlPoint& point, lua_State* L)
{
	const UDim2& p = point.position;
	lua_pushfstring(L, "Path2DControlPoint { Position = {%f, %d}, {%f, %d} }",
		p.x.scale, p.x.offset, p.y.scale, p.y.offset);
	return 1;
}

const luaL_reg ContentBridge::classLibrary[] = {
	{"fromUri", fromUri},
	{"fromAssetId", fromAssetId},
	{"fromObject", fromObject},
	{NULL, NULL}
};

void ContentBridge::registerClassLibrary(lua_State* L)
{
	luaL_register(L, className, classLibrary);
	pushNewObject(L, Content());
	lua_setfield(L, -2, "none");
	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

int ContentBridge::fromUri(lua_State* L)
{
	pushNewObject(L, Content::fromUri(throwable_lua_tostring(L, 1)));
	return 1;
}

int ContentBridge::fromAssetId(lua_State* L)
{
	const double value = luaL_checknumber(L, 1);
	if (!std::isfinite(value))
		throw std::runtime_error("assetId must be finite");
	if (value < 0.0 || value > static_cast<double>(std::numeric_limits<std::int64_t>::max()) ||
		std::floor(value) != value)
		throw std::runtime_error("assetId must be a non-negative integer");
	pushNewObject(L, Content::fromAssetId(static_cast<std::int64_t>(value)));
	return 1;
}

int ContentBridge::fromObject(lua_State* L)
{
	shared_ptr<Reflection::DescribedBase> object;
	if (!ObjectBridge::getPtr(L, 1, object) &&
		!ValueObjectBridge::getPtr(L, 1, object))
		throw std::runtime_error("object must be an Object");
	if (!object)
		throw std::runtime_error("object must not be nil");
	pushNewObject(L, Content::fromObject(object));
	return 1;
}

template<>
int Bridge<RBX::Content>::on_index(const RBX::Content& content, const char* name, lua_State* L)
{
	if (strcmp(name, "SourceType") == 0)
	{
		const Reflection::EnumDescriptor::Item* item =
			Reflection::EnumDesc<ContentSourceType>::singleton().convertToItem(content.getSourceType());
		EnumItem::push(L, item);
		return 1;
	}
	if (strcmp(name, "Uri") == 0)
	{
		if (content.getSourceType() == CONTENT_SOURCE_URI)
			lua_pushlstring(L, content.getUri().data(), content.getUri().size());
		else
			lua_pushnil(L);
		return 1;
	}
	if (strcmp(name, "Object") == 0)
	{
		if (content.getSourceType() == CONTENT_SOURCE_OBJECT)
		{
			const shared_ptr<Instance> instance =
				boost::dynamic_pointer_cast<Instance>(content.getObject());
			if (instance)
				ObjectBridge::push(L, instance);
			else
				ValueObjectBridge::push(L, content.getObject());
		}
		else
			lua_pushnil(L);
		return 1;
	}
	if (strcmp(name, "Opaque") == 0)
	{
		lua_pushnil(L);
		return 1;
	}
	throw RBX::runtime_error("%s is not a valid member of Content", name);
}

template<>
void Bridge<RBX::Content>::on_newindex(RBX::Content&, const char* name, lua_State*)
{
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
int Bridge<RBX::Content>::on_tostring(const RBX::Content& content, lua_State* L)
{
	if (content.getSourceType() == CONTENT_SOURCE_URI)
		lua_pushfstring(L, "Content(%s)", content.getUri().c_str());
	else
		lua_pushstring(L, content.empty() ? "Content(none)" : "Content(object)");
	return 1;
}

const luaL_reg FontBridge::classLibrary[] = {
	{"new", newFont},
	{"fromEnum", fromEnum},
	{NULL, NULL}
};

void FontBridge::registerClassLibrary(lua_State* L)
{
	luaL_register(L, className, classLibrary);
	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

namespace {
template<typename Enum>
Enum optionalFontEnum(lua_State* L, int index, Enum defaultValue, const char* expected)
{
	if (lua_isnoneornil(L, index))
		return defaultValue;
	EnumDescriptorItemPtr item;
	if (!EnumItem::getItem(L, static_cast<unsigned int>(index), item) || !item->owner.isType<Enum>())
		throw RBX::runtime_error("Font.new expects %s for argument %d", expected, index);
	return static_cast<Enum>(item->value);
}
}

int FontBridge::newFont(lua_State* L)
{
	const std::string family = throwable_lua_tostring(L, 1);
	const FontWeight weight = optionalFontEnum(
		L, 2, FONT_WEIGHT_REGULAR, "Enum.FontWeight");
	const FontStyle style = optionalFontEnum(
		L, 3, FONT_STYLE_NORMAL, "Enum.FontStyle");
	pushNewObject(L, Font(family, weight, style));
	return 1;
}

int FontBridge::fromEnum(lua_State* L)
{
	EnumDescriptorItemPtr item;
	if (!EnumItem::getItem(L, 1, item) || !item->owner.isType<TextService::Font>())
		throw RBX::runtime_error("Font.fromEnum expects Enum.Font");
	pushNewObject(L, Font::fromEnum(item->value));
	return 1;
}

template<>
int Bridge<RBX::Font>::on_index(const RBX::Font& font, const char* name, lua_State* L)
{
	if (strcmp(name, "Family") == 0)
	{
		lua_pushlstring(L, font.getFamily().data(), font.getFamily().size());
		return 1;
	}
	if (strcmp(name, "Weight") == 0)
	{
		EnumItem::push(L, Reflection::EnumDesc<FontWeight>::singleton().convertToItem(font.getWeight()));
		return 1;
	}
	if (strcmp(name, "Style") == 0)
	{
		EnumItem::push(L, Reflection::EnumDesc<FontStyle>::singleton().convertToItem(font.getStyle()));
		return 1;
	}
	if (strcmp(name, "Bold") == 0)
	{
		lua_pushboolean(L, font.getBold());
		return 1;
	}
	throw RBX::runtime_error("%s is not a valid member of Font", name);
}

template<>
void Bridge<RBX::Font>::on_newindex(RBX::Font&, const char* name, lua_State*)
{
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
int Bridge<RBX::Font>::on_tostring(const RBX::Font& font, lua_State* L)
{
	lua_pushfstring(L, "Font { Family = %s, Weight = %d, Style = %d }",
		font.getFamily().c_str(), static_cast<int>(font.getWeight()), static_cast<int>(font.getStyle()));
	return 1;
}

const char* safe_lua_tostring(lua_State *L, int idx)
{
	static const char* empty = "";
	const char* s = lua_tostring(L, idx);
	return s ? s : empty;
}

const char* throwable_lua_tostring(lua_State *L, int idx)
{
	const char* s = luaL_checkstring(L, idx);
	
	// keep this in sync with string length limits imposed in networking code
	static const size_t kMaxStringLength = 200000;

	if (strlen(s) >= kMaxStringLength) {
		throw std::runtime_error("String too long");
	}

	return s;
}

const char* lua_checkstring_secure(lua_State* L, int idx)
{
#if defined(RBX_LUAU_VM)
    const char* s = lua_tolstring(L, idx, NULL);
    if (!s)
        luaL_typeerror(L, idx, lua_typename(L, LUA_TSTRING));
#else
    const char* s = lua_tolstringsecure(L, idx, NULL);
    if (!s) luaL_typerror(L, idx, lua_typename(L, LUA_TSTRING));
#endif
    return s;
}

void lua_resetstack(lua_State* L, int idx)
{
	RBXASSERT(idx >= 0 && idx <= lua_gettop(L));

    if (idx < lua_gettop(L))
#if !defined(RBX_LUAU_VM)
		luaF_close(L, L->base + idx);
#endif

    lua_settop(L, idx);
}

float lua_tofloat(lua_State *L, int idx)
{
	double value = lua_tonumber(L, idx);

	if (value==std::numeric_limits<double>::infinity())
		return std::numeric_limits<float>::infinity();

	if (value==-std::numeric_limits<double>::infinity())
		return -std::numeric_limits<float>::infinity();

	// NAN:
	if (!((value < 0.0) || (value >= 0.0)))
		return (float) value;

	if (value > (double)std::numeric_limits<float>::max())
		return std::numeric_limits<float>::max();

	if (value < (double)-std::numeric_limits<float>::max())
		return -std::numeric_limits<float>::max();

	return (float) value;
}


/// G3D::Color3 has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
/// G3D::Color3 has a default implementation for registerClass() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<G3D::Color3>::className("Color3");

const luaL_reg Color3Bridge::classLibrary[] = {
	{"new", newColor3},
	{"fromRGB", newRGBColor3},
	{"fromHSV", newHSVColor3},
	{"fromHex", newHexColor3},
	{"toHSV", toHSV},
	{NULL, NULL}
};
    
void Color3Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

void Color3Bridge::pushColor3(lua_State *L, const G3D::Color3& color)
{
    pushNewObject(L, color);
}
    
int Color3Bridge::newColor3(lua_State *L)
{
	float color[3];

	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));
	for (int i = 0; i<count; i++)
		color[i] = lua_tofloat(L, i+1);
	for (int i = count; i<3; i++)
		color[i] = 0.0;
	pushNewObject(L, color);
	return 1;
}

int Color3Bridge::newRGBColor3(lua_State *L)
{
	float color[3];

	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));
	for (int i = 0; i<count; i++)
		color[i] = lua_tofloat(L, i+1) / 255;
	for (int i = count; i<3; i++)
		color[i] = 0.0;
	pushNewObject(L, color);
	return 1;
}

int Color3Bridge::newHSVColor3(lua_State* L)
{
	const float hue = lua_tofloat(L, 1);
	const float saturation = lua_tofloat(L, 2);
	const float value = lua_tofloat(L, 3);
	if (!std::isfinite(hue) || !std::isfinite(saturation) || !std::isfinite(value))
		throw RBX::runtime_error("Color3.fromHSV arguments must be finite");
	pushNewObject(L, G3D::Color3::fromHSV(G3D::Vector3(
		std::clamp(hue, 0.0f, 1.0f), std::clamp(saturation, 0.0f, 1.0f),
		std::clamp(value, 0.0f, 1.0f))));
	return 1;
}

int Color3Bridge::newHexColor3(lua_State* L)
{
	if (lua_gettop(L) != 1)
		throw RBX::runtime_error("Color3.fromHex requires one argument");
	std::string text = luaL_checkstring(L, 1);
	if (!text.empty() && text[0] == '#')
		text.erase(text.begin());
	if ((text.size() != 3 && text.size() != 6) || !std::all_of(text.begin(), text.end(), [](unsigned char value) {
		return std::isxdigit(value) != 0;
	}))
		throw RBX::runtime_error("Invalid hex color: %s", text.c_str());
	if (text.size() == 3)
	{
		std::string expanded;
		expanded.reserve(6);
		for (std::string::const_iterator it = text.begin(); it != text.end(); ++it)
		{
			expanded.push_back(*it);
			expanded.push_back(*it);
		}
		text.swap(expanded);
	}
	const unsigned long value = std::stoul(text, nullptr, 16);
	pushNewObject(L, G3D::Color3(
		static_cast<float>((value >> 16) & 0xff) / 255.0f,
		static_cast<float>((value >> 8) & 0xff) / 255.0f,
		static_cast<float>(value & 0xff) / 255.0f));
	return 1;
}

int Color3Bridge::lerp(lua_State* L)
{
	const G3D::Color3& value = getObject(L, 1);
	const G3D::Color3& target = getObject(L, 2);
	const float alpha = lua_tofloat(L, 3);
	pushNewObject(L, value.lerp(target, alpha));
	return 1;
}

int Color3Bridge::toHSV(lua_State* L)
{
	const G3D::Vector3 value = G3D::Color3::toHSV(getObject(L, 1));
	lua_pushnumber(L, value.x);
	lua_pushnumber(L, value.y);
	lua_pushnumber(L, value.z);
	return 3;
}

int Color3Bridge::toHex(lua_State* L)
{
	const G3D::Color3& value = getObject(L, 1);
	auto channel = [](float input) {
		return static_cast<unsigned int>(std::lround(std::clamp(input, 0.0f, 1.0f) * 255.0f));
	};
	char result[7];
	std::snprintf(result, sizeof(result), "%02X%02X%02X",
		channel(value.r), channel(value.g), channel(value.b));
	lua_pushlstring(L, result, 6);
	return 1;
}

template<>
int Bridge<G3D::Color3>::on_index(const G3D::Color3& object, const char* name, lua_State *L)
{
	if (strcmp(name,"r")==0 || strcmp(name, "R") == 0)
	{
		lua_pushnumber(L, object.r);
		return 1;
	}
	if (strcmp(name,"g")==0 || strcmp(name, "G") == 0)
	{
		lua_pushnumber(L, object.g);
		return 1;
	}
	if (strcmp(name,"b")==0 || strcmp(name, "B") == 0)
	{
		lua_pushnumber(L, object.b);
		return 1;
	}
	if (strcmp(name, "Lerp") == 0 || strcmp(name, "lerp") == 0)
		return lua_pushcfunction(L, Color3Bridge::lerp), 1;
	if (strcmp(name, "ToHSV") == 0 || strcmp(name, "toHSV") == 0)
		return lua_pushcfunction(L, Color3Bridge::toHSV), 1;
	if (strcmp(name, "ToHex") == 0)
		return lua_pushcfunction(L, Color3Bridge::toHex), 1;

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<G3D::Color3>::on_newindex(G3D::Color3& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// RBX::RbxRay has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
/// RBX::RbxRay has a default implementation for registerClass() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::RbxRay>::className("Ray");

const luaL_reg RbxRayBridge::classLibrary[] = {
	{"new", newRbxRay},
	{NULL, NULL}
};

void RbxRayBridge::registerClassLibrary (lua_State *L) {
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int RbxRayBridge::newRbxRay(lua_State *L)
{
	Vector3 origin;
	Vector3 direction;

	// There should be up to 2 Vector3 objects. Following Lua conventions ignore others and use 0 for missing
	int count = std::min(2,lua_gettop(L));

	//First extract the Origin
	if (count >= 1)
	{
		origin = Vector3Bridge::getObject(L, 1);
	}
	if (count >= 2)
	{
		direction = Vector3Bridge::getObject(L, 2);
	}
	pushNewObject(L, RBX::RbxRay::fromOriginAndDirection(origin, direction));

	return 1;
}

static int closestPointVector3(lua_State *L)
{
	RBX::RbxRay& self	= Bridge<RBX::RbxRay>::getObject(L, 1);
	G3D::Vector3& point = Bridge<G3D::Vector3>::getObject(L, 2);
	Bridge<G3D::Vector3>::pushNewObject(L, self.closestPoint(point));
	return 1;
}

static int distanceVector3(lua_State *L)
{
	RBX::RbxRay& self	= Bridge<RBX::RbxRay>::getObject(L, 1);
	G3D::Vector3& point = Bridge<G3D::Vector3>::getObject(L, 2);
	lua_pushnumber(L, self.distance(point));
	return 1;
}

//Lower case are legacy
template<>
int Bridge<RBX::RbxRay>::on_index(const RBX::RbxRay& object, const char* name, lua_State *L)
{
	if (strcmp(name,"Origin")==0)
	{
		Bridge<G3D::Vector3>::pushNewObject(L, object.origin());
		return 1;
	}
	if (strcmp(name,"Direction")==0)
	{
    		Bridge<G3D::Vector3>::pushNewObject(L, object.direction());
		return 1;
	}
	if (strcmp(name,"unit")==0 || strcmp(name,"Unit")==0)
	{
		Bridge<RBX::RbxRay>::pushNewObject(L, object.unit());
		return 1;
	}
	if (strcmp(name,"ClosestPoint")==0)
	{
		lua_pushcfunction(L, closestPointVector3);
		return 1;
	}
	if (strcmp(name,"Distance")==0)
	{
		lua_pushcfunction(L, distanceVector3);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::RbxRay>::on_newindex(RBX::RbxRay& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}



/// Region3 has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::Region3>::className("Region3");

const luaL_reg Region3Bridge::classLibrary[] = {
	{"new", newRegion3},
	{NULL, NULL}
};

void Region3Bridge::registerClassLibrary (lua_State *L) {
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Region3Bridge::newRegion3(lua_State *L)
{
	Vector3 vector[2];
	// There should be up to 2 vector3 parameters (min,max). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(2,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = Vector3Bridge::getObject(L, i+1);
	for (int i = count; i<2; i++)
		vector[i] = Vector3();
	pushNewObject(L, Region3(vector[0], vector[1]));
	return 1;
}

int Region3Bridge::expandToGrid(lua_State *L)
{
	Region3 region = Region3Bridge::getObject(L, 1);

	if (2 > lua_gettop(L))
        throw std::runtime_error("Argument 1 missing or nil");

	float resolution = lua_tofloat(L, 2);

	if (Math::isNanInf(resolution) || resolution <= 0)
		throw std::runtime_error("Resolution has to be a positive number");

	Vector3 min = region.minPos() / resolution;
	Vector3 max = region.maxPos() / resolution;

	Vector3 emin(floorf(min.x) * resolution, floorf(min.y) * resolution, floorf(min.z) * resolution);
	Vector3 emax(ceilf(max.x) * resolution, ceilf(max.y) * resolution, ceilf(max.z) * resolution);

	Region3Bridge::pushNewObject(L, Region3(emin, emax));
	return 1;
}

//Lower case are legacy
template<>
int Bridge<RBX::Region3>::on_index(const RBX::Region3& object, const char* name, lua_State *L)
{
	if (strcmp(name,"CFrame")==0)
	{
		CoordinateFrameBridge::pushCoordinateFrame(L, object.getCFrame());
		return 1;
	}
	if (strcmp(name,"Size")==0)
	{
		Vector3Bridge::pushVector3(L, object.getSize());
		return 1;
	}
	if (strcmp(name,"ExpandToGrid")==0)
	{
		lua_pushcfunction(L, Region3Bridge::expandToGrid);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::Region3>::on_newindex(RBX::Region3& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
const char* Bridge<RBX::Region3int16>::className("Region3int16");

const luaL_reg Region3int16Bridge::classLibrary[] = {
	{"new", newRegion3int16},
	{NULL, NULL}
};

void Region3int16Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Region3int16Bridge::newRegion3int16(lua_State *L)
{
	Vector3int16 vector[2];
	// There should be up to 2 vector3int16 parameters (min,max). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(2,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = Vector3int16Bridge::getObject(L, i+1);
	for (int i = count; i<2; i++)
		vector[i] = Vector3int16();
	pushNewObject(L, Region3int16(vector[0], vector[1]));
	return 1;
}

//Lower case are legacy
template<>
int Bridge<RBX::Region3int16>::on_index(const RBX::Region3int16& object, const char* name, lua_State *L)
{
	if (strcmp(name,"Min")==0)
	{
		Vector3int16Bridge::pushVector3int16(L, object.getMinPos());
		return 1;
	}
	if (strcmp(name,"Max")==0)
	{
		Vector3int16Bridge::pushVector3int16(L, object.getMaxPos());
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::Region3int16>::on_newindex(RBX::Region3int16& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}


// PhysicalProperties class implementation
template<>
const char* Bridge<PhysicalProperties>::className("PhysicalProperties");

const luaL_reg PhysicalPropertiesBridge::classLibrary[] =
{
	{"new", newPhysicalProperties},
	{NULL, NULL}
};
void PhysicalPropertiesBridge::registerClassLibrary(lua_State *L)
{
	// Register "new" function
	luaL_register(L, className, classLibrary);
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);	// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int PhysicalPropertiesBridge::newPhysicalProperties(lua_State *L)
{
	PhysicalProperties properties;
	// 1 Enum.Material;
	// 3 numbers;  density, friction, elasticity
	// 5 numbers;  density, friction, elasticity, frictionWeight, elasticityWeight
	const int count = lua_gettop(L);
	if (FFlag::PhysPropConstructFromMaterial && count == 1)
	{
		EnumDescriptorItemPtr item;
		if (EnumItem::getItem(L, 1, item))
		{
			if(!item->owner.isType<RBX::PartMaterial>())
				throw RBX::runtime_error("PhysicalProperties.new with 1 argument expects Enum.Material inputs");

			RBX::PartMaterial partMaterial = (RBX::PartMaterial)item->value;
			properties = MaterialProperties::generatePhysicalMaterialFromPartMaterial(partMaterial);
		}
	}
	else if (count == 3)
	{
		properties = PhysicalProperties(lua_tofloat(L, 1), lua_tofloat(L, 2), lua_tofloat(L, 3));
	}
	else if (count == 5)
	{
		properties = PhysicalProperties(lua_tofloat(L, 1), lua_tofloat(L, 2), lua_tofloat(L, 3), lua_tofloat(L, 4), lua_tofloat(L, 5));
	}
	else
	{
		if (FFlag::PhysPropConstructFromMaterial)
			throw RBX::runtime_error("Invalid number of arguments: %d, PhysicalProperties objects expect 1,3 or 5", count);
		else
			throw RBX::runtime_error("Invalid number of arguments: %d, PhysicalProperties objects expect 3 or 5", count);
	}

	pushNewObject(L, properties);
	return 1;
}


template<>
int Bridge<PhysicalProperties>::on_index(const PhysicalProperties& object, const char* name, lua_State *L)
{
	if (strcmp(name, "Density") == 0)
	{
		lua_pushnumber(L, object.getDensity());
		return 1;
	}
	if (strcmp(name, "Friction") == 0)
	{
		lua_pushnumber(L, object.getFriction());
		return 1;
	}
	if (strcmp(name, "Elasticity") == 0)
	{
		lua_pushnumber(L, object.getElasticity());
		return 1;
	}
	if (strcmp(name, "FrictionWeight") == 0)
	{
		lua_pushnumber(L, object.getFrictionWeight());
		return 1;
	}
	if (strcmp(name, "ElasticityWeight") == 0)
	{
		lua_pushnumber(L, object.getElasticityWeight());
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member of PhysicalProperties", name);
}

template<>
void Bridge<PhysicalProperties>::on_newindex(PhysicalProperties& object, const char* name, lua_State *L)
{
	// Failure CAN OVERRIDE THIS TO MAKE SETTING WORK
	throw RBX::runtime_error("PhysicalProperties.%s cannot be assigned to", name);
}

/// G3D::Rect2D has a default implementation for on_tostring() invoked from LuaBridge.cpp.
template<>
const char* Bridge<G3D::Rect2D>::className("Rect");

const luaL_reg Rect2DBridge::classLibrary[] = {
    {"new", newRect2D},
    {NULL, NULL}
};
void Rect2DBridge::registerClassLibrary (lua_State *L) {

	// Register the "new" function
	luaL_register(L, className, classLibrary);
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
};

int Rect2DBridge::newRect2D(lua_State *L)
{
    Rect2D rect;
   	// 0 args:   emptyRect
	// 2 vectors:   minVec, maxVec
	// 4 numbers:   x0,y0,x1,y1
	const int count = lua_gettop(L);
    if (count==0)
    {
        rect = Rect2D::xyxy(0.0f,0.0f,0.0f,0.0f);
    }
	else if (count == 2)
    {
        Vector2 min = Vector2Bridge::getObject(L, 1);
        Vector2 max = Vector2Bridge::getObject(L, 2);
        rect = Rect2D::xyxy(min,max);
    }
    else if (count == 4)
    {
        rect = Rect2D::xyxy(lua_tofloat(L, 1),lua_tofloat(L, 2),lua_tofloat(L, 3),lua_tofloat(L, 4));
    }
    else
    {
        throw RBX::runtime_error("Invalid number of arguments: %d", count);
	}
    pushNewObject(L, rect);
    return 1;
}
  
template<>
int Bridge<G3D::Rect2D>::on_index(const G3D::Rect2D& object, const char* name, lua_State *L)
{
    if (strcmp(name,"Min")==0)
	{
		Vector2Bridge::pushVector2(L, object.x0y0());
		return 1;
	}
	if (strcmp(name,"Max")==0)
	{
		Vector2Bridge::pushVector2(L, object.x1y1());
		return 1;
	}

	if (strcmp(name,"Width")==0)
	{
		lua_pushnumber(L, object.width());
		return 1;
	}
	if (strcmp(name,"Height")==0)
	{
		lua_pushnumber(L, object.height());
		return 1;
	}
    // Failure
    throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<G3D::Rect2D>::on_newindex(G3D::Rect2D& object, const char* name, lua_State *L)
{
    // Failure
    throw RBX::runtime_error("%s cannot be assigned to", name);
}
    
    
/// G3D::Vector3 has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<G3D::Vector3>::className("Vector3");

const luaL_reg Vector3Bridge::classLibrary[] = {
	{"new", newVector3},
	{"FromNormalId", newVector3FromNormalId},
	{"FromAxis", newVector3FromAxis},
	{NULL, NULL}
};

void Vector3Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary);
	pushVector3(L, G3D::Vector3::zero());
	lua_setfield(L, -2, "zero");
	pushVector3(L, G3D::Vector3(1, 1, 1));
	lua_setfield(L, -2, "one");
	pushVector3(L, G3D::Vector3(1, 0, 0));
	lua_setfield(L, -2, "xAxis");
	pushVector3(L, G3D::Vector3(0, 1, 0));
	lua_setfield(L, -2, "yAxis");
	pushVector3(L, G3D::Vector3(0, 0, 1));
	lua_setfield(L, -2, "zAxis");
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

G3D::Vector3& Vector3Bridge::getObject(lua_State* L, unsigned int index)
{
#if defined(RBX_LUAU_VM)
	if (lua_isvector(L, static_cast<int>(index)))
	{
		// Keep separate slots for simultaneously referenced arguments; several
		// atomic operators retain references to both operands.
		static thread_local G3D::Vector3 nativeValues[16];
		G3D::Vector3& value = nativeValues[index % 16];
		const float* native = lua_tovector(L, static_cast<int>(index));
		value = G3D::Vector3(native[0], native[1], native[2]);
		return value;
	}
#endif
	return Bridge<G3D::Vector3>::getObject(L, index);
}

bool Vector3Bridge::getValue(lua_State* L, unsigned int index,
	G3D::Vector3& value)
{
#if defined(RBX_LUAU_VM)
	if (lua_isvector(L, static_cast<int>(index)))
	{
		const float* native = lua_tovector(L, static_cast<int>(index));
		value = G3D::Vector3(native[0], native[1], native[2]);
		return true;
	}
#endif
	return Bridge<G3D::Vector3>::getValue(L, index, value);
}
    
int Vector3Bridge::on_add(lua_State *L)
{
	const G3D::Vector3& a = Vector3Bridge::getObject(L, 1);
	const G3D::Vector3& b = Vector3Bridge::getObject(L, 2);
	pushVector3(L, a + b);
	return 1;
};

int Vector3Bridge::on_sub(lua_State *L)
{
	const G3D::Vector3& a = Vector3Bridge::getObject(L, 1);
	const G3D::Vector3& b = Vector3Bridge::getObject(L, 2);
	pushVector3(L, a - b);
	return 1;
};

int Vector3Bridge::on_mul(lua_State *L)
{
	G3D::Vector3 a;
	if (Vector3Bridge::getValue(L, 1, a))
	{
		G3D::Vector3 b;
		if (Vector3Bridge::getValue(L, 2, b))
			pushVector3(L, a * b);
        else if(!lua_isnumber(L, 2))
        {
            throw std::runtime_error("attempt to multiply a Vector3 with an incompatible value type or nil");
        }
		else
		{
            float c = lua_tofloat(L, 2);
			pushVector3(L, a * c);
		}
	}
    else if(!lua_isnumber(L, 1))
    {
        throw std::runtime_error("attempt to multiply a Vector3 with an incompatible value type or nil");
    }
	else
    {
		a = Vector3Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector3(L, b * a);
	}
	return 1;
};

int Vector3Bridge::on_div(lua_State *L)
{
	G3D::Vector3 a;
	if (Vector3Bridge::getValue(L, 1, a))
	{
		G3D::Vector3 b;
		if (Vector3Bridge::getValue(L, 2, b))
			pushVector3(L, a / b);
        else if(!lua_isnumber(L, 2))
        {
            throw std::runtime_error("attempt to divide a Vector3 with an incompatible value type or nil");
        }
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector3(L, a / c);
		}
	}
    else if(!lua_isnumber(L, 1))
    {
        throw std::runtime_error("attempt to divide a Vector3 with an incompatible value type or nil");
    }
	else {
		a = Vector3Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector3(L, G3D::Vector3(b,b,b) / a);
	}
	return 1;
};

int Vector3Bridge::on_unm(lua_State *L)
{
	pushVector3(L, -Vector3Bridge::getObject(L, 1));
	return 1;
};


int Vector3Bridge::newVector3(lua_State *L)
{
#if defined(RBX_LUAU_VM)
	// Current Luau folds constant vector constructors in bytecode and passes the
	// native vector value back through Vector3.new.  Accept that representation
	// in addition to the historical three-number calling convention.
	if (lua_gettop(L) >= 1 && lua_isvector(L, 1))
	{
		const float* value = lua_tovector(L, 1);
		pushVector3(L, G3D::Vector3(value[0], value[1], value[2]));
		return 1;
	}
#endif

	float vector[3];

	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = lua_tofloat(L, i+1);
	for (int i = count; i<3; i++)
		vector[i] = 0.0;
	pushNewObject(L, vector);
	return 1;
}

int Vector3Bridge::newVector3FromNormalId(lua_State *L)
{
	EnumDescriptorItemPtr item;
	if (EnumItem::getItem(L, 1, item))
	{
		if(!item->owner.isType<RBX::NormalId>()){
			throw RBX::runtime_error("Vector3.FromNormalId expects Enum.NormalId input");
		}
		Bridge<G3D::Vector3>::pushNewObject(L,RBX::normalIdToVector3((RBX::NormalId)item->value));
	}
	else {
		throw RBX::runtime_error("Vector3.FromNormalId expects Enum.NormalId input");
	}
	return 1;
}

int Vector3Bridge::newVector3FromAxis(lua_State *L)
{
	EnumDescriptorItemPtr item;
	if (EnumItem::getItem(L, 1, item))
	{
		if(!item->owner.isType<RBX::Vector3::Axis>()){
			throw RBX::runtime_error("Vector3.FromAxis expects Enum.Axis input");
		}
		Bridge<G3D::Vector3>::pushNewObject(L,
			RBX::normalIdToVector3(
				Axes::axisToNormalId((RBX::Vector3::Axis)item->value)
				)
			);
	}
	else {
		throw RBX::runtime_error("Vector3.FromAxis expects Enum.Axis input");
	}
	return 1;
}

static int lerpVector3(lua_State *L)
{
	G3D::Vector3& self = Bridge<G3D::Vector3>::getObject(L, 1);
	G3D::Vector3& v = Bridge<G3D::Vector3>::getObject(L, 2);
	float alpha = lua_tofloat(L, 3);
	Bridge<G3D::Vector3>::pushNewObject(L, self.lerp(v, alpha));
	return 1;
}
static int crossVector3(lua_State *L)
{
	G3D::Vector3& self = Bridge<G3D::Vector3>::getObject(L, 1);
	G3D::Vector3& v = Bridge<G3D::Vector3>::getObject(L, 2);
	Bridge<G3D::Vector3>::pushNewObject(L, self.cross(v));
	return 1;
}

static int dotVector3(lua_State *L)
{
	G3D::Vector3& self = Bridge<G3D::Vector3>::getObject(L, 1);
	G3D::Vector3& v = Bridge<G3D::Vector3>::getObject(L, 2);
	lua_pushnumber(L, self.dot(v));
	return 1;
}

static int isCloseVector3(lua_State *L)
{
	int count = lua_gettop(L);
	G3D::Vector3& self = Bridge<G3D::Vector3>::getObject(L, 1);
	G3D::Vector3& v = Bridge<G3D::Vector3>::getObject(L, 2);

	bool result;
	if (count > 2) {
		result = Math::fuzzyEq(self, v, fabsf(lua_tofloat(L, 3)));
	} else {
		result = Math::fuzzyEq(self, v);
	}

	lua_pushboolean(L, result);
	return 1;
}

//Lower case are legacy
template<>
int Bridge<G3D::Vector3>::on_index(const G3D::Vector3& object, const char* name, lua_State *L)
{
	if (strcmp(name,"x")==0 || strcmp(name,"X")==0)
	{
		lua_pushnumber(L, object.x);
		return 1;
	}
	if (strcmp(name,"y")==0 || strcmp(name,"Y")==0)
	{
		lua_pushnumber(L, object.y);
		return 1;
	}
	if (strcmp(name,"z")==0 || strcmp(name,"Z")==0)
	{
		lua_pushnumber(L, object.z);
		return 1;
	}
	if (strcmp(name,"unit")==0 || strcmp(name,"Unit")==0)
	{
		Bridge<G3D::Vector3>::pushNewObject(L, object.unit());
		return 1;
	}
	if (strcmp(name,"magnitude")==0 || strcmp(name,"Magnitude") == 0)
	{
		lua_pushnumber(L, object.magnitude());
		return 1;
	}
	if (strcmp(name,"lerp")==0 || strcmp(name,"Lerp")==0)
	{
		lua_pushcfunction(L, lerpVector3);
		return 1;
	}
	if (strcmp(name,"Cross")==0)
	{
		lua_pushcfunction(L, crossVector3);
		return 1;
	}
	if (strcmp(name,"Dot")==0)
	{
		lua_pushcfunction(L, dotVector3);
		return 1;
	}
	if (strcmp(name,"isClose") == 0)
	{
		lua_pushcfunction(L, isCloseVector3);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<G3D::Vector3>::on_newindex(G3D::Vector3& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
const char* Bridge<RBX::Vector3int16>::className("Vector3int16");

const luaL_reg Vector3int16Bridge::classLibrary[] = {
	{"new", newVector3int16},
	{NULL, NULL}
};

void Vector3int16Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Vector3int16Bridge::on_add(lua_State *L)
{
	const RBX::Vector3int16& a = Vector3int16Bridge::getObject(L, 1);
	const RBX::Vector3int16& b = Vector3int16Bridge::getObject(L, 2);
	pushVector3int16(L, a + b);
	return 1;
};

int Vector3int16Bridge::on_sub(lua_State *L)
{
	const RBX::Vector3int16& a = Vector3int16Bridge::getObject(L, 1);
	const RBX::Vector3int16& b = Vector3int16Bridge::getObject(L, 2);
	pushVector3int16(L, a - b);
	return 1;
};

int Vector3int16Bridge::on_mul(lua_State *L)
{
	RBX::Vector3int16 a;
	if (Vector3int16Bridge::getValue(L, 1, a))
	{
		RBX::Vector3int16 b;
		if (Vector3int16Bridge::getValue(L, 2, b))
			pushVector3int16(L, a * b);
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector3int16(L, a * c);
		}
	}
	else {
		a = Vector3int16Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector3int16(L, a * b);
	}
	return 1;
};

int Vector3int16Bridge::on_div(lua_State *L)
{
	RBX::Vector3int16 a;
	if (Vector3int16Bridge::getValue(L, 1, a))
	{
		RBX::Vector3int16 b;
		if (Vector3int16Bridge::getValue(L, 2, b))
		{
			if (b.x == 0 || b.y == 0 || b.z == 0)
			{
				// Failure
				throw RBX::runtime_error("Divide by zero exception");
			}
				
			Vector3int16 c( a.x / b.x, a.y / b.y, a.z / b.z );
			pushVector3int16(L, c);
		}
		else
		{
			float c = lua_tofloat(L, 2);
			Vector3int16 d( a.x / c, a.y / c, a.z / c );
			pushVector3int16(L, d);
		}
	}
	else 
	{
		a = Vector3int16Bridge::getObject(L, 2);
		int b = lua_tointeger(L, 1);
		if (a.x == 0 || a. y == 0 || a.z == 0)
		{
			// Failure
			throw RBX::runtime_error("Divide by zero exception");
		}

		Vector3int16 c( b / a.x, b / a.y, b / a.z );
		pushVector3int16(L, c);
	}
	return 1;
};

int Vector3int16Bridge::on_unm(lua_State *L)
{
	Vector3int16 a = Vector3int16Bridge::getObject(L, 1);	
	Vector3int16 b( -a.x, -a.y, -a.z );
	pushVector3int16(L, b);
	return 1;
};


int Vector3int16Bridge::newVector3int16(lua_State *L)
{
	int vector[3];

	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = lua_tointeger(L, i+1);
	for (int i = count; i<3; i++)
		vector[i] = 0;
	pushNewObject(L, vector);
	return 1;
}


template<>
int Bridge<RBX::Vector3int16>::on_index(const RBX::Vector3int16& object, const char* name, lua_State *L)
{
	if (strcmp(name,"x")==0 || strcmp(name,"X")==0)
	{
		lua_pushinteger(L, object.x);
		return 1;
	}
	if (strcmp(name,"y")==0 || strcmp(name,"Y")==0)
	{
		lua_pushinteger(L, object.y);
		return 1;
	}
	if (strcmp(name,"z")==0 || strcmp(name,"Z")==0)
	{
		lua_pushinteger(L, object.z);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}


template<>
void Bridge<RBX::Vector3int16>::on_newindex(RBX::Vector3int16& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
const char* Bridge<RBX::Vector2int16>::className("Vector2int16");

const luaL_reg Vector2int16Bridge::classLibrary[] = {
	{"new", newVector2int16},
	{NULL, NULL}
};

void Vector2int16Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Vector2int16Bridge::on_add(lua_State *L)
{
	const RBX::Vector2int16& a = Vector2int16Bridge::getObject(L, 1);
	const RBX::Vector2int16& b = Vector2int16Bridge::getObject(L, 2);
	pushVector2int16(L, a + b);
	return 1;
};

int Vector2int16Bridge::on_sub(lua_State *L)
{
	const RBX::Vector2int16& a = Vector2int16Bridge::getObject(L, 1);
	const RBX::Vector2int16& b = Vector2int16Bridge::getObject(L, 2);
	pushVector2int16(L, a - b);
	return 1;
};

int Vector2int16Bridge::on_mul(lua_State *L)
{
	RBX::Vector2int16 a;
	if (Vector2int16Bridge::getValue(L, 1, a))
	{
		RBX::Vector2int16 b;
		if (Vector2int16Bridge::getValue(L, 2, b))
			pushVector2int16(L, a * b);
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector2int16(L, a * c);
		}
	}
	else {
		a = Vector2int16Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector2int16(L, b * a);
	}
	return 1;
};

int Vector2int16Bridge::on_div(lua_State *L)
{
	RBX::Vector2int16 a;
	if (Vector2int16Bridge::getValue(L, 1, a))
	{
		RBX::Vector2int16 b;
		if (Vector2int16Bridge::getValue(L, 2, b))
		{
			if (b.x == 0 || b.y == 0)
			{
				// Failure
				throw RBX::runtime_error("Divide by zero exception");
			}

			pushVector2int16(L, a / b);
		}
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector2int16(L, a / c);
		}
	}
	else 
	{
		a = Vector2int16Bridge::getObject(L, 2);
		int b = lua_tointeger(L, 1);

		if (a.x == 0 || a.y == 0)
		{
			// Failure
			throw RBX::runtime_error("Divide by zero exception");
		}
		
		pushVector2int16(L, RBX::Vector2int16(b,b) / a);
	}
	return 1;
};

int Vector2int16Bridge::on_unm(lua_State *L)
{
	pushVector2int16(L, -Vector2int16Bridge::getObject(L, 1));
	return 1;
};


int Vector2int16Bridge::newVector2int16(lua_State *L)
{
	int vector[2];

	// There should be up to 2 numerical parameters (x, y). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(2,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = lua_tointeger(L, i+1);
	for (int i = count; i<2; i++)
		vector[i] = 0;
	pushNewObject(L, vector);
	return 1;
}


template<>
int Bridge<RBX::Vector2int16>::on_index(const RBX::Vector2int16& object, const char* name, lua_State *L)
{
	if (strcmp(name,"x")==0 || strcmp(name,"X")==0)
	{
		lua_pushinteger(L, object.x);
		return 1;
	}
	if (strcmp(name,"y")==0 || strcmp(name,"Y")==0)
	{
		lua_pushinteger(L, object.y);
		return 1;
	}
	//if (strcmp(name,"unit")==0)
	//{
	//	Bridge<RBX::Vector2int16>::pushNewObject(L, object.direction());
	//	return 1;
	//}
	//if (strcmp(name,"magnitude")==0)
	//{
	//	lua_pushnumber(L, object.length());
	//	return 1;
	//}
	//if (strcmp(name,"lerp")==0)
	//{
	//	lua_pushcfunction(L, lerpVector2);
	//	return 1;
	//}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}


template<>
void Bridge<RBX::Vector2int16>::on_newindex(RBX::Vector2int16& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// RBX::Vector2 has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::Vector2>::className("Vector2");

const luaL_reg Vector2Bridge::classLibrary[] = {
	{"new", newVector2},
	{NULL, NULL}
};

void Vector2Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary);
	pushVector2(L, RBX::Vector2::zero());
	lua_setfield(L, -2, "zero");
	pushVector2(L, RBX::Vector2(1, 1));
	lua_setfield(L, -2, "one");
	pushVector2(L, RBX::Vector2(1, 0));
	lua_setfield(L, -2, "xAxis");
	pushVector2(L, RBX::Vector2(0, 1));
	lua_setfield(L, -2, "yAxis");
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int Vector2Bridge::on_add(lua_State *L)
{
	const RBX::Vector2& a = Vector2Bridge::getObject(L, 1);
	const RBX::Vector2& b = Vector2Bridge::getObject(L, 2);
	pushVector2(L, a + b);
	return 1;
};

int Vector2Bridge::on_sub(lua_State *L)
{
	const RBX::Vector2& a = Vector2Bridge::getObject(L, 1);
	const RBX::Vector2& b = Vector2Bridge::getObject(L, 2);
	pushVector2(L, a - b);
	return 1;
};

int Vector2Bridge::on_mul(lua_State *L)
{
	RBX::Vector2 a;
	if (Vector2Bridge::getValue(L, 1, a))
	{
		RBX::Vector2 b;
		if (Vector2Bridge::getValue(L, 2, b))
			pushVector2(L, a * b);
        else if(!lua_isnumber(L, 2))
        {
            throw std::runtime_error("attempt to multiply a Vector2 with an incompatible value type or nil");
        }
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector2(L, a * c);
		}
	}
    else if(!lua_isnumber(L, 1))
    {
        throw std::runtime_error("attempt to multiply a Vector2 with an incompatible value type or nil");
    }
	else {
		a = Vector2Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector2(L, b * a);
	}
	return 1;
};

int Vector2Bridge::on_div(lua_State *L)
{
	RBX::Vector2 a;
	if (Vector2Bridge::getValue(L, 1, a))
	{
		RBX::Vector2 b;
		if (Vector2Bridge::getValue(L, 2, b))
			pushVector2(L, a / b);
        else if(!lua_isnumber(L, 2))
        {
            throw std::runtime_error("attempt to divide a Vector2 with an incompatible value type or nil");
        }
		else
		{
			float c = lua_tofloat(L, 2);
			pushVector2(L, a / c);
		}
	}
    else if(!lua_isnumber(L, 1))
    {
        throw std::runtime_error("attempt to divide a Vector2 with an incompatible value type or nil");
    }
	else 
	{
		a = Vector2Bridge::getObject(L, 2);
		float b = lua_tofloat(L, 1);
		pushVector2(L, RBX::Vector2(b,b) / a);
	}
	return 1;
};

int Vector2Bridge::on_unm(lua_State *L)
{
	pushVector2(L, -Vector2Bridge::getObject(L, 1));
	return 1;
};


int Vector2Bridge::newVector2(lua_State *L)
{
	float vector[2];

	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(2,lua_gettop(L));
	for (int i = 0; i<count; i++)
		vector[i] = lua_tofloat(L, i+1);
	for (int i = count; i<2; i++)
		vector[i] = 0.0;
	pushNewObject(L, vector);
	return 1;
}

static int lerpVector2(lua_State *L)
{
	RBX::Vector2& self = Bridge<RBX::Vector2>::getObject(L, 1);
	RBX::Vector2& v = Bridge<RBX::Vector2>::getObject(L, 2);
	float alpha = lua_tofloat(L, 3);
	Bridge<RBX::Vector2>::pushNewObject(L, self.lerp(v, alpha));
	return 1;
}

template<>
int Bridge<RBX::Vector2>::on_index(const RBX::Vector2& object, const char* name, lua_State *L)
{
	if (strcmp(name,"x")==0 || strcmp(name,"X")==0)
	{
		lua_pushnumber(L, object.x);
		return 1;
	}
	if (strcmp(name,"y")==0 || strcmp(name,"Y")==0)
	{
		lua_pushnumber(L, object.y);
		return 1;
	}
	if (strcmp(name,"unit")==0 || strcmp(name,"Unit")==0)
	{
		Bridge<RBX::Vector2>::pushNewObject(L, object.direction());
		return 1;
	}
	if (strcmp(name,"magnitude")==0 || strcmp(name,"Magnitude")==0)
	{
		lua_pushnumber(L, object.length());
		return 1;
	}
	if (strcmp(name,"lerp")==0 || strcmp(name,"Lerp")==0)
	{
		lua_pushcfunction(L, lerpVector2);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::Vector2>::on_newindex(RBX::Vector2& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// RBX::BrickColor has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
/// RBX::BrickColor has a default implementation for registerClass() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::BrickColor>::className("BrickColor");

void BrickColorBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
static int pushRed(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickRed());
	return 1;
}

static int pushWhite(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickWhite());
	return 1;
}

static int pushGray(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickGray());
	return 1;
}

static int pushDarkGray(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickDarkGray());
	return 1;
}

static int pushBlack(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickBlack());
	return 1;
}

static int pushYellow(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickYellow());
	return 1;
}
static int pushGreen(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickGreen());
	return 1;
}
static int pushBlue(lua_State *L)
{
	BrickColorBridge::pushNewObject(L, BrickColor::brickBlue());
	return 1;
}

const luaL_reg BrickColorBridge::classLibrary[] = {
	{"new", newBrickColor},
	{"random", randomBrickColor},
	{"palette", paletteBrickColor},
	{"New", newBrickColor},
	{"Random", randomBrickColor},
	{"White", pushWhite },
	{"Gray", pushGray },
	{"DarkGray", pushDarkGray },
	{"Black", pushBlack },
	{"Red", pushRed },
	{"Yellow", pushYellow },
	{"Green", pushGreen },
	{"Blue", pushBlue },
	{NULL, NULL}
};

int BrickColorBridge::newBrickColor(lua_State *L)
{
	// There should be up to 3 numerical parameters (r,g,b). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));

	if (count==0)
	{
		pushNewObject(L, BrickColor::defaultColor());
	}
	if (count==1)
	{
		// TODO: Handle Color3 as option
		if (lua_isnumber(L, 1))
			pushNewObject(L, BrickColor(lua_tointeger(L, 1)));
		else if (lua_isstring(L, 1))
			pushNewObject(L, BrickColor::parse(lua_tostring(L, 1)));
		else 
			pushNewObject(L, BrickColor::closest(Color3Bridge::getObject(L, 1)));
	}
	else
	{
		G3D::Color4 color(0,0,0);
		for (int i = 0; i<count; i++)
			color[i] = lua_tofloat(L, i+1);
		pushNewObject(L, BrickColor::closest(color));
	}
	return 1;
}
int BrickColorBridge::paletteBrickColor(lua_State *L)
{
	// There should be 1 numerical parameter (index). Following Lua conventions ignore others and use 0 if it is missing
	int count = std::min(1,lua_gettop(L));
	int index = 0;
	if (count==1)
	{
		index = lua_tointeger(L, 1);
	}
	if(index < 0 || index >= int(BrickColor::colorPalette().size())){
		throw RBX::runtime_error("palette index out of bounds (%d)", index);
	}
	pushNewObject(L, BrickColor::colorPalette().at(index));
	return 1;
}
int BrickColorBridge::randomBrickColor(lua_State *L)
{
	pushNewObject(L, BrickColor::random());
	return 1;
}

template<>
int Bridge<RBX::BrickColor>::on_index(const RBX::BrickColor& object, const char* name, lua_State *L)
{
	if (strcmp(name,"number")==0)
	{
		lua_pushinteger(L, object.number);
		return 1;
	}
	if (strcmp(name,"Number")==0)
	{
		lua_pushinteger(L, object.number);
		return 1;
	}
	if (strcmp(name,"Color")==0)
	{
		Color3Bridge::pushNewObject(L, object.color3());
		return 1;
	}
	if (strcmp(name,"r")==0)
	{
		lua_pushnumber(L, object.color3().r);
		return 1;
	}
	if (strcmp(name,"g")==0)
	{
		lua_pushnumber(L, object.color3().g);
		return 1;
	}
	if (strcmp(name,"b")==0)
	{
		lua_pushnumber(L, object.color3().b);
		return 1;
	}
	if (strcmp(name,"name")==0)
	{
		lua_pushstring(L, object.name());
		return 1;
	}
	if (strcmp(name,"Name")==0)
	{
		lua_pushstring(L, object.name());
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::BrickColor>::on_newindex(RBX::BrickColor& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// G3D::CoordinateFrame has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<G3D::CoordinateFrame>::className("CFrame");

const luaL_reg CoordinateFrameBridge::classLibrary[] = {
	{"new", newCoordinateFrame},
	{"fromEulerAnglesXYZ", fromEulerAnglesXYZ},
	{"Angles", fromEulerAnglesXYZ},	//Synonym, much shorter for 
	{"fromEulerAnglesYXZ", fromEulerAnglesYXZ},
	{"fromOrientation", fromEulerAnglesYXZ},
	{"fromMatrix", fromMatrix},
	{"lookAt", lookAt},
	{"fromAxisAngle", fromAxisAngle},
	{NULL, NULL}
};

void CoordinateFrameBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int CoordinateFrameBridge::newCoordinateFrame(lua_State *L)
{
	G3D::CoordinateFrame cf;

	// 0 args:   identity
	// 1 vector:   translation
	// 2 vectors:   translation, lookAt
	// 3 numbers:   translation
	// 7 numbers:   translation, quat
	// 12 numbers:  translation, rotation matrix
	const int count = lua_gettop(L);
	switch (count)
	{
	case 0:
		// Identity
		break;

	case 1:
		cf.translation = Vector3Bridge::getObject(L, 1);
		break;

	case 2:
		cf.translation = Vector3Bridge::getObject(L, 1);
		cf.lookAt(Vector3Bridge::getObject(L, 2));
		break;

	case 3:
		for (int i = 0; i<3; i++)
			cf.translation[i] = lua_tofloat(L, i+1);
		break;

	case 7:
		{
			for (int i = 0; i<3; i++)
				cf.translation[i] = lua_tofloat(L, i+1);
			G3D::Quat q;
			q.x = lua_tofloat(L, 3+1);
			q.y = lua_tofloat(L, 4+1);
			q.z = lua_tofloat(L, 5+1);
			q.w = lua_tofloat(L, 6+1);
			cf.rotation = q;
		}
		break;

	case 12:
		for (int i = 0; i<3; i++)
			cf.translation[i] = lua_tofloat(L, i+1);
		for (int i = 0; i<3; ++i)
			for (int j = 0; j<3; ++j)
				cf.rotation[i][j] = lua_tofloat(L, 3 + 3*i + j +1);
		break;

	default:
		throw RBX::runtime_error("Invalid number of arguments: %d", count);
		break;

	}

	pushNewObject(L, cf);
	return 1;
}


int CoordinateFrameBridge::on_add(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	const G3D::Vector3& b = Vector3Bridge::getObject(L, 2);
	pushCoordinateFrame(L, a + b);
	return 1;
};

int CoordinateFrameBridge::on_sub(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	const G3D::Vector3& b = Vector3Bridge::getObject(L, 2);
	pushCoordinateFrame(L, a - b);
	return 1;
};

int CoordinateFrameBridge::on_mul(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);

	// Try Ma * Mb
	G3D::CoordinateFrame b;
	if (CoordinateFrameBridge::getValue(L, 2, b))
	{
		pushCoordinateFrame(L, a * b);
		return 1;
	}

	// Try Ma * Vb
	Vector3Bridge::pushVector3(L, a.toWorldSpace(G3D::Vector4(Vector3Bridge::getObject(L, 2),1)).xyz());

	return 1;
};

int CoordinateFrameBridge::on_inverse(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	pushCoordinateFrame(L, a.inverse());
	return 1;
};

int CoordinateFrameBridge::on_lerp(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	const G3D::CoordinateFrame& other = CoordinateFrameBridge::getObject(L, 2);
	float alpha = lua_tofloat(L, 3);
	pushCoordinateFrame(L, a.lerp(other, alpha));
	return 1;
};

int CoordinateFrameBridge::on_orthonormalize(lua_State *L)
{
	G3D::CoordinateFrame result = CoordinateFrameBridge::getObject(L, 1);
	result.rotation.orthonormalize();
	pushCoordinateFrame(L, result);
	return 1;
}

int CoordinateFrameBridge::on_toWorldSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		pushCoordinateFrame(L, a);
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::CoordinateFrame& b = CoordinateFrameBridge::getObject(L, i+2);
		pushCoordinateFrame(L, a * b);
	}
	return count;
};

int CoordinateFrameBridge::on_toObjectSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		pushCoordinateFrame(L, a.inverse());
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::CoordinateFrame& b = CoordinateFrameBridge::getObject(L, i+2);
		pushCoordinateFrame(L, a.toObjectSpace(b));
	}
	return count;
};

int CoordinateFrameBridge::on_pointToWorldSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		Vector3Bridge::pushVector3(L, a.pointToWorldSpace(G3D::Vector3::zero()));
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::Vector3& b = Vector3Bridge::getObject(L, i+2);
		Vector3Bridge::pushVector3(L, a.pointToWorldSpace(b));
	}
	return count;
}
int CoordinateFrameBridge::on_pointToObjectSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		Vector3Bridge::pushVector3(L, a.pointToObjectSpace(G3D::Vector3::zero()));
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::Vector3& b = Vector3Bridge::getObject(L, i+2);
		Vector3Bridge::pushVector3(L, a.pointToObjectSpace(b));
	}
	return count;
}
int CoordinateFrameBridge::on_vectorToWorldSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		Vector3Bridge::pushVector3(L, a.vectorToWorldSpace(G3D::Vector3::zero()));
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::Vector3& b = Vector3Bridge::getObject(L, i+2);
		Vector3Bridge::pushVector3(L, a.vectorToWorldSpace(b));
	}
	return count;
}

int CoordinateFrameBridge::on_components(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	lua_pushnumber(L, a.translation.x);
	lua_pushnumber(L, a.translation.y);
	lua_pushnumber(L, a.translation.z);
	for (int i=0; i<3; ++i)
		for (int j=0; j<3; ++j)
			lua_pushnumber(L, a.rotation[i][j]);
	return 12;
}

int CoordinateFrameBridge::on_toEulerAnglesXYZ(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	float x,y,z;
	a.rotation.toEulerAnglesXYZ(x,y,z);
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_pushnumber(L, z);
	return 3;
}

int CoordinateFrameBridge::on_toEulerAnglesYXZ(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	float x, y, z;
	a.rotation.toEulerAnglesYXZ(x, y, z);
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_pushnumber(L, z);
	return 3;
}

int CoordinateFrameBridge::on_toAxisAngle(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	G3D::Vector3 axis;
	float angle;
	a.rotation.toAxisAngle(axis, angle);
	Vector3Bridge::pushVector3(L, axis);
	lua_pushnumber(L, angle);
	return 2;
}

int CoordinateFrameBridge::on_fuzzyEq(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	const G3D::CoordinateFrame& b = CoordinateFrameBridge::getObject(L, 2);
	const float epsilon = lua_gettop(L) >= 3 ? luaL_checknumber(L, 3) : 1.0e-5f;
	if (epsilon < 0.0f)
		throw RBX::runtime_error("CFrame:FuzzyEq epsilon must be non-negative");

	const G3D::Vector3 delta = a.translation - b.translation;
	G3D::Vector3 axis;
	float angle;
	(a.rotation.transpose() * b.rotation).toAxisAngle(axis, angle);
	const bool equal = std::abs(delta.x) <= epsilon &&
		std::abs(delta.y) <= epsilon && std::abs(delta.z) <= epsilon &&
		std::abs(angle) <= epsilon;
	lua_pushboolean(L, equal);
	return 1;
}

int CoordinateFrameBridge::on_angleBetween(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	const G3D::CoordinateFrame& b = CoordinateFrameBridge::getObject(L, 2);
	G3D::Vector3 axis;
	float angle;
	(a.rotation.transpose() * b.rotation).toAxisAngle(axis, angle);
	lua_pushnumber(L, std::abs(angle));
	return 1;
}

int CoordinateFrameBridge::on_vectorToObjectSpace(lua_State *L)
{
	const G3D::CoordinateFrame& a = CoordinateFrameBridge::getObject(L, 1);
	int count = lua_gettop(L)-1;
	if (count==0)
	{
		Vector3Bridge::pushVector3(L, a.vectorToObjectSpace(G3D::Vector3::zero()));
		return 1;
	}
	for (int i=0; i<count; ++i)
	{
		const G3D::Vector3& b = Vector3Bridge::getObject(L, i+2);
		Vector3Bridge::pushVector3(L, a.vectorToObjectSpace(b));
	}
	return count;
}



int CoordinateFrameBridge::fromEulerAnglesXYZ(lua_State *L)
{
	G3D::CoordinateFrame cf;
	cf.rotation = G3D::Matrix3::fromEulerAnglesXYZ((float) luaL_checknumber(L, 1), (float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3));
	CoordinateFrameBridge::pushNewObject(L, cf);
	return 1;
};

int CoordinateFrameBridge::fromEulerAnglesYXZ(lua_State *L)
{
	G3D::CoordinateFrame cf;
	cf.rotation = G3D::Matrix3::fromEulerAnglesYXZ(
		(float) luaL_checknumber(L, 1),
		(float) luaL_checknumber(L, 2),
		(float) luaL_checknumber(L, 3));
	CoordinateFrameBridge::pushNewObject(L, cf);
	return 1;
};

int CoordinateFrameBridge::fromMatrix(lua_State* L)
{
	const int count = lua_gettop(L);
	if (count != 3 && count != 4)
		throw RBX::runtime_error("CFrame.fromMatrix expects 3 or 4 arguments");

	G3D::CoordinateFrame cf;
	cf.translation = Vector3Bridge::getObject(L, 1);
	const G3D::Vector3& right = Vector3Bridge::getObject(L, 2);
	const G3D::Vector3& up = Vector3Bridge::getObject(L, 3);
	const G3D::Vector3 back = count == 4
		? Vector3Bridge::getObject(L, 4)
		: right.cross(up);
	cf.rotation.setColumn(0, right);
	cf.rotation.setColumn(1, up);
	cf.rotation.setColumn(2, back);
	CoordinateFrameBridge::pushNewObject(L, cf);
	return 1;
}

int CoordinateFrameBridge::lookAt(lua_State *L)
{
	G3D::CoordinateFrame cf;
	cf.translation = Vector3Bridge::getObject(L, 1);
	const G3D::Vector3& target = Vector3Bridge::getObject(L, 2);
	if (lua_gettop(L) >= 3)
		cf.lookAt(target, Vector3Bridge::getObject(L, 3));
	else
		cf.lookAt(target);
	CoordinateFrameBridge::pushNewObject(L, cf);
	return 1;
};

int CoordinateFrameBridge::fromAxisAngle(lua_State *L)
{
	G3D::CoordinateFrame cf;
	cf.rotation = G3D::Matrix3::fromAxisAngle(Vector3Bridge::getObject(L, 1), (float) luaL_checknumber(L, 2));
	CoordinateFrameBridge::pushNewObject(L, cf);
	return 1;
};

template<>
int Bridge<G3D::CoordinateFrame>::on_index(const G3D::CoordinateFrame& object, const char* name, lua_State *L)
{
	if (strcmp(name,"p")==0 || strcmp(name,"Position")==0)
	{
		Vector3Bridge::pushVector3(L, object.translation);
		return 1;
	}
	if (strcmp(name,"Rotation")==0)
	{
		CoordinateFrameBridge::pushCoordinateFrame(L,
			G3D::CoordinateFrame(object.rotation, G3D::Vector3::zero()));
		return 1;
	}
	if (strcmp(name,"lookVector")==0 || strcmp(name,"LookVector")==0)
	{
		Vector3Bridge::pushVector3(L, object.lookVector());
		return 1;
	}
	if (strcmp(name,"rightVector")==0 || strcmp(name,"RightVector")==0 || strcmp(name,"XVector")==0)
	{
		Vector3Bridge::pushVector3(L, object.rotation.column(0));
		return 1;
	}
	if (strcmp(name,"upVector")==0 || strcmp(name,"UpVector")==0 || strcmp(name,"YVector")==0)
	{
		Vector3Bridge::pushVector3(L, object.rotation.column(1));
		return 1;
	}
	if (strcmp(name,"ZVector")==0)
	{
		Vector3Bridge::pushVector3(L, object.rotation.column(2));
		return 1;
	}

	if (strcmp(name,"inverse")==0 || strcmp(name,"Inverse")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_inverse, 1);
		return 1;
	}

	if (strcmp(name,"lerp")==0 || strcmp(name,"Lerp")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_lerp, 1);
		return 1;
	}
	if (strcmp(name,"Orthonormalize")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_orthonormalize, 1);
		return 1;
	}

	if (strcmp(name,"toWorldSpace")==0 || strcmp(name,"ToWorldSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_toWorldSpace, 1);
		return 1;
	}
	if (strcmp(name,"toObjectSpace")==0 || strcmp(name,"ToObjectSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_toObjectSpace, 1);
		return 1;
	}
	if (strcmp(name,"pointToWorldSpace")==0 || strcmp(name,"PointToWorldSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_pointToWorldSpace, 1);
		return 1;
	}
	if (strcmp(name,"pointToObjectSpace")==0 || strcmp(name,"PointToObjectSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_pointToObjectSpace, 1);
		return 1;
	}
	if (strcmp(name,"vectorToWorldSpace")==0 || strcmp(name,"VectorToWorldSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_vectorToWorldSpace, 1);
		return 1;
	}
	if (strcmp(name,"vectorToObjectSpace")==0 || strcmp(name,"VectorToObjectSpace")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_vectorToObjectSpace, 1);
		return 1;
	}
	if (strcmp(name,"toEulerAnglesXYZ")==0 || strcmp(name,"ToEulerAnglesXYZ")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_toEulerAnglesXYZ, 1);
		return 1;
	}
	if (strcmp(name,"ToEulerAnglesYXZ")==0 || strcmp(name,"ToOrientation")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_toEulerAnglesYXZ, 1);
		return 1;
	}
	if (strcmp(name,"ToAxisAngle")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_toAxisAngle, 1);
		return 1;
	}
	if (strcmp(name,"FuzzyEq")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_fuzzyEq, 1);
		return 1;
	}
	if (strcmp(name,"AngleBetween")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_angleBetween, 1);
		return 1;
	}
	if (strcmp(name,"components")==0 || strcmp(name,"Components")==0 ||
		strcmp(name,"GetComponents")==0)
	{
		lua_pushvalue(L, -1);
		lua_pushcclosure(L, CoordinateFrameBridge::on_components, 1);
		return 1;
	}

	if (strcmp(name,"x")==0 || strcmp(name,"X")==0)
	{
		lua_pushnumber(L, object.translation.x);
		return 1;
	}
	if (strcmp(name,"y")==0 || strcmp(name,"Y")==0)
	{
		lua_pushnumber(L, object.translation.y);
		return 1;
	}
	if (strcmp(name,"z")==0 || strcmp(name,"Z")==0)
	{
		lua_pushnumber(L, object.translation.z);
		return 1;
	}


	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<G3D::CoordinateFrame>::on_newindex(G3D::CoordinateFrame& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}


template<>
const char* Bridge<RBX::UDim>::className("UDim");


const luaL_reg UDimBridge::classLibrary[] = {
	{"new", newUDim},
	{NULL, NULL}
};
    
void UDimBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int UDimBridge::newUDim(lua_State *L)
{
	// There should be up to 2 numerical parameters (scale,offset). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(3,lua_gettop(L));

	if (count==0)
	{
		pushNewObject(L, UDim(0.0f, 0));
	}
	if (count==1)
	{
		pushNewObject(L, UDim(lua_tofloat(L, 1), 0));
	}
	else
	{
		pushNewObject(L, UDim(lua_tofloat(L, 1), lua_tointeger(L, 2)));
	}
	return 1;
}

int UDimBridge::on_add(lua_State *L)
{
	const UDim& a = UDimBridge::getObject(L, 1);
	const UDim& b = UDimBridge::getObject(L, 2);
	pushNewObject(L, a + b);
	return 1;
};

int UDimBridge::on_sub(lua_State *L)
{

	const UDim& a = UDimBridge::getObject(L, 1);
	const UDim& b = UDimBridge::getObject(L, 2);
	pushNewObject(L, a - b);
	return 1;
};

int UDimBridge::on_unm(lua_State *L)
{
	pushNewObject(L, -UDimBridge::getObject(L, 1));
	return 1;
};

int UDimBridge::lerp(lua_State *L)
{
	const UDim& value = getObject(L, 1);
	const UDim& goal = getObject(L, 2);
	pushNewObject(L, value.lerp(goal, lua_tofloat(L, 3)));
	return 1;
}

template<>
int Bridge<RBX::UDim>::on_index(const RBX::UDim& object, const char* name, lua_State *L)
{
	if (strcmp(name,"Scale")==0)
	{
		lua_pushnumber(L, object.scale);
		return 1;
	}
	if (strcmp(name,"Offset")==0)
	{
		lua_pushinteger(L, object.offset);
		return 1;
	}
	if (strcmp(name, "Lerp") == 0 || strcmp(name, "lerp") == 0)
	{
		lua_pushcfunction(L, UDimBridge::lerp);
		return 1;
	}

	if(name && !isupper(name[0]))
		throw RBX::runtime_error("%s is not a valid member, did you forget to capitalize the first letter?", name);

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::UDim>::on_newindex(RBX::UDim& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
const char* Bridge<RBX::UDim2>::className("UDim2");


const luaL_reg UDim2Bridge::classLibrary[] = {
	{"new", newUDim2},
	{"fromScale", fromScale},
	{"fromOffset", fromOffset},
	{NULL, NULL}
};

int UDim2Bridge::fromScale(lua_State* L)
{
	pushNewObject(L, UDim2(luaL_checknumber(L, 1), 0, luaL_checknumber(L, 2), 0));
	return 1;
}

int UDim2Bridge::fromOffset(lua_State* L)
{
	pushNewObject(L, UDim2(0.0f, luaL_checkinteger(L, 1),
		0.0f, luaL_checkinteger(L, 2)));
	return 1;
}
    
void UDim2Bridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int UDim2Bridge::newUDim2(lua_State *L)
{
	// Current Luau exposes UDim2.new(UDim x, UDim y) in addition to the
	// historical four-number constructor. Foundation composes dimensions as
	// UDim values (for example a fixed-height icon button with a selectable
	// width), so coercing those userdata values through lua_tonumber silently
	// collapses otherwise valid current UI to 0x0.
	if (lua_gettop(L) == 2)
	{
		UDim x;
		UDim y;
		if (UDimBridge::getValue(L, 1, x) && UDimBridge::getValue(L, 2, y))
		{
			pushNewObject(L, UDim2(x, y));
			return 1;
		}
	}

	// There should be up to 4 numerical parameters (scalex,offsetx, scaley,offset) . Following Lua conventions ignore others and use 0 for missing
	int count = std::min(5,lua_gettop(L));

	float scaleX = 0.0f, scaleY = 0.0f;
	int offsetX = 0, offsetY = 0;
	switch(count){
		case 5:
		case 4:
			offsetY = lua_tointeger(L, 4);
		case 3:
			scaleY = lua_tofloat(L, 3);
		case 2:
			offsetX = lua_tointeger(L, 2);
		case 1:
			scaleX = lua_tofloat(L, 1);
		case 0:
			pushNewObject(L, UDim2(scaleX, offsetX, scaleY, offsetY));
			break;
	}
	return 1;
}

int UDim2Bridge::on_add(lua_State *L)
{
	const UDim2& a = UDim2Bridge::getObject(L, 1);
	const UDim2& b = UDim2Bridge::getObject(L, 2);
	pushNewObject(L, a + b);
	return 1;
};

int UDim2Bridge::on_sub(lua_State *L)
{
	const UDim2& a = UDim2Bridge::getObject(L, 1);
	const UDim2& b = UDim2Bridge::getObject(L, 2);
	pushNewObject(L, a - b);
	return 1;
};

int UDim2Bridge::on_unm(lua_State *L)
{
	pushNewObject(L, -UDim2Bridge::getObject(L, 1));
	return 1;
};

int UDim2Bridge::lerp(lua_State *L)
{
	const UDim2& value = getObject(L, 1);
	const UDim2& goal = getObject(L, 2);
	pushNewObject(L, value.lerp(goal, lua_tofloat(L, 3)));
	return 1;
}

template<>
int Bridge<RBX::UDim2>::on_index(const RBX::UDim2& object, const char* name, lua_State *L)
{
	if (strcmp(name,"X")==0 || strcmp(name,"Width")==0)
	{
		UDimBridge::pushUDim(L, object.x);
		return 1;
	}
	if (strcmp(name,"Y")==0 || strcmp(name,"Height")==0)
	{
		UDimBridge::pushUDim(L, object.y);
		return 1;
	}
	if (strcmp(name, "Lerp") == 0 || strcmp(name, "lerp") == 0)
	{
		lua_pushcfunction(L, UDim2Bridge::lerp);
		return 1;
	}

	if(name && !isupper(name[0]))
		throw RBX::runtime_error("%s is not a valid member, did you forget to capitalize the first letter?", name);

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<RBX::UDim2>::on_newindex(RBX::UDim2& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// RBX::Faces has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
/// RBX::Faces has a default implementation for registerClass() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::Faces>::className("Faces");


const luaL_reg FacesBridge::classLibrary[] = {
	{"new", newFaces},
	{NULL, NULL}
};

void FacesBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}
    
int FacesBridge::newFaces(lua_State *L)
{
	// There should be up to 6 Enum parameters () . Following Lua conventions ignore others and use 0 for missing
	int count = std::min(6,lua_gettop(L));

	int normalIdMask = RBX::NORM_NONE_MASK;
	for(int index = 1; index<= count; index++){
		//Select each enum, 
		EnumDescriptorItemPtr item;
		if (EnumItem::getItem(L, index, item))
		{
			if(!item->owner.isType<RBX::NormalId>())
				throw RBX::runtime_error("Faces.new expects Enum.NormalId inputs");

			normalIdMask |= normalIdToMask((RBX::NormalId)item->value);
		}
	}

	pushNewObject(L, Faces(normalIdMask));

	return 1;
}

template<>
int Bridge<RBX::Faces>::on_index(const RBX::Faces& object, const char* name, lua_State *L)
{
	if (strcmp(name,"Top")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_Y));
		return 1;
	}
	if (strcmp(name,"Bottom")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_Y_NEG));
		return 1;
	}
	if (strcmp(name,"Back")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_Z));
		return 1;
	}
	if (strcmp(name,"Front")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_Z_NEG));
		return 1;
	}
	if (strcmp(name,"Right")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_X));
		return 1;
	}
	if (strcmp(name,"Left")==0)
	{
		lua_pushboolean(L, object.getNormalId(RBX::NORM_X_NEG));
		return 1;
	}

	if(name && !isupper(name[0]))
		throw RBX::runtime_error("%s is not a valid member, did you forget to capitalize the first letter?", name);
	// Failure
	throw RBX::runtime_error("%s is not a valid member. Valid members are Top,Bottom,Left,Right,Back,Front", name);
}

template<>
void Bridge<RBX::Faces>::on_newindex(RBX::Faces& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

/// RBX::Axes has a default implementation for on_tostring() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
/// RBX::Axes has a default implementation for registerClass() invoked from LuaBridge.cpp. It is important you read LuaBridge.cpp if you are adding or removing any specialization
template<>
const char* Bridge<RBX::Axes>::className("Axes");

const luaL_reg AxesBridge::classLibrary[] = {
	{"new", newAxes},
	{NULL, NULL}
};

void AxesBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int AxesBridge::newAxes(lua_State *L)
{
	// There should be up to 6 Enum parameters () . Following Lua conventions ignore others and use 0 for missing
	int count = std::min(6,lua_gettop(L));

	int axisMask = 0;
	for(int index = 1; index<= count; index++){
		//Select each enum, 
		EnumDescriptorItemPtr item;
		if (EnumItem::getItem(L, index, item))
		{
			if(!item->owner.isType<RBX::Vector3::Axis>() && !item->owner.isType<RBX::NormalId>() )
				throw RBX::runtime_error("Axes.new expects Enum.Axis or Enum.NormalId inputs");
			if(item->owner.isType<RBX::Vector3::Axis>()){
				axisMask |= RBX::Axes::axisToMask((RBX::Vector3::Axis)item->value);
			}
			else{
				axisMask |= RBX::Axes::axisToMask(RBX::Axes::normalIdToAxis((RBX::NormalId)item->value));

			}

		}
	}

	pushNewObject(L, RBX::Axes(axisMask));

	return 1;
}

template<>
int Bridge<RBX::Axes>::on_index(const RBX::Axes& object, const char* name, lua_State *L)
{
	if (strcmp(name,"X")==0)
	{
		lua_pushboolean(L, object.getAxis(RBX::Vector3::X_AXIS));
		return 1;
	}
	if (strcmp(name,"Y")==0)
	{
		lua_pushboolean(L, object.getAxis(RBX::Vector3::Y_AXIS));
		return 1;
	}
	if (strcmp(name,"Z")==0)
	{
		lua_pushboolean(L, object.getAxis(RBX::Vector3::Z_AXIS));
		return 1;
	}


	if (strcmp(name,"Top")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_Y));
		return 1;
	}
	if (strcmp(name,"Bottom")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_Y_NEG));
		return 1;
	}
	if (strcmp(name,"Back")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_Z));
		return 1;
	}
	if (strcmp(name,"Front")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_Z_NEG));
		return 1;
	}
	if (strcmp(name,"Right")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_X));
		return 1;
	}
	if (strcmp(name,"Left")==0)
	{
		lua_pushboolean(L, object.getAxisByNormalId(RBX::NORM_X_NEG));
		return 1;
	}

	if(name && !isupper(name[0]))
		throw RBX::runtime_error("%s is not a valid member, did you forget to capitalize the first letter?", name);
	// Failure
	throw RBX::runtime_error("%s is not a valid member, valid members are X,Y,Z,Top,Bottom,Left,Right,Front,Back", name);
}

template<>
void Bridge<RBX::Axes>::on_newindex(RBX::Axes& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

template<>
const char* Bridge<CellID>::className("CellId");

const luaL_reg CellIDBridge::classLibrary[] = {
	{"new", newCellID},
	{NULL, NULL}
};

void CellIDBridge::registerClassLibrary (lua_State *L) {
    
	// Register the "new" function
	luaL_register(L, className, classLibrary); 
	lua_setreadonly(L, -1, true);
	lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html

}
    
int CellIDBridge::newCellID(lua_State *L)
{
	bool IsNil;
	float vector[3];
	shared_ptr<Instance> objectRef;

	// There should be up to 1 boolean parameter and 3 numerical parameters (IsNil,x,y,z). Following Lua conventions ignore others and use 0 for missing
	int count = std::min(4,lua_gettop(L));
	IsNil = count > 0 ? lua_toboolean(L, 0) != 0 : false;
	for (int i = 1; i<count; i++)
		vector[i-1] = lua_tofloat(L, i);
	for (int i = count+1; i<3; i++)
		vector[i] = 0.0;

	pushNewObject(L, CellID::fromParameters( IsNil, vector, objectRef ) );
	

	return 1;
}

template<>
int Bridge<CellID>::on_index(const CellID& object, const char* name, lua_State *L)
{
	if (strcmp(name,"IsNil")==0)
	{
		lua_pushboolean(L, object.getIsNil());
		return 1;
	}
	if (strcmp(name,"Location")==0)
	{
		Vector3Bridge::pushVector3(L, object.getLocation());
		return 1;
	}
	if (strcmp(name,"TerrainPart")==0)
	{
		ObjectBridge::push(L, object.getTerrainPart());
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member, valid members are IsNil,Location,TerrainPart", name);
}
    
template<>
void Bridge<CellID>::on_newindex(CellID& object, const char* name, lua_State *L)
{
    // Failure
    throw RBX::runtime_error("%s cannot be assigned to", name);
}

//////////////////////////////////////////////////////////////////////////

template<> const char* Bridge<RBX::NumberSequence>::className("NumberSequence");

const luaL_reg NumberSequenceBridge::classLibrary[] = 
{
    { "new", newNumberSequence },
    { 0, 0 },
};

void NumberSequenceBridge::registerClassLibrary(lua_State* L)
{
    // Register the "new" function
    luaL_register(L, className, classLibrary); 
    lua_setreadonly(L, -1, true);
    lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int NumberSequenceBridge::newNumberSequence(lua_State* L)
{
    if (lua_isnumber(L,-1))
    {
        pushNumberSequence(L, RBX::NumberSequence(lua_tonumber(L,-1)));
        return 1;
    }

    if (!lua_istable(L,-1))
        throw std::runtime_error("NumberSequence ctor: table of NumberSequenceKeypoints expected.");

    int len = lua_objlen(L, -1); // NOTE: untrusted?

    std::vector<NumberSequence::Key> keys;
    if (len>0) keys.reserve(len);

    for( int j=1; ;++j)
    {
        lua_rawgeti(L,-1,j);

        if (lua_isnil(L,-1)) 
            break;

        NumberSequenceKeypoint kp;
        if (!NumberSequenceKeypointBridge::getValue(L, lua_gettop(L), kp))
        {
            throw RBX::runtime_error("NumberSequence ctor: expected 'NumberSequenceKeypoint' at index %d", j);
        }

        lua_pop(L,1); // pop the keypoint
        keys.push_back(kp);
    }

    pushNumberSequence(L, NumberSequence(keys, true));
    return 1;
}

template<>
int Bridge<NumberSequence>::on_index(const NumberSequence& object, const char* name, lua_State *L)
{
    if (0 == strcmp(name,"Keypoints"))
    {
        const std::vector<NumberSequence::Key>& kp = object.getPoints();
        lua_createtable(L, kp.size(), 0);
        for (unsigned j=0; j<kp.size(); ++j )
        {
            NumberSequenceKeypointBridge::pushNumberSequenceKeypoint(L, kp[j]);
            lua_rawseti(L, -2, j+1);
        }
        return 1;
    }
    throw RBX::runtime_error("'%s' is not a member of NumberSequence", name);
}

template<>
void Bridge<NumberSequence>::on_newindex( NumberSequence& object, const char* name, lua_State *L)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}


//////////////////////////////////////////////////////////////////////////
// ColorSequence

template<> const char* Bridge<RBX::ColorSequence>::className("ColorSequence");

const luaL_reg ColorSequenceBridge::classLibrary[] = 
{
    { "new", newColorSequence },
    { 0, 0 },
};

void ColorSequenceBridge::registerClassLibrary(lua_State* L)
{
    // Register the "new" function
    luaL_register(L, className, classLibrary); 
    lua_setreadonly(L, -1, true);
    lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int ColorSequenceBridge::newColorSequence(lua_State* L)
{
    do
    {
        Color3 c1, c2;
        bool b1 = Color3Bridge::getValue(L, 1, c1 );
        if (!b1)
            break;

        bool b2 = Color3Bridge::getValue(L, 2, c2 );
        if (!b2)
            c2 = c1;

        pushColorSequence(L, ColorSequence(c1, c2));
        return 1;
    }
    while(0);

    if (!lua_istable(L,-1))
    {
        throw std::runtime_error("ColorSequence.new(): table expected.");
    }

    int len = lua_objlen(L, -1); // NOTE: untrusted?

    std::vector<ColorSequence::Key> keys;
    if (len>0) keys.reserve(len);

    for( int j=1; ;++j)
    {
        ColorSequence::Key k;

        lua_rawgeti(L,-1,j);
        if (lua_isnil(L,-1)) 
            break;

        ColorSequenceKeypoint kp;
        if (!ColorSequenceKeypointBridge::getValue(L, lua_gettop(L), kp))
        {
            throw RBX::runtime_error("ColorSequence ctor: expected 'ColorSequenceKeypoint' at index %d", j);
        }

        lua_pop(L,1); // pop the keypoint
        keys.push_back(kp);
    }

    pushColorSequence(L, ColorSequence(keys, true));
    return 1;
}

template<>
int Bridge<ColorSequence>::on_index(const ColorSequence& object, const char* name, lua_State *L)
{
    if (0 == strcmp(name,"Keypoints"))
    {
        const std::vector<ColorSequence::Key>& kp = object.getPoints();
        lua_createtable(L, kp.size(), 0);
        for (unsigned j=0; j<kp.size(); ++j )
        {
            ColorSequenceKeypointBridge::pushColorSequenceKeypoint(L, kp[j]);
            lua_rawseti(L, -2, j+1);
        }
        return 1;
    }
    throw RBX::runtime_error("'%s' is not a member of ColorSequence", name);
}

template<>
void Bridge<ColorSequence>::on_newindex( ColorSequence& object, const char* name, lua_State *L)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}



//////////////////////////////////////////////////////////////////////////
// NumberSequenceKeypoint

template<> const char* Bridge<RBX::NumberSequenceKeypoint>::className("NumberSequenceKeypoint");

const luaL_reg NumberSequenceKeypointBridge::classLibrary[] = 
{
    { "new", newNumberSequenceKeypoint },
    { 0, 0 },
};

void NumberSequenceKeypointBridge::registerClassLibrary(lua_State* L)
{
    // Register the "new" function
    luaL_register(L, className, classLibrary); 
    lua_setreadonly(L, -1, true);
    lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int NumberSequenceKeypointBridge::newNumberSequenceKeypoint(lua_State* L)
{
    NumberSequenceKeypoint kp;
    kp.time = luaL_checknumber(L,1);
    kp.value = luaL_checknumber(L,2);
    kp.envelope = lua_tonumber(L,3); // optional, 0.0
    pushNumberSequenceKeypoint(L, kp);
    return 1;
}

template<>
int Bridge<NumberSequenceKeypoint>::on_index(const NumberSequenceKeypoint& object, const char* name, lua_State *L)
{
    if (0 == strcmp(name, "Time")) 
    {
        return lua_pushnumber(L, object.time), 1;
    }
    if (0 == strcmp(name, "Value"))
    {
        return lua_pushnumber(L,object.value), 1;
    }
    if (0 == strcmp(name, "Envelope"))
    {
        return lua_pushnumber(L,object.envelope), 1;
    }
    throw RBX::runtime_error("'%s' is not a valid member of NumberSequenceKeypoint", name);
}

template<>
void Bridge<NumberSequenceKeypoint>::on_newindex( NumberSequenceKeypoint& object, const char* name, lua_State *L)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}

//////////////////////////////////////////////////////////////////////////
// ColorSequenceKeypoint

template<> const char* Bridge<RBX::ColorSequenceKeypoint>::className("ColorSequenceKeypoint");

const luaL_reg ColorSequenceKeypointBridge::classLibrary[] = 
{
    { "new", newColorSequenceKeypoint },
    { 0, 0 },
};

void ColorSequenceKeypointBridge::registerClassLibrary(lua_State* L)
{
    // Register the "new" function
    luaL_register(L, className, classLibrary); 
    lua_setreadonly(L, -1, true);
    lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int ColorSequenceKeypointBridge::newColorSequenceKeypoint(lua_State* L)
{
    ColorSequenceKeypoint kp;
    kp.time = luaL_checknumber(L,1);
    bool b = Color3Bridge::getValue(L, 2, kp.value );
    if (!b)
        throw RBX::runtime_error("could not parse arg #2 to ColorSequenceKeypoint.new(), Color3 expected.");
    kp.envelope = 0; //lua_tonumber(L,3); // disabled for now
    pushColorSequenceKeypoint(L, kp);
    return 1;
}

template<>
int Bridge<ColorSequenceKeypoint>::on_index(const ColorSequenceKeypoint& object, const char* name, lua_State *L)
{
    if (0 == strcmp(name, "Time")) 
    {
        return lua_pushnumber(L, object.time), 1;
    }
    if (0 == strcmp(name, "Value"))
    {
        return Color3Bridge::pushColor3(L,object.value), 1;
    }
    /* disabled for now
    if (0 == strcmp(name, "Envelope"))
    {
        return lua_pushnumber(L,object.envelope), 1;
    }
    */
    throw RBX::runtime_error("'%s' is not a valid member of ColorSequenceKeypoint", name);
}

template<>
void Bridge<ColorSequenceKeypoint>::on_newindex( ColorSequenceKeypoint& object, const char* name, lua_State *L)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}
//////////////////////////////////////////////////////////////////////////


template<> const char* Bridge<RBX::NumberRange>::className("NumberRange");

const luaL_reg NumberRangeBridge::classLibrary[] = 
{
    { "new", newNumberRange },
    { 0, 0 },
};

void NumberRangeBridge::registerClassLibrary(lua_State* L)
{
    // Register the "new" function
    luaL_register(L, className, classLibrary); 
    lua_setreadonly(L, -1, true);
    lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
}

int NumberRangeBridge::newNumberRange(lua_State* L)
{
    float a,b;
    a = luaL_checknumber(L,1);
    if (lua_isnumber(L,2))
        b = lua_tonumber(L,2);
    else
        b = a;
    
    if (b < a)
        throw RBX::runtime_error("NumberRange: invalid range");

    pushNumberRange(L, NumberRange(a,b));
    return 1;
}

template<>
int Bridge<NumberRange>::on_index(const NumberRange& object, const char* name, lua_State *L)
{
    if (0 == strcmp(name, "Min")) 
    {
        return lua_pushnumber(L, object.min), 1;
    }
    if (0 == strcmp(name, "Max"))
    {
        return lua_pushnumber(L, object.max), 1;
    }
    throw RBX::runtime_error("'%s' is not a valid member of NumberRange", name);
}

template<>
void Bridge<NumberRange>::on_newindex( NumberRange& object, const char* name, lua_State *L)
{
    throw RBX::runtime_error("%s cannot be assigned to", name);
}


//////////////////////////////////////////////////////////////////////////

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<G3D::Vector3int16>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, Vector3int16Bridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, Vector3int16Bridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, Vector3int16Bridge::on_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "__div");
	lua_pushcfunction(L, Vector3int16Bridge::on_div);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, Vector3int16Bridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<G3D::Vector2int16>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, Vector2int16Bridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, Vector2int16Bridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, Vector2int16Bridge::on_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "__div");
	lua_pushcfunction(L, Vector2int16Bridge::on_div);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, Vector2int16Bridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.
template<>
void Bridge<G3D::Vector3>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, Vector3Bridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, Vector3Bridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, Vector3Bridge::on_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "__div");
	lua_pushcfunction(L, Vector3Bridge::on_div);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, Vector3Bridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<RBX::Vector2>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, Vector2Bridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, Vector2Bridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, Vector2Bridge::on_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "__div");
	lua_pushcfunction(L, Vector2Bridge::on_div);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, Vector2Bridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<G3D::CoordinateFrame>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, CoordinateFrameBridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, CoordinateFrameBridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, CoordinateFrameBridge::on_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "inverse");
	lua_pushcfunction(L, CoordinateFrameBridge::on_inverse);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}
    
// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.
template<>
void Bridge<RBX::Rect2D>::registerClass(lua_State *L)
{   
    // Register the object events
    luaL_newmetatable(L, className);
    Lua::protect_metatable(L, -1);
    
	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

    lua_pushstring(L, "__index");
    lua_pushcfunction(L, on_index);
    lua_settable(L, -3);
            
    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, on_newindex);
    lua_settable(L, -3);
            
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, on_gc);
    lua_settable(L, -3);
            
    lua_pushstring(L, "__eq");
    lua_pushcfunction(L, on_eq);
    lua_settable(L, -3);
            
    lua_pushstring(L, "__tostring");
    lua_pushcfunction(L, on_tostring);
    lua_settable(L, -3);
            
    lua_setreadonly(L, -1, true);
    lua_pop(L, 1);
}

template<>
void Bridge<PhysicalProperties>::registerClass(lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<RBX::UDim>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, UDimBridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, UDimBridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, UDimBridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

// The default implementation for registerClass() is available in LuaBridge.cpp. This is a specialization.		
template<>
void Bridge<RBX::UDim2>::registerClass (lua_State *L)
{
	// Register the object events
	luaL_newmetatable(L, className);
	Lua::protect_metatable(L, -1);

	lua_pushstring(L, "__type");
	lua_pushstring(L, className);
	lua_settable(L, -3);

	lua_pushstring(L, "__index");
	lua_pushcfunction(L, on_index);
	lua_settable(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, on_newindex);
	lua_settable(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, on_eq);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, on_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, UDim2Bridge::on_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, UDim2Bridge::on_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__unm");
	lua_pushcfunction(L, UDim2Bridge::on_unm);
	lua_settable(L, -3);

	lua_setreadonly(L, -1, true);
	lua_pop(L, 1);
}

} }
