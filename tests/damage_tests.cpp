#include "game/commands.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/progression_fixtures.hpp"
#include <iostream>
#include <sstream>
using namespace blobber;
using namespace blobber::rules;
namespace {
Catalog profiled(const Catalog& base, DamageProfile profile, bool undead = false,
                 SpellKind chosen = SpellKind::Count) {
    std::vector<AttackDefinition> attacks;
    std::vector<MonsterDefinition> monsters;
    std::vector<RaceDefinition> races;
    std::vector<ClassDefinition> classes;
    std::vector<ItemDefinition> items;
    std::vector<SpellDefinition> spells;
    std::vector<CreationClass> creation;
    for (unsigned i = 0; i < base.attackCount(); ++i)
        attacks.push_back(*base.attack({std::uint16_t(i)}));
    for (unsigned i = 0; i < base.monsterCount(); ++i) {
        auto m = *base.monster({std::uint16_t(i)});
        m.damageProfile = profile;
        m.sleepImmune = undead;
        m.humanoid = !undead;
        m.metalArmor = undead;
        monsters.push_back(m);
    }
    for (unsigned i = 0; i < base.raceCount(); ++i)
        races.push_back(*base.race({std::uint16_t(i)}));
    for (unsigned i = 0; i < base.classCount(); ++i) {
        classes.push_back(*base.characterClass({std::uint16_t(i)}));
        if (const auto* c = base.creation({std::uint16_t(i)}))
            creation.push_back(*c);
    }
    for (unsigned i = 0; i < base.itemCount(); ++i)
        items.push_back(*base.item({std::uint16_t(i)}));
    for (unsigned i = 0; i < base.spellCount(); ++i)
        spells.push_back(*base.spell({std::uint16_t(i)}));
    auto preset = *base.preset();
    const auto* chosenRules = spellRules(chosen);
    if (chosenRules && chosenRules->level == 0) {
        auto& cantrips = preset.characters[chosenRules->casting == Casting::Wisdom ? 1 : 3].spells.cantrips;
        const auto bit = spellBit(base.findSpell(chosenRules->id));
        if (!(cantrips & bit)) {
            cantrips &= cantrips - 1;
            cantrips |= bit;
        }
    }
    return {attacks, monsters, races, classes, items, preset, spells, creation};
}
void resolution() {
    const DamageProfile skeleton{damageBit(DamageType::Poison), damageBit(DamageType::Bludgeoning)};
    check(resolveDamage(7, DamageType::Bludgeoning, skeleton).damage == 14, "Blunt vulnerability");
    check(resolveDamage(7, DamageType::Piercing, skeleton).damage == 7, "Piercing normal");
    check(resolveDamage(7, DamageType::Poison, skeleton).damage == 0, "Poison immune");
    check(resolveDamage(-1, DamageType::Fire, skeleton).damage == 0, "Negative roll clamps");
    check(resolveDamage(INT64_MAX, DamageType::Bludgeoning, skeleton).damage == INT64_MAX, "Safe overflow");
    check(resolveDamage(0, DamageType::Bludgeoning, skeleton).damage == 0, "Zero remains zero");
    check(!validDamageProfile({1, 1}) && !validDamageProfile({0x8000, 0}), "Invalid profiles rejected");
    rejects([&] { (void)profiled(spellCatalog(), {1, 1}); }, "Catalog rejects conflicting profile");
}
void spellPaths() {
    const auto map = hybridRoom();
    for (const auto kind : {SpellKind::PoisonSpray, SpellKind::SacredFlame, SpellKind::BurningHands,
                            SpellKind::MagicMissile, SpellKind::ScorchingRay, SpellKind::Shatter,
                            SpellKind::RayOfFrost, SpellKind::GuidingBolt, SpellKind::Thunderwave}) {
        const auto type = spellRules(kind)->damage;
        for (const bool immune : {false, true}) {
            const auto catalog = profiled(
                creationCatalog(true),
                immune ? DamageProfile{damageBit(type), 0} : DamageProfile{0, damageBit(type)}, false, kind);
            const bool close = kind == SpellKind::PoisonSpray || kind == SpellKind::BurningHands ||
                               kind == SpellKind::Thunderwave;
            const auto places = spellPlacements(map, close ? 4 : 3, close);
            GameSession s(map, catalog, places, 5, spellSpawn, false);
            levelParty(s, 3, kind);
            if (close)
                levelReady(s, 3);
            auto c = spellCommand(s, kind);
            c.targetCount = spellRules(kind)->area ? 0 : 1;
            c.enemies[0] = s.actors().id(close ? 2 : 0);
            c.darts[0] = 3;
            if (kind == SpellKind::Shatter)
                c.aimedCell = Cell{6, 4};
            CommandController ui(s);
            auto visible = allCombatTargets(s);
            for (std::size_t i = 0; i < visible.count; ++i)
                visible.anchors[visible.ids[i].slot] = {200, 120, true};
            ui.refresh(visible, false, false, true);
            const auto mana = s.party().character(c.character)->mana;
            const auto accepted = s.submit(c);
            if (accepted.reason != CommandReason::Resolved)
                throw std::runtime_error(std::string(spellRules(kind)->id) + ": " +
                                         commandMessage(accepted.reason));
            check(s.party().character(c.character)->mana == mana - spellManaCost(spellRules(kind)->level),
                  "Spell spends its cost once");
            int impacts = 0, saves = 0;
            const auto inspect = [&](const CombatEvents& events) {
                for (const auto& e : events) {
                    if (e.spell != kind || e.enemyAttack)
                        continue;
                    if (e.event == CombatEvent::SpellSave)
                        ++saves;
                    if (e.event != CombatEvent::Attack || e.targetGone)
                        continue;
                    ++impacts;
                    check(e.damageType == type, "Impact carries concrete type");
                    if (e.hit) {
                        check(e.damage == (immune ? 0 : e.rawDamage * 2), "Exactly one mitigation at impact");
                        check(e.damageAdjustment ==
                                  (immune ? DamageAdjustment::Immune : DamageAdjustment::Vulnerable),
                              "Adjustment is observable");
                    }
                }
            };
            inspect(s.events());
            ui.record(s.events());
            const auto& arrived = s.update(.5, false);
            inspect(arrived);
            ui.record(arrived);
            if (immune && kind == SpellKind::PoisonSpray) {
                bool feedback = false;
                for (const auto& f : ui.snapshot().feedback)
                    feedback |= f.active && f.immune && f.text && std::string_view(f.text) == "IMMUNE";
                check(feedback, "Poison immunity has neutral explicit feedback");
            }
            check(impacts > 0, "Spell produces real impacts");
            if (immune) {
                check(kind == SpellKind::Thunderwave ? saves > 0 : saves == 0,
                      "Damage-only saves skip immunity; independent push saves remain");
                if (kind == SpellKind::RayOfFrost)
                    check(s.effects().has(c.enemies[0], EffectKind::Slow),
                          "Zero damage retains hit-based slow");
                if (kind == SpellKind::GuidingBolt)
                    check(s.effects().guiding(c.enemies[0]), "Zero damage retains hit-based guiding mark");
                for (const auto& a : s.actors().snapshot().actors)
                    check(!a.hasDamage, "Immune hit creates no hurt state");
            }
        }
    }
}
void weaponAndAllocation() {
    const auto map = combatMap();
    const auto catalog =
        profiled(combatCatalog(1000, -100, {1, 4, -100}, 1), {0, damageBit(DamageType::Bludgeoning)});
    const Player pose{{3, 3}, Heading::North};
    const auto places = combatPlacements(map, pose, 1);
    GameSession s(map, catalog, places, 5, pose, false);
    spellReady(s, 1);
    auto c = turnCommand(s, PartyAction::Melee);
    c.character = {1};
    c.target = s.actors().id(0);
    allocations = 0;
    watching = true;
    const auto accepted = s.submit(c);
    const auto& events = s.update(.3, false);
    watching = false;
    check(allocations == 0, "Typed weapon impact allocates nothing");
    check(accepted.event == CombatEvent::AttackStarted && accepted.rawDamage == 0 && accepted.damage == 0,
          "Acceptance redacts saved damage");
    bool found = false;
    for (const auto& e : events)
        if (e.event == CombatEvent::Attack && !e.enemyAttack) {
            found = true;
            check(e.damageType == DamageType::Bludgeoning && e.damage == e.rawDamage * 2,
                  "Mace vulnerability");
        }
    check(found, "Weapon impact observed");
}
void weaponCriticalAndRng() {
    const auto map = combatMap();
    const Player pose{{3, 3}, Heading::North};
    const auto normal = combatCatalog(1000, -100, {1, 4, -100}, 15);
    const auto vulnerable = profiled(normal, {0, damageBit(DamageType::Bludgeoning)});
    const auto immune = profiled(normal, {damageBit(DamageType::Bludgeoning), 0});
    const auto places = combatPlacements(map, pose, 1);
    bool hit = false, miss = false, critical = false;
    for (unsigned seed = 0; seed < 80 && !(hit && miss && critical); ++seed) {
        CombatOutcome baseline;
        for (const auto* catalog : {&normal, &vulnerable, &immune}) {
            GameSession s(map, *catalog, places, seed, pose, false);
            spellReady(s, 1);
            auto command = turnCommand(s, PartyAction::Melee);
            command.character = {1};
            command.target = s.actors().id(0);
            check(s.submit(command).event == CombatEvent::AttackStarted, "Mace starts normally");
            bool found = false;
            for (const auto& e : s.update(.3, false)) {
                if (e.event != CombatEvent::Attack || e.enemyAttack)
                    continue;
                found = true;
                if (catalog == &normal) {
                    baseline = e;
                    hit |= e.hit;
                    miss |= !e.hit;
                    critical |= e.critical;
                } else {
                    check(e.roll == baseline.roll && e.hit == baseline.hit &&
                              e.critical == baseline.critical && e.rawDamage == baseline.damage,
                          "Profiles preserve weapon rolls, including critical dice");
                    check(e.damage == (catalog == &immune ? 0 : baseline.damage * 2),
                          "Critical total resolves exactly once");
                    if (catalog == &immune)
                        check(!s.actors().actor(*command.target)->hasDamage,
                              "Immune critical creates no hurt");
                }
            }
            check(found, "Weapon impact present");
        }
    }
    check(hit && miss && critical, "Fixed seeds cover ordinary hit, miss and critical");
}
void speciesFeedback() {
    auto f = partyFixture();
    MonsterDefinition m;
    m.id = "skeleton";
    m.abilities.fill(10);
    m.armorClass = 13;
    m.hitPoints = 13;
    m.attacks[0] = {0};
    m.attackCount = 1;
    Catalog catalog({{"blade", AttackKind::Melee, 4, {1, 6, 2}, DamageType::Piercing}}, {m}, f.races,
                    f.classes, f.items, f.preset);
    const auto map = combatMap();
    const Player pose{{3, 3}, Heading::North};
    NpcWorld places(map, pose.cell, {"skeleton"}, {{{0}, {3, 2}, {0, 1}, Heading::South}});
    GameSession s(map, catalog, places, 5, pose, false);
    CommandController ui(s);
    check(ui.snapshot().enemyPortraits[0] == EnemyPortrait::Skeleton,
          "Initiative portrait has actual species");
    auto targets = allCombatTargets(s);
    targets.anchors[0] = {200, 120, true};
    ui.refresh(targets, false, false, true);
    CombatOutcome out;
    out.event = CombatEvent::Attack;
    out.enemy = s.actors().id(0);
    out.hit = true;
    out.damageType = DamageType::Poison;
    out.damageAdjustment = DamageAdjustment::Immune;
    ui.record(out);
    check(std::string_view(ui.snapshot().log.newest(0)).find("Skeleton #1: immune to poison") !=
              std::string_view::npos,
          "Feedback does not mislabel skeleton as goblin");
}
void undeadAndWeb() {
    const auto map = hybridRoom();
    const auto catalog = profiled(creationCatalog(true), {damageBit(DamageType::Fire), 0}, true);
    const auto places = spellPlacements(map, 1);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    levelParty(s, 3, SpellKind::HoldPerson);
    auto c = spellCommand(s, SpellKind::HoldPerson);
    c.targetCount = 1;
    c.enemies[0] = s.actors().id(0);
    const auto mana = s.party().character(c.character)->mana;
    check(s.submit(c).reason == CommandReason::InvalidTarget &&
              s.party().character(c.character)->mana == mana,
          "Nonhumanoid rejected without cost");
    c = spellCommand(s, SpellKind::Sleep);
    c.aimedCell = Cell{6, 4};
    check(s.combat().area(c, spellSpawn).recipientCount == 0, "Sleep excludes undead");
    check(s.effects().metalArmor(s.actors().id(0)), "Rusted armor participates in existing spell rule");
    Party party(catalog);
    ActorWorld actors(map, catalog, places);
    DiceRng rng(5);
    CombatEvents events;
    EffectController effects(party, actors, rng);
    effects.bindEvents(events, 0);
    effects.partyCell(spellSpawn.cell);
    const Cell cell{6, 4};
    check(effects.createWeb(&cell, 1, {3}, 1000, 0, events), "Web fixture created");
    effects.ignite(cell);
    events.clear();
    effects.advance(6000, 6000, true, events);
    bool found = false;
    for (const auto& e : events)
        if (e.event == CombatEvent::Attack && e.spell == SpellKind::Web) {
            found = true;
            check(e.damageType == DamageType::Fire && e.rawDamage > 0 && e.damage == 0,
                  "Web shares immunity");
        }
    check(found && !actors.actor(actors.id(0))->hasDamage, "Burning web produces no immune hurt");
}
} // namespace
int main() {
    try {
        resolution();
        spellPaths();
        weaponAndAllocation();
        weaponCriticalAndRng();
        undeadAndWeb();
        speciesFeedback();
        std::cout << "Typed damage checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
