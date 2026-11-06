#include "game/combat.hpp"
namespace blobber {
void SupportVisualJournal::clear(std::uint64_t world) noexcept {
    const auto epoch = snapshot_.epoch + 1;
    if (world != snapshot_.world)
        next_ = 1;
    snapshot_ = {};
    snapshot_.world = world;
    snapshot_.epoch = epoch;
    cursor_ = 0;
}
void SupportVisualJournal::emit(SupportVisual cue) noexcept {
    if (!snapshot_.world || next_ == UINT64_MAX)
        return;
    // A new application supersedes older accents on that recipient. End notifications
    // cancel their application immediately instead of displaying contradictory overlays.
    for (auto& old : snapshot_.records) {
        if (!old.id.serial || old.spell != cue.spell || !(old.target == cue.target))
            continue;
        const bool sameEffect =
            cue.effect.serial ? old.effect.world == cue.effect.world && old.effect.serial == cue.effect.serial
                              : !old.effect.serial;
        if ((cue.phase == SupportPhase::Apply && old.phase == SupportPhase::Apply) ||
            (cue.phase == SupportPhase::End && sameEffect && old.phase == SupportPhase::Apply))
            old = {};
    }
    auto& slot = snapshot_.records[cursor_];
    if (slot.id.serial && cue.occurredMs >= slot.occurredMs &&
        cue.occurredMs - slot.occurredMs < supportDuration(slot.phase))
        ++snapshot_.dropped;
    cue.id = {snapshot_.world, next_++};
    slot = cue;
    cursor_ = (cursor_ + 1) % snapshot_.capacity;
    ++snapshot_.revision;
}
void SupportVisualJournal::combat(const CombatOutcome& out) noexcept {
    SupportVisual cue;
    cue.occurredMs = out.occurredMs;
    cue.spell = out.spell;
    cue.source = out.character;
    cue.target = out.recipient;
    if (out.event == CombatEvent::Effect) {
        const auto& e = out.effect;
        if (e.kind == EffectKind::Web || e.kind == EffectKind::SpiritualWeapon ||
            e.kind == EffectKind::Slow || e.kind == EffectKind::Knockdown)
            return;
        cue.spell = effectSpell(e.kind);
        cue.target = e.target;
        cue.source = e.caster;
        cue.effect = e.id;
        cue.reason = e.reason;
        if (e.kind == EffectKind::MirrorImage && e.reason == EffectEnd::Consumed)
            return;
        if (e.notice == EffectNotice::Applied)
            cue.phase = SupportPhase::Apply;
        else if (e.notice == EffectNotice::Ended || e.notice == EffectNotice::RecipientRemoved)
            cue.phase = SupportPhase::End;
        else if (e.notice == EffectNotice::InitialSave && e.succeeded &&
                 (e.kind == EffectKind::Bane || e.kind == EffectKind::HoldPerson))
            cue.phase = SupportPhase::Negated;
        else
            return;
        if (cue.phase == SupportPhase::End &&
            (e.reason == EffectEnd::InvalidTarget || e.reason == EffectEnd::Incapacitated ||
             e.reason == EffectEnd::NoTargets))
            return;
    } else if (out.event == CombatEvent::Aid && out.spell == rules::SpellKind::SpareTheDying) {
        cue.phase = SupportPhase::Apply;
    } else if (out.event == CombatEvent::Healing && out.spell == rules::SpellKind::FalseLife) {
        cue.phase = SupportPhase::Apply;
    } else if (out.event == CombatEvent::Attack && out.spell == rules::SpellKind::SpiritualWeapon &&
               !out.targetGone && effects_) {
        const auto* weapon = effects_->spiritualWeapon(out.character);
        if (!weapon)
            return;
        cue.effect = weapon->id;
        cue.cell = weapon->cell;
        cue.target = out.enemy;
        cue.phase = out.hit ? SupportPhase::Strike : SupportPhase::Negated;
    } else
        return;
    // Whole-effect Ended notices intentionally carry no recipient. Resolve the still-live
    // instance before the authoritative owner deactivates it.
    if (out.event == CombatEvent::Effect && out.effect.notice == EffectNotice::Ended &&
        out.effect.id.serial && effects_) {
        if (const auto* effect = effects_->find(out.effect.id))
            for (std::size_t i = 0; i < effect->count; ++i) {
                cue.target = effect->targets[i];
                emit(cue);
            }
        return;
    }
    emit(cue);
}
} // namespace blobber
