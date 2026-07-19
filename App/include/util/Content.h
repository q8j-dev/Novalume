#pragma once

#include "boost/shared_ptr.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace RBX {

class Instance;
namespace Reflection { class DescribedBase; }

class ContentDataSource
{
public:
    virtual ~ContentDataSource() = default;
    virtual bool readContent(std::vector<std::uint8_t>& bytes) const = 0;
};

enum ContentSourceType
{
    CONTENT_SOURCE_NONE = 0,
    CONTENT_SOURCE_URI = 1,
    CONTENT_SOURCE_OBJECT = 2,
    CONTENT_SOURCE_OPAQUE = 3,
};

class OpaqueContent
{
public:
    explicit OpaqueContent(std::vector<std::uint8_t> value)
        : bytes(std::move(value))
    {
    }

    const std::vector<std::uint8_t>& getBytes() const { return bytes; }

private:
    std::vector<std::uint8_t> bytes;
};

class Content
{
public:
    Content() = default;

    static Content fromUri(std::string uri);
    static Content fromAssetId(std::int64_t assetId);
    static Content fromObject(boost::shared_ptr<Reflection::DescribedBase> object);
    static Content fromOpaque(boost::shared_ptr<const OpaqueContent> opaque);

    ContentSourceType getSourceType() const { return sourceType; }
    const std::string& getUri() const { return uri; }
    const boost::shared_ptr<Reflection::DescribedBase>& getObject() const { return object; }
    const boost::shared_ptr<const OpaqueContent>& getOpaque() const { return opaque; }
    bool empty() const { return sourceType == CONTENT_SOURCE_NONE; }

    friend bool operator==(const Content& left, const Content& right);
    friend bool operator!=(const Content& left, const Content& right) { return !(left == right); }
    friend bool operator<(const Content& left, const Content& right);

private:
    ContentSourceType sourceType = CONTENT_SOURCE_NONE;
    std::string uri;
    boost::shared_ptr<Reflection::DescribedBase> object;
    boost::shared_ptr<const OpaqueContent> opaque;
};

} // namespace RBX
