#pragma once
#include "game/actors.hpp"
#include <array>

namespace blobber {
class EffectController;
constexpr std::size_t npcMaxFrames = 4;
struct NpcClipTiming {
    std::array<std::uint32_t, npcMaxFrames> durationMs{};
    std::uint8_t count = 0;
};
using NpcTimings = std::array<NpcClipTiming, npcAnimationCount>;
// Absent clips have count zero. Throws during loading on malformed timing.
void validateNpcTimings(const NpcTimings& timings);
std::uint32_t npcClipDuration(const NpcClipTiming& clip) noexcept;
std::uint32_t npcAttackImpact(const NpcClipTiming& clip) noexcept;
struct NpcFrameSelection {
    std::uint8_t active = 0, idle = 0;
    bool visible = true, flash = false;
    NpcAnimation clip = NpcAnimation::Idle;
    float offsetX = 0, offsetY = 0;
};
struct NpcAnimationSnapshot {
    std::array<NpcFrameSelection, NpcWorld::maxInstances> frames{};
    std::size_t count = 0;
    std::uint64_t revision = 0, world = 0, actorRevision = 0;
};
// Borrows one runtime actor world; metadata and storage are copied/preallocated at startup.
constexpr std::uint64_t npcHitFlashMs = 100;
class NpcAnimator {
  public:
    NpcAnimator(const ActorWorld& world, const std::vector<NpcTimings>& timings);
    NpcAnimator(ActorWorld&&, const std::vector<NpcTimings>&) = delete;
    NpcAnimator(const ActorWorld&&, const std::vector<NpcTimings>&) = delete;
    void setTime(std::uint64_t elapsedMs, const EffectController* effects = nullptr) noexcept;
    const NpcAnimationSnapshot& snapshot() const noexcept { return snapshot_; }

  private:
    const ActorWorld& world_;
    std::array<NpcTimings, NpcWorld::maxTypes> timings_{};
    std::array<std::uint32_t, NpcWorld::maxInstances> idlePhases_{};
    NpcAnimationSnapshot snapshot_;
};
} // namespace blobber
