#include "app/ai_demo.hpp"
#include <fstream>
#include <stdexcept>
#include <string_view>
namespace blobber {
Map aiDemoMap() {
    return Map({"###############", "#.............#", "#....#........#", "#....#........#", "#....#........#",
                "#....#........#", "#....#........#", "#....#........#", "#.............#", "#.............#",
                "#.............#", "#.............#", "#.............#", "#.............#",
                "###############"});
}
Player aiDemoSpawn() noexcept {
    return {{3, 11}, Heading::North};
}
namespace {
std::vector<std::string> types(const NpcWorld& source) {
    std::vector<std::string> names;
    for (std::size_t i = 0; i < source.typeCount(); ++i)
        names.push_back(source.typeId({static_cast<std::uint16_t>(i)}));
    return names;
}
NpcTypeId goblin(const NpcWorld& source) {
    for (std::size_t i = 0; i < source.typeCount(); ++i)
        if (source.typeId({static_cast<std::uint16_t>(i)}) == "goblin")
            return {static_cast<std::uint16_t>(i)};
    throw std::runtime_error("AI demo requires goblin appearance");
}
} // namespace
NpcWorld aiDemoPlacements(const Map& map, const NpcWorld& source) {
    const auto type = goblin(source);
    std::vector<NpcInstance> actors{{type, {3, 6}, {0, 0}, Heading::North},
                                    {type, {9, 5}, {0, 0}, Heading::South},
                                    {type, {10, 12}, {0, 0}, Heading::East},
                                    {type, {7, 3}, {0, 0}, Heading::East}};
    auto& patrol = actors[1].ai;
    patrol.routine = NpcRoutine::Patrol;
    patrol.waypointCount = 4;
    patrol.waypoints = {Cell{9, 9}, Cell{12, 9}, Cell{12, 5}, Cell{9, 5}};
    actors[2].ai.routine = NpcRoutine::Wander;
    return {map, aiDemoSpawn().cell, types(source), std::move(actors)};
}
void captureAi(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
               const std::vector<NpcTimings>& timings, const SceneRenderer& renderer, Presenter& presenter,
               const std::filesystem::path& directory) {
    std::ofstream trace(directory / "ai-trace.tsv");
    if (!trace)
        throw std::runtime_error("Cannot open AI capture trace");
    trace << "case\ttime_ms\tslot\tx\ty\tfacing\tawareness\tclip\tpresent_x\tpresent_y\tcombat_phase\n";
    auto run = [&](const char* name, const NpcWorld& world, Player spawn, bool corner, bool mapView) {
        GameSession session(map, catalog, world, 5, spawn, true, timings);
        session.setAnimated(false);
        NpcAnimator animator(session.actors(), timings);
        FramePipeline frames;
        CommandController commands(session, timings);
        for (int time = 0; time <= 7200; time += 50) {
            if (time)
                commands.record(session.update(.05, false));
            if (corner && time == 200) {
                session.move(Action::TurnLeft);
                session.move(Action::Forward);
                session.move(Action::Forward);
                session.move(Action::TurnRight);
                session.move(Action::TurnRight);
            }
            const bool travelFrame =
                std::string_view(name) == "travel" && time >= 1200 && time <= 1800 && time % 150 == 0;
            const bool approachFrame =
                std::string_view(name) == "approach" &&
                (time == 850 || time == 1150 || time == 1300 || time == 2800 || time == 3050 || time == 3100);
            if (!travelFrame && !approachFrame && time != 0 && time != 700 && time != 1000 && time != 1900 &&
                time != 2500 && time != 3700 && time != 4900 && time != 7200)
                continue;
            animator.setTime(session.timeMs());
            frames.update(renderer, map, session.movement(), &session.actors(), &animator.snapshot());
            const auto* picking = frames.picking();
            commands.refresh(picking ? picking->targets() : TargetSnapshot{}, false, mapView, true);
            presenter.present(frames.frame(), session.movement(), map, mapView, true, &commands.snapshot(),
                              picking);
            presenter.capture(directory / (std::string("ai-") + name + "-" + std::to_string(time) + ".png"));
            for (std::size_t i = 0; i < world.instances().size(); ++i) {
                const auto& a = session.actors().snapshot().actors[i];
                trace << name << '\t' << session.timeMs() << '\t' << i << '\t' << a.npc.cell.x << '\t'
                      << a.npc.cell.y << '\t' << static_cast<unsigned>(a.npc.facing) << '\t'
                      << static_cast<unsigned>(session.ai().actors[i].awareness) << '\t'
                      << static_cast<unsigned>(animator.snapshot().frames[i].clip) << '\t'
                      << actorPosition(a, session.timeMs()).x << '\t' << actorPosition(a, session.timeMs()).y
                      << '\t' << static_cast<unsigned>(session.combat().snapshot().phase) << '\n';
            }
        }
    };
    run("routines", placements, aiDemoSpawn(), false, false);
    run("patrol", placements, {{9, 12}, Heading::North}, false, false);
    run("wander", placements, {{10, 9}, Heading::South}, false, false);
    const auto type = goblin(placements);
    NpcWorld approach(map, {3, 11}, types(placements), {{type, {3, 6}, {0, 0}, Heading::South}});
    run("approach", approach, aiDemoSpawn(), false, false);
    NpcWorld corner(map, {7, 8}, types(placements), {{type, {3, 8}, {0, 0}, Heading::East}});
    run("corner", corner, {{7, 8}, Heading::East}, true, false);
    auto lateralNpc = NpcInstance{type, {7, 10}, {0, 0}, Heading::East};
    lateralNpc.ai.routine = NpcRoutine::Patrol;
    lateralNpc.ai.waypointCount = 2;
    lateralNpc.ai.waypoints = {Cell{10, 10}, Cell{7, 10}};
    NpcWorld lateral(map, {8, 12}, types(placements), {lateralNpc});
    run("travel", lateral, {{8, 12}, Heading::North}, false, false);
    if (!trace)
        throw std::runtime_error("Failed to write AI capture trace");
}
} // namespace blobber
