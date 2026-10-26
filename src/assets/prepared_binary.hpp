#pragma once
#include "assets/fingerprint.hpp"
#include "render/preparation_limits.hpp"
#include "render/projection.hpp"
#include <cstring>
#include <fstream>
#include <limits>
#include <string_view>

namespace blobber::prepared_io {
// Field encoding is little-endian; floats are IEEE binary32 and colors are RGBA8.
// Bump revision when projection, lighting or preparation semantics change.
constexpr std::uint32_t formatVersion = 1, rendererRevision = 1;
class Writer {
  public:
    Writer(const std::filesystem::path& destination, std::string_view magic, std::uint64_t profile,
           std::uint64_t definitions);
    ~Writer();
    void bytes(const void* data, std::size_t count);
    void u32(std::uint32_t value);
    void u64(std::uint64_t value);
    void i32(int value) { u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value))); }
    void f32(float value);
    void finish();

  private:
    std::filesystem::path destination_, temporary_;
    std::ofstream out_;
    std::uint64_t hash_ = fingerprintBasis;
    bool finished_ = false;
};
class Reader {
  public:
    Reader(const std::filesystem::path& path, std::string_view magic, std::uint64_t profile,
           std::uint64_t definitions, PreparationLimits limits);
    void bytes(void* data, std::size_t count);
    std::uint32_t u32();
    std::uint64_t u64();
    int i32();
    float f32();
    std::size_t count(std::size_t maximum, std::size_t allocationStride, std::size_t wireStride);
    void claim(std::size_t bytes);
    void finish();
    static void require(bool valid);

  private:
    std::ifstream in_;
    std::uint64_t remaining_ = 0, hash_ = fingerprintBasis;
    std::size_t allocated_ = 0, limit_ = 0;
};
} // namespace blobber::prepared_io
