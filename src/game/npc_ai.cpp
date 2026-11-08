#include "game/npc_ai.hpp"
#include "game/approach_slots.hpp"
#include "game/effects.hpp"
#include "game/perception.hpp"
#include <algorithm>
#include <cstdlib>
namespace blobber {
namespace {
int distance(Cell a, Cell b) noexcept {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
} // namespace
NpcAiController::NpcAiController(GeometryView map, ActorWorld& actors, std::uint32_t seed)
    : map_(map), actors_(actors), navigation_(map, actors), tactical_(map, actors), rng_(seed ^ 0x41493037u) {
    state_.count = actors.snapshot().count;
    for (std::size_t i = 0; i < state_.count; ++i) {
        const auto& npc = actors.snapshot().actors[i].npc;
        auto& a = state_.actors[i];
        a.home = npc.cell;
        a.homeFacing = npc.facing;
        a.config = npc.ai;
        a.moveDueMs = 1200;
        if (!actors.snapshot().actors[i].hostile())
            continue;
        const auto& monster = *actors.catalog().monster(actors.snapshot().actors[i].monster);
        bool melee = false, ranged = false;
        for (std::size_t j = 0; j < monster.attackCount; ++j) {
            const auto kind = actors.catalog().attack(monster.attacks[j])->kind;
            melee |= kind == rules::AttackKind::Melee;
            ranged |= kind == rules::AttackKind::Ranged;
        }
        rangedOnly_[i] = ranged && !melee;
    }
}
void NpcAiController::face(std::size_t slot, Cell target) noexcept {
    const auto& npc = actors_.snapshot().actors[slot].npc;
    const int dx = target.x - npc.cell.x, dy = target.y - npc.cell.y;
    if (!dx && !dy)
        return;
    const auto facing = std::abs(dx) > std::abs(dy) ? (dx > 0 ? Heading::East : Heading::West)
                                                    : (dy > 0 ? Heading::South : Heading::North);
    actors_.face(actors_.id(slot), facing);
}
std::optional<Cell> NpcAiController::preferredFront(Player party) const noexcept {
    return party.cell + forward(party.heading);
}
NpcAiController::Goal NpcAiController::think(std::size_t slot, Player party) noexcept {
    auto& a = state_.actors[slot];
    const auto& actor = actors_.snapshot().actors[slot];
    const auto here = actor.npc.cell;
    const auto now = state_.timeMs;
    if (!actor.hostile() || actor.life != ActorLife::Alive)
        return {};
    if ((!effects_ || effects_->sees(actors_.id(slot))) &&
        npcSees(map_, here, actor.npc.facing, party.cell)) {
        if (a.awareness != Awareness::Pursuing)
            a.moveDueMs = std::min(a.moveDueMs, now + 600);
        a.awareness = Awareness::Pursuing;
        a.lastSeen = party.cell;
        a.lastSeenMs = now;
    } else if (a.awareness == Awareness::Pursuing)
        a.awareness = Awareness::Investigating;
    if ((a.awareness == Awareness::Investigating || a.awareness == Awareness::Searching) &&
        now - a.lastSeenMs >= 6000) {
        a.awareness = Awareness::Returning;
        a.moveDueMs = now + 1200;
    }
    // Only a usable forward-view shot stops pursuit. Hidden enemies still close;
    // investigation never acquires a new party position through a wall.
    if (rangedOnly_[slot] && (!effects_ || effects_->sees(actors_.id(slot))) &&
        (a.awareness == Awareness::Pursuing ||
         (a.awareness == Awareness::Investigating && a.lastSeen == party.cell)) &&
        inPartyView(map_, party.cell, party.heading, here)) {
        face(slot, party.cell);
        return {};
    }
    if (a.awareness == Awareness::Pursuing) {
        if (combatAdjacent(map_, here, party.cell)) {
            face(slot, party.cell);
            return {};
        }
        return {true, true, a.lastSeen, preferredFront(party)};
    }
    if (a.awareness == Awareness::Investigating) {
        if (!(here == a.lastSeen)) {
            // Last seen can still be occupied by the party even when our own
            // path turn put it outside the sight cone. Approach that remembered
            // cell, stop at its collision boundary, then look toward it.
            const bool occupiedByParty = a.lastSeen == party.cell;
            if (occupiedByParty && combatAdjacent(map_, here, a.lastSeen)) {
                face(slot, a.lastSeen);
                return {};
            }
            return {true, occupiedByParty, a.lastSeen,
                    occupiedByParty ? preferredFront(party) : std::nullopt};
        }
        a.awareness = Awareness::Searching;
        a.searchStartMs = now;
        a.searchTurns = 0;
    }
    if (a.awareness == Awareness::Searching) {
        const auto turns = std::min<std::uint64_t>(4, (now - a.searchStartMs) / 600);
        if (turns > a.searchTurns) {
            actors_.face(actors_.id(slot),
                         static_cast<Heading>((static_cast<unsigned>(actor.npc.facing) + 1) % 4));
            a.searchTurns = static_cast<std::uint8_t>(turns);
        }
        if (now - a.searchStartMs < 2400)
            return {};
        a.awareness = Awareness::Returning;
        a.moveDueMs = now + 1200;
    }
    if (a.awareness == Awareness::Returning) {
        const auto destination =
            a.config.routine == NpcRoutine::Patrol ? a.config.waypoints[a.waypoint] : a.home;
        if (!(here == destination))
            return {true, false, destination};
        a.awareness = Awareness::Routine;
        a.moveDueMs = now + 1200;
        if (a.config.routine == NpcRoutine::Guard)
            actors_.face(actors_.id(slot), a.homeFacing);
    }
    if (a.config.routine == NpcRoutine::Guard)
        return {};
    if (a.config.routine == NpcRoutine::Patrol) {
        if (here == a.config.waypoints[a.waypoint]) {
            a.waypoint = (a.waypoint + 1) % a.config.waypointCount;
            a.moveDueMs = now + 1200;
        }
        return {true, false, a.config.waypoints[a.waypoint]};
    }
    return {}; // Local wandering does not need a path search.
}
bool NpcAiController::step(std::size_t slot, Cell destination, Player party, std::uint64_t now,
                           bool approaching) noexcept {
    const auto& actor = actors_.snapshot().actors[slot];
    if (!actor.hostile() || actor.life != ActorLife::Alive || (!approaching && destination == actor.npc.cell))
        return false;
    const auto occupied = actors_.occupiedSlots(destination);
    auto subcell = actor.npc.subcell;
    if (approaching) {
        const auto choice = approachSlot(map_, actors_, actors_.id(slot), party, destination, true);
        if (!choice || (destination == actor.npc.cell && choice->subcell == actor.npc.subcell)) {
            prepared_[slot].reset();
            return false;
        }
        subcell = choice->subcell;
    } else if (occupied & (1u << (subcell.x + 2 * subcell.y))) {
        unsigned free = 0;
        while (free < 4 && (occupied & (1u << free)))
            ++free;
        if (free == 4) {
            prepared_[slot].reset();
            return false;
        }
        subcell = {int(free % 2), int(free / 2)};
    }
    const auto old = actor.npc.cell;
    const auto oldSubcell = actor.npc.subcell;
    if (!map_.npcAllowed(destination) || destination == party.cell) {
        prepared_[slot].reset();
        return false;
    }
    const auto factor = effects_ ? effects_->movementFactor(actors_.id(slot), old, destination) : 1;
    if (turnMovement_ && factor > 1 && (!prepared_[slot] || !(*prepared_[slot] == destination))) {
        prepared_[slot] = destination;
        return true;
    }
    prepared_[slot].reset();
    if (!actors_.walk(actors_.id(slot), destination, subcell, party.cell, now, npcWalkDurationMs * factor)
             .changed)
        return false;
    if (effects_)
        effects_->cross(actors_.id(slot), globalSubcell(old, oldSubcell), globalSubcell(destination, subcell),
                        now);
    // Face along the committed step, never turn toward a blocked attempted move.
    const auto delta = destination == old ? subcell + oldSubcell * -1 : destination + old * -1;
    const auto heading = delta.x ? (delta.x > 0 ? Heading::East : Heading::West)
                                 : (delta.y > 0 ? Heading::South : Heading::North);
    actors_.face(actors_.id(slot), heading);
    return true;
}
void NpcAiController::update(std::uint64_t elapsedMs, std::uint64_t sessionTime, Player party,
                             const std::array<bool, NpcWorld::maxInstances>* frozen,
                             const std::array<bool, NpcWorld::maxInstances>* incapacitated) noexcept {
    if (geometryRevision_ != map_.revision()) {
        geometryRevision_ = map_.revision();
        prepared_ = {};
        for (std::size_t i = 0; i < state_.count; ++i)
            navigation_.cancel(i);
    }

    state_.timeMs += elapsedMs;
    state_.searchWork = state_.completedSearches = 0;
    if (state_.timeMs < tickDue_)
        return;
    tickDue_ = state_.timeMs + 100;
    std::array<Goal, NpcWorld::maxInstances> goals{};
    for (std::size_t i = 0; i < state_.count; ++i) {
        if (incapacitated && (*incapacitated)[i]) {
            navigation_.cancel(i);
            continue; // Includes facing; ordinary attack/travel locks still perceive.
        }
        goals[i] = think(i, party);
        if ((frozen && (*frozen)[i]) || !goals[i].active)
            navigation_.cancel(i);
        else if (state_.timeMs >= state_.actors[i].moveDueMs)
            navigation_.request(i, actors_.snapshot().actors[i].npc.cell, goals[i].target, goals[i].adjacent,
                                goals[i].preferred);
    }
    navigation_.update(party.cell);
    state_.searchWork = navigation_.lastWork();
    state_.completedSearches = navigation_.lastCompleted();
    for (std::size_t offset = 0; offset < state_.count; ++offset) {
        const auto i = (priority_ + offset) % state_.count;
        auto& a = state_.actors[i];
        const auto& actor = actors_.snapshot().actors[i];
        if ((incapacitated && (*incapacitated)[i]) || (frozen && (*frozen)[i]) ||
            actorWalking(actor, sessionTime) || !actor.hostile() || actor.life != ActorLife::Alive ||
            state_.timeMs < a.moveDueMs)
            continue;
        if (goals[i].active) {
            const auto result = navigation_.take(i);
            if (!result.ready)
                continue;
            if (result.found && result.from == actor.npc.cell && result.target == goals[i].target &&
                result.adjacent == goals[i].adjacent && result.preferred == goals[i].preferred)
                step(i, result.step, party, sessionTime, goals[i].adjacent && goals[i].target == party.cell);
            a.moveDueMs =
                state_.timeMs +
                ((a.awareness == Awareness::Pursuing || a.awareness == Awareness::Investigating) ? 600
                                                                                                 : 1200) *
                    (actor.hasWalk && actor.walkTimeMs == sessionTime
                         ? actor.walkDurationMs / npcWalkDurationMs
                         : 1);
        } else if (a.awareness == Awareness::Routine && a.config.routine == NpcRoutine::Wander) {
            std::array<Cell, 5> choices{};
            std::size_t count = 1;
            choices[0] = actor.npc.cell;
            for (int d = 0; d < 4; ++d) {
                const auto c = actor.npc.cell + forward(static_cast<Heading>(d));
                if (distance(c, a.home) <= 3 && !(c == party.cell) && actors_.occupiedSlots(c) != 15)
                    choices[count++] = c;
            }
            const auto bound = static_cast<std::uint32_t>(count);
            const auto threshold = (0u - bound) % bound;
            std::uint32_t word;
            do {
                word = static_cast<std::uint32_t>(rng_());
            } while (word < threshold);
            const auto choice = word % bound;
            step(i, choices[static_cast<std::size_t>(choice)], party, sessionTime);
            a.moveDueMs = state_.timeMs + 1200 * (actor.hasWalk && actor.walkTimeMs == sessionTime
                                                      ? actor.walkDurationMs / npcWalkDurationMs
                                                      : 1);
        }
    }
    if (state_.count)
        priority_ = (priority_ + 1) % state_.count;
}
void NpcAiController::alert(ActorId id, Cell party) noexcept {
    const auto* actor = actors_.actor(id);
    if (!actor || !actor->hostile() || actor->life != ActorLife::Alive)
        return;
    auto& a = state_.actors[id.slot];
    a.awareness = Awareness::Pursuing;
    a.lastSeen = party;
    a.lastSeenMs = state_.timeMs;
    a.moveDueMs = std::min(a.moveDueMs, state_.timeMs + 600);
}
bool NpcAiController::aware(ActorId id) const noexcept {
    const auto* actor = actors_.actor(id);
    if (!actor || !actor->hostile() || actor->life != ActorLife::Alive)
        return false;
    const auto a = state_.actors[id.slot].awareness;
    return a == Awareness::Pursuing || a == Awareness::Investigating || a == Awareness::Searching;
}
bool NpcAiController::approach(ActorId id, Player party, std::uint64_t now) noexcept {
    return position(id, party, now) == PositioningResult::Moved;
}
TacticalNavigation::Result NpcAiController::combatStep(ActorId id, Player party, std::uint64_t now,
                                                       bool retreat) noexcept {
    if (tacticalTime_ != now) {
        tacticalTime_ = now;
        tacticalWork_ = 0;
    }
    const auto goal = effects_ && !effects_->sees(id) ? state_.actors[id.slot].lastSeen : party.cell;
    auto result = tactical_.next(id, party, retreat ? party.cell : goal, retreat, effects_, tacticalWork_);
    state_.searchWork = tacticalWork_;
    return result;
}
TacticalNavigation::Result NpcAiController::wakeStep(ActorId id, Player party, std::uint64_t now,
                                                     const TacticalNavigation::WakeGoals& goals) noexcept {
    if (tacticalTime_ != now) {
        tacticalTime_ = now;
        tacticalWork_ = 0;
    }
    auto result = tactical_.next(id, party, party.cell, false, effects_, tacticalWork_, &goals);
    state_.searchWork = tacticalWork_;
    return result;
}
bool NpcAiController::commitCombatStep(ActorId id, Player party, const TacticalNavigation::Result& step,
                                       std::uint64_t now) noexcept {
    const auto* actor = actors_.actor(id);
    if (!actor || actorWalking(*actor, now) || (effects_ && !effects_->mobile(id)) ||
        step.status != TacticalNavigation::Status::Step)
        return false;
    const auto old = actor->npc.cell;
    const auto oldTile = globalSubcell(old, actor->npc.subcell);
    const auto delta = step.cell * 2 + step.subcell + (old * 2 + actor->npc.subcell) * -1;
    if (std::abs(delta.x) + std::abs(delta.y) != 1 ||
        !actors_.walk(id, step.cell, step.subcell, party.cell, now, npcWalkDurationMs / 2).changed)
        return false;
    actors_.face(id, delta.x ? (delta.x > 0 ? Heading::East : Heading::West)
                             : (delta.y > 0 ? Heading::South : Heading::North));
    tactical_.committed();
    if (effects_)
        effects_->cross(id, oldTile, globalSubcell(step.cell, step.subcell), now);
    return true;
}
PositioningResult NpcAiController::position(ActorId id, Player party, std::uint64_t now) noexcept {
    const auto* actor = actors_.actor(id);
    if (!actor || !actor->hostile() || actor->life != ActorLife::Alive || actorWalking(*actor, now))
        return PositioningResult::Blocked;
    const auto goal = effects_ && !effects_->sees(id) ? state_.actors[id.slot].lastSeen : party.cell;
    const auto preferred = goal == party.cell ? preferredFront(party) : std::nullopt;
    navigation_.request(id.slot, actor->npc.cell, goal, true, preferred);
    navigation_.update(party.cell);
    const auto result = navigation_.take(id.slot);
    if (!result.ready)
        return PositioningResult::Pending;
    if (!result.found || !(result.from == actor->npc.cell) || !(result.target == goal) ||
        !(result.preferred == preferred))
        return PositioningResult::Blocked;
    if (result.step == actor->npc.cell) {
        const auto choice = approachSlot(map_, actors_, id, party, result.step, true);
        if (!choice || choice->subcell == actor->npc.subcell) {
            prepared_[id.slot].reset();
            return PositioningResult::AtGoal;
        }
    }
    return step(id.slot, result.step, party, now, goal == party.cell) ? PositioningResult::Moved
                                                                      : PositioningResult::Blocked;
}
bool NpcAiController::escape(ActorId id, Player party, std::uint64_t now) noexcept {
    const auto* actor = actors_.actor(id);
    if (!actor || !actor->hostile() || actor->life != ActorLife::Alive || actorWalking(*actor, now))
        return false;
    const auto away = static_cast<unsigned>(headingToward(party.cell, actor->npc.cell));
    for (const unsigned d : {away, (away + 3) % 4, (away + 1) % 4}) {
        const auto dest = actor->npc.cell + forward(static_cast<Heading>(d));
        const auto separation = [&](Cell cell) {
            return std::max(std::abs(cell.x - party.cell.x), std::abs(cell.y - party.cell.y));
        };
        if (separation(dest) > separation(actor->npc.cell) && step(id.slot, dest, party, now))
            return true;
    }
    return false;
}
} // namespace blobber
