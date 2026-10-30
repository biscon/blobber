#include "app/gameplay.hpp"
#include "app/guardian_captures.hpp"
#include "app/menu.hpp"
#include "app/options.hpp"
#include "assets/npc_loader.hpp"
#include "assets/preparation_profile.hpp"
#include "assets/prepared_assets.hpp"
#include "assets/prepared_loot.hpp"
#include "game/party.hpp"
#include "generation/diagnostic_view.hpp"
#include "generation/population.hpp"
#include "platform/presenter.hpp"
#include "render/frame_pipeline.hpp"
#include "render/preparation_limits.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>

namespace blobber {
static bool launchSession(const Options& options, const rules::Catalog& rules,
                          const rules::PresetParty& starting, Presenter& presenter,
                          const ArenaScenario* arena) {
    const auto seed = arena                     ? arena->seed
                      : options.seed            ? *options.seed
                      : options.captureMenuOnly ? 5u
                                                : std::random_device{}();
    std::optional<generation::GeneratedFloor> generated;
    std::optional<Map> arenaFloor;
    if (arena)
        arenaFloor.emplace(arenaMap());
    else {
        auto result = generation::generate({seed});
        if (!result)
            throw std::runtime_error(std::string("Floor generation failed: ") +
                                     generation::errorName(result.diagnostics.failure.code));
        generated = std::move(result.floor);
    }
    const Map& map = arena ? *arenaFloor : generated->map;
    const Player spawn = arena ? arenaSpawn() : generated->layout.spawn;
    const auto runtimeSeed = arena ? seed : generated->runtimeSeed;
    const auto diagnostics =
        generated ? generation::diagnosticView(*generated) : generation::DiagnosticView{};
    if (generated)
        std::cout << "Generated floor seed=" << seed << " fingerprint=" << generated->fingerprint
                  << " attempts=" << generated->diagnostics.attempts
                  << " fallback=" << generated->diagnostics.fallbackId << '\n';
    const auto loadStart = std::chrono::steady_clock::now();
    auto definitions = loadNpcDefinitions(options.assets / "npcs/level.json");
    auto population = generated ? std::optional<generation::PreparedPopulation>(
                                      generation::preparePopulation(*generated, rules, definitions.types))
                                : std::nullopt;
    LoadedNpcMetadata loadedNpcs{arena ? arenaPlacements(map, rules, *arena, definitions.types)
                                       : std::move(population->placements),
                                 std::move(definitions.timings), std::move(definitions.attachments),
                                 definitions.definitionFingerprint};
    const RecoveryLevel recovery = population ? population->recovery : RecoveryLevel{};

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
            GameSession session(map, rules, starting, loadedNpcs.world, arena->seed, arenaSpawn(), true,
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
        GameSession defeated(map, rules, starting, loadedNpcs.world, arena->seed, arenaSpawn(), true, timings,
                             recovery);
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
        for (int repeat = 0; repeat < 2; ++repeat) {
            GameSession session(map, rules, starting, loadedNpcs.world, runtimeSeed, spawn, true, timings,
                                recovery, generated->layout.fixtures, population->objective);
            NpcAnimator animator(session.actors(), timings);
            FramePipeline frames;
            session.enableDevelopment(options.development);
            CommandController commands(session, timings);
            frames.update(renderer, session.geometry(), session.movement(), &session.actors(),
                          &animator.snapshot());
            commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false,
                             true);
            presenter.present(frames.frame(), session.movement(), map, false, true, &commands.snapshot(),
                              frames.picking());
            presenter.capture(options.captureDirectory / (repeat ? "launch-restart.png" : "launch.png"));
            if (!repeat) {
                commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false,
                                 true, true);
                presenter.present(frames.frame(), session.movement(), map, true, false, &commands.snapshot(),
                                  frames.picking(), &diagnostics, false);
                presenter.capture(options.captureDirectory / "launch-discovered-map.png");
                presenter.present(frames.frame(), session.movement(), map, true, false, &commands.snapshot(),
                                  frames.picking(), &diagnostics, true);
                presenter.capture(options.captureDirectory / "launch-diagnostic-map.png");
                commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false,
                                 false, true);
                commands.enableMainMenu();
                commands.toggleMenu();
                presenter.present(frames.frame(), session.movement(), map, false, false, &commands.snapshot(),
                                  frames.picking());
                presenter.capture(options.captureDirectory / "pause-menu.png");
                commands.confirmLeave(true);
                presenter.present(frames.frame(), session.movement(), map, false, false, &commands.snapshot(),
                                  frames.picking());
                presenter.capture(options.captureDirectory / "discard-run.png");
            }
        }
        captureGuardians(*generated, *population, loadedNpcs.world, rules, starting, timings, renderer,
                         presenter, options.captureDirectory);
        return true;
    }
    const GameplayLaunch launch{spawn,
                                runtimeSeed,
                                bool(arena),
                                true,
                                generated ? &diagnostics : nullptr,
                                generated ? &generated->layout.fixtures : nullptr,
                                population ? &population->objective : nullptr};
    return runGameplay(map, rules, loadedNpcs.world, timings, renderer, presenter, options, recovery,
                       &starting, &launch) != PlayOutcome::Quit;
}
bool launchMenuParty(const Options& o, const rules::Catalog& c, const rules::PresetParty& p,
                     Presenter& presenter) {
    return launchSession(o, c, p, presenter, nullptr);
}
bool launchArena(const Options& o, const rules::Catalog& c, const ArenaScenario& arena,
                 Presenter& presenter) {
    const auto party = arenaParty(c, arena);
    return launchSession(o, c, party, presenter, &arena);
}
} // namespace blobber
