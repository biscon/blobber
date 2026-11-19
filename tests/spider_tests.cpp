#include "game/commands.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/progression_fixtures.hpp"
#include <iostream>

using namespace blobber;
using namespace blobber::rules;
namespace {
constexpr Player stance{{3, 3}, Heading::North};
constexpr CharacterId victim{0};
Catalog venomCatalog(int bonus = 4, int dc = 11) {
    auto f = partyFixture();
    ItemDefinition item;
    item.id = "test.antitoxin";
    item.name = "Antitoxin";
    item.description = "Test dose";
    item.kind = ItemKind::Consumable;
    item.consumable = Consumable::Antitoxin;
    f.items.push_back(item);
    MonsterDefinition spider;
    spider.id = "test.spider";
    spider.abilities = {7, 15, 11, 2, 10, 4};
    spider.hitPoints = 11;
    spider.armorClass = 12;
    spider.experience = 50;
    spider.attacks[0] = {0};
    spider.attackCount = 1;
    return {{{"test.bite", AttackKind::Melee, bonus, {1, 4, 2}, DamageType::Piercing, dc}},
            {spider},
            f.races,
            f.classes,
            f.items,
            f.preset};
}
std::uint64_t remaining(const EffectController& effects, CharacterId id = victim) {
    const auto status = effects.statuses(id);
    for (std::size_t i = 0; i < status.count; ++i)
        if (status.entries[i].effect == EffectKind::Poisoned)
            return status.entries[i].remainingMs;
    return 0;
}
void biteImpact() {
    const auto map = combatMap();
    enum Scenario { Miss, Save, Poison, Cancel, Mirror };
    for (auto scenario : {Miss, Save, Poison, Cancel, Mirror}) {
        const auto c = venomCatalog(scenario == Miss ? -100 : 100, scenario == Save ? 1 : 30);
        const NpcWorld placement(map, stance.cell, {"test.spider"}, {{{0}, {3, 2}, {0, 1}, Heading::South}});
        GameSession game(map, c, placement, 8, stance, false);
        for (std::uint16_t i = 1; i < partySize; ++i)
            game.party().damage({i}, 10000);
        if (scenario == Mirror) {
            EffectRequest request;
            request.world = game.actors().snapshot().world;
            request.caster = victim;
            request.targets[0] = victim;
            request.kind = EffectKind::MirrorImage;
            request.count = 1;
            check(bool(game.applyEffect(request)), "Mirror fixture");
        }
        bool windup = false, resolved = false;
        unsigned saves = 0;
        for (unsigned tick = 0; tick < 160 && !resolved; ++tick) {
            const auto& events = game.update(.05, false);
            bool cancel = false;
            for (const auto& event : events) {
                if (event.event == CombatEvent::AttackStarted && event.enemyAttack) {
                    windup = true;
                    cancel = scenario == Cancel;
                    check(!remaining(game.effects()), "No venom during windup");
                }
                if (event.event == CombatEvent::AttackCanceled && event.enemyAttack)
                    resolved = scenario == Cancel;
                if (event.event == CombatEvent::Attack && event.enemyAttack) {
                    resolved = true;
                    check(event.hit == (scenario != Miss), "Expected hit or miss at impact");
                    if (scenario == Mirror)
                        check(event.mirror.effect.serial && !event.damage, "Duplicate intercepted the bite");
                }
                if (event.event == CombatEvent::Effect && event.effect.kind == EffectKind::Poisoned &&
                    event.effect.notice == EffectNotice::InitialSave) {
                    ++saves;
                    check(event.effect.caster.npc &&
                              sameActor(event.effect.caster.enemy, game.actors().id(0)) &&
                              event.effect.succeeded == (scenario == Save),
                          "Tagged venom save");
                }
            }
            if (cancel)
                game.actors().damage(game.actors().id(0), 100, game.timeMs());
            const auto& turn = game.combat().snapshot();
            if (turn.awaitingAction && turn.activeCharacter) {
                auto c = turnCommand(game, PartyAction::Feature);
                c.feature = Feature::EndTurn;
                game.submit(c);
            }
        }
        check(windup && resolved && saves == unsigned(scenario == Save || scenario == Poison),
              "Only a landed bite on a living body rolls one save");
        check(remaining(game.effects()) == (scenario == Poison ? 12000u : 0u),
              "Poison follows a failed save");
        check(game.saveSafety() != nullptr, "Contact and unsettled victory reject a save");
    }
}

void clocksAndSources() {
    const auto map = combatMap();
    const auto c = venomCatalog();
    const NpcWorld placement(map, stance.cell, {"test.spider"}, {{{0}, {1, 1}, {0, 0}}});
    Party party(c);
    ActorWorld actors(map, c, placement);
    DiceRng rng(5);
    EffectController effects(party, actors, rng);
    CombatEvents events;
    const auto source = actors.id(0);
    effects.clockMode(true);
    effects.venomousBite(source, victim, 30, 0, events);
    const auto hp = party.character(victim)->hitPoints;
    check(effects.attackMode(victim, source, true) == RollMode::Disadvantage &&
              effects.attackMode(victim, source, true, true) == RollMode::Normal,
          "Shared attack-roll cancellation");
    watching = true;
    allocations = 0;
    effects.endVictimTurn({1}, 1, events);
    effects.advance(6000, 1, false, events);
    effects.endVictimTurn(victim, 2, events);
    const auto count = events.count;
    effects.venomousBite(source, victim, 30, 3, events);
    watching = false;
    check(!allocations && events.count == count && remaining(effects) == 6000,
          "Only the victim's turn decrements poison and later bites cannot refresh it");
    actors.damage(source, 100, 4);
    effects.reconcile(4, events);
    check(remaining(effects) == 6000, "Killing the spider leaves poison active");
    effects.clockMode(false);
    effects.advance(7500, 1504, true, events);
    check(remaining(effects) == 4500, "Real-time elapsed after disengagement");
    effects.clockMode(true);
    effects.clockMode(false);
    check(remaining(effects) == 4500, "Changing phase preserves a fractional turn");
    effects.clockMode(true);
    effects.endVictimTurn(victim, 1600, events);
    check(!remaining(effects) && hp == party.character(victim)->hitPoints,
          "Natural expiry at the second victim turn without periodic damage");
}

void initiativeOpportunities() {
    const auto map = hybridRoom();
    const auto c = creationCatalog(true);
    const auto actors = spellPlacements(map, 4, true);
    GameSession game(map, c, actors, 7, {{1, 10}, Heading::South}, false);
    levelParty(game, 3);
    auto floor = std::make_unique<FloorState>();
    game.exportFloor(*floor);
    floor->effects.poisonRemaining[0] = floor->effects.poisonRemaining[1] = 12000;
    game.importFloor(*floor);
    game.party().damage({1}, game.party().character({1})->hitPoints);
    game.party().stabilize({1});
    game.restorePose(spellSpawn, false);
    levelReady(game, 0);
    const auto before = remaining(game.effects());
    check(before > 6000, "Other initiatives leave two turns at the fighter's first opportunity");
    check(game.submit(featureCommand(game, 0, Feature::ActionSurge)).reason == CommandReason::Resolved,
          "Poisoned fighter activates Action Surge");
    game.update(1.1, false);
    check(game.submit(turnCommand(game, PartyAction::Defend)).reason == CommandReason::Resolved &&
              remaining(game.effects()) == before,
          "Spending the extra action does not advance the victim clock");
    game.update(1.1, false);
    check(game.submit(featureCommand(game, 0, Feature::EndTurn)).reason == CommandReason::Resolved,
          "Finish the fighter's first initiative");
    levelReady(game, 0);
    check(remaining(game.effects()) == before - 6000, "Exactly one completed fighter initiative counted");
    game.submit(featureCommand(game, 0, Feature::EndTurn));
    levelReady(game, 0);
    check(!remaining(game.effects()) && !remaining(game.effects(), {1}) &&
              game.party().character({1})->life == Life::Stable,
          "Second initiative expires both the acting and the skipped victim's poison");
}
void spellRolls() {
    const auto map = hybridRoom();
    const auto c = spellCatalog(1000, 1, 1);
    for (auto kind : {SpellKind::FireBolt, SpellKind::PoisonSpray}) {
        const bool spray = kind == SpellKind::PoisonSpray;
        const auto actors = spellPlacements(map, spray ? 12 : 3, spray);
        int healthyDamage = 0;
        for (bool poisoned : {false, true}) {
            GameSession game(map, c, actors, 5, {{1, 10}, Heading::South}, false);
            if (poisoned) {
                auto floor = std::make_unique<FloorState>();
                game.exportFloor(*floor);
                floor->effects.poisonRemaining[3] = 12000;
                game.importFloor(*floor);
            }
            game.restorePose(spellSpawn, false);
            if (spray)
                spellReady(game, 3);
            auto cast = spellCommand(game, kind);
            cast.targetCount = 1;
            cast.enemies[0] = game.actors().id(spray ? 6 : 0);
            check(!poisoned || remaining(game.effects(), cast.character), "Caster still has venom");
            check(game.submit(cast).reason == CommandReason::Resolved, "Poison permits spellcasting");
            bool sawImpact = false;
            auto examine = [&](const CombatEvents& events) {
                for (const auto& event : events) {
                    if (event.event != CombatEvent::Attack || event.spell != kind)
                        continue;
                    sawImpact = true;
                    if (spray) {
                        if (poisoned)
                            check(event.damage == healthyDamage && healthyDamage > 0,
                                  "Save-based Poison Spray keeps its damage and dice stream");
                        else
                            healthyDamage = event.damage;
                    } else
                        check(event.attackRollMode == (poisoned ? RollMode::Disadvantage : RollMode::Normal),
                              "Projectile attack uses the caster's poison status at resolution");
                }
            };
            examine(game.events());
            examine(game.update(.5, false));
            check(sawImpact, "Observed the spell result");
        }
    }
}
void inventoryAndRestore() {
    const auto map = combatMap();
    const auto c = venomCatalog();
    const NpcWorld empty(map, stance.cell, {"test.spider"}, {});
    RecoveryLevel ground;
    const auto vial = c.findItem("test.antitoxin");
    ground.ground[ground.groundCount++] = {stance.cell, vial, 1};
    GameSession game(map, c, empty, 3, stance, false, {}, ground);
    const CharacterId holder{1};
    auto& party = game.party();
    const auto& inventory = party.inventory();
    while (party.acquire({victim, {4}, 1, inventory.revision()})) {
    }
    const auto& loot = game.loot().snapshot();
    auto slot = std::find_if(loot.stacks.begin(), loot.stacks.end(), [](const auto& s) { return s.active; });
    check(slot != loot.stacks.end(), "Placed dose");
    const LootId bag{loot.world, std::uint16_t(slot - loot.stacks.begin()), slot->generation};
    check(!game.pickup({bag, victim, inventory.revision()}) && game.loot().find(bag),
          "Full bag leaves camp loot intact");
    check(bool(game.pickup({bag, holder, inventory.revision()})) && !game.loot().find(bag),
          "Another party member collects the unique dose");
    PartyCommand use = turnCommand(game, PartyAction::Item);
    use.character = holder;
    use.ally = victim;
    use.inventoryRevision = inventory.revision();
    for (std::uint16_t i = 0; i < bagCells; ++i) {
        auto handle = inventory.at({holder, i});
        auto stack = inventory.stack(handle);
        if (stack && stack->item.index == vial.index)
            use.item = handle;
    }
    check(game.submit(use).reason == CommandReason::InvalidTargets && inventory.stack(use.item),
          "An unaffected ally costs no item or action");
    auto saved = std::make_unique<FloorState>();
    game.exportFloor(*saved);
    saved->effects.poisonRemaining[0] = 10001;
    check(!game.saveSafety(), "Settled exploration admits checkpoints");
    game.importFloor(*saved);
    game.update(2, true);
    check(remaining(game.effects()) == 10001, "Pause freezes restored poison");
    game.update(1, false);
    check(remaining(game.effects()) == 9001 && !game.loot().find(bag),
          "Exact elapsed time and spent camp stack survive restore");
    use.inventoryRevision--;
    check(game.submit(use).reason == CommandReason::StaleInventory,
          "Obsolete item request rejects without cost");
    use.inventoryRevision = inventory.revision();
    use.character = victim;
    check(game.submit(use).reason == CommandReason::InvalidItem, "Other bag cannot supply the dose");
    use.character = holder;
    game.party().damage(victim, game.party().character(victim)->hitPoints);
    check(game.submit(use).reason == CommandReason::Resolved && !remaining(game.effects()) &&
              !inventory.stack(use.item) && !game.party().character(victim)->hitPoints,
          "One dose cures a downed living ally");
    check(game.submit(use).reason != CommandReason::Resolved, "Double use rejected");
    saved->effects.poisonRemaining[0] = 12001;
    rejects([&] { game.importFloor(*saved); }, "Malformed duration rejected");
}
void restoration() {
    const auto map = hybridRoom();
    const auto c = creationCatalog(true);
    const NpcWorld empty(map, spellSpawn.cell, {"goblin"}, {});
    GameSession game(map, c, empty, 5, spellSpawn, false);
    levelParty(game, 3, SpellKind::LesserRestoration);
    auto saved = std::make_unique<FloorState>();
    game.exportFloor(*saved);
    saved->effects.poisonRemaining[0] = 8000;
    game.importFloor(*saved);
    auto cast = spellCommand(game, SpellKind::LesserRestoration);
    cast.targetCount = 1;
    cast.allies[0] = victim;
    const auto mana = game.party().character(cast.character)->mana;
    check(game.submit(cast).reason == CommandReason::InvalidTargets, "Wrong condition costs nothing");
    cast.restoration = RestorationChoice::Poison;
    check(game.submit(cast).reason == CommandReason::Resolved && !remaining(game.effects()) &&
              game.party().character(cast.character)->mana == mana - 3,
          "Restoration cures poison for its normal cost");
}
} // namespace
int main() try {
    biteImpact();
    clocksAndSources();
    initiativeOpportunities();
    spellRolls();
    inventoryAndRestore();
    restoration();
    std::cout << "Spider bite, poison clocks, cures and checkpoint checks passed\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
