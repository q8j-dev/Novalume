
#include "V8DataModel/HttpService.h"
#include "V8DataModel/DataModel.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/Stats.h"
#include "V8Xml/WebParser.h"
#include "Network/Players.h"
#include "Util/Http.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "Util/standardout.h"
#include "Script/ScriptContext.h"

#include <boost/algorithm/string/predicate.hpp>
#include <algorithm>
#include <cctype>

DYNAMIC_FASTINTVARIABLE(UserHttpRequestsPerMinuteLimit, 500)

namespace {
	static inline void sendHttpServiceStats(int placeId)
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpService", RBX::StringConverter<int>::convertToString(placeId).c_str());
	}
}

namespace RBX {
	const char* const sHttpRequest = "HttpRequest";
	const char* const sHttpService = "HttpService";

    REFLECTION_BEGIN();
	static Reflection::BoundFuncDesc<HttpRequest, void(Lua::WeakFunctionRef)>
		startHttpRequest(&HttpRequest::start, "Start", "callback", Security::RobloxScript);
	static Reflection::BoundFuncDesc<HttpRequest, void()>
		cancelHttpRequest(&HttpRequest::cancel, "Cancel", Security::RobloxScript);
	static Reflection::BoundYieldFuncDesc<HttpService, std::string(std::string, bool)>  userHttpGetAsyncFunction(&HttpService::userHttpGetAsync, "GetAsync", "url", "nocache", false, Security::None);
	static Reflection::BoundYieldFuncDesc<HttpService, std::string(std::string, std::string, HttpService::HttpContentType, bool)>  userHttpPostAsyncFunction(&HttpService::userHttpPostAsync, "PostAsync", "url", "data", "content_type", HttpService::APPLICATION_JSON, "compress", false, Security::None);
	static Reflection::BoundFuncDesc<HttpService, Reflection::Variant(std::string)> decodeJSON(&HttpService::decodeJSON, "JSONDecode", "input", Security::None);
	static Reflection::BoundFuncDesc<HttpService, std::string(Reflection::Variant)> encodeJSON(&HttpService::encodeJSON, "JSONEncode", "input", Security::None);
	static Reflection::BoundFuncDesc<HttpService, std::string(std::string)> urlEncode(&HttpService::urlEncode, "UrlEncode", "input", Security::None);
	static Reflection::BoundFuncDesc<HttpService, std::string(bool)> generateGuid(&HttpService::generateGuid, "GenerateGUID", "wrapInCurlyBraces", true, Security::None);
	static Reflection::BoundFuncDesc<HttpService, std::string()> getUserAgent(&HttpService::getUserAgent, "GetUserAgent", Security::RobloxScript);
	static Reflection::BoundFuncDesc<HttpService,
		shared_ptr<Instance>(shared_ptr<const Reflection::ValueTable>)>
		requestInternal(&HttpService::requestInternal, "RequestInternal", "options",
			Security::RobloxScript);

	Reflection::BoundProp<bool> HttpService::prop_httpEnabled("HttpEnabled", category_Data, &HttpService::httpEnabled, Reflection::PropertyDescriptor::STANDARD, Security::LocalUser);
    REFLECTION_END();

	namespace {
		std::string requiredStringOption(
			const shared_ptr<const Reflection::ValueTable>& options,
			const char* name)
		{
			if (!options)
				throw std::runtime_error("RequestInternal options must be a table");
			Reflection::ValueTable::const_iterator value = options->find(name);
			if (value == options->end() || !value->second.isType<std::string>() ||
				value->second.cast<std::string>().empty())
				throw std::runtime_error(std::string("RequestInternal requires a non-empty ") + name);
			return value->second.cast<std::string>();
		}

		std::string optionalStringOption(
			const shared_ptr<const Reflection::ValueTable>& options,
			const char* name, const std::string& fallback = std::string())
		{
			if (!options)
				return fallback;
			Reflection::ValueTable::const_iterator value = options->find(name);
			return value != options->end() && value->second.isType<std::string>()
				? value->second.cast<std::string>() : fallback;
		}
	}

