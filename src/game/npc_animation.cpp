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
NpcAnimator::NpcAnimator(const ActorWorld& world, const std::vector<NpcTimings>& timings) : world_(world) {
    if (timings.size() != world.snapshot().typeCount)
        throw std::invalid_argument("NPC animation/world type counts differ");
    for (std::size_t i = 0; i < timings.size(); ++i) {
        validateNpcTimings(timings[i]);
        timings_[i] = timings[i];
    }
    for (std::size_t i = 0; i < world.snapshot().count; ++i) {
        const auto& npc = world.snapshot().actors[i].npc;
        if (!timings_[npc.type.index][static_cast<std::size_t>(npc.animation)].count)
            throw std::runtime_error("NPC placement selects an absent animation");
        std::uint32_t cycle = 0;
        const auto& idle = timings_[npc.type.index][0];
        for (std::size_t frame = 0; frame < idle.count; ++frame)
            cycle += idle.durationMs[frame];
        // A golden-ratio sequence spreads stable slots around the idle cycle.
        // Explicit phases override it; slot zero retains the unshifted timeline.
        const auto fraction = static_cast<std::uint32_t>(i) * 0x9e3779b9u;
        idlePhases_[i] = npc.phaseMs ? npc.phaseMs : (std::uint64_t(fraction) * cycle) >> 32;
    }
    snapshot_.count = world.snapshot().count;
    snapshot_.world = world.snapshot().world;
    setTime(0);
}
void NpcAnimator::setTime(std::uint64_t elapsedMs) noexcept {
    bool changed = false;
    for (std::size_t i = 0; i < snapshot_.count; ++i) {
        const auto& actor = world_.snapshot().actors[i];
        const auto& npc = actor.npc;
        const auto& timings = timings_[npc.type.index];
        const bool flash = actor.life != ActorLife::Removed && actor.hasDamage &&
                           elapsedMs >= actor.damageTimeMs && elapsedMs - actor.damageTimeMs < npcHitFlashMs;
        const bool visible = actor.life == ActorLife::Alive || flash;
        const auto time = actor.life == ActorLife::Dead ? actor.damageTimeMs : elapsedMs;
        NpcFrameSelection next{0, 0, visible, flash};
        if (visible) {
            const auto phase = npc.animation == NpcAnimation::Idle ? idlePhases_[i] : npc.phaseMs;
            next.active = sample(timings[static_cast<std::size_t>(npc.animation)], time, phase);
            next.idle = sample(timings[0], time, idlePhases_[i]);
        }
        auto& old = snapshot_.frames[i];
        changed |= old.active != next.active || old.idle != next.idle || old.visible != next.visible ||
                   old.flash != next.flash;
        old = next;
    }
    snapshot_.actorRevision = world_.snapshot().revision;
    if (changed)
        ++snapshot_.revision;
}
} // namespace blobber
