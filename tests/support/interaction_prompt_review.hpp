#pragma once
#include "assets/image.hpp"
#include "assets/rules_loader.hpp"
#include "game/campaign.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "world/sanctuary_city.hpp"
#include <iostream>
#include <string_view>
namespace blobber::test {
// Opt-in production-art verification. Ordinary prompt regressions own small CPU fixtures.
inline void interactionPromptReview(const std::filesystem::path& assets,
                                    const std::filesystem::path& output) {
    check(std::filesystem::create_directory(output), "Prompt review requires a new output directory");
    const auto catalog = loadRules(assets / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Review window remains hidden");
    GpuResources resources;
    Presenter presenter(resources, assets);
    const auto baseline = resources.counts();
    {
        WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
        auto capture = [&](GameSession& game, const char* name, const char* expected,
                           std::optional<LootId> hover = {}) {
            NpcAnimator animation(game.actors(), definitions.timings);
            animation.setTime(game.timeMs(), &game.effects());
            GameView view(renderer);
            view.update(game, animation.snapshot(), {});
            CommandController commands(game, definitions.timings);
            commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, false, false,
                             true);
            commands.floorHover(hover);
            const auto* prompt = commands.snapshot().interactionPrompt();
            check(expected ? prompt && std::string_view(prompt) == expected : !prompt,
                  "Production scene has expected prompt availability");
            view.present(presenter, game, game.geometry().base(), false, true, &commands.snapshot());
            const auto path = output / (std::string(name) + ".png");
            presenter.capture(path);
            using Error = unsigned (*)();
            check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                  "Prompt presentation has no GL errors");
            if (!expected)
                return;
            auto quiet = std::make_unique<HudSnapshot>(commands.snapshot());
            quiet->fixturePrompt[0] = quiet->lootHover[0] = 0;
            view.present(presenter, game, game.geometry().base(), false, true, quiet.get());
            const auto quietPath = output / (std::string(name) + "-baseline.png");
            presenter.capture(quietPath);
            OwnedImage shown(LoadImage(path.c_str())), hidden(LoadImage(quietPath.c_str()));
            check(shown.value.data && hidden.value.data, "Prompt capture readback succeeds");
            ImageFormat(&shown.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            ImageFormat(&hidden.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            const auto* a = static_cast<const unsigned char*>(shown.value.data);
            const auto* b = static_cast<const unsigned char*>(hidden.value.data);
            int left = gameWidth, right = 0, top = gameHeight, bottom = 0;
            unsigned changed = 0, white = 0;
            for (int y = viewY * 3; y < (viewY + viewHeight) * 3; ++y)
                for (int x = viewX * 3; x < (viewX + viewWidth) * 3; ++x) {
                    const auto p = (y * gameWidth + x) * 4;
                    if (a[p] == b[p] && a[p + 1] == b[p + 1] && a[p + 2] == b[p + 2])
                        continue;
                    ++changed;
                    white += a[p] == 255 && a[p + 1] == 255 && a[p + 2] == 255;
                    left = std::min(left, x);
                    right = std::max(right, x);
                    top = std::min(top, y);
                    bottom = std::max(bottom, y);
                }
            check(changed > 50 && white > 20, "Prompt contains readable white glyphs");
            check(std::abs((left + right) / 2.f - (viewX + viewWidth / 2.f) * 3) < 4,
                  "Prompt is horizontally centered in the dungeon viewport");
            check(top >= (viewY + viewHeight) * 3 - 55 && bottom <= (viewY + viewHeight) * 3 - 22,
                  "Prompt and two-pixel shadow stay above the 24-pixel bottom margin");
            check(changed * 4 < unsigned((right - left + 1) * (bottom - top + 1)) * 3,
                  "Prompt has sparse glyphs rather than a filled banner");
            std::cout << name << ": prompt bounds " << left << ',' << top << ".." << right << ',' << bottom
                      << ", changed pixels=" << changed << '\n';
        };
        {
            CampaignRun campaign(catalog, *catalog.preset(), definitions.types, definitions.timings, 5);
            campaign.game().setAnimated(false);
            campaign.game().restorePose(sanctuaryCity().doorway("inn").approach, false);
            capture(campaign.game(), "city-enter-inn", "[F] Enter inn");
        }
        WorldDefinition world;
        world.assetPack = "underkeep";
        const Map map(
            {"#########", "#...#...#", "#...#...#", "#.......#", "#...#...#", "#...#...#", "#########"},
            world);
        const Player pose{{3, 3}, Heading::East};
        FloorFixtures fixtures;
        fixtures.count = 1;
        fixtures.entries[0] = {{4, 3}, Heading::East, FixtureKind::Door};
        const NpcWorld empty(map, pose.cell, definitions.types, {});
        RecoveryLevel recovery;
        recovery.groundCount = 1;
        recovery.ground[0] = {pose.cell, catalog.preset()->characters[0].items[0].item, 1};
        {
            GameSession game(map, catalog, empty, 5, pose, false, definitions.timings, recovery, fixtures);
            game.setAnimated(false);
            capture(game, "dungeon-open", "[F] Open");
            const LootId loot{game.loot().snapshot().world, NpcWorld::maxInstances * lootPerActor};
            capture(game, "dungeon-loot", "[Click] Loot", loot);
        }
        {
            const auto goblin = std::find(definitions.types.begin(), definitions.types.end(), "goblin");
            check(goblin != definitions.types.end(), "Review goblin appearance exists");
            const NpcTypeId type{std::uint16_t(goblin - definitions.types.begin())};
            const NpcWorld enemy(
                map, pose.cell, definitions.types,
                {{type, {5, 3}, {0, 0}, Heading::West}, {type, {5, 3}, {1, 0}, Heading::West}});
            GameSession game(map, catalog, enemy, 5, pose, true, definitions.timings, recovery, fixtures);
            game.setAnimated(false);
            check(bool(game.interact(game.nearbyFixture())), "Review opens door before enemy contact");
            for (unsigned step = 0; step < 80 && game.combat().snapshot().phase != CombatPhase::Turns; ++step)
                game.update(.1, false);
            check(game.combat().snapshot().phase == CombatPhase::Turns,
                  "Production combat reaches turns beside an open door");
            capture(game, "combat-no-prompt", nullptr);
        }
    }
    const auto final = resources.counts();
    check(final.textures == baseline.textures && final.targets == baseline.targets &&
              final.meshes == baseline.meshes && final.shaders == baseline.shaders,
          "Prompt review releases world resources");
}
} // namespace blobber::test
