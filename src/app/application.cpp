#include "app/application.hpp"
#include "app/actor_captures.hpp"
#include "app/captures.hpp"
#include "app/gameplay.hpp"
#include "app/hud_captures.hpp"
#include "app/options.hpp"
#include "app/ui_demo.hpp"
#include "assets/material_loader.hpp"
#include "assets/npc_loader.hpp"
#include "assets/rules_loader.hpp"
#include "game/party.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "render/frame_pipeline.hpp"
#include <iostream>
#include <stdexcept>
namespace blobber {
int run(int argc, char** argv) {
    const Options options = parseOptions(argc, argv);
    if (options.help) {
        std::cout << "blobber [--size WIDTH HEIGHT] [--assets DIRECTORY] [--capture DIRECTORY | "
                     "--capture-hud DIRECTORY] [--ui-demo | "
                     "--combat-demo] [--seed UINT32]\n";
        return 0;
    }
    // Declaration order keeps immutable definitions alive for the starting party.
    const auto rules = loadRules(options.assets / "rules" / "catalog.json");
    const Party party(rules);
    const Map map = Map::load((options.assets / "maze.txt").string());
    Controller controller;
    if (map.solid(controller.state().player.cell) || !map.connected(controller.state().player.cell))
        throw std::runtime_error("Spawn is blocked or map contains unreachable floors");
    auto loadedNpcs = loadNpcs(options.assets / "npcs" / "level.json", map, controller.state().player.cell);
    ActorWorld npcs(map, rules, loadedNpcs.world);
    std::vector<NpcTimings> timings;
    for (const auto& appearance : loadedNpcs.appearances)
        timings.push_back(npcTimings(appearance));
    const SceneRenderer renderer(loadMaterials(options.assets / "textures"), loadedNpcs.appearances);
    // Source images are needed only during preparation.
    std::vector<NpcAppearance>().swap(loadedNpcs.appearances);
    // Declaration order guarantees all GPU resources die before the window/context.
    const Window window(options.width, options.height, !options.captureDirectory.empty());
    Presenter presenter(options.assets);
    if (!options.captureDirectory.empty()) {
        NpcAnimator animator(npcs, timings);
        FramePipeline frames;
        CommandController commands(party, npcs);
        std::filesystem::create_directories(options.captureDirectory);
        if (options.captureHudOnly) {
            captureHud(map, rules, loadedNpcs.world, timings, renderer, presenter, options.captureDirectory);
            return 0;
        }
        const auto shots = makeCaptures();
        for (std::size_t i = 0; i < shots.size() && !WindowShouldClose(); ++i) {
            const auto& shot = shots[i];
            MovementState state;
            state.player = shot.to;
            state.transition = {shot.from, shot.to, shot.action, shot.t * Transition::duration};
            state.revision = i + 1;
            animator.setTime(shot.npcTimeMs);
            const bool changed = frames.update(renderer, map, state, &npcs, &animator.snapshot());
            commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{},
                             state.transition.active(), false, true);
            presenter.present(frames.frame(), state, map, false, changed, &commands.snapshot(),
                              frames.picking());
            presenter.capture(options.captureDirectory / (shot.name + ".png"));
        }
        if (!WindowShouldClose())
            captureActors(map, party, rules, loadedNpcs.world, timings, renderer, presenter,
                          options.captureDirectory);
        if (!WindowShouldClose())
            captureUi(map, party, rules, loadedNpcs.world, timings, renderer, presenter,
                      options.captureDirectory);
        if (!WindowShouldClose())
            captureCombat(map, rules, loadedNpcs.world, timings, renderer, presenter,
                          options.captureDirectory);
        if (!WindowShouldClose())
            captureHud(map, rules, loadedNpcs.world, timings, renderer, presenter, options.captureDirectory);
        return 0;
    }
    if (options.uiDemo) {
        runUiDemo(map, party, rules, loadedNpcs.world, timings, renderer, presenter);
        return 0;
    }
    runGameplay(map, rules, loadedNpcs.world, timings, renderer, presenter, options);
    return 0;
}
} // namespace blobber
