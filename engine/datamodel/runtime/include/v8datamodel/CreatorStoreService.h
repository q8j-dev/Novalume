#pragma once

#include "v8tree/Service.h"

namespace RBX {

extern const char* const sCreatorStoreService;

class CreatorStoreService
    : public DescribedNonCreatable<CreatorStoreService, Instance, sCreatorStoreService>
    , public Service
{
public:
    CreatorStoreService();

    void getAssetInfoAsync(long long assetId,
        boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void getCreatorStoreProductInfoAsync(long long productTargetId,
        std::string assetType,
        boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void performCreatorStorePurchase(long long productTargetId,
        std::string assetType,
        boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

private:
    static void finishJsonRequest(const char* operation,
        boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
        boost::function<void(std::string)> errorFunction,
        std::string* response, std::exception* exception);
};

}
