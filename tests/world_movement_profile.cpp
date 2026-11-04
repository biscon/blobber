// Opt-in, bounded whole-frame profile; not part of ctest. No OS input automation.
#include "assets/rules_loader.hpp"
#include "platform/audio.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
using namespace blobber;
namespace {
struct SettingsDirectory {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("blobber-motion-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    SettingsDirectory() {
        if (!std::filesystem::create_directory(path))
            throw std::runtime_error("Cannot isolate profile settings");
    }
    ~SettingsDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc != 2)
            throw std::invalid_argument("Usage: blobber_world_movement_profile ASSETS");
        SetTraceLogLevel(LOG_WARNING);
        const std::filesystem::path assets = argv[1];
        const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
        const auto catalog = loadRules(assets / "rules/catalog.json");
        auto art =
            std::make_shared<const world_assets::CpuAssets>(world_assets::load(assets / "prepared/world"));
        WorldDefinition world;
        world.assetPack = "underkeep";
        const Map map({"#########", "#.......#", "#..#.#..#", "#..#.#..#", "#..#.#..#", "#.......#",
                       "#.......#", "#.......#", "#########"},
                      world);
        const auto found = std::find(definitions.types.begin(), definitions.types.end(), "goblin");
        if (found == definitions.types.end())
            throw std::runtime_error("Missing production goblin");
        const NpcTypeId type{std::uint16_t(found - definitions.types.begin())};
        const Player spawn{{4, 5}, Heading::North};
        NpcWorld places(map, spawn.cell, definitions.types,
                        {{type, {4, 3}, {0, 0}, Heading::South},
                         {type, {4, 3}, {1, 0}, Heading::South},
                         {type, {4, 3}, {0, 1}, Heading::South},
                         {type, {4, 3}, {1, 1}, Heading::South}});
        FloorFixtures fixtures;
        fixtures.count = 1;
        fixtures.entries[0] = {{4, 4}, Heading::North, FixtureKind::Door};
        GameSession game(map, catalog, places, 5, spawn, false, definitions.timings, {}, fixtures);
        game.setAnimated(false);
        if (!game.interact(game.nearbyFixture()))
            throw std::runtime_error("Profile door did not open");
        game.restorePose({{4, 6}, Heading::North}, false);
        game.setAnimated(true);
        Window window(1920, 1080, true, true);
        if (!IsWindowState(FLAG_WINDOW_HIDDEN) || IsWindowState(FLAG_VSYNC_HINT))
            throw std::runtime_error("Profile must remain hidden and without VSync");
        GpuResources resources;
        Presenter presenter(resources, assets);
        SettingsDirectory settings;
        AudioRuntime audio;
        // Do not read or change the user's audio settings/music history.
        audio.prepare(assets, false, settings.path / "audio.json");
        audio.settings.values.fill(0);
        audio.settings.musicEnabled = false;
        presenter.audio = &audio;
        WorldRenderer renderer(resources, art);
        renderer.measureFrames(true);
        GameView view(renderer);
        NpcAnimator animator(game.actors(), definitions.timings);
        CommandController commands(game, definitions.timings);
        auto refresh = [&] {
            animator.setTime(game.timeMs(), &game.effects());
            const auto area = commands.floorPreview();
            view.update(game, animator.snapshot(), area);
            const auto* picking = view.picking();
            commands.refresh(picking ? picking->targets() : TargetSnapshot{}, game.actionBusy(), false, true);
            const auto after = commands.floorPreview();
            if (area.kind != after.kind || area.cells != after.cells || area.eligible != after.eligible ||
                area.ground.revision != after.ground.revision)
                view.update(game, animator.snapshot(), after);
        };
        refresh();
        view.activate();
        const auto finish = reinterpret_cast<void (*)()>(rlGetProcAddress("glFinish"));
        if (!finish)
            throw std::runtime_error("Cannot measure GPU completion");
        using Clock = std::chrono::steady_clock;
        constexpr unsigned warmup = 32, samples = 256;
        std::cout << "PROFILE_CONFIG resolution=1920x1080 supersampling=" << renderer.supersampling()
                  << " vsync=0 cap=0 ai=0 warmup=" << warmup << '\n';
        for (unsigned mode = 0; mode < 3; ++mode) {
            const bool moving = mode == 2, hover = mode == 1;
            std::array<double, samples> times{}, updates{}, visuals{}, presentation{};
            unsigned moved = 0;
            for (unsigned i = 0; i < warmup + samples; ++i) {
                finish();
                const auto begin = Clock::now();
                if (moving && !game.movement().transition.active())
                    game.move(game.movement().player.cell.y == 6 ? Action::Forward : Action::Back);
                commands.record(game.update(1.0 / 120, false));
                const auto updated = Clock::now();
                refresh();
                audio.update(&game, false, true, 1.f / 120, false, true);
                refresh(); // Match the application's refresh before and after input routing.
                if (hover) {
                    const auto* picking = view.picking();
                    bool highlighted = false;
                    for (unsigned slot = 0; picking && slot < game.actors().snapshot().count; ++slot)
                        if (picking->targets().anchors[slot].valid) {
                            commands.hoverEnemyTarget(game.actors().id(slot));
                            highlighted = commands.snapshot().hover.kind == HoverKind::Enemy;
                            break;
                        }
                    if (!highlighted)
                        throw std::runtime_error("Hover profile must highlight a visible creature");
                } else
                    commands.hover(HoverKind::None);
                const auto rendered = Clock::now();
                view.present(presenter, game, map, false, true, &commands.snapshot());
                finish(); // Include completion, not just queued GPU submission.
                const auto done = Clock::now();
                if (i >= warmup) {
                    const auto n = i - warmup;
                    auto ms = [](auto a, auto b) {
                        return std::chrono::duration<double, std::milli>(b - a).count();
                    };
                    times[n] = ms(begin, done);
                    updates[n] = ms(begin, updated);
                    visuals[n] = ms(updated, rendered);
                    presentation[n] = ms(rendered, done);
                    moved += game.movement().transition.active();
                }
            }
            const double mean = std::accumulate(times.begin(), times.end(), 0.0) / samples;
            auto report = [](auto values) {
                std::sort(values.begin(), values.end());
                std::cout << values[samples / 2 - 1] << '/' << values[(samples * 95 + 99) / 100 - 1] << '/'
                          << values.back();
            };
            std::cout << "PROFILE "
                      << (moving  ? "movement"
                          : hover ? "hover"
                                  : "stationary-animation")
                      << " samples=" << samples << " moving=" << moved << " mean_fps=" << 1000 / mean
                      << " total_p50_p95_max_ms=";
            report(times);
            std::cout << " update=";
            report(updates);
            std::cout << " view_audio_hud=";
            report(visuals);
            std::cout << " present_gpu_complete=";
            report(presentation);
            std::cout << " audio=" << audio.ready() << '\n';
        }
        audio.detach();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
