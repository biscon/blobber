#include "assets/blueprint_loader.hpp"
#pragma once
#include "assets/fingerprint.hpp"
#include "assets/rules_loader.hpp"
#include "persistence/campaign_store.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "ui/map_model.hpp"
#include <iostream>
namespace blobber::test {
// Opt-in production-art review. Routine map tests own all data and need no window.
inline void mapReview(const std::filesystem::path& assets, const std::filesystem::path& output,
                      const std::filesystem::path& copiedSave = {}) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Choose a new map-review output directory");
    const auto roomBlueprints = blueprints::load(assets / "blueprints/underkeep.json");
    const auto catalog = loadRules(assets / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Map review uses a hidden window");
    GpuResources resources;
    {
        Presenter presenter(resources, assets);
        const auto baseline = resources.counts();
        {
            GpuLease<TargetHandle> blank(resources, resources.createTarget(viewWidth, viewHeight));
            const Map map({"###########", "#....#....#", "#.#..#....#", "#.........#", "#....#....#",
                           "#....#....#", "###########"});
            generation::FloorLayout layout;
            layout.width = map.width();
            layout.height = map.height();
            layout.roomCount = 2;
            for (int y = 0; y < map.height(); ++y)
                for (int x = 0; x < map.width(); ++x)
                    if (!map.solid({x, y}))
                        layout.membership[unsigned(y * map.width() + x)] = x < 5 ? 0 : 1;
            FloorFixtures fixtures;
            fixtures.discovery = true;
            fixtures.depth = 2;
            fixtures.entries[fixtures.count++] = {{5, 3}, Heading::East, FixtureKind::Door, 0};
            fixtures.entries[fixtures.count++] = {{6, 1}, Heading::North, FixtureKind::Door, 2};
            fixtures.entries[fixtures.count++] = {{2, 4}, Heading::North, FixtureKind::Key, 2};
            fixtures.entries[fixtures.count++] = {{8, 4}, Heading::North, FixtureKind::GuardianGate, 0};
            fixtures.entries[fixtures.count++] = {{1, 1}, Heading::North, FixtureKind::Upstairs, 0};
            fixtures.entries[fixtures.count++] = {{9, 5}, Heading::South, FixtureKind::Downstairs, 0};
            fixtures.entries[fixtures.count++] = {{9, 1}, Heading::North, FixtureKind::Endpoint, 0};
            const NpcWorld guards(map, {4, 3}, definitions.types, {{{0}, {6, 5}, {0, 0}, Heading::South}});
            FloorObjectiveDefinition objective;
            objective.count = 1;
            objective.members[0] = 0;
            objective.approachDoor = 0;
            objective.gate = 3;
            GameSession game(map, catalog, guards, 5, {{4, 3}, Heading::East}, false, definitions.timings, {},
                             fixtures, objective);
            CommandController commands(game, definitions.timings);
            game.setAnimated(false);
            MapOverlayModel model;
            model.prepare(map, &layout);
            const auto show = [&](const char* name, bool full = false, bool keys = false) {
                commands.refresh({}, false, true, true);
                auto hud = commands.snapshot();
                hud.development = true;
                if (keys)
                    hud.questKeys = (1u << 2) | (1u << 3);
                model.refresh(&game.fixtures());
                presenter.presentWorld(blank.get(), 1, game.movement(), map, &hud, nullptr, true, nullptr,
                                       full, &model);
                presenter.capture(output / name);
            };
            show("partial-closed-door.png");
            check(bool(game.interact(game.nearbyFixture())), "Map review opens an observed door");
            show("partial-open-door.png");
            auto state = std::make_unique<FloorState>();
            game.exportFloor(*state);
            for (int n = 0; n < map.width() * map.height(); ++n)
                state->fixtures.discovered.set(unsigned(n));
            for (unsigned n = 0; n < fixtures.count; ++n)
                state->fixtures.known[n] = true;
            ++state->fixtures.discoveryRevision;
            game.importFloor(*state);
            show("feature-icons-and-keys.png", false, true);
            presenter.capture(output / "feature-icons-and-keys-1280.png", 1280, 720);
            commands.refresh({}, false, true, true);
            auto hud = commands.snapshot();
            hud.development = true;
            model.refresh(&game.fixtures());
            allocations = 0;
            watching = true;
            for (int frame = 0; frame < 3; ++frame)
                presenter.presentWorld(blank.get(), 1, game.movement(), map, &hud, nullptr, true, nullptr,
                                       false, &model);
            watching = false;
            check(allocations == 0, "Engine-owned map presentation makes no C++ heap allocations");

            std::vector<std::string> rows(55, std::string(85, '#'));
            for (int y = 1; y < 54; ++y)
                for (int x = 1; x < 84; ++x)
                    if (y % 8 != 0 || x % 10 == 4)
                        rows[y][x] = '.';
            const Map large(rows);
            const NpcWorld largeEmpty(large, {1, 1}, definitions.types, {});
            GameSession largeGame(large, catalog, largeEmpty, 5, {{1, 1}, Heading::South}, false,
                                  definitions.timings);
            CommandController largeCommands(largeGame, definitions.timings);
            largeCommands.refresh({}, false, true, true);
            MapOverlayModel largeModel;
            largeModel.prepare(large);
            largeModel.refresh(&largeGame.fixtures());
            const auto largePose = [&](Cell cell, const char* name) {
                auto movement = largeGame.movement();
                movement.player.cell = cell;
                presenter.presentWorld(blank.get(), 1, movement, large, &largeCommands.snapshot(), nullptr,
                                       true, nullptr, false, &largeModel);
                presenter.capture(output / name);
            };
            largePose({1, 1}, "oversized-near-edge.png");
            largePose({83, 1}, "oversized-top-right.png");
            largePose({1, 53}, "oversized-bottom-left.png");
            largePose({83, 53}, "oversized-bottom-right.png");
            largePose({42, 27}, "oversized-center.png");

            std::vector<std::string> paddedRows(36, std::string(36, '#'));
            for (int y = 8; y <= 17; ++y)
                for (int x = 11; x <= 22; ++x)
                    paddedRows[y][x] = '.';
            const Map padded(paddedRows);
            const NpcWorld paddedEmpty(padded, {11, 8}, definitions.types, {});
            GameSession paddedGame(padded, catalog, paddedEmpty, 5, {{11, 8}, Heading::North}, false,
                                   definitions.timings);
            CommandController paddedCommands(paddedGame, definitions.timings);
            paddedCommands.refresh({}, false, true, true);
            MapOverlayModel paddedModel;
            paddedModel.prepare(padded);
            paddedModel.refresh(&paddedGame.fixtures());
            auto movement = paddedGame.movement();
            presenter.presentWorld(blank.get(), 1, movement, padded, &paddedCommands.snapshot(), nullptr,
                                   true, nullptr, false, &paddedModel);
            presenter.capture(output / "small-padded-top-left.png");
            movement.player.cell = {22, 17};
            presenter.presentWorld(blank.get(), 1, movement, padded, &paddedCommands.snapshot(), nullptr,
                                   true, nullptr, false, &paddedModel);
            presenter.capture(output / "small-padded-bottom-right.png");
        }
        if (!copiedSave.empty()) {
            const auto before = fingerprintFile(copiedSave);
            CampaignDisk disk(copiedSave);
            CampaignStore store(catalog, definitions,
                                {fingerprintFile(assets / "rules/catalog.json"),
                                 definitions.definitionFingerprint, &roomBlueprints},
                                disk);
            auto campaign = store.load();
            auto state = std::make_unique<FloorState>();
            campaign->game().exportFloor(*state);
            const auto digest = state->digest();
            const auto& location = campaign->location(campaign->activeLocation());
            WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
            GameView view(renderer, location.generated ? &location.generated->layout : nullptr);
            NpcAnimator animator(campaign->game().actors(), definitions.timings);
            animator.setTime(campaign->game().timeMs(), &campaign->game().effects());
            view.update(campaign->game(), animator.snapshot(), {});
            CommandController commands(campaign->game(), definitions.timings);
            commands.refresh(view.picking()->targets(), false, true, true);
            view.present(presenter, campaign->game(), campaign->map(), true, true, &commands.snapshot());
            presenter.capture(output / "continued-floor-map.png");
            presenter.capture(output / "continued-floor-map-1280.png", 1280, 720);
            view.present(presenter, campaign->game(), campaign->map(), true, true, &commands.snapshot(),
                         nullptr, true);
            presenter.capture(output / "continued-floor-map-full.png");
            campaign->game().exportFloor(*state);
            check(state->digest() == digest && fingerprintFile(copiedSave) == before,
                  "Map review preserves continued simulation and copied save bytes");
            std::cout << "Continued map captured at floor " << campaign->activeLocation() << '\n';
        }
        const auto released = resources.counts();
        check(released.textures == baseline.textures && released.targets == baseline.targets &&
                  released.meshes == baseline.meshes && released.shaders == baseline.shaders,
              "Map review releases temporary and world resources");
        using Error = unsigned (*)();
        check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
              "Map presentation draws without OpenGL errors");
    }
    std::cout << "Hidden floor-map review passed\n";
}
} // namespace blobber::test
