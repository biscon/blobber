#include "app/archer_captures.hpp"
#include "app/ai_demo.hpp"
#include "game/perception.hpp"
#include <fstream>
#include <stdexcept>

namespace blobber {
void captureArchers(const rules::Catalog& catalog, const NpcWorld& source,
                    const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                    Presenter& presenter, const std::filesystem::path& directory) {
    const auto map = aiDemoMap();
    const Party party(catalog);
    std::vector<std::string> names;
    NpcTypeId archer{}, melee{};
    bool foundArcher = false, foundMelee = false;
    for (std::size_t i = 0; i < source.typeCount(); ++i) {
        const NpcTypeId id{static_cast<std::uint16_t>(i)};
        names.push_back(source.typeId(id));
        if (names.back() == "goblin.archer") {
            archer = id;
            foundArcher = true;
        }
        if (names.back() == "goblin") {
            melee = id;
            foundMelee = true;
        }
    }
    if (!foundArcher || !foundMelee)
        throw std::runtime_error("Archer captures require both production appearances");
    for (const auto type : {melee, archer}) {
        const auto* monster = catalog.monster(catalog.findMonster(names[type.index]));
        const auto expected =
            type.index == archer.index ? rules::AttackKind::Ranged : rules::AttackKind::Melee;
        if (!monster || monster->attackCount != 1 || catalog.attack(monster->attacks[0])->kind != expected)
            throw std::runtime_error("Production role must have exactly its intended weapon kind");
    }
    std::ofstream trace(directory / "archer-trace.tsv");
    if (!trace)
        throw std::runtime_error("Cannot create archer capture trace");
    trace << "case\ttime_ms\tslot\ttype\tclip\tframe\tvisible\tpickable\n";
    const Player camera{{8, 11}, Heading::North};
    const std::array<Heading, 4> directions{Heading::South, Heading::North, Heading::West, Heading::East};
    const std::array<const char*, 4> sideNames{"front", "back", "left", "right"};
    const std::array<const char*, npcAnimationCount> clipNames{"idle", "walk",  "attack",
                                                               "hurt", "death", "bow"};
    auto present = [&](const std::string& name, ActorWorld& actors, NpcAnimator& animator,
                       FramePipeline& frames, MovementState& state, std::uint64_t time, int expectedTargets) {
        animator.setTime(time);
        frames.update(renderer, map, state, &actors, &animator.snapshot());
        const auto* picking = frames.picking();
        if (expectedTargets >= 0 && (!picking || picking->targets().count != std::size_t(expectedTargets)))
            throw std::runtime_error("Archer capture visibility/picking mismatch: " + name);
        CommandController commands(party, actors);
        commands.refresh(picking ? picking->targets() : TargetSnapshot{}, state.transition.active(), false,
                         true);
        presenter.present(frames.frame(), state, map, false, true, &commands.snapshot(), picking);
        presenter.capture(directory / ("archer-" + name + ".png"));
        if (name.find("roles") != std::string::npos || name == "idle-front-0")
            presenter.capture(directory / ("archer-" + name + "-720.png"), 1280, 720);
        for (std::size_t slot = 0; slot < actors.snapshot().count; ++slot) {
            const auto& frame = animator.snapshot().frames[slot];
            bool pickable = false;
            if (picking)
                for (std::size_t i = 0; i < picking->targets().count; ++i)
                    pickable |= picking->targets().ids[i].slot == slot;
            trace << name << '\t' << time << '\t' << slot << '\t'
                  << names[actors.snapshot().actors[slot].npc.type.index] << '\t' << int(frame.clip) << '\t'
                  << int(frame.active) << '\t' << frame.visible << '\t' << pickable << '\n';
        }
    };
    for (std::size_t direction = 0; direction < 4; ++direction) {
        for (const auto kind : {NpcAnimation::Idle, NpcAnimation::Walk, NpcAnimation::RangedAttack,
                                NpcAnimation::Hurt, NpcAnimation::Death}) {
            NpcInstance npc{archer, {8, 10}, {0, 1}, directions[direction]};
            if (kind == NpcAnimation::Walk)
                npc.animation = kind;
            const NpcWorld placements(map, camera.cell, names, {npc});
            ActorWorld actors(map, catalog, placements);
            NpcAnimator animator(actors, timings);
            FramePipeline frames;
            MovementState state;
            state.player = camera;
            if (kind == NpcAnimation::RangedAttack)
                actors.attack(actors.id(0), 0, true);
            if (kind == NpcAnimation::Hurt || kind == NpcAnimation::Death)
                actors.damage(actors.id(0), kind == NpcAnimation::Death ? 999 : 1, 0);
            const auto& timing = timings[archer.index][static_cast<std::size_t>(kind)];
            if (!timing.count)
                throw std::runtime_error("Archer is missing a required animation");
            std::uint64_t start = 0;
            const auto prefix =
                std::string(clipNames[static_cast<std::size_t>(kind)]) + "-" + sideNames[direction];
            for (std::size_t frame = 0; frame < timing.count; ++frame) {
                // Skip the first 100 ms white overlay when reviewing reaction artwork.
                const auto time =
                    start +
                    ((frame == 0 && (kind == NpcAnimation::Hurt || kind == NpcAnimation::Death)) ? 100 : 0);
                present(prefix + "-" + std::to_string(frame), actors, animator, frames, state, time,
                        kind == NpcAnimation::Death ? 0 : 1);
                if (animator.snapshot().frames[0].clip != kind ||
                    animator.snapshot().frames[0].active != frame)
                    throw std::runtime_error("Archer capture sampled the wrong authored pose");
                start += timing.durationMs[frame];
            }
            if (kind == NpcAnimation::Death)
                present(prefix + "-expired", actors, animator, frames, state, start, 0);
            if (kind == NpcAnimation::Idle) {
                state.player.cell = {8, 13};
                ++state.revision;
                present(prefix + "-far", actors, animator, frames, state, 0, 1);
                state.player = camera;
                ++state.revision;
                for (const auto action : {Action::Left, Action::Right}) {
                    auto destination = camera;
                    if (!tryAction(map, destination, action))
                        throw std::runtime_error("Archer strafe fixture is blocked");
                    state.player = destination;
                    state.transition = {camera, destination, action, Transition::duration * .5f};
                    ++state.revision;
                    present(prefix + "-strafe-" + std::to_string(int(action)), actors, animator, frames,
                            state, 0, -1);
                }
            }
        }
        const NpcWorld walking(map, camera.cell, names, {{archer, {7, 9}, {0, 1}, directions[direction]}});
        ActorWorld actors(map, catalog, walking);
        NpcAnimator animator(actors, timings);
        FramePipeline frames;
        MovementState state;
        state.player = camera;
        if (!actors.walk(actors.id(0), {8, 9}, {0, 1}, camera.cell, 0).changed)
            throw std::runtime_error("Archer travel fixture failed");
        for (const auto time : {0u, 150u, 300u, 450u, 600u})
            present(std::string("travel-") + sideNames[direction] + "-" + std::to_string(time), actors,
                    animator, frames, state, time, 1);
    }
    for (int heading = 0; heading < 4; ++heading) {
        const Player player{{8, 10}, static_cast<Heading>(heading)};
        std::vector<NpcInstance> instances;
        const auto f = forward(player.heading);
        for (int side = -1; side <= 1; ++side)
            for (int sub = 0; sub < 4; ++sub) {
                const Cell local{sub % 2 * 2 - 1, sub / 2 * 2 - 1};
                if (local.x * f.x + local.y * f.y >= 0)
                    continue;
                const auto cell = player.cell + f + right(player.heading) * side;
                instances.push_back({instances.size() % 2 ? melee : archer,
                                     cell,
                                     {sub % 2, sub / 2},
                                     headingToward(cell, player.cell)});
            }
        const NpcWorld placements(map, player.cell, names, instances);
        ActorWorld actors(map, catalog, placements);
        NpcAnimator animator(actors, timings);
        FramePipeline frames;
        MovementState state;
        state.player = player;
        present("roles-front-band-" + std::to_string(heading), actors, animator, frames, state, 0, 6);
        for (std::size_t i = 0; i < instances.size(); ++i)
            actors.attack(actors.id(i), 0, instances[i].type.index == archer.index);
        present("roles-release-" + std::to_string(heading), actors, animator, frames, state, 600, 6);
    }
    {
        const Player player{{6, 4}, Heading::West};
        const NpcWorld placements(map, player.cell, names, {{archer, {4, 4}, {0, 0}, Heading::East}});
        ActorWorld actors(map, catalog, placements);
        NpcAnimator animator(actors, timings);
        FramePipeline frames;
        MovementState state;
        state.player = player;
        present("wall-occlusion", actors, animator, frames, state, 0, 0);
    }
    if (!trace)
        throw std::runtime_error("Failed writing archer capture trace");
}
} // namespace blobber
