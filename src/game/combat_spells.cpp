#include "game/combat.hpp"
#include "game/perception.hpp"
#include <algorithm>
#include <limits>

namespace blobber {
namespace {
using rules::SpellKind;
bool effectSpell(SpellKind k) {
    return k == SpellKind::Bless || k == SpellKind::Bane || k == SpellKind::ShieldOfFaith ||
           k == SpellKind::MageArmor;
}
bool delayed(SpellKind k) {
    return k == SpellKind::FireBolt || k == SpellKind::GuidingBolt || k == SpellKind::MagicMissile ||
           k == SpellKind::InflictWounds;
}
} // namespace
SpellArea spellArea(const ActorWorld& actors, Player pose) noexcept {
    SpellArea result;
    for (int side = -1; side <= 1; ++side) {
        const auto cell = pose.cell + forward(pose.heading) + right(pose.heading) * side;
        result.cells[side + 1] = cell;
        result.eligible[side + 1] = inMeleeFront(actors.map(), pose.cell, pose.heading, cell);
    }
    for (std::size_t i = 0; i < actors.snapshot().count; ++i) {
        const auto* a = actors.actor(actors.id(i));
        if (!a || a->life != ActorLife::Alive)
            continue;
        for (std::size_t j = 0; j < result.cells.size(); ++j)
            if (result.eligible[j] && a->npc.cell == result.cells[j]) {
                assert(result.count < result.actors.size());
                result.actors[result.count++] = actors.id(i);
                break;
            }
    }
    return result;
}
bool CombatController::casterResolving(rules::CharacterId id) const noexcept {
    return id.index < rules::partySize && attackBusy(NpcWorld::maxInstances + id.index);
}
CommandReason CombatController::spellRecipient(const PartyCommand& c, Player pose,
                                               std::size_t i) const noexcept {
    const auto* definition = party_.catalog().spell(c.spell);
    if (!definition || i >= c.enemies.size() || !party_.character(c.character))
        return CommandReason::InvalidSpell;
    const auto kind = definition->kind;
    const auto* r = rules::spellRules(kind);
    if (r->hostile) {
        const auto* a = actors_.actor(c.enemies[i]);
        if (!a || a->life != ActorLife::Alive)
            return CommandReason::InvalidTarget;
        if (kind == SpellKind::InflictWounds) {
            if (!exposed(c.character))
                return CommandReason::RearBlocked;
            if (!inMeleeFront(map_, pose.cell, pose.heading, a->npc.cell))
                return CommandReason::OutOfRange;
            if (!exposed(c.enemies[i]))
                return CommandReason::FrontProtected;
        } else {
            if (!inBowRange(pose.cell, a->npc.cell))
                return CommandReason::OutOfRange;
            if (!inPartyView(map_, pose.cell, pose.heading, a->npc.cell))
                return CommandReason::NotVisible;
        }
        if (kind == SpellKind::AcidSplash && i &&
            (!actors_.actor(c.enemies[0]) || !(a->npc.cell == actors_.actor(c.enemies[0])->npc.cell)))
            return CommandReason::SameCellRequired;

    } else {
        const auto* ally = party_.character(c.allies[i]);
        if (!ally || ally->life == rules::Life::Dead)
            return CommandReason::DeadAlly;
        if (kind == SpellKind::CureWounds && ally->hitPoints == ally->stats.maxHitPoints)
            return CommandReason::FullHealth;
        if (kind == SpellKind::SpareTheDying && ally->life != rules::Life::Dying)
            return CommandReason::MustBeDying;
    }

    if (kind == SpellKind::MageArmor) {
        auto one = c;
        one.allies[0] = c.allies[i];
        one.targetCount = 1;
        if (effects_.validate(spellEffect(one, kind)) == EffectError::InvalidTargets)
            return CommandReason::Armored;
    }
    return CommandReason::Ready;
}
EffectRequest CombatController::spellEffect(const PartyCommand& c, SpellKind kind) const noexcept {
    EffectRequest request;
    request.world = c.world;
    request.caster = c.character;
    request.count = c.targetCount;
    request.kind = kind == SpellKind::Bless       ? EffectKind::Bless
                   : kind == SpellKind::Bane      ? EffectKind::Bane
                   : kind == SpellKind::MageArmor ? EffectKind::MageArmor
                                                  : EffectKind::ShieldOfFaith;
    request.difficulty =
        8 + party_.character(c.character)->stats.proficiency + party_.castingModifier(c.character);
    for (std::size_t i = 0; i < std::min(c.targetCount, request.targets.size()); ++i)
        request.targets[i] = kind == SpellKind::Bane ? EffectTarget(c.enemies[i]) : EffectTarget(c.allies[i]);
    return request;
}
bool CombatController::safeRecovery(Player pose) const noexcept {
    if (state_.phase != CombatPhase::Exploration || pendingImpacts())
        return false;
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto id = actors_.id(i);
        const auto* a = actors_.actor(id);
        if (a && a->life == ActorLife::Alive &&
            (ai_.aware(id) || combatAdjacent(map_, pose.cell, a->npc.cell)))
            return false;
    }
    return true;
}
CommandReason CombatController::validateMagic(const PartyCommand& c, Player pose) const noexcept {
    const auto* ch = party_.character(c.character);
    if (!ch)
        return CommandReason::InvalidCharacter;
    if (c.world != state_.world || c.turn != state_.turn)
        return CommandReason::StaleTurn;
    if (!effects_.canAct(c.character))
        return CommandReason::CannotAct;
    if (c.action == PartyAction::Feature && c.feature == rules::Feature::DismissConcentration)
        return effects_.concentration(c.character).serial ? CommandReason::Ready : CommandReason::Unavailable;
    if (realTime(state_.phase)) {
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
    if (attackBusy(NpcWorld::maxInstances + c.character.index) ||
        nextAttack_ > std::numeric_limits<std::uint64_t>::max() - 3)
        return CommandReason::AttackPending;
    if (c.action == PartyAction::Feature) {
        if (c.targetCount)
            return CommandReason::InvalidTargets;
        if (c.feature == rules::Feature::SecondWind) {
            if (!party_.hasFeature(c.character, rules::ClassFeature::SecondWind))
                return CommandReason::FeatureRequired;
            if (!ch->secondWind)
                return CommandReason::NoCharges;
            return ch->hitPoints < ch->stats.maxHitPoints ? CommandReason::Ready : CommandReason::FullHealth;
        }
        if (c.feature == rules::Feature::ArcaneRecovery) {
            if (!party_.hasFeature(c.character, rules::ClassFeature::ArcaneRecovery))
                return CommandReason::FeatureRequired;
            if (!ch->arcaneRecovery)
                return CommandReason::NoCharges;
            if (ch->mana == ch->maxMana)
                return CommandReason::FullMana;
            return safeRecovery(pose) ? CommandReason::Ready : CommandReason::Unsafe;
        }
        return CommandReason::InvalidAction;
    }
    const auto* definition = party_.catalog().spell(c.spell);
    if (!definition)
        return CommandReason::InvalidSpell;
    const auto* cls = party_.catalog().characterClass(ch->build.characterClass);
    bool known = false;
    for (std::size_t i = 0; i < cls->spellCount; ++i)
        known |= cls->spells[i].index == c.spell.index;
    if (!known)
        return CommandReason::InvalidSpell;
    const auto& r = *rules::spellRules(definition->kind);
    if (ch->mana < r.level * 2)
        return CommandReason::InsufficientMana;
    if (r.area) {
        if (c.targetCount)
            return CommandReason::InvalidTargets;
        return spellArea(actors_, pose).count ? CommandReason::Ready : CommandReason::MissingTarget;
    }
    if (!c.targetCount)
        return CommandReason::MissingTarget;
    if (c.targetCount > r.maxTargets)
        return CommandReason::InvalidTargets;
    unsigned darts = 0;
    for (std::size_t i = 0; i < c.targetCount; ++i) {
        const auto eligibility = spellRecipient(c, pose, i);
        if (eligibility != CommandReason::Ready)
            return eligibility;
        if (definition->kind == SpellKind::MagicMissile) {
            if (!c.darts[i] || c.darts[i] > 3)
                return CommandReason::InvalidTargets;
            darts += c.darts[i];
        }
        for (std::size_t j = 0; j < i; ++j)
            if (r.hostile ? sameActor(c.enemies[i], c.enemies[j]) : c.allies[i].index == c.allies[j].index)
                return CommandReason::InvalidTargets;
    }
    if (definition->kind == SpellKind::MagicMissile && darts != 3)
        return CommandReason::InvalidTargets;
    if (effectSpell(definition->kind)) {
        const auto error = effects_.validate(spellEffect(c, definition->kind));
        if (error != EffectError::None)
            return error == EffectError::Capacity ? CommandReason::EffectCapacity
                                                  : CommandReason::InvalidTargets;
    }
    return CommandReason::Ready;
}
bool CombatController::sneakPreview(rules::CharacterId id, ActorId target, bool ranged,
                                    Player pose) const noexcept {
    const auto* a = actors_.actor(target);
    if (!a)
        return false;
    const bool opening = ranged && realTime(state_.phase) && !ai_.aware(target) &&
                         !npcSees(map_, a->npc.cell, a->npc.facing, pose.cell);
    return sneakEligible(
        id, target,
        rules::rollMode(opening || effects_.guiding(target) || effects_.has(target, EffectKind::HoldPerson),
                        ranged && adjacent(pose.cell)),
        ranged, pose);
}
bool CombatController::sneakEligible(rules::CharacterId id, ActorId target, rules::RollMode mode, bool ranged,
                                     Player pose) const noexcept {
    if (!party_.hasFeature(id, rules::ClassFeature::SneakAttack) ||
        sneakReady_[id.index] > state_.gameplayMs || mode == rules::RollMode::Disadvantage)
        return false;
    const auto* c = party_.character(id);
    const auto item = static_cast<unsigned>(ranged ? EquipmentSlot::Ranged : EquipmentSlot::MainHand);
    if (item >= c->stats.weapons.size())
        return false;
    const auto* w = party_.equipped(id, ranged ? EquipmentSlot::Ranged : EquipmentSlot::MainHand);
    if (!w || (!w->finesse && !w->ranged))
        return false;
    if (mode == rules::RollMode::Advantage)
        return true;
    const auto* a = actors_.actor(target);
    if (!a || !inMeleeFront(map_, pose.cell, pose.heading, a->npc.cell))
        return false;
    for (std::size_t i = 0; i < 2; ++i) {
        const auto ally = party_.snapshot().formation[i];
        if (ally.index != id.index && effects_.canAct(ally))
            return true;
    }
    return false;
}
CombatOutcome CombatController::submitMagic(const PartyCommand& c, Player pose, std::uint64_t now,
                                            CombatEvents& events) noexcept {
    CombatOutcome out;
    out.reason = validateMagic(c, pose);
    if (out.reason != CommandReason::Ready)
        return out;
    out.reason = CommandReason::Resolved;
    out.character = c.character;
    out.occurredMs = now;
    out.feature = c.feature;
    out.phase = state_.phase;
    if (c.action == PartyAction::Feature && c.feature == rules::Feature::DismissConcentration) {
        effects_.remove(effects_.concentration(c.character), now, events);
        return out;
    }
    const auto* ch = party_.character(c.character);
    const bool turn = state_.phase == CombatPhase::Turns;
    partyReady_[c.character.index] = state_.gameplayMs + combatRoundMs;
    state_.spent[c.character.index] = true;
    out.event = CombatEvent::Cast;
    if (c.action == PartyAction::Spell)
        out.spell = party_.catalog().spell(c.spell)->kind;
    events.append(out);
    if (c.action == PartyAction::Feature) {
        const int oldMana = ch->mana;
        party_.useFeature(c.character, c.feature);
        auto result = out;
        result.recipient = c.character;
        result.hit = true;
        if (c.feature == rules::Feature::SecondWind) {
            const auto healing = rng_.roll({1, 10, ch->build.level}).value.total;
            const int hp = ch->hitPoints;
            party_.heal(c.character, healing);
            result.damage = ch->hitPoints - hp;
            result.event = CombatEvent::Healing;
        } else {
            result.event = CombatEvent::Recovery;
            result.damage = ch->mana - oldMana;
        }
        events.append(result);
    } else {
        const auto kind = out.spell;
        const auto& r = *rules::spellRules(kind);
        party_.spendMana(c.character, r.level * 2);
        if (effectSpell(kind)) {
            if (r.hostile)
                for (std::size_t i = 0; i < c.targetCount; ++i)
                    alert(c.enemies[i], pose);
            effects_.apply(spellEffect(c, kind), now, events);
            if (r.hostile)
                for (std::size_t i = 0; i < c.targetCount; ++i)
                    spellPulse(c.enemies[i], c.character, kind, pose, now);
            reconcileEffects(now, events);
        } else if (!r.hostile) {
            const auto recipient = c.allies[0];
            auto result = out;
            result.recipient = recipient;
            result.hit = true;
            if (kind == SpellKind::SpareTheDying) {
                party_.stabilize(recipient);
                result.event = CombatEvent::Aid;
            } else {
                const int bonus =
                    party_.castingModifier(c.character) +
                    (party_.hasFeature(c.character, rules::ClassFeature::LifeDomain) ? 2 + r.level : 0);
                const auto healing = std::max<std::int64_t>(0, rng_.roll({1, 8, bonus}).value.total);
                const int hp = party_.character(recipient)->hitPoints;
                party_.heal(recipient, healing);
                result.damage = party_.character(recipient)->hitPoints - hp;
                result.event = CombatEvent::Healing;
            }
            events.append(result);
        } else {
            std::array<ActorId, 12> targets{};
            std::size_t count = 0;
            if (r.area) {
                const auto area = spellArea(actors_, pose);
                targets = area.actors;
                count = area.count;
            } else {
                count = c.targetCount;
                std::copy_n(c.enemies.begin(), count, targets.begin());
                std::sort(targets.begin(), targets.begin() + count,
                          [](ActorId a, ActorId b) { return a.slot < b.slot; });
            }
            const auto sharedDamage = r.attack ? 0 : rng_.roll(r.dice).value.total;
            std::array<bool, 12> push{};
            auto& payload = spellPayloads_[c.character.index];
            payload = {};
            for (std::size_t i = 0; i < count; ++i) {
                const auto id = targets[i];
                const auto* a = actors_.actor(id);
                auto result = out;
                result.event = CombatEvent::Attack;
                result.enemy = id;
                result.kind =
                    kind == SpellKind::InflictWounds ? rules::AttackKind::Melee : rules::AttackKind::Ranged;
                result.hit = true;
                if (r.attack) {
                    const bool ranged = result.kind == rules::AttackKind::Ranged;
                    const bool opening = ranged && realTime(state_.phase) && !ai_.aware(id) &&
                                         !npcSees(map_, a->npc.cell, a->npc.facing, pose.cell);
                    const bool held = effects_.has(id, EffectKind::HoldPerson);
                    const auto mode = rules::rollMode(opening || held || effects_.guiding(id),
                                                      ranged && adjacent(pose.cell));
                    const auto roll = rng_.d20(mode).value;
                    effects_.consumeGuiding(id, now, events);
                    result.roll = roll.selected;
                    const auto hit =
                        rules::resolveAttack(roll,
                                             ch->stats.proficiency + party_.castingModifier(c.character) +
                                                 effects_.rollModifier(c.character),
                                             effects_.armorClass(id))
                            .value;
                    result.hit = hit.outcome != rules::AttackOutcome::Miss;
                    result.critical = hit.outcome == rules::AttackOutcome::Critical ||
                                      (result.hit && held && combatAdjacent(map_, pose.cell, a->npc.cell));
                    if (result.hit)
                        result.damage = rng_.damage(r.dice, result.critical).value.total;
                } else if (kind == SpellKind::MagicMissile) {
                    for (std::size_t j = 0; j < c.targetCount; ++j)
                        if (sameActor(id, c.enemies[j]))
                            result.darts = c.darts[j];
                    result.damage = sharedDamage * result.darts;
                } else {
                    const int dc = 8 + ch->stats.proficiency + party_.castingModifier(c.character);
                    const auto save = effects_.save(id, r.save, dc);
                    auto notice = result;
                    notice.event = CombatEvent::SpellSave;
                    notice.roll = save.roll.selected;
                    notice.saveTotal = static_cast<int>(save.total);
                    notice.saveDifficulty = dc;
                    notice.hit = save.succeeded;
                    events.append(notice);
                    result.hit = !save.succeeded || r.area;
                    result.damage = save.succeeded ? (r.area ? sharedDamage / 2 : 0) : sharedDamage;
                    push[i] = kind == SpellKind::Thunderwave && !save.succeeded;
                }
                if (delayed(kind))
                    payload.outcomes[payload.count++] = result;
                else {
                    alert(id, pose);
                    applyDamage(result, now, events);
                    spellPulse(id, c.character, kind, pose, now);
                }
            }
            if (kind == SpellKind::Thunderwave) {
                for (std::size_t i = 0; i < count; ++i) {
                    const auto* a = actors_.actor(targets[i]);
                    if (!push[i] || !a || a->life != ActorLife::Alive)
                        continue;
                    const auto destination = a->npc.cell + forward(pose.heading);
                    Cell subcell = a->npc.subcell;
                    const auto occupied = actors_.occupiedSlots(destination);
                    if (occupied & (1u << (subcell.x + 2 * subcell.y))) {
                        unsigned slot = 0;
                        while (slot < 4 && (occupied & (1u << slot)))
                            ++slot;
                        if (slot == 4)
                            continue;
                        subcell = {int(slot % 2), int(slot / 2)};
                    }
                    if (actors_.walk(targets[i], destination, subcell, pose.cell, now).changed) {
                        auto moved = out;
                        moved.event = CombatEvent::Move;
                        moved.enemy = targets[i];
                        events.append(moved);
                    }
                }
            }
            if (delayed(kind)) {
                out = queueAttack(payload.outcomes[0], pose, now);
                auto& main = visuals_.attacks[NpcWorld::maxInstances + c.character.index];
                for (std::size_t i = 1; i < payload.count; ++i) {
                    auto& v = visuals_.spellExtras[c.character.index * 2 + i - 1];
                    v = main;
                    v.id = {state_.world, nextAttack_++};
                    v.enemy = payload.outcomes[i].enemy;
                    const auto* a = actors_.actor(v.enemy);
                    v.npcOrigin = a->npc;
                    v.sourcePosition = v.targetPosition = actorPosition(*a, now);
                    v.darts = payload.outcomes[i].darts;
                    v.impactMs = now + arrowFlightMs(pose.cell, a->npc.cell);
                    v.endMs = v.impactMs + combatFeedbackDurationMs;
                    setAttackHold(NpcWorld::maxInstances + c.character.index, v.endMs);
                    payload.outcomes[i].attackId = v.id;
                    auto released = payload.outcomes[i];
                    released.event = CombatEvent::ProjectileReleased;
                    released.damage = 0;
                    released.hit = released.critical = false;
                    events.append(released);
                }
            }
        }
    }
    refreshReadiness();
    if (out.event != CombatEvent::AttackStarted) {
        std::uint64_t hold = now + combatFeedbackDurationMs;
        for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
            const auto& a = actors_.snapshot().actors[i];
            if (a.hasDamage && a.damageTimeMs == now)
                hold =
                    std::max(hold, now + (a.life == ActorLife::Dead ? presentation_[a.npc.type.index].death
                                                                    : presentation_[a.npc.type.index].hurt));
        }
        partyBusy_ = std::max(partyBusy_, hold);
        if (turn) {
            state_.awaitingAction = false;
            state_.resolvingAction = true;
            due_ = hold;
        }
    }
    if (!turn)
        ++state_.turn;
    out.phase = state_.phase;
    return out;
}
void CombatController::spellPulse(ActorId id, rules::CharacterId caster, SpellKind kind, Player pose,
                                  std::uint64_t now) noexcept {
    const auto* a = actors_.actor(id);
    if (!a)
        return;
    auto* slot = &visuals_.spellPulses[0];
    for (auto& v : visuals_.spellPulses) {
        if (v.stage == AttackStage::Empty || v.endMs <= now) {
            slot = &v;
            break;
        }
        if (v.startedMs < slot->startedMs)
            slot = &v;
    }
    auto& v = *slot;
    v = {};
    // Cosmetic pulse identity does not consume the authoritative attack serial.
    v.id = {state_.world, now + 1};
    v.stage = AttackStage::Resolved;
    v.enemy = id;
    v.character = caster;
    v.hit = true;
    v.spell = kind;
    v.partyOrigin = pose;
    v.npcOrigin = a->npc;
    v.sourcePosition = v.targetPosition = actorPosition(*a, now);
    v.startedMs = v.releaseMs = v.impactMs = now;
    v.endMs = now + 120;
    ++visuals_.revision;
}
} // namespace blobber
