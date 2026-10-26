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
std::uint32_t npcClipDuration(const NpcClipTiming& clip) noexcept {
    std::uint32_t total = 0;
    for (std::size_t i = 0; i < clip.count; ++i)
        total += clip.durationMs[i];
    return total;
}
std::uint32_t npcAttackImpact(const NpcClipTiming& clip) noexcept {
    std::uint32_t total = 0;
    for (std::size_t i = 0; i < clip.count / 2; ++i)
        total += clip.durationMs[i];
    return total;
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
        const auto& hurt = timings[static_cast<std::size_t>(NpcAnimation::Hurt)];
        const auto& death = timings[static_cast<std::size_t>(NpcAnimation::Death)];
        const bool dying = actor.life == ActorLife::Dead && death.count;
        const bool deathVisible = dying && actor.hasDamage && elapsedMs >= actor.damageTimeMs &&
                                  elapsedMs - actor.damageTimeMs < npcClipDuration(death);
        const bool visible = actor.life == ActorLife::Alive || (dying ? deathVisible : flash);
        // Legacy death art freezes the exact living pose at the lethal timestamp.
        const auto time = actor.life == ActorLife::Dead && !dying ? actor.damageTimeMs : elapsedMs;
        NpcFrameSelection next{0, 0, visible, flash};
        const auto position = actorPosition(actor, elapsedMs);
        next.offsetX = position.x - (npc.cell.x + .25f + npc.subcell.x * .5f);
        next.offsetY = position.y - (npc.cell.y + .25f + npc.subcell.y * .5f);
        next.clip = npc.animation;
        if (visible) {
            const auto& attack = timings[static_cast<std::size_t>(NpcAnimation::Attack)];
            const bool damageCue = hurt.count && actor.hasDamage && time >= actor.damageTimeMs;
            const bool attackCue = actor.hasAttack && time >= actor.attackTimeMs;
            const bool hurtWins = damageCue && (!attackCue || actor.damageTimeMs >= actor.attackTimeMs);
            if (dying) {
                next.clip = NpcAnimation::Death;
                next.active = sample(death, time - actor.damageTimeMs, 0);
            } else if (hurtWins && time - actor.damageTimeMs < npcClipDuration(hurt)) {
                next.clip = NpcAnimation::Hurt;
                next.active = sample(hurt, time - actor.damageTimeMs, 0);
            } else if (!hurtWins && attackCue && time - actor.attackTimeMs < npcClipDuration(attack)) {
                next.clip = NpcAnimation::Attack;
                next.active = sample(attack, time - actor.attackTimeMs, 0);
            } else if (actor.hasWalk && (!actor.hasAttack || actor.walkTimeMs > actor.attackTimeMs) &&
                       (!actor.hasDamage || actor.walkTimeMs > actor.damageTimeMs ||
                        (actor.life == ActorLife::Dead && !dying)) &&
                       time >= actor.walkTimeMs && time - actor.walkTimeMs < npcWalkDurationMs &&
                       timings[static_cast<std::size_t>(NpcAnimation::Walk)].count) {
                next.clip = NpcAnimation::Walk;
                next.active =
                    sample(timings[static_cast<std::size_t>(NpcAnimation::Walk)], time - actor.walkTimeMs, 0);
            } else {
                const auto phase = npc.animation == NpcAnimation::Idle ? idlePhases_[i] : npc.phaseMs;
                next.active = sample(timings[static_cast<std::size_t>(npc.animation)], time, phase);
            }
            // Only an attack can expose a directional idle fallback. Keep legacy
            // idle snapshots stable; reaction/corpse holds need no idle invalidation.
            if (next.clip <= NpcAnimation::Attack)
                next.idle = sample(timings[0], time, idlePhases_[i]);
        }
        auto& old = snapshot_.frames[i];
        changed |= old.active != next.active || old.idle != next.idle || old.visible != next.visible ||
                   old.flash != next.flash || old.clip != next.clip || old.offsetX != next.offsetX ||
                   old.offsetY != next.offsetY;
        old = next;
    }
    snapshot_.actorRevision = world_.snapshot().revision;
    if (changed)
        ++snapshot_.revision;
}
} // namespace blobber
