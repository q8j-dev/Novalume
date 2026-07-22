#include "v8datamodel/SessionService.h"

#include "util/Guid.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace RBX {

const char* const sSessionService = "SessionService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<SessionService, void(std::string)>
    funcAcquireContextFocus(&SessionService::acquireContextFocus,
        "AcquireContextFocus", "context", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService,
    std::string(bool, bool, bool, bool)> funcGenerateSessionInfoString(
        &SessionService::generateSessionInfoString, "GenerateSessionInfoString",
        "includeArbitrarySessions", "includeTag", "includeTimestamps",
        "includeMetadata", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService,
    shared_ptr<const Reflection::ValueArray>()> funcGetBreadcrumbs(
        &SessionService::getBreadcrumbs, "GetBreadcrumbs", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService, long long(std::string)>
    funcGetCreatedTimestampUtcMs(&SessionService::getCreatedTimestampUtcMs,
        "GetCreatedTimestampUtcMs", "sid", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService,
    shared_ptr<const Reflection::ValueArray>()> funcGetHistory(
        &SessionService::getHistory, "GetHistory", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService,
    Reflection::Variant(std::string, std::string)> funcGetMetadata(
        &SessionService::getMetadata, "GetMetadata", "sid", "key",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService, std::string()> funcGetRootSID(
    &SessionService::getRootSID, "GetRootSID", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService, std::string(std::string)>
    funcGetSessionID(&SessionService::getSessionID, "GetSessionID",
        "structuralId", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService, std::string(std::string)>
    funcGetSessionTag(&SessionService::getSessionTag, "GetSessionTag", "sid",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService, bool(std::string)>
    funcIsContextFocused(&SessionService::isContextFocused, "IsContextFocused",
        "context", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService, void(std::string)>
    funcReleaseContextFocus(&SessionService::releaseContextFocus,
        "ReleaseContextFocus", "context", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService,
    void(std::string, std::string, std::string)> funcRemoveMetadata(
        &SessionService::removeMetadata, "RemoveMetadata", "sid", "key",
        "context", std::string(), Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService, void(std::string, std::string)>
    funcRemoveSession(&SessionService::removeSession, "RemoveSession", "sid",
        "context", std::string(), Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService, void(std::string)>
    funcRemoveSessionsWithMetadataKey(&SessionService::removeSessionsWithMetadataKey,
        "RemoveSessionsWithMetadataKey", "key", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService, void(std::string, std::string)>
    funcReplaceSession(&SessionService::replaceSession, "ReplaceSession", "sid",
        "tag", Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService, bool(std::string)>
    funcSessionExists(&SessionService::sessionExists, "SessionExists", "sid",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService,
    void(std::string, std::string, Reflection::Variant, std::string)>
        funcSetMetadata(&SessionService::setMetadata, "SetMetadata", "sid", "key",
            "value", "context", std::string(), Security::RobloxScript);
static Reflection::BoundFuncDesc<SessionService,
    void(std::string, std::string, std::string, std::string)> funcSetSession(
        &SessionService::setSession, "SetSession", "parentSid", "childSid", "tag",
        "context", std::string(), Security::RobloxScript);
static Reflection::EventDesc<SessionService,
    void(std::string, std::string, std::string, std::string, std::string)>
        eventSessionChanged(&SessionService::sessionChangedSignal, "SessionChanged",
            "structuralId", "currentTag", "currentSessionId", "previousTag",
            "previousSessionId", Security::RobloxScript);
REFLECTION_END();

std::string SessionService::newSessionId()
{
    std::string result;
    Guid::generateStandardGUID(result);
    if (result.size() > 2 && result.front() == '{' && result.back() == '}')
        result = result.substr(1, result.size() - 2);
    return result;
}

long long SessionService::nowUtcMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

SessionService::SessionService()
    : Service(true)
    , rootStructuralId("level0")
{
    setName(sSessionService);
    setRobloxLocked(true);
    Node root;
    root.structuralId = rootStructuralId;
    root.sessionId = newSessionId();
    root.tag = "AppSession";
    root.createdUtcMs = nowUtcMs();
    sessions[root.structuralId] = root;
}

void SessionService::recordHistory(const std::string& operation,
    const std::string& sid)
{
    shared_ptr<Reflection::ValueTable> entry(new Reflection::ValueTable());
    (*entry)["operation"] = Reflection::Variant(operation);
    (*entry)["structuralId"] = Reflection::Variant(sid);
    (*entry)["timestampUtcMs"] = Reflection::Variant(static_cast<double>(nowUtcMs()));
    history.push_back(Reflection::Variant(
        shared_ptr<const Reflection::ValueTable>(entry)));
}

void SessionService::fireChanged(const std::string& sid,
    const std::string& currentTag, const std::string& currentSessionId,
    const std::string& previousTag, const std::string& previousSessionId)
{
    sessionChangedSignal(sid, currentTag, currentSessionId, previousTag,
        previousSessionId);
}

void SessionService::acquireContextFocus(std::string context)
{
    if (context.empty() || (!focusStack.empty() && focusStack.back() == context))
        return;
    focusStack.erase(std::remove(focusStack.begin(), focusStack.end(), context),
        focusStack.end());
    focusStack.push_back(std::move(context));
}

void SessionService::releaseContextFocus(std::string context)
{
    focusStack.erase(std::remove(focusStack.begin(), focusStack.end(), context),
        focusStack.end());
}

bool SessionService::isContextFocused(std::string context)
{
    return !focusStack.empty() && focusStack.back() == context;
}

std::string SessionService::generateSessionInfoString(bool includeArbitrarySessions,
    bool includeTag, bool includeTimestamps, bool includeMetadata)
{
    std::ostringstream result;
    bool first = true;
    for (std::map<std::string, Node>::const_iterator it = sessions.begin();
         it != sessions.end(); ++it)
    {
        const Node& node = it->second;
        const bool knownLevel = node.structuralId == "level0" ||
            node.structuralId == "level1" || node.structuralId == "level2a" ||
            node.structuralId == "level2b";
        if (!includeArbitrarySessions && !knownLevel)
            continue;
        if (!first)
            result << ',';
        first = false;
        result << node.structuralId << ".SID." << node.sessionId;
        if (includeTag)
            result << ".SD." << node.tag;
        if (includeTimestamps)
            result << ".CT." << node.createdUtcMs;
        if (includeMetadata)
            for (std::map<std::string, Reflection::Variant>::const_iterator metadata =
                     node.metadata.begin(); metadata != node.metadata.end(); ++metadata)
                result << ".MD." << metadata->first;
    }
    return result.str();
}

shared_ptr<const Reflection::ValueArray> SessionService::getBreadcrumbs()
{
    shared_ptr<Reflection::ValueArray> result(new Reflection::ValueArray());
    std::string sid = rootStructuralId;
    while (!sid.empty())
    {
        std::map<std::string, Node>::const_iterator node = sessions.find(sid);
        if (node == sessions.end())
            break;
        result->push_back(Reflection::Variant(node->second.structuralId));
        sid = node->second.children.empty() ? std::string() :
            node->second.children.back();
    }
    return result;
}

long long SessionService::getCreatedTimestampUtcMs(std::string sid)
{
    std::map<std::string, Node>::const_iterator node = sessions.find(sid);
    return node == sessions.end() ? 0 : node->second.createdUtcMs;
}

shared_ptr<const Reflection::ValueArray> SessionService::getHistory()
{
    return shared_ptr<const Reflection::ValueArray>(
        new Reflection::ValueArray(history.begin(), history.end()));
}

Reflection::Variant SessionService::getMetadata(std::string sid, std::string key)
{
    std::map<std::string, Node>::const_iterator node = sessions.find(sid);
    if (node == sessions.end())
        return Reflection::Variant();
    std::map<std::string, Reflection::Variant>::const_iterator value =
        node->second.metadata.find(key);
    return value == node->second.metadata.end() ? Reflection::Variant() : value->second;
}

std::string SessionService::getRootSID() { return sessions[rootStructuralId].sessionId; }
std::string SessionService::getSessionID(std::string structuralId)
{
    std::map<std::string, Node>::const_iterator node = sessions.find(structuralId);
    return node == sessions.end() ? std::string() : node->second.sessionId;
}
std::string SessionService::getSessionTag(std::string sid)
{
    std::map<std::string, Node>::const_iterator node = sessions.find(sid);
    return node == sessions.end() ? std::string() : node->second.tag;
}
bool SessionService::sessionExists(std::string sid)
{ return sessions.find(sid) != sessions.end(); }

void SessionService::setMetadata(std::string sid, std::string key,
    Reflection::Variant value, std::string context)
{
    std::map<std::string, Node>::iterator node = sessions.find(sid);
    if (node == sessions.end())
        return;
    node->second.metadata[std::move(key)] = value;
    recordHistory(context.empty() ? "SetMetadata" : context, sid);
}

void SessionService::removeMetadata(std::string sid, std::string key,
    std::string context)
{
    std::map<std::string, Node>::iterator node = sessions.find(sid);
    if (node == sessions.end())
        return;
    node->second.metadata.erase(key);
    recordHistory(context.empty() ? "RemoveMetadata" : context, sid);
}

void SessionService::setSession(std::string parentSid, std::string childSid,
    std::string tag, std::string context)
{
    if (childSid.empty() || childSid == rootStructuralId)
        return;
    if (sessions.find(childSid) != sessions.end())
        removeSessionTree(childSid, context);
    if (sessions.find(parentSid) == sessions.end())
        parentSid = rootStructuralId;

    Node child;
    child.structuralId = childSid;
    child.sessionId = newSessionId();
    child.tag = tag;
    child.parent = parentSid;
    child.createdUtcMs = nowUtcMs();
    sessions[parentSid].children.push_back(childSid);
    sessions[childSid] = child;
    recordHistory(context.empty() ? "SetSession" : context, childSid);
    fireChanged(childSid, tag, child.sessionId, std::string(), std::string());
}

void SessionService::replaceSession(std::string sid, std::string tag)
{
    std::map<std::string, Node>::iterator node = sessions.find(sid);
    if (node == sessions.end())
        return;
    const std::string previousTag = node->second.tag;
    const std::string previousId = node->second.sessionId;
    node->second.tag = tag;
    node->second.sessionId = newSessionId();
    node->second.createdUtcMs = nowUtcMs();
    node->second.metadata.clear();
    recordHistory("ReplaceSession", sid);
    fireChanged(sid, tag, node->second.sessionId, previousTag, previousId);
}

void SessionService::removeSessionTree(const std::string& sid,
    const std::string& context)
{
    std::map<std::string, Node>::iterator node = sessions.find(sid);
    if (node == sessions.end() || sid == rootStructuralId)
        return;
    const Node removed = node->second;
    const std::vector<std::string> children = removed.children;
    for (std::vector<std::string>::const_iterator child = children.begin();
         child != children.end(); ++child)
        removeSessionTree(*child, context);
    std::map<std::string, Node>::iterator parent = sessions.find(removed.parent);
    if (parent != sessions.end())
        parent->second.children.erase(std::remove(parent->second.children.begin(),
            parent->second.children.end(), sid), parent->second.children.end());
    sessions.erase(sid);
    recordHistory(context.empty() ? "RemoveSession" : context, sid);
    fireChanged(sid, std::string(), std::string(), removed.tag, removed.sessionId);
}

void SessionService::removeSession(std::string sid, std::string context)
{ removeSessionTree(sid, context); }

void SessionService::removeSessionsWithMetadataKey(std::string key)
{
    std::vector<std::string> matches;
    for (std::map<std::string, Node>::const_iterator node = sessions.begin();
         node != sessions.end(); ++node)
        if (node->first != rootStructuralId &&
            node->second.metadata.find(key) != node->second.metadata.end())
            matches.push_back(node->first);
    for (std::vector<std::string>::const_iterator match = matches.begin();
         match != matches.end(); ++match)
        removeSessionTree(*match, "RemoveSessionsWithMetadataKey");
}

} // namespace RBX
