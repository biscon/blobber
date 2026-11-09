#pragma once
#include "assets/image.hpp"
#include "assets/rules_loader.hpp"
#include "generation/population.hpp"
#include "generation_blueprints.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include <iostream>
namespace blobber::test {
inline void underkeepReview(const std::filesystem::path& source, const std::filesystem::path& output,
                            bool corrections = false, bool props = false, bool rackOnly = false) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Review output must be new");
    const auto catalog = loadRules(source / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(source / "npcs/level.json");
    Window window(1920, 1080, true);
    GpuResources resources;
    {
        Presenter presenter(resources, source);
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, world_assets::load(source / "prepared/world"));
            GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
            auto capture = [&](GameSession& game, const std::string& name, bool expectChest = false,
                               int expectActor = -1) {
                NpcAnimator animator(game.actors(), definitions.timings);
                animator.setTime(game.timeMs());
                GameView view(renderer);
                view.update(game, animator.snapshot(), {});
                CommandController commands(game, definitions.timings);
                commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, false, false,
                                 true);
                view.present(presenter, game, game.geometry().base(), false, true, &commands.snapshot());
                presenter.capture(output / (name + ".png"));
                using Error = unsigned (*)();
                check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0, "Underkeep GL draw");
                renderer.diagnosticIds(ids.get());
                OwnedImage raster(LoadImageFromTexture(resources.target(ids.get()).texture));
                check(raster.value.data, "Underkeep GPU ID readback");
                ImageFlipVertical(&raster.value);
                ImageFormat(&raster.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                const auto* pixels = static_cast<const unsigned char*>(raster.value.data);
                const auto& picking = renderer.visibility()->frame().picking;
                unsigned cpuChest = 0, gpuChest = 0, cpuActor = 0, gpuActor = 0, interiorMismatch = 0;
                for (int y = 1; y < viewHeight - 1; ++y)
                    for (int x = 1; x < viewWidth - 1; ++x) {
                        const auto at = (y * viewWidth + x) * 4;
                        const auto actual = std::uint16_t(pixels[at] + 256 * pixels[at + 1]),
                                   expected = picking.slot(x, y);
                        cpuChest += expected >= PickingBuffer::containerBase &&
                                    expected < PickingBuffer::containerBase + containerCapacity;
                        gpuChest += actual >= PickingBuffer::containerBase &&
                                    actual < PickingBuffer::containerBase + containerCapacity;
                        cpuActor += expectActor >= 0 && expected == expectActor;
                        gpuActor += expectActor >= 0 && actual == expectActor;
                        if (actual == expected)
                            continue;
                        bool edge = false;
                        for (int dy = -1; dy <= 1; ++dy)
                            for (int dx = -1; dx <= 1; ++dx)
                                edge |= picking.slot(x + dx, y + dy) != expected;
                        if (!edge)
                            ++interiorMismatch;
                    }
                check(interiorMismatch < 100, "Underkeep CPU/GPU interior IDs agree");
                if (expectChest)
                    check(cpuChest > 10 && gpuChest > 10, "Chest mesh is pickable on CPU and GPU");
                if (expectActor >= 0)
                    check(cpuActor > 10 && gpuActor > 10, "Resumed enemy is visible on CPU and GPU");
            };
            if (props) {
                const std::vector<std::string> room{"#########", "#.......#", "#.......#",
                                                    "#.......#", "#.......#", "#.......#",
                                                    "#.......#", "#.......#", "#########"};
                for (auto kind : {DecorationKind::Chest, DecorationKind::Cage, DecorationKind::Rack}) {
                    if (rackOnly && kind != DecorationKind::Rack)
                        continue;
                    WorldDefinition world;
                    world.assetPack = "underkeep";
                    Decoration prop{{4, 4}, Heading::North, kind, 0, .5f, 0, .76f, .36f, .58f};
                    if (kind == DecorationKind::Chest)
                        prop.container = 0;
                    world.decorations = {prop};
                    const Map map(room, world);
                    const Player initial{{4, 5}, Heading::North};
                    NpcWorld empty(map, initial.cell, definitions.types, {});
                    GameSession game(map, catalog, empty, 5, initial, false, definitions.timings);
                    for (unsigned state = 0; state < (kind == DecorationKind::Chest ? 2u : 1u); ++state) {
                        if (state) {
                            game.restorePose(initial, false);
                            check(bool(game.openContainer({game.loot().snapshot().world, 0})),
                                  "Review opens chest");
                        }
                        const std::string prefix =
                            std::string(decorationName(kind)) + (state ? "-open" : "-closed");
                        for (unsigned h = 0; h < 4; ++h) {
                            const auto heading = Heading(h);
                            game.restorePose({prop.cell + forward(heading) * -2, heading}, false);
                            capture(game, prefix + "-side-" + std::to_string(h),
                                    kind == DecorationKind::Chest);
                        }
                        game.restorePose(initial, false);
                        capture(game, prefix + "-close", kind == DecorationKind::Chest);
                        game.setAnimated(true);
                        game.move(Action::TurnLeft);
                        for (unsigned frame = 0; frame < 5; ++frame) {
                            game.update(.025, false);
                            capture(game, prefix + "-motion-" + std::to_string(frame));
                        }
                        game.setAnimated(false);
                    }
                }
            } else if (corrections) {
                const std::vector<std::string> room{"#########", "#.......#", "#.......#",
                                                    "#.......#", "#.......#", "#.......#",
                                                    "#.......#", "#.......#", "#########"};
                WorldDefinition world;
                world.assetPack = "underkeep";
                Decoration chest{{4, 3}, Heading::North, DecorationKind::Chest, 0, .5f, 0, .76f, .36f, .58f};
                chest.container = 0;
                world.decorations = {
                    chest,
                    {{3, 1}, Heading::North, DecorationKind::DampA, 0, .5f, .25f, .75f, .45f, .01f},
                    {{5, 1}, Heading::North, DecorationKind::DampB, 0, .5f, .25f, .75f, .45f, .01f}};
                const Map map(room, world);
                const Player pose{{4, 4}, Heading::North};
                NpcWorld empty(map, pose.cell, definitions.types, {});
                GameSession inspect(map, catalog, empty, 5, pose, false, definitions.timings);
                capture(inspect, "chest-half-closed", true);
                check(bool(inspect.openContainer({inspect.loot().snapshot().world, 0})), "Open scaled chest");
                capture(inspect, "chest-half-open", true);
                inspect.restorePose({{4, 2}, Heading::North}, false);
                capture(inspect, "moss-at-floor");

                auto corridor = room;
                for (unsigned y = 1; y <= 3; ++y)
                    corridor[y][3] = corridor[y][5] = '#';
                WorldDefinition stone;
                stone.assetPack = "underkeep";
                const Map doorMap(corridor, stone);
                NpcWorld doorEmpty(doorMap, {4, 3}, definitions.types, {});
                FloorFixtures fixtures;
                fixtures.count = 1;
                fixtures.entries[0] = {{4, 2}, Heading::North, FixtureKind::Door};
                GameSession door(doorMap, catalog, doorEmpty, 5, {{4, 3}, Heading::North}, false,
                                 definitions.timings, {}, fixtures);
                capture(door, "door-smaller-closed");
                check(bool(door.interact({door.fixtures().id(0), door.fixtures().revision()})),
                      "Open review door");
                door.update(.15, false);
                capture(door, "door-smaller-mid-swing");
                door.update(.15, false);
                capture(door, "door-smaller-open");
                door.restorePose({{4, 1}, Heading::South}, false);
                door.setAnimated(false);
                check(bool(door.interact({door.fixtures().id(0), door.fixtures().revision()})),
                      "Close from back");
                capture(door, "door-smaller-reverse");

                const Map fightMap(room, stone);
                const Player fighter{{4, 5}, Heading::North};
                NpcWorld goblins(
                    fightMap, fighter.cell, definitions.types,
                    {{{0}, {4, 4}, {0, 1}, Heading::South}, {{0}, {4, 4}, {1, 1}, Heading::South}});
                GameSession fight(fightMap, catalog, goblins, 5, fighter, false, definitions.timings);
                for (unsigned i = 0; i < 100 && !(fight.combat().snapshot().awaitingAction &&
                                                  fight.combat().snapshot().activeCharacter->index == 0);
                     ++i) {
                    if (fight.combat().snapshot().awaitingAction)
                        fight.submit(turnCommand(fight));
                    else
                        fight.update(.5, false);
                }
                check(fight.combat().snapshot().awaitingAction &&
                          fight.combat().snapshot().activeCharacter->index == 0,
                      "Review reaches fighter decision");
                const auto survivor = fight.actors().id(1);
                check(bool(fight.actors().move(survivor, {5, 4}, {1, 1}, fighter.cell)),
                      "Goblin goes to outer diagonal");
                capture(fight, "combat-before-front-clears", false, 0);
                fight.actors().damage(fight.actors().id(0), 1000, fight.timeMs());
                fight.update(0, false);
                for (unsigned i = 0; i < 6 && fight.combat().snapshot().phase == CombatPhase::Facing; ++i)
                    fight.update(.16, false);
                check(fight.movement().player.heading == Heading::East,
                      "Cleared front faces the side goblin");
                auto attack = turnCommand(fight, PartyAction::Melee);
                attack.target = survivor;
                check(fight.combat().validate(attack, fight.movement().player) == CommandReason::Ready,
                      "Survivor is legally attackable at resumed decision");
                capture(fight, "combat-after-auto-facing", false, 1);
            } else
                for (unsigned depth : {1u, 3u}) {
                    const unsigned seed = 0;
                    const auto generated =
                        generation::generate({seed, depth}, blobber::test::campaignBlueprints());
                    check(bool(generated), "Underkeep review generation");
                    const auto& floor = *generated.floor;
                    const auto& f = floor.layout;
                    const auto population = generation::preparePopulation(floor, catalog, definitions.types);
                    GameSession game(floor.map, catalog, population.placements, floor.runtimeSeed, f.spawn,
                                     false, definitions.timings, population.recovery, f.fixtures,
                                     population.objective);
                    const std::string prefix =
                        std::string(generation::profileName(f.profile)) + "-depth-" + std::to_string(depth);
                    auto show = [&](Player p, const std::string& name) {
                        if (game.geometry().movementBlocked(p.cell) || game.actors().occupied(p.cell))
                            return false;
                        game.restorePose(p, false);
                        capture(game, prefix + "-" + name);
                        return true;
                    };
                    bool hallShown =
                        show({f.rooms[0].anchor,
                              f.rooms[0].size.x > f.rooms[0].size.y ? Heading::West : Heading::North},
                             "hall");
                    for (int y = f.rooms[0].origin.y;
                         !hallShown && y < f.rooms[0].origin.y + f.rooms[0].size.y; ++y)
                        for (int x = f.rooms[0].origin.x;
                             !hallShown && x < f.rooms[0].origin.x + f.rooms[0].size.x; ++x)
                            if (generation::regionAt(f, {x, y}) == 0)
                                hallShown = show(
                                    {{x, y},
                                     f.rooms[0].size.x > f.rooms[0].size.y ? Heading::East : Heading::South},
                                    "hall");
                    check(hallShown, "Review has a legal broad hall view");
                    std::array<bool, decorationKinds> done{};
                    for (const auto& d : floor.map.definition().decorations) {
                        if (done[unsigned(d.kind)])
                            continue;
                        const bool interesting =
                            d.kind == DecorationKind::DampA || d.kind == DecorationKind::BannerB ||
                            d.kind == DecorationKind::BannerA || d.kind == DecorationKind::WebA ||
                            d.kind == DecorationKind::BonesA || d.kind == DecorationKind::Cage ||
                            d.kind == DecorationKind::Rack;
                        if (!interesting)
                            continue;
                        bool captured = false;
                        if (d.cutout() && !d.floorCutout())
                            captured = show({d.cell + forward(d.face) * -1, d.face}, decorationName(d.kind));
                        else
                            for (unsigned h = 0; h < 4 && !captured; ++h) {
                                const auto direction = Heading(
                                    (h + unsigned(d.face) + (d.kind == DecorationKind::Rack ? 1 : 0)) % 4);
                                captured = show(
                                    {d.cell + forward(direction) * (d.floorCutout() ? -1 : -2), direction},
                                    decorationName(d.kind));
                            }
                        done[unsigned(d.kind)] = captured;
                    }
                    // Isolated interaction review: same generated geometry/supply contents,
                    // with NPCs and gameplay fixtures omitted so the checked command is safe.
                    NpcWorld empty(floor.map, f.spawn.cell, definitions.types, {});
                    GameSession inspect(floor.map, catalog, empty, floor.runtimeSeed, f.spawn, false,
                                        definitions.timings, population.recovery);
                    const auto& chest = f.caches[0];
                    inspect.restorePose({chest.cell + forward(chest.facing) * -1, chest.facing}, false);
                    capture(inspect, prefix + "-chest-closed", true);
                    check(bool(inspect.openContainer({inspect.loot().snapshot().world, 0})),
                          "Review opens chest through checked command");
                    capture(inspect, prefix + "-chest-open", true);
                    for (unsigned i = 0; i < lootCapacity; ++i) {
                        const auto& stack = inspect.loot().snapshot().stacks[i];
                        if (stack.active && stack.container == 0)
                            check(bool(inspect.pickup(
                                      {{inspect.loot().snapshot().world, std::uint16_t(i), stack.generation},
                                       {0},
                                       inspect.party().inventory().revision()})),
                                  "Review collects unchanged chest contents");
                    }
                    capture(inspect, prefix + "-chest-empty", true);
                    inspect.setAnimated(true);
                    inspect.move(Action::TurnLeft);
                    inspect.update(.10, false);
                    capture(inspect, prefix + "-mid-turn");
                    std::cout << prefix << " identity=" << floor.fingerprint
                              << " dressing=" << f.dressingFingerprint << '\n';
                }
            std::cout << "Underkeep review CPU bytes=" << renderer.cpuBytes()
                      << " GPU bytes=" << renderer.gpuBytes() << '\n';
        }
        const auto counts = resources.counts();
        check(counts.textures == baseline.textures && counts.targets == baseline.targets &&
                  counts.meshes == baseline.meshes && counts.shaders == baseline.shaders,
              "Underkeep world leases released");
    }
    std::cout << "Underkeep hidden rendering/IDs/resource review passed\n";
}
} // namespace blobber::test
