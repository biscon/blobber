#include "game/combat.hpp"
#include "game/perception.hpp"
#include <algorithm>
#include <stdexcept>

namespace blobber {
void CombatController::prepareVault() {
    std::array<bool, 6> seen{};
    for (unsigned i = 0; i < actors_.snapshot().count; ++i) {
        const auto& a = actors_.snapshot().actors[i];
        const auto part = a.npc.vault;
        if (part == VaultPart::None)
            continue;
        const auto index = unsigned(part);
        if (index >= seen.size() || seen[index] || a.npc.role != NpcRole::Hostile)
            throw std::invalid_argument("Invalid or duplicate vault placement");
        seen[index] = true;
        if (part == VaultPart::Mother)
            vaultMother_ = actors_.id(i);
        else if (a.npc.sac())
            clutches_[part == VaultPart::EastSac].sac = actors_.id(i);
        else {
            auto& clutch = clutches_[part == VaultPart::EastYoung];
            clutch.young = std::uint16_t(i);
            clutch.hatch = a.npc;
        }
    }
    const auto count = std::count(seen.begin(), seen.end(), true);
    if (count && count != 5)
        throw std::invalid_argument("Incomplete vault encounter");
    if (!count)
        return;
    for (const auto& c : clutches_) {
        const auto& sac = *actors_.actor(c.sac);
        if (cellDistance(c.hatch.cell, sac.npc.cell) != 1 || !map_.npcAllowed(c.hatch.cell))
            throw std::invalid_argument("Invalid vault hatch reservation");
    }
    if (clutches_[0].hatch.cell == clutches_[1].hatch.cell)
        throw std::invalid_argument("Vault hatches share a cell");
}

bool CombatController::stirring(ActorId id) const noexcept {
    const auto* a = actors_.actor(id);
    if (!a || !a->npc.sac() || a->life != ActorLife::Alive)
        return false;
    const auto* mother = actors_.actor(vaultMother_);
    return mother && mother->life == ActorLife::Alive &&
           clutches_[a->npc.vault == VaultPart::EastSac].warningRound != 0;
}

void CombatController::silenceVault() noexcept {
    if (!actors_.snapshot().actors[vaultMother_.slot].hasDeath)
        return;
    for (auto& c : clutches_) {
        c.warningRound = 0;
        if (const auto* sac = actors_.actor(c.sac); sac && sac->life == ActorLife::Alive)
            actors_.remove(c.sac);
    }
}

bool CombatController::vaultCleared() const noexcept {
    const auto& world = actors_.snapshot();
    if (!vaultMother_.world || !world.actors[vaultMother_.slot].hasDeath)
        return false;
    for (const auto& c : clutches_) {
        const auto& child = world.actors[c.young];
        if (child.generation && !child.hasDeath)
            return false;
    }
    return true;
}

void CombatController::vaultBoundary(std::uint64_t round, Player pose, CombatEvents& events) noexcept {
    const auto* mother = actors_.actor(vaultMother_);
    if (!mother || mother->life != ActorLife::Alive || !member(vaultMother_))
        return;
    for (unsigned side = 0; side < clutches_.size(); ++side) {
        auto& c = clutches_[side];
        const auto* sac = actors_.actor(c.sac);
        if (!sac || sac->life != ActorLife::Alive || round <= side)
            continue;
        const auto& old = actors_.snapshot().actors[c.young];
        if (old.life == ActorLife::Alive || (old.generation && !old.hasDeath))
            continue;
        CombatOutcome event;
        event.enemy = c.sac;
        event.occurredMs = noticeTime_;
        if (!c.warningRound) {
            c.warningRound = round;
            c.opportunity = false;
            event.event = CombatEvent::ClutchWarning;
            events.append(event);
        } else if (round > c.warningRound && c.opportunity && !attackBusy(c.young)) {
            const auto child = actors_.hatch(c.young, c.hatch, pose.cell);
            if (!child.world)
                continue;
            c.warningRound = 0;
            // Remove the preceding incarnation before the ordinary boundary enrollment.
            auto first = state_.enemies.begin();
            auto last = first + state_.enemyCount;
            state_.enemyCount =
                std::remove_if(first, last, [&](ActorId id) { return id.slot == c.young; }) - first;
            members_[c.young] = known_[c.young] = episodeActors_[c.young] = surprised_[c.young] = false;
            enemyReady_[c.young] = enemyBusy_[c.young] = knownUntil_[c.young] = 0;
            effects_.resetYoung(c.young);
            ai_.hatch(child, pose.cell);
            event.event = CombatEvent::ClutchHatch;
            events.append(event);
        }
    }
}
} // namespace blobber
