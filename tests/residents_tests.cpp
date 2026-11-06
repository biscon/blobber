#include "game/damage_resolution.hpp"
#include "game/target_visibility.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <iostream>
using namespace blobber;
namespace {
NpcInstance resident(Cell cell = {4, 4}) {
    NpcInstance npc{{0}, cell, {0, 0}, Heading::South};
    npc.role = NpcRole::Resident;
    return npc;
}
NpcTimings residentTimings() {
    NpcTimings t{};
    t[0] = {{{80, 120, 0, 0}}, 2};
    t[unsigned(NpcAnimation::Talk)] = {{{40, 60, 80, 100}}, 4};
    return t;
}
rules::Catalog residentCatalog() {
    const auto base = spellCatalog();
    std::vector<rules::AttackDefinition> attacks;
    std::vector<rules::MonsterDefinition> monsters;
    std::vector<rules::RaceDefinition> races;
    std::vector<rules::ClassDefinition> classes;
    std::vector<rules::ItemDefinition> items;
    std::vector<rules::SpellDefinition> spells;
    for (std::uint16_t i = 0; i < base.attackCount(); ++i)
        attacks.push_back(*base.attack({i}));
    for (std::uint16_t i = 0; i < base.monsterCount(); ++i)
        monsters.push_back(*base.monster({i}));
    for (std::uint16_t i = 0; i < base.raceCount(); ++i)
        races.push_back(*base.race({i}));
    for (std::uint16_t i = 0; i < base.classCount(); ++i)
        classes.push_back(*base.characterClass({i}));
    for (std::uint16_t i = 0; i < base.itemCount(); ++i)
        items.push_back(*base.item({i}));
    for (unsigned i = 0; i < unsigned(rules::SpellKind::Count); ++i) {
        const auto kind = rules::SpellKind(i);
        const auto& spell = *rules::spellRules(kind);
        spells.push_back({spell.id, spell.name, spell.description, kind});
    }
    return {attacks, monsters, races, classes, items, *base.preset(), spells};
}
void peacefulActors() {
    const auto map = test::worldRoom(9, 9);
    const auto catalog = residentCatalog();
    const Player pose{{4, 5}, Heading::North};
    const NpcWorld places(map, pose.cell, {"resident"}, {resident()});
    GameSession game(map, catalog, places, 5, pose, true, {residentTimings()});
    auto& actors = game.actors();
    rules::DiceRng rng(5);
    EffectController effects(game.party(), actors, rng);
    const auto id = actors.id(0);
    const auto& actor = *actors.actor(id);
    check(!actor.hostile() && !catalog.monster(actor.monster) && !actor.hitPoints && !actor.maxHitPoints &&
              actors.appearanceId(actor.npc.type) == "resident",
          "Resident has an owned appearance identity without monster rules or fabricated HP");
    check(actors.occupied({4, 4}), "Resident blocks walking through his body");
    const auto revision = actors.snapshot().revision;
    check(actors.move(id, {3, 4}, {0, 0}, pose.cell).error == ActorError::NotCombatant &&
              actors.walk(id, {3, 4}, {0, 0}, pose.cell, 0).error == ActorError::NotCombatant &&
              actors.face(id, Heading::North).error == ActorError::NotCombatant &&
              actors.attack(id, 0).error == ActorError::NotCombatant &&
              actors.damage(id, 100, 0).error == ActorError::NotCombatant &&
              actors.remove(id).error == ActorError::NotCombatant,
          "Stationary resident rejects monster movement, attack, damage and removal");
    check(damageNpc(actors, effects, id, 100, rules::DamageType::Fire, false, 0).actor.error ==
                  ActorError::NotCombatant &&
              actors.snapshot().revision == revision,
          "Typed damage rejects before monster rules access and preserves actor state");
    game.setAnimated(false);
    game.move(Action::Forward);
    check(game.movement().player == pose, "Party cannot enter the occupied resident cell");
    for (unsigned i = 0; i < 20; ++i)
        game.update(.1, false);
    check(actor.npc.cell == Cell{4, 4} && actor.npc.facing == Heading::South && !actor.hasAttack &&
              game.ai().actors[0].awareness == Awareness::Routine &&
              game.combat().snapshot().phase == CombatPhase::Exploration &&
              !game.combat().adjacent(pose.cell) && !game.combat().occupiedFront(pose) &&
              !game.combat().snapshot().knownDirections && game.combat().safeRecovery(pose),
          "Adjacent resident never detects, pursues, starts combat or prevents safe recovery");
    NpcAiController ai(map, actors, 5);
    ai.alert(id, pose.cell);
    check(!ai.aware(id) && ai.position(id, pose, 0) == PositioningResult::Blocked && !ai.escape(id, pose, 0),
          "Direct AI commands cannot recruit or move a resident");
    check(!game.effects().canAct(id) && game.effects().armorClass(id) == 0,
          "Combat effects do not treat residents as combat recipients");
    for (auto kind :
         {rules::SpellKind::FireBolt, rules::SpellKind::HoldPerson, rules::SpellKind::MagicMissile}) {
        auto command = spellCommand(game, kind);
        command.enemies[0] = id;
        command.targetCount = 1;
        check(game.combat().spellRecipient(command, pose, 0) == CommandReason::InvalidTarget,
              "Direct hostile spell recipient rejects resident before monster rules access");
    }
    for (auto kind : {rules::SpellKind::BurningHands, rules::SpellKind::Sleep, rules::SpellKind::Grease,
                      rules::SpellKind::ColorSpray, rules::SpellKind::Shatter, rules::SpellKind::Web})
        check(spellArea(actors, game.party(), game.effects(), pose, kind, Cell{4, 4}).count == 0,
              "Area spell footprints omit peaceful residents");
    CombatEvents events;
    EffectRequest bane;
    bane.world = actors.snapshot().world;
    bane.caster = rules::CharacterId{1};
    bane.kind = EffectKind::Bane;
    bane.targets[0] = id;
    bane.count = 1;
    check(!effects.apply(bane, game.timeMs(), events), "Direct condition effect rejects resident");
    CommandController commands(game, {residentTimings()});
    auto forged = allCombatTargets(game);
    forged.candidates[forged.candidateCount++] = id;
    commands.refresh(forged, false, false, true);
    commands.hover(HoverKind::Enemy, 0, id);
    check(!commands.snapshot().targets.count && !commands.snapshot().targets.candidateCount &&
              commands.snapshot().hover.kind == HoverKind::None,
          "HUD discards forged resident target lists and enemy hover");
    PartyCommand ranged;
    ranged.action = PartyAction::Ranged;
    ranged.target = id;
    check(targetVisibility(ranged, catalog, forged, actors.snapshot()) == CommandReason::NotVisible,
          "Target visibility independently excludes forged resident candidates");
    NpcAnimator animation(actors, {residentTimings()});
    check(actors.talk({}, true).error == ActorError::InvalidHandle, "Talk checks stable actor handles");
    check(actors.talk(id, true).changed && !actors.talk(id, true).changed,
          "Talk changes only the resident presentation loop and is idempotent");
    for (const auto time : {0u, 39u, 40u, 99u, 100u, 179u, 180u, 279u, 280u}) {
        animation.setTime(time);
        const unsigned expected = time < 40 ? 0 : time < 100 ? 1 : time < 180 ? 2 : time < 280 ? 3 : 0;
        check(animation.snapshot().frames[0].clip == NpcAnimation::Talk &&
                  animation.snapshot().frames[0].active == expected,
              "Talk obeys frame boundaries and loops");
    }
    auto idleOnly = residentTimings();
    idleOnly[unsigned(NpcAnimation::Talk)] = {};
    actors.talk(id, false);
    NpcAnimator fallback(actors, {idleOnly});
    actors.talk(id, true);
    fallback.setTime(200);
    check(fallback.snapshot().frames[0].clip == NpcAnimation::Idle,
          "Optional absent talk art safely falls back to idle");
    NpcAnimator rebuiltFallback(actors, {idleOnly});
    check(rebuiltFallback.snapshot().frames[0].clip == NpcAnimation::Idle,
          "Rebuilding presentation while optional talk art is absent preserves idle fallback");
    const auto clock = game.timeMs();
    game.update(.5, true);
    check(game.timeMs() == clock, "Pause preserves the resident animation clock");
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 32; ++i) {
        actors.talk(id, i % 2);
        game.update(.016, false);
        animation.setTime(game.timeMs(), &game.effects());
        commands.refresh({}, false, false, true);
    }
    watching = false;
    check(allocations == 0, "Warmed resident simulation, talk and HUD processing allocate no C++ storage");
    auto invalid = resident();
    invalid.ai.routine = NpcRoutine::Wander;
    rejects([&] { NpcWorld bad(map, pose.cell, {"resident"}, {invalid}); },
            "Residents cannot have roaming routines");
    invalid = resident();
    invalid.monster = rules::MonsterId{0};
    rejects([&] { NpcWorld bad(map, pose.cell, {"resident"}, {invalid}); },
            "Residents cannot carry monster overrides");
}
void mixedPopulation() {
    const auto map = test::worldRoom(9, 9);
    const auto catalog = residentCatalog();
    const Player pose{{4, 6}, Heading::North};
    const NpcWorld places(map, pose.cell, {"resident", "goblin"},
                          {resident(), {{1}, {4, 5}, {1, 1}, Heading::South}});
    GameSession game(map, catalog, places, 5, pose, false);
    check(game.combat().snapshot().enemyCount == 1 && game.combat().member(game.actors().id(1)) &&
              !game.combat().member(game.actors().id(0)),
          "A mixed population enrolls only the hostile actor in combat");
    CommandController commands(game);
    commands.refresh(allCombatTargets(game), false, false, true);
    check(commands.snapshot().targets.count == 1 && commands.snapshot().targets.ids[0].slot == 1,
          "Resident filtering preserves ordinary hostile targets");
    check(game.actors().damage(game.actors().id(1), 1, 0).changed,
          "Hostile actors retain their ordinary damage behavior");
}
void renderedTalkAndPicking() {
    test::VisibilityFixture fixture;
    auto publication = fixture.publication();
    auto& appearance = publication.appearances[0];
    appearance.id = "resident";
    for (unsigned i = 1; i < npcAnimationCount; ++i)
        if (i != unsigned(NpcAnimation::Talk))
            appearance.clips[i] = {};
    for (unsigned side = 1; side < 4; ++side)
        appearance.clips[unsigned(NpcAnimation::Talk)].textures[side].fill(UINT16_MAX);
    world_assets::CpuAssets assets(std::move(publication));
    const auto lease = assets.acquire(world_assets::allAssets(assets.publication));
    const auto map = test::worldRoom(9, 9);
    const auto scene = world_render::prepare(map);
    const auto catalog = spellCatalog();
    const NpcWorld places(map, {4, 6}, {"resident"}, {resident()});
    ActorWorld actors(map, catalog, places);
    NpcTimings timings{};
    for (unsigned clip = 0; clip < npcAnimationCount; ++clip)
        timings[clip] = assets.publication.appearances[0].clips[clip].timing;
    NpcAnimator animator(actors, {timings});
    actors.talk(actors.id(0), true);
    animator.setTime(100);
    world_render::VisibilityPipeline pipeline(scene, assets);
    const Player views[]{
        {{4, 6}, Heading::North}, {{4, 2}, Heading::South}, {{2, 4}, Heading::East}, {{6, 4}, Heading::West}};
    const unsigned sides[]{0, 1, 3, 2};
    for (unsigned view = 0; view < 4; ++view) {
        MovementState movement;
        movement.player = views[view];
        pipeline.update({map, movement, &actors, &animator.snapshot()});
        bool found = false;
        for (unsigned i = 0; i < pipeline.frame().count; ++i) {
            const auto& b = pipeline.frame().billboards[i];
            if (b.id != 0 || !b.alive)
                continue;
            found = true;
            const auto clip = view == 0 ? NpcAnimation::Talk : NpcAnimation::Idle;
            check(b.texture ==
                      assets.publication.appearances[0].clips[unsigned(clip)].textures[sides[view]][1],
                  "Front uses talking art; all other directions use the current directional idle frame");
        }
        check(found && pipeline.picking()->targets().anchors[0].valid &&
                  !pipeline.picking()->targets().count && !pipeline.picking()->targets().candidateCount,
              "Resident remains visible and occluding while absent from offensive picking");
        allocations = 0;
        watching = true;
        pipeline.update({map, movement, &actors, &animator.snapshot()});
        watching = false;
        check(allocations == 0, "Warmed resident visibility uses existing fixed frame storage");
    }
}
} // namespace
int main() {
    try {
        peacefulActors();
        mixedPopulation();
        renderedTalkAndPicking();
        std::cout << "Peaceful resident checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
