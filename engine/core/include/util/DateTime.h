#pragma once

#include <cstdint>

namespace RBX {

class DateTime
{
public:
    static constexpr std::int64_t MinimumUnixTimestampMillis = -62135596800000LL;
    static constexpr std::int64_t MaximumUnixTimestampMillis = 253402300799999LL;

    DateTime()
        : unixTimestampMillis(0)
    {
    }

    explicit DateTime(std::int64_t value)
        : unixTimestampMillis(value)
    {
    }

    std::int64_t getUnixTimestampMillis() const { return unixTimestampMillis; }

    bool operator==(const DateTime& other) const
    {
        return unixTimestampMillis == other.unixTimestampMillis;
    }
    bool operator!=(const DateTime& other) const { return !(*this == other); }

private:
    std::int64_t unixTimestampMillis;
};

} // namespace RBX
