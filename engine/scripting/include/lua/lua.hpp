#pragma once

#include "lua.h"
#if defined(RBX_LUAU_VM)
#include "lualib.h"

typedef luaL_Reg luaL_reg;
typedef lua_Number LUA_NUMBER;

#undef lua_pushcfunction
#undef lua_pushcclosure
#define lua_pushcfunction(L, fn) lua_pushcclosurek((L), (fn), #fn, 0, NULL)
#define lua_pushcclosure(L, fn, nup) lua_pushcclosurek((L), (fn), #fn, (nup), NULL)
#ifndef LUA_QL
#define LUA_QL(x) "'" x "'"
#endif
#ifndef LUA_QS
#define LUA_QS LUA_QL("%s")
#endif

inline int luaL_ref(lua_State* state, int tableIndex)
{
    if (tableIndex != LUA_REGISTRYINDEX)
    {
        luaL_error(state, "Luau engine references must use the registry");
        return LUA_NOREF;
    }
    const int result = lua_ref(state, -1);
    lua_pop(state, 1);
    return result;
}

inline void luaL_unref(lua_State* state, int tableIndex, int reference)
{
    if (tableIndex != LUA_REGISTRYINDEX)
    {
        luaL_error(state, "Luau engine references must use the registry");
        return;
    }
    lua_unref(state, reference);
}
#else
#include "lauxlib.h"
#include "lualib.h"
#endif

#include <string>

namespace RBX
{
	namespace Lua
	{
		extern const char* safe_lua_tostring(lua_State *L, int idx);
		extern const char* throwable_lua_tostring(lua_State *L, int idx);
		extern float lua_tofloat(lua_State *L, int idx);
		extern void protect_metatable(lua_State* thread, int index);
		inline void lua_pushstring(lua_State* thread, const std::string& s)
		{
			lua_pushlstring(thread, s.c_str(), s.size());
		}

        const char* lua_checkstring_secure(lua_State* L, int idx);

        void lua_resetstack(lua_State* L, int idx);

		// Pops items from the stack when it goes out of scope
		class ScopedPopper
		{
			int popCount;
			lua_State* const thread;
		public:
			ScopedPopper(lua_State* thread, int popCount)
				:thread(thread),popCount(popCount) 
			{}

			ScopedPopper& operator +=(int popCount)
			{
				this->popCount += popCount;
				return *this;
			}

			ScopedPopper& operator -=(int popCount)
			{
				this->popCount -= popCount;
				return *this;
			}

			~ScopedPopper()
			{
				lua_pop(thread, popCount);
			}
		};

		class ScopedState
		{
			lua_State* const thread;
		public:
			ScopedState()
				:thread(luaL_newstate()) 
			{}

			~ScopedState()
			{
				lua_close(thread);
			}

			operator lua_State*()
			{
				return thread;
			}
		};

	}

}
