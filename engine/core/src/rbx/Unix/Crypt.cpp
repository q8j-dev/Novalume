#include "rbx/Crypt.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view publicKey =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCtfLLFT36v5r9bNP7STBteDU5a\n"
    "0hk1+5rK8KEhXTPQysst6oZGBV8zW6pMB8QNDkX8gxLLI40gM9Vj6rIwnT6Jp+3u\n"
    "AvDu4aamxxVE4mfLHsRz+hW0OlULYdGwWlq6KXQd8zsSbJkBsFXbiy8xIbZPKPA5\n"
    "2YkChMUF0+X0sUxtowIDAQAB\n"
    "-----END PUBLIC KEY-----\n";

std::vector<unsigned char> decodeBase64(std::string_view encoded)
{
    constexpr std::size_t maximumEncodedSignatureBytes = 64 * 1024;
    if (encoded.empty() || encoded.size() > maximumEncodedSignatureBytes)
        throw std::runtime_error("signature verification failed");

    std::string compact;
    compact.reserve(encoded.size());
    std::copy_if(encoded.begin(), encoded.end(), std::back_inserter(compact),
        [](char character) {
            return std::isspace(static_cast<unsigned char>(character)) == 0;
        });
    if (compact.empty() || compact.size() % 4 != 0)
        throw std::runtime_error("signature verification failed");

    std::vector<unsigned char> decoded((compact.size() / 4) * 3);
    const int decodedSize = EVP_DecodeBlock(decoded.data(),
        reinterpret_cast<const unsigned char*>(compact.data()),
        static_cast<int>(compact.size()));
    if (decodedSize < 0)
        throw std::runtime_error("signature verification failed");

    std::size_t padding = 0;
    if (!compact.empty() && compact.back() == '=')
        ++padding;
    if (compact.size() > 1 && compact[compact.size() - 2] == '=')
        ++padding;
    if (static_cast<std::size_t>(decodedSize) < padding)
        throw std::runtime_error("signature verification failed");
    const std::size_t unpaddedSize = static_cast<std::size_t>(decodedSize) - padding;
    decoded.resize(unpaddedSize);
    return decoded;
}

}

namespace RBX {

Crypt::Crypt() = default;
Crypt::~Crypt() = default;

void Crypt::verifySignatureBase64(
    std::string message, std::string signatureBase64)
{
    const std::vector<unsigned char> signature = decodeBase64(signatureBase64);

    std::unique_ptr<BIO, decltype(&BIO_free)> keyInput(
        BIO_new_mem_buf(publicKey.data(), static_cast<int>(publicKey.size())),
        &BIO_free);
    if (!keyInput)
        throw std::runtime_error("signature verification failed");

    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(
        PEM_read_bio_PUBKEY(keyInput.get(), nullptr, nullptr, nullptr),
        &EVP_PKEY_free);
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context(
        EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!key || !context ||
        EVP_DigestVerifyInit(context.get(), nullptr, EVP_sha1(), nullptr, key.get()) != 1 ||
        EVP_DigestVerifyUpdate(context.get(), message.data(), message.size()) != 1 ||
        EVP_DigestVerifyFinal(context.get(), signature.data(), signature.size()) != 1)
    {
        throw std::runtime_error("signature verification failed");
    }
}

}
