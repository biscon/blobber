#include "app/guardian_captures.hpp"
#include <stdexcept>
namespace blobber {
void captureGuardians(const generation::GeneratedFloor& f, const generation::PreparedPopulation& p,
                      const NpcWorld& placements, const rules::Catalog& rules,
                      const rules::PresetParty& party, const std::vector<NpcTimings>& timings,
                      const SceneRenderer& renderer, Presenter& presenter,
                      const std::filesystem::path& directory) {
    // Diagnostic reconstructions, not an expedition or a combat balance workload.
    // Every window/target belongs to this application; no desktop input is injected.
    const auto& layout = f.layout;
    const auto capture = [&](const char* name, GameSession& g) {
        NpcAnimator animator(g.actors(), timings);
        animator.setTime(g.timeMs(), &g.effects());
        FramePipeline frames;
        CommandController ui(g, timings);
        frames.update(renderer, g.geometry(), g.movement(), &g.actors(), &animator.snapshot(), nullptr,
                      g.timeMs(), nullptr, &g.loot().snapshot());
        ui.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false, true);
        if (std::string_view(name) == "pc03-guardian-label")
            ui.hover(HoverKind::Enemy, 0, g.actors().id(p.objective.members[0]), 240, 150);
        presenter.present(frames.frame(), g.movement(), f.map, false, true, &ui.snapshot(), frames.picking());
        presenter.capture(directory / (std::string(name) + ".png"));
        presenter.capture(directory / (std::string(name) + "-1280.png"), 1280, 720);
    };
    const auto session = [&](Player pose) {
        return std::make_unique<GameSession>(f.map, rules, party, placements, f.runtimeSeed, pose, false,
                                             timings, p.recovery, layout.fixtures, p.objective);
    };
    const auto entrance = layout.fixtures.entries[layout.depth == 1 ? 3 : 4];
    auto stairs = session({entrance.cell + forward(entrance.facing) * -1, entrance.facing});
    capture("pc03-stairs-door-closed", *stairs);
    if (!stairs->interact({stairs->fixtures().id(layout.depth == 1 ? 3 : 4), stairs->fixtures().revision()})
             .changed)
        throw std::runtime_error("Capture could not open stairs door");
    capture("pc03-stairs-door-open", *stairs);
    const auto approach = layout.fixtures.entries[layout.approachDoor];
    auto warning = session({approach.cell + forward(approach.facing) * -1, approach.facing});
    capture("pc03-guardian-warning", *warning);
    auto encounter = session({layout.guardianApproach, approach.facing});
    capture("pc03-guardian-pair", *encounter);
    capture("pc03-guardian-label", *encounter);
    const auto gate = layout.fixtures.entries[layout.guardianGate];
    auto objective = session({gate.cell + forward(gate.facing) * -1, gate.facing});
    capture("pc03-gate-sealed", *objective);
    objective->actors().damage(objective->actors().id(p.objective.members[0]), 100000, 0);
    objective->update(0, false);
    capture("pc03-gate-partial", *objective);
    objective->actors().damage(objective->actors().id(p.objective.members[1]), 100000, 0);
    objective->update(0, false);
    capture("pc03-gate-open-reward", *objective);
    objective->setAnimated(false);
    objective->move(Action::Forward);
    objective->move(Action::Forward);
    capture("pc03-reward", *objective);
}
} // namespace blobber
