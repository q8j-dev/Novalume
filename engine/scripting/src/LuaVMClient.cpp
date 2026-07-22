#include "Script/LuaVM.h"

#include "util/Guid.h"
#include "util/ProtectedString.h"

#include "util/MD5Hasher.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/HackDefines.h"

#define LUAVM_COMPILER
#include "lcode.c"
#include "lparser.c"

#define LUAVM_DESERIALIZER
#include "LuaSerializer.inl"

#include "lauxlib.h"
#include "lopcodes.h"

struct CoreScriptBytecode 
{ 
    const char* name; 
    const unsigned char* value; 
    size_t dataSize; 
};

#include "LuaGenCS.inl"

struct LoadS
{
    const char* data;
    size_t size;
};

static uint32_t encodeDaxArguments(uint32_t instruction, uint32_t evenMultiplier,
                                   uint32_t evenAddend, uint32_t oddMultiplier,
                                   uint32_t oddAddend)
{
    uint32_t result = 0;
    uint32_t mask = 1;
    for (size_t bit = 0; bit < 8 * sizeof(uint32_t); ++bit)
    {
        const uint32_t desired = mask & instruction;
        const uint32_t odd = mask & (result * oddMultiplier + oddAddend);
        const uint32_t even = mask & (result * evenMultiplier + evenAddend);
        if ((even ^ odd) != desired)
            result |= mask;
        mask <<= 1;
    }
    return result;
}

static const char* readSource(lua_State*, void* userData, size_t* size)
{
    LoadS* source = static_cast<LoadS*>(userData);
    if (source->size == 0)
        return nullptr;
    *size = source->size;
    source->size = 0;
    return source->data;
}

static void finalizeSource(Proto* prototype, RbxOpEncoder encode,
                           unsigned int key)
{
    lua_assert(key);
    for (int index = 0; index < prototype->sizecode; ++index)
        prototype->code[index].v = encode(prototype->code[index].v, index, key);
    for (int index = 0; index < prototype->sizep; ++index)
        finalizeSource(prototype->p[index], encode, key);
}

namespace LuaVM
{
    std::string compile(const std::string& source)
    {
        return "";
    }

    std::string compileLegacy(const std::string& source)
    {
        return "";
    }

    int load(lua_State* L, const RBX::ProtectedString& source, const char* chunkname, unsigned int modkey)
    {
        if (!source.getSource().empty())
        {
            const std::string& code = source.getSource();
            LoadS input = {code.data(), code.size()};
            const int result = lua_load(L, readSource, &input, chunkname);
            if (result == 0)
            {
                const LClosure* closure =
                    static_cast<const LClosure*>(lua_topointer(L, -1));
                finalizeSource(closure->p, LuaVM::rbxDaxEncode, modkey);
            }
            return result;
        }
        try
        {
            return LuaDeserializer::deserialize(L, source.getBytecode(), chunkname, modkey);
        }
        catch (std::bad_alloc&)
        {
            return LuaDeserializer::deserializeFailure(L, chunkname);
        }
    }
    
    unsigned int getKey()
    {
        // This is an initial value, it will be corrected by the server via SET_GLOBALS packet
        return LUAVM_KEY_DUMMY;
    }

    std::string compileCore(const std::string& source)
    {
        return "";
    }

    unsigned int getKeyCore()
    {
        return LUAVM_INTERNAL_CORE_DECODE_KEY;
    }

    unsigned int getModKeyCore()
    {
        // This is an initial value, it will be corrected by the server via SET_GLOBALS packet
        return LUAVM_MODKEY_DUMMY;
    }

    bool useSecureReplication()
    {
        // The portable runtime transports authenticated source through the
        // engine-owned replication boundary instead of the retired desktop
        // instruction-obfuscation protocol.
        return false;
    }

    bool canCompileScripts()
    {
        return true;
    }

	std::string getBytecodeCore(const std::string& name)
    {
        std::string rotName = RBX::rot13(name);
        for (int i = 0; i < sizeof(gCoreScripts)/sizeof(gCoreScripts[0]); i++)
            if (gCoreScripts[i].name == rotName)
                return std::string(reinterpret_cast<const char*>(gCoreScripts[i].value), gCoreScripts[i].dataSize);

        return "";
    }

	boost::unordered_map<std::string, std::string> getBytecodeCoreModules()
	{
		boost::unordered_map<std::string, std::string> coreModuleScripts;

		for (int i = 0; i < sizeof(gCoreModuleScripts)/sizeof(gCoreModuleScripts[0]); i++)
		{
			coreModuleScripts[gCoreModuleScripts[i].name] = std::string(reinterpret_cast<const char*>(gCoreModuleScripts[i].value), gCoreModuleScripts[i].dataSize);
		}

		return coreModuleScripts;
	}

    unsigned int rbxOldEncode(unsigned int i, int pc, unsigned int key)
    {
        return i;
    } 

    unsigned int rbxDaxEncode(unsigned int i, int pc, unsigned int key) 
    {
        Instruction encoded = i;
        const Instruction opcode = GET_OPCODE(i);
        switch (opcode)
        {
        case OP_CALL:
        case OP_TAILCALL:
        case OP_RETURN:
        case OP_CLOSURE:
            encoded = encodeDaxArguments(i, LUAVM_DAX_ME, pc,
                                         LUAVM_DAX_MO, LUAVM_DAX_AO);
            SET_OPCODE(encoded, opcode);
            break;
        case OP_MOVE:
            SETARG_C(encoded, pc | 1);
            break;
        default:
            break;
        }
        return LUAVM_ENCODEINSN(encoded, key);
    }
}
