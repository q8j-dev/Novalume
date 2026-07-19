#include "V8DataModel/DataModelPatch.h"

#include "Script/CoreScript.h"
#include "Script/ModuleScript.h"
#include "V8DataModel/CorePackages.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/LocalizationTable.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/ScreenGui.h"
#include "V8DataModel/Value.h"
#include "V8Xml/SerializerV2.h"

#include <openssl/evp.h>

#include <array>
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
