#pragma once
#include "world/npc.hpp"
#include <array>

namespace blobber {
constexpr std::size_t npcMaxFrames = 4;
struct NpcClipTiming {
    std::array<std::uint32_t, npcMaxFrames> durationMs{};
    std::uint8_t count = 0;
};
using NpcTimings = std::array<NpcClipTiming, npcAnimationCount>;
// Absent clips have count zero. Throws during loading on malformed timing.
void validateNpcTimings(const NpcTimings& timings);
struct NpcFrameSelection {
    std::uint8_t active = 0, idle = 0;
};
struct NpcAnimationSnapshot {
    std::array<NpcFrameSelection, NpcWorld::maxInstances> frames{};
    std::size_t count = 0;
    std::uint64_t revision = 0;
};
// Borrows one immutable level; metadata and storage are copied/preallocated at startup.
class NpcAnimator {
  public:
    NpcAnimator(const NpcWorld& world, const std::vector<NpcTimings>& timings);
    void setTime(std::uint64_t elapsedMs) noexcept;
    const NpcAnimationSnapshot& snapshot() const noexcept { return snapshot_; }

  private:
    const NpcWorld& world_;
    std::array<NpcTimings, NpcWorld::maxTypes> timings_{};
    NpcAnimationSnapshot snapshot_;
};
} // namespace blobber
