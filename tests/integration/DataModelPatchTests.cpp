#include "FastLog.h"
#include "Script/CoreScript.h"
#include "Script/LuaSettings.h"
#include "Util/Http.h"
#include "Util/Profiling.h"
#include "V8DataModel/CommonVerbs.h"
#include "V8DataModel/CorePackages.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/DataModelPatch.h"
#include "V8DataModel/DebugSettings.h"
#include "V8DataModel/FactoryRegistration.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/GameSettings.h"
#include "V8DataModel/PhysicsSettings.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/ScreenGui.h"
#include "security/SecurityContext.h"

#include <stdexcept>

int main(int argc, char** argv)
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
