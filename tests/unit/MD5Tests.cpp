#include "util/md5.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string hashChunks(const std::string& value, std::size_t chunkSize)
{
    RBX_MD5_CTX context;
    RBX_MD5_Init(&context);
    for (std::size_t offset = 0; offset < value.size(); offset += chunkSize)
    {
        const std::size_t size = std::min(chunkSize, value.size() - offset);
        RBX_MD5_Update(&context, value.data() + offset, size);
    }

    std::array<unsigned char, 16> digest{};
    RBX_MD5_Final(digest.data(), &context);
    const char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(32);
    for (unsigned char byte : digest)
    {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 15]);
    }
    return result;
}

void requireHash(const std::string& value, const char* expected)
{
    require(hashChunks(value, value.empty() ? 1 : value.size()) == expected,
        "single-update MD5 digest mismatch");
    require(hashChunks(value, 1) == expected,
        "bytewise MD5 digest mismatch");
    require(hashChunks(value, 17) == expected,
        "chunked MD5 digest mismatch");
}

}

int main()
{
    try
    {
        requireHash("", "d41d8cd98f00b204e9800998ecf8427e");
        requireHash("abc", "900150983cd24fb0d6963f7d28e17f72");
        requireHash(std::string(55, 'a'), "ef1772b6dff9a122358552954ad0df65");
        requireHash(std::string(56, 'a'), "3b0c8ac703f828b04c6c197006d17218");
        requireHash(std::string(63, 'a'), "b06521f39153d618550606be297466d5");
        requireHash(std::string(64, 'a'), "014842d480b571495a4a0363793f7367");
        requireHash(std::string(65, 'a'), "c743a45e0d2e6a95cb859adae0248435");
        requireHash(std::string(100000, 'a'), "1af6d6f2f682f76f80e606aeaaee1680");
        std::cout << "MD5 contract passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "MD5 contract failed: " << error.what() << '\n';
        return 1;
    }
}
