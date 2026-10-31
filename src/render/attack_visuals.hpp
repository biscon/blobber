#pragma once
#include "game/attack_timeline.hpp"
#include <algorithm>
namespace blobber {
// Cosmetic copy with an independently bounded capacity; no simulation callbacks.
class ProjectilePlayer {
  public:
    explicit ProjectilePlayer(std::size_t capacity = attackCapacity) noexcept
        : capacity_(std::min(capacity, attackCapacity)) {}
    bool update(const AttackVisualSnapshot*, std::uint64_t now) noexcept;
    const AttackVisual* begin() const noexcept { return shots_.data(); }
    const AttackVisual* end() const noexcept { return shots_.data() + count_; }
    std::size_t size() const noexcept { return count_; }
    std::uint64_t timeMs() const noexcept { return now_; }

  private:
    std::array<AttackVisual, attackCapacity> shots_{};
    std::size_t count_ = 0, capacity_;
    std::uint64_t now_ = 0, revision_ = 0, world_ = 0;
};
} // namespace blobber
