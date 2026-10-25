#pragma once
#include "render/layout.hpp"
#include <algorithm>
#include <memory>
#include <raylib.h>

namespace blobber {
// Fixed capacity, allocated only at construction. Deliberately not copyable or resizable.
class Frame {
  public:
    static constexpr std::size_t pixelCount = viewWidth * viewHeight;
    explicit Frame(Color fill = BLACK) : pixels_(std::make_unique<Color[]>(pixelCount)) {
        std::fill_n(data(), pixelCount, fill);
    }
    Frame(Frame&&) noexcept = default;
    Frame& operator=(Frame&&) noexcept = default;
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    Color* data() noexcept { return pixels_.get(); }
    const Color* data() const noexcept { return pixels_.get(); }
    Color* begin() noexcept { return data(); }
    Color* end() noexcept { return data() + pixelCount; }
    const Color* begin() const noexcept { return data(); }
    const Color* end() const noexcept { return data() + pixelCount; }
    constexpr std::size_t size() const noexcept { return pixelCount; }
    Color& operator[](std::size_t index) { return data()[index]; }
    const Color& operator[](std::size_t index) const { return data()[index]; }
    void copyFrom(const Frame& other) noexcept {
        if (data() != other.data())
            std::copy_n(other.data(), pixelCount, data());
    }

  private:
    std::unique_ptr<Color[]> pixels_;
};
} // namespace blobber
