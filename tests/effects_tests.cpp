#include "game/session.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/hybrid_fixtures.hpp"
#include <chrono>
#include <iostream>
#include <string_view>
using namespace blobber;
namespace {
EffectRequest request(std::uint64_t world, EffectKind kind, unsigned caster,
                      std::initializer_list<EffectTarget> targets, int dc = 1000) {
    EffectRequest r;
    r.world = world;
    r.kind = kind;
    r.caster = {static_cast<std::uint16_t>(caster)};
    r.difficulty = dc;
    for (const auto t : targets)
        r.targets[r.count++] = t;
    return r;
}
EffectRequest request(GameSession& s, EffectKind kind, unsigned caster,
                      std::initializer_list<EffectTarget> targets, int dc = 1000) {
    return request(s.actors().snapshot().world, kind, caster, targets, dc);
}
struct Fixture {
    Map map = hybridRoom();
    rules::Catalog catalog = hybridCatalog(false);
    Player pose{{6, 10}, Heading::North};
    NpcWorld placements{map,
                        pose.cell,
                        {"goblin"},
                        {{{0}, {5, 4}, {0, 0}, Heading::South},
                         {{0}, {6, 4}, {0, 0}, Heading::South},
                         {{0}, {7, 4}, {0, 0}, Heading::South}}};
    GameSession session{map, catalog, placements, 5, pose, false};
};
std::size_t notices(const CombatEvents& events, EffectNotice kind) {
    std::size_t n = 0;
    for (const auto& e : events)
        n += e.event == CombatEvent::Effect && e.effect.notice == kind;
    return n;
}
void lifecycle() {
    Fixture f;
    auto& s = f.session;
    const rules::CharacterId wizard{3}, fighter{0};
    const auto base = s.party().character(wizard)->stats.armorClass;
    const auto mage = s.applyEffect(request(s, EffectKind::MageArmor, 3, {wizard}));
    check(bool(mage) && mage.id.serial && s.effects().armorClass(wizard) == base + 3,
          "Mage Armor changes effective, not base, armor");
    check(s.party().character(wizard)->stats.armorClass == base, "Base stats remain intact");
    check(s.applyEffect(request(s, EffectKind::MageArmor, 3, {fighter})).error == EffectError::InvalidTargets,
          "Armored target rejects Mage Armor");
    const auto shield = s.applyEffect(request(s, EffectKind::ShieldOfFaith, 1, {wizard}));
    check(bool(shield) && s.effects().armorClass(wizard) == base + 5, "Different armor effects combine");
    const auto second = s.applyEffect(request(s, EffectKind::ShieldOfFaith, 0, {wizard}));
    check(bool(second) && s.effects().armorClass(wizard) == base + 5, "Same-kind effects do not stack");
    check(bool(s.removeEffect(shield.id)) && s.effects().armorClass(wizard) == base + 5,
          "Independent duplicate survives removal");
    const auto replaced = s.applyEffect(request(s, EffectKind::Bless, 0, {fighter, wizard}));
    check(bool(replaced) && !s.effects().find(second.id) && s.effects().armorClass(wizard) == base + 3,
          "New concentration replaces whole old instance");
    auto bad = request(s, EffectKind::Bane, 0, {s.actors().id(0), s.actors().id(0)});
    check(s.applyEffect(bad).error == EffectError::InvalidTargets && s.effects().find(replaced.id),
          "Rejected replacement preserves old concentration");
    s.update(59.999, false);
    check(s.effects().find(replaced.id), "Bless survives immediately before expiry");
    const auto before = s.timeMs();
    s.update(60, true);
    check(s.timeMs() == before && s.effects().find(replaced.id), "Pause freezes effects");
    check(notices(s.update(.001, false), EffectNotice::Ended) == 1 && !s.effects().find(replaced.id),
          "Bless expires exactly at gameplay deadline");
    check(!s.events().step.effect.id.serial && notices(s.update(0, false), EffectNotice::Ended) == 0,
          "Expiry does not replay or become automatic step");
    check(s.effects().find(mage.id), "Long-lived armor survives shorter effects");
    s.party().damage(wizard, 999);
    s.update(0, false);
    check(!s.effects().find(mage.id), "Recipient death removes non-concentration armor");
    check(s.removeEffect(mage.id).error == EffectError::InvalidHandle, "Ended ID rejects removal");
    GameSession restarted(f.map, f.catalog, f.placements, 5, f.pose, false);
    check(restarted.removeEffect(mage.id).error == EffectError::InvalidHandle, "Restart invalidates IDs");
    auto foreign = request(s, EffectKind::Bless, 0, {fighter});
    check(restarted.applyEffect(foreign).error == EffectError::InvalidWorld,
          "Party requests carry world identity");
}
void capacityAndCleanup() {
    Fixture f;
    auto& s = f.session;
    const rules::CharacterId wizard{3}, fighter{0};
    const auto held = s.applyEffect(request(s, EffectKind::Bless, 0, {wizard}));
    for (std::size_t i = 1; i < EffectController::capacity; ++i)
        check(bool(s.applyEffect(request(s, EffectKind::MageArmor, 3, {wizard}))), "Fill bounded effects");
    check(s.applyEffect(request(s, EffectKind::ShieldOfFaith, 1, {fighter})).error == EffectError::Capacity,
          "Full storage rejects without evicting effects");
    const auto replacement = s.applyEffect(request(s, EffectKind::ShieldOfFaith, 0, {fighter}));
    check(bool(replacement) && !s.effects().find(held.id), "Full storage reuses own concentration slot");
    s.party().damage(wizard, 999);
    s.update(0, false);
    check(notices(s.events(), EffectNotice::RecipientRemoved) == 63 &&
              notices(s.events(), EffectNotice::Ended) == 63,
          "Maximum duplicate cleanup fits event storage");
    const auto next = s.applyEffect(request(s, EffectKind::Bless, 1, {fighter}));
    check(bool(next) && !s.effects().find(held.id), "Reused slot never revives stale ID");
    s.party().damage({1}, 1);
    s.update(0, false);
    check(s.effects().find(next.id), "Raw health setter is not a combat damage notification");
    s.party().damage({1}, 999);
    s.update(0, false);
    check(!s.effects().find(next.id), "Dead caster loses concentration on reconciliation");
}
void repeatSavesAndMovement() {
    Fixture f;
    auto& s = f.session;
    const auto enemy = s.actors().id(0);
    const auto held = s.applyEffect(request(s, EffectKind::HoldPerson, 3, {enemy}));
    check(bool(held) && s.effects().has(enemy, EffectKind::HoldPerson), "Failed initial save holds NPC");
    check(notices(s.update(5.999, false), EffectNotice::RepeatSave) == 0, "No early repeat save");
    const auto& at = s.update(.001, false);
    check(notices(at, EffectNotice::RepeatSave) == 1, "Real-time retry at six seconds");
    for (const auto& e : at)
        if (e.event == CombatEvent::Effect)
            check(e.occurredMs == 6000 && e.effect.gameplayMs == 6000, "Retry carries both timestamps");
    check(notices(s.update(54, false), EffectNotice::RepeatSave) == 8 && !s.effects().find(held.id),
          "Large step drains retries through 54s, expiry wins 60s tie");
    const auto again = s.applyEffect(request(s, EffectKind::HoldPerson, 3, {enemy}));
    check(bool(again), "Hold again");
    s.actors().remove(enemy);
    s.update(0, false);
    check(!s.effects().find(again.id), "Removed actor ends final-recipient concentration");
    const auto partyHold = s.applyEffect(request(s, EffectKind::HoldPerson, 3, {rules::CharacterId{0}}));
    check(bool(partyHold) && !s.effects().canAct(rules::CharacterId{0}), "Held party member cannot act");
    check(s.combat().exposed(rules::CharacterId{2}), "Paralyzed front member stops protecting paired rear");
    const auto old = s.movement().player.cell;
    s.setAnimated(false);
    s.move(Action::Left);
    check(!(s.movement().player.cell == old), "Mobile allies carry paralyzed member");
    s.party().damage({3}, s.party().character({3})->hitPoints);
    s.update(0, false);
    check(!s.effects().find(partyHold.id) && s.effects().canAct(rules::CharacterId{0}),
          "Caster unconsciousness releases held member");
}
void turnRounds() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const Player pose{{3, 3}, Heading::North};
    const auto places = combatPlacements(map, pose, 2);
    GameSession s(map, catalog, places, 5, pose, false);
    const auto hold = s.applyEffect(request(s, EffectKind::HoldPerson, 3, {s.actors().id(0)}));
    check(bool(hold), "Apply turn-phase hold");
    s.update(0, false);
    for (int i = 0; i < 30 && !s.combat().snapshot().awaitingAction; ++i)
        s.update(.5, false);
    check(s.combat().snapshot().awaitingAction, "Reach player decision");
    check(notices(s.update(60, false), EffectNotice::RepeatSave) == 0 && s.effects().gameplayMs() == 0,
          "Decision and playback seconds do not grant saving throws");
    std::size_t retries = 0;
    for (int i = 0; i < 100 && s.effects().gameplayMs() < 6000; ++i) {
        if (s.combat().snapshot().awaitingAction)
            s.submit(turnCommand(s));
        const auto& events = s.update(.5, false);
        retries += notices(events, EffectNotice::RepeatSave);
        for (const auto& e : events)
            if (e.event == CombatEvent::AttackStarted)
                check(!e.enemyAttack || e.enemy.slot != 0, "Held NPC skips actions");
    }
    check(s.effects().gameplayMs() == 6000 && retries == 1, "One retry at completed full round");
    s.actors().remove(s.actors().id(1));
    s.update(0, false);
    check(s.combat().snapshot().phase != CombatPhase::Victory, "Living held NPC still prevents victory");
    s.actors().damage(s.actors().id(0), 9999, s.timeMs());
    for (int i = 0; i < 30 && s.combat().snapshot().phase != CombatPhase::Victory; ++i) {
        if (s.combat().snapshot().awaitingAction)
            s.submit(turnCommand(s));
        s.update(.5, false);
    }
    check(s.combat().snapshot().phase == CombatPhase::Victory, "Death of final held threat permits victory");
}
void savesAndConcentration() {
    const auto map = combatMap();
    const auto catalog = combatCatalog();
    const auto places = combatPlacements(map);
    Party party(catalog);
    ActorWorld actors(map, catalog, places);
    rules::DiceRng rng(7), oracle(7);
    EffectController effects(party, actors, rng);
    CombatEvents events;
    const auto world = actors.snapshot().world;
    check(bool(effects.apply(request(world, EffectKind::Bless, 0, {rules::CharacterId{0}}), 0, events)),
          "Bless caster");
    auto roll = oracle.d20().value;
    const auto bonus = oracle.roll({1, 4, 0}).value.total;
    const auto saved = effects.save(rules::CharacterId{0}, rules::Ability::Constitution, 12);
    check(saved.roll.selected == roll.selected &&
              saved.total == roll.selected + party.character({0})->stats.saves[2] + bonus,
          "Bless modifies ordinary save after d20");
    const auto bless = effects.concentration({0});
    events.clear();
    effects.damaged({0}, 0, 100, events);
    check(events.count == 0 && effects.find(bless), "Zero damage consumes no concentration roll");
    roll = oracle.d20().value;
    const auto concentrationBonus = oracle.roll({1, 4, 0}).value.total;
    effects.damaged({0}, 23, 200, events);
    check(notices(events, EffectNotice::ConcentrationSave) == 1, "Positive damage checks concentration once");
    check(events.values[0].effect.roll == roll.selected && events.values[0].effect.difficulty == 11 &&
              events.values[0].effect.total ==
                  roll.selected + party.character({0})->stats.saves[2] + concentrationBonus,
          "Concentration uses floor damage/2 and caster Bless");
    check(bool(effects.apply(request(world, EffectKind::HoldPerson, 3, {actors.id(0)}), 300, events)),
          "Hold actor");
    const auto before = rng;
    const auto automatic = effects.save(actors.id(0), rules::Ability::Dexterity, 1);
    check(!automatic.succeeded && automatic.roll.selected == 0, "Paralysis automatically fails Dex save");
    auto expected = before;
    check(rng.d20().value.selected == expected.d20().value.selected, "Automatic failure consumes no RNG");
    events.clear();
    check(bool(effects.apply(request(world, EffectKind::Bless, 1, {rules::CharacterId{3}}), 400, events)),
          "Bless downable target");
    party.damage({3}, party.character({3})->hitPoints);
    effects.reconcile(500, events);
    check(effects.has(rules::CharacterId{3}, EffectKind::Bless) && !effects.concentration({3}).serial,
          "Downed target retains external blessing but loses own concentration");
    rules::HealthState down{0, rules::Life::Dying, 0, 0};
    check(rules::deathSave(down, 10, 9, 1).state.successes == 1, "Bless can rescue death save nine");
    check(rules::deathSave(down, 10, 10, -1).state.failures == 1, "Bane can spoil death save ten");
    check(rules::deathSave(down, 10, 1, 100).state.failures == 2, "Natural one still fails twice");
    check(rules::deathSave(down, 10, 20, -100).state.hitPoints == 1, "Natural twenty still restores HP");
}
void initialSavesAndRng() {
    const auto map = combatMap();
    const auto catalog = combatCatalog();
    const auto places = combatPlacements(map);
    bool partial = false, all = false;
    for (unsigned seed = 0; seed < 40 && !(partial && all); ++seed) {
        Party party(catalog);
        ActorWorld actors(map, catalog, places);
        rules::DiceRng rng(seed), expected(seed);
        EffectController effects(party, actors, rng);
        CombatEvents events;
        auto r = request(actors.snapshot().world, EffectKind::Bane, 1,
                         {actors.id(2), actors.id(0), actors.id(1)}, 10);
        auto bad = r;
        bad.targets[1] = bad.targets[0];
        check(effects.apply(bad, 0, events).error == EffectError::InvalidTargets && events.count == 0,
              "Invalid recipients reject before RNG");
        std::size_t failures = 0;
        for (unsigned i = 0; i < 3; ++i)
            failures += expected.d20().value.selected - 1 < 10;
        const auto result = effects.apply(r, 0, events);
        check(bool(result) && notices(events, EffectNotice::InitialSave) == 3,
              "One initial save per distinct recipient");
        const auto* instance = effects.find(result.id);
        check(instance ? instance->count == failures : failures == 0, "Only failed recipients retained");
        check(rng.d20().value.selected == expected.d20().value.selected,
              "Rejected request spends no RNG; initial saves exactly once");
        partial |= failures > 0 && failures < 3;
        all |= failures == 0;
    }
    check(partial && all, "Fixed seeds cover partial and complete resistance");
}
rules::Catalog impactCatalog() {
    auto f = partyFixture();
    f.preset.characters[2].equipment.ranged = {2};
    rules::MonsterDefinition m;
    m.id = "archer";
    m.abilities.fill(10);
    m.armorClass = 1;
    m.hitPoints = 1000;
    m.attacks[0] = {0};
    m.attackCount = 1;
    return {{{"bow", rules::AttackKind::Ranged, 100, {1, 4, 0}, rules::DamageType::Piercing}},
            {m},
            f.races,
            f.classes,
            f.items,
            f.preset};
}
NpcTimings impactTiming() {
    NpcTimings t{};
    t[0] = {{600, 600, 0, 0}, 2};
    t[2] = {{700, 250, 250, 350}, 4};
    t[3] = {{150, 150, 0, 0}, 2};
    t[4] = {{200, 200, 200, 600}, 4};
    t[5] = {{600, 400, 0, 0}, 2};
    return t;
}
const AttackVisual& attack(const GameSession& s, CombatAttackId id) {
    for (const auto& v : s.combat().attacks().attacks)
        if (v.id.serial == id.serial && v.id.world == id.world)
            return v;
    throw std::runtime_error("Missing attack fixture");
}
void impactOrdering() {
    const auto map = hybridRoom();
    const auto catalog = impactCatalog();
    const Player pose{{5, 9}, Heading::North};
    const NpcWorld places(map, pose.cell, {"archer"}, {{{0}, {5, 5}, {0, 0}, Heading::South}});
    std::size_t checks = 0, breaks = 0, surviving = 0;
    for (unsigned seed = 0; seed < 12; ++seed) {
        GameSession large(map, catalog, places, seed, pose, true, {impactTiming()});
        GameSession split(map, catalog, places, seed, pose, true, {impactTiming()});
        for (auto* s : {&large, &split})
            for (unsigned i = 0; i < 4; ++i)
                check(bool(s->applyEffect(request(*s, EffectKind::Bless, i,
                                                  {rules::CharacterId{static_cast<std::uint16_t>(i)}}))),
                      "Bless all casters");
        const auto started = large.update(58.9, false).step;
        const auto other = split.update(58.9, false).step;
        check(started.event == CombatEvent::AttackStarted && other.recipient.index == started.recipient.index,
              "Deterministic late approach attack");
        const auto deadline = attack(large, started.attackId).impactMs;
        check(deadline < 60000, "Fixture impact precedes expiry");
        const auto hp = large.party().character(started.recipient)->hitPoints;
        const auto& whole = large.update(2, false);
        bool sawImpact = false;
        EffectEvent concentration{};
        CombatOutcome impact{};
        for (const auto& e : whole) {
            if (e.event == CombatEvent::Attack && e.attackId.serial == started.attackId.serial) {
                sawImpact = true;
                impact = e;
            }
            if (e.event == CombatEvent::Effect && e.effect.notice == EffectNotice::ConcentrationSave) {
                check(sawImpact && e.occurredMs == deadline && e.effect.gameplayMs == deadline,
                      "Earlier impact concentration check precedes later expiry, after damage event");
                concentration = e.effect;
                ++checks;
                breaks += !e.effect.succeeded;
                surviving += e.effect.succeeded;
            }
        }
        check(sawImpact && large.party().character(started.recipient)->hitPoints ==
                               std::max<std::int64_t>(0, hp - impact.damage),
              "Damage happens exactly once");
        split.update(double(deadline - split.timeMs() - 1) / 1000, false);
        check(split.party().character(started.recipient)->hitPoints == hp &&
                  split.effects().concentration(started.recipient).serial,
              "HP/concentration unchanged before impact");
        const auto& exact = split.update(.001, false);
        for (const auto& e : exact)
            if (e.event == CombatEvent::Effect && e.effect.notice == EffectNotice::ConcentrationSave)
                check(e.effect.roll == concentration.roll && e.effect.total == concentration.total,
                      "Large and split updates consume identical concentration dice");
        split.update(double(60900 - split.timeMs()) / 1000, false);
        check(large.party().character(started.recipient)->hitPoints ==
                  split.party().character(started.recipient)->hitPoints,
              "Subdividing interval preserves pending damage");
        check(notices(split.update(0, false), EffectNotice::ConcentrationSave) == 0,
              "No concentration replay");
    }
    check(checks > 0 && breaks > 0 && surviving > 0, "Fixed seeds cover concentration success/failure");
    GameSession expired(map, catalog, places, 5, pose, true, {impactTiming()});
    for (unsigned i = 0; i < 4; ++i)
        expired.applyEffect(
            request(expired, EffectKind::Bless, i, {rules::CharacterId{static_cast<std::uint16_t>(i)}}));
    const auto started = expired.update(59.7, false).step;
    check(attack(expired, started.attackId).impactMs > 60000, "Fixture arrival after expiry");
    check(notices(expired.update(2, false), EffectNotice::ConcentrationSave) == 0,
          "Already expired concentration incurs no damage save");
}
void paralysisAndFlight() {
    const auto map = hybridRoom();
    const auto catalog = impactCatalog();
    const Player pose{{5, 9}, Heading::North};
    const NpcWorld places(map, pose.cell, {"archer"}, {{{0}, {5, 5}, {0, 0}, Heading::South}});
    for (const bool released : {false, true}) {
        GameSession s(map, catalog, places, 5, pose, true, {impactTiming()});
        const auto started = s.update(.1, false).step;
        const auto initial = attack(s, started.attackId);
        if (released)
            s.update(double(initial.releaseMs - s.timeMs()) / 1000, false);
        const auto held = s.applyEffect(request(s, EffectKind::HoldPerson, 3, {s.actors().id(0)}));
        check(bool(held), "Paralyze enemy attacker");
        const auto stage = attack(s, started.attackId).stage;
        check(stage == (released ? AttackStage::Flight : AttackStage::Canceled),
              "Paralysis cancels windup but preserves released arrows");
        const auto hp = s.party().character(started.recipient)->hitPoints;
        const auto& events = s.update(1.5, false);
        bool hit = false;
        for (const auto& e : events)
            hit |= e.event == CombatEvent::Attack && e.attackId.serial == started.attackId.serial;
        check(hit == released && (released || s.party().character(started.recipient)->hitPoints == hp),
              "Canceled windup causes no late damage");
        const auto cell = s.actors().actor(s.actors().id(0))->npc.cell;
        s.update(5, false);
        check(s.actors().actor(s.actors().id(0))->npc.cell == cell && s.actors().occupied(cell),
              "Held actor neither moves nor releases occupancy");
    }
    // Accepted attack bonuses survive removal while the projectile is flying.
    GameSession kept(map, catalog, places, 12, pose, false, {impactTiming()});
    GameSession removed(map, catalog, places, 12, pose, false, {impactTiming()});
    const auto blessA = kept.applyEffect(request(kept, EffectKind::Bless, 3, {rules::CharacterId{2}}));
    const auto blessB = removed.applyEffect(request(removed, EffectKind::Bless, 3, {rules::CharacterId{2}}));
    const auto shoot = [](GameSession& s) {
        auto c = turnCommand(s, PartyAction::Ranged);
        c.character = {2};
        c.target = s.actors().id(0);
        return s.submit(c);
    };
    const auto a = shoot(kept), b = shoot(removed);
    check(a.event == CombatEvent::AttackStarted && b.event == CombatEvent::AttackStarted && bool(blessA),
          "Accept blessed shots");
    removed.removeEffect(blessB.id);
    const auto& ea = kept.update(.5, false);
    const auto& eb = removed.update(.5, false);
    CombatOutcome ra, rb;
    for (const auto& e : ea)
        if (e.event == CombatEvent::Attack && !e.enemyAttack)
            ra = e;
    for (const auto& e : eb)
        if (e.event == CombatEvent::Attack && !e.enemyAttack)
            rb = e;
    check(ra.event == CombatEvent::Attack && rb.event == CombatEvent::Attack && ra.roll == rb.roll &&
              ra.hit == rb.hit && ra.damage == rb.damage,
          "Removal in flight never rerolls accepted attack");
}

