#include "game/npc_animation.hpp"
#include <stdexcept>
namespace blobber {
void validateNpcTimings(const NpcTimings& timings) {
    if (!timings[0].count)
        throw std::runtime_error("NPC requires idle animation");
    for (const auto& clip : timings) {
        if (clip.count > npcMaxFrames)
            throw std::runtime_error("NPC clips require 1..4 frames");
        for (std::size_t i = 0; i < clip.count; ++i)
            if (clip.durationMs[i] == 0 || clip.durationMs[i] > 60000)
                throw std::runtime_error("NPC frame duration must be 1..60000 ms");
    }
}
namespace {
std::uint8_t sample(const NpcClipTiming& clip, std::uint64_t time, std::uint32_t phase) noexcept {
    std::uint32_t total = 0;
    for (std::size_t i = 0; i < clip.count; ++i)
        total += clip.durationMs[i];
    // Modulo before addition also handles the full uint64 timestamp range.
    auto t = (time % total + phase % total) % total;
    for (std::uint8_t i = 0; i < clip.count; ++i) {
        if (t < clip.durationMs[i])
            return i;
        t -= clip.durationMs[i];
    }
    return 0;
}
} // namespace
NpcAnimator::NpcAnimator(const NpcWorld& world, const std::vector<NpcTimings>& timings) : world_(world) {
    if (timings.size() != world.typeCount())
        throw std::invalid_argument("NPC animation/world type counts differ");
    for (std::size_t i = 0; i < timings.size(); ++i) {
        validateNpcTimings(timings[i]);
        timings_[i] = timings[i];
    }
    for (const auto& npc : world.instances())
        if (!timings_[npc.type.index][static_cast<std::size_t>(npc.animation)].count)
            throw std::runtime_error("NPC placement selects an absent animation");
    snapshot_.count = world.instances().size();
    setTime(0);
}
void NpcAnimator::setTime(std::uint64_t elapsedMs) noexcept {
    bool changed = false;
    for (std::size_t i = 0; i < snapshot_.count; ++i) {
        const auto& npc = world_.instances()[i];
        const auto& timings = timings_[npc.type.index];
        const NpcFrameSelection next{
            sample(timings[static_cast<std::size_t>(npc.animation)], elapsedMs, npc.phaseMs),
            sample(timings[0], elapsedMs, npc.phaseMs)};
        auto& old = snapshot_.frames[i];
        changed |= old.active != next.active || old.idle != next.idle;
        old = next;
    }
    if (changed)
        ++snapshot_.revision;
}
} // namespace blobber
