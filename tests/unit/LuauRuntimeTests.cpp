#include "Script/LuauRuntime.h"

#include "lua.h"

#include <chrono>
#include <stdexcept>
#include <string>

namespace {

template<typename Function>
void requireFailureContaining(Function function, const std::string& expected)
{
    try
    {
        function();
    }
    catch (const std::exception& error)
    {
        if (std::string(error.what()).find(expected) != std::string::npos)
            return;
        throw;
    }
    throw std::runtime_error("Expected Luau runtime failure was not raised");
}

} // namespace

int main()
{
    {
        RBX::Luau::Runtime runtime;
        runtime.openStandardLibraries();
        runtime.sandboxEnvironment();
        runtime.compileAndLoad("return 6 * 7", "=arithmetic");
        runtime.protectedCall(0, 1);
        if (!lua_isnumber(runtime.state(), -1) || lua_tonumber(runtime.state(), -1) != 42)
            throw std::runtime_error("Compiled Luau did not execute correctly");
        lua_pop(runtime.state(), 1);
        if (runtime.memoryUsage() == 0 || runtime.peakMemoryUsage() < runtime.memoryUsage())
            throw std::runtime_error("Luau allocator accounting is invalid");
    }

    {
        RBX::Luau::Runtime runtime;
        lua_State* state = runtime.state();

        lua_newtable(state);
        lua_newtable(state);
        lua_newtable(state);
        lua_pushnumber(state, 1.0);
        lua_setfield(state, -2, "lookVector");
        lua_setfield(state, -2, "CFrame");
        lua_setfield(state, -2, "CurrentCamera");
        lua_setfield(state, LUA_GLOBALSINDEX, "workspace");

        runtime.compileAndLoad(
            "return function() return workspace.CurrentCamera.CFrame.lookVector end",
            "=mutable-workspace");
        runtime.protectedCall(0, 1);
        const int reader = lua_ref(state, -1);
        lua_pop(state, 1);

        lua_getref(state, reader);
        runtime.protectedCall(0, 1);
        if (lua_tonumber(state, -1) != 1.0)
            throw std::runtime_error("Initial mutable workspace property read failed");
        lua_pop(state, 1);

        lua_getfield(state, LUA_GLOBALSINDEX, "workspace");
        lua_getfield(state, -1, "CurrentCamera");
        lua_getfield(state, -1, "CFrame");
        lua_pushnumber(state, 2.0);
        lua_setfield(state, -2, "lookVector");
        lua_pop(state, 3);

        lua_getref(state, reader);
        runtime.protectedCall(0, 1);
        if (lua_tonumber(state, -1) != 2.0)
            throw std::runtime_error("Luau cached a mutable workspace property chain");
        lua_pop(state, 1);
        lua_unref(state, reader);
    }

    {
        RBX::Luau::ExecutionLimits limits;
        limits.interruptBudget = 64;
        limits.wallTime = std::chrono::seconds(1);
        RBX::Luau::Runtime runtime(limits);
        runtime.compileAndLoad("while true do end", "=interrupt-limit");
        requireFailureContaining([&] { runtime.protectedCall(0, 0); }, "interrupt budget");
    }

    {
        RBX::Luau::ExecutionLimits limits;
        limits.memoryBytes = 2u * 1024u * 1024u;
        RBX::Luau::Runtime runtime(limits);
        runtime.openStandardLibraries();
        runtime.compileAndLoad("return string.rep('x', 4 * 1024 * 1024)", "=memory-limit");
        requireFailureContaining([&] { runtime.protectedCall(0, 1); }, "memory");
    }
    return 0;
}