void longDurationsAndValidation() {
    const auto map = combatMap();
    const auto catalog = combatCatalog();
    const auto places = combatPlacements(map);
    Party party(catalog);
    ActorWorld actors(map, catalog, places);
    rules::DiceRng rng(19), oracle(19);
    EffectController effects(party, actors, rng);
    CombatEvents events;
    const auto world = actors.snapshot().world;
    auto r = request(world, EffectKind::Bless, 0, {rules::CharacterId{3}});
    for (const auto kind : {EffectKind::Count, static_cast<EffectKind>(255)}) {
        r.kind = kind;
        check(effects.apply(r, 0, events).error == EffectError::InvalidKind, "Reject unknown kind");
    }
    r.kind = EffectKind::Bless;
    r.difficulty = 0;
    check(effects.apply(r, 0, events).error == EffectError::InvalidDifficulty, "Reject invalid DC");
    r.difficulty = 10;
    r.count = 0;
    check(effects.apply(r, 0, events).error == EffectError::InvalidTargets, "Reject empty recipients");
    r.count = 4;
    check(effects.apply(r, 0, events).error == EffectError::InvalidTargets,
          "Reject count before reading array");
    r.count = 1;
    r.targets[0] = rules::CharacterId{99};
    check(effects.apply(r, 0, events).error == EffectError::InvalidTargets, "Reject invalid party ID");
    r.targets[0] = ActorId{0, world + 1};
    check(effects.apply(r, 0, events).error == EffectError::InvalidTargets, "Reject foreign NPC ID");
    check(rng.d20().value.selected == oracle.d20().value.selected && events.count == 0,
          "Validation neither rolls nor emits consequences");
    const auto mage =
        effects.apply(request(world, EffectKind::MageArmor, 0, {rules::CharacterId{3}}), 0, events);
    const auto shield =
        effects.apply(request(world, EffectKind::ShieldOfFaith, 1, {rules::CharacterId{3}}), 0, events);
    party.damage({0}, 999);
    effects.reconcile(1, events);
    check(effects.find(mage.id), "Non-concentration survives caster death with living recipient");
    effects.advance(599999, 599999, true, events);
    check(effects.find(shield.id), "Shield survives just before ten minutes");
    effects.advance(600000, 600000, true, events);
    check(!effects.find(shield.id) && effects.find(mage.id),
          "Ten-minute boundary does not expire eight-hour armor");
    effects.advance(28799999, 28799999, true, events);
    check(effects.find(mage.id), "Mage Armor survives just before eight hours");
    effects.advance(28800000, 28800000, true, events);
    check(!effects.find(mage.id), "Mage Armor expires at eight gameplay hours");
}
void paralysisFacingAndProtection() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100}, 1);
    const Player pose{{3, 3}, Heading::North};
    const NpcWorld places(map, pose.cell, {"goblin"},
                          {{{0}, {3, 2}, {0, 1}, Heading::West},
                           {{0}, {3, 2}, {1, 1}, Heading::South},
                           {{0}, {3, 2}, {0, 0}, Heading::South}});
    GameSession s(map, catalog, places, 5, pose, true, {impactTiming()});
    const auto front = s.actors().id(0), rear = s.actors().id(2);
    check(!s.combat().exposed(rear), "Paired front originally protects rear NPC");
    check(bool(s.applyEffect(request(s, EffectKind::HoldPerson, 3, {front}))), "Hold front NPC");
    check(s.combat().exposed(rear), "Held front NPC stops protecting paired rear");
    bool rearAttacks = false, partyCritical = false;
    for (int i = 0; i < 160 && !(rearAttacks && partyCritical); ++i) {
        if (s.combat().snapshot().awaitingAction) {
            auto c = turnCommand(s);
            if (s.combat().exposed(c.character)) {
                c.action = PartyAction::Melee;
                c.target = front;
            }
            s.submit(c);
        }
        const auto& events = s.update(.1, false);
        check(s.actors().actor(front)->npc.facing == Heading::West, "Held NPC cannot turn during contact/AI");
        for (const auto& e : events) {
            rearAttacks |=
                e.event == CombatEvent::AttackStarted && e.enemyAttack && e.enemy.slot == rear.slot;
            if (e.event == CombatEvent::Attack && !e.enemyAttack && e.enemy.slot == front.slot && e.hit) {
                check(e.critical, "Successful adjacent attacks on paralysis are critical");
                partyCritical = true;
            }
        }
    }
    check(rearAttacks && partyCritical,
          "Rear attacks without overlapping held front, party gets close critical");
    check(s.actors().actor(rear)->npc.subcell == Cell{0, 0} && s.actors().occupiedSlots({3, 2}) == 13,
          "Paralysis changes exposure without moving bodies or releasing occupancy");
}
void victoryAndPhasePersistence() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const Player pose{{3, 3}, Heading::North};
    const auto places = combatPlacements(map, pose, 1);
    GameSession s(map, catalog, places, 5, pose, false);
    const auto mage = s.applyEffect(request(s, EffectKind::MageArmor, 3, {rules::CharacterId{3}}));
    const auto bless = s.applyEffect(request(s, EffectKind::Bless, 1, {rules::CharacterId{0}}));
    s.update(0, false);
    const auto gameplay = s.effects().gameplayMs();
    s.actors().damage(s.actors().id(0), 9999, s.timeMs());
    for (int i = 0; i < 30 && s.combat().snapshot().phase != CombatPhase::Victory; ++i) {
        if (s.combat().snapshot().awaitingAction)
            s.submit(turnCommand(s));
        s.update(.5, false);
    }
    check(s.combat().snapshot().phase == CombatPhase::Victory, "Enter victory");
    s.update(60, false);
    check(s.effects().gameplayMs() == gameplay && s.effects().find(bless.id) && s.effects().find(mage.id),
          "Victory and partial round neither expire nor clear retained effects");
    check(s.combat().continueExploring(), "Continue after victory");
    s.update(60, false);
    check(!s.effects().find(bless.id) && s.effects().find(mage.id), "Continue resumes gameplay deadlines");
    // A self-hold cannot create a permanently paralyzed caster: it immediately
    // breaks its own concentration. All-paralyzed concentration is unreachable
    // with the approved party-only caster scope.
    const auto self = s.applyEffect(request(s, EffectKind::HoldPerson, 3, {rules::CharacterId{3}}));
    check(bool(self) && !s.effects().find(self.id) && s.effects().mobileParty(),
          "Self-incapacitation breaks concentration immediately without deadlock");
}

