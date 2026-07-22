#include "v8datamodel/CreatorStoreService.h"

#include "util/Http.h"
#include "v8xml/WebParser.h"

#include <boost/algorithm/string.hpp>

namespace RBX {

const char* const sCreatorStoreService = "CreatorStoreService";

REFLECTION_BEGIN();
static Reflection::BoundYieldFuncDesc<CreatorStoreService,
    shared_ptr<const Reflection::ValueTable>(long long)> funcGetAssetInfoAsync(
        &CreatorStoreService::getAssetInfoAsync, "GetAssetInfoAsync", "assetId",
        Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<CreatorStoreService,
    shared_ptr<const Reflection::ValueTable>(long long, std::string)>
    funcGetCreatorStoreProductInfoAsync(
        &CreatorStoreService::getCreatorStoreProductInfoAsync,
        "GetCreatorStoreProductInfoAsync", "productTargetId", "assetType",
        Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<CreatorStoreService,
    shared_ptr<const Reflection::ValueTable>(long long, std::string)>
    funcPerformCreatorStorePurchase(
        &CreatorStoreService::performCreatorStorePurchase,
        "PerformCreatorStorePurchase", "productTargetId", "assetType",
        Security::RobloxScript);
REFLECTION_END();

CreatorStoreService::CreatorStoreService()
{
    setName(sCreatorStoreService);
    setRobloxLocked(true);
}

void CreatorStoreService::finishJsonRequest(const char* operation,
    boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
    boost::function<void(std::string)> errorFunction,
    std::string* response, std::exception* exception)
{
    if (exception)
    {
        errorFunction(format("CreatorStoreService::%s() %s", operation,
            exception->what()));
        return;
    }
    if (!response)
    {
        errorFunction(format("CreatorStoreService::%s() received no response",
            operation));
        return;
    }

    try
    {
        shared_ptr<const Reflection::ValueTable> result;
        WebParser::parseJSONTable(*response, result);
        resumeFunction(result);
    }
    catch (const std::exception& parseError)
    {
        errorFunction(format("CreatorStoreService::%s() an error occurred while parsing web response: %s",
            operation, parseError.what()));
    }
}

void CreatorStoreService::getAssetInfoAsync(long long assetId,
    boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (assetId <= 0)
    {
        errorFunction("Invalid AssetId");
        return;
    }
    const std::string url = format("https://develop.roblox.com/v1/assets/%lld", assetId);
    Http request(url);
    request.get(boost::bind(&CreatorStoreService::finishJsonRequest,
        "getAssetInfoAsync", resumeFunction, errorFunction, _1, _2));
}

void CreatorStoreService::getCreatorStoreProductInfoAsync(long long productTargetId,
    std::string assetType,
    boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (productTargetId <= 0)
    {
        errorFunction("Invalid ProductTargetId");
        return;
    }
    if (assetType.empty())
    {
        errorFunction("Missing AssetType");
        return;
    }
    const std::string url = format(
        "https://apis.roblox.com/user/cloud/v2/creator-store-products/%lld",
        productTargetId);
    Http request(url);
    request.get(boost::bind(&CreatorStoreService::finishJsonRequest,
        "getCreatorStoreProductInfoAsync", resumeFunction, errorFunction, _1, _2));
}

void CreatorStoreService::performCreatorStorePurchase(long long productTargetId,
    std::string assetType,
    boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (productTargetId <= 0)
    {
        errorFunction("Invalid ProductTargetId");
        return;
    }
    if (assetType.empty())
    {
        errorFunction("Missing AssetType");
        return;
    }

    // The purchase endpoint verifies expectedPrice. Fetch the authoritative
    // creator-store product first instead of reproducing the old client bug
    // that always sent a zero USD price for paid plugins.
    getCreatorStoreProductInfoAsync(productTargetId, assetType,
        [productTargetId, assetType, resumeFunction, errorFunction](
            shared_ptr<const Reflection::ValueTable> product) mutable {
            shared_ptr<const Reflection::ValueTable> basePrice;
            shared_ptr<const Reflection::ValueTable> quantity;
            std::string currencyCode;
            long long significand = 0;
            int exponent = 0;

            try
            {
                Reflection::ValueTable::const_iterator priceIt = product->find("basePrice");
                if (priceIt == product->end() ||
                    !priceIt->second.isType<shared_ptr<const Reflection::ValueTable> >())
                    throw std::runtime_error("creator-store response has no basePrice");
                basePrice = priceIt->second.cast<shared_ptr<const Reflection::ValueTable> >();

                Reflection::ValueTable::const_iterator currencyIt = basePrice->find("currencyCode");
                Reflection::ValueTable::const_iterator quantityIt = basePrice->find("quantity");
                if (currencyIt == basePrice->end() ||
                    !currencyIt->second.isType<std::string>() ||
                    quantityIt == basePrice->end() ||
                    !quantityIt->second.isType<shared_ptr<const Reflection::ValueTable> >())
                    throw std::runtime_error("creator-store response has an invalid basePrice");
                currencyCode = currencyIt->second.cast<std::string>();
                quantity = quantityIt->second.cast<shared_ptr<const Reflection::ValueTable> >();

                Reflection::ValueTable::const_iterator significandIt = quantity->find("significand");
                Reflection::ValueTable::const_iterator exponentIt = quantity->find("exponent");
                if (significandIt == quantity->end() || exponentIt == quantity->end())
                    throw std::runtime_error("creator-store response has an invalid price quantity");
                if (significandIt->second.isType<long long>())
                    significand = significandIt->second.cast<long long>();
                else if (significandIt->second.isType<int>())
                    significand = significandIt->second.cast<int>();
                else
                    throw std::runtime_error("creator-store price significand is not integral");
                if (exponentIt->second.isType<int>())
                    exponent = exponentIt->second.cast<int>();
                else if (exponentIt->second.isType<long long>())
                    exponent = static_cast<int>(exponentIt->second.cast<long long>());
                else
                    throw std::runtime_error("creator-store price exponent is not integral");
            }
            catch (const std::exception& exception)
            {
                errorFunction(exception.what());
                return;
            }

            boost::to_upper(assetType);
            const std::string body = format(
                "{\"productKey\":{\"productNamespace\":\"PRODUCT_NAMESPACE_CREATOR_MARKETPLACE_ASSET\","
                "\"productType\":\"PRODUCT_TYPE_%s\",\"productTargetId\":\"%lld\"},"
                "\"expectedPrice\":{\"currencyCode\":\"%s\",\"quantity\":{\"significand\":%lld,\"exponent\":%d}}}",
                assetType.c_str(), productTargetId, currencyCode.c_str(), significand, exponent);
            Http request("https://apis.roblox.com/marketplace-fiat-service/v1/product/purchase");
            request.post(body, Http::kContentTypeApplicationJson, false,
                boost::bind(&CreatorStoreService::finishJsonRequest,
                    "performCreatorStorePurchase", resumeFunction, errorFunction, _1, _2), true);
        }, errorFunction);
}

}
