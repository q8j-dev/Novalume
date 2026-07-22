#include "v8datamodel/DataModelPatch.h"

#include "Script/CoreScript.h"
#include "Script/ModuleScript.h"
#include "v8datamodel/CorePackages.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/LocalizationTable.h"
#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/ScreenGui.h"
#include "v8datamodel/Value.h"
#include "v8xml/SerializerV2.h"

#if !defined(__EMSCRIPTEN__)
#include <openssl/evp.h>
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace RBX {
namespace {

std::string readText(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("could not open patch checksum: " + path.string());
    std::string result((std::istreambuf_iterator<char>(stream)), {});
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n'))
        result.pop_back();
    return result;
}

std::string blake2b512(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("could not open bundled data-model patch: " + path.string());

#if defined(__EMSCRIPTEN__)
    static constexpr std::array<std::uint64_t, 8> initialization = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL,
        0xa54ff53a5f1d36f1ULL, 0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL};
    static constexpr unsigned char schedule[12][16] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
        {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
        {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
        {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
        {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
        {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
        {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
        {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
        {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3}};
    std::array<std::uint64_t, 8> state = initialization;
    state[0] ^= 0x01010040ULL;
    std::uint64_t bytesLow = 0;
    std::uint64_t bytesHigh = 0;
    std::array<unsigned char, 128> block{};
    std::size_t blockSize = 0;
    auto rotate = [](std::uint64_t value, unsigned int count) {
        return (value >> count) | (value << (64 - count));
    };
    auto compress = [&](bool finalBlock) {
        std::array<std::uint64_t, 16> message{};
        for (std::size_t index = 0; index < message.size(); ++index)
            for (unsigned int byte = 0; byte < 8; ++byte)
                message[index] |= static_cast<std::uint64_t>(block[index * 8 + byte]) << (byte * 8);
        std::array<std::uint64_t, 16> work{};
        std::copy(state.begin(), state.end(), work.begin());
        std::copy(initialization.begin(), initialization.end(), work.begin() + 8);
        work[12] ^= bytesLow;
        work[13] ^= bytesHigh;
        if (finalBlock)
            work[14] = ~work[14];
        auto mix = [&](unsigned int a, unsigned int b, unsigned int c, unsigned int d,
                       std::uint64_t x, std::uint64_t y) {
            work[a] = work[a] + work[b] + x;
            work[d] = rotate(work[d] ^ work[a], 32);
            work[c] += work[d];
            work[b] = rotate(work[b] ^ work[c], 24);
            work[a] = work[a] + work[b] + y;
            work[d] = rotate(work[d] ^ work[a], 16);
            work[c] += work[d];
            work[b] = rotate(work[b] ^ work[c], 63);
        };
        for (unsigned int round = 0; round < 12; ++round)
        {
            const unsigned char* order = schedule[round];
            mix(0, 4, 8, 12, message[order[0]], message[order[1]]);
            mix(1, 5, 9, 13, message[order[2]], message[order[3]]);
            mix(2, 6, 10, 14, message[order[4]], message[order[5]]);
            mix(3, 7, 11, 15, message[order[6]], message[order[7]]);
            mix(0, 5, 10, 15, message[order[8]], message[order[9]]);
            mix(1, 6, 11, 12, message[order[10]], message[order[11]]);
            mix(2, 7, 8, 13, message[order[12]], message[order[13]]);
            mix(3, 4, 9, 14, message[order[14]], message[order[15]]);
        }
        for (std::size_t index = 0; index < state.size(); ++index)
            state[index] ^= work[index] ^ work[index + 8];
    };
    std::array<char, 64 * 1024> input{};
    while (stream)
    {
        stream.read(input.data(), static_cast<std::streamsize>(input.size()));
        std::size_t remaining = static_cast<std::size_t>(stream.gcount());
        const unsigned char* current = reinterpret_cast<const unsigned char*>(input.data());
        while (remaining != 0)
        {
            if (blockSize == block.size())
            {
                const std::uint64_t previous = bytesLow;
                bytesLow += block.size();
                if (bytesLow < previous)
                    ++bytesHigh;
                compress(false);
                blockSize = 0;
            }
            const std::size_t count = std::min(remaining, block.size() - blockSize);
            std::memcpy(block.data() + blockSize, current, count);
            blockSize += count;
            current += count;
            remaining -= count;
        }
    }
    if (!stream.eof())
        throw std::runtime_error("could not hash bundled data-model patch");
    const std::uint64_t previous = bytesLow;
    bytesLow += blockSize;
    if (bytesLow < previous)
        ++bytesHigh;
    std::fill(block.begin() + blockSize, block.end(), 0);
    compress(true);
    std::array<unsigned char, 64> digest{};
    for (std::size_t index = 0; index < state.size(); ++index)
        for (unsigned int byte = 0; byte < 8; ++byte)
            digest[index * 8 + byte] = static_cast<unsigned char>(state[index] >> (byte * 8));
    const unsigned int digestSize = digest.size();
#else
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context)
        throw std::bad_alloc();
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestSize = 0;
    try
    {
        if (EVP_DigestInit_ex(context, EVP_blake2b512(), NULL) != 1)
            throw std::runtime_error("could not initialize BLAKE2b patch verification");
        std::array<char, 64 * 1024> buffer{};
        while (stream)
        {
            stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize count = stream.gcount();
            if (count > 0 && EVP_DigestUpdate(context, buffer.data(),
                                 static_cast<std::size_t>(count)) != 1)
                throw std::runtime_error("could not hash bundled data-model patch");
        }
        if (!stream.eof() || EVP_DigestFinal_ex(context, digest.data(), &digestSize) != 1)
            throw std::runtime_error("could not finish bundled patch verification");
    }
    catch (...)
    {
        EVP_MD_CTX_free(context);
        throw;
    }
    EVP_MD_CTX_free(context);
#endif

    std::ostringstream encoded;
    encoded << std::hex << std::setfill('0');
    for (unsigned int index = 0; index < digestSize; ++index)
        encoded << std::setw(2) << static_cast<unsigned int>(digest[index]);
    return encoded.str();
}

boost::shared_ptr<Instance> namedChild(Instance* parent, const char* name)
{
    if (!parent)
        return boost::shared_ptr<Instance>();
    for (std::size_t index = 0; index < parent->numChildren(); ++index)
    {
        Instance* child = parent->getChild(index);
        if (child->getName() == name)
            return shared_from(child);
    }
    return boost::shared_ptr<Instance>();
}

std::size_t mergeChildren(Instance* destination, Instance* patch)
{
    Instances children;
    children.reserve(patch->numChildren());
    for (std::size_t index = 0; index < patch->numChildren(); ++index)
        children.push_back(shared_from(patch->getChild(index)));

    std::size_t count = 0;
    for (const boost::shared_ptr<Instance>& child : children)
    {
        boost::shared_ptr<Instance> existing = namedChild(destination, child->getName().c_str());
        if (existing && child->numChildren() != 0)
            count += mergeChildren(existing.get(), child.get());
        else
        {
            if (existing)
                throw std::runtime_error("data-model patch conflicts with existing child: " +
                    child->getName());
            child->setParent(destination);
            ++count;
        }
    }
    return count;
}

} // namespace

DataModelPatch::Result DataModelPatch::applyBundled(DataModel* dataModel,
    const std::filesystem::path& modelPath, const std::filesystem::path& checksumPath)
{
    if (!dataModel)
        throw std::invalid_argument("data-model patch requires a DataModel");
    const std::string expectedChecksum = readText(checksumPath);
    const std::string actualChecksum = blake2b512(modelPath);
    if (expectedChecksum.size() != 128 || actualChecksum != expectedChecksum)
        throw std::runtime_error("bundled data-model patch failed BLAKE2b verification");

    std::ifstream stream(modelPath, std::ios::binary);
    Instances roots;
    SerializerV2().loadInstances(stream, roots);
    if (roots.size() != 1 || roots.front()->getName() != "PatchRoot")
        throw std::runtime_error("data-model patch has no unique PatchRoot");

    boost::shared_ptr<Instance> coreScripts = namedChild(roots.front().get(), "CoreScripts");
    boost::shared_ptr<Instance> dataModelInstances =
        namedChild(roots.front().get(), "DataModelInstances");
    if (!coreScripts)
        throw std::runtime_error("data-model patch has no CoreScripts branch");
    if (!dataModelInstances)
        throw std::runtime_error("data-model patch has no DataModelInstances branch");

    Result result;
    std::map<std::string, ProtectedString> sources;
    CoreGuiService* coreGui = ServiceProvider::create<CoreGuiService>(dataModel);
    CorePackages* corePackages = ServiceProvider::create<CorePackages>(dataModel);
    Instances coreScriptChildren;
    coreScriptChildren.reserve(coreScripts->numChildren());
    for (std::size_t index = 0; index < coreScripts->numChildren(); ++index)
        coreScriptChildren.push_back(shared_from(coreScripts->getChild(index)));
    for (const boost::shared_ptr<Instance>& childPointer : coreScriptChildren)
    {
        Instance* child = childPointer.get();
        if (ModuleScript* module = Instance::fastDynamicCast<ModuleScript>(child))
        {
            sources[module->getName()] = module->getSource();
            ++result.coreScriptCount;
        }
        else if (LocalizationTable* table = Instance::fastDynamicCast<LocalizationTable>(child))
            shared_from(table)->setParent(coreGui);
        else if (StringValue* assets = Instance::fastDynamicCast<StringValue>(child))
        {
            if (assets->getName() != "Assets")
                throw std::runtime_error("unknown StringValue in patch CoreScripts: " +
                    assets->getName());
            corePackages->setPatchAssetManifest(assets->getValue());
            result.assetManifestBytes = assets->getValue().size();
        }
        else
            throw std::runtime_error("unknown child in patch CoreScripts: " + child->getName());
    }
    if (sources.find("StarterScript") == sources.end() ||
        sources.find("CoreScripts/ExperienceChatMain") == sources.end())
        throw std::runtime_error("data-model patch is missing required in-experience CoreScripts");
    CoreScript::installPackagedSources(sources);

    for (std::size_t index = 0; index < dataModelInstances->numChildren(); ++index)
    {
        Instance* servicePatch = dataModelInstances->getChild(index);
        if (servicePatch->getName() == "CoreGui")
        {
            boost::shared_ptr<ScreenGui> robloxGui = coreGui->getRobloxScreenGui();
            boost::shared_ptr<Instance> patchRobloxGui = namedChild(servicePatch, "RobloxGui");
            if (!patchRobloxGui)
                throw std::runtime_error("CoreGui patch has no RobloxGui branch");
            result.dataModelInstanceCount += mergeChildren(robloxGui.get(), patchRobloxGui.get());
        }
        else if (servicePatch->getName() == "CorePackages")
            result.dataModelInstanceCount += mergeChildren(corePackages, servicePatch);
        else
            throw std::runtime_error("unknown DataModelInstances child: " +
                servicePatch->getName());
    }
    return result;
}

} // namespace RBX
