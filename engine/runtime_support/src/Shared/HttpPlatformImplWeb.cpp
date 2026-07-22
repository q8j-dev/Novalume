#include "stdafx.h"

#include "util/HttpPlatformImpl.h"

#include <emscripten/fetch.h>
#include <emscripten.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

DYNAMIC_FASTFLAGVARIABLE(HttpZeroLatencyCaching, false)

EM_JS(int, rbx_http_has_xml_http_request, (), {
    return typeof XMLHttpRequest !== 'undefined';
});

namespace RBX::HttpPlatformImpl {
namespace {

std::string lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::string responseHeader(emscripten_fetch_t* fetch, std::string_view name)
{
    const std::size_t length = emscripten_fetch_get_response_headers_length(fetch);
    if (!length)
        return {};
    std::vector<char> storage(length + 1);
    emscripten_fetch_get_response_headers(fetch, storage.data(), storage.size());
    std::istringstream input(std::string(storage.data(), length));
    const std::string requested = lowercase(std::string(name));
    std::string line;
    while (std::getline(input, line)) {
        const std::size_t separator = line.find(':');
        if (separator == std::string::npos || lowercase(line.substr(0, separator)) != requested)
            continue;
        std::size_t start = separator + 1;
        while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start])))
            ++start;
        while (line.size() > start && std::isspace(static_cast<unsigned char>(line.back())))
            line.pop_back();
        return line.substr(start);
    }
    return {};
}

struct FetchResult final {
    unsigned short status = 0;
    std::string statusText;
    std::string body;
    std::string csrfToken;
};

FetchResult fetch(HttpOptions& options, const std::string& postBody)
{
    if (!rbx_http_has_xml_http_request())
        throw RBX::runtime_error("Browser HTTP transport is unavailable in this JavaScript host");

    std::vector<std::string> headerStorage;
    if (options.hdrContentType && !options.hdrContentType->empty()) {
        headerStorage.emplace_back("Content-Type");
        headerStorage.push_back(*options.hdrContentType);
    }
    if (options.compressedPostData) {
        headerStorage.emplace_back("Content-Encoding");
        headerStorage.emplace_back("gzip");
    }
    if (options.addlHeaders) {
        for (const auto& [name, value] : *options.addlHeaders) {
            headerStorage.push_back(name);
            headerStorage.push_back(value);
        }
    }
    if (options.postData && !Http::getLastCsrfToken().empty()) {
        headerStorage.emplace_back("X-CSRF-TOKEN");
        headerStorage.push_back(Http::getLastCsrfToken());
    }
    std::vector<const char*> headers;
    headers.reserve(headerStorage.size() + 1);
    for (const std::string& value : headerStorage)
        headers.push_back(value.c_str());
    headers.push_back(nullptr);

    emscripten_fetch_attr_t attributes;
    emscripten_fetch_attr_init(&attributes);
    std::strcpy(attributes.requestMethod, options.postData ? "POST" : "GET");
    attributes.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
    attributes.timeoutMSecs = static_cast<std::uint32_t>(std::max(0L,
        options.performTimeoutMillis));
    attributes.withCredentials = true;
    attributes.requestHeaders = headers.data();
    if (options.postData) {
        attributes.requestData = postBody.data();
        attributes.requestDataSize = postBody.size();
    }

    emscripten_fetch_t* request = emscripten_fetch(&attributes, options.url.c_str());
    if (!request)
        throw RBX::runtime_error("Browser fetch could not create the request");
    FetchResult result;
    result.status = request->status;
    result.statusText = request->statusText;
    if (request->data && request->numBytes)
        result.body.assign(request->data, static_cast<std::size_t>(request->numBytes));
    result.csrfToken = responseHeader(request, "x-csrf-token");
    emscripten_fetch_close(request);
    return result;
}

}

void init(Http::CookieSharingPolicy)
{
}

void shutdown()
{
}

void setCookiesForDomain(const std::string&, const std::string&)
{
}

void getCookiesForDomain(const std::string&, std::string& cookies)
{
    cookies.clear();
}

boost::filesystem::path getRobloxCookieJarPath()
{
    return {};
}

void setProxy(const std::string&, long)
{
}

void perform(HttpOptions& options, std::string& response)
{
    std::string postBody;
    if (options.postData) {
        std::ostringstream output;
        output << options.postData->rdbuf();
        postBody = output.str();
    }
    FetchResult result = fetch(options, postBody);
    if (options.postData && result.status == 403 && !result.csrfToken.empty() &&
        result.csrfToken != Http::getLastCsrfToken()) {
        Http::setLastCsrfToken(result.csrfToken);
        result = fetch(options, postBody);
    }
    response = std::move(result.body);
    if (result.status < 200 || result.status > 299 || result.status == 202) {
        if (result.status == 0)
            throw RBX::runtime_error("Browser fetch failed before receiving an HTTP response");
        throw RBX::http_status_error(result.status, result.statusText);
    }
}

}
