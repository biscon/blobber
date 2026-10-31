#include "app/world_interaction_preview.hpp"
#include "app/starting_build_diagnostics.hpp"
#include "assets/image.hpp"
#include "assets/rules_loader.hpp"
#include "platform/input.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "ui/interaction.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
namespace blobber {
namespace {
Map previewMap() {
    std::vector<std::string> rows(11, std::string(11, '.'));
    for (unsigned i = 0; i < 11; ++i)
        rows[0][i] = rows[10][i] = rows[i][0] = rows[i][10] = '#';
    rows[4][3] = '#';
    return Map(rows);
}
const Player spawn{{5, 8}, Heading::North};
// Release externally owned views before their borrowed sessions, including error exits.
struct ResetView {
    WorldRenderer& renderer;
    bool enabled = true;
    ~ResetView() {
        if (enabled)
            renderer.clear();
    }
};
std::vector<NpcInstance> previewActors(unsigned scene, unsigned types) {
    if (scene)
        return {};
    return {{{0}, {5, 6}, {0, 0}, Heading::South},
            {{0}, {5, 6}, {1, 0}, Heading::South},
            {{0}, {4, 6}, {0, 0}, Heading::South},
            {{0}, {6, 6}, {1, 0}, Heading::South},
            {{std::uint16_t(types > 1 ? 1 : 0)}, {5, 4}, {0, 0}, Heading::South}};
}
FloorFixtures previewFixtures(unsigned scene) {
    FloorFixtures f;
    if (scene == 2) {
        f.count = 4;
        f.entries[0] = {{5, 8}, Heading::North, FixtureKind::Key, 2};
        f.entries[1] = {{5, 7}, Heading::South, FixtureKind::Door, 2};
        f.entries[2] = {{4, 7}, Heading::South, FixtureKind::Upstairs};
        f.entries[3] = {{6, 7}, Heading::South, FixtureKind::Endpoint};
    }
    return f;
}
RecoveryLevel previewLoot(unsigned scene, const rules::Catalog& catalog) {
    RecoveryLevel r;
    if (scene == 1) {
        r.groundCount = 2;
        r.ground[0] = {spawn.cell, catalog.findItem("dagger"), 1};
        r.ground[1] = {{5, 7}, catalog.findItem("dagger"), 1};
    }
    return r;
}
struct Preview {
    const Map& map;
    const NpcDefinitions& definitions;
    NpcWorld placements;
    GameSession game;
    NpcAnimator animator;
    CommandController commands;
    Preview(const Map& m, const rules::Catalog& catalog, const NpcDefinitions& defs, unsigned scene,
            std::uint32_t seed, const rules::PresetParty* party = nullptr)
        : map(m), definitions(defs),
          placements(m, spawn.cell, defs.types, previewActors(scene, unsigned(defs.types.size()))),
          game(m, catalog, party ? *party : *catalog.preset(), placements, seed, spawn, false, defs.timings,
               previewLoot(scene, catalog), previewFixtures(scene)),
          animator(game.actors(), defs.timings), commands(game, defs.timings) {}
    world_render::FrameInput input() {
        animator.setTime(game.timeMs(), &game.effects());
        return {game.geometry(), game.movement(), &game.actors(), &animator.snapshot(),
                &game.loot().snapshot()};
    }
    WorldImage refresh(WorldRenderer& renderer, bool focused = true) {
        const auto image = renderer.render(input());
        const auto* p = renderer.picking();
        commands.refresh(p ? p->targets() : TargetSnapshot{}, game.movement().transition.active(), false,
                         focused);
        return image;
    }
};
void present(Preview& view, WorldRenderer& renderer, Presenter& presenter) {
    const auto image = view.refresh(renderer);
    presenter.presentWorld(image.target, image.revision, view.game.movement(), view.map,
                           &view.commands.snapshot(), renderer.picking());
}
void saveLogical(GpuResources& resources, WorldRenderer& renderer, const std::filesystem::path& path) {
    OwnedImage image(LoadImageFromTexture(resources.target(renderer.image().target).texture));
    if (!image.value.data)
        throw std::runtime_error("World interaction readback failed");
    ImageFlipVertical(&image.value);
    if (!ExportImage(image.value, path.string().c_str()))
        throw std::runtime_error("World interaction image export failed");
}
} // namespace
int runWorldInteractionPreview(const Options& options) {
    const auto catalog = loadRules(options.assets / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(options.assets / "npcs/level.json");
    const auto map = previewMap();
    auto assets = std::make_shared<const world_assets::CpuAssets>(
        world_assets::load(options.assets / "prepared/world"));
    if (assets->publication.definitions != definitions.definitionFingerprint)
        throw std::runtime_error("World NPC definitions differ; rebuild world assets");
    Window window(options.width, options.height, false);
    GpuResources resources;
    Presenter presenter(resources, options.assets);
    auto view = std::make_unique<Preview>(map, catalog, definitions, 0, options.seed.value_or(5));
    WorldRenderer renderer(resources, assets);
    renderer.supersampling(options.supersampling);
    renderer.stage(view->input());
    renderer.commit();
    Input input;
    SetWindowTitle("R03 interaction preview | F1 formation, F2 loot, F3 fixtures | F5 native, F6 "
                   "supersampled | Esc exit");
    std::cout << "R03 isolated interaction preview. F1/F2/F3 reset scenes; F5/F6 select quality. "
                 "Normal controls and HUD apply. Projectile/spell images and animated doors remain R04.\n";
    while (!WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), .1f);
        auto sample = input.poll(dt);
        sample.ui.text = pollTextInput();
        if (IsKeyPressed(KEY_ESCAPE) && !view->commands.snapshot().modal())
            break;
        if (sample.ui.focused) {
            if (IsKeyPressed(KEY_F5))
                renderer.supersampling(false);
            if (IsKeyPressed(KEY_F6))
                renderer.supersampling(true);
            for (unsigned scene = 0; scene < 3; ++scene)
                if (IsKeyPressed(KEY_F1 + int(scene))) {
                    auto replacement =
                        std::make_unique<Preview>(map, catalog, definitions, scene, options.seed.value_or(5));
                    try {
                        renderer.stage(replacement->input());
                        renderer.commit();
                        view.swap(replacement);
                    } catch (const std::exception& e) {
                        std::cerr << e.what() << '\n';
                    }
                }
        }
        auto& v = *view;
        if (sample.clearPending)
            v.game.clearPending();
        const bool campWasOpen = v.commands.snapshot().camp.open;
        v.commands.tickCamp(dt, sample.ui.focused);
        const bool paused = campWasOpen || !sample.ui.focused || v.commands.snapshot().modal();
        const auto previous = v.game.movement().player;
        const auto before = v.commands.snapshot().combat;
        v.commands.record(v.game.update(dt, paused));
        if (!paused) {
            if (sample.toggleAnimation)
                v.game.setAnimated(!v.game.movement().animated);
            if (sample.action) {
                v.game.move(*sample.action, sample.fresh);
                v.commands.record(v.game.events());
            }
        }
        if (!(previous == v.game.movement().player))
            v.commands.movementStarted();
        v.refresh(renderer, sample.ui.focused);
        const auto* picking = renderer.picking();
        const auto pointer = sample.ui.pointer;
        const int x = pointer ? int(pointer->x) - viewX : -1, y = pointer ? int(pointer->y) - viewY : -1;
        const auto ground = renderer.visibility()->groundCell(v.input(), x, y);
        routeHudInput(v.commands, sample.ui, picking ? picking->pick(x, y) : std::nullopt,
                      confirmationMatches(before, v.game.combat().snapshot()),
                      picking ? picking->pickLoot(x, y) : std::nullopt, ground,
                      picking ? picking->pickFixture(x, y) : std::nullopt);
        present(v, renderer, presenter);
    }
    return 0;
}
void benchmarkWorldInteractions(const Options& options, GpuResources& resources, WorldRenderer& renderer,
                                const std::filesystem::path& output) {
    using Clock = std::chrono::steady_clock;
    using Json = nlohmann::json;
    const auto catalog = loadRules(options.assets / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(options.assets / "npcs/level.json");
    const auto map = previewMap();
    Preview view(map, catalog, definitions, 0, options.seed.value_or(5));
    ResetView reset{renderer};
    renderer.supersampling(options.supersampling);
    renderer.stage(view.input());
    renderer.commit();
    renderer.measureFrames(true);
    using Finish = void (*)();
    const auto finish = reinterpret_cast<Finish>(rlGetProcAddress("glFinish"));
    if (!finish)
        throw std::runtime_error("glFinish unavailable");
    std::array<MovementState, 96> samples{};
    unsigned count = 0;
    for (auto action :
         {Action::Forward, Action::Back, Action::Left, Action::Right, Action::TurnLeft, Action::TurnRight}) {
        Controller controller(spawn);
        controller.submit(map, action);
        for (unsigned i = 0; i < 16; ++i) {
            samples[count++] = controller.state();
            controller.update(map, .01f);
        }
    }
    Json report = {{"workload", "R03 isolated 11x11 room, five current-art actors"},
                   {"supersampling", options.supersampling},
                   {"visibility", world_render::visibilityVersion},
                   {"warmup", 120},
                   {"samples", 600},
                   {"trials", Json::array()}};
    std::array<double, 600> visibility{}, submission{}, elapsed{};
    auto animation = view.animator.snapshot();
    for (bool sync : {false, true})
        for (unsigned workload = 0; workload < 3; ++workload)
            for (unsigned trial = 0; trial < 3; ++trial) {
                unsigned draws = 0, hits = 0;
                GpuTransfers before{};
                for (unsigned i = 0; i < 720; ++i) {
                    if (i == 120)
                        before = resources.transfers();
                    if (workload) {
                        animation.revision++;
                        for (unsigned a = 0; a < animation.count; ++a)
                            animation.frames[a].active = std::uint8_t(
                                i %
                                definitions.timings[view.game.actors().snapshot().actors[a].npc.type.index][0]
                                    .count);
                    }
                    const auto& movement = workload == 2 ? samples[i % samples.size()] : view.game.movement();
                    const auto start = Clock::now();
                    renderer.render({view.game.geometry(), movement, &view.game.actors(), &animation,
                                     &view.game.loot().snapshot()});
                    if (sync)
                        finish();
                    if (i >= 120) {
                        visibility[i - 120] = renderer.stats.visibilityMs;
                        submission[i - 120] = renderer.stats.submissionMs;
                        elapsed[i - 120] =
                            std::chrono::duration<double, std::milli>(Clock::now() - start).count();
                        draws += renderer.stats.draws;
                        hits += renderer.stats.cached;
                    }
                }
                const auto percentiles = [](auto& v) {
                    std::sort(v.begin(), v.end());
                    return Json{{"p50_ms", v[299]}, {"p95_ms", v[569]}, {"p99_ms", v[593]}};
                };
                report["trials"].push_back(
                    {{"synchronized", sync},
                     {"workload", workload == 0   ? "cached_idle"
                                  : workload == 1 ? "animation"
                                                  : "motion"},
                     {"trial", trial},
                     {"visibility", percentiles(visibility)},
                     {"submission", percentiles(submission)},
                     {"elapsed", percentiles(elapsed)},
                     {"draws", draws},
                     {"cache_hits", hits},
                     {"mesh_texture_uploads", resources.transfers().uploads - before.uploads},
                     {"uploaded_bytes", resources.transfers().bytes - before.bytes}});
            }
    report["cpu_bytes"] = renderer.cpuBytes();
    report["gpu_estimated_bytes"] = renderer.gpuBytes();
    report["visibility_scratch_bytes"] = renderer.visibility()->bytes();
    renderer.stage(view.input());
    report["active_staged_cpu_bytes"] = renderer.cpuBytes();
    report["active_staged_gpu_estimated_bytes"] = renderer.gpuBytes();
    renderer.cancel();
    renderer.measureFrames(false);
    renderer.clear();
    std::ofstream file(output / "r03-benchmark.json");
    file << report.dump(2) << '\n';
    if (!file)
        throw std::runtime_error("Cannot write R03 benchmark report");
}
void captureWorldInteractions(const Options& options, GpuResources& resources, WorldRenderer& renderer,
                              Presenter& presenter, const std::filesystem::path& output) {
    const auto catalog = loadRules(options.assets / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(options.assets / "npcs/level.json");
    if (renderer.assets().publication.definitions != definitions.definitionFingerprint)
        throw std::runtime_error("World definitions differ");
    const auto map = previewMap();
    nlohmann::json report = {{"visibility", world_render::visibilityVersion},
                             {"images", nlohmann::json::array()}};
    for (unsigned scene = 0; scene < 3; ++scene) {
        Preview view(map, catalog, definitions, scene, options.seed.value_or(5));
        ResetView reset{renderer};
        renderer.stage(view.input());
        renderer.commit();
        const auto capture = [&](const std::string& name) {
            present(view, renderer, presenter);
            presenter.capture(output / (name + "-hud.png"));
            saveLogical(resources, renderer, output / (name + ".png"));
            report["images"].push_back(name);
            report["last_targets"] = renderer.picking() ? renderer.picking()->targets().count : 0;
        };
        for (bool quality : {false, true}) {
            renderer.supersampling(quality);
            capture("r03-scene-" + std::to_string(scene) + (quality ? "-ss" : "-native"));
        }
        if (scene == 0) {
            auto snap = view.animator.snapshot();
            unsigned actionIndex = 0;
            for (auto action : {Action::Forward, Action::Back, Action::Left, Action::Right, Action::TurnLeft,
                                Action::TurnRight}) {
                for (bool quality : {false, true}) {
                    renderer.supersampling(quality);
                    Controller controller(spawn);
                    controller.submit(map, action);
                    for (unsigned step = 0; step < 16; ++step) {
                        renderer.render({view.game.geometry(), controller.state(), &view.game.actors(), &snap,
                                         &view.game.loot().snapshot()});
                        const std::string name = "r03-motion-" + std::to_string(actionIndex) +
                                                 (quality ? "-ss-" : "-native-") + std::to_string(step);
                        saveLogical(resources, renderer, output / (name + ".png"));
                        report["images"].push_back(name);
                        controller.update(map, .01f);
                    }
                }
                ++actionIndex;
            }
            for (unsigned type = 0; type < renderer.assets().publication.appearances.size(); ++type) {
                const unsigned actorSlot = type ? 4 : 0;
                const auto& a = renderer.assets().publication.appearances[type];
                for (unsigned clip = 0; clip < npcAnimationCount; ++clip)
                    for (unsigned direction = 0; direction < 4; ++direction)
                        for (unsigned f = 0; f < a.clips[clip].timing.count; ++f) {
                            if (a.clips[clip].textures[direction][f] == UINT16_MAX)
                                continue;
                            // Observe every registered source texture through the shared billboard draw path.
                            const Heading facing = direction == 0   ? Heading::South
                                                   : direction == 1 ? Heading::North
                                                   : direction == 2 ? Heading::West
                                                                    : Heading::East;
                            view.game.actors().face(view.game.actors().id(actorSlot), facing);
                            snap.actorRevision = view.game.actors().snapshot().revision;
                            snap.revision++;
                            snap.frames[actorSlot].clip = NpcAnimation(clip);
                            snap.frames[actorSlot].active = std::uint8_t(f);
                            renderer.render({view.game.geometry(), view.game.movement(), &view.game.actors(),
                                             &snap, &view.game.loot().snapshot()});
                            const std::string name = "r03-art-" + std::to_string(type) + "-" +
                                                     std::to_string(clip) + "-" + std::to_string(direction) +
                                                     "-" + std::to_string(f);
                            saveLogical(resources, renderer, output / (name + ".png"));
                            report["images"].push_back(name);
                        }
            }
            view.game.move(Action::Forward);
            view.game.update(.16, false);
            for (bool quality : {false, true}) {
                renderer.supersampling(quality);
                capture(quality ? "r03-close-formation-ss" : "r03-close-formation-native");
            }
            {
                const auto party = spellCaptureParty(catalog, rules::SpellKind::Sleep);
                Preview status(map, catalog, definitions, 0, options.seed.value_or(5), &party);
                ResetView resetStatus{renderer};
                renderer.stage(status.input());
                renderer.commit();
                PartyCommand sleep;
                sleep.character = {3};
                sleep.action = PartyAction::Spell;
                sleep.spell = catalog.findSpell("sleep");
                sleep.aimedCell = Cell{5, 4};
                sleep.world = status.game.actors().snapshot().world;
                sleep.turn = status.game.combat().snapshot().turn;
                if (status.game.submit(sleep).reason != CommandReason::Resolved)
                    throw std::runtime_error("R03 status capture cast failed");
                status.game.update(.5, false);
                if (!status.game.effects().has(status.game.actors().id(4), EffectKind::Sleep))
                    throw std::runtime_error("R03 status capture missing sleeper");
                for (bool quality : {false, true}) {
                    renderer.supersampling(quality);
                    present(status, renderer, presenter);
                    status.commands.selectTarget(status.game.actors().id(0));
                    status.commands.hover(HoverKind::Enemy, 0, status.game.actors().id(4));
                    present(status, renderer, presenter);
                    const std::string name =
                        quality ? "r03-status-occlusion-ss" : "r03-status-occlusion-native";
                    presenter.capture(output / (name + "-hud.png"));
                    saveLogical(resources, renderer, output / (name + ".png"));
                    report["images"].push_back(name);
                }
                report["sleep_resolved"] = true;
                renderer.stage(view.input());
                renderer.commit();
                resetStatus.enabled = false;
            }
            view.game.actors().damage(view.game.actors().id(0), 100000, view.game.timeMs());
            view.game.update(.15, false);
            capture("r03-corpse");
        } else if (scene == 1) {
            const auto& stacks = view.game.loot().snapshot().stacks;
            unsigned slot = 0;
            while (slot < stacks.size() && (!stacks[slot].active || !(stacks[slot].cell == spawn.cell)))
                ++slot;
            if (slot == stacks.size())
                throw std::runtime_error("Preview ground loot missing");
            const auto id =
                LootId{view.game.loot().snapshot().world, std::uint16_t(slot), stacks[slot].generation};
            const auto result = view.game.pickup({id, {0}, view.game.party().inventory().revision()});
            if (!result)
                throw std::runtime_error(result.reason);
            report["pickup_accepted"] = true;
            capture("r03-after-pickup");
            // Drop through the same inventory authority used by the normal HUD.
            const auto& inv = view.game.party().inventory();
            ItemHandle item{};
            for (unsigned character = 0; character < 4 && !item; ++character)
                for (unsigned cell = 0; cell < bagCells; ++cell) {
                    const auto handle = inv.at({{std::uint16_t(character)}, std::uint16_t(cell)});
                    const auto* stack = inv.stack(handle);
                    if (stack && stack->item.index == catalog.findItem("dagger").index) {
                        item = handle;
                        break;
                    }
                }
            const auto drop = view.game.dropItem(item, inv.revision());
            if (!drop)
                throw std::runtime_error(drop.reason);
            report["drop_accepted"] = true;
            capture("r03-after-drop");
        } else {
            auto key = view.game.interact(view.game.nearbyFixture());
            if (!key)
                throw std::runtime_error(key.reason);
            capture("r03-key-collected");
            const auto door =
                view.game.interact({view.game.fixtures().id(1), view.game.fixtures().revision()});
            if (!door)
                throw std::runtime_error(door.reason);
            report["key_and_door_accepted"] = true;
            capture("r03-door-completed");
        }
        // The renderer borrows this view's world for coverage identity.
        renderer.clear();
    }
    std::ofstream file(output / "r03-report.json");
    file << report.dump(2) << '\n';
    if (!file)
        throw std::runtime_error("Cannot write R03 capture report");
}
} // namespace blobber
