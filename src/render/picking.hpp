#pragma once
#include "game/commands.hpp"
#include "render/layout.hpp"
#include <algorithm>
#include <limits>
#include <memory>

namespace blobber {
// One fixed allocation; never resized. Slot IDs are meaningful only with targets.world.
class PickingBuffer {
  public:
    static constexpr std::uint16_t fixtureBase = NpcWorld::maxInstances + lootCapacity;
    static constexpr std::uint16_t empty = UINT16_MAX;
    PickingBuffer() : pixels_(std::make_unique<std::uint16_t[]>(viewWidth * viewHeight)) { clear(); }
    void clear() noexcept {
        std::fill_n(pixels_.get(), viewWidth * viewHeight, empty);
        targets_ = {};
        badgeAnchors_ = {};
        bodyDepth_.fill(std::numeric_limits<float>::infinity());
        wallDepth_.fill(std::numeric_limits<float>::infinity());
        lootWorld_ = 0;
        fixtureWorld_ = 0;
        lootAnchors_ = {};
        lootGenerations_ = {};
    }
    void badgeAnchor(std::uint16_t slot, int x, int top, float depth) noexcept {
        badgeAnchors_[slot] = {x, top};
        bodyDepth_[slot] = depth;
    }
    void wallDepths(const std::array<float, viewWidth>& depths) noexcept { wallDepth_ = depths; }
    bool badgeVisible(std::uint16_t owner, int x, int y) const noexcept {
        if (owner >= bodyDepth_.size() || x < 0 || x >= viewWidth || y < 0 || y >= viewHeight)
            return false;
        const auto depth = bodyDepth_[owner];
        if (depth >= wallDepth_[x])
            return false;
        const auto body = slot(x, y);
        return body == owner || body >= bodyDepth_.size() || bodyDepth_[body] > depth;
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
        std::array<Cell, NpcWorld::maxInstances> minimum{}, maximum{};
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                const auto slot = pixels_[y * viewWidth + x];
                if (slot >= seen.size())
                    continue;
                if (!seen[slot]) {
                    minimum[slot] = maximum[slot] = {x, y};
                    seen[slot] = true;
                } else {
                    minimum[slot].x = std::min(minimum[slot].x, x);
                    minimum[slot].y = std::min(minimum[slot].y, y);
                    maximum[slot].x = std::max(maximum[slot].x, x);
                    maximum[slot].y = std::max(maximum[slot].y, y);
                }
            }
        for (std::size_t i = 0; i < actors->snapshot().count; ++i)
            if (seen[i]) {
                if (actors->snapshot().actors[i].life == ActorLife::Alive)
                    targets_.ids[targets_.count++] = actors->id(i);
                targets_.anchors[i] = {(minimum[i].x + maximum[i].x) / 2,
                                       (minimum[i].y + maximum[i].y) / 2,
                                       true,
                                       minimum[i].y,
                                       badgeAnchors_[i].x,
                                       badgeAnchors_[i].y};
            }
    }
    std::optional<ActorId> pick(int x, int y) const noexcept {
        const auto s = slot(x, y);
        for (std::size_t i = 0; i < targets_.count; ++i)
            if (targets_.ids[i].slot == s)
                return targets_.ids[i];
        return {};
    }
    void lootGeneration(std::uint16_t slot, std::uint64_t generation) noexcept {
        lootGenerations_[slot] = generation;
    }
    void lootWorld(std::uint64_t world) noexcept { lootWorld_ = world; }
    void lootAnchor(std::uint16_t slot, int x, int y) noexcept { lootAnchors_[slot] = {x, y}; }
    Cell lootAnchor(std::uint16_t slot) const noexcept {
        return slot < lootCapacity ? lootAnchors_[slot] : Cell{};
    }
    std::optional<LootId> pickLoot(int x, int y) const noexcept {
        const auto s = slot(x, y);
        if (lootWorld_ && s >= NpcWorld::maxInstances && s < NpcWorld::maxInstances + lootCapacity)
            return LootId{lootWorld_, static_cast<std::uint16_t>(s - NpcWorld::maxInstances),
                          lootGenerations_[s - NpcWorld::maxInstances]};
        return {};
    }
    void fixtureWorld(std::uint64_t world) noexcept { fixtureWorld_ = world; }
    std::optional<FixtureId> pickFixture(int x, int y) const noexcept {
        const auto s = slot(x, y);
        if (fixtureWorld_ && s >= fixtureBase && s < fixtureBase + fixtureCapacity)
            return FixtureId{fixtureWorld_, std::uint16_t(s - fixtureBase)};
        return {};
    }
    const TargetSnapshot& targets() const noexcept { return targets_; }

  private:
    std::unique_ptr<std::uint16_t[]> pixels_;
    TargetSnapshot targets_;
    std::uint64_t lootWorld_ = 0, fixtureWorld_ = 0;
    std::array<Cell, lootCapacity> lootAnchors_{};
    std::array<std::uint64_t, lootCapacity> lootGenerations_{};
    std::array<Cell, NpcWorld::maxInstances> badgeAnchors_{};
    std::array<float, NpcWorld::maxInstances> bodyDepth_{};
    std::array<float, viewWidth> wallDepth_{};
};
} // namespace blobber
