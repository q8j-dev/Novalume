#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

struct lua_State;

namespace RBX::Luau {

struct ExecutionLimits
{
    std::size_t memoryBytes = 128u * 1024u * 1024u;
    std::uint64_t interruptBudget = 1'000'000;
    std::chrono::milliseconds wallTime = std::chrono::seconds(10);
};

class Runtime
{
public:
    explicit Runtime(ExecutionLimits limits = {});
    ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    lua_State* state() const { return vm; }
    std::size_t memoryUsage() const { return allocatedBytes; }
    std::size_t peakMemoryUsage() const { return peakAllocatedBytes; }

    void openStandardLibraries();
    void sandboxEnvironment();
    void loadBytecode(std::span<const std::uint8_t> bytecode, const std::string& chunkName);
    void compileAndLoad(const std::string& source, const std::string& chunkName);

    // Runs the function and arguments already on the stack. Errors include
    // syntax/runtime failures and enforced resource-limit termination.
    void protectedCall(int argumentCount, int resultCount);

private:
    static void* allocate(void* context, void* pointer, std::size_t oldSize,
        std::size_t newSize);
    static void interrupt(lua_State* state, int gc);
    void beginExecution();

    ExecutionLimits limits;
    lua_State* vm = nullptr;
    std::size_t allocatedBytes = 0;
    std::size_t peakAllocatedBytes = 0;
    std::uint64_t interruptsRemaining = 0;
    std::chrono::steady_clock::time_point deadline;
    bool librariesOpened = false;
    bool sandboxed = false;
};

} // namespace RBX::Luau
