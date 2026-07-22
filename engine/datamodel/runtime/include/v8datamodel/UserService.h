#pragma once

#include "v8tree/Service.h"

namespace RBX {

extern const char* const sUserService;

class UserService
    : public DescribedNonCreatable<UserService, Instance, sUserService>
    , public Service
{
public:
    UserService();

    void getUserInfosByUserIdsAsync(
        shared_ptr<const Reflection::ValueArray> userIds,
        boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void getUserFromGlobalUserIdAsync(long long userId,
        boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

private:
    static void processUserInfoResponse(std::string response,
        boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
};

} // namespace RBX
