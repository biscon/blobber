#include "game/commands.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include <iostream>
#include <sstream>
using namespace blobber;
using namespace blobber::rules;
namespace {
constexpr Player spawn{{3, 4}, Heading::North};
Catalog catalog() {
    auto f = partyFixture();
    auto& archer = f.preset.characters[0];
    archer.equipment.offHand = {};
    const auto slot = archer.itemCount++;
    archer.items[slot] = {{3}, 1};
    archer.equipment.ranged = {std::uint16_t(slot)};
    archer.items[archer.itemCount++] = {{9}, 20};
    MonsterDefinition shaman;
    shaman.id = "goblin_shaman";
    shaman.abilities = {10, 10, 10, 10, 14, 11};
    shaman.hitPoints = 100;
    shaman.armorClass = 10;
    shaman.attacks[0] = {0};
    shaman.attackCount = 1;
    shaman.humanoid = true;
    shaman.spellcasting = EnemyCasting::AcolyteSupport;
    auto ally = shaman;
    ally.id = "goblin";
    ally.spellcasting = EnemyCasting::None;
    return {{{"club", AttackKind::Melee, -100, {1, 4, -100}, DamageType::Bludgeoning}},
            {shaman, ally},
            f.races,
            f.classes,
            f.items,
            f.preset};
}
NpcTimings timings() {
    NpcTimings t{};
    t[0] = {{{600, 600}}, 2};
    t[2] = {{{700, 250, 250, 350}}, 4};
    t[3] = {{{150, 150}}, 2};
    t[5] = {{{600, 400}}, 2};
    t[6] = {{{1000}}, 1};
    return t;
}
struct Fixture {
    Map map = combatMap();
    Catalog cat = catalog();
    NpcWorld npcs;
    GameSession game;
    Fixture(bool support = false, unsigned seed = 8, bool far = false, bool ai = false)
        : npcs(map, spawn.cell, {"goblin_shaman", "goblin"},
               support
                   ? std::vector<NpcInstance>{{{0}, far ? Cell{3, 1} : Cell{3, 3}, {0, 0}, Heading::South},
                                              {{1}, far ? Cell{3, 2} : Cell{3, 3}, {0, 1}, Heading::South}}
                   : std::vector<NpcInstance>{{{0}, far ? Cell{3, 1} : Cell{3, 3}, {0, 0}, Heading::South}}),
          game(map, cat, npcs, seed, spawn, ai, {timings(), timings()}) {}
    ActorId caster() const { return game.actors().id(0); }
    EffectController& effects() { return const_cast<EffectController&>(game.effects()); }
    CombatOutcome cast() {
        for (unsigned n = 0; n < 150; ++n) {
            if (game.combat().snapshot().awaitingAction)
                game.submit(turnCommand(game));
            watching = true;
            allocations = 0;
            const auto step = game.update(.05, false).step;
            watching = false;
            check(!allocations, "NPC policy and cast acceptance allocate no engine memory");
            if (step.event == CombatEvent::Cast && step.enemyAttack)
                return step;
        }
        check(false, "Fixture reaches NPC cast");
        return {};
    }
};
void supportAndTimeline() {
    Fixture f(true);
    const auto out = f.cast();
    check(out.spell == SpellKind::Bless && !out.hit && !out.damage && !out.roll && !out.saveDifficulty,
          "Accepted Bless announces identity without saved results");
    check(f.game.combat().enemyMana(f.caster()) == 4 && !f.effects().has(f.caster(), EffectKind::Bless),
          "Mana charged once, Bless absent before release");
    check(!f.effects().concentrationSource(f.caster()).serial, "Reservation is not concentration");
    const auto time = f.game.timeMs();
    f.game.update(30, true);
    check(f.game.timeMs() == time, "Pause freezes casting and resources");
    watching = true;
    allocations = 0;
    f.game.update(.599, false);
    const bool before = f.effects().has(f.caster(), EffectKind::Bless);
    const auto& released = f.game.update(.001, false);
    watching = false;
    check(!allocations && !before && f.effects().has(f.caster(), EffectKind::Bless) &&
              f.effects().has(f.game.actors().id(1), EffectKind::Bless),
          "Release commits reserved ally/self Bless at 600ms without allocation");
    unsigned applied = 0;
    for (const auto& e : released)
        if (e.event == CombatEvent::Effect && e.effect.notice == EffectNotice::Applied) {
            ++applied;
            check(e.effect.caster.npc && e.effect.caster.enemy.slot == 0,
                  "Lifecycle has actor caster identity");
        }
    check(applied == 2, "Exactly two recipients, no fabricated party recipient");
    CommandController ui(f.game);
    ui.record(out);
    check(ui.snapshot().enemyPortraits[0] == EnemyPortrait::Shaman &&
              std::string(ui.snapshot().log.newest(0)).find("#1 begins Bless") != std::string::npos,
          "Native portrait discriminator and casting log identify the shaman actor");
    ui.record(released);
    ui.refresh(allCombatTargets(f.game), false, false, true);
    ui.hover(HoverKind::Enemy, 0, f.caster());
    check(std::string(ui.snapshot().hover.lines[4].data()).find("Concentrating: Bless") != std::string::npos,
          "Ordinary visible enemy hover identifies concentration");
    const auto next = f.cast();
    check(next.spell == SpellKind::SacredFlame && f.game.combat().enemyMana(f.caster()) == 4,
          "Concentrating shaman uses free cantrip without refilling mana");
}
void soloAndRotation() {
    Fixture f;
    std::array<bool, 4> selected{};
    for (unsigned i = 0; i < 4; ++i) {
        const auto out = f.cast();
        check(out.spell == SpellKind::SacredFlame && f.game.combat().enemyMana(f.caster()) == 6,
              "Solo shaman never self-buffs or spends cantrip mana");
        check(out.recipient.index == f.game.party().snapshot().formation[i].index,
              "Conscious party recipients rotate in formation order");
        selected[out.recipient.index] = true;
        auto& p = f.game.party();
        // Self-owned fixture keeps targets conscious through this policy-only test.
        for (std::uint16_t j = 0; j < 4; ++j)
            p.heal({j}, 100);
    }
    for (bool value : selected)
        check(value, "Rotation reaches each conscious member");
}
void reservesAndCleanup() {
    Fixture f(true);
    CombatEvents events;
    EffectRequest r;
    r.world = f.caster().world;
    r.caster = f.caster();
    r.count = 2;
    r.targets[0] = f.caster();
    r.targets[1] = f.game.actors().id(1);
    watching = true;
    allocations = 0;
    const auto reserved = f.effects().reserveBless(r);
    watching = false;
    check(reserved && !allocations, "Reservation is allocation-free");
    check(!f.effects().reserveBless(r), "Source cannot reserve a second pending Bless");
    f.effects().cancelBless(reserved.id);
    check(bool(f.effects().reserveBless(r)), "Cancellation releases the reserved effect capacity");
    // Cancel the replacement and create ordinary active NPC Bless.
    for (const auto& e : f.effects().snapshot())
        if (e.pendingBless)
            f.effects().cancelBless(e.id);
    const auto active = f.effects().apply(r, 0, events);
    check(active && f.effects().concentrationSource(f.caster()).serial,
          "NPC source owns active concentration");
    auto wrong = r;
    wrong.kind = EffectKind::Bane;
    check(f.effects().validate(wrong) == EffectError::InvalidKind,
          "NPC source cannot cast unrelated effects");
    wrong = r;
    wrong.targets[0] = CharacterId{0};
    check(f.effects().validate(wrong) == EffectError::InvalidTargets, "NPC Bless cannot buff party");
    wrong = r;
    wrong.caster = ActorId{0, f.caster().world + 1};
    check(f.effects().validate(wrong) == EffectError::InvalidCaster, "Foreign source rejects");
    events.clear();
    f.effects().damaged(f.caster(), 0, 0, events);
    check(!events.count, "Zero damage does not roll concentration");
    f.effects().damaged(f.caster(), INT64_MAX, 1, events);
    check(!f.effects().concentrationSource(f.caster()).serial, "Failed concentration ends all recipients");
    check(!f.effects().has(f.game.actors().id(1), EffectKind::Bless),
          "No permanent ally buff after source failure");
}
void cancellation() {
    Fixture f(true);
    f.cast();
    f.game.actors().damage(f.caster(), 100, f.game.timeMs());
    const auto& events = f.game.update(.6, false);
    bool canceled = false;
    for (const auto& e : events)
        canceled |= e.event == CombatEvent::AttackCanceled && !e.damage && !e.roll;
    check(canceled && f.game.combat().enemyMana(f.caster()) == 4,
          "Death cancels once without refund/disclosure");
    for (const auto& e : f.effects().snapshot())
        check(!e.pendingBless && !e.active, "Canceled source leaves no reservation or buff");
}
void resourcesAndRestore() {
    Fixture f(true);
    CombatEvents events;
    for (unsigned i = 0; i < 3; ++i) {
        const auto out = f.cast();
        check(out.spell == SpellKind::Bless, "Available mana supports recasting after concentration ends");
        f.game.update(.6, false);
        const auto held = f.effects().concentrationSource(f.caster());
        check(bool(held.serial), "Recast commits concentration");
        f.effects().remove(held, f.game.timeMs(), events);
    }
    check(f.game.combat().enemyMana(f.caster()) == 0 && f.cast().spell == SpellKind::SacredFlame,
          "Three Bless casts exhaust six mana, cantrip remains available");
    Fixture source(false, 8, true), restored(false, 8, true);
    EffectRequest r;
    r.world = source.caster().world;
    r.caster = source.caster();
    r.count = 1;
    r.targets[0] = source.caster();
    check(bool(source.effects().apply(r, 0, events)), "Own local Bless fixture");
    auto saved = std::make_unique<FloorState>();
    source.game.exportFloor(*saved);
    saved->combat.enemyMana[0] = 2;
    saved->combat.enemyTarget[0] = 3;
    restored.game.importFloor(*saved);
    check(restored.game.combat().enemyMana(restored.caster()) == 2 &&
              restored.game.combat().enemyTarget(restored.caster()) == 3 &&
              restored.effects().concentrationSource(restored.caster()).serial &&
              !restored.effects().concentrationSource(source.caster()).serial,
          "Floor restore retains resources and rebinds NPC source identity");
    auto again = std::make_unique<FloorState>();
    restored.game.exportFloor(*again);
    check(again->digest() == saved->digest(), "Fresh-world canonical digest preserves NPC casting state");
    for (unsigned fault = 0; fault < 4; ++fault) {
        auto bad = std::make_unique<FloorState>(*saved);
        if (fault == 0)
            bad->combat.enemyMana[0] = 7;
        if (fault == 1)
            bad->combat.enemyTarget[0] = 4;
        for (auto& e : bad->effects.instances)
            if (e.active) {
                if (fault == 2)
                    e.caster.enemy.slot = UINT16_MAX;
                if (fault == 3)
                    e.pendingBless = true;
            }
        bool rejected = false;
        try {
            restored.game.importFloor(*bad);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "Malformed NPC resources/source/reservation reject at import");
    }
}
void interruptionAndIncapacity() {
    std::unique_ptr<Fixture> chosen;
    for (unsigned seed = 0; seed < 16 && !chosen; ++seed) {
        auto candidate = std::make_unique<Fixture>(true, seed, true, true);
        check(candidate->cast().spell == SpellKind::Bless, "Support starts Bless during approach");
        auto shot = turnCommand(candidate->game, PartyAction::Ranged);
        shot.character = {0};
        shot.target = candidate->caster();
        check(candidate->game.submit(shot).event == CombatEvent::AttackStarted,
              "Real arrow accepted during approach cast windup");
        candidate->game.update(.3, false);
        if (candidate->game.actors().actor(candidate->caster())->hasDamage)
            chosen = std::move(candidate);
    }
    check(bool(chosen), "Bounded fixed seeds include a real arrow hit during windup");
    auto& f = *chosen;
    f.game.update(.3, false);
    check(!f.effects().has(f.caster(), EffectKind::Bless), "Positive hurt suspends pending cast release");
    f.game.update(.3, false);
    check(f.effects().has(f.caster(), EffectKind::Bless), "Suspended cast resumes once after hurt");
    CombatEvents events;
    f.effects().condition(f.caster(), EffectKind::Knockdown, CharacterId{0}, f.game.timeMs(), events);
    check(f.effects().concentrationSource(f.caster()).serial, "Prone alone never ends concentration");
    f.effects().condition(f.caster(), EffectKind::Sleep, CharacterId{0}, f.game.timeMs(), events);
    check(!f.effects().concentrationSource(f.caster()).serial &&
              !f.effects().has(f.game.actors().id(1), EffectKind::Bless),
          "Sleep ends source concentration and ally buff");
    Fixture sf;
    const auto sfAccepted = sf.cast();
    sf.game.party().damage(sfAccepted.recipient, 100);
    const auto& canceled = sf.game.update(.6, false);
    bool found = false;
    for (const auto& e : canceled)
        found |= e.event == CombatEvent::AttackCanceled && e.targetGone;
    check(found, "Downed locked target cancels Sacred Flame without retargeting");
}

void concentrationDamagePaths() {
    bool hit = false;
    for (unsigned seed = 0; seed < 16 && !hit; ++seed) {
        Fixture f(false, seed, true, true);
        CombatEvents notices;
        EffectRequest bless;
        bless.world = f.caster().world;
        bless.caster = f.caster();
        bless.count = 1;
        bless.targets[0] = f.caster();
        check(bool(f.effects().apply(bless, 0, notices)), "Owned active caster buff");
        check(f.cast().spell == SpellKind::SacredFlame, "Concentrating caster uses cantrip");
        auto arrow = turnCommand(f.game, PartyAction::Ranged);
        arrow.character = {0};
        arrow.target = f.caster();
        check(f.game.submit(arrow).event == CombatEvent::AttackStarted,
              "Real party arrow targets concentrating NPC");
        const auto& events = f.game.update(.3, false);
        unsigned checks = 0;
        for (const auto& e : events)
            checks += e.event == CombatEvent::Effect && e.effect.notice == EffectNotice::ConcentrationSave &&
                      e.effect.caster.npc;
        hit = f.game.actors().actor(f.caster())->hasDamage;
        check(checks == unsigned(hit), "Exactly one NPC concentration check per real positive arrow hit");
    }
    check(hit, "Bounded seeded arrows include concentration damage");
    Fixture web;
    CombatEvents events;
    EffectRequest bless;
    bless.world = web.caster().world;
    bless.caster = web.caster();
    bless.count = 1;
    bless.targets[0] = web.caster();
    check(bool(web.effects().apply(bless, 0, events)), "Owned Web victim concentrates");
    const auto cell = web.game.actors().actor(web.caster())->npc.cell;
    check(web.effects().createWeb(&cell, 1, {0}, 1, 0, events), "Own one-cell burning Web fixture");
    web.effects().ignite(cell);
    events.clear();
    watching = true;
    allocations = 0;
    web.effects().advance(6000, 6000, true, events);
    watching = false;
    unsigned checks = 0;
    for (const auto& e : events)
        checks += e.event == CombatEvent::Effect && e.effect.notice == EffectNotice::ConcentrationSave &&
                  e.effect.caster.npc;
    check(checks == 1 && !allocations, "Burning Web notifies NPC concentration once without allocation");
}
void subdivision() {
    Fixture a, b;
    a.cast();
    b.cast();
    a.game.update(.6, false);
    for (unsigned i = 0; i < 60; ++i)
        b.game.update(.01, false);
    auto one = std::make_unique<FloorState>(), many = std::make_unique<FloorState>();
    a.game.exportFloor(*one);
    b.game.exportFloor(*many);
    check(one->digest() == many->digest(),
          "Casting impact, resources and RNG agree across timestep subdivision");
}

void wardsAndSourceSeparation() {
    Fixture f;
    CombatEvents events;
    EffectRequest ward;
    ward.world = f.caster().world;
    ward.caster = CharacterId{0};
    ward.kind = EffectKind::Sanctuary;
    ward.count = 1;
    ward.difficulty = 1000;
    for (std::uint16_t i = 0; i < 4; ++i) {
        ward.targets[0] = CharacterId{i};
        check(bool(f.effects().apply(ward, 0, events)), "Own impossible Sanctuary wards");
    }
    bool spent = false;
    for (unsigned i = 0; i < 150 && !spent; ++i) {
        if (f.game.combat().snapshot().awaitingAction)
            f.game.submit(turnCommand(f.game));
        const auto& batch = f.game.update(.05, false);
        if (batch.step.event == CombatEvent::Spent && batch.step.enemyAttack) {
            unsigned saves = 0;
            for (const auto& e : batch)
                saves += e.event == CombatEvent::Effect && e.effect.kind == EffectKind::Sanctuary &&
                         e.effect.notice == EffectNotice::InitialSave;
            check(saves == 4 && f.game.combat().enemyMana(f.caster()) == 6,
                  "All wards tried once in rotation, blocked action costs no cantrip mana");
            spent = true;
        }
    }
    check(spent, "Warded party consumes the caster action instead of forcing damage");
    Fixture independent;
    EffectRequest buff;
    buff.world = independent.caster().world;
    buff.caster = independent.caster();
    buff.count = 1;
    buff.targets[0] = independent.caster();
    const auto npc = independent.effects().apply(buff, 0, events);
    buff.caster = CharacterId{0};
    buff.targets[0] = CharacterId{0};
    const auto party = independent.effects().apply(buff, 0, events);
    check(npc && party && npc.id.serial != party.id.serial &&
              independent.effects().concentrationSource(independent.caster()).serial == npc.id.serial &&
              independent.effects().concentration({0}).serial == party.id.serial,
          "NPC slot zero and party character zero own independent concentration");
    independent.game.actors().remove(independent.caster());
    independent.effects().reconcile(0, events);
    check(!independent.effects().find(npc.id) && independent.effects().find(party.id),
          "Removed NPC source ends only its own concentration");
}

void meleeFallback() {
    Fixture f;
    CombatEvents events;
    f.effects().condition(f.caster(), EffectKind::Blind, CharacterId{0}, 0, events);
    bool melee = false;
    for (unsigned i = 0; i < 150 && !melee; ++i) {
        if (f.game.combat().snapshot().awaitingAction)
            f.game.submit(turnCommand(f.game));
        const auto out = f.game.update(.05, false).step;
        check(out.event != CombatEvent::Cast, "Blind shaman does not bypass seeing requirements to cast");
        if (out.event == CombatEvent::AttackStarted && out.enemyAttack) {
            check(out.spell == SpellKind::Count && out.kind == AttackKind::Melee && out.impactMs == 950 &&
                      !f.game.actors().actor(f.caster())->rangedAttack &&
                      f.game.combat().enemyMana(f.caster()) == 6,
                  "Unavailable casting uses the ordinary staff melee timeline without mana cost");
            melee = true;
        }
    }
    check(melee, "Adjacent blinded caster reaches its weak melee fallback");
}

void fullCapacity() {
    Fixture f(true);
    CombatEvents events;
    EffectRequest r;
    r.world = f.caster().world;
    r.caster = CharacterId{0};
    r.kind = EffectKind::MageArmor;
    r.count = 1;
    r.targets[0] = CharacterId{3};
    // This fixture's wizard is unarmored. Same-kind independent effects do not stack.
    for (unsigned i = 0; i < EffectController::capacity; ++i)
        check(bool(f.effects().apply(r, 0, events)), "Fill owned effect storage");
    r.caster = f.caster();
    r.kind = EffectKind::Bless;
    r.targets[0] = f.caster();
    check(f.effects().reserveBless(r).error == EffectError::Capacity,
          "Full storage rejects before resource charge");
    check(f.game.combat().enemyMana(f.caster()) == 6, "Rejected reservation leaves mana intact");
    bool checked = false;
    for (unsigned i = 0; i < 150 && !checked; ++i) {
        const auto state = f.game.combat().snapshot();
        if (state.activeEnemy && state.activeEnemy->slot == 0 && !state.resolvingAction) {
            auto before = std::make_unique<FloorState>(), after = std::make_unique<FloorState>();
            f.game.exportFloor(*before);
            watching = true;
            allocations = 0;
            const auto out = f.game.update(.5, false).step;
            watching = false;
            f.game.exportFloor(*after);
            std::ostringstream a, b;
            before->rng.describe(a);
            after->rng.describe(b);
            check(out.reason == CommandReason::EffectCapacity && a.str() == b.str() && !allocations &&
                      f.game.combat().enemyMana(f.caster()) == 6,
                  "Actual NPC action rejects full effect capacity before mana/RNG/allocation");
            checked = true;
        } else {
            if (state.awaitingAction)
                f.game.submit(turnCommand(f.game));
            f.game.update(.05, false);
        }
    }
    check(checked, "Capacity fixture reaches the NPC action");
}
} // namespace
int main() {
    try {
        supportAndTimeline();
        soloAndRotation();
        reservesAndCleanup();
        cancellation();
        fullCapacity();
        resourcesAndRestore();
        interruptionAndIncapacity();
        concentrationDamagePaths();
        subdivision();
        wardsAndSourceSeparation();
        meleeFallback();
        std::cout << "shaman checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
