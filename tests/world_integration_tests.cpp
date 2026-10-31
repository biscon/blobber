#include "render/world_effects.hpp"
#include "support/allocations.hpp"
#include "support/automation_fixtures.hpp"
#include "support/check.hpp"
#include "support/fixture_world.hpp"
#include "support/spell_parties.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <chrono>
#include <iostream>
using namespace blobber;
using namespace blobber::world_render;
namespace {
void barriers() {
    auto map = fixtureMap();
    const auto cat = fixtureCatalog();
    NpcWorld empty(map, {3, 3}, {"goblin"}, {});
    GameSession game(map, cat, empty, 5, {{3, 3}, Heading::East}, false, {}, {}, fixtureDefinitions(false));
    check(game.interact(fixtureRequest(game, 0)).changed && game.opening(), "opening starts shared barrier");
    check(!game.geometry().solid({4, 3}) && game.doors().active(),
          "logical completion precedes settled legacy presentation");
    check(game.saveSafety() && game.travelSafety(), "save/travel reject unsettled opening");
    const auto start = game.movement().player;
    game.move(Action::Forward);
    game.update(.1, false);
    check(game.movement().player == start && game.opening(), "fresh movement queues during opening");
    check(!game.interact(fixtureRequest(game, 0), true), "fixture preview shares barrier");
    game.update(.1, true);
    check(game.timeMs() == 100 && game.opening() && !game.doorPending(),
          "pause freezes hinge and clears input");
    game.move(Action::Forward);
    game.update(.25, false);
    check(!game.opening() && game.movement().player.cell == Cell{4, 3}, "queued step starts after opening");
    check(std::abs(game.movement().transition.elapsed - .05f) < .0001f,
          "only unblocked remainder advances movement");
    GameSession off(map, cat, empty, 5, {{3, 3}, Heading::East}, false, {}, {}, fixtureDefinitions(false));
    off.interact(fixtureRequest(off, 0));
    off.setAnimated(false);
    check(!off.opening() && !off.geometry().solid({4, 3}), "animation off settles opening");
    off.move(Action::Forward);
    check(off.movement().player.cell == Cell{4, 3},
          "settled opening permits immediate animation-off movement");
}
void restoredDoorClock() {
    auto map = fixtureMap();
    const auto cat = fixtureCatalog();
    NpcWorld placements(map, {3, 3}, {"goblin"}, {});
    GameSession original(map, cat, placements, 5, {{3, 3}, Heading::East}, false, {}, {},
                         fixtureDefinitions(false));
    original.update(1.2, false);
    auto checkpoint = std::make_unique<FloorState>();
    original.exportFloor(*checkpoint);
    GameSession restored(map, cat, placements, 5, {{3, 3}, Heading::East}, false, {}, {},
                         fixtureDefinitions(false));
    restored.importFloor(*checkpoint);
    check(restored.interact(fixtureRequest(restored, 0)).changed, "Restored session opens remaining door");
    check(restored.doors().progress(0) == 0, "Restored clock starts hinge at closed pose");
    restored.move(Action::Forward);
    restored.setAnimated(false);
    check(restored.saveSafety() && restored.travelSafety(),
          "Settling animation does not hide pending movement from save/travel");
    restored.clearPending();
    check(!restored.saveSafety(), "Clearing settled input restores save eligibility");
}
void acceptedImpacts() {
    const auto cat = automation::test::catalog(false, 50, 50);
    const auto defs = automation::test::definitions();
    const auto map = fixtureMap();
    FloorFixtures fixtures;
    fixtures.count = 2;
    fixtures.entries[0] = {{4, 3}, Heading::East, FixtureKind::Door, 0};
    fixtures.entries[1] = {{6, 3}, Heading::East, FixtureKind::GuardianGate, 0};
    FloorObjectiveDefinition objective;
    objective.count = 1;
    objective.members[0] = 1;
    objective.approachDoor = 0;
    objective.gate = 1;
    NpcWorld placements(map, {2, 3}, defs.types,
                        {{{0}, {2, 1}, {0, 0}, Heading::South}, {{0}, {7, 3}, {0, 0}, Heading::West}});
    GameSession game(map, cat, placements, 5, {{2, 3}, Heading::North}, false, defs.timings, {}, fixtures,
                     objective);
    PartyCommand command;
    command.character = {2};
    command.action = PartyAction::Ranged;
    command.target = game.actors().id(0);
    command.world = game.combat().snapshot().world;
    command.turn = game.combat().snapshot().turn;
    check(game.submit(command).reason == CommandReason::Resolved,
          "Accepted ranged attack precedes guardian gate opening");
    const auto due = game.combat().attacks().attacks[NpcWorld::maxInstances + 2].impactMs;
    game.actors().damage(game.actors().id(1), 10000, game.timeMs());
    game.update(0, false);
    check(game.opening(), "Guardian completion starts shared barrier during accepted flight");
    check(game.submit(command).reason == CommandReason::Moving,
          "Barrier rejects new attacks without consuming a turn");
    game.update(double(due) / 1000, false);
    bool impact = false;
    for (unsigned i = 0; i < game.events().count; ++i)
        impact |= game.events().values[i].event == CombatEvent::Attack;
    check(impact, "Previously accepted impact still resolves during opening");
}
void doorsAndEffects() {
    test::VisibilityFixture fixtures;
    world_assets::CpuAssets assets(fixtures.publication());
    auto map = fixtureMap();
    const auto cat = fixtureCatalog();
    NpcWorld empty(map, {3, 3}, {"goblin"}, {});
    GameSession game(map, cat, empty, 5, {{3, 3}, Heading::East}, false, fixtures.timings, {},
                     fixtureDefinitions(false));
    auto scene = prepare(map);
    VisibilityPipeline visibility(scene, assets);
    EffectsPipeline effects(assets);
    NpcAnimator animator(game.actors(), fixtures.timings);
    SpellArea area;
    area.ground.revision = 1;
    area.ground.areas[0].active = true;
    area.ground.areas[0].web = true;
    area.ground.areas[0].burnMs = 1;
    area.ground.areas[0].cell = {3, 3};
    area.ground.weapons[0] = {{5, 3}, true};
    AttackVisualSnapshot attacks;
    attacks.world = game.actors().snapshot().world;
    attacks.revision = 1;
    auto& shot = attacks.attacks[0];
    shot.id = {attacks.world, 1};
    shot.stage = AttackStage::Flight;
    shot.ranged = shot.hit = true;
    shot.partyOrigin = game.movement().player;
    shot.targetPosition = {5.5f, 3.5f};
    shot.impactMs = 200;
    shot.endMs = 400;
    auto input = [&](std::uint64_t now) {
        animator.setTime(game.timeMs());
        return FrameInput{game.geometry(),
                          game.movement(),
                          &game.actors(),
                          &animator.snapshot(),
                          &game.loot().snapshot(),
                          &game.doors(),
                          &attacks,
                          &area,
                          now};
    };
    auto in = input(100);
    visibility.update(in);
    check(visibility.frame().doorCount == 4 && visibility.picking(),
          "closed slab and three frame pieces prepared");
    const auto original = visibility.frame().doors[3];
    effects.update(in, visibility.frame());
    check(effects.frame().count == 4, "ground, burn, weapon and projectile all present without targeting");
    const auto revision = visibility.frame().revision;
    in.now = 120;
    check(!visibility.update(in) && effects.update(in, visibility.frame()),
          "effects animate independently of visibility");
    check(visibility.frame().revision == revision, "effect-only changes leave CPU visibility cached");
    const auto depthBefore = visibility.frame().picking.depth(100);
    effects.update(in, visibility.frame());
    check(visibility.frame().picking.depth(100) == depthBefore, "effects cannot change picking depth");
    allocations = 0;
    watching = true;
    for (unsigned tick = 121; tick < 180; ++tick) {
        in.now = tick;
        visibility.update(in);
        effects.update(in, visibility.frame());
    }
    watching = false;
    check(allocations == 0, "Effect-only frame processing allocates no C++ storage");
    auto foreign = game.doors();
    ++foreign.world;
    auto invalid = input(150);
    invalid.doors = &foreign;
    rejects([&] { visibility.update(invalid); }, "Foreign door snapshot rejects before cache reuse");
    game.interact(fixtureRequest(game, 0));
    game.update(.15, false);
    auto half = input(150);
    visibility.update(half);
    check(!visibility.picking() && visibility.frame().doorCount == 4,
          "hinge animation suspends actionable picking");
    const auto partial = visibility.frame().doors[3];
    check(std::abs(partial.x.x - original.x.x) > .01f || std::abs(partial.x.z - original.x.z) > .01f,
          "shared slab rotates at midpoint");
    for (const auto& p : corners(partial))
        check(p.x >= 4 - .0001f && p.x <= 5.0001f && p.z >= 3 - .0001f && p.z <= 4.0001f,
              "slab swept volume stays inside fixture cell");
    game.update(.15, false);
    auto end = input(300);
    visibility.update(end);
    check(visibility.picking() && visibility.frame().doors[3].id == PickingBuffer::empty,
          "settled leaf is opaque without a picking ID");
}
void authoredFixtureKnowledge() {
    const auto map = fixtureMap();
    const auto cat = fixtureCatalog();
    NpcWorld empty(map, {3, 3}, {"goblin"}, {});
    auto fixtures = fixtureDefinitions(false);
    fixtures.discovery = false;
    GameSession game(map, cat, empty, 5, {{3, 3}, Heading::East}, false, {}, {}, fixtures);
    check(game.fixtures().known(0) && game.fixtures().known(1),
          "Authored fixtures without discovery are available to visual observations");
    check(game.interact(fixtureRequest(game, 0)).changed && game.fixtures().rememberedComplete(0),
          "Authored fixture observations report authoritative completion without discovery");
}
void spellVisuals() {
    test::VisibilityFixture fixture;
    world_assets::CpuAssets assets(fixture.publication());
    const auto map = test::worldRoom(9, 9);
    const auto cat = fixtureCatalog();
    const Player spawn{{4, 6}, Heading::North};
    NpcWorld placement(map, spawn.cell, {"goblin"}, {{{0}, {4, 3}, {0, 0}, Heading::South}});
    GameSession game(map, cat, placement, 5, spawn, false, fixture.timings);
    auto scene = prepare(map);
    VisibilityPipeline visibility(scene, assets);
    EffectsPipeline effects(assets);
    NpcAnimator animator(game.actors(), fixture.timings);
    animator.setTime(0);
    AttackVisualSnapshot attacks;
    attacks.world = game.actors().snapshot().world;
    attacks.revision = 1;
    auto& shot = attacks.attacks[NpcWorld::maxInstances + 3];
    shot.id = {attacks.world, 1};
    shot.enemy = game.actors().id(0);
    shot.character = {3};
    shot.stage = AttackStage::Flight;
    shot.ranged = shot.hit = true;
    shot.impactMs = 200;
    shot.endMs = 320;
    shot.partyOrigin = spawn;
    shot.spell = rules::SpellKind::ScorchingRay;
    shot.targetPosition = actorPosition(game.actors().snapshot().actors[0], 0);
    attacks.spellExtras[6] = shot;
    attacks.spellExtras[6].id.serial = 2;
    attacks.spellExtras[7] = shot;
    attacks.spellExtras[7].id.serial = 3;
    FrameInput input{game.geometry(),
                     game.movement(),
                     &game.actors(),
                     &animator.snapshot(),
                     nullptr,
                     nullptr,
                     &attacks,
                     nullptr,
                     100};
    visibility.update(input);
    effects.update(input, visibility.frame());
    check(effects.frame().count == 3, "Three simultaneous Scorching Rays retain three visible glyphs");
    auto center = [](const EffectPacket& p) {
        return Vec{(p.points[0].x + p.points[2].x) * .5f, (p.points[0].y + p.points[2].y) * .5f,
                   (p.points[0].z + p.points[2].z) * .5f};
    };
    std::array<float, 3> xs{};
    for (unsigned i = 0; i < 3; ++i) {
        const auto& p = effects.frame().packets[i];
        check(p.tint[0] == 1 && std::abs(p.tint[1] - 114.f / 255) < .0001f && p.tint[2] < .15f,
              "Fire spells retain fiery tint instead of generic gold");
        xs[i] = project(visibility.frame().camera, center(p)).x;
    }
    std::sort(xs.begin(), xs.end());
    check(xs[1] - xs[0] > 5.9f && xs[2] - xs[1] > 5.9f,
          "Rays assigned to one target remain visually distinct");
    attacks.spellExtras = {};
    ++attacks.revision;
    shot.stage = AttackStage::Resolved;
    input.now = 200;
    effects.update(input, visibility.frame());
    check(effects.frame().count == 1, "Hit effect exists at impact");
    const auto impact = project(visibility.frame().camera, center(effects.frame().packets[0]));
    const auto& body = visibility.frame().billboards[0];
    check(impact.depth < body.depth && body.depth - impact.depth < .003f,
          "Impact sits just ahead of target depth to avoid clipping itself");
    input.now = 260;
    effects.update(input, visibility.frame());
    check(std::abs(effects.frame().packets[0].tint[3] - .5f) < .0001f,
          "Impact fades halfway through its existing 120 ms tail");
    input.now = 320;
    effects.update(input, visibility.frame());
    check(!effects.frame().count, "Impact disappears at its actual expiry");
    shot.spell = rules::SpellKind::BurningHands;
    shot.ranged = false;
    shot.impactMs = shot.releaseMs = shot.startedMs = 400;
    shot.endMs = 520;
    ++attacks.revision;
    input.now = 400;
    effects.update(input, visibility.frame());
    check(effects.frame().count == 1 && effects.frame().packets[0].tint[0] == 1,
          "Burning Hands emits a fiery immediate impact pulse");
    input.now = 519;
    effects.update(input, visibility.frame());
    check(effects.frame().count == 1, "Burning Hands pulse remains until its last millisecond");
    input.now = 520;
    effects.update(input, visibility.frame());
    check(!effects.frame().count, "Burning Hands aftermath has no expired pulse");
    shot.spell = rules::SpellKind::MagicMissile;
    shot.ranged = true;
    shot.stage = AttackStage::Flight;
    shot.releaseMs = 0;
    shot.impactMs = 200;
    shot.darts = 3;
    ++attacks.revision;
    input.now = 100;
    effects.update(input, visibility.frame());
    check(effects.frame().count == 3 && effects.frame().packets[0].tint[2] == 1,
          "A multi-dart record emits all blue Magic Missile glyphs");
    SpellArea area;
    area.ground.weapons[0] = {{4, 3}, true};
    area.ground.revision = 1;
    input.attacks = nullptr;
    input.area = &area;
    effects.update(input, visibility.frame());
    check(effects.frame().count == 1, "Persistent spiritual weapon is present without transient spells");
    for (const auto& p : effects.frame().packets[0].points)
        check(p.y > .17f && p.y < .83f, "Spiritual Weapon clears both the floor and flat ceiling");
}
void captureBuilds() {
    const auto catalog = automation::test::catalog();
    for (unsigned i = 0; i < unsigned(rules::SpellKind::Count); ++i) {
        const auto kind = rules::SpellKind(i);
        Party party(catalog, leveledSpellParty(catalog, kind));
        prepareSpellParty(party, kind);
        const auto* rule = rules::spellRules(kind);
        const rules::CharacterId id{std::uint16_t(rule->casting == rules::Casting::Wisdom ? 1 : 3)};
        check(bool(party.spellAccess(id, catalog.findSpell(rule->id))),
              "Every current spell has a legal capture build");
    }
}
void stressEffects() {
    test::VisibilityFixture fixture;
    world_assets::CpuAssets assets(fixture.publication());
    auto map = fixtureMap();
    const auto cat = fixtureCatalog();
    NpcWorld empty(map, {3, 3}, {"goblin"}, {});
    GameSession game(map, cat, empty, 5, {{3, 3}, Heading::East}, false, fixture.timings);
    EffectsPipeline effects(assets);
    WorldFrame world;
    world.camera = camera(game.movement());
    world.revision = 1;
    SpellArea area;
    area.ground.revision = 1;
    area.eligible.fill(true);
    area.cells.fill({3, 3});
    for (auto& patch : area.ground.areas) {
        patch.active = patch.web = true;
        patch.burnMs = 1;
        patch.cell = {3, 3};
    }
    for (auto& weapon : area.ground.weapons)
        weapon = {{5, 3}, true};
    AttackVisualSnapshot attacks;
    attacks.world = game.actors().snapshot().world;
    attacks.revision = 1;
    for (unsigned i = 0; i < attacks.attacks.size(); ++i) {
        auto& shot = attacks.attacks[i];
        shot.id = {attacks.world, i + 1};
        shot.ranged = shot.hit = true;
        shot.stage = AttackStage::Flight;
        shot.impactMs = 1000;
        shot.endMs = 1200;
        shot.partyOrigin = game.movement().player;
        shot.sourcePosition = shot.targetPosition = {6.5f, 3.5f};
        shot.incoming = i < NpcWorld::maxInstances;
        shot.darts = shot.incoming ? 0 : 3;
    }
    FrameInput input{game.geometry(), game.movement(), &game.actors(), nullptr, nullptr,
                     nullptr,         &attacks,        &area,          200};
    effects.update(input, world);
    check(effects.frame().count == effectCapacity,
          "Maximum ground/weapon/projectile capacity retains every packet");
    for (unsigned i = 0; i < 120; ++i) {
        input.now = 200 + i;
        effects.update(input, world);
    }
    std::array<double, 600> samples{};
    for (unsigned trial = 0; trial < 3; ++trial) {
        allocations = 0;
        watching = true;
        for (unsigned i = 0; i < samples.size(); ++i) {
            input.now = 200 + i;
            const auto start = std::chrono::steady_clock::now();
            effects.update(input, world);
            samples[i] =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        }
        watching = false;
        check(allocations == 0, "Maximum effect packet workload allocates no C++ storage");
        std::sort(samples.begin(), samples.end());
        std::cout << "Effect-only maximum (313 packets), trial " << trial << ": p50_ms=" << samples[300]
                  << " p95_ms=" << samples[570] << " p99_ms=" << samples[594]
                  << " allocations=" << allocations << '\n';
    }
}
} // namespace
int main(int argc, char**) {
    try {
        barriers();
        doorsAndEffects();
        acceptedImpacts();
        restoredDoorClock();
        captureBuilds();
        spellVisuals();
        authoredFixtureKnowledge();
        if (argc > 1)
            stressEffects();
        std::cout << "R04 barriers, shared geometry and effect invariants passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
