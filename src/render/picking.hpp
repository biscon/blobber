#pragma once
#include "game/commands.hpp"
#include "render/layout.hpp"
#include <algorithm>
#include <memory>

namespace blobber {
// One fixed allocation; never resized. Slot IDs are meaningful only with targets.world.
class PickingBuffer {
  public:
    static constexpr std::uint16_t empty = UINT16_MAX;
    PickingBuffer() : pixels_(std::make_unique<std::uint16_t[]>(viewWidth * viewHeight)) { clear(); }
    void clear() noexcept {
        std::fill_n(pixels_.get(), viewWidth * viewHeight, empty);
        targets_ = {};
    }
    void write(std::size_t pixel, std::uint16_t slot) noexcept { pixels_[pixel] = slot; }
    std::uint16_t slot(int x, int y) const noexcept {
        return x >= 0 && x < viewWidth && y >= 0 && y < viewHeight ? pixels_[y * viewWidth + x] : empty;
    }
    void finish(const ActorWorld* actors) noexcept {
        targets_ = {};
        if (!actors)
            return;
        targets_.world = actors->snapshot().world;
        targets_.revision = actors->snapshot().revision;
        std::array<bool, NpcWorld::maxInstances> seen{};
        for (int i = 0; i < viewWidth * viewHeight; ++i)
            if (pixels_[i] < seen.size())
                seen[pixels_[i]] = true;
        for (std::size_t i = 0; i < actors->snapshot().count; ++i)
            if (seen[i] && actors->snapshot().actors[i].life == ActorLife::Alive)
                targets_.ids[targets_.count++] = actors->id(i);
    }
    std::optional<ActorId> pick(int x, int y) const noexcept {
        const auto s = slot(x, y);
        for (std::size_t i = 0; i < targets_.count; ++i)
            if (targets_.ids[i].slot == s)
                return targets_.ids[i];
        return {};
    }
    const TargetSnapshot& targets() const noexcept { return targets_; }

  private:
    std::unique_ptr<std::uint16_t[]> pixels_;
    TargetSnapshot targets_;
};
} // namespace blobber
