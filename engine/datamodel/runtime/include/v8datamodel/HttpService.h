#pragma once 
#include "Reflection/Reflection.h"
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "rbx/RunningAverage.h"
#include "Script/ThreadRef.h"
#include "Util/HttpAux.h"

#include <atomic>


namespace RBX {
	class Http;
	class DataModel;

	extern const char* const sHttpRequest;
	class HttpRequest
		: public DescribedNonCreatable<HttpRequest, Instance, sHttpRequest>
	{
		weak_ptr<DataModel> dataModel;
		std::string url;
		std::string method;
		std::string body;
		std::string contentType;
		HttpAux::AdditionalHeaders headers;
		Lua::WeakFunctionRef callback;
		std::atomic<bool> cancelled;
		std::atomic<bool> started;

		void onResponse(std::string* response, std::exception* error);
		void deliver(bool success, int statusCode,
			std::string statusMessage, std::string responseBody);

	public:
		HttpRequest(const shared_ptr<DataModel>& dataModel,
			const shared_ptr<const Reflection::ValueTable>& options);
		void start(Lua::WeakFunctionRef callback);
		void cancel();
	};

	extern const char* const sHttpService;
	class HttpService
		:public DescribedCreatable<HttpService, Instance, sHttpService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
		,public Service
	{
		ThrottlingHelper throttle;

		bool httpEnabled;
		static Reflection::BoundProp<bool> prop_httpEnabled;
		bool checkEverything(std::string& url, boost::function<void(std::string)> errorFunction);

		void addIdHeader(Http& request);

	public:	
		HttpService();

		enum HttpContentType
		{	
			APPLICATION_JSON = 0,
			APPLICATION_XML = 1,
			APPLICATION_URLENCODED = 2,
			TEXT_PLAIN = 3, 
			TEXT_XML = 4, 
		};


		void userHttpGetAsync(std::string url, bool noCache, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void userHttpPostAsync(std::string url, std::string data, HttpContentType content, bool compress, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);

		Reflection::Variant decodeJSON(std::string input);
		std::string encodeJSON(Reflection::Variant obj);
		std::string urlEncode(std::string data);
		std::string generateGuid(bool wrapInCurlyBraces);
		std::string getUserAgent();
		shared_ptr<Instance> requestInternal(
			shared_ptr<const Reflection::ValueTable> options);
	};
}
