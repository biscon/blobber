#include "game/spell_visuals.hpp"
namespace blobber {
SpellCastVisual* SpellVisualJournal::begin(rules::SpellKind spell, rules::CharacterId caster, Player origin,
                                           std::uint64_t now) noexcept {
    if (!elementalSpell(spell) || !snapshot_.world || next_ == UINT64_MAX)
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
void SpellVisualJournal::attack(const AttackVisual& v, SpellTargetVisual target) noexcept {
    if (v.incoming || !elementalSpell(v.spell))
        return;
    if (auto* c = begin(v.spell, v.character, v.partyOrigin, v.releaseMs)) {
        c->attack = v.id;
        c->stage = v.stage;
        c->formation = v.formationSlot;
        c->impactMs = v.impactMs;
        c->targets[0] = target;
        c->count = 1;
    }
}
void SpellVisualJournal::resolve(CombatAttackId id, SpellTargetVisual target, std::uint64_t now,
                                 bool canceled) noexcept {
    for (auto& c : snapshot_.casts)
        if (c.attack.world == id.world && c.attack.serial == id.serial && id.serial) {
            c.stage = canceled ? AttackStage::Canceled : AttackStage::Resolved;
            c.impactMs = now;
            c.targets[0] = target;
            ++snapshot_.revision;
            return;
        }
}
} // namespace blobber
