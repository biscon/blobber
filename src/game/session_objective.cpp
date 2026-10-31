#include "game/perception.hpp"
#include "game/session.hpp"
#include <stdexcept>
namespace blobber {
void GameSession::prepareObjective() {
    const auto& d = objectiveDefinition_;
    if (d.count > d.members.size() || d.rewardCount > d.rewards.size() ||
        recovery_.groundCount + d.rewardCount > recovery_.ground.size())
        throw std::invalid_argument("Floor objective capacity exceeded");
    if (!d.count) {
        if (d.rewardCount)
            throw std::invalid_argument("Reward without guardians");
        for (unsigned i = 0; i < floor_.definition().count; ++i)
            if (floor_.definition().entries[i].kind == FixtureKind::GuardianGate)
                throw std::invalid_argument("Guardian gate without an objective");
        return;
    }
    const auto& defs = floor_.definition();
    if (d.approachDoor >= defs.count || d.gate >= defs.count ||
        defs.entries[d.approachDoor].kind != FixtureKind::Door ||
        defs.entries[d.gate].kind != FixtureKind::GuardianGate)
        throw std::invalid_argument("Invalid objective fixtures");
    for (unsigned i = 0; i < d.count; ++i) {
        const auto id = actors_.id(d.members[i]);
        if (!actors_.actor(id))
            throw std::invalid_argument("Invalid guardian identity");
        for (unsigned j = 0; j < i; ++j)
            if (d.members[j] == d.members[i])
                throw std::invalid_argument("Duplicate guardian");
        objective_.members[objective_.count++] = id;
    }
    for (unsigned i = 0; i < d.rewardCount; ++i) {
        const auto& r = d.rewards[i];
        const auto* item = party_.catalog().item(r.item);
        if (map_.solid(r.cell) || !item || !r.quantity || r.quantity > rules::stackLimit(*item) ||
            (defs.landingCount && defs.npcExcluded[std::size_t(r.cell.y * map_.width() + r.cell.x)]))
            throw std::invalid_argument("Invalid guardian reward");
        loot_.state_.stacks[NpcWorld::maxInstances * lootPerActor + recovery_.groundCount + i] = {
            r.cell, r.item, r.quantity, false};
    }
}
bool GameSession::guardian(ActorId id) const noexcept {
    for (unsigned i = 0; i < objective_.count; ++i)
        if (objective_.members[i].world == id.world && objective_.members[i].slot == id.slot)
            return true;
    return false;
}
void GameSession::reconcileObjective() noexcept {
    if (!objective_.count || objective_.completed)
        return;
    const auto& actors = actors_.snapshot();
    for (unsigned i = 0; i < objective_.count; ++i) {
        const auto id = objective_.members[i];
        if (id.world != actors.world || id.slot >= actors.count || !actors.actors[id.slot].hasDeath)
            return;
    }
    if (floor_.revision() == UINT64_MAX || floor_.geometryRevision() == UINT64_MAX ||
        loot_.state_.revision == UINT64_MAX) {
        objective_.capacityFailure = true;
        return;
    }
    if (!completeFixture(objectiveDefinition_.gate)) {
        objective_.capacityFailure = true;
        return;
    }
    for (unsigned i = 0; i < objectiveDefinition_.rewardCount; ++i)
        loot_.state_.stacks[NpcWorld::maxInstances * lootPerActor + recovery_.groundCount + i].active = true;
    ++loot_.state_.revision;
    objective_.completed = true;
    objective_.rewardIssued = true;
    objective_.capacityFailure = false;
    ++objective_.revision;
}
LandingSafety GameSession::landingSafety(unsigned index) const noexcept {
    const auto& defs = floor_.definition();
    if (index >= defs.landingCount)
        return {};
    const auto c = defs.landings[index];
    if (actionBusy())
        return {false, "Wait until movement finishes."};
    if (map_.solid(c) || actors_.occupied(c))
        return {false, "Landing is occupied."};
    if (effects_.area(c) || !combat_.settledRecovery({c, movement().player.heading}, timeMs_))
        return {false, "Landing is not settled and safe."};
    bool able = false;
    for (std::uint16_t i = 0; i < rules::partySize; ++i)
        able |= effects_.canAct(rules::CharacterId{i});
    if (!able)
        return {false, "An able companion is required."};
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto& a = actors_.snapshot().actors[i];
        if (a.life == ActorLife::Alive && (combatAdjacent(map_, a.npc.cell, c) ||
                                           (inBowRange(a.npc.cell, c) && lineOfSight(map_, a.npc.cell, c))))
            return {false, "Landing is threatened."};
    }
    return {true, "Landing is safe."};
}
} // namespace blobber
