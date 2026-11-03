#include "audio/events.hpp"
#include "game/combat.hpp"
#include <algorithm>
namespace blobber::audio {
const char* spellFamily(rules::SpellKind s) noexcept {
    using S = rules::SpellKind;
    switch (s) {
    case S::Bane:
    case S::InflictWounds:
    case S::FalseLife:
        return "dark";
    case S::FireBolt:
    case S::BurningHands:
    case S::ScorchingRay:
        return "fire";
    case S::RayOfFrost:
        return "ice";
    case S::ShockingGrasp:
        return "lightning";
    case S::AcidSplash:
        return "acid";
    case S::PoisonSpray:
        return "poison";
    case S::Thunderwave:
    case S::Shatter:
        return "thunder";
    case S::Sleep:
    case S::HoldPerson:
        return "control";
    case S::Grease:
    case S::Web:
        return "material";
    case S::MageArmor:
    case S::MagicMissile:
    case S::ColorSpray:
    case S::MirrorImage:
    case S::SpiritualWeapon:
        return "arcane";
    case S::CureWounds:
    case S::SpareTheDying:
    case S::LesserRestoration:
    case S::Aid:
        return "healing";
    case S::SacredFlame:
    case S::Bless:
    case S::ShieldOfFaith:
    case S::GuidingBolt:
    case S::Guidance:
    case S::Resistance:
    case S::Sanctuary:
        return "radiant";
    case S::Count:
        return "arcane";
    }
    return "arcane";
}
void Journal::emit(Event e) noexcept {
    if (!enabled || !e.cue)
        return;
    if (sequence == UINT64_MAX) {
        ++dropped;
        return;
    }
    e.sequence = ++sequence;
    if (count == capacity) {
        unsigned victim = 0;
        for (unsigned i = 1; i < count; ++i)
            if (entries[i].priority < entries[victim].priority)
                victim = i;
        ++dropped;
        if (e.priority < entries[victim].priority)
            return;
        for (unsigned i = victim + 1; i < count; ++i)
            entries[i - 1] = entries[i];
        --count;
    }
    entries[count++] = e;
}
void Journal::combat(const CombatOutcome& o) noexcept {
    if (o.event == CombatEvent::Started && !encounterActive) {
        encounterActive = true;
        ++encounter;
    } else if (o.event == CombatEvent::Finished) {
        encounterActive = false;
    }
    if (!enabled)
        return;
    Event e;
    e.actor = o.enemy;
    e.positional = o.enemyAttack;
    switch (o.event) {
    case CombatEvent::Started:
        e.cue = "encounter";
        e.priority = 3;
        break;
    case CombatEvent::Finished:
        e.cue = o.phase == CombatPhase::Defeat ? "defeat" : "victory";
        e.priority = 4;
        break;
    case CombatEvent::AttackStarted:
        if (o.kind == rules::AttackKind::Melee && o.spell == rules::SpellKind::Count) {
            e.cue = o.enemyAttack                                    ? "attack"
                    : o.damageType == rules::DamageType::Bludgeoning ? "weapon.heavy"
                                                                     : "weapon.swing";
            e.creature = o.enemyAttack;
        }
        break;
    case CombatEvent::ProjectileReleased:
        e.cue = o.spell == rules::SpellKind::Count ? "weapon.release" : spellFamily(o.spell);
        break;
    case CombatEvent::Cast:
        e.cue = spellFamily(o.spell);
        break;
    case CombatEvent::Attack:
        e.positional = !o.enemyAttack;
        e.cue = !o.hit                                           ? "weapon.miss"
                : o.spell != rules::SpellKind::Count             ? spellFamily(o.spell)
                : o.kind != rules::AttackKind::Melee             ? "weapon.arrow_impact"
                : o.damageType == rules::DamageType::Bludgeoning ? "weapon.blunt"
                : o.damageType == rules::DamageType::Slashing    ? "weapon.slash"
                                                                 : "weapon.pierce";
        emit(e);
        if (o.damage > 0) {
            e.cue = o.enemyAttack ? (o.died     ? "party.death"
                                     : o.downed ? "party.down"
                                                : "party.hurt")
                                  : (o.died ? "death" : "hurt");
            e.creature = !o.enemyAttack;
            e.priority = (o.died || o.downed) ? 3 : 2;
            emit(e);
        }
        return;
    case CombatEvent::DeathSave:
        if (o.died) {
            e.cue = "party.death";
            e.priority = 3;
        }
        break;
    case CombatEvent::Healing:
    case CombatEvent::Aid:
        e.cue = o.item.index != UINT16_MAX ? "potion" : "healing";
        break;
    case CombatEvent::Recovery:
        e.cue = o.item.index != UINT16_MAX ? "potion" : "recovery";
        e.interface = true;
        break;
    case CombatEvent::Effect:
        if (o.effect.notice == EffectNotice::Applied || o.effect.notice == EffectNotice::Ended)
            e.cue = "effect.change";
        break;
    default:
        break;
    }
    emit(e);
}
} // namespace blobber::audio
