#include "app/application.hpp"
#include "app/actor_captures.hpp"
#include "app/ai_demo.hpp"
#include "app/archer_captures.hpp"
#include "app/captures.hpp"
#include "app/gameplay.hpp"
#include "app/hud_captures.hpp"
#include "app/hybrid_demo.hpp"
#include "app/inventory_captures.hpp"
#include "app/options.hpp"
#include "app/projectile_captures.hpp"
#include "app/recovery_demo.hpp"
#include "app/spell_captures.hpp"
#include "app/ui_demo.hpp"
#include "assets/npc_loader.hpp"
#include "assets/preparation_profile.hpp"
#include "assets/prepared_assets.hpp"
#include "assets/prepared_loot.hpp"
#include "assets/recovery_loader.hpp"
#include "assets/rules_loader.hpp"
#include "game/party.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "render/frame_pipeline.hpp"
#include "render/preparation_limits.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
namespace blobber {
int run(int argc, char** argv) {
    const Options options = parseOptions(argc, argv);
    if (options.help) {
        std::cout << "blobber [--size WIDTH HEIGHT] [--assets DIRECTORY] [--capture DIRECTORY | "
                     "--capture-hud DIRECTORY | --capture-reactions DIRECTORY | --capture-ai DIRECTORY | "
                     "--capture-recovery DIRECTORY | --capture-inventory DIRECTORY | --capture-spells "
                     "DIRECTORY | --capture-hybrid DIRECTORY | "
                     "--capture-preparation "
                     "DIRECTORY | --capture-projectiles "
                     "DIRECTORY] "
                     "[--ui-demo | "
                     "--recovery-demo | --inventory-demo | --combat-demo | --ai-demo | --hybrid-demo | "
                     "--spell-demo] [--seed UINT32] "
                     "[--check-prepared]\n";
        return 0;
    }
    // Declaration order keeps immutable definitions alive for the starting party.
    const auto rules = loadRules(options.assets / "rules" / "catalog.json");
    const Party party(rules);
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
    ActorWorld npcs(map, rules, loadedNpcs.world);
    const auto& timings = loadedNpcs.timings;
    const auto profile = loadPreparationProfile(options.assets / "preparation.json");
    validatePreparedPublication(options.assets / "prepared", profile, loadedNpcs.definitionFingerprint);
    auto scene = loadPreparedScene(options.assets / "prepared/scene.bin", profile);
    const auto sceneBytes = sceneStorageBytes(scene);
    auto loot = loadPreparedLoot(options.assets / "prepared/loot.bin");
    loot.bind(rules);
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
    if (options.checkPreparedOnly)
        return 0;
    // Declaration order guarantees all GPU resources die before the window/context.
    const Window window(options.width, options.height, !options.captureDirectory.empty());
    Presenter presenter(options.assets);
    if (!options.captureDirectory.empty()) {
        NpcAnimator animator(npcs, timings);
        FramePipeline frames;
        CommandController commands(party, npcs);
        std::filesystem::create_directories(options.captureDirectory);
        if (options.captureRecoveryOnly) {
            captureRecovery(rules, loadedNpcs.world, timings, renderer, presenter, options.captureDirectory);
            return 0;
        }
        if (options.captureInventoryOnly) {
            captureInventory(map, rules, loadedNpcs.world, timings, renderer, presenter,
                             options.captureDirectory);
            return 0;
        }
        if (options.captureSpellsOnly) {
            captureSpells(rules, loadedNpcs.world, timings, renderer, presenter, options.captureDirectory);
            return 0;
        }
        if (options.captureProjectilesOnly) {
            captureProjectiles(rules, loadedNpcs.world, timings, renderer, presenter,
                               options.captureDirectory);
            return 0;
        }
        if (options.captureHybridOnly) {
            captureHybrid(map, rules, loadedNpcs.world, timings, renderer, presenter,
                          options.captureDirectory);
            captureArchers(rules, loadedNpcs.world, timings, renderer, presenter, options.captureDirectory);
            return 0;
        }
        if (options.captureAiOnly) {
            captureAi(map, rules, loadedNpcs.world, timings, renderer, presenter, options.captureDirectory);
            return 0;
        }
        if (options.captureReactionsOnly) {
            captureReactions(map, party, rules, loadedNpcs.world, timings, renderer, presenter,
                             options.captureDirectory);
            captureCombat(map, rules, loadedNpcs.world, timings, renderer, presenter,
                          options.captureDirectory);
            return 0;
        }
        if (options.captureHudOnly) {
            captureHud(map, rules, loadedNpcs.world, timings, renderer, presenter, options.captureDirectory);
            return 0;
        }
        const auto shots = makeCaptures();
        for (std::size_t i = 0; i < shots.size() && !WindowShouldClose(); ++i) {
            const auto& shot = shots[i];
            MovementState state;
            state.player = shot.to;
            state.transition = {shot.from, shot.to, shot.action, shot.t * Transition::duration};
            state.revision = i + 1;
            animator.setTime(shot.npcTimeMs);
            const bool changed = frames.update(renderer, map, state, &npcs, &animator.snapshot());
            commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{},
                             state.transition.active(), false, true);
            presenter.present(frames.frame(), state, map, false, changed, &commands.snapshot(),
                              frames.picking());
            presenter.capture(options.captureDirectory / (shot.name + ".png"));
        }
        if (options.capturePreparationOnly) {
            captureReactions(map, party, rules, loadedNpcs.world, timings, renderer, presenter,
                             options.captureDirectory);
            const auto hybridMap = hybridDemoMap();
            const auto hybrid = hybridDemoPlacements(hybridMap, loadedNpcs.world, rules);
            captureHybrid(hybridMap, rules, hybrid, timings, renderer, presenter, options.captureDirectory);
            captureArchers(rules, loadedNpcs.world, timings, renderer, presenter, options.captureDirectory);
            const auto aiMap = aiDemoMap();
            const auto ai = aiDemoPlacements(aiMap, loadedNpcs.world);
            captureAi(aiMap, rules, ai, timings, renderer, presenter, options.captureDirectory);
            return 0;
        }
        if (!WindowShouldClose())
            captureActors(map, party, rules, loadedNpcs.world, timings, renderer, presenter,
                          options.captureDirectory);
        if (!WindowShouldClose())
            captureReactions(map, party, rules, loadedNpcs.world, timings, renderer, presenter,
                             options.captureDirectory);
        if (!WindowShouldClose())
            captureUi(map, party, rules, loadedNpcs.world, timings, renderer, presenter,
                      options.captureDirectory);
        if (!WindowShouldClose())
            captureCombat(map, rules, loadedNpcs.world, timings, renderer, presenter,
                          options.captureDirectory);
        if (!WindowShouldClose())
            captureHud(map, rules, loadedNpcs.world, timings, renderer, presenter, options.captureDirectory);
        return 0;
    }
    if (options.uiDemo) {
        runUiDemo(map, party, rules, loadedNpcs.world, timings, renderer, presenter);
        return 0;
    }
    runGameplay(map, rules, loadedNpcs.world, timings, renderer, presenter, options, recovery);
    return 0;
}
} // namespace blobber
