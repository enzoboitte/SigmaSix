#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace yaml {

// Chiffrement ChaCha20 des fichiers .yml (format binaire "SSYE").
class Cipher {
public:
    using Key = std::array<std::uint8_t, 32>;

    static void setKey(const std::string& passphrase);
    static const Key& key();
    static Key deriveKey(const std::string& passphrase);

    static bool isEncrypted(const std::string& data);
    static std::string encrypt(const std::string& plain);
    static std::string encrypt(const std::string& plain, const Key& key);
    static bool decrypt(const std::string& data, std::string& plain);
    static bool decrypt(const std::string& data, std::string& plain, const Key& key);
};

}
