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
bool launchMenuParty(const Options& options, const rules::Catalog& rules, const rules::PresetParty& starting,
                     Presenter& presenter) {
    const Map sourceMap = Map::load((options.assets / "maze.txt").string());
    const bool aiScenario = options.aiDemo || options.captureAiOnly;
    const bool hybridScenario = options.hybridDemo || options.captureHybridOnly;
    const Map map = hybridScenario ? hybridDemoMap() : aiScenario ? aiDemoMap() : sourceMap;
    Controller controller(hybridScenario ? hybridDemoSpawn() : aiScenario ? aiDemoSpawn() : Player{});
    if (map.solid(controller.state().player.cell) || !map.connected(controller.state().player.cell))
        throw std::runtime_error("Spawn is blocked or map contains unreachable floors");
    const auto loadStart = std::chrono::steady_clock::now();
    auto loadedNpcs = loadNpcMetadata(options.assets / "npcs" / "level.json", sourceMap, Player{}.cell);
    if (aiScenario)
        loadedNpcs.world = aiDemoPlacements(map, loadedNpcs.world);
    if (hybridScenario)
        loadedNpcs.world = hybridDemoPlacements(map, loadedNpcs.world, rules);
    if (options.recoveryDemo)
        loadedNpcs.world = recoveryDemoPlacements(map, loadedNpcs.world);
    const auto recovery = options.recoveryDemo
                              ? recoveryDemoLevel(rules)
                              : (aiScenario || hybridScenario
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
    if (options.captureMenuOnly) {
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
    return runGameplay(map, rules, loadedNpcs.world, timings, renderer, presenter, options, recovery,
                       &starting);
}
} // namespace blobber
