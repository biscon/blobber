#include "assets/prepared_binary.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>

namespace blobber::prepared_io {
namespace {
template <class T> void configuration(T& io) {
    // Writer and reader deliberately share the signature's ordered constants.
    io.u32(formatVersion);
    io.u32(rendererRevision);
    io.u32(viewWidth);
    io.u32(viewHeight);
    io.u32(ViewProjection::visibleRows);
    io.u32(ViewProjection::strafeSteps);
    io.f32(ViewProjection::horizontalFovDegrees);
    io.f32(ViewProjection::cameraSetback);
    io.f32(ViewProjection::horizon);
    io.f32(ViewProjection::eyeHeight);
    io.f32(ViewProjection::wallHeight);
}
struct CheckConfiguration {
    Reader& in;
    void u32(std::uint32_t value) { Reader::require(in.u32() == value); }
    void f32(float value) { Reader::require(in.f32() == value); }
};
} // namespace
Writer::Writer(const std::filesystem::path& destination, std::string_view magic, std::uint64_t profile,
               std::uint64_t definitions)
    : destination_(destination) {
    if (magic.size() != 8)
        throw std::invalid_argument("Bundle magic must have eight bytes");
    if (!destination.parent_path().empty())
        std::filesystem::create_directories(destination.parent_path());
    static std::atomic<unsigned> serial{0};
    temporary_ = destination;
    temporary_ += ".tmp-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                  "-" + std::to_string(serial++);
    out_.open(temporary_, std::ios::binary | std::ios::trunc);
    if (!out_)
        throw std::runtime_error("Cannot write prepared bundle: " + destination.string());
    try {
        bytes(magic.data(), magic.size());
        configuration(*this);
        u64(profile);
        u64(definitions);
    } catch (...) {
        out_.close();
        std::error_code error;
        std::filesystem::remove(temporary_, error);
        throw;
    }
}
Writer::~Writer() {
    out_.close();
    if (!finished_) {
        std::error_code e;
        std::filesystem::remove(temporary_, e);
    }
}
void Writer::bytes(const void* data, std::size_t count) {
    if (!count)
        return;
    out_.write(static_cast<const char*>(data), static_cast<std::streamsize>(count));
    if (!out_)
        throw std::runtime_error("Cannot write prepared bundle");
    fingerprintBytes(hash_, data, count);
}
void Writer::u32(std::uint32_t value) {
    unsigned char data[4];
    for (int i = 0; i < 4; ++i)
        data[i] = static_cast<unsigned char>(value >> (8 * i));
    bytes(data, sizeof(data));
}
void Writer::u64(std::uint64_t value) {
    unsigned char data[8];
    for (int i = 0; i < 8; ++i)
        data[i] = static_cast<unsigned char>(value >> (8 * i));
    bytes(data, sizeof(data));
}
void Writer::f32(float value) {
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    u32(bits);
}
void Writer::finish() {
    const auto checksum = hash_;
    u64(checksum);
    out_.close();
    if (!out_)
        throw std::runtime_error("Cannot finish prepared bundle");
    std::filesystem::rename(temporary_, destination_);
    finished_ = true;
}
Reader::Reader(const std::filesystem::path& path, std::string_view magic, std::uint64_t profile,
               std::uint64_t definitions, PreparationLimits limits)
    : in_(path, std::ios::binary), limit_(std::min(limits.residentBytes, limits.loadingBytes)) {
    if (!in_)
        throw std::runtime_error(
            "Missing prepared bundle: " + path.string() +
            ". Rebuild with cmake --build --preset debug --target blobber_bake_runtime_assets");
    remaining_ = std::filesystem::file_size(path);
    require(remaining_ >= 76 && remaining_ <= 600u * 1024u * 1024u && magic.size() == 8);
    char actual[8];
    bytes(actual, 8);
    require(std::memcmp(actual, magic.data(), 8) == 0);
    CheckConfiguration expected{*this};
    configuration(expected);
    require(u64() == profile);
    require(u64() == definitions);
}
void Reader::require(bool valid) {
    if (!valid)
        throw std::runtime_error(
            "Invalid, corrupt or incompatible prepared bundle; run cmake --build --preset debug --target "
            "blobber_bake_runtime_assets (remove stale assets/prepared/*.bin first if necessary)");
}
void Reader::bytes(void* data, std::size_t count) {
    require(count <= remaining_);
    if (count) {
        in_.read(static_cast<char*>(data), static_cast<std::streamsize>(count));
        require(bool(in_));
        fingerprintBytes(hash_, data, count);
    }
    remaining_ -= count;
}
std::uint32_t Reader::u32() {
    unsigned char data[4];
    bytes(data, 4);
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i)
        value |= std::uint32_t(data[i]) << (8 * i);
    return value;
}
std::uint64_t Reader::u64() {
    unsigned char data[8];
    bytes(data, 8);
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value |= std::uint64_t(data[i]) << (8 * i);
    return value;
}
int Reader::i32() {
    const auto value = u32();
    return value <= 0x7fffffff ? static_cast<int>(value)
                               : static_cast<int>(std::int64_t(value) - 0x100000000ll);
}
float Reader::f32() {
    const auto bits = u32();
    float value;
    std::memcpy(&value, &bits, 4);
    require(std::isfinite(value));
    return value;
}
void Reader::claim(std::size_t bytes) {
    requirePreparationBytes(allocated_, bytes, limit_);
    allocated_ += bytes;
}
std::size_t Reader::count(std::size_t maximum, std::size_t allocationStride, std::size_t wireStride) {
    const auto n = u64();
    require(n <= maximum && wireStride && n <= remaining_ / wireStride);
    require(n <= std::numeric_limits<std::size_t>::max() / allocationStride);
    claim(static_cast<std::size_t>(n) * allocationStride);
    return static_cast<std::size_t>(n);
}
void Reader::finish() {
    require(remaining_ == 8);
    const auto expected = hash_;
    const auto actual = u64();
    require(actual == expected);
}
} // namespace blobber::prepared_io
