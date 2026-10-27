#include "app/hybrid_demo.hpp"
#include "app/ai_demo.hpp"
#include "game/perception.hpp"
#include <fstream>
#include <stdexcept>
namespace blobber {
Map hybridDemoMap() {
    return aiDemoMap();
}
Player hybridDemoSpawn() noexcept {
    return {{3, 11}, Heading::North};
}
namespace {
std::vector<std::string> typeNames(const NpcWorld& source) {
    std::vector<std::string> names;
    for (std::size_t i = 0; i < source.typeCount(); ++i)
        names.push_back(source.typeId({static_cast<std::uint16_t>(i)}));
    return names;
}
NpcTypeId type(const NpcWorld& source, const char* name = "goblin") {
    for (std::size_t i = 0; i < source.typeCount(); ++i)
        if (source.typeId({static_cast<std::uint16_t>(i)}) == name)
            return {static_cast<std::uint16_t>(i)};
    throw std::runtime_error(std::string("Hybrid demo requires appearance: ") + name);
}
} // namespace
NpcWorld hybridDemoPlacements(const Map& map, const NpcWorld& source, const rules::Catalog& catalog) {
    const auto t = type(source);
    std::vector<NpcInstance> actors{{t, {3, 6}, {0, 0}, Heading::North}, {t, {4, 5}, {1, 0}, Heading::North},
                                    {t, {8, 9}, {0, 0}, Heading::West},  {t, {9, 12}, {0, 0}, Heading::West},
                                    {t, {2, 7}, {0, 0}, Heading::South}, {t, {4, 7}, {1, 0}, Heading::South}};
    const auto archer = catalog.findMonster("goblin.archer");
    if (!catalog.monster(archer))
        throw std::runtime_error("Hybrid demo requires goblin.archer rules");
    actors[4].type = actors[5].type = type(source, "goblin.archer");
    actors[2].ai.routine = NpcRoutine::Patrol;
    actors[2].ai.waypointCount = 2;
    actors[2].ai.waypoints = {Cell{6, 9}, Cell{10, 9}};
    return {map, hybridDemoSpawn().cell, typeNames(source), std::move(actors)};
}
void captureHybrid(const Map& map, const rules::Catalog& catalog, const NpcWorld& source,
                   const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                   Presenter& presenter, const std::filesystem::path& directory) {
    std::ofstream trace(directory / "hybrid-trace.tsv");
    if (!trace)
        throw std::runtime_error("Cannot create hybrid trace");
    trace << "case\tpresentation_ms\tgameplay_ms\tphase\tround\tturn\tactor\tx\ty\thp\tclip\n";
    auto run = [&](const char* name, Player spawn, std::vector<NpcInstance> instances, int mode) {
        NpcWorld placements(map, spawn.cell, typeNames(source), std::move(instances));
        GameSession s(map, catalog, placements, 5, spawn, mode < 10, timings);
        s.setAnimated(mode == 1 || mode == 2 || mode == 5);
        NpcAnimator animator(s.actors(), timings);
        FramePipeline frames;
        CommandController ui(s, timings);
        auto save = [&](const std::string& suffix) {
            animator.setTime(s.timeMs());
            frames.update(renderer, map, s.movement(), &s.actors(), &animator.snapshot(),
                          &s.combat().attacks(), s.timeMs());
            ui.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{},
                       s.movement().transition.active(), false, true);
            presenter.present(frames.frame(), s.movement(), map, false, true, &ui.snapshot(),
                              frames.picking());
            presenter.capture(directory / (std::string("hybrid-") + name + "-" + suffix + ".png"));
            const auto& state = s.combat().snapshot();
            for (std::size_t i = 0; i < s.actors().snapshot().count; ++i) {
                const auto& a = s.actors().snapshot().actors[i];
                trace << name << '\t' << s.timeMs() << '\t' << state.gameplayMs << '\t' << int(state.phase)
                      << '\t' << state.round << '\t' << state.turn << '\t' << i << '\t' << a.npc.cell.x
                      << '\t' << a.npc.cell.y << '\t' << a.hitPoints << '\t'
                      << int(animator.snapshot().frames[i].clip) << '\n';
            }
        };
        if (mode == 3)
            s.actors().damage(s.actors().id(0), 4, 0);
        if (mode >= 10) {
            s.actors().attack(s.actors().id(0), 0, true);
            for (unsigned ms : {0u, 599u, 600u, 999u, 1000u}) {
                if (ms > s.timeMs())
                    s.update((ms - s.timeMs()) / 1000.0, false);
                save(std::to_string(ms));
            }
            return;
        }
        if (mode == 8) {
            for (int step = 0; step < 100; ++step) {
                const auto st = s.combat().snapshot();
                if (st.awaitingAction && st.activeCharacter->index == 0)
                    break;
                if (st.awaitingAction) {
                    PartyCommand command;
                    command.character = *st.activeCharacter;
                    command.action = PartyAction::Defend;
                    command.world = st.world;
                    command.turn = st.turn;
                    ui.record(s.submit(command));
                } else
                    ui.record(s.update(.1, false));
            }
            save("front-line");
            if (ui.snapshot().targets.count != 6)
                throw std::runtime_error("All six front-line subslots must remain visible and clickable");
            const auto st = s.combat().snapshot();
            PartyCommand attack;
            attack.character = {0};
            attack.action = PartyAction::Melee;
            attack.world = st.world;
            attack.turn = st.turn;
            for (std::size_t i = 0; i < 6; ++i) {
                attack.target = s.actors().id(i);
                if (s.combat().validate(attack, spawn) != CommandReason::Ready)
                    throw std::runtime_error("Every exposed front-line target must permit melee");
            }
            attack.target = s.actors().id(0);
            ui.hover(HoverKind::Enemy, 0, attack.target, 200, 180);
            save("diagonal-melee-ready");
            ui.record(s.submit(attack));
            save("diagonal-melee-hit");
            return;
        }
        save("initial");
        if (mode == 2) {
            for (int step = 0; step < 100; ++step) {
                const auto st = s.combat().snapshot();
                if (st.awaitingAction && st.activeCharacter->index == 0)
                    break;
                if (st.awaitingAction) {
                    PartyCommand command;
                    command.character = *st.activeCharacter;
                    command.action = PartyAction::Defend;
                    command.world = st.world;
                    command.turn = st.turn;
                    ui.record(s.submit(command));
                } else
                    ui.record(s.update(.1, false));
            }
            const auto before = s.combat().snapshot();
            if (!before.awaitingAction || before.activeCharacter->index != 0)
                throw std::runtime_error("Handoff fixture did not reach fighter");
            save("fighter-before");
            // A diagnostic fixture mutation isolates the reported handoff from combat RNG.
            s.actors().damage(s.actors().id(0), 1000, s.timeMs());
            ui.record(s.update(0, false));
            save("turn-start");
            s.update(60, true);
            save("paused");
            s.update(.159, false);
            save("turn-tail");
            s.update(.001, false);
            save("fighter-after");
            const auto after = s.combat().snapshot();
            if (!after.awaitingAction || after.activeCharacter->index != 0 ||
                after.gameplayMs != before.gameplayMs)
                throw std::runtime_error("Handoff consumed fighter action or gameplay time");
            PartyCommand attack;
            attack.character = {0};
            attack.action = PartyAction::Melee;
            attack.target = s.actors().id(1);
            attack.world = after.world;
            attack.turn = after.turn;
            const auto out = s.submit(attack);
            if (out.reason != CommandReason::Resolved)
                throw std::runtime_error("Fighter cannot attack side arrival");
            ui.record(out);
            save("fighter-attacks");
            return;
        }

        if (mode == 0) {
            ui.selectCharacter(2);
            ui.selectAction(PartyAction::Ranged);
            ui.selectTarget(s.actors().id(0));
            ui.confirm();
            save("opening-shot");
            s.update(60, true);
            save("paused");
        }
        std::uint64_t lastTurn = ~std::uint64_t(0);
        int captured = 0, shots = 0;
        bool rangeCaptured = false;
        for (int step = 0; step < 220; ++step) {
            const auto& events = s.update(.1, false);
            ui.record(events);
            const auto out = events.step;
            if (mode == 6) {
                shots += out.event == CombatEvent::AttackStarted && out.kind == rules::AttackKind::Ranged;
                for (std::size_t i = 0; i < placements.instances().size(); ++i)
                    if (!(s.actors().snapshot().actors[i].npc.cell == placements.instances()[i].cell))
                        throw std::runtime_error("Bow-only demo goblin abandoned its clear shot");
            }
            const auto st = s.combat().snapshot();
            if (mode == 7 && st.round > 0) {
                for (std::size_t i = 0; i < 2; ++i) {
                    const auto& a = s.actors().snapshot().actors[i];
                    if (a.life == ActorLife::Alive && a.hitPoints == a.maxHitPoints &&
                        !inPartyView(map, spawn.cell, spawn.heading, a.npc.cell))
                        throw std::runtime_error("Healthy front pursuer left the visible demo fight");
                }
            }
            if (st.turn != lastTurn && captured < 16) {
                save(std::to_string(s.timeMs()));
                lastTurn = st.turn;
                ++captured;
            }
            if (mode == 7 && !rangeCaptured && st.awaitingAction && st.activeCharacter->index == 0) {
                save("range-before");
                ui.selectAction(PartyAction::Melee);
                ui.selectTarget(s.actors().id(4));
                if (ui.confirm() != CommandReason::OutOfRange || s.combat().snapshot().turn != st.turn)
                    throw std::runtime_error("Distant archer melee rejection must preserve the active turn");
                ui.hover(HoverKind::Enemy, 0, s.actors().id(4), 740, 580);
                save("melee-out-of-range");
                ui.hover(HoverKind::None);
                rangeCaptured = true;
            }
            if (st.phase == CombatPhase::Victory || st.phase == CombatPhase::Defeat)
                break;
            if (st.awaitingAction) {
                PartyCommand cmd;
                cmd.character = *st.activeCharacter;
                cmd.action = PartyAction::Defend;
                cmd.world = st.world;
                cmd.turn = st.turn;
                ui.record(s.submit(cmd));
            }
        }
        if (mode == 6 && shots < 2)
            throw std::runtime_error("Bow-only demo goblins did not shoot repeatedly");
        save("final");
    };
    const auto t = type(source);
    for (int d = 0; d < 4; ++d) {
        const Player p{{8, 10}, static_cast<Heading>(d)};
        std::vector<NpcInstance> line;
        for (int side = -1; side <= 1; ++side)
            for (int sub = 0; sub < 4; ++sub) {
                const Cell local{sub % 2 * 2 - 1, sub / 2 * 2 - 1};
                const auto f = forward(p.heading);
                if (local.x * f.x + local.y * f.y >= 0)
                    continue;
                line.push_back({t,
                                p.cell + f + right(p.heading) * side,
                                {sub % 2, sub / 2},
                                headingToward(p.cell + f, p.cell)});
            }
        const auto name = "front-band-" + std::to_string(d);
        run(name.c_str(), p, std::move(line), 8);
    }
    run("demo", hybridDemoSpawn(), source.instances(), 7);
    std::vector<NpcInstance> archers;
    for (const auto& npc : source.instances())
        if (npc.monster.value_or(catalog.findMonster(source.typeId(npc.type))).index ==
            catalog.findMonster("goblin.archer").index)
            archers.push_back(npc);
    if (archers.size() != 2)
        throw std::runtime_error("Hybrid demo must contain two bow-only goblins");
    run("ranged-only", hybridDemoSpawn(), std::move(archers), 6);
    run("approach", {{3, 11}, Heading::North},
        {{t, {3, 6}, {0, 0}, Heading::North}, {t, {3, 4}, {1, 1}, Heading::South}}, 0);
    run("side-contact", {{8, 10}, Heading::North},
        {{t, {8, 11}, {0, 0}, Heading::North}, {t, {9, 10}, {0, 0}, Heading::West}}, 1);
    run("front-handoff", {{8, 10}, Heading::North},
        {{t, {8, 9}, {0, 1}, Heading::South}, {t, {9, 10}, {0, 0}, Heading::West}}, 2);
    run("rear-contact", {{8, 10}, Heading::North}, {{t, {8, 11}, {0, 0}, Heading::North}}, 5);
    run("wounded-melee", {{3, 11}, Heading::North}, {{t, {3, 10}, {0, 1}, Heading::South}}, 3);
    run("reinforcement", {{8, 10}, Heading::North},
        {{t, {8, 9}, {0, 1}, Heading::South}, {t, {11, 10}, {0, 0}, Heading::West}}, 4);
    for (int d = 0; d < 4; ++d) {
        const std::string name = "bow-" + std::to_string(d);
        run(name.c_str(), {{3, 11}, Heading::North},
            {{type(source, "goblin.archer"), {3, 9}, {0, 0}, static_cast<Heading>(d)}}, 10 + d);
    }
    if (!trace)
        throw std::runtime_error("Failed writing hybrid trace");
}
} // namespace blobber
