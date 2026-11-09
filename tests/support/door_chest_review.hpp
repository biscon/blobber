#include "assets/blueprint_loader.hpp"
#pragma once
#include "app/gameplay.hpp"
#include "assets/fingerprint.hpp"
#include "assets/image.hpp"
#include "assets/rules_loader.hpp"
#include "persistence/campaign_store.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include <algorithm>
#include <iostream>
namespace blobber::test {
inline void doorChestReview(const std::filesystem::path& assets, const std::filesystem::path& output,
                            const std::filesystem::path& copiedSave) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Choose a new door/chest review directory");
    const auto before = fingerprintFile(copiedSave);
    const auto roomBlueprints = blueprints::load(assets / "blueprints/underkeep.json");
    const auto catalog = loadRules(assets / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    CampaignDisk disk(copiedSave);
    CampaignStore store(
        catalog, definitions,
        {fingerprintFile(assets / "rules/catalog.json"), definitions.definitionFingerprint, &roomBlueprints},
        disk);
    auto campaign = store.load();
    Window window(1920, 1080, true);
    GpuResources resources;
    {
        Presenter presenter(resources, assets);
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
            GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
            auto capture = [&](GameSession& game, const std::string& name, std::uint64_t sampleTime = 0) {
                NpcAnimator animator(game.actors(), definitions.timings);
                animator.setTime(sampleTime ? sampleTime : game.timeMs(), &game.effects());
                GameView view(renderer);
                view.update(game, animator.snapshot(), {});
                CommandController commands(game, definitions.timings);
                commands.refresh(view.picking()->targets(), false, false, true);
                view.present(presenter, game, game.geometry().base(), false, true, &commands.snapshot());
                presenter.capture(output / (name + ".png"));
                renderer.diagnosticIds(ids.get());
                OwnedImage raster(LoadImageFromTexture(resources.target(ids.get()).texture));
                check(raster.value.data, "Door/chest GPU ID readback");
                ImageFlipVertical(&raster.value);
                ImageFormat(&raster.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                const auto* pixels = static_cast<const unsigned char*>(raster.value.data);
                const auto& picking = renderer.visibility()->frame().picking;
                unsigned interior = 0;
                for (int y = 1; y < viewHeight - 1; ++y)
                    for (int x = 1; x < viewWidth - 1; ++x) {
                        const auto at = (y * viewWidth + x) * 4;
                        const auto actual = std::uint16_t(pixels[at] + 256 * pixels[at + 1]),
                                   expected = picking.slot(x, y);
                        if (actual == expected)
                            continue;
                        bool edge = false;
                        for (int dy = -1; dy <= 1; ++dy)
                            for (int dx = -1; dx <= 1; ++dx)
                                edge |= picking.slot(x + dx, y + dy) != expected;
                        interior += !edge;
                    }
                check(interior < 100, "Door/chest CPU/GPU interior picking agreement");
                using Error = unsigned (*)();
                check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0, "Door/chest GL draw");
            };
            auto& game = campaign->game();
            game.setAnimated(false);
            capture(game, "saved-door");
            // Isolate the copy's visual review, without progressing fights/rewards.
            for (unsigned i = 0; i < game.actors().snapshot().count; ++i)
                game.actors().remove(game.actors().id(i));
            const auto original = game.movement().player;
            const auto request = game.nearbyFixture();
            const auto* fixture = game.fixtures().find(request.id);
            check(fixture && fixture->kind == FixtureKind::Door, "Reported save faces a normal door");
            const auto door = *fixture;
            if (game.fixtures().open(request.id.slot))
                check(bool(game.interact(game.nearbyFixture())), "Close saved door for review");
            capture(game, "door-front-closed");
            game.restorePose(
                {door.cell * 2 + original.cell * -1, Heading((unsigned(original.heading) + 2) % 4)}, false);
            capture(game, "door-back-closed");
            check(bool(game.interact(game.nearbyFixture())), "Open door from reverse side");
            capture(game, "door-back-open");
            game.restorePose(original, false);
            capture(game, "door-front-open");
            bool chestShown = false;
            for (const auto& d : campaign->map().definition().decorations) {
                if (d.kind != DecorationKind::Chest || d.sealed)
                    continue;
                game.restorePose({d.cell + forward(d.face) * -1, d.face}, false);
                const ContainerId id{game.loot().snapshot().world, d.container};
                if (!game.containerAccess(id))
                    continue;
                capture(game, "chest-at-wall");
                game.move(Action::Forward);
                check(game.movement().player.cell == d.cell && game.nearbyContainer().slot == d.container,
                      "Player steps onto chest tile with F interaction available");
                capture(game, "standing-on-chest-cell");
                check(bool(game.openContainer(id)), "Open chest from its own tile");
                capture(game, "chest-open-from-own-cell");
                chestShown = true;
                break;
            }
            check(chestShown, "Saved floor has a supply chest review");

            const auto rat = std::find(definitions.types.begin(), definitions.types.end(), "giant.rat");
            check(rat != definitions.types.end(), "Production rat appearance exists");
            const NpcTypeId type{std::uint16_t(rat - definitions.types.begin())};
            WorldDefinition world;
            world.assetPack = "underkeep";
            const Map map({"#########", "#.......#", "#..#.#..#", "#..#.#..#", "#..#.#..#", "#.......#",
                           "#.......#", "#.......#", "#########"},
                          world);
            NpcWorld rats(map, {4, 4}, definitions.types,
                          {{type, {4, 2}, {0, 1}, Heading::South}, {type, {4, 2}, {1, 1}, Heading::South}});
            FloorFixtures fixtures;
            fixtures.count = 1;
            fixtures.entries[0] = {{4, 3}, Heading::North, FixtureKind::Door};
            GameSession encounter(map, catalog, rats, 5, {{4, 4}, Heading::North}, false, definitions.timings,
                                  {}, fixtures);
            encounter.setAnimated(false);
            check(bool(encounter.interact(encounter.nearbyFixture())), "Rat fixture opens door");
            check(bool(encounter.actors().walk(encounter.actors().id(0), {4, 3}, {0, 1}, {4, 4}, 0, 600)) &&
                      bool(encounter.actors().move(encounter.actors().id(1), {4, 3}, {1, 1}, {4, 4})),
                  "Rats traverse both subslots beside open leaf");
            for (unsigned time : {0u, 150u, 300u, 450u, 600u})
                capture(encounter, "rats-at-open-door-" + std::to_string(time), time);
        }
        const auto released = resources.counts();
        check(released.textures == baseline.textures && released.targets == baseline.targets &&
                  released.meshes == baseline.meshes && released.shaders == baseline.shaders,
              "Door/chest review releases world resources");
    }
    check(fingerprintFile(copiedSave) == before, "Door/chest diagnostic never rewrites copied save");
    std::cout << "Door/chest saved-position and rat review passed; copied save unchanged\n";
}
} // namespace blobber::test
