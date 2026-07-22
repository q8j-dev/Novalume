#pragma once

#include "Script/script.h"

#include <boost/optional.hpp>
#include <map>

namespace RBX
{
	extern const char* const sCoreScript;
	class CoreScript
		: public DescribedNonCreatable<CoreScript, BaseScript, sCoreScript, RBX::Reflection::ClassDescriptor::INTERNAL_LOCAL>
	{
	private:
		typedef DescribedNonCreatable<CoreScript, BaseScript, sCoreScript, RBX::Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
        Code code;

	public:
		CoreScript();

        static boost::optional<ProtectedString> fetchSource(const std::string& name);
        static bool hasPackagedSources();
        static void installPackagedSources(
            const std::map<std::string, ProtectedString>& sources);

		virtual Code requestCode(ScriptInformationProvider* scriptInfoProvider=NULL);

		virtual void extraErrorReporting(lua_State *thread);

	protected:
		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	};
}
