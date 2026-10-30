#include "game/combat.hpp"
#include "game/perception.hpp"
#include <algorithm>

namespace blobber {
const rules::WeaponPreview* CombatController::weapon(rules::CharacterId id, bool ranged) const noexcept {
    const auto* c = party_.character(id);
    if (!c)
        return nullptr;
    const auto i = static_cast<unsigned>(ranged ? EquipmentSlot::Ranged : EquipmentSlot::MainHand);
    const auto& w = c->stats.weapons[i];
    return w.available && w.ranged == ranged ? &w : nullptr;
}
CommandReason CombatController::validate(const PartyCommand& c, Player pose) const noexcept {
    const auto* ch = party_.character(c.character);
    if (!ch)
        return CommandReason::InvalidCharacter;
    if (static_cast<unsigned>(c.action) >= static_cast<unsigned>(PartyAction::Count))
        return CommandReason::InvalidAction;
    if (c.action == PartyAction::Spell || c.action == PartyAction::Feature)
        return validateMagic(c, pose);
    if (c.action == PartyAction::BackMelee &&
        !party_.hasFeature(c.character, rules::ClassFeature::SneakAttack))
        return CommandReason::FeatureRequired;
    if (c.action == PartyAction::Item)
        return validateItem(c, pose);
    if (c.world != state_.world || c.turn != state_.turn)
        return CommandReason::StaleTurn;
    if (!effects_.canAct(c.character))
        return CommandReason::CannotAct;
    const bool ranged = c.action == PartyAction::Ranged;
    if (realTime(state_.phase)) {
        if (!ranged && c.action != PartyAction::Aid)
            return CommandReason::NoEncounter;
        if (!normalAction(c.character))
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
    if (!normalAction(c.character))
        return CommandReason::Cooldown;
    if (c.action == PartyAction::Defend)
        return CommandReason::Ready;
    if (c.action == PartyAction::Aid) {
        const auto* ally = c.ally ? party_.character(*c.ally) : nullptr;
        return ally && c.ally->index != c.character.index &&
                       (ally->life == rules::Life::Dying || effects_.has(*c.ally, EffectKind::Sleep))
                   ? CommandReason::Ready
                   : CommandReason::InvalidAlly;
    }
    if (nextAttack_ == std::numeric_limits<std::uint64_t>::max() ||
        attackBusy(NpcWorld::maxInstances + c.character.index))
        return CommandReason::AttackPending;
    if (!ranged && !exposed(c.character) && !party_.hasFeature(c.character, rules::ClassFeature::SneakAttack))
        return CommandReason::RearBlocked;
    if (!weapon(c.character, ranged))
        return CommandReason::NoWeapon;
    if (ranged) {
        const auto* item = party_.equipped(c.character, EquipmentSlot::Ranged);
        if (item && rules::ammunitionFor(*item) != rules::Ammunition::None &&
            !party_.inventory().ammunition(c.character, rules::ammunitionFor(*item), party_.catalog()))
            return CommandReason::NoAmmunition;
    }
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
CombatOutcome CombatController::submit(const PartyCommand& c, Player pose, std::uint64_t now,
                                       CombatEvents& events) noexcept {
    currentEvents_ = &events;
    noticeTime_ = now;
    effects_.bindEvents(events, now);
    if (c.action == PartyAction::Spell || c.action == PartyAction::Feature)
        return submitMagic(c, pose, now, events);
    if (c.action == PartyAction::Item)
        return submitItem(c, pose, now, events);
    CombatOutcome out;
    out.reason = validate(c, pose);
    if (out.reason != CommandReason::Ready)
        return out;
    if (c.action == PartyAction::Ranged) {
        const auto* item = party_.equipped(c.character, EquipmentSlot::Ranged);
        const auto kind = rules::ammunitionFor(*item);
        if (kind != rules::Ammunition::None &&
            !party_.consumeItem(party_.inventory().ammunition(c.character, kind, party_.catalog()))) {
            out.reason = CommandReason::NoAmmunition;
            return out;
        }
    }
    out.reason = CommandReason::Resolved;
    out.character = c.character;
    const bool turn = state_.phase == CombatPhase::Turns;
    spendAction(c);
    if (c.action == PartyAction::Defend) {
        out.event = CombatEvent::Defend;
        state_.defending[c.character.index] = true;
        defendDue_[c.character.index] = state_.gameplayMs + combatRoundMs;
    } else if (c.action == PartyAction::Aid) {
        out.event = CombatEvent::Aid;
        out.recipient = *c.ally;
        const bool sleeping = effects_.has(out.recipient, EffectKind::Sleep);
        out.roll = sleeping ? 0 : rng_.d20().value.selected;
        out.hit = sleeping || out.roll + party_.character(c.character)->stats.modifiers[4] +
                                      effects_.oneShot(c.character, EffectKind::Guidance) >=
                                  10;
        if (sleeping)
            effects_.wake(out.recipient, now, events);
        if (out.hit)
            party_.stabilize(out.recipient);
    } else {
        effects_.breakWard(c.character, now, events);
        out.event = CombatEvent::Attack;
        out.enemy = *c.target;
        const bool ranged = c.action == PartyAction::Ranged;
        out.kind = ranged ? rules::AttackKind::Ranged : rules::AttackKind::Melee;
        const auto& w = *weapon(c.character, ranged);
        const auto* a = actors_.actor(out.enemy);
        const bool advantage = ranged && realTime(state_.phase) && !ai_.aware(out.enemy) &&
                               !npcSees(map_, a->npc.cell, a->npc.facing, pose.cell);
        const bool held = effects_.helpless(out.enemy);
        const auto mode =
            effects_.attackMode(c.character, out.enemy, combatAdjacent(map_, pose.cell, a->npc.cell),
                                advantage || effects_.guiding(out.enemy), ranged && adjacent(pose.cell));
        const auto roll = rng_.d20(mode).value;
        effects_.consumeGuiding(out.enemy, now, events);
        out.roll = roll.selected;
        const auto hit = rules::resolveAttack(roll, w.attackBonus + effects_.rollModifier(c.character),
                                              effects_.armorClass(out.enemy))
                             .value;
        out.hit = hit.outcome != rules::AttackOutcome::Miss;
        out.critical =
            hit.outcome == rules::AttackOutcome::Critical ||
            (out.roll == 19 && party_.progressionClass(c.character) == rules::ProgressionClass::Fighter &&
             party_.character(c.character)->build.level >= 3) ||
            (out.hit && held && combatAdjacent(map_, a->npc.cell, pose.cell));
        out.hit |= out.critical;
        if (out.hit) {
            out.damage = rng_.damage(w.damage, out.critical).value.total;
            if (sneakEligible(c.character, out.enemy, mode, ranged, pose)) {
                out.damage +=
                    rng_.damage({party_.character(c.character)->build.level >= 3 ? 2 : 1, 6, 0}, out.critical)
                        .value.total;
                sneakReady_[c.character.index] = state_.gameplayMs + combatRoundMs;
            }
        }
        out = queueAttack(out, pose, now);
    }
    refreshReadiness();
    if (turn) {
        if (out.event != CombatEvent::AttackStarted)
            completeAction(now);
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
    if (!a || !effects_.canAct(id) || !frontal(id) || attackBusy(id.slot) ||
        nextAttack_ == std::numeric_limits<std::uint64_t>::max())
        return out;
    const auto* attack = actors_.catalog().attack(ranged ? ranged_[id.slot] : melee_[id.slot]);
    if (!attack || (ranged && !effects_.sees(id)))
        return out;
    ai_.discardPreparation(id);
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
    enemyReady_[id.slot] = state_.gameplayMs + combatRoundMs;
    bool allowed = false;
    for (std::size_t attempt = 0; attempt < count; ++attempt) {
        const auto candidate = targets[(choice + attempt) % count];
        if (!currentEvents_ || effects_.wardAllows(id, candidate, now, *currentEvents_)) {
            out.recipient = candidate;
            allowed = true;
            break;
        }
    }
    if (!allowed) {
        out.event = CombatEvent::Spent;
        return out;
    }
    out.event = CombatEvent::Attack;
    out.kind = ranged ? rules::AttackKind::Ranged : rules::AttackKind::Melee;
    const auto& timing = presentation_[a->npc.type.index];
    out.impactMs = ranged ? timing.bowImpact : timing.impact;
    const auto roll =
        rng_.d20(effects_.attackMode(id, out.recipient, combatAdjacent(map_, a->npc.cell, origin_.cell),
                                     opening,
                                     state_.defending[out.recipient.index] ||
                                         (ranged && combatAdjacent(map_, a->npc.cell, origin_.cell))))
            .value;
    out.roll = roll.selected;
    const int attackBonus = attack->bonus + effects_.rollModifier(id);
    out.mirror = effects_.mirror(id, out.recipient, roll.selected, attackBonus);
    const auto hit = rules::resolveAttack(roll, attackBonus, effects_.armorClass(out.recipient)).value;
    out.hit = hit.outcome != rules::AttackOutcome::Miss;
    out.critical =
        hit.outcome == rules::AttackOutcome::Critical ||
        (out.hit && effects_.helpless(out.recipient) && combatAdjacent(map_, a->npc.cell, origin_.cell));
    actors_.face(id, headingToward(a->npc.cell, origin_.cell));
    actors_.attack(id, now, ranged);
    if (out.mirror.effect.serial) {
        out.hit = out.mirror.hit;
        out.critical = false;
    }
    if (out.hit && !out.mirror.effect.serial) {
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
    if (currentEvents_ && effects_.stand(id, now, *currentEvents_)) {
        out.event = CombatEvent::Spent;
        return out;
    }
    if (!effects_.canAct(id) || surprised_[id.slot] || enemyReady_[id.slot] > state_.gameplayMs) {
        surprised_[id.slot] = false;
        out.event = CombatEvent::Spent;
        return out;
    }
    if (currentEvents_ && effects_.escapeWeb(id, now, *currentEvents_)) {
        out.event = CombatEvent::Spent;
        return out;
    }
    const bool adjacent = combatAdjacent(map_, a->npc.cell, origin_.cell);
    const bool wounded =
        rules::hasTrait(actors_.catalog().monster(a->monster)->traits, rules::Trait::NimbleEscape) &&
        a->hitPoints * 2 <= a->maxHitPoints;
    const bool melee = actors_.catalog().attack(melee_[id.slot]);
    const bool ranged = actors_.catalog().attack(ranged_[id.slot]);
    const bool shot = ranged && effects_.sees(id) && inBowRange(a->npc.cell, origin_.cell) &&
                      lineOfSight(map_, a->npc.cell, origin_.cell);
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
        due_ = now + (actorWalking(*a, now) ? a->walkDurationMs : actionDelayMs);
        return out;
    }
    if (shot) {
        closeNext_[id.slot] = melee && !wounded;
        return enemyAttack(id, true, now);
    }
    if (currentEvents_)
        for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
            const auto ally = actors_.id(i);
            if (effects_.has(ally, EffectKind::Sleep) &&
                (a->npc.cell == actors_.actor(ally)->npc.cell ||
                 combatAdjacent(map_, a->npc.cell, actors_.actor(ally)->npc.cell))) {
                effects_.wake(ally, now, *currentEvents_);
                break;
            }
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
    out.died = party_.deathSave(id, out.roll, effects_.saveModifier(id)).died;
    saveDue_[id.index] = state_.gameplayMs + combatRoundMs;
    return out;
}
} // namespace blobber
