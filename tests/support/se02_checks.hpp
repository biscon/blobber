#pragma once
#include "game/commands.hpp"
#include "support/progression_fixtures.hpp"
#include "ui/portrait_feedback.hpp"
#include <cstring>
namespace blobber::test {
inline void se02Commands() {
    using K = rules::SpellKind;
    const auto map = hybridRoom();
    for (K kind : {K::MagicMissile, K::ScorchingRay, K::GuidingBolt, K::SacredFlame, K::ShockingGrasp,
                   K::InflictWounds, K::AcidSplash, K::PoisonSpray, K::Thunderwave, K::Shatter}) {
        const bool close = kind == K::ShockingGrasp || kind == K::InflictWounds || kind == K::PoisonSpray ||
                           kind == K::Thunderwave;
        const bool advanced = kind == K::ScorchingRay || kind == K::Shatter;
        const auto catalog = advanced ? creationCatalog(true) : spellCatalog();
        const auto places = spellPlacements(map, close ? 1 : 3, close);
        GameSession game(map, catalog, places, 5, spellSpawn, false);
        if (advanced)
            levelParty(game, 3, kind);
        if (kind == rules::SpellKind::ShockingGrasp) {
            game.party().lockFormation(false);
            game.party().swapFormation(FormationSlot::FrontRight, FormationSlot::BackRight);
            game.party().lockFormation(true);
        }
        const unsigned caster = rules::spellRules(kind)->casting == rules::Casting::Wisdom ? 1 : 3;
        if (close)
            spellReady(game, caster);
        auto command = spellCommand(game, kind);
        if (kind == K::Shatter)
            command.aimedCell = Cell{6, 4};
        else if (kind != K::Thunderwave) {
            command.targetCount = kind == K::MagicMissile || kind == K::ScorchingRay ? 3
                                  : kind == K::AcidSplash                            ? 2
                                                                                     : 1;
            for (unsigned i = 0; i < command.targetCount; ++i) {
                command.enemies[i] = game.actors().id(i);
                command.darts[i] = 1;
            }
        }
        const auto now = game.timeMs();
        const int mana = game.party().character(command.character)->mana;
        allocations = 0;
        watching = true;
        const auto result = game.submit(command);
        game.update(.7, false);
        watching = false;
        check(!allocations, "SE02 command and resolution allocate nothing");
        check(result.reason == CommandReason::Resolved,
              (std::string(rules::spellRules(kind)->id) + ": " + commandMessage(result.reason)).c_str());
        check(game.party().character(command.character)->mana ==
                  mana - rules::spellManaCost(rules::spellRules(kind)->level),
              "SE02 keeps existing mana costs");
        unsigned count = 0;
        for (const auto& c : game.combat().spellVisuals().casts)
            if (c.id.serial) {
                ++count;
                check(c.spell == kind, "Journal publishes intended spell");
                if (kind == K::MagicMissile || kind == K::ScorchingRay) {
                    check(c.deliveryCount == 3 && c.count == 3,
                          "One cast contains all three independently linked deliveries");
                    for (unsigned i = 0; i < 3; ++i) {
                        check(c.deliveries[i].attack.serial && c.deliveries[i].stage == AttackStage::Resolved,
                              "Extra delivery resolves with its own attack identity");
                        check(c.targets[i].actor.slot == i, "Recipient assignment preserved");
                    }
                }
                if (kind == K::ShockingGrasp || kind == K::InflictWounds)
                    check(c.deliveries[0].impactMs == now + 200, "Contact preserves existing 200ms impact");
                if (kind == K::Shatter) {
                    check(!c.includeOrigin && c.center == Cell{6, 4},
                          "Shatter clips at aimed area without caster corridor");
                    check(std::count(c.eligible.begin(), c.eligible.end(), true) == 9 && c.count == 3,
                          "Shatter publishes all empty and occupied eligible cells once");
                }
                if (kind == K::Thunderwave)
                    check(std::count(c.eligible.begin(), c.eligible.end(), true) == 3,
                          "Thunderwave retains front footprint");
            }
        check(count == 1, "Exactly one journal cast per accepted SE02 command");
    }
}
inline void se02Journal() {
    SpellVisualJournal journal;
    journal.clear(7);
    AttackVisual a;
    a.id = {7, 10};
    a.spell = rules::SpellKind::ScorchingRay;
    a.stage = AttackStage::Flight;
    a.releaseMs = 100;
    a.impactMs = 300;
    SpellTargetVisual t;
    t.actor = {7, 0};
    t.hit = true;
    t.damage = 4;
    journal.attack(a, t);
    auto b = a;
    b.id.serial = 11;
    b.impactMs = 450;
    journal.attack(b, t, a.id);
    journal.resolve(b.id, t, 450);
    auto miss = t;
    miss.hit = false;
    miss.damage = 0;
    journal.resolve(a.id, miss, 300);
    const auto& c = journal.snapshot().casts[0];
    check(c.deliveryCount == 2 && c.stage == AttackStage::Resolved && c.impactMs == 450,
          "Out-of-order outcomes retain latest cast deadline");
    check(!c.targets[0].hit && c.targets[1].hit && c.deliveries[0].impactMs == 300,
          "Same target rays retain distinct hit/miss and timestamps");
    journal.resolve(a.id, {}, 300, true);
    check(journal.snapshot().casts[0].deliveries[1].stage == AttackStage::Resolved,
          "Cancellation does not erase another ray result");
    auto missile = a;
    missile.id.serial = 20;
    missile.spell = rules::SpellKind::MagicMissile;
    missile.darts = 3;
    journal.attack(missile, t);
    check(journal.snapshot().casts[1].deliveries[0].darts == 3,
          "One target retains three assigned force darts");
}
inline void se02Portraits() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map, 1);
    GameSession game(map, catalog, places, 5, spellSpawn, false);
    CommandController commands(game);
    CombatOutcome healing;
    healing.event = CombatEvent::Healing;
    healing.character = {1};
    healing.recipient = {0};
    healing.damage = 4;
    healing.spell = rules::SpellKind::CureWounds;
    healing.hit = true;
    commands.record(healing);
    auto& state = commands.snapshot();
    check(state.portraitFeedback[0][0].kind == PortraitFeedbackKind::Healing,
          "Healing uses supportive shimmer");
    CombatOutcome damage;
    damage.event = CombatEvent::Attack;
    damage.enemyAttack = true;
    damage.enemy = game.actors().id(0);
    damage.recipient = {0};
    damage.spell = rules::SpellKind::SacredFlame;
    damage.damageType = rules::DamageType::Radiant;
    damage.damage = 3;
    damage.hit = true;
    commands.record(damage);
    check(state.portraitFeedback[0][0].kind == PortraitFeedbackKind::Healing &&
              state.portraitFeedback[0][1].kind == PortraitFeedbackKind::Damage,
          "Damage and healing retain separate channels");
    game.party().lockFormation(false);
    game.party().swapFormation(FormationSlot::FrontLeft, FormationSlot::BackLeft);
    commands.refresh({}, false, false, true);
    check(state.cards[2].id.index == 0 && state.portraitFeedback[0][0].kind == PortraitFeedbackKind::Healing,
          "Portrait feedback follows character identity after formation changes");
    const auto initial = portraitFeedbackSample(state.portraitFeedback[0][1], state.presentationMs);
    commands.refresh({}, false, false, true);
    check(portraitFeedbackSample(state.portraitFeedback[0][1], state.presentationMs).frame == initial.frame,
          "Paused repeated HUD refresh does not restart or advance portrait effect");
    damage.hit = false;
    damage.damage = 0;
    commands.record(damage);
    check(state.portraitFeedback[0][1].kind == PortraitFeedbackKind::Negated,
          "Save does not flash damaging hit");
    bool saved = false;
    for (const auto& f : state.feedback)
        if (f.active && f.party && f.text && !std::strcmp(f.text, "SAVED"))
            saved = true;
    check(saved, "Small save outcome remains available");
    CombatOutcome buff;
    buff.event = CombatEvent::Effect;
    buff.effect.notice = EffectNotice::Applied;
    buff.effect.kind = EffectKind::Bless;
    buff.effect.target = rules::CharacterId{0};
    commands.record(buff);
    check(state.portraitFeedback[0][0].kind == PortraitFeedbackKind::Protection,
          "Buff uses blue protection, not healing");
    for (const auto& f : state.feedback)
        if (f.active && f.party)
            check(!f.text || !*f.text, "Buff badge has no effect name");
    auto checkpoint = std::make_unique<FloorState>();
    game.exportFloor(*checkpoint);
    game.importFloor(*checkpoint);
    commands.refresh({}, false, false, true);
    check(state.portraitFeedback[0][0].kind == PortraitFeedbackKind::None &&
              state.portraitFeedback[0][1].kind == PortraitFeedbackKind::None,
          "Same-world restore clears portrait bursts through the journal epoch");
    commands.record(buff);
    game.update(.7, false);
    commands.refresh({}, false, false, true);
    check(state.portraitFeedback[0][0].kind == PortraitFeedbackKind::None &&
              state.portraitFeedback[0][1].kind == PortraitFeedbackKind::None,
          "Portrait effects expire without steady buff animation");
    check(!portraitFeedbackSample({PortraitFeedbackKind::Healing, rules::DamageType::Radiant, 100, 640}, 740)
               .active,
          "Portrait expiry boundary is exact");
    check(!portraitFeedbackSample({PortraitFeedbackKind::Healing, rules::DamageType::Radiant, 100, 640}, 99)
               .active,
          "Future outcomes cannot render early");
}
} // namespace blobber::test
