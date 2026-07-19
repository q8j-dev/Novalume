#pragma once


#include "util/runstateowner.h"
#include "g3d/Array.h"
#include "boost/any.hpp"
#include "boost/shared_ptr.hpp"
#include "lua/LuaBridge.h"
#include "script/threadref.h"
#include <boost/thread/mutex.hpp>

#include <vector>

struct lua_State;
class ThreadInfo;



namespace RBX { 
	class Instance;
	class ScriptContext;

	namespace Lua {

	class YieldingThreads
	{
		ScriptContext* context;

		struct WaitingThread
		{
			boost::intrusive_ptr<WeakThreadRef> thread;
			RBX::Time waitTime;
			RBX::Time resumeTime;
			int resumeArguments;
			bool appendElapsedTime;
			WaitingThread(lua_State *L, RBX::Time::Interval requestedDelay)
				:thread(new WeakThreadRef(L)),
				waitTime(RBX::Time::now<RBX::Time::Precise>()),
				resumeArguments(0), appendElapsedTime(true)
			{
				resumeTime = waitTime + requestedDelay;
			}
			WaitingThread(lua_State *L, RBX::Time::Interval requestedDelay,
				int resumeArguments, bool appendElapsedTime)
				: thread(new WeakThreadRef(L)),
				waitTime(RBX::Time::now<RBX::Time::Precise>()),
				resumeArguments(resumeArguments),
				appendElapsedTime(appendElapsedTime)
			{
				resumeTime = waitTime + requestedDelay;
			}

			bool operator <(const WaitingThread& other) const
			{
				return this->resumeTime > other.resumeTime;
			}
		};
		typedef std::priority_queue< WaitingThread > WaitThreadRefs;

		// Lua refs to threads that are waiting on the event
		WaitThreadRefs waitingThreads;

	public:
		YieldingThreads(ScriptContext* context);

		// Hooking up consumers:
		void queueWaiter(lua_State *L);
		void queueWaiter(lua_State *L, LUA_NUMBER delay);
		void queueWaiter(lua_State *L, LUA_NUMBER delay, int resumeArguments,
			bool appendElapsedTime);
		void cancelWaiter(lua_State* L);

		void resume(double wallTime, Time expirationTime, bool& throttling);
		void cancelAllWaiters();

		std::size_t waiterCount() const;
	};

	// specialization
	template<>
	int Bridge<rbx::signals::connection>::on_tostring(const rbx::signals::connection& object, lua_State *L);

	template<>
	int Bridge<boost::intrusive_ptr<class WeakThreadRef::Node> >::on_tostring(const boost::intrusive_ptr<class WeakThreadRef::Node>& object, lua_State *L);

	template<>
	int Bridge< shared_ptr<GenericFunction> >::on_tostring(const shared_ptr<GenericFunction>& object, lua_State *L);

	template<>
	int Bridge< shared_ptr<GenericAsyncFunction> >::on_tostring(const shared_ptr<GenericAsyncFunction>& object, lua_State *L);

} }
