#pragma once

#include "v8tree/Service.h"

#include <string>

namespace RBX {

extern const char* const sPolicyService;

class PolicyService
    : public DescribedNonCreatable<PolicyService, Instance, sPolicyService>
    , public Service
{
public:
    enum TriStateBoolean
    {
        TRI_STATE_UNKNOWN = 0,
        TRI_STATE_TRUE = 1,
        TRI_STATE_FALSE = 2
    };

    PolicyService();

    TriStateBoolean getIsLuobuServer() const { return isLuobuServer; }
    void setIsLuobuServer(TriStateBoolean value);
    TriStateBoolean getLuobuWhitelisted() const { return luobuWhitelisted; }
    void setLuobuWhitelisted(TriStateBoolean value);

    void canViewBrandProjectAsync(shared_ptr<Instance> player,
        std::string brandProjectId, boost::function<void(bool)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void getPolicyInfoForPlayerAsync(shared_ptr<Instance> player,
        boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void getPolicyInfoForServerRobloxOnlyAsync(
        boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

private:
    shared_ptr<const Reflection::ValueTable> buildPolicyInfo() const;

    TriStateBoolean isLuobuServer;
    TriStateBoolean luobuWhitelisted;
};

} // namespace RBX
