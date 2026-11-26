#include <SigmaYaml/Cipher.hpp>
#include <cstring>

#ifndef SIGMA_YAML_KEY
#define SIGMA_YAML_KEY "SigmaSix-default-asset-key"
#endif

namespace yaml {

namespace {

constexpr char kMagic[4] = {'S', 'S', 'Y', 'E'};
constexpr std::uint8_t kVersion = 1;
constexpr std::size_t kNonceSize = 12;
constexpr std::size_t kTagSize = 8;
constexpr std::size_t kHeaderSize = 8 + kNonceSize + kTagSize;

std::uint32_t rotl(std::uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }

void quarterRound(std::uint32_t& a, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d)
{
    a += b; d ^= a; d = rotl(d, 16);
    c += d; b ^= c; b = rotl(b, 12);
    a += b; d ^= a; d = rotl(d, 8);
    c += d; b ^= c; b = rotl(b, 7);
}

std::uint32_t load32(const std::uint8_t* p)
{
    return std::uint32_t(p[0]) | std::uint32_t(p[1]) << 8 | std::uint32_t(p[2]) << 16 | std::uint32_t(p[3]) << 24;
}

// ChaCha20 (RFC 8439), XOR en place.
void chacha20(std::uint8_t* data, std::size_t size, const Cipher::Key& key, const std::uint8_t* nonce)
{
    std::uint32_t state[16] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};
    for (int i = 0; i < 8; ++i) state[4 + i] = load32(key.data() + i * 4);
    state[12] = 1;
    for (int i = 0; i < 3; ++i) state[13 + i] = load32(nonce + i * 4);

    std::uint8_t stream[64];
    for (std::size_t offset = 0; offset < size; offset += 64, ++state[12]) {
        std::uint32_t x[16];
        std::memcpy(x, state, sizeof(x));
        for (int r = 0; r < 10; ++r) {
            quarterRound(x[0], x[4], x[8], x[12]);
            quarterRound(x[1], x[5], x[9], x[13]);
            quarterRound(x[2], x[6], x[10], x[14]);
            quarterRound(x[3], x[7], x[11], x[15]);
            quarterRound(x[0], x[5], x[10], x[15]);
            quarterRound(x[1], x[6], x[11], x[12]);
            quarterRound(x[2], x[7], x[8], x[13]);
            quarterRound(x[3], x[4], x[9], x[14]);
        }
        for (int i = 0; i < 16; ++i) {
            const std::uint32_t v = x[i] + state[i];
            for (int b = 0; b < 4; ++b) stream[i * 4 + b] = static_cast<std::uint8_t>(v >> (8 * b));
        }
        const std::size_t n = size - offset < 64 ? size - offset : 64;
        for (std::size_t i = 0; i < n; ++i) data[offset + i] ^= stream[i];
    }
}

std::uint64_t splitmix(std::uint64_t& s)
{
    std::uint64_t z = (s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

std::uint64_t fnv1a(std::uint64_t h, const void* data, std::size_t size)
{
    const auto* p = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i) h = (h ^ p[i]) * 0x100000001B3ull;
    return h;
}

std::uint64_t keyedHash(const Cipher::Key& key, std::uint64_t domain, const std::string& data)
{
    std::uint64_t h = fnv1a(0xCBF29CE484222325ull ^ domain, key.data(), key.size());
    h = fnv1a(h, data.data(), data.size());
    std::uint64_t s = h;
    return splitmix(s);
}

}

Cipher::Key Cipher::deriveKey(const std::string& passphrase)
{
    Key out{};
    std::uint64_t seed = fnv1a(0xCBF29CE484222325ull, passphrase.data(), passphrase.size());
    for (int round = 0; round < 4096; ++round)
        seed = fnv1a(splitmix(seed), passphrase.data(), passphrase.size());
    for (std::size_t i = 0; i < out.size(); i += 8) {
        const std::uint64_t v = splitmix(seed);
        for (int b = 0; b < 8; ++b) out[i + b] = static_cast<std::uint8_t>(v >> (8 * b));
    }
    return out;
}

static Cipher::Key& globalKey()
{
    static Cipher::Key instance = Cipher::deriveKey(SIGMA_YAML_KEY);
    return instance;
}

void Cipher::setKey(const std::string& passphrase) { globalKey() = deriveKey(passphrase); }
const Cipher::Key& Cipher::key() { return globalKey(); }

bool Cipher::isEncrypted(const std::string& data)
{
    return data.size() >= kHeaderSize && std::memcmp(data.data(), kMagic, 4) == 0;
}

std::string Cipher::encrypt(const std::string& plain) { return encrypt(plain, key()); }

std::string Cipher::encrypt(const std::string& plain, const Key& k)
{
    // Nonce déterministe (dérivé du contenu) : builds reproductibles.
    std::uint64_t seed = keyedHash(k, 1, plain);
    std::uint8_t nonce[16];
    for (int i = 0; i < 2; ++i) {
        const std::uint64_t v = splitmix(seed);
        for (int b = 0; b < 8; ++b) nonce[i * 8 + b] = static_cast<std::uint8_t>(v >> (8 * b));
    }
    const std::uint64_t tag = keyedHash(k, 2, plain);

    std::string out(kHeaderSize + plain.size(), '\0');
    std::memcpy(&out[0], kMagic, 4);
    out[4] = static_cast<char>(kVersion);
    std::memcpy(&out[8], nonce, kNonceSize);
    for (std::size_t b = 0; b < kTagSize; ++b) out[8 + kNonceSize + b] = static_cast<char>(tag >> (8 * b));
    std::memcpy(&out[kHeaderSize], plain.data(), plain.size());
    chacha20(reinterpret_cast<std::uint8_t*>(&out[kHeaderSize]), plain.size(), k, nonce);
    return out;
}

bool Cipher::decrypt(const std::string& data, std::string& plain) { return decrypt(data, plain, key()); }

bool Cipher::decrypt(const std::string& data, std::string& plain, const Key& k)
{
    if (!isEncrypted(data) || static_cast<std::uint8_t>(data[4]) != kVersion) return false;
    std::string out = data.substr(kHeaderSize);
    chacha20(reinterpret_cast<std::uint8_t*>(&out[0]), out.size(),
             k, reinterpret_cast<const std::uint8_t*>(data.data() + 8));
    std::uint64_t tag = 0;
    for (std::size_t b = 0; b < kTagSize; ++b)
        tag |= std::uint64_t(static_cast<std::uint8_t>(data[8 + kNonceSize + b])) << (8 * b);
    if (tag != keyedHash(k, 2, out)) return false;
    plain = std::move(out);
    return true;
}

}
