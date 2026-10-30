#include "app/gameplay.hpp"
#include "app/menu.hpp"
#include "app/options.hpp"
#include "app/save_captures.hpp"
#include "assets/npc_loader.hpp"
#include "assets/preparation_profile.hpp"
#include "assets/prepared_assets.hpp"
#include "assets/prepared_loot.hpp"
#include "game/campaign.hpp"
#include "game/party.hpp"
#include "generation/diagnostic_view.hpp"
#include "generation/population.hpp"
#include "persistence/campaign_store.hpp"
#include "platform/presenter.hpp"
#include "render/frame_pipeline.hpp"
#include "render/preparation_limits.hpp"
#include "ui/interaction.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>

namespace blobber {
static bool launchSession(const Options& options, const rules::Catalog& rules,
                          const rules::PresetParty* starting, Presenter& presenter,
                          const ArenaScenario* arena, CampaignStore* saves = nullptr,
                          std::unique_ptr<CampaignRun> restored = {}) {
    const auto seed = arena                     ? arena->seed
                      : options.seed            ? *options.seed
                      : options.captureMenuOnly ? 5u
                                                : std::random_device{}();
    const auto loadStart = std::chrono::steady_clock::now();
    auto definitions = loadNpcDefinitions(options.assets / "npcs/level.json");
    std::unique_ptr<CampaignRun> campaign = std::move(restored);
    std::optional<Map> arenaFloor;
    if (arena)
        arenaFloor.emplace(arenaMap());
    else if (!campaign)
        campaign =
            std::make_unique<CampaignRun>(rules, *starting, definitions.types, definitions.timings, seed);
    const Map& map = arena ? *arenaFloor : campaign->map();
    const Player spawn = arena ? arenaSpawn() : campaign->game().movement().player;
    const auto runtimeSeed = seed;
    LoadedNpcMetadata loadedNpcs{arena ? arenaPlacements(map, rules, *arena, definitions.types)
                                       : *campaign->location(campaign->activeLocation()).placements,
                                 std::move(definitions.timings), std::move(definitions.attachments),
                                 definitions.definitionFingerprint};
    const RecoveryLevel recovery =
        campaign ? campaign->location(campaign->activeLocation()).recovery : RecoveryLevel{};

    const auto& timings = loadedNpcs.timings;
    const auto profile = loadPreparationProfile(options.assets / "preparation.json");
    validatePreparedPublication(options.assets / "prepared", profile, loadedNpcs.definitionFingerprint);
    auto scene = loadPreparedScene(options.assets / "prepared/scene.bin", profile);
    const auto sceneBytes = sceneStorageBytes(scene);
    auto loot = loadPreparedLoot(options.assets / "prepared/loot.bin");
    const auto lootBytes = loot.storageBytes();
    requirePreparationBytes(sceneBytes, projectilePreparedBytes() + lootBytes,
                            PreparationLimits{}.residentBytes);
    const auto remaining =
        PreparationLimits{}.residentBytes - sceneBytes - projectilePreparedBytes() - lootBytes;
    auto preparedNpcs = loadPreparedNpcs(options.assets / "prepared/npcs.bin", timings,
                                         loadedNpcs.definitionFingerprint, profile, {remaining, remaining});
    const auto npcBytes = preparedNpcs.storageBytes();

    const SceneRenderer renderer(std::move(scene), std::move(preparedNpcs), loadedNpcs.attachments,
                                 std::move(loot));
    std::cout << "Prepared assets loaded: scene=" << sceneBytes << " npc=" << npcBytes
              << " projectiles=" << projectilePreparedBytes() << " loot=" << lootBytes
              << " resident=" << sceneBytes + npcBytes + projectilePreparedBytes() + lootBytes
              << " limit=" << PreparationLimits{}.residentBytes << " load_ms="
              << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                       loadStart)
                     .count()
              << '\n';
    if (options.captureSaveOnly && campaign && saves) {
        captureCampaignSave(*campaign, *saves, timings, renderer, presenter, options);
        return true;
    }
    if (options.captureArenaOnly && arena) {
        auto capture = [&](const char* name, GameSession& session, CommandController& commands) {
            NpcAnimator animator(session.actors(), timings);
            animator.setTime(session.timeMs(), &session.effects());
            FramePipeline frames;
            frames.update(renderer, session.geometry(), session.movement(), &session.actors(),
                          &animator.snapshot(), &session.combat().attacks(), session.timeMs());
            commands.arenaContext(arenaCleared(session.actors().snapshot()));
            commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false,
                             true);
            presenter.present(frames.frame(), session.movement(), map, false, true, &commands.snapshot(),
                              frames.picking());
            const auto file = options.captureDirectory / (std::string(name) + ".png");
            presenter.capture(file);
            presenter.capture(options.captureDirectory / (std::string(name) + "-1280.png"), 1280, 720);
            presenter.capture(options.captureDirectory / (std::string(name) + "-4x3.png"), 1024, 768);
        };
        for (unsigned repeat = 0; repeat < 2; ++repeat) {
            GameSession session(map, rules, *starting, loadedNpcs.world, arena->seed, arenaSpawn(), true,
                                timings, recovery);
            CommandController commands(session, timings);
            capture(repeat ? "arena-rematch" : "arena-spawn", session, commands);
            if (repeat)
                continue;
            commands.toggleMenu();
            capture("arena-pause", session, commands);
            commands.confirmRetry();
            capture("arena-retry-confirm", session, commands);
            commands.confirmLeave(false);
            commands.closeOverlay();
            session.setAnimated(false);
            for (unsigned step = 0; step < 10; ++step) {
                session.move(Action::Forward);
                session.update(.2, false);
            }
            for (unsigned step = 0; step < 100 && session.combat().snapshot().phase != CombatPhase::Turns;
                 ++step)
                session.update(.1, false);
            capture("arena-approach", session, commands);
            for (std::size_t i = 0; i < session.actors().snapshot().count; ++i)
                session.actors().damage(session.actors().id(i), 100000, session.timeMs());
            for (unsigned i = 0; i < 100 && session.combat().snapshot().phase != CombatPhase::Victory; ++i)
                session.update(.1, false);
            if (session.combat().snapshot().phase != CombatPhase::Victory)
                throw std::runtime_error("Arena capture did not reach victory");
            capture("arena-victory", session, commands);
        }
        GameSession defeated(map, rules, *starting, loadedNpcs.world, arena->seed, arenaSpawn(), true,
                             timings, recovery);
        CommandController commands(defeated, timings);
        defeated.setAnimated(false);
        for (unsigned step = 0; step < 10; ++step) {
            defeated.move(Action::Forward);
            defeated.update(.2, false);
        }
        for (unsigned step = 0; step < 100 && defeated.combat().snapshot().phase != CombatPhase::Turns;
             ++step)
            defeated.update(.1, false);
        for (unsigned i = 0; i < 4; ++i)
            defeated.party().damage({std::uint16_t(i)}, 100000);
        for (unsigned step = 0; step < 100 && defeated.combat().snapshot().phase != CombatPhase::Defeat;
             ++step)
            defeated.update(.1, false);
        if (defeated.combat().snapshot().phase != CombatPhase::Defeat)
            throw std::runtime_error("Arena capture did not reach defeat");
        capture("arena-defeat", defeated, commands);
        return true;
    }
    if (options.captureMenuOnly && !arena) {
        auto capture = [&](const char* name, bool camp = false, bool victoryReconstruction = false,
                           bool travelPanel = false) {
            auto& session = campaign->game();
            session.enableDevelopment(options.development);
            NpcAnimator animator(session.actors(), timings);
            animator.setTime(session.timeMs(), &session.effects());
            CommandController commands(session, timings);
            commands.enableMainMenu();
            FramePipeline frames;
            frames.update(renderer, session.geometry(), session.movement(), &session.actors(),
                          &animator.snapshot(), &session.combat().attacks(), session.timeMs(), nullptr,
                          &session.loot().snapshot());
            commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false,
                             true);
            if (camp) {
                commands.openCamp();
                commands.campKind(RestKind::Long);
            }
            if (travelPanel) {
                UiInput input;
                input.pressed[std::size_t(UiKey::Travel)] = true;
                routeHudInput(commands, input);
            }
            auto hud = commands.snapshot();
            // Explicit presentation reconstruction; CPU owned fixtures prove final-guardian victory.
            if (victoryReconstruction) {
                hud.campaignTerminal = hud.campaignVictory = true;
                hud.combat.phase = CombatPhase::Victory;
                std::snprintf(hud.fixturePrompt.data(), hud.fixturePrompt.size(), "%s",
                              "This expedition has ended. Return to Main Menu.");
            }
            presenter.present(frames.frame(), session.movement(), campaign->map(), false, true, &hud,
                              frames.picking());
            presenter.capture(options.captureDirectory / (std::string(name) + ".png"));
            presenter.capture(options.captureDirectory / (std::string(name) + "-1280.png"), 1280, 720);
        };
        capture("launch");
        capture("travel-fresh-selector", false, false, true);
        auto ticket = campaign->prepareTravel(campaign->request(campaign->game().nearbyFixture()));
        if (!campaign->commitTravel(ticket))
            throw std::runtime_error("Capture travel failed");
        capture("campaign-floor1");
        capture("travel-recall-confirmation", false, false, true);
        const auto kits = campaign->party().campSupplies().kits;
        campaign->party().damage({0}, 2);
        const auto hp = campaign->party().character({0})->hitPoints;
        auto paid = campaign->prepareTravel(campaign->recallRequest());
        if (!campaign->commitTravel(paid) || campaign->party().campSupplies().kits != kits - 1 ||
            campaign->party().character({0})->hitPoints != hp)
            throw std::runtime_error("Capture paid recall failed");
        capture("travel-recalled-hub");
        capture("travel-unlocked-selector", false, false, true);
        auto warp = campaign->prepareTravel(campaign->waypointRequest(1));
        if (!campaign->commitTravel(warp) || campaign->party().campSupplies().kits != kits - 1)
            throw std::runtime_error("Capture waypoint return failed");
        capture("travel-waypoint-entry");
        while (campaign->party().campSupplies().supply)
            campaign->party().consumeItem(campaign->party().campSupplies().supply);
        capture("travel-no-kits", false, false, true);
        // Return from at-feet entrance stairs with zero kits, without a balance expedition.
        auto back = campaign->prepareTravel(campaign->request(campaign->game().nearbyFixture()));
        if (!campaign->commitTravel(back))
            throw std::runtime_error("Capture ascent failed");
        capture("campaign-hub-return");
        campaign->party().damage({0}, 2);
        capture("campaign-sanctuary-rest", true);
        capture("campaign-victory-reconstruction", false, true);
        for (std::uint16_t i = 0; i < 4; ++i)
            campaign->party().damage({i}, 100000);
        capture("campaign-defeat");
        return true;
    }
    const GameplayLaunch launch{spawn,   runtimeSeed, bool(arena),    true, nullptr,
                                nullptr, nullptr,     campaign.get(), saves};
    return runGameplay(map, rules, loadedNpcs.world, timings, renderer, presenter, options, recovery,
                       starting, &launch) != PlayOutcome::Quit;
}
bool launchMenuParty(const Options& o, const rules::Catalog& c, const rules::PresetParty& p,
                     Presenter& presenter, CampaignStore* saves) {
    return launchSession(o, c, &p, presenter, nullptr, saves);
}
bool launchArena(const Options& o, const rules::Catalog& c, const ArenaScenario& arena,
                 Presenter& presenter) {
    const auto party = arenaParty(c, arena);
    return launchSession(o, c, &party, presenter, &arena);
}
bool launchSavedCampaign(const Options& o, const rules::Catalog& c, Presenter& presenter,
                         CampaignStore& saves) {
    auto run = saves.load();
    return launchSession(o, c, nullptr, presenter, nullptr, &saves, std::move(run));
}
} // namespace blobber