	HttpRequest::HttpRequest(const shared_ptr<DataModel>& owner,
		const shared_ptr<const Reflection::ValueTable>& options)
		: dataModel(owner)
		, url(requiredStringOption(options, "Url"))
		, method(optionalStringOption(options, "Method", "GET"))
		, body(optionalStringOption(options, "Body"))
		, contentType(Http::kContentTypeApplicationJson)
		, cancelled(false)
		, started(false)
	{
		setName(sHttpRequest);
		setRobloxLocked(true);
		std::transform(method.begin(), method.end(), method.begin(), ::toupper);
		if (method != "GET" && method != "POST")
			throw std::runtime_error("RequestInternal method must be GET or POST");

		if (options)
		{
			Reflection::ValueTable::const_iterator headerValue = options->find("Headers");
			if (headerValue != options->end() &&
				headerValue->second.isType<shared_ptr<const Reflection::ValueTable> >())
			{
				shared_ptr<const Reflection::ValueTable> source =
					headerValue->second.cast<shared_ptr<const Reflection::ValueTable> >();
				if (source)
				{
					for (Reflection::ValueTable::const_iterator it = source->begin();
						it != source->end(); ++it)
					{
						if (!it->second.isType<std::string>())
							throw std::runtime_error("RequestInternal header values must be strings");
						headers[it->first] = it->second.cast<std::string>();
						if (boost::iequals(it->first, "Content-Type"))
							contentType = it->second.cast<std::string>();
					}
				}
			}
		}
	}

	void HttpRequest::start(Lua::WeakFunctionRef callback)
	{
		if (!callback.lock())
			throw std::runtime_error("HttpRequest:Start requires a callback");
		if (started.exchange(true))
			throw std::runtime_error("HttpRequest has already been started");
		if (cancelled.load())
			return;
		this->callback = callback;

		shared_ptr<HttpRequest> self = shared_from(this);
		Http request(url);
		request.additionalHeaders = headers;
		if (method == "POST")
			request.post(body.empty() ? std::string(" ") : body, contentType, false,
				boost::bind(&HttpRequest::onResponse, self, _1, _2), true);
		else
			request.get(boost::bind(&HttpRequest::onResponse, self, _1, _2), true);
	}

	void HttpRequest::cancel()
	{
		cancelled.store(true);
	}

	void HttpRequest::onResponse(std::string* response, std::exception* error)
	{
		if (cancelled.load())
			return;
		shared_ptr<DataModel> owner = dataModel.lock();
		if (!owner)
			return;

		bool success = response != NULL && error == NULL;
		int statusCode = success ? 200 : 0;
		std::string statusMessage = success ? "OK" : (error ? error->what() : "Request failed");
		if (error)
		{
			if (http_status_error* status = dynamic_cast<http_status_error*>(error))
				statusCode = status->statusCode;
		}
		const std::string responseBody = response ? *response : std::string();
		shared_ptr<HttpRequest> self = shared_from(this);
		owner->submitTask(boost::bind(&HttpRequest::deliver, self,
			success, statusCode, statusMessage, responseBody), DataModelJob::Write);
	}

