#include "Script/LuaVM.h"

#include "Script/LuauBytecode.h"
#include "util/ProtectedString.h"

#include "Luau/Compiler.h"
#include "lua.h"

#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

const Luau::CompileOptions& playerCompileOptions()
{
    static const char* const mutableGlobals[] = {
        "Game", "Workspace", "game", "plugin", "script", "shared", "workspace", nullptr};
    static const Luau::CompileOptions options = [] {
        Luau::CompileOptions value;
        value.mutableGlobals = mutableGlobals;
        return value;
    }();
    return options;
}

std::string compilePlayerSource(const std::string& source)
{
    return Luau::compile(source, playerCompileOptions());
}

int loadBytes(lua_State* state, std::span<const std::uint8_t> bytes, const char* chunkName)
{
    return luau_load(state, chunkName, reinterpret_cast<const char*>(bytes.data()),
        bytes.size(), 0);
}

int loadFailure(lua_State* state, const char* chunkName, const std::exception& error)
{
    const std::string message = std::string(chunkName ? chunkName : "script") + ": " + error.what();
    lua_pushlstring(state, message.data(), message.size());
    return LUA_ERRSYNTAX;
}

} // namespace

namespace LuaVM {

std::string compile(const std::string& source)
{
    return compilePlayerSource(source);
}

std::string compileLegacy(const std::string& source)
{
    return compilePlayerSource(source);
}

int load(lua_State* state, const RBX::ProtectedString& source, const char* chunkName,
    unsigned int modkey)
{
    (void)modkey;
    try
    {
        if (!source.getSource().empty())
        {
            const std::string bytecode = compilePlayerSource(source.getSource());
            return loadBytes(state, std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t*>(bytecode.data()), bytecode.size()),
                chunkName);
        }

        const std::string& stored = source.getBytecode();
        if (stored.empty())
            throw std::runtime_error("script contains neither source nor bytecode");

        const std::span<const std::uint8_t> bytes(
            reinterpret_cast<const std::uint8_t*>(stored.data()), stored.size());
        return loadBytecode(state, bytes, chunkName);
    }
    catch (const std::exception& error)
    {
        return loadFailure(state, chunkName, error);
    }
}

int loadBytecode(lua_State* state, std::span<const std::uint8_t> bytes,
    const char* chunkName)
{
    try
    {
        if (bytes.empty())
            throw std::runtime_error("empty bytecode container");
        if (bytes.front() == 0x52)
        {
            const std::vector<std::uint8_t> signedBytes(bytes.begin(), bytes.end());
            const RBX::LuauBytecode::DecodedChunk decoded =
                RBX::LuauBytecode::decodeSignedChunk(signedBytes);
            return loadBytes(state, decoded.bytes, chunkName);
        }
        if (bytes.front() > 8)
            throw std::runtime_error("unsupported bytecode container");
        return loadBytes(state, bytes, chunkName);
    }
    catch (const std::exception& error)
    {
        return loadFailure(state, chunkName, error);
    }
}

unsigned int getKey()
{
    return LUAVM_KEY_DUMMY;
}

std::string compileCore(const std::string& source)
{
    return compilePlayerSource(source);
}

unsigned int getKeyCore()
{
    return LUAVM_KEY_DUMMY;
}

unsigned int getModKeyCore()
{
    return LUAVM_MODKEY_DUMMY;
}

bool useSecureReplication()
{
    return false;
}

bool canCompileScripts()
{
    return true;
}

std::string getBytecodeCore(const std::string&)
{
    return std::string();
}

boost::unordered_map<std::string, std::string> getBytecodeCoreModules()
{
    return boost::unordered_map<std::string, std::string>();
}

unsigned int rbxOldEncode(unsigned int instruction, int, unsigned int)
{
    return instruction;
}

unsigned int rbxDaxEncode(unsigned int instruction, int, unsigned int)
{
    return instruction;
}

} // namespace LuaVM
