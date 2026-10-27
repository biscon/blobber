#include "app/projectile_captures.hpp"
#include "app/ai_demo.hpp"
#include <fstream>
#include <stdexcept>
namespace blobber {
void captureProjectiles(const rules::Catalog& catalog, const NpcWorld& source,
                        const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                        Presenter& presenter, const std::filesystem::path& directory) {
    const auto map = aiDemoMap();
    const Player spawn{{8, 11}, Heading::North};
    std::vector<std::string> names;
    NpcTypeId melee{}, archer{};
    for (std::size_t i = 0; i < source.typeCount(); ++i) {
        names.push_back(source.typeId({static_cast<std::uint16_t>(i)}));
        if (names.back() == "goblin")
            melee.index = i;
        if (names.back() == "goblin.archer")
            archer.index = i;
    }
    std::ofstream trace(directory / "projectiles.tsv");
    if (!trace)
        throw std::runtime_error("Cannot create projectile trace");
    trace << "case\ttime_ms\tphase\tattack\tstage\trelease_ms\timpact_ms\tnpc_hp\trecipient_hp\n";
    for (bool incoming : {false, true})
        for (int scenario = 0; scenario < 7; ++scenario)
            for (unsigned seed : (incoming && scenario == 6 ? std::array<unsigned, 2>{8, 3}
                                                            : std::array<unsigned, 2>{5, 12})) {
                const Cell target = scenario == 0                    ? Cell{8, 9}
                                    : scenario == 1 || scenario >= 4 ? Cell{8, 6}
                                    : scenario == 2                  ? Cell{7, 10}
                                                                     : Cell{9, 10};
                NpcWorld placements(
                    map, spawn.cell, names,
                    {{incoming ? archer : melee, target, {scenario == 2 ? 0 : 1, 1}, Heading::South}});
                GameSession s(map, catalog, placements, seed, spawn, incoming, timings);
                if (!incoming && scenario == 5)
                    s.actors().damage(s.actors().id(0), s.actors().snapshot().actors[0].hitPoints - 1, 0);
                NpcAnimator animator(s.actors(), timings);
                FramePipeline frames;
                CommandController ui(s, timings);
                auto refresh = [&] {
                    animator.setTime(s.timeMs());
                    frames.update(renderer, map, s.movement(), &s.actors(), &animator.snapshot(),
                                  &s.combat().attacks(), s.timeMs());
                    ui.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{},
                               s.movement().transition.active(), false, true);
                };
                refresh();
                CombatOutcome accepted;
                for (int i = 0; i < 200 && accepted.event != CombatEvent::AttackStarted; ++i) {
                    const auto& state = s.combat().snapshot();
                    if (!incoming && (realTime(state.phase) ||
                                      (state.awaitingAction && state.activeCharacter->index == 2))) {
                        PartyCommand command;
                        command.character = {2};
                        command.action = PartyAction::Ranged;
                        command.target = s.actors().id(0);
                        command.world = state.world;
                        command.turn = state.turn;
                        accepted = s.submit(command);
                        ui.record(accepted);
                        if (accepted.reason != CommandReason::Resolved)
                            throw std::runtime_error("Projectile capture shot rejected");
                    } else if (state.awaitingAction) {
                        PartyCommand command;
                        command.character = *state.activeCharacter;
                        command.action = PartyAction::Defend;
                        command.world = state.world;
                        command.turn = state.turn;
                        ui.record(s.submit(command));
                    } else {
                        const auto& events = s.update(.05, false);
                        ui.record(events);
                        if (incoming && events.step.event == CombatEvent::AttackStarted)
                            accepted = events.step;
                    }
                    refresh();
                }
                if (accepted.event != CombatEvent::AttackStarted)
                    throw std::runtime_error("Projectile capture did not start attack");
                const auto index = incoming ? std::size_t(accepted.enemy.slot)
                                            : NpcWorld::maxInstances + accepted.character.index;
                auto shot = s.combat().attacks().attacks[index];
                const std::string name = std::string(incoming ? "incoming-" : "outgoing-") +
                                         std::to_string(scenario) + "-seed" + std::to_string(seed);
                auto save = [&](const char* phase) {
                    refresh();
                    presenter.present(frames.frame(), s.movement(), map, false, true, &ui.snapshot(),
                                      frames.picking());
                    presenter.capture(directory / (name + "-" + phase + ".png"));
                    presenter.capture(directory / (name + "-" + phase + "-720.png"), 1280, 720);
                    const auto& v = s.combat().attacks().attacks[index];
                    trace << name << '-' << phase << '\t' << s.timeMs() << '\t'
                          << int(s.combat().snapshot().phase) << '\t' << v.id.serial << '\t' << int(v.stage)
                          << '\t' << v.releaseMs << '\t' << v.impactMs << '\t'
                          << s.actors().snapshot().actors[0].hitPoints << '\t'
                          << s.party().character(v.character)->hitPoints << '\n';
                };
                auto npcHp = s.actors().snapshot().actors[0].hitPoints;
                const auto partyHp = s.party().character(shot.character)->hitPoints;
                const auto advance = [&](std::uint64_t when) {
                    if (when < s.timeMs())
                        throw std::runtime_error("Capture clock regressed");
                    ui.record(s.update(double(when - s.timeMs()) / 1000, false));
                };
                if (incoming && scenario == 6) {
                    PartyCommand counter;
                    counter.character = {2};
                    counter.action = PartyAction::Ranged;
                    counter.target = s.actors().id(0);
                    counter.world = s.combat().snapshot().world;
                    counter.turn = s.combat().snapshot().turn;
                    const auto acceptedCounter = s.submit(counter);
                    if (acceptedCounter.reason != CommandReason::Resolved)
                        throw std::runtime_error("Capture counter-shot rejected");
                    ui.record(acceptedCounter);
                    advance(s.combat().attacks().attacks[NpcWorld::maxInstances + 2].impactMs);
                    save("interruption");
                    shot = s.combat().attacks().attacks[index];
                    npcHp = s.actors().snapshot().actors[0].hitPoints;
                    if (shot.stage == AttackStage::Canceled)
                        continue;
                    if (shot.releaseMs > s.timeMs() + 300) {
                        advance(s.timeMs() + 300);
                        save("resumed");
                    }
                }
                if (shot.releaseMs > s.timeMs()) {
                    advance(shot.releaseMs - 1);
                    save("before-release");
                }
                advance(shot.releaseMs);
                save("launch");
                shot = s.combat().attacks().attacks[index];
                advance(shot.releaseMs + (shot.impactMs - shot.releaseMs) / 2);
                save("flight");
                if (scenario == 4) {
                    s.move(Action::Left);
                    advance(s.timeMs() + 80);
                    save("moving");
                }
                if (scenario == 5 && incoming) {
                    s.actors().damage(s.actors().id(0), npcHp, s.timeMs());
                    npcHp = 0;
                    save("shooter-dead");
                }
                if (scenario == 6 && !incoming) {
                    if (!s.actors().walk(s.actors().id(0), {7, 6}, {1, 1}, s.movement().player.cell,
                                         s.timeMs()))
                        throw std::runtime_error("Capture target travel rejected");
                    advance(s.timeMs() + 80);
                    save("target-moving");
                }
                if (scenario == 1) {
                    s.update(60, true);
                    save("paused");
                }
                advance(shot.impactMs - 1);
                save("before-impact");
                if (s.actors().snapshot().actors[0].hitPoints != npcHp ||
                    s.party().character(shot.character)->hitPoints != partyHp)
                    throw std::runtime_error("Capture found premature health change");
                advance(shot.impactMs);
                save("impact");
                advance(shot.impactMs + 60);
                save("ending");
                advance(s.combat().attacks().attacks[index].endMs);
                save("settled");
            }
}
} // namespace blobber
