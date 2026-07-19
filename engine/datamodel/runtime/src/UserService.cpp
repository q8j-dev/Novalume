#include "V8DataModel/UserService.h"

#include "Network/Player.h"
#include "Network/Players.h"
#include "Util/Http.h"
#include "V8DataModel/DataModel.h"
#include "V8Xml/WebParser.h"

#include <sstream>
#include <limits>
#include <vector>

namespace RBX {

const char* const sUserService = "UserService";

REFLECTION_BEGIN();
static Reflection::BoundYieldFuncDesc<UserService,
    shared_ptr<const Reflection::ValueArray>(shared_ptr<const Reflection::ValueArray>)>
        funcGetUserInfosByUserIdsAsync(&UserService::getUserInfosByUserIdsAsync,
            "GetUserInfosByUserIdsAsync", "userIds", Security::None);
static Reflection::BoundYieldFuncDesc<UserService,
    shared_ptr<const Reflection::ValueTable>(long long)>
    funcGetUserFromGlobalUserIdAsync(&UserService::getUserFromGlobalUserIdAsync,
        "GetUserFromGlobalUserIdAsync", "userId", Security::None);
REFLECTION_END();

UserService::UserService()
    : Service(true)
{
    setName(sUserService);
    setRobloxLocked(true);
}

namespace {

bool readUserId(const Reflection::Variant& value, long long& userId)
{
    if (value.isType<long long>())
        userId = value.cast<long long>();
    else if (value.isType<int>())
        userId = value.cast<int>();
    else if (value.isType<double>())
        userId = static_cast<long long>(value.cast<double>());
    else
        return false;
    return true;
}

shared_ptr<const Reflection::ValueTable> playerInfo(
    const Network::Player& player)
{
    shared_ptr<Reflection::ValueTable> info(new Reflection::ValueTable());
    (*info)["Id"] = Reflection::Variant(static_cast<long long>(player.getUserID()));
    (*info)["Username"] = Reflection::Variant(player.getName());
    (*info)["DisplayName"] = Reflection::Variant(player.getName());
    (*info)["HasVerifiedBadge"] = Reflection::Variant(false);
    return info;
}

const Reflection::Variant* findValue(const Reflection::ValueTable& table,
    const char* first, const char* second = NULL)
{
    Reflection::ValueTable::const_iterator value = table.find(first);
    if (value != table.end())
        return &value->second;
    if (second)
    {
        value = table.find(second);
        if (value != table.end())
            return &value->second;
    }
    return NULL;
}

} // namespace

void UserService::getUserInfosByUserIdsAsync(
    shared_ptr<const Reflection::ValueArray> userIds,
    boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (!userIds)
    {
        errorFunction("UserService:GetUserInfosByUserIdsAsync() expected an array");
        return;
    }
    if (userIds->size() > 200)
    {
        errorFunction("UserService:GetUserInfosByUserIdsAsync() accepts at most 200 userIds");
        return;
    }

    Network::Players* players = ServiceProvider::find<Network::Players>(this);
    shared_ptr<Reflection::ValueArray> result(new Reflection::ValueArray());
    std::vector<long long> unresolved;
    for (Reflection::ValueArray::const_iterator it = userIds->begin();
         it != userIds->end(); ++it)
    {
        long long userId = 0;
        if (!readUserId(*it, userId) || userId < 0)
        {
            errorFunction("UserService:GetUserInfosByUserIdsAsync() expected valid userIds");
            return;
        }

        shared_ptr<Network::Player> player;
        if (players && userId <= static_cast<long long>(std::numeric_limits<int>::max()))
            player = players->getPlayerByID(static_cast<int>(userId));
        if (player)
            result->push_back(Reflection::Variant(playerInfo(*player)));
        else
            unresolved.push_back(userId);
    }

    if (unresolved.empty())
    {
        resumeFunction(shared_ptr<const Reflection::ValueArray>(result));
        return;
    }

    std::ostringstream body;
    body << "{\"userIds\":[";
    for (std::size_t i = 0; i < unresolved.size(); ++i)
    {
        if (i)
            body << ',';
        body << unresolved[i];
    }
    body << "],\"excludeBannedUsers\":false}";

    Http request("https://users.roblox.com/v1/users");
    request.post(body.str(), Http::kContentTypeApplicationJson, false,
        [resumeFunction, errorFunction](std::string* response,
            std::exception* exception) {
            if (exception)
            {
                errorFunction(exception->what());
                return;
            }
            if (!response)
            {
                errorFunction("UserService:GetUserInfosByUserIdsAsync() received no response");
                return;
            }
            UserService::processUserInfoResponse(*response, resumeFunction,
                errorFunction);
        }, true);
}

void UserService::processUserInfoResponse(std::string response,
    boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    shared_ptr<const Reflection::ValueTable> root;
    if (!WebParser::parseJSONTable(response, root) || !root)
    {
        errorFunction("UserService:GetUserInfosByUserIdsAsync() received an invalid response");
        return;
    }
    const Reflection::Variant* dataValue = findValue(*root, "data", "Data");
    if (!dataValue || !dataValue->isType<shared_ptr<const Reflection::ValueArray> >())
    {
        errorFunction("UserService:GetUserInfosByUserIdsAsync() response did not contain user data");
        return;
    }

    shared_ptr<const Reflection::ValueArray> data =
        dataValue->cast<shared_ptr<const Reflection::ValueArray> >();
    shared_ptr<Reflection::ValueArray> result(new Reflection::ValueArray());
    for (Reflection::ValueArray::const_iterator it = data->begin();
         it != data->end(); ++it)
    {
        if (!it->isType<shared_ptr<const Reflection::ValueTable> >())
            continue;
        shared_ptr<const Reflection::ValueTable> source =
            it->cast<shared_ptr<const Reflection::ValueTable> >();
        const Reflection::Variant* id = findValue(*source, "id", "Id");
        const Reflection::Variant* username = findValue(*source, "name", "Username");
        const Reflection::Variant* displayName = findValue(*source, "displayName", "DisplayName");
        const Reflection::Variant* verified = findValue(*source, "hasVerifiedBadge", "HasVerifiedBadge");
        if (!id || !username || !displayName)
            continue;

        shared_ptr<Reflection::ValueTable> info(new Reflection::ValueTable());
        (*info)["Id"] = *id;
        (*info)["Username"] = *username;
        (*info)["DisplayName"] = *displayName;
        (*info)["HasVerifiedBadge"] = verified ? *verified : Reflection::Variant(false);
        result->push_back(Reflection::Variant(
            shared_ptr<const Reflection::ValueTable>(info)));
    }
    resumeFunction(shared_ptr<const Reflection::ValueArray>(result));
}

void UserService::getUserFromGlobalUserIdAsync(long long,
    boost::function<void(shared_ptr<const Reflection::ValueTable>)>,
    boost::function<void(std::string)> errorFunction)
{
    // This is the observed default path in the supplied client: the User data
    // type and its server-only feature gate are disabled in ordinary players.
    errorFunction("UserService: GetUserFromGlobalUserIdAsync is not yet enabled");
}

} // namespace RBX
