#include "FastLog.h"
#include "Script/CoreScript.h"
#include "Script/LuaSettings.h"
#include "util/Http.h"
#include "util/Profiling.h"
#include "v8datamodel/CommonVerbs.h"
#include "v8datamodel/CorePackages.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/DataModelPatch.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/factoryregistration.h"
#include "v8datamodel/GameBasicSettings.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/PhysicsSettings.h"
#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/ScreenGui.h"
#include "security/SecurityContext.h"

#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    try
    {
    if (argc != 3)
        throw std::runtime_error("expected bundled patch and checksum paths");

    RBX::Profiling::init(false);
    static RBX::FactoryRegistrator factoryObjects;
    RBX::Http::init(RBX::Http::WinHttp,
        RBX::Http::CookieSharingSingleProcessMultipleThreads);
    RBX::GameSettings::singleton();
    RBX::LuaSettings::singleton();
    RBX::DebugSettings::singleton();
    RBX::PhysicsSettings::singleton();
    RBX::GameBasicSettings::singleton();

    boost::shared_ptr<RBX::DataModel> dataModel = RBX::DataModel::createDataModel(
        true, new RBX::NullVerb(nullptr, ""), false);
    {
        RBX::Security::Impersonator permission(RBX::Security::COM);
        RBX::DataModel::LegacyLock lock(dataModel.get(), RBX::DataModelJob::Write);
        const RBX::DataModelPatch::Result result =
            RBX::DataModelPatch::applyBundled(dataModel.get(), argv[1], argv[2]);
        if (result.coreScriptCount != 74 || result.dataModelInstanceCount != 3 ||
            result.assetManifestBytes != 43073)
            throw std::runtime_error("authoritative patch applied with unexpected content counts");

        RBX::CoreGuiService* coreGui =
            RBX::ServiceProvider::find<RBX::CoreGuiService>(dataModel.get());
        RBX::CorePackages* corePackages =
            RBX::ServiceProvider::find<RBX::CorePackages>(dataModel.get());
        if (!coreGui || !coreGui->getRobloxScreenGui()->findFirstChildByName("Modules"))
            throw std::runtime_error("authoritative CoreGui Modules were not mounted");
        if (!corePackages || !corePackages->findFirstChildByName("Packages") ||
            !corePackages->findFirstChildByName("Workspace") ||
            corePackages->getPatchAssetManifest().size() != 43073)
            throw std::runtime_error("authoritative CorePackages were not mounted");

        const boost::optional<RBX::ProtectedString> starter =
            RBX::CoreScript::fetchSource("StarterScript");
        if (!starter || starter->getBytecode().empty() ||
            static_cast<unsigned char>(starter->getBytecode().front()) != 0x52)
            throw std::runtime_error("authoritative signed StarterScript was not installed");
    }
    RBX::DataModel::closeDataModel(dataModel);
    return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "data-model patch contract failed: " << error.what() << '\n';
        return 1;
    }
}
