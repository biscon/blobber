#include "game/approach_slots.hpp"
#include "game/perception.hpp"

namespace blobber {
std::optional<ApproachSlot> approachSlot(GeometryView map, const ActorWorld& actors, ActorId id, Player party,
                                         Cell destination, bool includeSelf) noexcept {
    const auto* actor = actors.actor(id);
    if (!actor || actor->life != ActorLife::Alive || destination == party.cell ||
        !map.npcAllowed(destination))
        return {};
    auto occupied = actors.occupiedSlots(destination);
    if (includeSelf && destination == actor->npc.cell)
        occupied &= ~(1u << (actor->npc.subcell.x + 2 * actor->npc.subcell.y));
    const auto f = forward(party.heading);
    auto near = actor->npc.subcell;
    if (f.x)
        near.x = f.x > 0 ? 0 : 1;
    else
        near.y = f.y > 0 ? 0 : 1;
    auto other = near;
    if (f.x)
        other.y = 1 - other.y;
    else
        other.x = 1 - other.x;
    for (const auto rank : {ApproachRank::VisibleNear, ApproachRank::VisibleRear, ApproachRank::Hidden})
        for (const auto candidate : {near, other, near + f, other + f}) {
            if (occupied & (1u << (candidate.x + 2 * candidate.y)))
                continue;
            const bool visible = inPartyViewport(party.cell, party.heading, destination, candidate);
            const bool nearRow = f.x ? candidate.x == near.x : candidate.y == near.y;
            const auto quality = !visible  ? ApproachRank::Hidden
                                 : nearRow ? ApproachRank::VisibleNear
                                           : ApproachRank::VisibleRear;
            if (quality == rank)
                return ApproachSlot{candidate, rank};
        }
    return {};
}
} // namespace blobber
