#pragma once
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include <algorithm>
#include <iostream>

namespace blobber::test {
// Opt-in production-art review. Routine correctness fixtures own their generated artwork.
inline void creatureOcclusionReview(const std::filesystem::path& assets,
                                    const std::filesystem::path& output) {
    if (std::filesystem::exists(output) && !std::filesystem::is_empty(output))
        throw std::runtime_error("Choose a new or empty creature occlusion review directory");
    std::filesystem::create_directories(output);
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    const auto catalog = loadRules(assets / "rules/catalog.json");
    auto artwork =
        std::make_shared<const world_assets::CpuAssets>(world_assets::load(assets / "prepared/world"));
    WorldDefinition world;
    world.assetPack = "underkeep";
    const Map map({"#########", "#.......#", "#..#.#..#", "#..#.#..#", "#..#.#..#", "#.......#", "#.......#",
                   "#.......#", "#########"},
                  world);
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Creature review uses only a hidden window");
    GpuResources resources;
    {
        Presenter presenter(resources, assets);
        const auto baseline = resources.counts();
        for (const char* name : {"goblin", "bugbear"})
            for (bool reverse : {false, true}) {
                const auto found = std::find(definitions.types.begin(), definitions.types.end(), name);
                check(found != definitions.types.end(), "Production review creature exists");
                const NpcTypeId type{std::uint16_t(found - definitions.types.begin())};
                const Heading heading = reverse ? Heading::South : Heading::North;
                const auto f = forward(heading);
                const Cell doorCell{4, 4};
                const Player pose{doorCell + f * -2, heading};
                NpcWorld places(map, pose.cell, definitions.types,
                                {{type, doorCell + f, {0, 1}, Heading((unsigned(heading) + 2) % 4)},
                                 {type, doorCell + f, {1, 1}, Heading((unsigned(heading) + 2) % 4)},
                                 {type, doorCell + f * 2, {1, 1}, Heading((unsigned(heading) + 2) % 4)}});
                FloorFixtures fixtures;
                fixtures.count = 1;
                fixtures.entries[0] = {doorCell, Heading::North, FixtureKind::Door};
                GameSession game(map, catalog, places, 5, pose, false, definitions.timings, {}, fixtures);
                game.setAnimated(false);
                const Player adjacent{doorCell + f * -1, heading};
                game.restorePose(adjacent, false);
                check(bool(game.interact(game.nearbyFixture())), "Review opens doorway through game command");
                game.restorePose(pose, false);
                NpcAnimator animator(game.actors(), definitions.timings);
                WorldRenderer renderer(resources, artwork);
                GameView view(renderer);
                bool sawMasks = false;
                auto capture = [&](unsigned time) {
                    animator.setTime(time);
                    view.update(game, animator.snapshot(), {});
                    view.activate();
                    const auto& frame = renderer.visibility()->frame();
                    world_render::BillboardClips clips;
                    for (unsigned i = 0; i < frame.count; ++i) {
                        const auto& b = frame.billboards[i];
                        if (b.id >= NpcWorld::maxInstances)
                            continue;
                        const auto& appearance = artwork->publication.appearances[type.index];
                        const auto& clip = appearance.clips[unsigned(animator.snapshot().frames[b.id].clip)];
                        check(b.width == (clip.width ? clip.width : appearance.width) &&
                                  b.height == appearance.height,
                              "Production doorway traversal never shrinks the creature");
                        world_render::billboardClips(frame, b, clips);
                        sawMasks |= clips.count != 0;
                    }
                    CommandController commands(game, definitions.timings);
                    commands.refresh(view.picking()->targets(), false, false, true);
                    view.present(presenter, game, map, false, true, &commands.snapshot());
                    presenter.capture(output / (std::string(name) + (reverse ? "-back-" : "-front-") +
                                                std::to_string(time) + ".png"));
                    using Error = unsigned (*)();
                    check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                          "Production creature occlusion has no GL errors");
                };
                for (unsigned i = 0; i < 2; ++i)
                    check(bool(game.actors().walk(game.actors().id(i), doorCell, {int(i), 1}, pose.cell, 0,
                                                  600)),
                          "Review creatures approach doorway");
                for (unsigned time : {0u, 250u, 300u, 350u, 375u, 400u, 425u, 450u, 600u})
                    capture(time);
                game.restorePose(adjacent, false);
                capture(700);
                game.restorePose(pose, false);
                for (unsigned i = 0; i < 2; ++i)
                    check(bool(game.actors().walk(game.actors().id(i), doorCell + f * -1, {int(i), 1},
                                                  pose.cell, 1000, 600)),
                          "Review creatures pass through doorway");
                for (unsigned time : {1000u, 1050u, 1075u, 1100u, 1125u, 1150u, 1200u, 1300u, 1600u})
                    capture(time);
                check(sawMasks, "Production motion sample exercises scenery intersection clipping");
            }
        const auto released = resources.counts();
        check(released.textures == baseline.textures && released.targets == baseline.targets &&
                  released.meshes == baseline.meshes && released.shaders == baseline.shaders,
              "Production creature review releases all world resources");
    }
    std::cout << "Production goblin/bugbear doorway captures passed from both sides\n";
}
} // namespace blobber::test
