#include "game/combat.hpp"
#include "game/perception.hpp"
#include <limits>

namespace blobber {
bool CombatController::enemyCaster(ActorId id) const noexcept {
    const auto* a = actors_.actor(id);
    return a && a->hostile() &&
           actors_.catalog().monster(a->monster)->spellcasting == rules::EnemyCasting::AcolyteSupport;
}
bool CombatController::enemySpellGeometry(ActorId id) const noexcept {
    const auto* a = actors_.actor(id);
    return enemyCaster(id) && effects_.canAct(id) && effects_.sees(id) && frontal(id) &&
           inBowRange(a->npc.cell, origin_.cell) && lineOfSight(map_, a->npc.cell, origin_.cell);
}
CombatOutcome CombatController::enemySpell(ActorId id, std::uint64_t now) noexcept {
    CombatOutcome out;
    out.enemy = id;
    out.enemyAttack = true;
    if (!enemyCaster(id) || !effects_.canAct(id) || !frontal(id) || !effects_.sees(id) ||
        attackBusy(id.slot) || enemyReady_[id.slot] > state_.gameplayMs)
        return out;
    const auto* a = actors_.actor(id);
    const auto& timing = presentation_[a->npc.type.index];
    const auto clip = timing.bow ? timing.bow : 1000;
    if (nextAttack_ == UINT64_MAX || now > UINT64_MAX - clip - combatFeedbackDurationMs ||
        state_.gameplayMs > UINT64_MAX - combatRoundMs) {
        out.reason = CommandReason::AttackPending;
        return out;
    }
    EffectRequest bless;
    bless.world = state_.world;
    bless.caster = id;
    bless.difficulty = 12;
    if (enemyMana_[id.slot] >= 2 && !effects_.concentrationSource(id).serial) {
        for (std::size_t i = 0; i < actors_.snapshot().count && bless.count < 3; ++i) {
            const auto other = actors_.id(i);
            const auto* ally = actors_.actor(other);
            if (i == id.slot || !member(other) || !effects_.conscious(other) ||
                effects_.has(other, EffectKind::Bless))
                continue;
            if ((ally->npc.cell == a->npc.cell || inBowRange(a->npc.cell, ally->npc.cell)) &&
                lineOfSight(map_, a->npc.cell, ally->npc.cell))
                bless.targets[bless.count++] = other;
        }
        if (bless.count && bless.count < 3 && !effects_.has(id, EffectKind::Bless))
            bless.targets[bless.count++] = id;
    }
    EffectId reservation;
    if (bless.count) {
        const auto result = effects_.reserveBless(bless);
        if (!result) {
            out.reason = CommandReason::EffectCapacity;
            return out;
        }
        reservation = result.id;
        enemyMana_[id.slot] -= 2;
        out.spell = rules::SpellKind::Bless;
        out.hit = true;
    } else {
        if (!enemySpellGeometry(id))
            return out;
        std::size_t selected = rules::partySize;
        const auto start = enemyTarget_[id.slot];
        for (std::size_t n = 0; n < rules::partySize; ++n) {
            const auto index = (start + n) % rules::partySize;
            if (party_.character(party_.snapshot().formation[index])->life == rules::Life::Conscious) {
                selected = index;
                break;
            }
        }
        if (selected == rules::partySize)
            return out;
        // Capacity/deadlines are checked before ward or damage/save RNG.
        enemyTarget_[id.slot] = std::uint8_t((selected + 1) % rules::partySize);
        enemyReady_[id.slot] = state_.gameplayMs + combatRoundMs;
        bool allowed = false;
        for (std::size_t n = 0; n < rules::partySize; ++n) {
            const auto index = (selected + n) % rules::partySize;
            const auto candidate = party_.snapshot().formation[index];
            if (party_.character(candidate)->life != rules::Life::Conscious)
                continue;
            if (currentEvents_ && !effects_.wardAllows(id, candidate, now, *currentEvents_))
                continue;
            out.recipient = candidate;
            enemyTarget_[id.slot] = std::uint8_t((index + 1) % rules::partySize);
            allowed = true;
            break;
        }
        if (!allowed) {
            out.event = CombatEvent::Spent;
            return out;
        }
        out.spell = rules::SpellKind::SacredFlame;
        out.damageType = rules::DamageType::Radiant;
        const auto damage = rng_.roll({1, 8, 0}).value.total;
        const auto save = effects_.save(out.recipient, rules::Ability::Dexterity, 12);
        out.saveTotal = int(save.total);
        out.saveDifficulty = 12;
        out.saveSucceeded = save.succeeded;
        out.roll = save.roll.selected;
        out.hit = !save.succeeded;
        out.damage = out.hit ? damage : 0;
    }
    ai_.discardPreparation(id);
    actors_.face(id, headingToward(a->npc.cell, origin_.cell));
    actors_.attack(id, now, true);
    enemyReady_[id.slot] = state_.gameplayMs + combatRoundMs;
    alert(id, origin_);
    out.kind = rules::AttackKind::Ranged;
    out.impactMs = timing.bowImpact ? timing.bowImpact : 600;
    out = queueAttack(out, origin_, now);
    pending_[id.slot].blessing = reservation;
    // Cast announces intent only. All accepted rolls remain in the pending record.
    out.event = CombatEvent::Cast;
    return out;
}
} // namespace blobber