	void HttpRequest::deliver(bool success,
		int statusCode, std::string statusMessage, std::string responseBody)
	{
		if (cancelled.load() || !callback.lock())
			return;
		shared_ptr<DataModel> owner = dataModel.lock();
		if (!owner)
			return;

		shared_ptr<Reflection::ValueTable> response(new Reflection::ValueTable());
		(*response)["Success"] = Reflection::Variant(success);
		(*response)["StatusCode"] = Reflection::Variant(statusCode);
		(*response)["StatusMessage"] = Reflection::Variant(statusMessage);
		(*response)["Body"] = Reflection::Variant(responseBody);
		(*response)["Headers"] = Reflection::Variant(
			shared_ptr<const Reflection::ValueTable>(new Reflection::ValueTable()));
		Reflection::Tuple arguments;
		arguments.values.push_back(Reflection::Variant(success));
		arguments.values.push_back(Reflection::Variant(
			shared_ptr<const Reflection::ValueTable>(response)));
		try
		{
			ServiceProvider::create<ScriptContext>(owner.get())->callInNewThread(callback, arguments);
		}
		catch (const base_exception& exception)
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR,
				"HttpRequest callback failed: %s", exception.what());
		}
	}

	namespace Reflection {
		template<>
		EnumDesc<HttpService::HttpContentType>::EnumDesc()
			:EnumDescriptor("HttpContentType")
		{
			addPair(HttpService::APPLICATION_JSON,			"ApplicationJson");		
			addPair(HttpService::APPLICATION_XML,			"ApplicationXml");		
			addPair(HttpService::APPLICATION_URLENCODED,	"ApplicationUrlEncoded");	
			addPair(HttpService::TEXT_PLAIN,				"TextPlain");
			addPair(HttpService::TEXT_XML,					"TextXml");
		}

		template<>
		HttpService::HttpContentType& Variant::convert<HttpService::HttpContentType>(void)
		{
			return genericConvert<HttpService::HttpContentType>();
		}
	}//namespace Reflection

	template<>
	bool StringConverter<HttpService::HttpContentType>::convertToValue(const std::string& text, HttpService::HttpContentType& value)
	{
		return Reflection::EnumDesc<HttpService::HttpContentType>::singleton().convertToValue(text.c_str(),value);
	}

	HttpService::HttpService() :
		httpEnabled(false),
		throttle(&DFInt::UserHttpRequestsPerMinuteLimit)
	{
		setName(sHttpService);
	}

	bool HttpService::checkEverything(std::string& url, boost::function<void(std::string)> errorFunction)
	{
		if (url.size() == 0)
		{
			errorFunction("Empty URL");
			return false;
		}

		if (!httpEnabled)
		{
			errorFunction("Http requests are not enabled");
			return false;
		}

		if (!Network::Players::backendProcessing(this))
		{
			errorFunction("Http requests can only be executed by game server");
			return false;
		}

		if (Instance::fastDynamicCast<DataModel>(getParent()) == NULL)
		{
			errorFunction("Unrecognized HttpService");
			return false;
		}

		if (!throttle.checkLimit())
		{
			errorFunction("Number of requests exceeded limit");
			return false;
		}

		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			DataModel* dm = DataModel::get(this);
			boost::call_once(flag, boost::bind(&sendHttpServiceStats,dm->getPlaceID()));
		}

		return true;
	}

	void HttpService::addIdHeader(Http& request)
	{
		DataModel* dm = DataModel::get(this);
		request.additionalHeaders["Roblox-Id"] = RBX::StringConverter<int>::convertToString(dm->getPlaceID());
	}

	void HttpService::userHttpPostAsync(std::string url, std::string data, HttpContentType contentType, bool compress, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!checkEverything(url, errorFunction))
			return;
		 
		Http http(url);
		std::string contentTypeName;
		switch(contentType)
		{
		case APPLICATION_JSON: contentTypeName = Http::kContentTypeApplicationJson; break;
		case APPLICATION_XML: contentTypeName = Http::kContentTypeApplicationXml; break;
		case APPLICATION_URLENCODED: contentTypeName = Http::kContentTypeUrlEncoded; break;
		case TEXT_PLAIN: contentTypeName = Http::kContentTypeTextPlain; break;
		case TEXT_XML: contentTypeName = Http::kContentTypeTextXml; break;
		default:
			errorFunction("Unsupported content type");
			return; 
		}

		if(data.length() == 0)
			data = " ";
		addIdHeader(http);
		http.post(data, contentTypeName, compress, boost::bind(&DataModel::HttpHelper, _1, _2, resumeFunction, errorFunction), true);
	}

	void HttpService::userHttpGetAsync(std::string url, bool nocache, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!checkEverything(url, errorFunction))
			return;

		Http http(url);
		if(nocache)
		{
			http.additionalHeaders["Cache-Control"] = "no-cache";
			http.doNotUseCachedResponse = true;
		}

		addIdHeader(http);
		http.get(boost::bind(&DataModel::HttpHelper, _1, _2, resumeFunction, errorFunction), true);
	}

	Reflection::Variant HttpService::decodeJSON(std::string input)
	{
		Reflection::Variant value;
		if(WebParser::parseJSONObject(input, value))
		{
			return value;
		}
		else
		{
			throw std::runtime_error("Can't parse JSON");
		}
	}

	std::string HttpService::encodeJSON(Reflection::Variant obj)
	{
		std::string result;

		if(!WebParser::writeJSON(obj, result, WebParser::FailOnNonJSON))
			throw std::runtime_error("Can't convert to JSON");

		return result;
	}

	std::string HttpService::urlEncode(std::string data)
	{
		return Http::urlEncode(data);
	}

	std::string HttpService::generateGuid(bool wrapInCurlyBraces)
	{
		std::string result;
		Guid::generateStandardGUID(result);

		if (!wrapInCurlyBraces)
		{
			RBXASSERT(result.size() > 2);
			result = result.substr(1, result.size() - 2);
		}

		return result;
	}

	std::string HttpService::getUserAgent()
	{
		return Http::rbxUserAgent;
	}

	shared_ptr<Instance> HttpService::requestInternal(
		shared_ptr<const Reflection::ValueTable> options)
	{
		DataModel* owner = DataModel::get(this);
		if (!owner)
			throw std::runtime_error("RequestInternal requires a DataModel");
		return shared_ptr<HttpRequest>(new HttpRequest(shared_from(owner), options));
	}
}
