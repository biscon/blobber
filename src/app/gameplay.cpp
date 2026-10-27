#include "app/gameplay.hpp"
#include "app/ai_demo.hpp"
#include "app/hybrid_demo.hpp"
#include "platform/input.hpp"
#include "ui/interaction.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>

namespace blobber {
NpcWorld gameplayPlacements(const Map& map, const NpcWorld& source) {
    auto instances = source.instances();
    for (auto& npc : instances)
        npc.animation = NpcAnimation::Idle;
    std::vector<std::string> types;
    for (std::size_t i = 0; i < source.typeCount(); ++i)
        types.push_back(source.typeId({static_cast<std::uint16_t>(i)}));
    return NpcWorld(map, Player{}.cell, std::move(types), std::move(instances));
}
Player combatCamera(const Map& map, const NpcWorld& placements) {
    for (const auto& actor : placements.instances())
        for (const auto heading : {Heading::North, Heading::East, Heading::South, Heading::West}) {
            const auto cell = actor.cell + forward(heading) * -1;
            if (!map.solid(cell) && !placements.occupied(cell))
                return {cell, heading};
        }
    throw std::runtime_error("Combat demo requires an accessible monster cell");
}
namespace {
struct PlayView {
    GameSession session;
    NpcAnimator animator;
    FramePipeline frames;
    CommandController commands;
    const Map& map;
    const SceneRenderer& renderer;
    PlayView(const Map& m, const rules::Catalog& catalog, const NpcWorld& placements,
             const std::vector<NpcTimings>& timings, const SceneRenderer& r, std::uint32_t seed, Player spawn,
             bool enableAi = false, const RecoveryLevel& recovery = {})
        : session(m, catalog, placements, seed, spawn, enableAi, timings, recovery),
          animator(session.actors(), timings), commands(session, timings), map(m), renderer(r) {}
    bool refresh(bool debug = false, bool focused = true) {
        animator.setTime(session.timeMs());
        const auto area = debug ? SpellArea{} : commands.floorPreview();
        const bool changed =
            frames.update(renderer, map, session.movement(), &session.actors(), &animator.snapshot(),
                          &session.combat().attacks(), session.timeMs(), &area, &session.loot().snapshot());
        const auto* picking = frames.picking();
        commands.refresh(picking ? picking->targets() : TargetSnapshot{},
                         session.movement().transition.active(), debug, focused);
        const auto currentArea = debug ? SpellArea{} : commands.floorPreview();
        if (area.kind != currentArea.kind || area.cells != currentArea.cells ||
            area.eligible != currentArea.eligible)
            return frames.update(renderer, map, session.movement(), &session.actors(), &animator.snapshot(),
                                 &session.combat().attacks(), session.timeMs(), &currentArea,
                                 &session.loot().snapshot()) ||
                   changed;
        return changed;
    }
};
} // namespace
void runGameplay(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                 const std::vector<NpcTimings>& timings, const SceneRenderer& renderer, Presenter& presenter,
                 const Options& options, const RecoveryLevel& recovery) {
    const auto idle = gameplayPlacements(map, placements);
    const Player spawn = options.combatDemo   ? combatCamera(map, idle)
                         : options.hybridDemo ? hybridDemoSpawn()
                         : options.aiDemo     ? aiDemoSpawn()
                                              : Player{};
    const std::uint32_t seed = options.seed ? *options.seed
                               : (options.combatDemo || options.aiDemo || options.hybridDemo)
                                   ? 5u
                                   : std::random_device{}();
    std::cout << "Gameplay seed: " << seed << '\n';
    Input input; // Keep edge latches across restart, so held confirmation never replays.
    for (;;) {
        PlayView view(map, catalog, idle, timings, renderer, seed, spawn, !options.combatDemo, recovery);
        bool debug = false, first = true, restart = false;
        view.refresh();
        if (options.inventoryDemo)
            view.commands.openInventory(rules::CharacterId{2});
        if (options.spellDemo) {
            view.commands.selectCharacter(3);
            view.commands.openSpellbook();
        }
        while (!WindowShouldClose()) {
            const float dt = std::min(GetFrameTime(), 0.1f);
            auto sample = input.poll(dt);
            if (debug && !view.commands.snapshot().modal() && sample.ui.has(UiKey::Cancel)) {
                debug = false;
                sample.ui.pressed[static_cast<std::size_t>(UiKey::Cancel)] = false;
            }
            if (sample.clearPending)
                view.session.clearPending();
            if (sample.toggleMap)
                debug = !debug;
            const bool campWasOpen = view.commands.snapshot().camp.open;
            view.commands.tickCamp(dt, sample.ui.focused);
            const bool paused =
                campWasOpen || !sample.ui.focused || debug || view.commands.snapshot().modal();
            const auto presentedCombat = view.commands.snapshot().combat;
            const auto previous = view.session.movement().player;
            view.commands.record(view.session.update(dt, paused));
            if (!paused) {
                if (sample.toggleAnimation)
                    view.session.setAnimated(!view.session.movement().animated);
                if (sample.action) {
                    view.session.move(*sample.action, sample.fresh);
                    view.commands.record(view.session.events());
                }
            }
            if (!(previous == view.session.movement().player))
                view.commands.movementStarted();
            bool changed = view.refresh(debug, sample.ui.focused);
            const auto& currentCombat = view.session.combat().snapshot();
            const auto phase = currentCombat.phase;
            const bool samePresentedTurn = confirmationMatches(presentedCombat, currentCombat);
            const bool result = phase == CombatPhase::Victory || phase == CombatPhase::Defeat;
            const bool confirmResult =
                sample.ui.focused && !paused &&
                (sample.ui.has(UiKey::Confirm) || (sample.ui.has(UiKey::Click) && sample.ui.pointer &&
                                                   hudLayout::resultButton.contains(*sample.ui.pointer)));
            if (result && samePresentedTurn && confirmResult) {
                if (phase == CombatPhase::Defeat) {
                    restart = true;
                    break;
                }
                view.session.combat().continueExploring();
                view.commands.cancel();
                view.session.clearPending();
            } else if (!(campWasOpen && !view.commands.snapshot().camp.open)) {
                const auto* picking = view.frames.picking();
                const auto hit =
                    picking && sample.ui.pointer
                        ? picking->pick(int(sample.ui.pointer->x) - viewX, int(sample.ui.pointer->y) - viewY)
                        : std::nullopt;
                auto lootHit = picking && sample.ui.pointer
                                   ? picking->pickLoot(int(sample.ui.pointer->x) - viewX,
                                                       int(sample.ui.pointer->y) - viewY)
                                   : std::nullopt;
                routeHudInput(view.commands, sample.ui, hit, samePresentedTurn && !result, lootHit);
            }
            if (view.commands.snapshot().quitRequested)
                return;
            changed |= view.refresh(debug, sample.ui.focused);
            presenter.present(view.frames.frame(), view.session.movement(), map, debug, changed || first,
                              &view.commands.snapshot(), view.frames.picking());
            first = false;
        }
        if (!restart)
            break;
    }
}
void captureCombat(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                   const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                   Presenter& presenter, const std::filesystem::path& directory) {
    if (placements.instances().empty())
        return;
    const auto idle = gameplayPlacements(map, placements);
    const auto spawn = combatCamera(map, idle);
    auto capture = [&](PlayView& view, const char* name) {
        view.refresh();
        presenter.present(view.frames.frame(), view.session.movement(), map, false, true,
                          &view.commands.snapshot(), view.frames.picking());
        presenter.capture(directory / (std::string(name) + ".png"));
    };
    {
        PlayView view(map, catalog, idle, timings, renderer, 5, spawn);
        view.commands.record(view.session.update(0, false));
        capture(view, "combat-start");
        for (int step = 0; step < 500 && view.session.combat().snapshot().phase != CombatPhase::Victory &&
                           view.session.combat().snapshot().phase != CombatPhase::Defeat;
             ++step) {
            view.refresh();
            const auto& combat = view.session.combat().snapshot();
            if (combat.awaitingAction) {
                view.commands.selectAction(PartyAction::Defend);
                if (view.session.combat().exposed(*combat.activeCharacter)) {
                    for (std::size_t i = 0; i < view.commands.snapshot().targets.count; ++i) {
                        const auto id = view.commands.snapshot().targets.ids[i];
                        if (view.session.combat().exposed(id) && view.session.combat().member(id)) {
                            view.commands.selectAction(PartyAction::Melee);
                            view.commands.selectTarget(id);
                            break;
                        }
                    }
                }
                view.commands.confirm();
            } else
                view.commands.record(view.session.update(0.45, false));
            if (step == 6)
                capture(view, "combat-exchange");
        }
        if (view.session.combat().snapshot().phase != CombatPhase::Victory)
            throw std::runtime_error("Deterministic combat capture did not reach victory");
        capture(view, "combat-victory");
        view.session.update(.6, false);
        capture(view, "combat-victory-corpse");
        view.session.combat().continueExploring();
        view.session.update(.6, false);
        capture(view, "combat-continue-corpses-expired");
    }
    {
        PlayView view(map, catalog, idle, timings, renderer, 5, spawn);
        view.commands.record(view.session.update(0, false));
        for (std::uint16_t i = 0; i < rules::partySize; ++i)
            view.session.party().damage({i}, view.session.party().character({i})->hitPoints);
        view.commands.record(view.session.update(0.45, false));
        capture(view, "combat-defeat");
    }
    {
        PlayView view(map, catalog, idle, timings, renderer, 5, spawn);
        view.session.party().damage({0}, view.session.party().character({0})->hitPoints);
        view.session.party().deathSave({0}, 1);
        view.session.party().damage({1}, view.session.party().character({1})->hitPoints);
        view.session.party().stabilize({1});
        view.commands.record(view.session.update(0, false));
        capture(view, "combat-front-collapse");
        view.session.party().heal({0}, 3);
        view.commands.record(view.session.update(.45, false));
        capture(view, "combat-healed-from-zero");
        view.commands.toggleLog();
        capture(view, "combat-log");
    }
    {
        PlayView view(map, catalog, idle, timings, renderer, 5, spawn);
        view.session.party().damage({0}, view.session.party().character({0})->hitPoints);
        view.commands.record(view.session.update(0, false));
        view.refresh();
        view.commands.selectAction(PartyAction::Aid);
        view.commands.selectAlly(0);
        capture(view, "combat-aid");
        if (view.commands.confirm() != CommandReason::Resolved)
            throw std::runtime_error("Aid capture failed to resolve");
        capture(view, "combat-aid-result");
    }
    {
        PlayView view(map, catalog, idle, timings, renderer, 5, spawn);
        view.commands.record(view.session.update(0, false));
        view.refresh();
        view.commands.selectAction(PartyAction::Defend);
        if (view.commands.confirm() != CommandReason::Resolved)
            throw std::runtime_error("Defend capture failed to resolve");
        capture(view, "combat-defend");
    }
}
} // namespace blobber
