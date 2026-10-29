#include "app/ai_demo.hpp"
#include "app/gameplay.hpp"
#include "app/hybrid_demo.hpp"
#include "app/menu.hpp"
#include "app/options.hpp"
#include "app/recovery_demo.hpp"
#include "assets/npc_loader.hpp"
#include "assets/preparation_profile.hpp"
#include "assets/prepared_assets.hpp"
#include "assets/prepared_loot.hpp"
#include "assets/recovery_loader.hpp"
#include "game/party.hpp"
#include "platform/presenter.hpp"
#include "render/frame_pipeline.hpp"
#include "render/preparation_limits.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace blobber {
static bool launchSession(const Options& options, const rules::Catalog& rules,
                          const rules::PresetParty& starting, Presenter& presenter,
                          const ArenaScenario* arena) {
    const Map sourceMap = arena ? arenaMap() : Map::load((options.assets / "maze.txt").string());
    const bool aiScenario = options.aiDemo || options.captureAiOnly;
    const bool hybridScenario = options.hybridDemo || options.captureHybridOnly;
    const Map map = hybridScenario ? hybridDemoMap() : aiScenario ? aiDemoMap() : sourceMap;
    Controller controller(arena            ? arenaSpawn()
                          : hybridScenario ? hybridDemoSpawn()
                          : aiScenario     ? aiDemoSpawn()
                                           : Player{});
    if (map.solid(controller.state().player.cell) || !map.connected(controller.state().player.cell))
        throw std::runtime_error("Spawn is blocked or map contains unreachable floors");
    const auto loadStart = std::chrono::steady_clock::now();
    auto load = [&]() -> LoadedNpcMetadata {
        if (!arena)
            return loadNpcMetadata(options.assets / "npcs/level.json", sourceMap, Player{}.cell);
        auto definitions = loadNpcDefinitions(options.assets / "npcs/level.json");
        return {arenaPlacements(map, rules, *arena, definitions.types), std::move(definitions.timings),
                std::move(definitions.attachments), definitions.definitionFingerprint};
    };
    auto loadedNpcs = load();
    if (aiScenario)
        loadedNpcs.world = aiDemoPlacements(map, loadedNpcs.world);
    if (hybridScenario)
        loadedNpcs.world = hybridDemoPlacements(map, loadedNpcs.world, rules);
    if (options.recoveryDemo)
        loadedNpcs.world = recoveryDemoPlacements(map, loadedNpcs.world);
    const auto recovery = options.recoveryDemo
                              ? recoveryDemoLevel(rules)
                              : (arena || aiScenario || hybridScenario
                                     ? RecoveryLevel{}
                                     : loadRecoveryLevel(options.assets / "rules/recovery.json", map, rules));

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
            frames.update(renderer, map, session.movement(), &session.actors(), &animator.snapshot(),
                          &session.combat().attacks(), session.timeMs());
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
            GameSession session(map, rules, starting, loadedNpcs.world, 5, Player{}, true, timings, recovery);
            NpcAnimator animator(session.actors(), timings);
            FramePipeline frames;
            CommandController commands(session, timings);
            frames.update(renderer, map, session.movement(), &session.actors(), &animator.snapshot());
            commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false,
                             true);
            presenter.present(frames.frame(), session.movement(), map, false, true, &commands.snapshot(),
                              frames.picking());
            presenter.capture(options.captureDirectory / (repeat ? "launch-restart.png" : "launch.png"));
            if (!repeat) {
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
        return true;
    }
    const GameplayLaunch launch{controller.state().player, arena ? arena->seed : options.seed.value_or(5),
                                bool(arena), true};
    return runGameplay(map, rules, loadedNpcs.world, timings, renderer, presenter, options, recovery,
                       &starting, arena ? &launch : nullptr) != PlayOutcome::Quit;
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
