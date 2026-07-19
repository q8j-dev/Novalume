#include "FastLog.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

FASTFLAGVARIABLE(ContractStaticFlag, false)
DYNAMIC_FASTINTVARIABLE(ContractDynamicInt, 7)
FASTSTRINGVARIABLE(ContractStaticString, "initial")
SYNCHRONIZED_FASTFLAGVARIABLE(ContractSynchronizedFlag, false)

namespace {

void collect(const std::string& name, const std::string&, void* context) {
    static_cast<std::vector<std::string>*>(context)->push_back(name);
}

} // namespace

int main() {
    assert(FLog::SetValue("ContractStaticFlag", "true", FASTVARTYPE_STATIC));
    assert(FFlag::ContractStaticFlag);

    assert(!FLog::SetValue("ContractDynamicInt", "9", FASTVARTYPE_STATIC));
    assert(DFInt::ContractDynamicInt == 7);
    assert(FLog::SetValue("ContractDynamicInt", "9", FASTVARTYPE_DYNAMIC));
    assert(DFInt::ContractDynamicInt == 9);

    assert(FLog::SetValue("ContractStaticString", "updated"));
    std::string value;
    assert(FLog::GetValue("ContractStaticString", value));
    assert(value == "updated");

    std::vector<std::string> dynamicNames;
    FLog::ForEachVariable(&collect, &dynamicNames, FASTVARTYPE_DYNAMIC);
    assert(std::find(dynamicNames.begin(), dynamicNames.end(), "ContractDynamicInt") != dynamicNames.end());
    assert(std::find(dynamicNames.begin(), dynamicNames.end(), "ContractStaticFlag") == dynamicNames.end());

    assert(!*SFFlag::ContractSynchronizedFlagIsSync);
    assert(FLog::SetValueFromServer("ContractSynchronizedFlag", "true"));
    assert(SFFlag::ContractSynchronizedFlag);
    assert(*SFFlag::ContractSynchronizedFlagIsSync);
    FLog::ResetSynchronizedVariablesState();
    assert(!*SFFlag::ContractSynchronizedFlagIsSync);

    assert(!FLog::SetValue("RegisteredAfterOverride", "42", FASTVARTYPE_STATIC));
    int registeredAfterOverride = 0;
    FLog::RegisterLogGroup("RegisteredAfterOverride", &registeredAfterOverride);
    assert(registeredAfterOverride == 42);
    return 0;
}
