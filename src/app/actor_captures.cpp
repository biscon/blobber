#include "app/actor_captures.hpp"
#include <stdexcept>

namespace blobber {
void captureActors(const Map& map, const Party& party, const rules::Catalog& catalog,
                   const NpcWorld& placements, const std::vector<NpcTimings>& timings,
                   const SceneRenderer& renderer, Presenter& presenter,
                   const std::filesystem::path& directory) {
    // Like the existing captures, these poses describe the shipped demo level.
    if (placements.instances().empty() || !(placements.instances()[0].cell == Cell{18, 17}))
        return;
    // In the demo, slot two is the near-left goblin, clearly visible for hit feedback.
    const std::size_t subject = placements.instances().size() > 2 ? 2 : 0;
    const Player camera{{18, 18}, Heading::North};
    auto require = [](ActorResult result) {
        if (!result || !result.changed)
            throw std::runtime_error("Actor capture scenario command failed");
    };
    auto scenario = [&](const char* name, auto mutate, std::uint64_t time, int action = -1) {
        ActorWorld actors(map, catalog, placements);
        NpcAnimator animator(actors, timings);
        FramePipeline frames;
        MovementState state;
        state.player = camera;
        if (action >= 0) {
            const auto kind = static_cast<Action>(action);
            const Player from =
                action == static_cast<int>(Action::Right) ? Player{{17, 18}, Heading::North} : camera;
            Player to = from;
            if (!tryAction(map, to, kind))
                throw std::runtime_error("Actor capture transition is blocked by map");
            state.player = to;
            state.transition = {from, to, kind, Transition::duration * 0.5f};
        }
        animator.setTime(700);
        frames.update(renderer, map, state, &actors, &animator.snapshot());
        mutate(actors, state.player.cell);
        animator.setTime(time);
        frames.update(renderer, map, state, &actors, &animator.snapshot());
        // Each scenario owns a fresh pipeline; its final image must be uploaded.
        CommandController commands(party, actors);
        commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{},
                         state.transition.active(), false, true);
        presenter.present(frames.frame(), state, map, false, true, &commands.snapshot(), frames.picking());
        presenter.capture(directory / (std::string(name) + ".png"));
    };
    scenario("actor-baseline", [](ActorWorld&, Cell) {}, 700);
    scenario(
        "actor-moved",
        [&](ActorWorld& a, Cell party) { require(a.move(a.id(subject), {18, 16}, {0, 0}, party)); }, 700);
    scenario(
        "actor-turned", [&](ActorWorld& a, Cell) { require(a.face(a.id(subject), Heading::West)); }, 700);
    for (const auto time : {700u, 799u, 800u}) {
        const auto name = "actor-hit-" + std::to_string(time);
        scenario(name.c_str(), [&](ActorWorld& a, Cell) { require(a.damage(a.id(subject), 1, 700)); }, time);
        const auto deathName = "actor-death-" + std::to_string(time);
        scenario(
            deathName.c_str(),
            [&](ActorWorld& a, Cell) {
                require(a.damage(a.id(subject), a.actor(a.id(subject))->hitPoints, 700));
            },
            time);
    }
    auto remove = [&](ActorWorld& a, Cell) {
        for (std::size_t i = 0; i < a.snapshot().count; ++i)
            require(a.remove(a.id(i)));
    };
    scenario("actor-removed", remove, 700);
    for (int action = 0; action < 6; ++action) {
        const auto name = "actor-transition-removed-" + std::to_string(action);
        scenario(name.c_str(), remove, 700, action);
    }
}
} // namespace blobber
