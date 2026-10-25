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
void captureReactions(const Map& map, const Party& party, const rules::Catalog& catalog,
                      const NpcWorld& placements, const std::vector<NpcTimings>& timings,
                      const SceneRenderer& renderer, Presenter& presenter,
                      const std::filesystem::path& directory) {
    if (placements.instances().empty() || !(placements.instances()[0].cell == Cell{18, 17}))
        return;
    std::vector<std::string> types;
    for (std::size_t i = 0; i < placements.typeCount(); ++i)
        types.push_back(placements.typeId({static_cast<std::uint16_t>(i)}));
    const std::array<Heading, 4> headings{Heading::South, Heading::North, Heading::West, Heading::East};
    const std::array<const char*, 4> sides{"front", "back", "left", "right"};
    for (std::size_t side = 0; side < 4; ++side) {
        auto npc = placements.instances()[0];
        npc.subcell = {0, 1};
        npc.facing = headings[side];
        npc.animation = NpcAnimation::Idle;
        npc.phaseMs = 0;
        const NpcWorld single(map, Player{}.cell, types, {npc});
        for (bool lethal : {false, true}) {
            ActorWorld actors(map, catalog, single);
            NpcAnimator animator(actors, timings);
            FramePipeline frames;
            CommandController commands(party, actors);
            MovementState state;
            state.player = {{18, 18}, Heading::North};
            auto capture = [&](const std::string& name, std::uint64_t time) {
                animator.setTime(time);
                frames.update(renderer, map, state, &actors, &animator.snapshot());
                commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{},
                                 state.transition.active(), false, true);
                presenter.present(frames.frame(), state, map, false, true, &commands.snapshot(),
                                  frames.picking());
                presenter.capture(directory / (name + ".png"));
                if (name.find("-100") != std::string::npos || name.find("-600") != std::string::npos)
                    presenter.capture(directory / (name + "-720.png"), 1280, 720);
            };
            const std::string prefix = std::string("reaction-") + (lethal ? "death-" : "hurt-") + sides[side];
            capture(prefix + "-baseline", 700);
            actors.attack(actors.id(0), 650); // Reactions must interrupt a real cue.
            const auto damage = lethal ? actors.actor(actors.id(0))->hitPoints : 1;
            if (!actors.damage(actors.id(0), damage, 700).changed)
                throw std::runtime_error("Reaction capture damage failed");
            if (lethal) {
                for (const auto elapsed : {0u, 100u, 200u, 400u, 600u, 1199u, 1200u})
                    capture(prefix + "-" + std::to_string(elapsed), 700 + elapsed);
                for (int action = 0; action < 6; ++action) {
                    const auto kind = static_cast<Action>(action);
                    Player from{{18, 18}, Heading::North}, to = from;
                    if (!tryAction(map, to, kind))
                        continue;
                    state.player = to;
                    ++state.revision;
                    state.transition = {from, to, kind, Transition::duration * .5f};
                    capture(prefix + "-transition-" + std::to_string(action), 1300);
                }
            } else {
                for (const auto elapsed : {0u, 100u, 150u, 300u})
                    capture(prefix + "-" + std::to_string(elapsed), 700 + elapsed);
            }
        }
    }
    // Retained corpse and a living replacement in exactly the same subslot.
    auto instances = placements.instances();
    for (auto& npc : instances)
        npc.animation = NpcAnimation::Idle;
    if (instances.size() >= 2) {
        instances[0].subcell = {0, 0};
        instances[1].subcell = {0, 1};
        instances.resize(2);
        const NpcWorld pair(map, Player{}.cell, types, instances);
        ActorWorld actors(map, catalog, pair);
        actors.damage(actors.id(1), actors.actor(actors.id(1))->hitPoints, 700);
        actors.move(actors.id(0), instances[1].cell, instances[1].subcell, {18, 18});
        NpcAnimator animator(actors, timings);
        animator.setTime(1300);
        FramePipeline frames;
        MovementState state;
        state.player = {{18, 18}, Heading::North};
        frames.update(renderer, map, state, &actors, &animator.snapshot());
        CommandController commands(party, actors);
        commands.refresh(frames.picking()->targets(), false, false, true);
        presenter.present(frames.frame(), state, map, false, true, &commands.snapshot(), frames.picking());
        presenter.capture(directory / "reaction-corpse-replacement.png");
    }
}
} // namespace blobber