void stackedRolls() {
    const auto map = combatMap();
    const auto catalog = combatCatalog();
    const auto places = combatPlacements(map);
    Party party(catalog);
    ActorWorld actors(map, catalog, places);
    rules::DiceRng rng(17);
    EffectController effects(party, actors, rng);
    CombatEvents events;
    const auto world = actors.snapshot().world;
    const rules::CharacterId target{0};
    effects.apply(request(world, EffectKind::Bless, 1, {target}), 0, events);
    effects.apply(request(world, EffectKind::Bless, 2, {target}), 0, events);
    effects.apply(request(world, EffectKind::Bane, 3, {target}), 0, events);
    auto oracle = rng;
    const auto plus = oracle.roll({1, 4, 0}).value.total;
    const auto minus = oracle.roll({1, 4, 0}).value.total;
    check(effects.rollModifier(target) == plus - minus,
          "Duplicate Bless contributes once; Bane rolls separately");
    check(rng.d20().value.selected == oracle.d20().value.selected, "Stacking consumes exactly two d4 rolls");
    effects.apply(request(world, EffectKind::ShieldOfFaith, 0, {target}), 0, events);
    oracle = rng;
    const auto face = oracle.d20().value.selected;
    const auto add = oracle.roll({1, 4, 0}).value.total;
    const auto subtract = oracle.roll({1, 4, 0}).value.total;
    events.clear();
    effects.damaged(target, 1, 100, events);
    check(events.values[0].effect.total == face + party.character(target)->stats.saves[2] + add - subtract,
          "Concentration saves use both Bless and Bane without duplicate stacking");
}

