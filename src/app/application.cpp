#include "app/application.hpp"
#include "app/actor_captures.hpp"
#include "app/captures.hpp"
#include "app/options.hpp"
#include "app/ui_demo.hpp"
#include "assets/material_loader.hpp"
#include "assets/npc_loader.hpp"
#include "assets/rules_loader.hpp"
#include "game/party.hpp"
#include "platform/input.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "render/frame_pipeline.hpp"
#include "ui/interaction.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace blobber {
int run(int argc, char** argv) {
    const Options options = parseOptions(argc, argv);
    if (options.help) {
        std::cout << "blobber [--size WIDTH HEIGHT] [--assets DIRECTORY] [--capture DIRECTORY] [--ui-demo]\n";
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
    NpcAnimator animator(npcs, timings);
    const SceneRenderer renderer(loadMaterials(options.assets / "textures"), loadedNpcs.appearances);
    // Source images are needed only during preparation.
    std::vector<NpcAppearance>().swap(loadedNpcs.appearances);
    FramePipeline frames;
    CommandController commands(party, npcs);
    // Declaration order guarantees all GPU resources die before the window/context.
    const Window window(options.width, options.height);
    Presenter presenter;
    if (!options.captureDirectory.empty()) {
        std::filesystem::create_directories(options.captureDirectory);
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
            if (i == 0)
                TakeScreenshot((options.captureDirectory / "window.png").string().c_str());
        }
        if (!WindowShouldClose())
            captureActors(map, party, rules, loadedNpcs.world, timings, renderer, presenter,
                          options.captureDirectory);
        if (!WindowShouldClose())
            captureUi(map, party, rules, loadedNpcs.world, timings, renderer, presenter,
                      options.captureDirectory);
        return 0;
    }
    if (options.uiDemo) {
        runUiDemo(map, party, rules, loadedNpcs.world, timings, renderer, presenter);
        return 0;
    }
    Input input;
    bool debug = false;
    const double animationStart = GetTime();
    while (!WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), 0.1f);
        const auto sample = input.poll(dt);
        if (sample.clearPending)
            controller.clearPending();
        const auto previousPose = controller.state().player;
        controller.update(map, dt, &npcs);
        if (sample.toggleAnimation)
            controller.setAnimated(!controller.state().animated);
        if (sample.toggleMap)
            debug = !debug;
        if (sample.action)
            controller.submit(map, *sample.action, sample.fresh, &npcs);
        if (!(previousPose == controller.state().player))
            commands.movementStarted();
        animator.setTime(static_cast<std::uint64_t>((GetTime() - animationStart) * 1000.0));
        const bool changed = frames.update(renderer, map, controller.state(), &npcs, &animator.snapshot());
        const auto* picking = frames.picking();
        commands.refresh(picking ? picking->targets() : TargetSnapshot{},
                         controller.state().transition.active(), debug, sample.ui.focused);
        const auto hit = picking && sample.ui.pointer ? picking->pick(int(sample.ui.pointer->x) - viewX,
                                                                      int(sample.ui.pointer->y) - viewY)
                                                      : std::nullopt;
        routeHudInput(commands, sample.ui, hit);
        presenter.present(frames.frame(), controller.state(), map, debug, changed, &commands.snapshot(),
                          picking);
    }
    return 0;
}
} // namespace blobber
