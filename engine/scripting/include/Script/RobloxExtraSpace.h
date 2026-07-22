#pragma once

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"
#include "rbx/Intrusive/Set.h"
#include "rbx/Debug.h"
#include "Script/LuaVM.h"
#include "Script/ScriptContext.h"
#include "Script/ThreadRef.h"
#include "security/FuzzyTokens.h"
#include "security/SecurityContext.h"

struct lua_State;

#if defined(RBX_LUAU_VM)
#include "lua.h"
#endif

namespace RBX
{
class BaseScript;
class ScriptContext;
}

#pragma pack(push)
#pragma pack(8)

// Per-thread engine state used by ScriptContext. The legacy VM embeds this
// object before lua_State; the Luau backend installs it through thread data.
class RobloxExtraSpace : public RBX::Intrusive::Set<RobloxExtraSpace>::Hook
{
    typedef RBX::Intrusive::Set<RobloxExtraSpace> AllThreads;

    struct Shared
    {
        int threadCount;
        RBX::ScriptContext* context;
        AllThreads allThreads;
        Shared()
            : threadCount(0)
            , context(NULL)
        {
        }
    };

    const boost::shared_ptr<Shared> shared;
    typedef RBX::Lua::WeakThreadRef::Node Node;
    boost::intrusive_ptr<Node> node;

public:
    RBX::Security::Identities identity : 5;
    bool yieldCaptured : 1;
    bool taskCancelled : 1;
    boost::weak_ptr<RBX::BaseScript> script;
    boost::scoped_ptr<RBX::Lua::Continuations> continuations;

    RBX::ScriptContext* context() const { return shared->context; }
    size_t getThreadCount() const { return static_cast<size_t>(shared->threadCount); }
    void setContext(RBX::ScriptContext* context) { shared->context = context; }
    Node* getNode() const { return node.get(); }

    void createNewNode()
    {
        node = new Node();
    }

    void eraseRefsFromAllNodes()
    {
        for (AllThreads::Iterator iter = shared->allThreads.begin();
             iter != shared->allThreads.end(); ++iter)
            iter->node->eraseAllRefs();
    }

    template<class Func>
    void forEachThread(Func func)
    {
        for (AllThreads::Iterator iter = shared->allThreads.begin();
             iter != shared->allThreads.end(); ++iter)
            iter->node->forEachRefs(func);
    }

    static RobloxExtraSpace* get(lua_State* state)
    {
#if defined(RBX_LUAU_VM)
        return state ? static_cast<RobloxExtraSpace*>(lua_getthreaddata(state)) : NULL;
#else
        return state ? reinterpret_cast<RobloxExtraSpace*>(
                           reinterpret_cast<char*>(state) - sizeof(RobloxExtraSpace))
                     : NULL;
#endif
    }

    static void constructRoot(lua_State* state)
    {
#if defined(RBX_LUAU_VM)
        RBXASSERT(state && !lua_getthreaddata(state));
        lua_setthreaddata(state, new RobloxExtraSpace());
#else
        new (get(state)) RobloxExtraSpace();
#endif
    }

    static void destroyRoot(lua_State* state)
    {
#if defined(RBX_LUAU_VM)
        delete get(state);
        lua_setthreaddata(state, NULL);
#else
        get(state)->~RobloxExtraSpace();
#endif
    }

    static void constructChild(lua_State* state, RobloxExtraSpace* parent)
    {
#if defined(RBX_LUAU_VM)
        RBXASSERT(state && parent && !lua_getthreaddata(state));
        lua_setthreaddata(state, new RobloxExtraSpace(parent));
#else
        new (get(state)) RobloxExtraSpace(parent);
#endif
    }

    static void destroyChild(lua_State* state)
    {
#if defined(RBX_LUAU_VM)
        delete get(state);
        lua_setthreaddata(state, NULL);
#else
        get(state)->~RobloxExtraSpace();
#endif
    }

#if defined(RBX_LUAU_VM)
    static void destroyDetached(RobloxExtraSpace* space)
    {
        delete space;
    }
#endif

private:
    RobloxExtraSpace()
        : shared(new Shared())
        , identity(RBX::Security::Anonymous)
        , yieldCaptured(false)
        , taskCancelled(false)
        , node(NULL)
    {
        ++shared->threadCount;
        shared->allThreads.insert(*this);
    }

    explicit RobloxExtraSpace(RobloxExtraSpace* parent)
        : shared(parent->shared)
        , identity(parent->identity)
        , yieldCaptured(false)
        , taskCancelled(false)
        , script(parent->script)
        , node(parent->node)
    {
        ++shared->threadCount;
        RBXASSERT(node);

        if (!shared->context->checkSecurityAnchorValid())
            RBX::Tokens::apiToken.addFlagSafe(RBX::kScriptContextCopy);
        else
            shared->allThreads.insert(*this);
    }

    ~RobloxExtraSpace()
    {
        shared->allThreads.remove_element(*this);
        --shared->threadCount;
        RBXASSERT(shared->threadCount >= 0);
    }
};

#pragma pack(pop)