void trace() {
    const auto print = [](const GameSession& s) {
        constexpr const char* names[]{
            "applied", "initial save", "repeat save", "concentration save", "recipient removed", "ended"};
        constexpr const char* kinds[]{"Bless", "Bane", "Shield of Faith", "Mage Armor", "Hold Person"};
        constexpr const char* reasons[]{
            "none",  "expired",    "replaced",       "dismissed",     "incapacitated", "failed concentration",
            "saved", "no targets", "invalid target", "armor equipped"};
        for (const auto& e : s.events()) {
            if (e.event == CombatEvent::Effect)
                std::cout << "session_ms=" << e.occurredMs << " gameplay_ms=" << e.effect.gameplayMs << ' '
                          << kinds[static_cast<unsigned>(e.effect.kind)] << " #" << e.effect.id.serial << ' '
                          << names[static_cast<unsigned>(e.effect.notice)]
                          << " caster=" << e.effect.caster.index << " d20=" << e.effect.roll
                          << " total=" << e.effect.total << " dc=" << e.effect.difficulty
                          << " success=" << e.effect.succeeded
                          << " reason=" << reasons[static_cast<unsigned>(e.effect.reason)] << '\n';
            else if (e.event == CombatEvent::Attack)
                std::cout << "session_ms=" << e.occurredMs << " impact damage=" << e.damage
                          << " recipient=" << e.recipient.index << '\n';
            else if (e.event == CombatEvent::Round)
                std::cout << "Full round completed; gameplay_ms=" << s.effects().gameplayMs() << '\n';
        }
        std::cout << "  session_ms=" << s.timeMs() << " gameplay_ms=" << s.effects().gameplayMs()
                  << " wizard AC=" << s.effects().armorClass(rules::CharacterId{3})
                  << " HP=" << s.party().character({3})->hitPoints
                  << " concentration=" << s.effects().concentration({3}).serial << '\n';
    };
    std::cout << "REAL TIME AND PAUSE\n";
    Fixture f;
    auto& s = f.session;
    s.applyEffect(request(s, EffectKind::MageArmor, 3, {rules::CharacterId{3}}));
    print(s);
    s.applyEffect(request(s, EffectKind::ShieldOfFaith, 1, {rules::CharacterId{3}}));
    print(s);
    s.applyEffect(request(s, EffectKind::Bane, 0, {s.actors().id(0)}));
    print(s);
    s.applyEffect(request(s, EffectKind::HoldPerson, 3, {s.actors().id(0)}, 14));
    print(s);
    s.update(6, false);
    print(s);
    std::cout << "Pause for 60 seconds:\n";
    s.update(60, true);
    print(s);
    s.update(54, false);
    print(s);
    std::cout << "TURN-BASED: WAITING VERSUS COMPLETING A ROUND\n";
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const Player pose{{3, 3}, Heading::North};
    const auto places = combatPlacements(map, pose, 1);
    GameSession turns(map, catalog, places, 5, pose, false);
    turns.applyEffect(request(turns, EffectKind::HoldPerson, 3, {turns.actors().id(0)}));
    print(turns);
    for (int i = 0; i < 30 && !turns.combat().snapshot().awaitingAction; ++i)
        turns.update(.5, false);
    std::cout << "Wait 60 seconds at player decision:\n";
    turns.update(60, false);
    print(turns);
    while (turns.effects().gameplayMs() < 6000) {
        if (turns.combat().snapshot().awaitingAction)
            turns.submit(turnCommand(turns));
        turns.update(.5, false);
        if (notices(turns.events(), EffectNotice::RepeatSave))
            print(turns);
    }
    std::cout << "CONCENTRATION AT IMPACT\n";
    const auto room = hybridRoom();
    const auto ranged = impactCatalog();
    const Player archerPose{{5, 9}, Heading::North};
    const NpcWorld archerPlaces(room, archerPose.cell, {"archer"}, {{{0}, {5, 5}, {0, 0}, Heading::South}});
    GameSession hit(room, ranged, archerPlaces, 5, archerPose, true, {impactTiming()});
    const auto started = hit.update(.1, false).step;
    hit.applyEffect(request(hit, EffectKind::Bless, started.recipient.index, {started.recipient}));
    print(hit);
    const auto deadline = attack(hit, started.attackId).impactMs;
    hit.update(double(deadline - hit.timeMs() - 1) / 1000, false);
    print(hit);
    hit.update(.001, false);
    print(hit);
    std::cout << "Storage bytes: EffectController=" << sizeof(EffectController)
              << " CombatEvents=" << sizeof(CombatEvents) << '\n';
}
void stress() {
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 100; ++i)
        capacityAndCleanup();
    const auto ms =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << "100 full-capacity application/replacement/cleanup cycles: " << ms << " ms\n";
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--stress") {
            stress();
            return 0;
        }
        lifecycle();
        capacityAndCleanup();
        repeatSavesAndMovement();
        turnRounds();
        savesAndConcentration();
        initialSavesAndRng();
        impactOrdering();
        paralysisAndFlight();
        longDurationsAndValidation();
        paralysisFacingAndProtection();
        victoryAndPhasePersistence();
        stackedRolls();
        if (argc == 2 && std::string_view(argv[1]) == "--trace")
            trace();
        std::cout << "Effects checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
