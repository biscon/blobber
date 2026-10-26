#include "game/goblin_ai.hpp"
#include <algorithm>
#include <cstdlib>
namespace blobber {
namespace {
int distance(Cell a, Cell b) noexcept {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
} // namespace
bool goblinSees(const Map& map, Cell from, Heading facing, Cell target) noexcept {
    if (map.solid(from) || map.solid(target) || static_cast<unsigned>(facing) >= 4)
        return false;
    const int dx = target.x - from.x, dy = target.y - from.y;
    const auto f = forward(facing);
    const int ahead = dx * f.x + dy * f.y, side = dx * f.y - dy * f.x;
    if (ahead <= 0 || std::abs(side) > ahead || dx * dx + dy * dy > 36)
        return false;
    // Integer supercover ray between cell centers. Exact corner crossings test
    // both neighboring cells, so two diagonal floors cannot see through a seam.
    const int nx = std::abs(dx), ny = std::abs(dy), sx = dx > 0 ? 1 : -1, sy = dy > 0 ? 1 : -1;
    int ix = 0, iy = 0;
    Cell c = from;
    while (ix < nx || iy < ny) {
        const int xCross = (1 + 2 * ix) * ny, yCross = (1 + 2 * iy) * nx;
        if (xCross == yCross) {
            if (map.solid({c.x + sx, c.y}) || map.solid({c.x, c.y + sy}))
                return false;
            c.x += sx;
            c.y += sy;
            ++ix;
            ++iy;
        } else if (xCross < yCross) {
            c.x += sx;
            ++ix;
        } else {
            c.y += sy;
            ++iy;
        }
        if (map.solid(c))
            return false;
    }
    return true;
}
GoblinAiController::GoblinAiController(const Map& map, ActorWorld& actors, std::uint32_t seed)
    : map_(map), actors_(actors), navigation_(map, actors), rng_(seed ^ 0x41493037u) {
    state_.count = actors.snapshot().count;
    for (std::size_t i = 0; i < state_.count; ++i) {
        const auto& npc = actors.snapshot().actors[i].npc;
        auto& a = state_.actors[i];
        a.home = npc.cell;
        a.homeFacing = npc.facing;
        a.config = npc.ai;
        a.moveDueMs = 1200;
    }
}
void GoblinAiController::face(std::size_t slot, Cell target) noexcept {
    const auto& npc = actors_.snapshot().actors[slot].npc;
    const int dx = target.x - npc.cell.x, dy = target.y - npc.cell.y;
    if (!dx && !dy)
        return;
    const auto facing = std::abs(dx) > std::abs(dy) ? (dx > 0 ? Heading::East : Heading::West)
                                                    : (dy > 0 ? Heading::South : Heading::North);
    actors_.face(actors_.id(slot), facing);
}
GoblinAiController::Goal GoblinAiController::think(std::size_t slot, Player party) noexcept {
    auto& a = state_.actors[slot];
    const auto& actor = actors_.snapshot().actors[slot];
    const auto here = actor.npc.cell;
    const auto now = state_.timeMs;
    if (actor.life != ActorLife::Alive)
        return {};
    if (goblinSees(map_, here, actor.npc.facing, party.cell)) {
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
    if (a.awareness == Awareness::Pursuing) {
        if (distance(here, party.cell) == 1) {
            face(slot, party.cell);
            return {};
        }
        return {true, true, a.lastSeen};
    }
    if (a.awareness == Awareness::Investigating) {
        if (!(here == a.lastSeen)) {
            // Last seen can still be occupied by the party even when our own
            // path turn put it outside the sight cone. Approach that remembered
            // cell, stop at its collision boundary, then look toward it.
            const bool occupiedByParty = a.lastSeen == party.cell;
            if (occupiedByParty && distance(here, a.lastSeen) == 1) {
                face(slot, a.lastSeen);
                return {};
            }
            return {true, occupiedByParty, a.lastSeen};
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
bool GoblinAiController::step(std::size_t slot, Cell destination, Cell party, std::uint64_t now) noexcept {
    const auto& actor = actors_.snapshot().actors[slot];
    if (actor.life != ActorLife::Alive || destination == actor.npc.cell)
        return false;
    const auto occupied = actors_.occupiedSlots(destination);
    auto subcell = actor.npc.subcell;
    if (occupied & (1u << (subcell.x + 2 * subcell.y))) {
        unsigned free = 0;
        while (free < 4 && (occupied & (1u << free)))
            ++free;
        if (free == 4)
            return false;
        subcell = {int(free % 2), int(free / 2)};
    }
    const auto old = actor.npc.cell;
    if (!actors_.walk(actors_.id(slot), destination, subcell, party, now).changed)
        return false;
    // Face along the committed step, never turn toward a blocked attempted move.
    const auto delta = Cell{destination.x - old.x, destination.y - old.y};
    const auto heading = delta.x ? (delta.x > 0 ? Heading::East : Heading::West)
                                 : (delta.y > 0 ? Heading::South : Heading::North);
    actors_.face(actors_.id(slot), heading);
    return true;
}
void GoblinAiController::update(std::uint64_t elapsedMs, std::uint64_t sessionTime, Player party) noexcept {
    state_.timeMs += elapsedMs;
    state_.searchWork = state_.completedSearches = 0;
    if (state_.timeMs < tickDue_)
        return;
    tickDue_ = state_.timeMs + 100;
    std::array<Goal, NpcWorld::maxInstances> goals{};
    for (std::size_t i = 0; i < state_.count; ++i) {
        goals[i] = think(i, party);
        if (!goals[i].active)
            navigation_.cancel(i);
        else if (state_.timeMs >= state_.actors[i].moveDueMs)
            navigation_.request(i, actors_.snapshot().actors[i].npc.cell, goals[i].target, goals[i].adjacent);
    }
    navigation_.update(party.cell);
    state_.searchWork = navigation_.lastWork();
    state_.completedSearches = navigation_.lastCompleted();
    for (std::size_t offset = 0; offset < state_.count; ++offset) {
        const auto i = (priority_ + offset) % state_.count;
        auto& a = state_.actors[i];
        const auto& actor = actors_.snapshot().actors[i];
        if (actor.life != ActorLife::Alive || state_.timeMs < a.moveDueMs)
            continue;
        if (goals[i].active) {
            const auto result = navigation_.take(i);
            if (!result.ready)
                continue;
            if (result.found && result.from == actor.npc.cell && result.target == goals[i].target &&
                result.adjacent == goals[i].adjacent)
                step(i, result.step, party.cell, sessionTime);
            a.moveDueMs =
                state_.timeMs +
                ((a.awareness == Awareness::Pursuing || a.awareness == Awareness::Investigating) ? 600
                                                                                                 : 1200);
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
            step(i, choices[static_cast<std::size_t>(choice)], party.cell, sessionTime);
            a.moveDueMs = state_.timeMs + 1200;
        }
    }
    if (state_.count)
        priority_ = (priority_ + 1) % state_.count;
}
} // namespace blobber
