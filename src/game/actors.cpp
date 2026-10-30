#include "game/actors.hpp"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <stdexcept>

namespace blobber {
namespace {
std::uint64_t nextWorld() {
    static std::atomic<std::uint64_t> next{1};
    auto value = next.load(std::memory_order_relaxed);
    for (;;) {
        if (value == std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("Actor world identity exhausted");
        if (next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed))
            return value;
    }
}
std::uint8_t bit(Cell subcell) noexcept {
    return static_cast<std::uint8_t>(1u << (subcell.y * 2 + subcell.x));
}
} // namespace
bool actorWalking(const ActorState& a, std::uint64_t timeMs) noexcept {
    return a.hasTravel && a.life == ActorLife::Alive &&
           (timeMs < a.walkTimeMs || timeMs - a.walkTimeMs < a.walkDurationMs);
}
ActorPosition actorPosition(const ActorState& a, std::uint64_t timeMs) noexcept {
    const ActorPosition destination{a.npc.cell.x + .25f + a.npc.subcell.x * .5f,
                                    a.npc.cell.y + .25f + a.npc.subcell.y * .5f};
    if (!a.hasTravel)
        return destination;
    if (a.life == ActorLife::Dead)
        timeMs = std::min(timeMs, a.damageTimeMs);
    const float t =
        timeMs < a.walkTimeMs ? 0.f : std::min(1.f, float(timeMs - a.walkTimeMs) / a.walkDurationMs);
    return {a.walkFrom.x + (destination.x - a.walkFrom.x) * t,
            a.walkFrom.y + (destination.y - a.walkFrom.y) * t};
}
ActorWorld::ActorWorld(GeometryView map, const rules::Catalog& catalog, const NpcWorld& placements)
    : map_(map), catalog_(catalog), occupancy_(static_cast<std::size_t>(map.width()) * map.height(), 0) {
    state_.world = nextWorld();
    state_.count = placements.instances().size();
    state_.typeCount = placements.typeCount();
    std::array<rules::MonsterId, NpcWorld::maxTypes> bindings{};
    for (std::size_t i = 0; i < state_.typeCount; ++i) {
        const auto& name = placements.typeId({static_cast<std::uint16_t>(i)});
        bindings[i] = catalog.findMonster(name);
        if (!catalog.monster(bindings[i]))
            throw std::runtime_error("NPC appearance has no monster definition: " + name);
    }
    for (std::size_t i = 0; i < state_.count; ++i) {
        auto& actor = state_.actors[i];
        actor.npc = placements.instances()[i];
        if (!map.npcAllowed(actor.npc.cell))
            throw std::runtime_error("Actor placement overlaps runtime map wall or boundary");
        actor.monster = actor.npc.monster.value_or(bindings[actor.npc.type.index]);
        if (!catalog.monster(actor.monster))
            throw std::runtime_error("Actor placement has an invalid monster definition");
        actor.hitPoints = actor.maxHitPoints = catalog.monster(actor.monster)->hitPoints;
        mask(actor.npc.cell) |= bit(actor.npc.subcell);
    }
}
ActorId ActorWorld::id(std::size_t slot) const noexcept {
    if (slot >= state_.count || state_.actors[slot].life == ActorLife::Removed)
        return {};
    return {static_cast<std::uint16_t>(slot), state_.world};
}
const ActorState* ActorWorld::actor(ActorId id) const noexcept {
    if (id.world != state_.world || id.slot >= state_.count ||
        state_.actors[id.slot].life == ActorLife::Removed)
        return nullptr;
    return &state_.actors[id.slot];
}
bool ActorWorld::occupied(Cell c) const noexcept {
    return c.x >= 0 && c.y >= 0 && c.x < map_.width() && c.y < map_.height() &&
           occupancy_[static_cast<std::size_t>(c.y) * map_.width() + c.x] != 0;
}
std::uint8_t ActorWorld::occupiedSlots(Cell c) const noexcept {
    return map_.solid(c) ? 15 : occupancy_[static_cast<std::size_t>(c.y) * map_.width() + c.x];
}
ActorResult ActorWorld::walk(ActorId id, Cell cell, Cell subcell, Cell partyCell, std::uint64_t timeMs,
                             std::uint64_t durationMs) noexcept {
    const auto* a = actor(id);
    if (!durationMs)
        return {ActorError::InvalidTime};
    if (!a)
        return {ActorError::InvalidHandle};
    if (a->hasWalk && timeMs < a->walkTimeMs)
        return {ActorError::InvalidTime};
    const bool crossCell = !(a->npc.cell == cell);
    const auto from = actorPosition(*a, timeMs);
    const auto result = move(id, cell, subcell, partyCell);
    if (result.changed && crossCell) {
        state_.actors[id.slot].hasTravel = true;
        state_.actors[id.slot].walkFrom = from;
        state_.actors[id.slot].hasWalk = true;
        state_.actors[id.slot].walkTimeMs = timeMs;
        state_.actors[id.slot].walkDurationMs = durationMs;
    }
    return result;
}
std::uint8_t& ActorWorld::mask(Cell c) noexcept {
    return occupancy_[static_cast<std::size_t>(c.y) * map_.width() + c.x];
}
void ActorWorld::vacate(const ActorState& a) noexcept {
    mask(a.npc.cell) &= static_cast<std::uint8_t>(~bit(a.npc.subcell));
}
ActorResult ActorWorld::move(ActorId id, Cell cell, Cell subcell, Cell partyCell) noexcept {
    const auto* old = actor(id);
    if (!old)
        return {ActorError::InvalidHandle};
    if (old->life != ActorLife::Alive)
        return {ActorError::NotAlive};
    if (!map_.npcAllowed(cell) || subcell.x < 0 || subcell.x > 1 || subcell.y < 0 || subcell.y > 1)
        return {ActorError::InvalidDestination};
    if (std::abs(cell.x - old->npc.cell.x) + std::abs(cell.y - old->npc.cell.y) > 1)
        return {ActorError::InvalidDestination};
    if (cell == partyCell)
        return {ActorError::PartyCell};
    if (cell == old->npc.cell && subcell == old->npc.subcell)
        return {};
    if (mask(cell) & bit(subcell))
        return {ActorError::Occupied};
    auto& a = state_.actors[id.slot];
    vacate(a);
    mask(cell) |= bit(subcell);
    a.hasTravel = false;
    a.npc.cell = cell;
    a.npc.subcell = subcell;
    ++state_.revision;
    return {ActorError::None, true};
}
ActorResult ActorWorld::face(ActorId id, Heading facing) noexcept {
    const auto* old = actor(id);
    if (!old)
        return {ActorError::InvalidHandle};
    if (old->life != ActorLife::Alive)
        return {ActorError::NotAlive};
    if (static_cast<unsigned>(facing) >= 4)
        return {ActorError::InvalidFacing};
    if (old->npc.facing == facing)
        return {};
    state_.actors[id.slot].npc.facing = facing;
    ++state_.revision;
    return {ActorError::None, true};
}
ActorResult ActorWorld::attack(ActorId id, std::uint64_t timeMs, bool ranged) noexcept {
    const auto* old = actor(id);
    if (!old)
        return {ActorError::InvalidHandle};
    if (old->life != ActorLife::Alive)
        return {ActorError::NotAlive};
    if (old->hasAttack && timeMs < old->attackTimeMs)
        return {ActorError::InvalidTime};
    auto& a = state_.actors[id.slot];
    a.hasAttack = true;
    a.rangedAttack = ranged;
    a.attackTimeMs = timeMs;
    a.resumableAttack = false;
    a.attackPausedMs = a.attackPauseFromMs = a.attackPauseUntilMs = 0;
    ++state_.revision;
    return {ActorError::None, true};
}
ActorResult ActorWorld::suspendAttack(ActorId id, std::uint64_t when, std::uint64_t duration) noexcept {
    if (!actor(id))
        return {ActorError::InvalidHandle};
    auto& a = state_.actors[id.slot];
    if (!a.hasAttack || when < a.attackTimeMs)
        return {ActorError::InvalidTime};
    if (when >= a.attackPauseUntilMs) {
        a.attackPausedMs += a.attackPauseUntilMs - a.attackPauseFromMs;
        a.attackPauseFromMs = when;
    }
    a.attackPauseUntilMs = std::max(a.attackPauseUntilMs, when + duration);
    a.resumableAttack = true;
    ++state_.revision;
    return {ActorError::None, true};
}
ActorResult ActorWorld::cancelAttack(ActorId id) noexcept {
    if (!actor(id))
        return {ActorError::InvalidHandle};
    auto& a = state_.actors[id.slot];
    a.hasAttack = a.resumableAttack = false;
    ++state_.revision;
    return {ActorError::None, true};
}
ActorResult ActorWorld::damage(ActorId id, std::int64_t amount, std::uint64_t timeMs) noexcept {
    const auto* old = actor(id);
    if (!old)
        return {ActorError::InvalidHandle};
    if (old->life != ActorLife::Alive)
        return {ActorError::NotAlive};
    if (amount < 0)
        return {ActorError::InvalidDamage};
    if (!amount)
        return {};
    if (old->hasDamage && timeMs < old->damageTimeMs)
        return {ActorError::InvalidTime};
    auto& a = state_.actors[id.slot];
    const auto applied = static_cast<std::int32_t>(std::min<std::int64_t>(amount, a.hitPoints));
    a.hitPoints -= applied;
    a.hasDamage = true;
    a.damageTimeMs = timeMs;
    const bool died = a.hitPoints == 0;
    if (died) {
        a.life = ActorLife::Dead;
        a.hasDeath = true;
        vacate(a);
    }
    ++state_.revision;
    return {ActorError::None, true, died, applied};
}
ActorResult ActorWorld::remove(ActorId id) noexcept {
    const auto* old = actor(id);
    if (!old)
        return {ActorError::InvalidHandle};
    auto& a = state_.actors[id.slot];
    if (a.life == ActorLife::Alive)
        vacate(a);
    a.life = ActorLife::Removed;
    ++state_.revision;
    return {ActorError::None, true};
}
} // namespace blobber
