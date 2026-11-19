#include "game/combat.hpp"
namespace blobber {
CombatOutcome CombatController::spectralAttack(const PartyCommand& c, Player pose, std::uint64_t now,
                                               CombatEvents& events) noexcept {
    CombatOutcome out;
    out.event = CombatEvent::Attack;
    out.character = c.character;
    out.enemy = c.enemies[0];
    out.spell = rules::SpellKind::SpiritualWeapon;
    out.damageType = rules::DamageType::Force;
    out.kind = rules::AttackKind::Melee;
    const auto* ch = party_.character(c.character);
    out.attackRollMode = spellAttackRoll(c.character, out.enemy, pose, out.spell).mode;
    const auto roll = rng_.d20(out.attackRollMode).value;
    effects_.consumeGuiding(out.enemy, now, events);
    out.roll = roll.selected;
    const auto hit = effects_.partyHit(c.character, out.enemy, roll,
                                       ch->stats.proficiency + ch->stats.modifiers[4] +
                                           ch->stats.spellAttackBonus + effects_.rollModifier(c.character));
    out.hit = hit.outcome != rules::AttackOutcome::Miss;
    out.critical = hit.outcome == rules::AttackOutcome::Critical || (out.hit && effects_.helpless(out.enemy));
    if (out.hit)
        out.damage = rng_.damage({1, 8, ch->stats.modifiers[4]}, out.critical).value.total;
    effects_.breakWard(c.character, now, events);
    return queueAttack(out, pose, now);
}
void CombatController::shatter(const PartyCommand& c, Player pose, std::uint64_t now,
                               CombatEvents& events) noexcept {
    const auto affected = area(c, pose);
    auto* visual = spellVisuals_.begin(rules::SpellKind::Shatter, c.character, pose, now);
    if (visual) {
        visual->cells = affected.cells;
        visual->footprint = affected.footprint;
        visual->eligible = affected.eligible;
        visual->includeOrigin = false;
        visual->center = c.aimedCell.value_or(
            c.aimedSubcell ? dungeonCell({c.aimedSubcell->x, c.aimedSubcell->y}) : pose.cell);
    }
    const auto damage = rng_.roll({3, 8, 0}).value.total;
    const auto dc = party_.spellAccess(c.character, c.spell).difficulty;
    for (std::size_t i = 0; i < affected.recipientCount; ++i) {
        const auto t = affected.recipients[i];
        const auto* actor = t.npc ? actors_.actor(t.enemy) : nullptr;
        const bool object = actor && actor->npc.sac();
        const bool immune = actor && !object &&
                            (actors_.catalog().monster(actor->monster)->damageProfile.immunities &
                             rules::damageBit(rules::DamageType::Thunder));
        const bool rollsSave = !object && !immune;
        const auto save =
            rollsSave ? effects_.save(t, rules::Ability::Constitution, dc) : rules::SaveResult{};
        CombatOutcome out;
        out.event = CombatEvent::SpellSave;
        out.spell = rules::SpellKind::Shatter;
        out.damageType = rules::DamageType::Thunder;
        out.character = c.character;
        out.enemy = t.enemy;
        out.recipient = t.character;
        out.enemyAttack = !t.npc;
        out.roll = save.roll.selected;
        out.saveTotal = int(save.total);
        out.saveDifficulty = dc;
        out.hit = save.succeeded;
        out.occurredMs = now;
        if (rollsSave)
            events.append(out);
        out.event = CombatEvent::Attack;
        out.hit = true;
        out.damage = save.succeeded ? damage / 2 : damage;
        if (t.npc)
            alert(t.enemy, pose);
        applyDamage(out, now, events);
        if (t.npc && visual && visual->count < visual->targets.size()) {
            auto target = spellTarget(out, now);
            target.saved = save.succeeded;
            visual->targets[visual->count++] = target;
        }
        if (t.npc)
            spellPulse(t.enemy, c.character, rules::SpellKind::Shatter, pose, now);
    }
}
} // namespace blobber
