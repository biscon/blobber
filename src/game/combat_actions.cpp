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
    if (!a || !a->hostile() || a->life != ActorLife::Alive)
        return CommandReason::InvalidTarget;
    if (ranged) {
        if (!inBowRange(pose.cell, a->npc.cell))
            return CommandReason::OutOfRange;
        if (!lineOfSight(map_, pose.cell, a->npc.cell))
            return CommandReason::NotVisible;
        // Rendering additionally verifies exposure to dungeon geometry, ignoring actor overlap.
        if (!inPartyView(map_, pose.cell, pose.heading, a->npc.cell, a->npc.subcell))
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
    ai_.alertNearby(target, pose.cell);
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
        defendDue_[c.character.index] = actionReadyMs();
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
        out.damageType = w.damageType;
        const auto* a = actors_.actor(out.enemy);
        const bool advantage = ranged && realTime(state_.phase) && !ai_.aware(out.enemy) &&
                               !npcSees(map_, a->npc.cell, a->npc.facing, pose.cell);
        const bool held = effects_.helpless(out.enemy);
        const auto mode = effects_.attackMode(
            c.character, out.enemy, combatAdjacent(map_, pose.cell, a->npc.cell),
            advantage || effects_.guiding(out.enemy), w.heavyDisadvantage || (ranged && adjacent(pose.cell)));
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
                sneakReady_[c.character.index] = actionReadyMs();
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
bool CombatController::packSupported(ActorId id, Cell partyCell) const noexcept {
    const auto* attacker = actors_.actor(id);
    if (!attacker || !attacker->hostile() || attacker->life != ActorLife::Alive ||
        !rules::hasTrait(actors_.catalog().monster(attacker->monster)->traits, rules::Trait::PackTactics))
        return false;
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto ally = actors_.id(i);
        if (i != id.slot && effects_.conscious(ally) &&
            combatAdjacent(map_, actors_.actor(ally)->npc.cell, partyCell))
            return true;
    }
    return false;
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
    enemyReady_[id.slot] = actionReadyMs();
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
    out.damageType = attack->damageType;
    const auto& timing = presentation_[a->npc.type.index];
    out.impactMs = ranged ? timing.bowImpact : timing.impact;
    out.packTactics = packSupported(id, origin_.cell);
    out.attackRollMode = effects_.attackMode(
        id, out.recipient, combatAdjacent(map_, a->npc.cell, origin_.cell), opening || out.packTactics,
        state_.defending[out.recipient.index] || (ranged && combatAdjacent(map_, a->npc.cell, origin_.cell)));
    const auto roll = rng_.d20(out.attackRollMode).value;
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
    enemyReady_[id.slot] = actionReadyMs();
    alert(id, origin_);
    return queueAttack(out, origin_, now);
}
std::uint64_t CombatController::enemySpeed(ActorId id) const noexcept {
    const auto* actor = actors_.actor(id);
    if (!actor || effects_.has(id, EffectKind::Restrained))
        return 0;
    const auto base = std::uint64_t(actors_.catalog().monster(actor->monster)->speedFeet);
    return effects_.has(id, EffectKind::Slow) ? (base > 10 ? base - 10 : 0) : base;
}
CombatOutcome CombatController::enemyTurn(ActorId id, std::uint64_t now, bool& waitingForPath) noexcept {
    waitingForPath = false;
    CombatOutcome out;
    out.enemy = id;
    out.event = CombatEvent::Spent;
    const auto* a = actors_.actor(id);
    if (!a || !a->hostile() || a->life != ActorLife::Alive || !effects_.conscious(id))
        return out;
    if (!enemyMovement_.initialized) {
        if (surprised_[id.slot] || enemyReady_[id.slot] > state_.gameplayMs) {
            out.spentCause = surprised_[id.slot] ? SpentCause::Surprise : SpentCause::Cooldown;
            surprised_[id.slot] = false;
            return out;
        }
        enemyMovement_.initialized = true;
        if (currentEvents_ && effects_.escapeWeb(id, now, *currentEvents_)) {
            enemyMovement_.actionSpent = true;
            enemyReady_[id.slot] = actionReadyMs();
        }
    }
    const auto speed = enemySpeed(id);
    const auto allowance = speed * (enemyMovement_.dashed ? 2 : 1);
    auto remaining = allowance > enemyMovement_.spentFeet ? allowance - enemyMovement_.spentFeet : 0;
    if (currentEvents_ && effects_.has(id, EffectKind::Knockdown) && speed && remaining >= speed / 2 &&
        effects_.stand(id, now, *currentEvents_)) {
        enemyMovement_.spentFeet += speed / 2;
        remaining -= speed / 2;
    }
    // Rescue intent shares the ordinary, bounded route workspace and movement budget.
    const auto intelligence = actors_.catalog().monster(a->monster)->abilities[3];
    if (!enemyMovement_.actionSpent && currentEvents_ && intelligence >= 6 &&
        (!enemyMovement_.wakeDecided || enemyMovement_.wakeTarget.world)) {
        TacticalNavigation::WakeGoals goals;
        goals.budgetFeet = unsigned(remaining);
        std::array<int, 8> proximity{};
        const auto here = globalSubcell(a->npc.cell, a->npc.subcell);
        for (std::size_t i = 0; i < state_.enemyCount; ++i) {
            const auto ally = state_.enemies[i];
            const auto* sleeper = actors_.actor(ally);
            if (!sleeper || ally.slot == id.slot || sleeper->life != ActorLife::Alive ||
                !effects_.has(ally, EffectKind::Sleep) ||
                (enemyMovement_.wakeDecided && ally.slot != enemyMovement_.wakeTarget.slot))
                continue;
            bool unspent = false;
            for (std::size_t o = position_; o < orderCount_; ++o)
                if (!order_[o].party && groups_[order_[o].index].index == sleeper->monster.index)
                    unspent = o > position_ || i > enemyCursor_;
            if (effects_.sleepRemaining(ally) <= (unspent ? 0u : combatRoundMs))
                continue;
            const auto tile = globalSubcell(sleeper->npc.cell, sleeper->npc.subcell);
            const int distance = std::abs(tile.x - here.x) + std::abs(tile.y - here.y);
            const unsigned rank = (unspent ? 0u : unsigned(NpcWorld::maxInstances)) + ally.slot;
            unsigned pos = 0;
            while (pos < goals.count &&
                   (proximity[pos] < distance || (proximity[pos] == distance && goals.rank[pos] < rank)))
                ++pos;
            if (pos >= goals.allies.size())
                continue;
            for (unsigned j = std::min<unsigned>(goals.count, 7); j > pos; --j) {
                goals.allies[j] = goals.allies[j - 1];
                goals.rank[j] = goals.rank[j - 1];
                proximity[j] = proximity[j - 1];
            }
            goals.allies[pos] = ally;
            goals.rank[pos] = rank;
            proximity[pos] = distance;
            goals.count = std::min<unsigned>(8, goals.count + 1);
        }
        if (goals.count) {
            const auto step = ai_.wakeStep(id, origin_, now, goals);
            if (step.status == TacticalNavigation::Status::Pending) {
                waitingForPath = true;
                out.event = CombatEvent::None;
                return out;
            }
            if (!enemyMovement_.wakeDecided) {
                enemyMovement_.wakeDecided = true;
                if ((step.status == TacticalNavigation::Status::Step ||
                     step.status == TacticalNavigation::Status::AtGoal) &&
                    rng_.roll({1, 100, 0}).value.total <= wakeChance(unsigned(intelligence)))
                    enemyMovement_.wakeTarget = step.target;
            }
            if (enemyMovement_.wakeTarget.world) {
                if (step.status == TacticalNavigation::Status::AtGoal) {
                    effects_.wake(enemyMovement_.wakeTarget, now, *currentEvents_);
                    enemyMovement_.actionSpent = true;
                    enemyReady_[id.slot] = actionReadyMs();
                    out.event = CombatEvent::Wake;
                    out.awakened = enemyMovement_.wakeTarget;
                    return out;
                }
                if (step.status == TacticalNavigation::Status::Step && step.costFeet <= remaining &&
                    ai_.commitCombatStep(id, origin_, step, now)) {
                    enemyMovement_.spentFeet += step.costFeet;
                    enemyMovement_.arriving = true;
                    state_.resolvingAction = true;
                    state_.awaitingAction = false;
                    due_ = now + a->walkDurationMs;
                    out.event = CombatEvent::Move;
                    return out;
                }
                enemyMovement_.wakeTarget = {};
            }
        } else {
            enemyMovement_.wakeDecided = true;
            enemyMovement_.wakeTarget = {};
        }
    }
    if (!enemyMovement_.actionSpent && enemyCaster(id)) {
        auto spell = enemySpell(id, now);
        if (spell.event != CombatEvent::None || spell.reason != CommandReason::Resolved)
            return spell;
    }
    const bool adjacent = combatAdjacent(map_, a->npc.cell, origin_.cell);
    const bool wounded =
        rules::hasTrait(actors_.catalog().monster(a->monster)->traits, rules::Trait::NimbleEscape) &&
        a->hitPoints * 2 <= a->maxHitPoints;
    const bool melee = actors_.catalog().attack(melee_[id.slot]);
    const bool ranged = actors_.catalog().attack(ranged_[id.slot]);
    const bool shot = frontal(id) && ranged && effects_.sees(id) && inBowRange(a->npc.cell, origin_.cell) &&
                      lineOfSight(map_, a->npc.cell, origin_.cell);
    const bool retreat = ranged && (!melee || wounded) && (adjacent || !shot);
    if (!enemyMovement_.actionSpent && !retreat && melee && frontal(id)) {
        const auto slot = meleeSlot(id, actors_.occupiedSlots(a->npc.cell));
        if (slot && *slot == a->npc.subcell)
            return enemyAttack(id, false, now);
    }
    const bool travel = retreat || (!enemyMovement_.retreated && ((!wounded && melee) || !shot));
    if (travel && speed && effects_.mobile(id) && !effects_.has(id, EffectKind::Knockdown)) {
        const auto step = ai_.combatStep(id, origin_, now, retreat);
        if (step.status == TacticalNavigation::Status::Pending) {
            waitingForPath = true;
            out.event = CombatEvent::None;
            return out;
        }
        if (step.status == TacticalNavigation::Status::Step) {
            if (step.costFeet > remaining && step.costFeet <= remaining + speed &&
                !enemyMovement_.actionSpent && !retreat && !shot) {
                enemyMovement_.dashed = true;
                enemyMovement_.actionSpent = true;
                enemyReady_[id.slot] = actionReadyMs();
                remaining += speed;
                if (currentEvents_) {
                    auto dash = out;
                    dash.event = CombatEvent::Dash;
                    currentEvents_->append(dash);
                }
            }
            if (step.costFeet <= remaining && ai_.commitCombatStep(id, origin_, step, now)) {
                enemyMovement_.spentFeet += step.costFeet;
                enemyMovement_.retreated |= retreat;
                enemyMovement_.arriving = true;
                state_.resolvingAction = true;
                state_.awaitingAction = false;
                due_ = now + a->walkDurationMs;
                out.event = CombatEvent::Move;
                return out;
            }
        }
    }
    if (!enemyMovement_.actionSpent && shot)
        return enemyAttack(id, true, now);
    // Movement headings are presentation, not an instruction to stare away from
    // the known opponent when a turn ends without a shot.
    if (enemyMovement_.spentFeet && effects_.sees(id) && frontal(id))
        actors_.face(id, headingToward(a->npc.cell, origin_.cell));
    // Exhaustion/blocked movement and non-attacking actions still finish readiness once.
    enemyReady_[id.slot] = actionReadyMs();
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
    saveDue_[id.index] = actionReadyMs();
    return out;
}
} // namespace blobber
