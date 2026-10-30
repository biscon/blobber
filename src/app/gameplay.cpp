#include "app/gameplay.hpp"
#include "app/ai_demo.hpp"
#include "app/campaign_saves.hpp"
#include "app/hybrid_demo.hpp"
#include "game/arena.hpp"
#include "game/campaign.hpp"
#include "platform/input.hpp"
#include "ui/interaction.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>

namespace blobber {
NpcWorld gameplayPlacements(const Map& map, const NpcWorld& source, Cell spawn) {
    auto instances = source.instances();
    for (auto& npc : instances)
        npc.animation = NpcAnimation::Idle;
    std::vector<std::string> types;
    for (std::size_t i = 0; i < source.typeCount(); ++i)
        types.push_back(source.typeId({static_cast<std::uint16_t>(i)}));
    return NpcWorld(map, spawn, std::move(types), std::move(instances));
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
    std::unique_ptr<GameSession> ownedSession;
    GameSession& session;
    NpcAnimator animator;
    FramePipeline frames;
    CommandController commands;
    const Map& map;
    const SceneRenderer& renderer;
    std::optional<generation::DiagnosticView> diagnostics;
    PlayView(const Map& m, const rules::Catalog& catalog, const NpcWorld& placements,
             const std::vector<NpcTimings>& timings, const SceneRenderer& r, std::uint32_t seed, Player spawn,
             bool enableAi = false, const RecoveryLevel& recovery = {},
             const rules::PresetParty* starting = nullptr, const FloorFixtures& fixtures = {},
             const FloorObjectiveDefinition& objective = {})
        : ownedSession(std::make_unique<GameSession>(m, catalog, starting ? *starting : *catalog.preset(),
                                                     placements, seed, spawn, enableAi, timings, recovery,
                                                     fixtures, objective)),
          session(*ownedSession), animator(session.actors(), timings), commands(session, timings), map(m),
          renderer(r) {}
    PlayView(GameSession& game, const Map& m, const std::vector<NpcTimings>& timings, const SceneRenderer& r,
             const generation::GeneratedFloor* floor = nullptr)
        : session(game), animator(game.actors(), timings), commands(game, timings), map(m), renderer(r) {
        if (floor)
            diagnostics = generation::diagnosticView(*floor);
    }
    bool refresh(bool debug = false, bool focused = true) {
        animator.setTime(session.timeMs(), &session.effects());
        const auto area = debug ? SpellArea{} : commands.floorPreview();
        const bool changed = frames.update(
            renderer, session.geometry(), session.movement(), &session.actors(), &animator.snapshot(),
            &session.combat().attacks(), session.timeMs(), &area, &session.loot().snapshot());
        const auto* picking = frames.picking();
        commands.refresh(picking ? picking->targets() : TargetSnapshot{},
                         session.movement().transition.active(), debug, focused);
        const auto currentArea = debug ? SpellArea{} : commands.floorPreview();
        if (area.kind != currentArea.kind || area.cells != currentArea.cells ||
            area.eligible != currentArea.eligible || area.ground.revision != currentArea.ground.revision)
            return frames.update(renderer, session.geometry(), session.movement(), &session.actors(),
                                 &animator.snapshot(), &session.combat().attacks(), session.timeMs(),
                                 &currentArea, &session.loot().snapshot()) ||
                   changed;
        return changed;
    }
};
} // namespace
PlayOutcome runGameplay(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                        const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                        Presenter& presenter, const Options& options, const RecoveryLevel& recovery,
                        const rules::PresetParty* starting, const GameplayLaunch* launch) {
    const Player spawn = launch               ? launch->spawn
                         : options.combatDemo ? combatCamera(map, placements)
                         : options.hybridDemo ? hybridDemoSpawn()
                         : options.aiDemo     ? aiDemoSpawn()
                                              : Player{};
    const auto idle = gameplayPlacements(map, placements, spawn.cell);
    const std::uint32_t seed = launch         ? launch->seed
                               : options.seed ? *options.seed
                               : (options.combatDemo || options.aiDemo || options.hybridDemo)
                                   ? 5u
                                   : std::random_device{}();
    std::cout << "Gameplay seed: " << seed << '\n';
    bool inputArmed = false;
    Input input; // Keep edge latches across restart, so held confirmation never replays.
    auto* campaign = launch ? launch->campaign : nullptr;
    for (;;) {
        auto current =
            campaign ? std::make_unique<PlayView>(campaign->game(), campaign->map(), timings, renderer)
                     : std::make_unique<PlayView>(
                           map, catalog, idle, timings, renderer, seed, spawn,
                           launch ? launch->enableAi : !options.combatDemo, recovery, starting,
                           launch && launch->fixtures ? *launch->fixtures : FloorFixtures{},
                           launch && launch->objective ? *launch->objective : FloorObjectiveDefinition{});
        auto& initial = *current;
        CampaignSaveFlow saveFlow(launch ? launch->saves : nullptr);
        initial.commands.saveMenu().enabled = campaign && launch && launch->saves;
        initial.session.enableDevelopment(options.development);
        if (starting || campaign)
            initial.commands.enableMainMenu();
        bool debug = false, reveal = false, first = true, restart = false;
        if (launch && launch->arena)
            initial.commands.arenaContext(false);
        initial.refresh();
        if (options.inventoryDemo)
            initial.commands.openInventory(rules::CharacterId{2});
        if (options.spellDemo) {
            initial.commands.selectCharacter(3);
            initial.commands.openSpellbook();
        }
        for (;;) {
            auto& view = *current;
            if (WindowShouldClose() && (!campaign || saveFlow.closeRequested(*campaign, view.commands)))
                return PlayOutcome::Quit;
            const float dt = std::min(GetFrameTime(), 0.1f);
            auto sample = input.poll(dt);
            sample.ui.text = pollTextInput();
            if (!inputArmed) {
                inputArmed = !IsKeyDown(KEY_T) && !IsKeyDown(KEY_ENTER) && !IsKeyDown(KEY_F) &&
                             !IsKeyDown(KEY_W) && !IsKeyDown(KEY_S) && !IsKeyDown(KEY_A) &&
                             !IsKeyDown(KEY_D) && !IsKeyDown(KEY_Q) && !IsKeyDown(KEY_E) &&
                             !IsMouseButtonDown(MOUSE_BUTTON_LEFT);
                sample.ui.pressed.fill(false);
                sample.action.reset();
            }
            if (debug && !view.commands.snapshot().modal() && sample.ui.has(UiKey::Cancel)) {
                debug = false;
                sample.ui.pressed[static_cast<std::size_t>(UiKey::Cancel)] = false;
            }
            if (sample.clearPending)
                view.session.clearPending();
            if (sample.toggleMap && !view.commands.snapshot().modal())
                debug = !debug;
            if (debug && options.development && sample.ui.has(UiKey::Click) && sample.ui.pointer &&
                hudLayout::diagnosticMap.contains(*sample.ui.pointer)) {
                reveal = !reveal;
                sample.ui.pressed[std::size_t(UiKey::Click)] = false;
            }
            const bool campWasOpen = view.commands.snapshot().camp.open;
            view.commands.tickCamp(dt, sample.ui.focused);
            const bool paused =
                campWasOpen || !sample.ui.focused || debug || view.commands.snapshot().modal();
            const bool presentedCampaignTerminal = view.commands.snapshot().campaignTerminal;
            const auto presentedCombat = view.commands.snapshot().combat;
            const auto previous = view.session.movement().player;
            view.commands.record(campaign ? campaign->update(dt, paused) : view.session.update(dt, paused));
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
            const bool arenaResult =
                launch && launch->arena &&
                (phase == CombatPhase::Defeat ||
                 (phase == CombatPhase::Victory && arenaCleared(view.session.actors().snapshot())));
            if (launch && launch->arena)
                view.commands.arenaContext(arenaCleared(view.session.actors().snapshot()));
            const bool terminalCampaign = campaign && campaign->status() != CampaignStatus::Active;
            const bool result =
                terminalCampaign || phase == CombatPhase::Victory || phase == CombatPhase::Defeat;
            const bool confirmResult = (!terminalCampaign || presentedCampaignTerminal) &&
                                       sample.ui.focused && !paused &&
                                       (sample.ui.has(UiKey::Confirm) ||
                                        (sample.ui.has(UiKey::Click) && sample.ui.pointer &&
                                         (arenaResult ? hudLayout::arenaRematch : hudLayout::resultButton)
                                             .contains(*sample.ui.pointer)));
            if (arenaResult && samePresentedTurn && !paused && sample.ui.has(UiKey::Click) &&
                sample.ui.pointer && hudLayout::arenaEdit.contains(*sample.ui.pointer))
                return PlayOutcome::ArenaSetup;
            if (terminalCampaign && !view.commands.snapshot().save.terminalWarning && samePresentedTurn &&
                confirmResult)
                return PlayOutcome::MainMenu;
            if (result && samePresentedTurn && confirmResult) {
                if (phase == CombatPhase::Defeat || arenaResult) {
                    restart = true;
                    break;
                }
                view.session.combat().continueExploring();
                view.commands.cancel();
                view.session.clearPending();
            } else if ((!terminalCampaign || view.commands.snapshot().save.terminalWarning) &&
                       !(campWasOpen && !view.commands.snapshot().camp.open)) {
                const auto* picking = view.frames.picking();
                const auto hit =
                    picking && sample.ui.pointer
                        ? picking->pick(int(sample.ui.pointer->x) - viewX, int(sample.ui.pointer->y) - viewY)
                        : std::nullopt;
                auto lootHit = picking && sample.ui.pointer
                                   ? picking->pickLoot(int(sample.ui.pointer->x) - viewX,
                                                       int(sample.ui.pointer->y) - viewY)
                                   : std::nullopt;
                const auto groundHit =
                    sample.ui.pointer && !view.session.movement().transition.active()
                        ? SceneRenderer::groundCell(view.session.geometry(), view.session.movement().player,
                                                    int(sample.ui.pointer->x) - viewX,
                                                    int(sample.ui.pointer->y) - viewY)
                        : std::nullopt;
                routeHudInput(view.commands, sample.ui, hit, samePresentedTurn && !result, lootHit, groundHit,
                              picking && sample.ui.pointer
                                  ? picking->pickFixture(int(sample.ui.pointer->x) - viewX,
                                                         int(sample.ui.pointer->y) - viewY)
                                  : std::nullopt);
            }
            if (campaign && saveFlow.poll(*campaign, view.commands))
                return PlayOutcome::MainMenu;
            if (campaign && view.commands.snapshot().travelRequested) {
                const auto request = *view.commands.snapshot().travelRequested;
                view.commands.cancelTravel();
                try {
                    auto ticket = campaign->prepareTravel(request);
                    auto replacement = std::make_unique<PlayView>(
                        ticket.game(), campaign->location(ticket.destination()).map(), timings, renderer,
                        campaign->location(ticket.destination()).generated
                            ? &*campaign->location(ticket.destination()).generated
                            : nullptr);
                    // Compose destination before commit; refresh does not mutate Party.
                    replacement->refresh();
                    replacement->commands.enableMainMenu();
                    replacement->commands.saveMenu().enabled = launch && launch->saves;
                    if (campaign->commitTravel(ticket)) {
                        replacement->commands.travelMessage(
                            request.kind == TravelKind::Recall
                                ? "Recalled to the sanctuary hub. One camp kit consumed. Rest separately."
                                : "Travel complete. No camp kit consumed.");
                        current.swap(replacement);
                        replacement.reset(); // old view before old session in ticket
                        inputArmed = false;
                        first = true;
                        continue;
                    }
                    view.commands.travelMessage("Travel changed; reopen Travel and try again.");
                } catch (const std::exception& e) {
                    view.commands.travelMessage(e.what());
                }
            }
            if (view.commands.snapshot().retryRequested) {
                restart = true;
                break;
            }
            if (view.commands.snapshot().menuRequested)
                return launch && launch->arena ? PlayOutcome::ArenaSetup : PlayOutcome::MainMenu;
            if (view.commands.snapshot().quitRequested)
                return PlayOutcome::Quit;
            changed |= view.refresh(debug, sample.ui.focused);
            presenter.present(view.frames.frame(), view.session.movement(), view.map, debug, changed || first,
                              &view.commands.snapshot(), view.frames.picking(),
                              view.diagnostics ? &*view.diagnostics
                              : launch         ? launch->diagnostics
                                               : nullptr,
                              reveal);
            first = false;
        }
        if (!restart)
            break;
    }
    return PlayOutcome::Quit;
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
