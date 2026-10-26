#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace blobber {
// Deterministic change/corruption detection, not a cryptographic authenticity check.
inline constexpr std::uint64_t fingerprintBasis = 14695981039346656037ull;
inline void fingerprintBytes(std::uint64_t& hash, const void* bytes, std::size_t count) {
    const auto* p = static_cast<const unsigned char*>(bytes);
    for (std::size_t i = 0; i < count; ++i)
        hash = (hash ^ p[i]) * 1099511628211ull;
}
inline std::string fingerprintFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot fingerprint " + path.string());
    std::uint64_t hash = fingerprintBasis;
    char block[16384];
    while (input.read(block, sizeof(block)) || input.gcount())
        fingerprintBytes(hash, block, static_cast<std::size_t>(input.gcount()));
    if (input.bad())
        throw std::runtime_error("Cannot read " + path.string());
    return std::to_string(hash);
}
} // namespace blobber
