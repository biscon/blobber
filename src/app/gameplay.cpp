#include "app/gameplay.hpp"
#include "app/audio_settings.hpp"
#include "app/campaign_saves.hpp"
#include "app/loading.hpp"
#include "app/result_flow.hpp"
#include "app/travel_fade.hpp"
#include "assets/dialogue.hpp"
#include "game/arena.hpp"
#include "game/campaign.hpp"
#include "platform/audio.hpp"
#include "platform/game_view.hpp"
#include "platform/input.hpp"
#include "render/dialogue_camera.hpp"
#include "ui/interaction.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>

namespace blobber {
std::optional<NpcWorld> gameplayPlacements(const Map& map, const NpcWorld& source, Cell spawn,
                                           const CampaignRun* campaign) {
    if (campaign)
        return std::nullopt;
    auto instances = source.instances();
    for (auto& npc : instances)
        npc.animation = NpcAnimation::Idle;
    std::vector<std::string> types;
    for (std::size_t i = 0; i < source.typeCount(); ++i)
        types.push_back(source.typeId({static_cast<std::uint16_t>(i)}));
    return NpcWorld(map, spawn, std::move(types), std::move(instances));
}
namespace {
struct PlayView {
    std::unique_ptr<GameSession> ownedSession;
    GameSession& session;
    WorldRenderer& renderer;
    world_render::DialogueCameraController dialogueCamera;
    NpcAnimator animator;
    GameView frames;
    CommandController commands;
    const Map& map;
    WorldViewTransform previousInputTransform;
    std::optional<generation::DiagnosticView> diagnostics;
    PlayView(const Map& m, const rules::Catalog& catalog, const NpcWorld& placements,
             const std::vector<NpcTimings>& timings, WorldRenderer& world, std::uint32_t seed, Player spawn,
             bool enableAi = true, const RecoveryLevel& recovery = {},
             const rules::PresetParty* starting = nullptr, const FloorFixtures& fixtures = {},
             const FloorObjectiveDefinition& objective = {}, const ArenaScenario* arena = nullptr)
        : ownedSession(std::make_unique<GameSession>(
              m,
              arena ? prepareArenaParty(catalog, *arena)
                    : std::make_unique<Party>(catalog, starting ? *starting : *catalog.preset()),
              placements, seed, spawn, enableAi, timings, recovery, fixtures, objective)),
          session(*ownedSession), renderer(world), animator(session.actors(), timings), frames(world),
          commands(session, timings), map(m) {}
    PlayView(GameSession& game, const Map& m, const std::vector<NpcTimings>& timings, WorldRenderer& world,
             const generation::GeneratedFloor* floor = nullptr)
        : session(game), renderer(world), animator(game.actors(), timings),
          frames(world, floor ? &floor->layout : nullptr), commands(game, timings), map(m) {
        if (floor)
            diagnostics = generation::diagnosticView(*floor);
    }
    bool refresh(bool debug = false, bool focused = true) {
        animator.setTime(session.timeMs(), &session.effects(), commands.dialogueSpeaker(),
                         commands.dialoguePlayer().clockMs(), &commands.dialogueAnimation());
        const auto& player = commands.dialoguePlayer();
        const auto* character = player.character();
        const auto primaryId =
            character ? commands.dialogueActor(character->primary) : std::optional<ActorId>{};
        const auto targetId = player.beat() ? commands.dialogueActor(player.beat()->target) : primaryId;
        const auto override =
            renderer.scene()
                ? dialogueCamera.update(*renderer.scene(), renderer.assets(), session.geometry(),
                                        world_render::camera(session.movement(), session.geometry()),
                                        primaryId ? session.actors().actor(*primaryId) : nullptr,
                                        targetId ? session.actors().actor(*targetId) : nullptr, player)
                : std::optional<world_render::Camera>{};
        const auto area = debug ? SpellArea{} : commands.floorPreview();
        const bool changed =
            frames.update(session, animator.snapshot(), area, override, dialogueCamera.moving());
        const auto* picking = frames.picking();
        commands.residentVisibility(picking ? picking->residents() : TargetSnapshot{});
        commands.refresh(picking ? picking->targets() : TargetSnapshot{}, session.actionBusy(), debug,
                         focused);
        const auto currentArea = debug ? SpellArea{} : commands.floorPreview();
        if (area.kind != currentArea.kind || area.cells != currentArea.cells ||
            area.eligible != currentArea.eligible || area.ground.revision != currentArea.ground.revision)
            return frames.update(session, animator.snapshot(), currentArea, override,
                                 dialogueCamera.moving()) ||
                   changed;
        return changed;
    }
};
} // namespace
PlayOutcome runGameplay(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                        const std::vector<NpcTimings>& timings, Presenter& presenter, const Options& options,
                        WorldRenderer& world, const RecoveryLevel& recovery,
                        const rules::PresetParty* starting, const GameplayLaunch* launch) {
    auto dialogueCatalog = prepareCpu([&] {
        return loadDialogue(options.dialogueDir.empty() ? options.assets / "dialogue" : options.dialogueDir,
                            options.assets / "audio", options.assets / "ui");
    });
    prepareDialogueLayout(
        dialogueCatalog,
        [](const char* text, void* context) {
            return static_cast<const UiFonts*>(context)->width(text, UiTextStyle::Label);
        },
        const_cast<UiFonts*>(&presenter.fonts()));
    DialogueHistory standaloneDialogue;
    struct AudioLeave {
        AudioRuntime* audio;
        Presenter& presenter;
        ~AudioLeave() {
            presenter.clearDialogue();
            if (audio) {
                audio->clearDialogue();
                audio->persistMusic();
                audio->detach();
                audio->worldGain = 1;
            }
        }
    } audioLeave{presenter.audio, presenter};
    presenter.prepareDialogue(dialogueCatalog.overlayFiles);
    if (presenter.audio)
        presenter.audio->prepareDialogue(dialogueCatalog.audioFiles);
    const Player spawn = launch ? launch->spawn : Player{};
    auto* campaign = launch ? launch->campaign : nullptr;
    const auto idle = gameplayPlacements(map, placements, spawn.cell, campaign);
    const std::uint32_t seed =
        launch ? launch->seed : chooseCampaignSeed(options.seed, {}, [] { return std::random_device{}(); });
    std::cout << "Gameplay seed: " << seed << '\n';
    bool inputArmed = false;
    Input input; // Keep edge latches across restart, so held confirmation never replays.
    for (;;) {
        auto loading = std::make_unique<LoadingScreen>(presenter.loadingWorker(), &presenter);
        auto current =
            campaign
                ? std::make_unique<PlayView>(campaign->game(), campaign->map(), timings, world,
                                             campaign->location(campaign->activeLocation()).generated
                                                 ? &*campaign->location(campaign->activeLocation()).generated
                                                 : nullptr)
                : std::make_unique<PlayView>(
                      map, catalog, *idle, timings, world, seed, spawn, launch ? launch->enableAi : true,
                      recovery, starting, launch && launch->fixtures ? *launch->fixtures : FloorFixtures{},
                      launch && launch->objective ? *launch->objective : FloorObjectiveDefinition{},
                      launch ? launch->arenaScenario : nullptr);
        auto& initial = *current;
        initial.commands.bindDialogue(&dialogueCatalog,
                                      campaign ? &campaign->dialogue() : &standaloneDialogue);
        CampaignSaveFlow saveFlow(launch ? launch->saves : nullptr);
        initial.commands.saveMenu().enabled = campaign && launch && launch->saves;
        initial.session.enableDevelopment(options.development);
        if (starting || campaign)
            initial.commands.enableMainMenu();
        bool debug = false, reveal = false, first = true, restart = false;
        if (launch && launch->arena)
            initial.commands.arenaContext(false);
        initial.refresh();
        initial.frames.activate();
        if (presenter.audio)
            presenter.audio->update(&initial.session, false, true, 0);
        if (loading->closing())
            return PlayOutcome::Quit;
        loading.reset();
        presenter.fade(0);
        for (;;) {
            auto& view = *current;
            if (WindowShouldClose() && (!campaign || saveFlow.closeRequested(*campaign, view.commands)))
                return PlayOutcome::Quit;
            const float dt = std::min(GetFrameTime(), 0.1f);
            auto sample = input.poll(dt);
            sample.ui.text = pollTextInput();
            if (!inputArmed) {
                inputArmed = !IsKeyDown(KEY_SPACE) && !IsKeyDown(KEY_T) && !IsKeyDown(KEY_ENTER) &&
                             !IsKeyDown(KEY_F) && !IsKeyDown(KEY_W) && !IsKeyDown(KEY_S) &&
                             !IsKeyDown(KEY_A) && !IsKeyDown(KEY_D) && !IsKeyDown(KEY_Q) &&
                             !IsKeyDown(KEY_E) && !IsMouseButtonDown(MOUSE_BUTTON_LEFT);
                sample.ui.pressed.fill(false);
                sample.action.reset();
            }
            if (debug && !view.commands.snapshot().modal() && sample.ui.has(UiKey::Cancel)) {
                debug = false;
                sample.ui.pressed[static_cast<std::size_t>(UiKey::Cancel)] = false;
            }
            if (sample.clearPending) {
                view.session.clearPending();
                view.commands.cancelPointerButtons();
            }
            if (sample.toggleMap && !view.commands.snapshot().modal())
                debug = !debug;
            if (debug && options.development && sample.ui.has(UiKey::Click) && sample.ui.pointer &&
                hudLayout::diagnosticMap.contains(*sample.ui.pointer)) {
                reveal = !reveal;
                sample.ui.pressed[std::size_t(UiKey::Click)] = false;
            }
            const bool dialogueWasOpen = view.commands.snapshot().dialogue.open;
            const bool finishWithMove =
                view.commands.dialoguePlayer().leaving() && sample.action && sample.fresh;
            if (finishWithMove)
                view.commands.finishDialogue();
            view.commands.tickDialogue(dt, sample.ui.focused);
            if (dialogueWasOpen && !view.commands.snapshot().dialogue.open && !finishWithMove) {
                sample.action.reset();
                inputArmed = false;
            }
            const bool campWasOpen = view.commands.snapshot().camp.open;
            view.commands.tickCamp(dt, sample.ui.focused);
            const bool paused =
                campWasOpen || !sample.ui.focused || debug || view.commands.snapshot().modal();
            const bool presentedCampaignTerminal = view.commands.snapshot().campaignTerminal;
            const auto presentedCombat = view.commands.snapshot().combat;
            const auto previous = view.session.movement().player;
            view.commands.record(campaign ? campaign->update(dt, paused) : view.session.update(dt, paused));
            if (!paused && continueLocalVictory(view.session, launch && launch->arena))
                view.commands.cancel();
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
            sample.ui = routeHudButtons(view.commands, sample.ui, samePresentedTurn);
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
                const auto displayed = presenter.displayedWorldTransform();
                const auto previousTransform = view.previousInputTransform;
                const bool imageMoved = displayed.x != previousTransform.x ||
                                        displayed.y != previousTransform.y ||
                                        displayed.zoom != previousTransform.zoom;
                view.previousInputTransform = displayed;
                const auto worldPointer =
                    sample.ui.pointer ? displayed.inverse(*sample.ui.pointer) : std::nullopt;
                const int px = worldPointer ? int(std::floor(worldPointer->x - viewX)) : -1;
                const int py = worldPointer ? int(std::floor(worldPointer->y - viewY)) : -1;
                const auto hit = picking && worldPointer ? picking->pick(px, py) : std::nullopt;
                const auto lootHit = picking && worldPointer ? picking->pickLoot(px, py) : std::nullopt;
                const auto groundHit =
                    worldPointer ? view.frames.groundCell(view.session, px, py) : std::nullopt;
                routeHudInput(view.commands, sample.ui, hit, samePresentedTurn && !result, lootHit, groundHit,
                              picking && worldPointer ? picking->pickFixture(px, py) : std::nullopt,
                              picking && worldPointer ? picking->pickContainer(px, py) : std::nullopt,
                              imageMoved,
                              picking && worldPointer ? picking->pickResident(px, py) : std::nullopt);
            }
            if (campaign) {
                std::optional<SavingScreen> saving;
                const auto intent = view.commands.saveMenu().intent;
                if (intent == SaveIntent::Save || intent == SaveIntent::SaveMenu) {
                    view.commands.cancelPointerButtons();
                    saving.emplace(presenter.loadingWorker(), presenter);
                }
                const bool leave = saveFlow.poll(*campaign, view.commands);
                const bool closing = saving && saving->closing();
                saving.reset();
                if (closing) {
                    if (saveFlow.closeRequested(*campaign, view.commands))
                        return PlayOutcome::Quit;
                } else if (leave)
                    return PlayOutcome::MainMenu;
            }
            if (campaign && view.commands.snapshot().travelRequested) {
                const auto request = *view.commands.snapshot().travelRequested;
                view.commands.cancelTravel();
                // No simulation or Party readers run while loading borrows their owners.
                TravelFade fade;
                while (fade.phase() == TravelFade::Phase::Out) {
                    if (WindowShouldClose())
                        return PlayOutcome::Quit;
                    fade.advance(GetFrameTime());
                    if (presenter.audio) {
                        presenter.audio->worldGain = 1 - fade.alpha();
                        presenter.audio->update(&current->session, false, IsWindowFocused(), GetFrameTime());
                    }
                    presenter.presentCached(fade.alpha());
                }
                presenter.presentCached(1); // A fully black frame precedes any destination work.
                fade.blackPresented();
                if (presenter.audio) {
                    presenter.audio->persistMusic();
                    presenter.audio->update(&current->session, true, IsWindowFocused(), 0);
                }
                bool closing = false;
                {
                    LoadingScreen travelLoading(presenter.loadingWorker(), &presenter, true);
                    try {
                        auto ticket = prepareCpu([&] { return campaign->prepareTravel(request); });
                        auto replacement = prepareCpu([&] {
                            return std::make_unique<PlayView>(
                                ticket.game(), campaign->location(ticket.destination()).map(), timings, world,
                                campaign->location(ticket.destination()).generated
                                    ? &*campaign->location(ticket.destination()).generated
                                    : nullptr);
                        });
                        replacement->refresh();
                        replacement->commands.enableMainMenu();
                        replacement->commands.bindDialogue(&dialogueCatalog, &campaign->dialogue());
                        replacement->commands.saveMenu().enabled = launch && launch->saves;
                        if (campaign->commitTravel(ticket)) {
                            replacement->frames.activate();
                            replacement->commands.travelMessage(
                                request.kind == TravelKind::Recall
                                    ? "Returned to Sanctuary. One camp kit used."
                                    : "Arrived.");
                            current.swap(replacement);
                            replacement.reset(); // Source view dies before the source session in ticket.
                        } else {
                            current->commands.travelMessage("Travel changed; reopen Travel and try again.");
                        }
                    } catch (const std::exception& e) {
                        std::fprintf(stderr, "Travel failed: %s\n", e.what());
                        current->commands.travelMessage(
                            "Couldn't travel. Your party is still here. Try again.");
                    }
                    closing = travelLoading.closing();
                }
                if (closing)
                    return PlayOutcome::Quit;
                auto& arrived = *current;
                if (presenter.audio) {
                    presenter.audio->update(&arrived.session, true, IsWindowFocused(), 0);
                }
                arrived.refresh(debug, sample.ui.focused);
                presenter.fade(1);
                arrived.frames.present(presenter, arrived.session, arrived.map, debug, true,
                                       &arrived.commands.snapshot(),
                                       arrived.diagnostics ? &*arrived.diagnostics : nullptr, reveal);
                fade.prepared();
                while (fade.phase() == TravelFade::Phase::In) {
                    if (WindowShouldClose())
                        return PlayOutcome::Quit;
                    fade.advance(GetFrameTime());
                    if (presenter.audio) {
                        presenter.audio->worldGain = 1 - fade.alpha();
                        presenter.audio->update(&current->session, false, IsWindowFocused(), GetFrameTime());
                    }
                    presenter.presentCached(fade.alpha());
                }
                presenter.fade(0);
                if (presenter.audio) {
                    presenter.audio->worldGain = 1;
                }
                inputArmed = false;
                first = true;
                continue;
            }

            if (view.commands.snapshot().retryRequested) {
                restart = true;
                break;
            }
            if (view.commands.snapshot().menuRequested)
                return launch && launch->arena ? PlayOutcome::ArenaSetup : PlayOutcome::MainMenu;
            if (view.commands.snapshot().quitRequested)
                return PlayOutcome::Quit;
            if (presenter.audio) {
                const auto& hud = view.commands.snapshot();
                // Dialogue pauses gameplay, but keeps its ambient bed continuous even
                // on entry/exit frames. Every other pause reason still takes precedence.
                std::optional<bool> ambiencePaused;
                if (dialogueWasOpen || hud.dialogue.open)
                    ambiencePaused = campWasOpen || debug || hud.modal(false);
                presenter.audio->update(&view.session, paused || view.commands.snapshot().modal(),
                                        sample.ui.focused, dt, view.commands.snapshot().menuOpen || debug,
                                        !view.commands.snapshot().modal() && !debug, ambiencePaused,
                                        campWasOpen || debug || hud.modal(false));
                const auto& dialogue = view.commands.dialoguePlayer();
                presenter.audio->presentDialogue(dialogue.beatSerial(),
                                                 dialogue.beat() ? dialogue.beat()->audio : 64,
                                                 dialogue.cuesReady());
            }
            if (view.commands.saveMenu().settingsRequested) {
                view.commands.saveMenu().settingsRequested = false;
                runAudioSettings(presenter, &view.session);
                inputArmed = false;
                first = true;
            }
            changed |= view.refresh(debug, sample.ui.focused);
            view.frames.present(presenter, view.session, view.map, debug, changed || first,
                                &view.commands.snapshot(),
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
} // namespace blobber
