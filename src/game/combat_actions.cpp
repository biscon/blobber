#include "game/combat.hpp"
#include "game/perception.hpp"
#include <algorithm>

namespace blobber {
const rules::WeaponPreview* CombatController::weapon(rules::CharacterId id, bool ranged) const noexcept {
    const auto* c = party_.character(id);
    if (!c)
        return nullptr;
    const auto i = (ranged ? c->build.equipment.ranged : c->build.equipment.mainHand).index;
    if (i >= c->build.itemCount)
        return nullptr;
    const auto& w = c->stats.weapons[i];
    return w.available && w.ranged == ranged ? &w : nullptr;
}
CommandReason CombatController::validate(const PartyCommand& c, Player pose) const noexcept {
    const auto* ch = party_.character(c.character);
    if (!ch)
        return CommandReason::InvalidCharacter;
    if (static_cast<unsigned>(c.action) >= static_cast<unsigned>(PartyAction::Count))
        return CommandReason::InvalidAction;
    if (c.action == PartyAction::BackMelee)
        return CommandReason::FeatureRequired;
    if (c.action == PartyAction::Spell || c.action == PartyAction::Item)
        return CommandReason::Unavailable;
    if (c.world != state_.world || c.turn != state_.turn)
        return CommandReason::StaleTurn;
    if (ch->life != rules::Life::Conscious)
        return CommandReason::CannotAct;
    const bool ranged = c.action == PartyAction::Ranged;
    if (realTime(state_.phase)) {
        if (!ranged)
            return CommandReason::NoEncounter;
        if (partyReady_[c.character.index] > state_.gameplayMs)
            return CommandReason::Cooldown;
    } else {
        if (state_.phase != CombatPhase::Turns)
            return CommandReason::NoEncounter;
        if (!state_.awaitingAction || !state_.activeCharacter ||
            state_.activeCharacter->index != c.character.index)
            return CommandReason::NotYourTurn;
        if (!(pose == origin_))
            return CommandReason::Moving;
    }
    if (c.action == PartyAction::Defend)
        return CommandReason::Ready;
    if (c.action == PartyAction::Aid) {
        const auto* ally = c.ally ? party_.character(*c.ally) : nullptr;
        return ally && c.ally->index != c.character.index && ally->life == rules::Life::Dying
                   ? CommandReason::Ready
                   : CommandReason::InvalidAlly;
    }
    if (nextAttack_ == std::numeric_limits<std::uint64_t>::max() ||
        attackBusy(NpcWorld::maxInstances + c.character.index))
        return CommandReason::AttackPending;
    if (!ranged && !exposed(c.character))
        return CommandReason::RearBlocked;
    if (!weapon(c.character, ranged))
        return CommandReason::NoWeapon;
    if (!c.target)
        return CommandReason::MissingTarget;
    const auto* a = actors_.actor(*c.target);
    if (!a || a->life != ActorLife::Alive)
        return CommandReason::InvalidTarget;
    if (ranged) {
        if (!inBowRange(pose.cell, a->npc.cell))
            return CommandReason::OutOfRange;
        if (!lineOfSight(map_, pose.cell, a->npc.cell))
            return CommandReason::NotVisible;
        // Rendering additionally verifies that the current body is visible.
        if (!inPartyView(map_, pose.cell, pose.heading, a->npc.cell))
            return CommandReason::NotVisible;
        return CommandReason::Ready;
    }
    if (!inMeleeFront(map_, pose.cell, pose.heading, a->npc.cell))
        return CommandReason::OutOfRange;
    return exposed(*c.target) ? CommandReason::Ready : CommandReason::FrontProtected;
}
void CombatController::alert(ActorId target, Player pose) noexcept {
    const auto* a = actors_.actor(target);
    if (!a)
        return;
    episode_ = true;
    if (realTime(state_.phase) && !members_[target.slot]) {
        members_[target.slot] = true;
        state_.enemies[state_.enemyCount++] = target;
    }
    episodeActors_[target.slot] = true;
    // Radius-three floor flood has at most 25 distinct cells; actors do not block sound.
    struct Node {
        Cell cell;
        int depth;
    };
    std::array<Node, 25> cells{};
    std::size_t head = 0, count = 1;
    cells[0] = {a->npc.cell, 0};
    while (head < count) {
        const auto n = cells[head++];
        if (n.depth == 3)
            continue;
        for (int d = 0; d < 4; ++d) {
            const auto c = n.cell + forward(static_cast<Heading>(d));
            if (map_.solid(c))
                continue;
            bool seen = false;
            for (std::size_t i = 0; i < count; ++i)
                seen |= cells[i].cell == c;
            if (!seen && count < cells.size())
                cells[count++] = {c, n.depth + 1};
        }
    }
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto& other = actors_.snapshot().actors[i];
        if (other.life != ActorLife::Alive)
            continue;
        for (std::size_t j = 0; j < count; ++j)
            if (other.npc.cell == cells[j].cell) {
                ai_.alert(actors_.id(i), pose.cell);
                break;
            }
    }
    known_[target.slot] = true;
    knownUntil_[target.slot] = state_.gameplayMs + combatRoundMs;
    if (realTime(state_.phase)) {
        enroll(pose, false);
        state_.phase = CombatPhase::Approach;
    }
}
CombatOutcome CombatController::submit(const PartyCommand& c, Player pose, std::uint64_t now) noexcept {
    CombatOutcome out;
    out.reason = validate(c, pose);
    if (out.reason != CommandReason::Ready)
        return out;
    out.reason = CommandReason::Resolved;
    out.character = c.character;
    const bool turn = state_.phase == CombatPhase::Turns;
    partyReady_[c.character.index] = state_.gameplayMs + combatRoundMs;
    state_.spent[c.character.index] = true;
    if (c.action == PartyAction::Defend) {
        out.event = CombatEvent::Defend;
        state_.defending[c.character.index] = true;
        defendDue_[c.character.index] = state_.gameplayMs + combatRoundMs;
    } else if (c.action == PartyAction::Aid) {
        out.event = CombatEvent::Aid;
        out.recipient = *c.ally;
        out.roll = rng_.d20().value.selected;
        out.hit = out.roll + party_.character(c.character)->stats.modifiers[4] >= 10;
        if (out.hit)
            party_.stabilize(out.recipient);
    } else {
        out.event = CombatEvent::Attack;
        out.enemy = *c.target;
        const bool ranged = c.action == PartyAction::Ranged;
        out.kind = ranged ? rules::AttackKind::Ranged : rules::AttackKind::Melee;
        const auto& w = *weapon(c.character, ranged);
        const auto* a = actors_.actor(out.enemy);
        const bool advantage = ranged && realTime(state_.phase) && !ai_.aware(out.enemy) &&
                               !npcSees(map_, a->npc.cell, a->npc.facing, pose.cell);
        const auto roll = rng_.d20(rules::rollMode(advantage, ranged && adjacent(pose.cell))).value;
        out.roll = roll.selected;
        const auto hit =
            rules::resolveAttack(roll, w.attackBonus, actors_.catalog().monster(a->monster)->armorClass)
                .value;
        out.hit = hit.outcome != rules::AttackOutcome::Miss;
        out.critical = hit.outcome == rules::AttackOutcome::Critical;
        if (out.hit) {
            out.damage = rng_.damage(w.damage, out.critical).value.total;
        }
        alert(out.enemy, pose);
        out = queueAttack(out, pose, now);
    }
    refreshReadiness();
    if (turn) {
        if (out.event != CombatEvent::AttackStarted)
            advance(now);
    } else
        ++state_.turn;
    out.phase = state_.phase;
    return out;
}
CombatOutcome CombatController::enemyAttack(ActorId id, bool ranged, std::uint64_t now,
                                            bool opening) noexcept {
    CombatOutcome out;
    out.enemy = id;
    out.enemyAttack = true;
    const auto* a = actors_.actor(id);
    if (!a || !frontal(id) || attackBusy(id.slot) || nextAttack_ == std::numeric_limits<std::uint64_t>::max())
        return out;
    const auto* attack = actors_.catalog().attack(ranged ? ranged_[id.slot] : melee_[id.slot]);
    if (!attack)
        return out;
    std::array<rules::CharacterId, 4> targets{};
    std::size_t count = 0;
    for (const auto target : party_.snapshot().formation)
        if (party_.character(target)->life == rules::Life::Conscious && (ranged || partyExposed(target)))
            targets[count++] = target;
    if (!count)
        return out;
    unsigned choice = 0;
    if (count > 1) {
        do {
            choice = static_cast<unsigned>(rng_.roll({1, 4, 0}).value.total - 1);
        } while (choice >= count && count != 2);
        if (count == 2)
            choice %= 2;
    }
    out.recipient = targets[choice];
    out.event = CombatEvent::Attack;
    out.kind = ranged ? rules::AttackKind::Ranged : rules::AttackKind::Melee;
    const auto& timing = presentation_[a->npc.type.index];
    out.impactMs = ranged ? timing.bowImpact : timing.impact;
    const auto roll =
        rng_.d20(rules::rollMode(opening, state_.defending[out.recipient.index] ||
                                              (ranged && combatAdjacent(map_, a->npc.cell, origin_.cell))))
            .value;
    out.roll = roll.selected;
    const auto hit =
        rules::resolveAttack(roll, attack->bonus, party_.character(out.recipient)->stats.armorClass).value;
    out.hit = hit.outcome != rules::AttackOutcome::Miss;
    out.critical = hit.outcome == rules::AttackOutcome::Critical;
    actors_.face(id, headingToward(a->npc.cell, origin_.cell));
    actors_.attack(id, now, ranged);
    if (out.hit) {
        out.damage = rng_.damage(attack->damage, out.critical).value.total;
    }
    enemyReady_[id.slot] = state_.gameplayMs + combatRoundMs;
    alert(id, origin_);
    return queueAttack(out, origin_, now);
}
CombatOutcome CombatController::enemyTurn(ActorId id, std::uint64_t now) noexcept {
    CombatOutcome out;
    out.enemy = id;
    const auto* a = actors_.actor(id);
    if (!frontal(id))
        return out;
    if (surprised_[id.slot] || enemyReady_[id.slot] > state_.gameplayMs) {
        surprised_[id.slot] = false;
        out.event = CombatEvent::Spent;
        return out;
    }
    const bool adjacent = combatAdjacent(map_, a->npc.cell, origin_.cell);
    const bool wounded =
        rules::hasTrait(actors_.catalog().monster(a->monster)->traits, rules::Trait::NimbleEscape) &&
        a->hitPoints * 2 <= a->maxHitPoints;
    const bool melee = actors_.catalog().attack(melee_[id.slot]);
    const bool ranged = actors_.catalog().attack(ranged_[id.slot]);
    const bool shot =
        ranged && inBowRange(a->npc.cell, origin_.cell) && lineOfSight(map_, a->npc.cell, origin_.cell);
    bool moved = false;
    if (adjacent && ranged && (!melee || wounded))
        moved = ai_.escape(id, origin_, now);
    if (!moved && adjacent && melee) {
        const auto slot = meleeSlot(id, actors_.occupiedSlots(a->npc.cell));
        if (slot && (a->npc.subcell == *slot || actors_.move(id, a->npc.cell, *slot, origin_.cell)))
            return enemyAttack(id, false, now);
    }
    if (!moved && !adjacent && (!shot || (melee && !wounded && closeNext_[id.slot]))) {
        moved = ai_.approach(id, origin_, now);
        closeNext_[id.slot] = false;
    }
    if (moved) {
        out.event = CombatEvent::Move;
        enemyReady_[id.slot] = state_.gameplayMs + combatRoundMs;
        state_.resolvingAction = true;
        state_.awaitingAction = false;
        due_ = now + npcWalkDurationMs;
        return out;
    }
    if (shot) {
        closeNext_[id.slot] = melee && !wounded;
        return enemyAttack(id, true, now);
    }
    out.event = CombatEvent::Spent;
    return out;
}
CombatOutcome CombatController::deathSave(rules::CharacterId id, std::uint64_t) noexcept {
    CombatOutcome out;
    out.character = id;
    if (party_.character(id)->life != rules::Life::Dying ||
        (saveDue_[id.index] && saveDue_[id.index] > state_.gameplayMs))
        return out;
    out.event = CombatEvent::DeathSave;
    out.roll = rng_.d20().value.selected;
    out.died = party_.deathSave(id, out.roll).died;
    saveDue_[id.index] = state_.gameplayMs + combatRoundMs;
    return out;
}
} // namespace blobber
