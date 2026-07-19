#include "Script/LuauBytecode.h"
#include "Script/LuauRuntime.h"
#include "Script/LuaVM.h"

#include "lua.h"
#include "lualib.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> readFile(const char* path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error(std::string("Could not open bytecode fixture: ") + path);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(stream), {});
}

void requireFailure(const std::vector<std::uint8_t>& bytes)
{
    try
    {
        (void)RBX::LuauBytecode::decodeSignedChunk(bytes);
    }
    catch (const std::exception&)
    {
        return;
    }
    throw std::runtime_error("Malformed signed bytecode was accepted");
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 5)
        throw std::runtime_error("expected two signed and decoded bytecode fixture pairs");

    const std::vector<std::uint8_t> signedBytes = readFile(argv[1]);
    const std::vector<std::uint8_t> expected = readFile(argv[2]);
    const RBX::LuauBytecode::DecodedChunk decoded =
        RBX::LuauBytecode::decodeSignedChunk(signedBytes);

    if (decoded.bytecodeVersion != 7 || decoded.typeVersion != 3 ||
        decoded.integrityTrailerSize != 24 || decoded.bytes != expected)
        throw std::runtime_error("Decoded authoritative bytecode does not match the inspected chunk");

    RBX::Luau::Runtime runtime;
    runtime.loadBytecode(decoded.bytes, "=InExperience/Unibar/Constants");
    if (!lua_isfunction(runtime.state(), -1))
        throw std::runtime_error("Authoritative bytecode did not load as a Luau function");
    lua_pop(runtime.state(), 1);

    const std::vector<std::uint8_t> playerListSignedBytes = readFile(argv[3]);
    const std::vector<std::uint8_t> expectedPlayerList = readFile(argv[4]);
    const RBX::LuauBytecode::DecodedChunk playerListDecoded =
        RBX::LuauBytecode::decodeSignedChunk(playerListSignedBytes);
    if (playerListDecoded.bytecodeVersion != 7 ||
        playerListDecoded.typeVersion != 3 ||
        playerListDecoded.bytes != expectedPlayerList)
        throw std::runtime_error("Decoded PlayerList bytecode does not match its inspected chunk");

    if (LuaVM::loadBytecode(runtime.state(), playerListSignedBytes,
            "=InExperience/PlayerList/Common/Constants") != LUA_OK)
    {
        const char* error = lua_tostring(runtime.state(), -1);
        throw std::runtime_error(std::string("Production Player bytecode loader failed: ") +
            (error ? error : "unknown error"));
    }
    runtime.protectedCall(0, 1);
    if (!lua_istable(runtime.state(), -1))
        throw std::runtime_error("Authoritative PlayerList constants did not return a table");
    lua_getfield(runtime.state(), -1, "HEADER_HEIGHT");
    if (lua_tointeger(runtime.state(), -1) != 36)
        throw std::runtime_error("Authoritative PlayerList HEADER_HEIGHT changed or executed incorrectly");
    lua_pop(runtime.state(), 1);
    lua_getfield(runtime.state(), -1, "ROW_FLEX_TAG");
    const char* rowFlexTag = lua_tostring(runtime.state(), -1);
    if (!rowFlexTag || std::string(rowFlexTag) !=
            "row align-y-center padding-x-large gap-xsmall")
        throw std::runtime_error("Authoritative PlayerList ROW_FLEX_TAG executed incorrectly");
    lua_pop(runtime.state(), 2);

    std::vector<std::uint8_t> malformed = signedBytes;
    malformed[0] ^= 0xff;
    requireFailure(malformed);
    requireFailure(std::vector<std::uint8_t>(signedBytes.begin(), signedBytes.begin() + 16));
    malformed = signedBytes;
    malformed.push_back(0);
    requireFailure(malformed);
    return 0;
}
