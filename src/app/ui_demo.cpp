#include "app/ui_demo.hpp"
#include "platform/input.hpp"
#include "ui/interaction.hpp"
#include <algorithm>
#include <stdexcept>
namespace blobber {
namespace {
constexpr const char* names[]{"ui-default",     "ui-melee-preview", "ui-back-row",       "ui-missing-target",
                              "ui-unavailable", "ui-sample-status", "ui-target-removed", "ui-log-overflow"};
constexpr int scenarioCount = sizeof(names) / sizeof(names[0]);
Player camera(const Map& map, const ActorWorld& actors) {
    for (std::size_t i = 0; i < actors.snapshot().count; ++i) {
        for (const auto h : {Heading::North, Heading::East, Heading::South, Heading::West}) {
            const auto cell = actors.snapshot().actors[i].npc.cell + forward(h) * -1;
            if (!map.solid(cell) && !actors.occupied(cell))
                return {cell, h};
        }
    }
    return {};
}
struct Scenario {
    ActorWorld actors;
    Controller movement;
    NpcAnimator animator;
    FramePipeline frames;
    CommandController commands;
    HudDemo demo;
    const SceneRenderer& renderer;
    const Map& map;
    Scenario(int index, const Map& m, const Party& party, const rules::Catalog& catalog,
             const NpcWorld& placements, const std::vector<NpcTimings>& timings, const SceneRenderer& r)
        : actors(m, catalog, placements), movement(camera(m, actors)), animator(actors, timings),
          commands(party, actors), renderer(r), map(m) {
        demo.enabled = true;
        demo.scenario = index;
        for (std::size_t i = 0; i < rules::partySize; ++i)
            demo.resources[i] = {party.snapshot().characters[i].hitPoints, 0, 0, DisplayStatus::Ready};
        animator.setTime(700);
        refresh();
        const auto targets = commands.snapshot().targets;
        switch (index) {
        case 0:
            break;
        case 1:
            if (targets.count)
                commands.selectTarget(targets.ids[targets.count - 1]);
            commands.confirm();
            break;
        case 2:
            commands.selectCharacter(2);
            commands.selectAction(PartyAction::BackMelee);
            if (targets.count)
                commands.selectTarget(targets.ids[targets.count - 1]);
            commands.confirm();
            break;
        case 3:
            commands.confirm();
            break;
        case 4:
            commands.selectAction(PartyAction::Spell);
            commands.confirm();
            break;
        case 5:
            demo.resources[0].hp = 2;
            demo.resources[1].mana = 2;
            demo.resources[1].maxMana = 6;
            demo.resources[2] = {0, 0, 0, DisplayStatus::Unconscious};
            demo.resources[3] = {0, 0, 8, DisplayStatus::Dead};
            refresh();
            commands.selectCharacter(2);
            commands.confirm();
            break;
        case 6:
            if (targets.count) {
                commands.selectTarget(targets.ids[targets.count - 1]);
                const auto stale = commands.snapshot().selection;
                if (!actors.remove(targets.ids[targets.count - 1]))
                    throw std::runtime_error("UI demo removal failed");
                animator.setTime(700);
                refresh();
                commands.submit(stale);
            }
            break;
        case 7:
            commands.selectAction(PartyAction::Defend);
            for (int i = 0; i < 40; ++i) {
                commands.selectCharacter(i % 4);
                commands.confirm();
            }
            commands.toggleLog();
            break;
        }
    }
    bool refresh(bool debug = false, bool focused = true) {
        const bool changed = frames.update(renderer, map, movement.state(), &actors, &animator.snapshot());
        const auto* picking = frames.picking();
        commands.refresh(picking ? picking->targets() : TargetSnapshot{},
                         movement.state().transition.active(), debug, focused, &demo);
        return changed;
    }
};
} // namespace
void captureUi(const Map& map, const Party& party, const rules::Catalog& catalog, const NpcWorld& placements,
               const std::vector<NpcTimings>& timings, const SceneRenderer& renderer, Presenter& presenter,
               const std::filesystem::path& directory) {
    for (int i = 0; i < scenarioCount && !WindowShouldClose(); ++i) {
        Scenario scenario(i, map, party, catalog, placements, timings, renderer);
        presenter.present(scenario.frames.frame(), scenario.movement.state(), map, false, true,
                          &scenario.commands.snapshot(), scenario.frames.picking());
        presenter.capture(directory / (std::string(names[i]) + ".png"));
    }
}
void runUiDemo(const Map& map, const Party& party, const rules::Catalog& catalog, const NpcWorld& placements,
               const std::vector<NpcTimings>& timings, const SceneRenderer& renderer, Presenter& presenter) {
    int index = 0;
    Input input; // Keep edge latches across scenario preparation, including held next/reset.
    while (!WindowShouldClose()) {
        Scenario scenario(index, map, party, catalog, placements, timings, renderer);
        bool debug = false, first = true;
        while (!WindowShouldClose()) {
            const float dt = std::min(GetFrameTime(), 0.1f);
            const auto sample = input.poll(dt);
            const auto& ui = sample.ui;
            const bool click = ui.has(UiKey::Click) && ui.pointer;
            if (ui.has(UiKey::DemoNext) || (click && hudLayout::demoNext.contains(*ui.pointer))) {
                index = (index + 1) % scenarioCount;
                break;
            }
            if (ui.has(UiKey::DemoReset) || (click && hudLayout::demoReset.contains(*ui.pointer)))
                break;
            if (sample.clearPending)
                scenario.movement.clearPending();
            const auto previousPose = scenario.movement.state().player;
            scenario.movement.update(map, dt, &scenario.actors);
            if (sample.toggleMap)
                debug = !debug;
            if (sample.toggleAnimation)
                scenario.movement.setAnimated(!scenario.movement.state().animated);
            if (sample.action)
                scenario.movement.submit(map, *sample.action, sample.fresh, &scenario.actors);
            if (!(previousPose == scenario.movement.state().player))
                scenario.commands.movementStarted();
            // Fixed animation time makes the explicit development environment reproducible.
            const bool changed = scenario.refresh(debug, ui.focused);
            const auto* picking = scenario.frames.picking();
            const auto hit = picking && ui.pointer
                                 ? picking->pick(int(ui.pointer->x) - viewX, int(ui.pointer->y) - viewY)
                                 : std::nullopt;
            routeHudInput(scenario.commands, ui, hit);
            presenter.present(scenario.frames.frame(), scenario.movement.state(), map, debug,
                              changed || first, &scenario.commands.snapshot(), picking);
            first = false;
        }
    }
}
} // namespace blobber
