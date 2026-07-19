#include "Script/LuauRuntime.h"

#include "Luau/Compiler.h"
#include "lua.h"
#include "lualib.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace RBX::Luau {

namespace {

const ::Luau::CompileOptions& runtimeCompileOptions()
{
    static const char* const mutableGlobals[] = {
        "Game", "Workspace", "game", "plugin", "script", "shared", "workspace", nullptr};
    static const ::Luau::CompileOptions options = [] {
        ::Luau::CompileOptions value;
        value.mutableGlobals = mutableGlobals;
        return value;
    }();
    return options;
}

} // namespace

Runtime::Runtime(ExecutionLimits limits)
    : limits(limits)
{
    if (limits.memoryBytes == 0 || limits.interruptBudget == 0 || limits.wallTime.count() <= 0)
        throw std::invalid_argument("Luau execution limits must be positive");
    vm = lua_newstate(&Runtime::allocate, this);
    if (!vm)
        throw std::runtime_error("Could not create Luau VM within its memory limit");
    lua_Callbacks* callbacks = lua_callbacks(vm);
    callbacks->userdata = this;
    callbacks->interrupt = &Runtime::interrupt;
}

Runtime::~Runtime()
{
    if (vm)
        lua_close(vm);
}

void* Runtime::allocate(void* context, void* pointer, std::size_t oldSize,
    std::size_t newSize)
{
    Runtime* runtime = static_cast<Runtime*>(context);
    if (newSize == 0)
    {
        std::free(pointer);
        runtime->allocatedBytes -= std::min(oldSize, runtime->allocatedBytes);
        return nullptr;
    }

    const std::size_t retained = runtime->allocatedBytes -
        std::min(oldSize, runtime->allocatedBytes);
    if (newSize > runtime->limits.memoryBytes - std::min(retained, runtime->limits.memoryBytes))
        return nullptr;

    void* replacement = std::realloc(pointer, newSize);
    if (!replacement)
        return nullptr;
    runtime->allocatedBytes = retained + newSize;
    runtime->peakAllocatedBytes = std::max(runtime->peakAllocatedBytes, runtime->allocatedBytes);
    return replacement;
}

void Runtime::interrupt(lua_State* state, int gc)
{
    if (gc >= 0)
        return;
    Runtime* runtime = static_cast<Runtime*>(lua_callbacks(state)->userdata);
    if (!runtime)
        luaL_error(state, "Luau runtime has no execution context");
    if (runtime->interruptsRemaining == 0)
        luaL_error(state, "Luau execution interrupt budget exceeded");
    --runtime->interruptsRemaining;
    if (std::chrono::steady_clock::now() >= runtime->deadline)
        luaL_error(state, "Luau execution time limit exceeded");
}

void Runtime::openStandardLibraries()
{
    if (!librariesOpened)
    {
        luaL_openlibs(vm);
        librariesOpened = true;
    }
}

void Runtime::sandboxEnvironment()
{
    if (!librariesOpened)
        openStandardLibraries();
    if (!sandboxed)
    {
        luaL_sandbox(vm);
        sandboxed = true;
    }
}

void Runtime::loadBytecode(std::span<const std::uint8_t> bytecode,
    const std::string& chunkName)
{
    const int result = luau_load(vm, chunkName.c_str(),
        reinterpret_cast<const char*>(bytecode.data()), bytecode.size(), 0);
    if (result != 0)
    {
        const char* error = lua_tostring(vm, -1);
        const std::string message = error ? error : "unknown bytecode loader error";
        lua_pop(vm, 1);
        throw std::runtime_error("Luau load failed: " + message);
    }
}

void Runtime::compileAndLoad(const std::string& source, const std::string& chunkName)
{
    const std::string bytecode = ::Luau::compile(source, runtimeCompileOptions());
    loadBytecode(std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(bytecode.data()), bytecode.size()), chunkName);
}

void Runtime::beginExecution()
{
    interruptsRemaining = limits.interruptBudget;
    deadline = std::chrono::steady_clock::now() + limits.wallTime;
}

void Runtime::protectedCall(int argumentCount, int resultCount)
{
    beginExecution();
    const int result = lua_pcall(vm, argumentCount, resultCount, 0);
    if (result != 0)
    {
        const char* error = lua_tostring(vm, -1);
        const std::string message = error ? error : "unknown runtime error";
        lua_pop(vm, 1);
        throw std::runtime_error("Luau execution failed: " + message);
    }
}

} // namespace RBX::Luau
