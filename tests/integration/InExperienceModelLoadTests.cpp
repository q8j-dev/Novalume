#include "Script/ModuleScript.h"
#include "V8DataModel/FactoryRegistration.h"
#include "V8DataModel/LocalizationTable.h"
#include "V8DataModel/Value.h"
#include "V8Xml/SerializerV2.h"

#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>

int main(int argc, char** argv)
{
    if (argc != 2)
        throw std::runtime_error("expected the authoritative InExperience.rbxm path");

    static std::once_flag registration;
    std::call_once(registration, [] { static RBX::FactoryRegistrator registrator; });

    std::ifstream stream(argv[1], std::ios::binary);
    if (!stream)
        throw std::runtime_error("could not open the authoritative InExperience model");

    RBX::Instances roots;
    SerializerV2().loadInstances(stream, roots);
    std::size_t instanceCount = roots.size();
    std::size_t moduleCount = 0;
    std::size_t localizationTableCount = 0;
    bool packagedTranslationVerified = false;
    for (const boost::shared_ptr<RBX::Instance>& root : roots)
    {
        if (RBX::Instance::fastDynamicCast<RBX::ModuleScript>(root.get()))
            ++moduleCount;
        if (RBX::LocalizationTable* table =
                RBX::Instance::fastDynamicCast<RBX::LocalizationTable>(root.get()))
        {
            ++localizationTableCount;
            if (table->getName() == "CoreScriptLocalization")
                packagedTranslationVerified =
                    table->format("de", "Network", RBX::Reflection::Variant()) == "(Netzwerk)";
        }
        root->visitDescendants([&](const boost::shared_ptr<RBX::Instance>& instance) {
            ++instanceCount;
            if (RBX::Instance::fastDynamicCast<RBX::ModuleScript>(instance.get()))
                ++moduleCount;
            if (RBX::LocalizationTable* table =
                    RBX::Instance::fastDynamicCast<RBX::LocalizationTable>(instance.get()))
            {
                ++localizationTableCount;
                if (table->getName() == "CoreScriptLocalization")
                    packagedTranslationVerified =
                        table->format("de", "Network", RBX::Reflection::Variant()) == "(Netzwerk)";
            }
        });
    }

    if (roots.size() != 1 || roots.front()->getName() != "PatchRoot" ||
        roots.front()->numChildren() != 2)
        throw std::runtime_error("authoritative package does not contain the expected PatchRoot");
    RBX::Instance* coreScripts = roots.front()->findFirstChildByName("CoreScripts");
    RBX::Instance* instances = roots.front()->findFirstChildByName("DataModelInstances");
    if (!coreScripts || coreScripts->numChildren() != 77 || !instances ||
        instances->numChildren() != 2)
        throw std::runtime_error("authoritative package branches have unexpected structure");
    RBX::StringValue* assets = RBX::Instance::fastDynamicCast<RBX::StringValue>(
        coreScripts->findFirstChildByName("Assets"));
    if (!assets || assets->getValue().size() != 43073 ||
        !instances->findFirstChildByName("CoreGui") ||
        !instances->findFirstChildByName("CorePackages"))
        throw std::runtime_error("authoritative package assets or service branches are missing");

    if (instanceCount != 26634 || moduleCount != 24388)
    {
        std::cerr << "loaded " << instanceCount << " instances and " << moduleCount
                  << " modules\n";
        return 1;
    }
    if (localizationTableCount != 2 || !packagedTranslationVerified)
    {
        std::cerr << "loaded " << localizationTableCount
                  << " localization tables; packaged translation verified="
                  << packagedTranslationVerified << "\n";
        return 1;
    }

    // Translation lookups are paint-path operations. Verify the parsed cache
    // preserves key/source semantics and is atomically rebuilt with Contents.
    RBX::LocalizationTable cacheTable;
    cacheTable.setContents(
        R"([{"Key":"Greeting","Source":"Hello","Values":{"de":"Hallo"}}])");
    if (cacheTable.format("de", "Greeting", RBX::Reflection::Variant()) != "Hallo" ||
        cacheTable.translate("de", "Hello") != "Hallo")
        throw std::runtime_error("localization lookup cache changed table semantics");
    cacheTable.setContents(
        R"([{"Key":"Greeting","Source":"Hello","Values":{"de":"Guten Tag"}}])");
    if (cacheTable.translate("de", "Hello") != "Guten Tag")
        throw std::runtime_error("localization lookup cache was not invalidated");
    return 0;
}
