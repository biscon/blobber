#include "game/arena.hpp"
#include "game/commands.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/progression_fixtures.hpp"
#include <iostream>
#include <sstream>

using namespace blobber;
using namespace blobber::rules;
namespace {
Catalog zombieCatalog(int hp = 22, DamageProfile profile = {damageBit(DamageType::Poison), 0},
                      SpellKind chosen = SpellKind::Count, bool loot = false) {
    const auto base = creationCatalog(true);
    std::vector<RaceDefinition> races;
    std::vector<ClassDefinition> classes;
    std::vector<ItemDefinition> items;
    std::vector<SpellDefinition> spells;
    std::vector<CreationClass> creation;
    for (std::uint16_t i = 0; i < base.raceCount(); ++i)
        races.push_back(*base.race({i}));
    for (std::uint16_t i = 0; i < base.classCount(); ++i) {
        classes.push_back(*base.characterClass({i}));
        creation.push_back(*base.creation({i}));
    }
    for (std::uint16_t i = 0; i < base.itemCount(); ++i)
        items.push_back(*base.item({i}));
    for (std::uint16_t i = 0; i < base.spellCount(); ++i)
        spells.push_back(*base.spell({i}));
    auto preset = *base.preset();
    if (const auto* r = spellRules(chosen); r && !r->level) {
        auto& cantrips = preset.characters[r->casting == Casting::Wisdom ? 1 : 3].spells.cantrips;
        const auto bit = spellBit(base.findSpell(r->id));
        if (!((cantrips | preset.characters[r->casting == Casting::Wisdom ? 1 : 3].spells.racial) & bit)) {
            cantrips &= cantrips - 1;
            cantrips |= bit;
        }
    }
    MonsterDefinition zombie;
    zombie.id = "zombie";
    zombie.abilities = {13, 6, 16, 3, 6, 5};
    zombie.armorClass = 8;
    zombie.hitPoints = hp;
    zombie.attacks[0] = {0};
    zombie.attackCount = 1;
    zombie.traits = static_cast<Traits>(Trait::UndeadFortitude);
    zombie.sleepImmune = true;
    zombie.experience = 50;
    zombie.damageProfile = profile;
    if (loot) {
        zombie.loot[0].item = {0};
        zombie.lootCount = 1;
    }
    // No enemy damage in spell-path fixtures; player rolls and Fortitude stay real.
    return {{{"zombie.slam", AttackKind::Melee, -100, {1, 6, -100}, DamageType::Bludgeoning}},
            {zombie},
            races,
            classes,
            items,
            preset,
            spells,
            creation};
}
NpcWorld placements(const Map& map, Player pose, Cell cell, int count = 1) {
    std::vector<NpcInstance> instances;
    for (int i = 0; i < count; ++i)
        instances.push_back({{0}, cell, {i % 2, i / 2}, Heading::South});
    return {map, pose.cell, {"zombie"}, instances};
}
NpcTimings timing() {
    NpcTimings t{};
    t[0] = {{{600, 600}}, 2};
    t[1] = {{{300, 300}}, 2};
    t[2] = {{{700, 250, 250, 350}}, 4};
    t[3] = {{{150, 150}}, 2};
    t[4] = {{{200, 200, 200, 600}}, 4};
    t[6] = {{{1000}}, 1};
    return t;
}
struct Fixture {
    Map map = combatMap();
    Catalog catalog;
    NpcWorld npcs;
    Party party;
    ActorWorld actors;
    DiceRng rng;
    EffectController effects;
    Fixture(unsigned seed = 0, int hp = 3, DamageProfile profile = {damageBit(DamageType::Poison), 0})
        : catalog(zombieCatalog(hp, profile)), npcs(placements(map, {{3, 3}, Heading::North}, {3, 2})),
          party(catalog), actors(map, catalog, npcs), rng(seed), effects(party, actors, rng) {}
    NpcDamageResult hit(std::int64_t amount, DamageType type = DamageType::Slashing, bool critical = false,
                        std::uint64_t when = 100) {
        watching = true;
        allocations = 0;
        const auto out = damageNpc(actors, effects, actors.id(0), amount, type, critical, when);
        watching = false;
        check(!allocations, "NPC damage, save and commit allocate no engine memory");
        return out;
    }
};
void savesAndAtomicity() {
    bool success = false, failure = false;
    for (unsigned seed = 0; seed < 64 && !(success && failure); ++seed) {
        Fixture f(seed);
        DiceRng expected = f.rng;
        const auto roll = expected.d20().value.selected;
        const bool saved = roll + 3 >= 8;
        const auto id = f.actors.id(0);
        const auto revision = f.actors.snapshot().revision;
        const auto out = f.hit(3);
        const auto& a = *f.actors.actor(id);
        check(out.actor && out.fortitude.roll == roll && out.fortitude.total == roll + 3 &&
                  out.fortitude.difficulty == 8 && out.resolved.damage == 3,
              "Exact lethal uses Con +3 and DC 5 plus full damage");
        check(f.rng.d20().value.selected == expected.d20().value.selected,
              "Exactly one ordinary save roll at eligible impact");
        check(f.actors.id(0).world == id.world && f.actors.id(0).slot == id.slot &&
                  f.actors.snapshot().revision == revision + 1 && a.hasDamage && a.damageTimeMs == 100,
              "One atomic revision retains identity and records positive hit");
        check(a.hitPoints == (saved ? 1 : 0) && a.hasDeath == !saved && out.actor.died == !saved &&
                  f.actors.occupied({3, 2}) == saved &&
                  a.life == (saved ? ActorLife::Alive : ActorLife::Dead),
              "Survival/death commits HP, life, occupancy and permanent markers together");
        NpcAnimator animator(f.actors, {timing()});
        animator.setTime(100, &f.effects);
        check(animator.snapshot().frames[0].clip == (saved ? NpcAnimation::Hurt : NpcAnimation::Death),
              "Survival never publishes a death clip");
        success |= saved;
        failure |= !saved;
    }
    check(success && failure, "Fixed seeds exercise success and failure");
    Fixture f(0);
    auto expected = f.rng;
    for (unsigned i = 0; i < 3; ++i) {
        const auto roll = expected.d20().value.selected;
        const auto out = f.hit(i ? 1 : 3, DamageType::Slashing, false, 100 + i);
        check(out.fortitude.roll == roll && out.fortitude.status == FortitudeStatus::Saved &&
                  !out.actor.died && f.actors.actor(f.actors.id(0))->hitPoints == 1,
              "Repeated successful impacts at one HP retain occupancy and identity");
        check(out.actor.changed && out.actor.damage == (i ? 0 : 2),
              "Positive hit remains observable even when no HP is removed");
    }
    Fixture overkill(0, 1);
    const auto out = overkill.hit(20);
    check(out.fortitude.difficulty == 25 && out.fortitude.status == FortitudeStatus::Failed && out.actor.died,
          "Overkill uses full damage, not HP removed");
    Fixture huge(0, 1);
    const auto big = huge.hit(INT64_MAX);
    check(big.fortitude.difficulty == INT64_MAX && big.actor.died,
          "Huge lethal amount saturates DC without overflow");
    Fixture natural20(0, 1);
    natural20.rng.d20(); // Seed zero's next selected face is 20.
    const auto high = natural20.hit(20);
    check(high.fortitude.roll == 20 && high.fortitude.status == FortitudeStatus::Failed && high.actor.died,
          "A natural 20 does not automatically pass an impossible save");
}
void eligibility() {
    for (const auto type : {DamageType::Radiant, DamageType::Slashing}) {
        Fixture f;
        auto expected = f.rng;
        const auto out = f.hit(3, type, true);
        check(out.actor.died &&
                  out.fortitude.status == (type == DamageType::Radiant ? FortitudeStatus::RadiantBypass
                                                                       : FortitudeStatus::CriticalBypass),
              "Radiant and critical damage bypass, with radiant explanatory precedence");
        check(!out.fortitude.roll && !out.fortitude.difficulty &&
                  f.rng.d20().value.selected == expected.d20().value.selected,
              "Bypass consumes no Fortitude RNG");
    }
    for (const auto amount : {0, 1, 2}) {
        Fixture f;
        const auto expected = f.rng;
        const auto out = f.hit(amount);
        std::ostringstream a, b;
        f.rng.describe(a);
        expected.describe(b);
        check(out.fortitude.status == FortitudeStatus::None && a.str() == b.str() && !out.actor.died,
              "Zero and nonlethal damage do not roll Fortitude");
    }
    Fixture immune;
    auto expected = immune.rng;
    const auto out = immune.hit(INT64_MAX, DamageType::Poison);
    check(out.resolved.damage == 0 && out.resolved.adjustment == DamageAdjustment::Immune &&
              out.fortitude.status == FortitudeStatus::None &&
              !immune.actors.actor(immune.actors.id(0))->hasDamage &&
              immune.rng.d20().value.selected == expected.d20().value.selected,
          "Poison immunity precedes lethal/save checks and hurt");
    Fixture vulnerable(0, 3, {0, damageBit(DamageType::Bludgeoning)});
    check(vulnerable.hit(2, DamageType::Bludgeoning).fortitude.difficulty == 9,
          "Shared helper applies mitigation exactly once before save DC");
    for (unsigned invalid = 0; invalid < 5; ++invalid) {
        Fixture f;
        auto id = f.actors.id(0);
        auto amount = std::int64_t(3);
        if (invalid == 0)
            ++id.world;
        if (invalid == 1)
            f.actors.remove(id);
        if (invalid == 2)
            f.actors.damage(id, 100, 100);
        if (invalid == 3)
            f.actors.damage(id, 1, 200);
        if (invalid == 4)
            amount = -1;
        auto before = f.rng;
        const auto rejected = damageNpc(f.actors, f.effects, id, amount, DamageType::Slashing, false, 100);
        check(!rejected.actor && rejected.fortitude.status == FortitudeStatus::None &&
                  f.rng.d20().value.selected == before.d20().value.selected,
              "Invalid handle/life/time/damage rejects before save RNG");
    }
}
void saveModifiers() {
    Fixture f;
    CombatEvents events;
    EffectRequest bane;
    bane.world = f.actors.snapshot().world;
    bane.caster = {3};
    bane.kind = EffectKind::Bane;
    bane.targets[0] = f.actors.id(0);
    bane.count = 1;
    bane.difficulty = 100;
    check(bool(f.effects.apply(bane, 0, events)), "Bane applies to the zombie fixture");
    f.rng = DiceRng(0);
    auto expected = f.rng;
    const auto roll = expected.d20().value.selected;
    const auto penalty = expected.roll({1, 4, 0}).value.total;
    const auto out = f.hit(3);
    check(out.fortitude.total == roll + 3 - penalty &&
              f.rng.d20().value.selected == expected.d20().value.selected,
          "Fortitude shares the existing effect-modified save and exact RNG order");
}
void spellPaths() {
    const auto map = hybridRoom();
    for (const auto kind : {SpellKind::PoisonSpray, SpellKind::SacredFlame, SpellKind::BurningHands,
                            SpellKind::MagicMissile, SpellKind::ScorchingRay, SpellKind::Shatter,
                            SpellKind::RayOfFrost, SpellKind::GuidingBolt, SpellKind::Thunderwave,
                            SpellKind::SpiritualWeapon, SpellKind::InflictWounds, SpellKind::ShockingGrasp}) {
        bool positive = false;
        for (const unsigned seed : {0u, 5u, 8u}) {
            const auto c = zombieCatalog(1, {damageBit(DamageType::Poison), 0}, kind);
            const bool close = kind == SpellKind::PoisonSpray || kind == SpellKind::BurningHands ||
                               kind == SpellKind::Thunderwave || kind == SpellKind::InflictWounds ||
                               kind == SpellKind::ShockingGrasp;
            const auto npcs = placements(map, spellSpawn, close ? Cell{6, 7} : Cell{6, 4});
            GameSession s(map, c, npcs, seed, spellSpawn, false, {timing()});
            if (kind == SpellKind::ShockingGrasp) {
                s.party().lockFormation(false);
                check(s.party().swapFormation(FormationSlot::FrontRight, FormationSlot::BackRight),
                      "Melee caster occupies a legal front slot");
                s.party().lockFormation(true);
            }
            levelParty(s, 3, kind);
            if (close)
                levelReady(s, spellRules(kind)->casting == Casting::Wisdom ? 1 : 3);
            auto command = spellCommand(s, kind);
            command.targetCount = spellRules(kind)->area ? 0 : 1;
            command.enemies[0] = s.actors().id(0);
            command.darts[0] = 3;
            if (kind == SpellKind::Shatter)
                command.aimedCell = Cell{6, 4};
            unsigned impacts = 0, checked = 0;
            const auto inspect = [&](const CombatEvents& events) {
                for (const auto& e : events) {
                    if (e.event != CombatEvent::Attack || e.spell != kind || e.enemyAttack)
                        continue;
                    ++impacts;
                    if (e.targetGone || !e.hit || !e.damage) {
                        check(e.fortitude.status == FortitudeStatus::None,
                              "Miss/immune/gone spell recipient has no Fortitude");
                        continue;
                    }
                    positive = true;
                    ++checked;
                    if (e.damageType == DamageType::Radiant)
                        check(e.fortitude.status == FortitudeStatus::RadiantBypass && e.died,
                              "Real radiant spell is a dependable lethal finisher");
                    else if (e.critical)
                        check(e.fortitude.status == FortitudeStatus::CriticalBypass && e.died,
                              "Real critical spell bypasses Fortitude");
                    else {
                        check(e.fortitude.status == FortitudeStatus::Saved ||
                                  e.fortitude.status == FortitudeStatus::Failed,
                              "Every real lethal spell path resolves Fortitude");
                        check(e.fortitude.difficulty == 5 + e.damage &&
                                  e.died == (e.fortitude.status == FortitudeStatus::Failed),
                              "Spell save uses full adjusted impact and final death state");
                    }
                    if (kind == SpellKind::MagicMissile)
                        check(e.darts == 3 && e.damage % 3 == 0,
                              "Magic Missile keeps one aggregated impact/save per target");
                }
            };
            const auto accepted = s.submit(command);
            check(accepted.reason == CommandReason::Resolved,
                  (std::string(spellRules(kind)->id) + ": " + commandMessage(accepted.reason)).c_str());
            check(accepted.fortitude.status == FortitudeStatus::None,
                  "Acceptance does not precompute a Fortitude result");
            inspect(s.events());
            for (unsigned step = 0; step < 12; ++step)
                inspect(s.update(.2, false));
            check(impacts > 0, "Submitted spell produces normal resolved events");
            if (kind == SpellKind::MagicMissile)
                check(impacts == 1 && checked == 1, "Three darts do not invent three saves");
            if (kind == SpellKind::ScorchingRay)
                check(checked <= 3, "Separate rays never roll Fortitude after target death");
        }
        check(positive || kind == SpellKind::PoisonSpray,
              "Fixed spell seeds include a positive damaging hit");
    }
}
void burningWeb() {
    const auto map = hybridRoom();
    const auto c = zombieCatalog(1);
    const auto npcs = placements(map, spellSpawn, {6, 4});
    Party party(c);
    ActorWorld actors(map, c, npcs);
    DiceRng rng(0);
    EffectController effects(party, actors, rng);
    CombatEvents events;
    effects.bindEvents(events, 0);
    effects.partyCell(spellSpawn.cell);
    const Cell cell{6, 4};
    check(effects.createWeb(&cell, 1, {3}, 1000, 0, events), "Owned Web created");
    effects.ignite(cell);
    events.clear();
    effects.advance(6000, 6000, true, events);
    unsigned found = 0;
    for (const auto& e : events)
        if (e.event == CombatEvent::Attack && e.spell == SpellKind::Web) {
            ++found;
            check(e.damageType == DamageType::Fire && e.damage > 0 &&
                      e.fortitude.difficulty == 5 + e.damage &&
                      (e.fortitude.status == FortitudeStatus::Saved ||
                       e.fortitude.status == FortitudeStatus::Failed),
                  "Burning Web uses the authoritative NPC lethal/save helper");
        }
    check(found == 1, "Web burns a creature once");
}
void weaponsAndTiming() {
    const auto map = hybridRoom();
    const auto c = zombieCatalog(1);
    bool saved = false, failed = false, critical = false, missed = false;
    for (unsigned seed = 0; seed < 32 && !(saved && failed && critical && missed); ++seed) {
        const auto npcs = placements(map, spellSpawn, {6, 4});
        GameSession s(map, c, npcs, seed, spellSpawn, false, {timing()});
        PartyCommand command;
        command.character = {2};
        command.action = PartyAction::Ranged;
        command.world = s.combat().snapshot().world;
        command.turn = s.combat().snapshot().turn;
        command.target = s.actors().id(0);
        const auto accepted = s.submit(command);
        check(accepted.event == CombatEvent::AttackStarted &&
                  accepted.fortitude.status == FortitudeStatus::None,
              "Arrow acceptance reveals and rolls no Fortitude");
        auto state = std::make_unique<FloorState>();
        s.exportFloor(*state); // Read RNG only; unsettled snapshots are never imported.
        auto expected = state->rng;
        const auto roll = expected.d20().value.selected;
        const auto impactMs = s.combat().attacks().attacks[NpcWorld::maxInstances + 2].impactMs;
        check(impactMs > 0, "Arrow has a real delayed impact");
        s.update(double(impactMs - 1) / 1000, false);
        check(s.actors().actor(*command.target)->hitPoints == 1 &&
                  !s.actors().actor(*command.target)->hasDamage,
              "No damage/death/save before arrival");
        s.update(30, true);
        watching = true;
        allocations = 0;
        const auto& events = s.update(.001, false);
        watching = false;
        check(!allocations, "Real delayed lethal/save processing allocates no engine memory");
        unsigned impacts = 0;
        for (const auto& e : events)
            if (e.event == CombatEvent::Attack && !e.enemyAttack &&
                e.attackId.serial == accepted.attackId.serial) {
                ++impacts;
                check(e.occurredMs == impactMs, "Save and HP resolve exactly at arrow deadline");
                if (!e.hit) {
                    missed = true;
                    check(e.fortitude.status == FortitudeStatus::None, "Miss never saves");
                } else if (e.critical) {
                    critical = true;
                    check(e.fortitude.status == FortitudeStatus::CriticalBypass && e.died,
                          "Natural critical arrow bypasses Fortitude");
                } else {
                    saved |= e.fortitude.status == FortitudeStatus::Saved;
                    failed |= e.fortitude.status == FortitudeStatus::Failed;
                    check(e.fortitude.roll == roll && e.fortitude.total == roll + 3 &&
                              e.fortitude.difficulty == 5 + e.damage,
                          "Saved attack/damage rolls precede exactly one impact-time Con save");
                }
            }
        check(impacts == 1, "Arrow applies exactly one authoritative consequence");
        const auto hp = s.actors().actor(*command.target)->hitPoints;
        s.update(0, false);
        check(s.actors().actor(*command.target)->hitPoints == hp, "Impact does not repeat");
    }
    check(saved && failed && critical && missed, "Bounded arrow seeds cover all outcomes");
    const Player close{{6, 5}, Heading::North};
    const auto npcs = placements(map, close, {6, 4});
    GameSession melee(map, c, npcs, 8, close, false, {timing()});
    levelReady(melee, 0);
    auto command = turnCommand(melee, PartyAction::Melee);
    command.target = melee.actors().id(0);
    check(melee.submit(command).event == CombatEvent::AttackStarted, "Ordinary melee windup accepted");
    bool resolved = false;
    for (unsigned step = 0; step < 15 && !resolved; ++step)
        for (const auto& e : melee.update(.1, false))
            if (e.event == CombatEvent::Attack && !e.enemyAttack) {
                resolved = true;
                check(!e.hit || e.fortitude.status != FortitudeStatus::None,
                      "Melee uses the same authoritative lethal path");
            }
    check(resolved, "Melee resolves through normal timeline");
}
void rewardsFeedbackAndRestore() {
    const auto c = zombieCatalog(3, {damageBit(DamageType::Poison), 0}, SpellKind::Count, true);
    const auto map = combatMap();
    const Player spawn{{3, 5}, Heading::North};
    const auto npcs = placements(map, spawn, {3, 2});
    FloorFixtures fixtures;
    fixtures.count = 2;
    fixtures.entries[0] = {{1, 2}, Heading::North, FixtureKind::Door};
    fixtures.entries[1] = {{5, 2}, Heading::North, FixtureKind::GuardianGate};
    FloorObjectiveDefinition objective;
    objective.count = 1;
    objective.members[0] = 0;
    objective.approachDoor = 0;
    objective.gate = 1;
    objective.rewardCount = 1;
    objective.rewards[0] = {{4, 4}, {0}, 1};
    GameSession original(map, c, npcs, 0, spawn, false, {timing()}, {}, fixtures, objective);
    DiceRng rng(0);
    EffectController effects(original.party(), original.actors(), rng);
    const auto id = original.actors().id(0);
    const auto hit = damageNpc(original.actors(), effects, id, 3, DamageType::Slashing, false, 0);
    check(hit.fortitude.status == FortitudeStatus::Saved, "Fixture survives its first lethal hit");
    original.update(0, true);
    check(!original.objective().completed && !arenaCleared(original.actors().snapshot()) &&
              original.geometry().solid({5, 2}) && original.party().character({0})->experience == 0,
          "Fortitude cannot publish gate completion, arena clear or XP");
    for (const auto& stack : original.loot().snapshot().stacks)
        check(!stack.active, "Neither corpse loot nor guardian reward is published on survival");
    CommandController ui(original);
    check(ui.snapshot().enemyPortraits[0] == EnemyPortrait::Zombie, "Zombie initiative portrait selected");
    auto targets = allCombatTargets(original);
    targets.anchors[0] = {200, 120, true};
    ui.refresh(targets, false, false, true);
    CombatOutcome event;
    event.event = CombatEvent::Attack;
    event.enemy = id;
    event.hit = true;
    event.damage = hit.resolved.damage;
    event.fortitude = hit.fortitude;
    watching = true;
    allocations = 0;
    ui.record(event);
    watching = false;
    check(!allocations &&
              std::string_view(ui.snapshot().log.newest(0)).find("remains at 1 HP") != std::string_view::npos,
          "Survival feedback is explicit and allocation-free");
    bool splash = false;
    for (const auto& f : ui.snapshot().feedback)
        splash |= f.active && f.text && std::string_view(f.text) == "1 HP";
    check(splash, "Survival splash is legible at one HP");
    auto saved = std::make_unique<FloorState>();
    original.exportFloor(*saved);
    GameSession restored(map, c, npcs, 999, spawn, false, {timing()}, {}, fixtures, objective);
    restored.importFloor(*saved);
    check(restored.actors().id(0).world != id.world && restored.actors().occupied({3, 2}) &&
              restored.actors().actor(restored.actors().id(0))->hitPoints == 1 &&
              !restored.actors().actor(restored.actors().id(0))->hasDeath,
          "One-HP survival roundtrips under a fresh actor-world identity");
    for (auto* s : {&original, &restored}) {
        DiceRng finishRng(0);
        EffectController finishEffects(s->party(), s->actors(), finishRng);
        check(damageNpc(s->actors(), finishEffects, s->actors().id(0), 1, DamageType::Radiant, false, 0)
                  .actor.died,
              "Radiant finisher commits real death");
        s->update(0, true);
        check(s->objective().completed && s->objective().rewardIssued && !s->geometry().solid({5, 2}) &&
                  arenaCleared(s->actors().snapshot()),
              "Real death completes ordinary objective and arena checks");
        unsigned active = 0;
        for (const auto& stack : s->loot().snapshot().stacks)
            active += stack.active;
        check(active == 2 && s->party().character({0})->experience == 12 &&
                  s->party().character({0})->experienceFraction == 6,
              "Exactly one corpse drop, guardian reward and 50 XP share");
        const auto revision = s->loot().snapshot().revision;
        s->update(0, true);
        check(s->loot().snapshot().revision == revision && s->party().character({0})->experience == 12,
              "Repeated reconciliation cannot duplicate rewards");
    }
    original.exportFloor(*saved);
    GameSession dead(map, c, npcs, 10, spawn, false, {timing()}, {}, fixtures, objective);
    dead.importFloor(*saved);
    dead.update(0, true);
    check(dead.actors().actor(dead.actors().id(0))->hasDeath && !dead.actors().occupied({3, 2}) &&
              dead.party().character({0})->experience == 0 && dead.objective().completed,
          "Dead floor snapshot retains credited death without awarding XP twice to a fresh party");
}
void restoredImpactOrder() {
    const auto c = zombieCatalog(1, {damageBit(DamageType::Poison), 0}, SpellKind::ScorchingRay);
    const auto map = hybridRoom();
    const auto npcs = placements(map, spellSpawn, {6, 4});
    GameSession original(map, c, npcs, 0, spellSpawn, false, {timing()});
    GameSession restored(map, c, npcs, 999, spellSpawn, false, {timing()});
    levelParty(original, 3, SpellKind::ScorchingRay);
    levelParty(restored, 3, SpellKind::ScorchingRay);
    auto saved = std::make_unique<FloorState>();
    original.exportFloor(*saved);
    restored.importFloor(*saved);
    std::array<std::vector<CombatOutcome>, 2> results;
    unsigned index = 0;
    for (auto* s : {&original, &restored}) {
        auto command = spellCommand(*s, SpellKind::ScorchingRay);
        command.targetCount = 1;
        command.enemies[0] = s->actors().id(0);
        command.darts[0] = 3;
        check(s->submit(command).reason == CommandReason::Resolved, "Restored caster launches real rays");
        const auto steps = index ? 20u : 200u;
        for (unsigned step = 0; step < steps; ++step)
            for (const auto& e : s->update(index ? .1 : .01, false))
                if (e.event == CombatEvent::Attack && e.spell == SpellKind::ScorchingRay)
                    results[index].push_back(e);
        ++index;
    }
    check(results[0].size() == 3 && results[1].size() == 3,
          "Equal-time rays keep all three ordered impact publications");
    for (unsigned i = 0; i < 3; ++i) {
        const auto& a = results[0][i];
        const auto& b = results[1][i];
        check(a.roll == b.roll && a.hit == b.hit && a.critical == b.critical && a.damage == b.damage &&
                  a.targetGone == b.targetGone && a.died == b.died &&
                  a.fortitude.status == b.fortitude.status && a.fortitude.roll == b.fortitude.roll &&
                  a.fortitude.difficulty == b.fortitude.difficulty && a.occurredMs == b.occurredMs,
              "Fresh identities and subdivided ticks preserve accepted rays and impact-time saves");
    }
    auto after = std::make_unique<FloorState>();
    original.exportFloor(*saved);
    restored.exportFloor(*after);
    std::ostringstream a, b;
    saved->rng.describe(a);
    after->rng.describe(b);
    check(a.str() == b.str() && saved->actors.actors[0].hitPoints == after->actors.actors[0].hitPoints &&
              saved->actors.actors[0].hasDeath == after->actors.actors[0].hasDeath,
          "Restored simulation preserves final HP/death and exact combat RNG continuation");
}
} // namespace
int main() {
    try {
        savesAndAtomicity();
        eligibility();
        saveModifiers();
        spellPaths();
        burningWeb();
        weaponsAndTiming();
        rewardsFeedbackAndRestore();
        restoredImpactOrder();
        std::cout << "Zombie checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
