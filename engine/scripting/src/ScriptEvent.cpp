
#include "Script/ScriptEvent.h"
#include "Script/RobloxExtraSpace.h"
#include "Script/ScriptContext.h"
#include "lua/lua.hpp"
#include "util/standardout.h"
#include "Script/LuaInstanceBridge.h"
#include "rbx/rbxTime.h"
#include "Script/LuaSettings.h"
#include "FastLog.h"

DYNAMIC_FASTFLAGVARIABLE(FixYieldThrottling, false)

using namespace RBX;
using namespace RBX::Lua;

YieldingThreads::YieldingThreads(ScriptContext* context)
	:context(context)
{
}


void YieldingThreads::queueWaiter(lua_State *L)
{
	queueWaiter(L, 0.0);
}


void YieldingThreads::queueWaiter(lua_State *L, LUA_NUMBER delay)
{
	queueWaiter(L, delay, 0, true);
}

void YieldingThreads::queueWaiter(lua_State *L, LUA_NUMBER delay,
	int resumeArguments, bool appendElapsedTime)
{
	RBXASSERT(!RobloxExtraSpace::get(L)->yieldCaptured);
	RobloxExtraSpace::get(L)->yieldCaptured = true;

	waitingThreads.push(WaitingThread(L, RBX::Time::Interval(delay),
		resumeArguments, appendElapsedTime));
}

void YieldingThreads::cancelWaiter(lua_State* L)
{
	WaitThreadRefs retained;
	while (!waitingThreads.empty())
	{
		WaitingThread waiter = waitingThreads.top();
		waitingThreads.pop();
		ThreadRef thread = waiter.thread->lock();
		if (!thread || thread.get() != L)
			retained.push(waiter);
	}
	waitingThreads.swap(retained);
}

std::size_t YieldingThreads::waiterCount() const
{
	return waitingThreads.size();
}


void YieldingThreads::resume(double wallTime, Time expirationTime, bool& throttling)
{
	FASTLOG(FLog::ScriptContext, "Resuming waiting threads");

	int count = waitingThreads.size();

	while ((!DFFlag::FixYieldThrottling || count-- > 0) && !waitingThreads.empty()) 
	{
		RBX::Time now = RBX::Time::now<RBX::Time::Precise>();

		const WaitingThread& w = waitingThreads.top();

		if (now < w.resumeTime)
			break;	// we're done

		RBX::Time::Interval elapsedTime = now - w.waitTime;
		ThreadRef thread = w.thread->lock();
		const int resumeArguments = w.resumeArguments;
		const bool appendElapsedTime = w.appendElapsedTime;

		waitingThreads.pop();

		if (thread)
		{
			if (RobloxExtraSpace::get(thread)->taskCancelled)
				continue;
			// A coroutine owner can resume and finish a thread before its queued
			// next-cycle wake-up. That completes the scheduled work; the obsolete
			// wake-up must be retired instead of resuming a dead coroutine.
				if (lua_status(thread) == 0 && lua_gettop(thread) == 0)
				continue;
			if (appendElapsedTime)
			{
				lua_pushnumber(thread, elapsedTime.seconds());
				lua_pushnumber(thread, wallTime);
			}

			// resume the waiting thread
			const int argumentCount = resumeArguments +
				(appendElapsedTime ? 2 : 0);
			const ScriptContext::Result resumeResult = context->resume(thread, argumentCount);
			if (resumeResult != ScriptContext::Yield)
			{
				// success or error means thread is finished, clear the stack
				lua_resetstack(thread, 0);
			}

			context->scriptResumedFromEvent();
		}

		// We always do at least one thread, so as to make forward progress
		if (now > expirationTime)
		{
			throttling = true;
			break;
		}
	}
}

void YieldingThreads::cancelAllWaiters()
{
	while (!waitingThreads.empty())
		waitingThreads.pop();
}

namespace RBX
{
	namespace Lua
	{
		// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
		template<>
		int Bridge<rbx::signals::connection>::on_tostring(const rbx::signals::connection& object, lua_State *L)
		{
			lua_pushliteral(L, "Connection");
			return 1;
		}

		// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
		template<>
		int Bridge<boost::intrusive_ptr<class WeakThreadRef::Node> >::on_tostring(const boost::intrusive_ptr<class WeakThreadRef::Node>& object, lua_State *L)
		{
			lua_pushliteral(L, "WeakThreadRef");
			return 1;
		}

		// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
		template<>
		int Bridge< shared_ptr<GenericFunction> >::on_tostring(const shared_ptr<GenericFunction>& object, lua_State *L)
		{
			lua_pushliteral(L, "GenericFunction");
			return 1;
		}

		// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
		template<>
		int Bridge< shared_ptr<GenericAsyncFunction> >::on_tostring(const shared_ptr<GenericAsyncFunction>& object, lua_State *L)
		{
			lua_pushliteral(L, "GenericAsyncFunction");
			return 1;
		}
	}
}
