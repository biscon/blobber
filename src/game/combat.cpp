#include "game/combat.hpp"
#include "game/perception.hpp"
#include <algorithm>
#include <stdexcept>

namespace blobber {
CombatController::CombatController(const Map& map, Party& party, ActorWorld& actors, rules::DiceRng& rng,
                                   NpcAiController& ai, EffectController& effects,
                                   const std::vector<NpcTimings>& timings)
    : map_(map), party_(party), actors_(actors), rng_(rng), ai_(ai), effects_(effects) {
    if (&party.catalog() != &actors.catalog())
        throw std::invalid_argument("Combat catalog mismatch");
    if (!timings.empty() && timings.size() != actors.snapshot().typeCount)
        throw std::invalid_argument("Combat animation/world type counts differ");
    for (std::size_t i = 0; i < timings.size(); ++i) {
        validateNpcTimings(timings[i]);
        const auto& t = timings[i];
        const auto& a = t[static_cast<std::size_t>(NpcAnimation::Attack)];
        const auto& b = t[static_cast<std::size_t>(NpcAnimation::RangedAttack)];
        presentation_[i] = {npcClipDuration(a),
                            npcAttackImpact(a),
                            npcClipDuration(t[static_cast<std::size_t>(NpcAnimation::Hurt)]),
                            npcClipDuration(t[static_cast<std::size_t>(NpcAnimation::Death)]),
                            npcClipDuration(b),
                            npcAttackImpact(b)};
    }
    state_.world = actors.snapshot().world;
    visuals_.world = state_.world;
    for (std::size_t i = 0; i < actors.snapshot().count; ++i) {
        const auto& m = *actors.catalog().monster(actors.snapshot().actors[i].monster);
        for (std::size_t j = 0; j < m.attackCount; ++j) {
            auto& slot = actors.catalog().attack(m.attacks[j])->kind == rules::AttackKind::Melee ? melee_[i]
                                                                                                 : ranged_[i];
            if (!actors.catalog().attack(slot))
                slot = m.attacks[j];
        }
    }
}
bool CombatController::member(ActorId id) const noexcept {
    return actors_.actor(id) && id.slot < members_.size() && members_[id.slot];
}
bool CombatController::adjacent(Cell party) const noexcept {
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x) {
            const auto cell = party + Cell{x, y};
            if (actors_.occupied(cell) && combatAdjacent(map_, party, cell))
                return true;
        }
    return false;
}
bool CombatController::occupiedFront(Player pose) const noexcept {
    for (int side = -1; side <= 1; ++side) {
        const auto cell = pose.cell + forward(pose.heading) + right(pose.heading) * side;
        if (actors_.occupied(cell) && inMeleeFront(map_, pose.cell, pose.heading, cell))
            return true;
    }
    return false;
}
bool CombatController::near(const ActorState& a, Heading approach) const noexcept {
    switch (approach) {
    case Heading::North:
        return a.npc.subcell.y == 1;
    case Heading::East:
        return a.npc.subcell.x == 0;
    case Heading::South:
        return a.npc.subcell.y == 0;
    case Heading::West:
        return a.npc.subcell.x == 1;
    }
    return false;
}
bool CombatController::partyExposed(rules::CharacterId id) const noexcept {
    const auto& f = party_.snapshot().formation;
    for (std::size_t i = 0; i < f.size(); ++i)
        if (f[i].index == id.index)
            return i < 2 || !effects_.canAct(f[i - 2]);
    return false;
}
bool CombatController::exposed(rules::CharacterId id) const noexcept {
    return partyExposed(id);
}
bool CombatController::frontal(ActorId id) const noexcept {
    const auto* a = actors_.actor(id);
    return a && a->life == ActorLife::Alive && inPartyView(map_, origin_.cell, origin_.heading, a->npc.cell);
}
bool CombatController::presentationBusy(std::uint64_t now) const noexcept {
    if (pendingImpacts() || now < partyBusy_)
        return true;
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto& a = actors_.snapshot().actors[i];
        if (now < enemyBusy_[i] || actorWalking(a, now))
            return true;
    }
    return false;
}
void CombatController::approach() noexcept {
    state_.phase = CombatPhase::Approach;
    state_.facing.reset();
    state_.awaitingAction = false;
    state_.activeCharacter.reset();
    state_.activeEnemy.reset();
    party_.lockFormation(false);
    ++state_.turn;
}
bool CombatController::settleFront(std::uint64_t now) noexcept {
    if (occupiedFront(origin_))
        return false;
    if (presentationBusy(now))
        return true;
    for (unsigned offset : {1u, 3u, 2u}) {
        const auto heading = static_cast<Heading>((static_cast<unsigned>(origin_.heading) + offset) % 4);
        if (!occupiedFront({origin_.cell, heading}))
            continue;
        resumePhase_ = state_.phase;
        resumeAwaiting_ = state_.awaitingAction;
        state_.phase = CombatPhase::Facing;
        state_.facing = heading;
        state_.awaitingAction = false;
        ++state_.turn;
        return true;
    }
    approach();
    return true;
}
bool CombatController::exposed(ActorId id) const noexcept {
    const auto* a = actors_.actor(id);
    if (!a || a->life != ActorLife::Alive)
        return false;
    const auto approach = origin_.heading;
    if (near(*a, approach))
        return true;
    const auto front = a->npc.subcell + forward(approach) * -1;
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto& other = actors_.snapshot().actors[i];
        if (other.life == ActorLife::Alive && other.npc.cell == a->npc.cell && other.npc.subcell == front)
            return !effects_.canAct(actors_.id(i));
    }
    return true;
}
std::optional<Cell> CombatController::meleeSlot(ActorId id, std::uint8_t occupied) const noexcept {
    const auto* a = actors_.actor(id);
    if (!a || a->life != ActorLife::Alive || !inMeleeFront(map_, origin_.cell, origin_.heading, a->npc.cell))
        return {};
    const auto approach = origin_.heading;
    if (near(*a, approach))
        return a->npc.subcell;
    // Incapacitated front bodies occupy their slots but do not protect the rear.
    const auto paired = a->npc.subcell + forward(approach) * -1;
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto& other = actors_.snapshot().actors[i];
        if (other.life == ActorLife::Alive && other.npc.cell == a->npc.cell && other.npc.subcell == paired &&
            !effects_.canAct(actors_.id(i)))
            return a->npc.subcell;
    }
    auto slot = paired;
    for (int i = 0; i < 2; ++i) {
        if (!(occupied & (1u << (slot.x + 2 * slot.y))))
            return slot;
        if (approach == Heading::North || approach == Heading::South)
            slot.x = 1 - slot.x;
        else
            slot.y = 1 - slot.y;
    }
    return {};
}
void CombatController::refreshReadiness() noexcept {
    for (std::size_t i = 0; i < rules::partySize; ++i) {
        state_.cooldownMs[i] = partyReady_[i] > state_.gameplayMs ? partyReady_[i] - state_.gameplayMs : 0;
        if (state_.defending[i] && state_.gameplayMs >= defendDue_[i])
            state_.defending[i] = false;
    }
}
void CombatController::elapse(std::uint64_t elapsed) noexcept {
    if (!realTime(state_.phase))
        return;
    state_.gameplayMs += elapsed;
    for (std::size_t i = 0; i < rules::partySize; ++i)
        if (state_.defending[i] && state_.gameplayMs >= defendDue_[i])
            state_.defending[i] = false;
    refreshReadiness();
}
void CombatController::observe(Player pose) noexcept {
    origin_ = pose;
    state_.knownDirections = 0;
    state_.pendingCount = 0;
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto& a = actors_.snapshot().actors[i];
        if (a.life != ActorLife::Alive)
            continue;
        const bool sees = inPartyView(map_, pose.cell, pose.heading, a.npc.cell);
        if (sees) {
            known_[i] = true;
            knownUntil_[i] = state_.gameplayMs + combatRoundMs;
        } else if (realTime(state_.phase) && state_.gameplayMs >= knownUntil_[i])
            known_[i] = false;
        if (known_[i])
            state_.knownDirections |= 1u << static_cast<unsigned>(headingToward(pose.cell, a.npc.cell));
        if (known_[i] && !members_[i] &&
            (ai_.aware(actors_.id(i)) || combatAdjacent(map_, a.npc.cell, pose.cell)))
            ++state_.pendingCount;
    }
}
std::array<bool, NpcWorld::maxInstances> CombatController::movementLocks(std::uint64_t now) const noexcept {
    std::array<bool, NpcWorld::maxInstances> result{};
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto& a = actors_.snapshot().actors[i];
        const bool wounded =
            rules::hasTrait(actors_.catalog().monster(a.monster)->traits, rules::Trait::NimbleEscape) &&
            a.hitPoints * 2 <= a.maxHitPoints && ai_.aware(actors_.id(i)) &&
            actors_.catalog().attack(melee_[i]) && actors_.catalog().attack(ranged_[i]);
        result[i] = !effects_.canAct(actors_.id(i)) || now < enemyBusy_[i] ||
                    (!realTime(state_.phase) && members_[i]) || wounded;
    }
    return result;
}
void CombatController::enroll(Player pose, bool opening) noexcept {
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto& a = actors_.snapshot().actors[i];
        if (a.life != ActorLife::Alive || members_[i] ||
            (!ai_.aware(actors_.id(i)) && !combatAdjacent(map_, a.npc.cell, pose.cell)))
            continue;
        if (state_.enemyCount >= state_.enemies.size())
            return; // capacity equals all possible actors
        members_[i] = true;
        state_.enemies[state_.enemyCount++] = actors_.id(i);
        surprised_[i] = opening && !episodeActors_[i] && !ai_.aware(actors_.id(i)) &&
                        !npcSees(map_, a.npc.cell, a.npc.facing, pose.cell);
        episodeActors_[i] = true;
    }
}
bool CombatController::contact(Player pose) noexcept {
    if (!realTime(state_.phase) || !adjacent(pose.cell))
        return false;
    origin_ = pose;
    enroll(pose, true);
    episode_ = true;
    for (std::size_t i = 0; i < state_.enemyCount; ++i) {
        const auto id = state_.enemies[i];
        ai_.alert(id, pose.cell);
        known_[id.slot] = true;
    }
    state_.phase = CombatPhase::Contact;
    ++state_.turn;
    party_.lockFormation(true);
    return true;
}
void CombatController::addGroups() noexcept {
    for (std::size_t i = 0; i < state_.enemyCount; ++i) {
        const auto* a = actors_.actor(state_.enemies[i]);
        if (!a || a->life != ActorLife::Alive)
            continue;
        bool found = false;
        for (std::size_t j = 0; j < groupCount_; ++j)
            found |= groups_[j].index == a->monster.index;
        if (found)
            continue;
        const auto g = groupCount_++;
        groups_[g] = a->monster;
        const auto& m = *actors_.catalog().monster(a->monster);
        const int dex = m.abilities[1];
        order_[orderCount_++] = {false, static_cast<std::uint16_t>(g),
                                 rng_.d20().value.selected + rules::abilityModifier(dex).value, dex};
    }
    std::sort(order_.begin(), order_.begin() + orderCount_, [](const Entry& a, const Entry& b) {
        if (a.initiative != b.initiative)
            return a.initiative > b.initiative;
        if (a.dexterity != b.dexterity)
            return a.dexterity > b.dexterity;
        if (a.party != b.party)
            return a.party;
        return a.index < b.index;
    });
}
void CombatController::begin(std::uint64_t now) noexcept {
    for (std::size_t i = 0; i < state_.enemyCount; ++i) {
        const auto* a = actors_.actor(state_.enemies[i]);
        if (a && effects_.canAct(state_.enemies[i]))
            actors_.face(state_.enemies[i], headingToward(a->npc.cell, origin_.cell));
    }
    if (!orderCount_) {
        for (std::uint16_t i = 0; i < rules::partySize; ++i) {
            const auto& c = *party_.character({i});
            order_[orderCount_++] = {true, i, rng_.d20().value.selected + c.stats.initiative,
                                     c.stats.abilities[1]};
        }
    }
    addGroups();
    ++state_.round;
    position_ = enemyCursor_ = 0;
    state_.spent.fill(false);
    due_ = now + actionDelayMs;
    state_.phase = CombatPhase::Turns;
    state_.awaitingAction = false;
    state_.resolvingAction = false;
    ++state_.turn;
    selectTurn();
}
void CombatController::selectTurn() noexcept {
    state_.awaitingAction = false;
    state_.activeCharacter.reset();
    state_.activeEnemy.reset();
    while (position_ < orderCount_) {
        const auto& e = order_[position_];
        if (e.party) {
            if (partyReady_[e.index] <= state_.gameplayMs)
                state_.defending[e.index] = false;
            const auto life = party_.character({e.index})->life;
            if (life == rules::Life::Conscious || life == rules::Life::Dying) {
                state_.activeCharacter = rules::CharacterId{e.index};
                state_.spent[e.index] = state_.spent[e.index] || partyReady_[e.index] > state_.gameplayMs;
                state_.awaitingAction =
                    effects_.canAct(rules::CharacterId{e.index}) && !state_.spent[e.index];
                ++state_.turn;
                return;
            }
        } else {
            for (; enemyCursor_ < state_.enemyCount; ++enemyCursor_) {
                const auto id = state_.enemies[enemyCursor_];
                const auto* a = actors_.actor(id);
                if (frontal(id) && a->monster.index == groups_[e.index].index &&
                    (actors_.catalog().attack(ranged_[id.slot]) ||
                     !combatAdjacent(map_, a->npc.cell, origin_.cell) ||
                     meleeSlot(id, actors_.occupiedSlots(a->npc.cell)))) {
                    state_.activeEnemy = id;
                    ++state_.turn;
                    return;
                }
            }
        }
        enemyCursor_ = 0;
        ++position_;
    }
    state_.phase = CombatPhase::RoundEnd;
    ++state_.turn;
}
void CombatController::advance(std::uint64_t now) noexcept {
    due_ = now + actionDelayMs;
    if (finish(now))
        return;
    if (state_.activeEnemy)
        ++enemyCursor_;
    else
        ++position_;
    state_.activeCharacter.reset();
    state_.activeEnemy.reset();
    state_.awaitingAction = false;
    if (!settleFront(now))
        selectTurn();
}
void CombatController::leave() noexcept {
    state_.phase = CombatPhase::Exploration;
    state_.enemyCount = 0;
    members_.fill(false);
    state_.awaitingAction = state_.resolvingAction = false;
    state_.activeCharacter.reset();
    state_.activeEnemy.reset();
    state_.facing.reset();
    state_.spent.fill(false);
    episode_ = false;
    episodeActors_.fill(false);
    orderCount_ = groupCount_ = 0;
    state_.round = 0;
    party_.lockFormation(false);
    ++state_.turn;
}
bool CombatController::finish(std::uint64_t now) noexcept {
    if (pendingImpacts())
        return false;
    bool conscious = false, living = false, threat = false;
    for (const auto& c : party_.snapshot().characters)
        conscious |= c.life == rules::Life::Conscious;
    // Pending arrivals still prevent victory; initiative membership changes only
    // at the boundary, even if the final current participant dies first.
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto id = actors_.id(i);
        const auto* a = actors_.actor(id);
        if (!a || a->life != ActorLife::Alive)
            continue;
        const bool active = ai_.aware(id) || combatAdjacent(map_, a->npc.cell, origin_.cell);
        if (members_[i] || active) {
            living = true;
            threat |= active;
        }
    }
    if (conscious && living) {
        if (!threat && realTime(state_.phase))
            leave();
        return false;
    }
    if (!episode_ || !state_.enemyCount)
        return false;
    std::uint64_t busy = partyBusy_;
    for (const auto t : enemyBusy_)
        busy = std::max(busy, t);
    if (now < busy)
        return false;
    state_.phase = conscious ? CombatPhase::Victory : CombatPhase::Defeat;
    state_.awaitingAction = false;
    state_.resolvingAction = false;
    state_.activeCharacter.reset();
    state_.activeEnemy.reset();
    state_.defending.fill(false);
    ++state_.turn;
    if (conscious)
        for (std::uint16_t i = 0; i < rules::partySize; ++i)
            if (party_.character({i})->life == rules::Life::Dying)
                party_.stabilize({i});
    return true;
}
bool CombatController::continueExploring() noexcept {
    if (state_.phase != CombatPhase::Victory)
        return false;
    leave();
    return true;
}
void CombatController::previewTurns(CombatSnapshot& dst) const noexcept {
    dst.upcomingCount = 0;
    if (state_.phase != CombatPhase::Turns)
        return;
    std::array<std::uint8_t, NpcWorld::maxInstances> occupied{};
    for (std::size_t i = 0; i < state_.enemyCount; ++i) {
        const auto* a = actors_.actor(state_.enemies[i]);
        if (a)
            occupied[i] = actors_.occupiedSlots(a->npc.cell);
    }
    for (std::size_t p = position_; p < orderCount_ && dst.upcomingCount < dst.upcoming.size(); ++p) {
        const auto e = order_[p];
        if (e.party) {
            const auto life = party_.character({e.index})->life;
            if (life == rules::Life::Conscious || life == rules::Life::Dying)
                dst.upcoming[dst.upcomingCount++] = {
                    rules::CharacterId{e.index}, {}, life == rules::Life::Dying};
        } else
            for (std::size_t i = p == position_ ? enemyCursor_ : 0;
                 i < state_.enemyCount && dst.upcomingCount < dst.upcoming.size(); ++i) {
                const auto id = state_.enemies[i];
                const auto* a = actors_.actor(id);
                if (!frontal(id) || a->monster.index != groups_[e.index].index)
                    continue;
                const auto slot = meleeSlot(id, occupied[i]);
                if (!slot && !actors_.catalog().attack(ranged_[id.slot]) &&
                    combatAdjacent(map_, a->npc.cell, origin_.cell))
                    continue;
                dst.upcoming[dst.upcomingCount++] = {{}, id, false};
                if (slot)
                    for (std::size_t j = 0; j < state_.enemyCount; ++j) {
                        const auto* other = actors_.actor(state_.enemies[j]);
                        if (other && other->npc.cell == a->npc.cell)
                            occupied[j] |= 1u << (slot->x + 2 * slot->y);
                    }
            }
    }
}
} // namespace blobber
