#pragma once

#include "v8tree/Service.h"

#include "rbx/signal.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace RBX {

extern const char* const sSessionService;

class SessionService
    : public DescribedNonCreatable<SessionService, Instance, sSessionService>
    , public Service
{
public:
    SessionService();

    void acquireContextFocus(std::string context);
    std::string generateSessionInfoString(bool includeArbitrarySessions,
        bool includeTag, bool includeTimestamps, bool includeMetadata);
    shared_ptr<const Reflection::ValueArray> getBreadcrumbs();
    long long getCreatedTimestampUtcMs(std::string sid);
    shared_ptr<const Reflection::ValueArray> getHistory();
    Reflection::Variant getMetadata(std::string sid, std::string key);
    std::string getRootSID();
    std::string getSessionID(std::string structuralId);
    std::string getSessionTag(std::string sid);
    bool isContextFocused(std::string context);
    void releaseContextFocus(std::string context);
    void removeMetadata(std::string sid, std::string key, std::string context);
    void removeSession(std::string sid, std::string context);
    void removeSessionsWithMetadataKey(std::string key);
    void replaceSession(std::string sid, std::string tag);
    bool sessionExists(std::string sid);
    void setMetadata(std::string sid, std::string key, Reflection::Variant value,
        std::string context);
    void setSession(std::string parentSid, std::string childSid, std::string tag,
        std::string context);

    rbx::signal<void(std::string, std::string, std::string, std::string,
        std::string)> sessionChangedSignal;

private:
    struct Node
    {
        std::string structuralId;
        std::string sessionId;
        std::string tag;
        std::string parent;
        long long createdUtcMs;
        std::map<std::string, Reflection::Variant> metadata;
        std::vector<std::string> children;
    };

    static std::string newSessionId();
    static long long nowUtcMs();
    void removeSessionTree(const std::string& sid, const std::string& context);
    void recordHistory(const std::string& operation, const std::string& sid);
    void fireChanged(const std::string& sid, const std::string& currentTag,
        const std::string& currentSessionId, const std::string& previousTag,
        const std::string& previousSessionId);

    std::map<std::string, Node> sessions;
    std::vector<Reflection::Variant> history;
    std::vector<std::string> focusStack;
    std::string rootStructuralId;
};

} // namespace RBX
