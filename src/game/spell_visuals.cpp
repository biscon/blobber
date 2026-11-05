#include "game/spell_visuals.hpp"
#include <algorithm>
namespace blobber {
SpellCastVisual* SpellVisualJournal::begin(rules::SpellKind spell, rules::CharacterId caster, Player origin,
                                           std::uint64_t now) noexcept {
    if ((!elementalSpell(spell) && spell != rules::SpellKind::Sleep &&
         spell != rules::SpellKind::ColorSpray) ||
        !snapshot_.world || next_ == UINT64_MAX)
        return nullptr;
    SpellCastVisual* slot = nullptr;
    for (auto& c : snapshot_.casts) {
        if (c.stage == AttackStage::Empty ||
            ((c.stage == AttackStage::Resolved || c.stage == AttackStage::Canceled) && now >= c.impactMs &&
             now - c.impactMs >= 1000)) {
            slot = &c;
            break;
        }
        const bool completed = c.stage == AttackStage::Resolved || c.stage == AttackStage::Canceled;
        const bool oldCompleted =
            slot && (slot->stage == AttackStage::Resolved || slot->stage == AttackStage::Canceled);
        if (!slot || (completed && !oldCompleted) ||
            (completed == oldCompleted && c.id.serial < slot->id.serial))
            slot = &c;
    }
    if (slot->stage != AttackStage::Empty && (now < slot->impactMs || now - slot->impactMs < 1000))
        ++snapshot_.dropped;
    *slot = {};
    slot->id = {snapshot_.world, next_++};
    slot->spell = spell;
    slot->caster = caster;
    slot->origin = origin;
    slot->stage = AttackStage::Resolved;
    slot->releaseMs = slot->impactMs = now;
    ++snapshot_.revision;
    return slot;
}
void SpellVisualJournal::attack(const AttackVisual& v, SpellTargetVisual target,
                                CombatAttackId parent) noexcept {
    if (v.incoming || !elementalSpell(v.spell))
        return;
    SpellCastVisual* c = nullptr;
    if (parent.serial) {
        for (auto& candidate : snapshot_.casts)
            if (candidate.attack.world == parent.world && candidate.attack.serial == parent.serial)
                c = &candidate;
    } else
        c = begin(v.spell, v.character, v.partyOrigin, v.releaseMs);
    if (!c || c->deliveryCount == c->deliveries.size())
        return;
    if (!parent.serial) {
        c->attack = v.id;
        c->formation = v.formationSlot;
    }
    const auto i = c->deliveryCount++;
    c->deliveries[i] = {v.id, v.stage, v.releaseMs, v.impactMs, std::max(1u, unsigned(v.darts))};
    c->stage = v.stage;
    c->impactMs = std::max(c->impactMs, v.impactMs);
    c->targets[i] = target;
    c->count = c->deliveryCount;
    ++snapshot_.revision;
}
void SpellVisualJournal::resolve(CombatAttackId id, SpellTargetVisual target, std::uint64_t now,
                                 bool canceled) noexcept {
    if (!id.serial)
        return;
    for (auto& c : snapshot_.casts)
        for (unsigned i = 0; i < c.deliveryCount; ++i) {
            auto& d = c.deliveries[i];
            if (d.attack.world != id.world || d.attack.serial != id.serial)
                continue;
            d.stage = canceled ? AttackStage::Canceled : AttackStage::Resolved;
            d.impactMs = now;
            c.targets[i] = target;
            c.stage = AttackStage::Resolved;
            c.impactMs = 0;
            for (unsigned j = 0; j < c.deliveryCount; ++j) {
                c.impactMs = std::max(c.impactMs, c.deliveries[j].impactMs);
                if (c.deliveries[j].stage == AttackStage::Flight ||
                    c.deliveries[j].stage == AttackStage::Windup)
                    c.stage = AttackStage::Flight;
            }
            ++snapshot_.revision;
            return;
        }
}
} // namespace blobber
