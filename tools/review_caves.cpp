#include "app/gameplay.hpp"
#include "assets/image.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "cave_review.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
using namespace blobber;
using namespace generation;
namespace {
using Site = CaveSite;
void require(bool ok, const char* error) {
    if (!ok)
        throw std::runtime_error(error);
}
unsigned number(std::string_view value) {
    unsigned n = 0;
    const auto end = value.data() + value.size();
    const auto parse = std::from_chars(value.data(), end, n);
    require(parse.ec == std::errc{} && parse.ptr == end, "Expected unsigned integer");
    return n;
}
void face(GameSession& game, Heading wanted) {
    for (unsigned i = 0; i < 3 && game.movement().player.heading != wanted; ++i) {
        game.move(Action::TurnRight);
        game.update(.2, false);
    }
    require(game.movement().player.heading == wanted, "Failed to turn");
}
// Route planning is review-only. Replay each edge through ordinary game commands,
// including discovering and opening closed doors from their reachable side.
void walkTo(GameSession& game, Cell goal, std::optional<Cell> avoid = {},
            std::vector<Cell>* trace = nullptr) {
    const auto& map = game.geometry().base();
    auto index = [&](Cell c) { return c.y * map.width() + c.x; };
    const auto start = game.movement().player.cell;
    std::array<int, maxCells> previous, queue;
    previous.fill(-1);
    previous[index(start)] = index(start);
    unsigned head = 0, tail = 1;
    queue[0] = index(start);
    while (head < tail && previous[index(goal)] < 0) {
        const int n = queue[head++];
        Cell c{n % map.width(), n / map.width()};
        for (unsigned h = 0; h < 4; ++h) {
            const Cell next = c + forward(Heading(h));
            if (!map.canStep(c, next) || (avoid && next == *avoid) || previous[index(next)] != -1)
                continue;
            const auto slot = game.fixtures().at(next);
            if (slot >= 0 && game.fixtures().definition().entries[unsigned(slot)].kind != FixtureKind::Door)
                continue;
            previous[index(next)] = n;
            queue[tail++] = index(next);
        }
    }
    require(previous[index(goal)] >= 0, "No walkable review route");
    std::vector<Cell> steps;
    for (int n = index(goal); n != index(start); n = previous[n])
        steps.push_back({n % map.width(), n / map.width()});
    for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
        const auto from = game.movement().player.cell;
        for (unsigned h = 0; h < 4; ++h)
            if (from + forward(Heading(h)) == *it) {
                face(game, Heading(h));
                break;
            }
        const auto slot = game.fixtures().at(*it);
        if (slot >= 0 && !game.fixtures().open(unsigned(slot))) {
            game.move(Action::Forward);
            require(game.movement().player.cell == from, "Closed door admitted movement");
            game.discover();
            require(bool(game.interact(game.nearbyFixture())), "Review door interaction failed");
        }
        game.move(Action::Forward);
        game.update(.2, false);
        if (trace)
            trace->push_back(*it);
        require(game.movement().player.cell == *it, "Walkthrough failed to reach next cell");
    }
}
Player reviewRoute(GameSession& game, const CaveFloor& f) {
    const auto& s = f.sites;
    game.restorePose(f.layout.spawn, false);
    Player photo;
    if (f.layout.depth == 4) {
        walkTo(game, s[Site::Camp]);
        walkTo(game, s[Site::SpiderArch]);
        photo = {f.layout.rooms[1].anchor + Cell{-2, 0}, Heading::East};
    } else if (f.layout.depth == 5) {
        walkTo(game, s[Site::SoloNewt]);
        const Cell center = s[Site::Fork] + Cell{3, 0};
        walkTo(game, s[Site::Fork]);
        walkTo(game, s[Site::FarBank], center + Cell{0, 2});
        walkTo(game, s[Site::Fork], center + Cell{0, -2});
        photo = {s[Site::Fork] + Cell{-1, 0}, Heading::East};
    } else {
        walkTo(game, s[Site::WestMelee]);
        walkTo(game, s[Site::EastMelee]);
        photo = {s[Site::Mother] + Cell{0, -4}, Heading::South};
    }
    const auto& l = f.layout;
    const auto inlet = l.rooms[l.connections[2].b].anchor;
    walkTo(game, inlet);
    std::vector<Cell> outbound;
    walkTo(game, l.guardianApproach, {}, &outbound);
    auto crossing = std::find_if(outbound.begin(), outbound.end(), [&](Cell c) {
        for (unsigned i = 0; i < l.connectionCount; ++i) {
            const auto& e = l.connections[i];
            if (e.a >= 6 && e.b >= 6 && e.socketA == c)
                return true;
        }
        return false;
    });
    require(crossing != outbound.end(), "Outbound walk missed the exploration network");
    walkTo(game, inlet, *crossing);
    walkTo(game, s[Site::HighGallery]);
    walkTo(game, s[Site::Lookout]);
    walkTo(game, s[Site::Goal]);
    walkTo(game, s[Site::LowGallery]);
    walkTo(game, s[Site::Optional]);
    walkTo(game, f.layout.spawn.cell);
    return photo;
}
} // namespace
int main(int argc, char** argv) try {
    require(argc >= 4, "Usage: blobber_review_caves validate|plan|check|walk|capture DEPTH SEED [NEW_OUTPUT "
                       "ASSETS PREPARED] [--fallback]");
    const bool forced = std::string_view(argv[argc - 1]) == "--fallback";
    if (forced)
        --argc;
    std::string mode = argv[1];
    const unsigned depth = number(argv[2]), seed = number(argv[3]);
    auto result = generateCave(seed, depth, forced);
    if (!result.floor)
        throw std::runtime_error(std::string("Cave generation failed: ") +
                                 errorName(result.diagnostics.failure.code));
    const auto& f = *result.floor;
    std::cout << profileName(f.layout.profile) << ", seed " << seed << ", attempts "
              << result.diagnostics.attempts << ", fallback " << result.diagnostics.fallback << '\n';
    unsigned walkable = 0;
    for (int y = 0; y < f.map.height(); ++y)
        for (int x = 0; x < f.map.width(); ++x)
            walkable += !f.map.movementBlocked({x, y});
    std::cout << f.layout.width << " x " << f.layout.height << ", " << f.layout.roomCount << " rooms, "
              << f.layout.connectionCount << " passages, "
              << (f.layout.connectionCount + 1 - f.layout.roomCount) << " independent circuits, " << walkable
              << " walkable terrain cells\n";
    if (mode == "validate") {
        for (unsigned i = 0; i < result.diagnostics.attempts; ++i)
            if (const auto f = result.diagnostics.rejected[i])
                std::cout << "Candidate " << i + 1 << ": " << errorName(f.code) << " room " << f.room
                          << " link " << f.connection << '\n';
        require(argc == 4, "Unexpected options for validate");
        return 0;
    }
    const bool plan = mode == "plan", check = mode == "check", hidden = mode == "capture";
    require((plan && argc == 5) || ((hidden || check || mode == "walk") && argc == 7),
            "Invalid cave review arguments");
    const std::filesystem::path output = argv[4];
    require(std::filesystem::create_directory(output), "Choose a new output directory");
    std::ofstream svg(output / "plan.svg");
    cave_review::writePlan(svg, f, seed, result.diagnostics.fallback);
    svg.close();
    if (plan)
        return 0;
    const std::filesystem::path root = argv[5], prepared = argv[6];
    const auto rules = loadRules(root / "rules/catalog.json");
    const auto defs = loadNpcDefinitions(root / "npcs/level.json");
    const NpcWorld actors(f.map, f.layout.spawn.cell, defs.types, {});
    if (check) {
        GameSession game(f.map, rules, actors, seed, f.layout.spawn, false, defs.timings, {},
                         f.layout.fixtures);
        reviewRoute(game, f);
        std::cout << "Walked the required route, optional pocket and return landing.\n";
        return 0;
    }
    auto pub = world_assets::load(prepared / "world");
    require(pub.definitions == defs.definitionFingerprint, "Prepared world and NPC definitions differ");
    {
        OwnedImage master(LoadImage((root / "packs/sanctuary-city/source/water-master.png").u8string().c_str()));
        require(master.value.data, "Missing cave water painting");
        constexpr int side = cave_review::waterSourceSize;
        ImageResize(&master.value, side, side);
        ImageFormat(&master.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        require(master.value.data && master.value.width == side && master.value.height == side &&
                    master.value.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
                "Cannot prepare cave water");
        const auto* pixels = static_cast<const unsigned char*>(master.value.data);
        cave_review::bindBlockouts(pub, {pixels, pixels + side * side * 4});
    }
    SetTraceLogLevel(LOG_WARNING);
    Window window(1920, 1080, hidden, hidden);
    require(!hidden || IsWindowState(FLAG_WINDOW_HIDDEN), "Capture window must be hidden");
    GpuResources resources;
    {
        Presenter presenter(resources, root, nullptr, prepared);
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, std::move(pub));
            if (hidden) {
                GameSession game(f.map, rules, actors, seed, f.layout.spawn, false, defs.timings, {},
                                 f.layout.fixtures);
                auto photo = reviewRoute(game, f);
                walkTo(game, photo.cell);
                face(game, photo.heading);
                NpcAnimator animator(game.actors(), defs.timings);
                {
                    GameView view(renderer, &f.layout);
                    CommandController commands(game, defs.timings);
                    auto capture = [&](const std::string& file) {
                        animator.setTime(game.timeMs(), &game.effects());
                        view.update(game, animator.snapshot(), {});
                        commands.refresh(view.picking()->targets(), false, false, true);
                        view.present(presenter, game, f.map, false, true, &commands.snapshot());
                        presenter.capture(output / file);
                        using GetError = unsigned (*)();
                        const auto glError = reinterpret_cast<GetError>(rlGetProcAddress("glGetError"));
                        require(glError && glError() == 0, "OpenGL error in cave capture");
                    };
                    capture("view.png");
                    if (depth == 5) {
                        walkTo(game, f.sites[Site::Fork] + Cell{3, -2});
                        face(game, Heading::South);
                        capture("north-bank.png");
                        walkTo(game, f.sites[Site::Fork] + Cell{2, 2});
                        face(game, Heading::North);
                        // Cross the repeat boundary in a continuous four-second sample.
                        constexpr unsigned period = world_assets::WaterCurrent::periodMs;
                        const auto untilStart = (period - 2000 + period - game.timeMs() % period) % period;
                        game.update(double(untilStart) / 1000, false);
                        for (unsigned i = 0; i <= 32; ++i) {
                            capture("south-bank-" + std::to_string(i) + ".png");
                            game.update(.125, false);
                        }
                    }
                }
                require(!renderer.memory().textureCount && !renderer.assets().residency().meshes,
                        "Cave scene retained assets");
            } else {
                Options options;
                options.assets = root;
                options.preparedAssets = prepared;
                options.noAudio = true;
                options.settings = output / "settings.json";
                GameplayLaunch launch;
                launch.spawn = f.layout.spawn;
                launch.seed = seed;
                launch.enableAi = false;
                launch.fixtures = &f.layout.fixtures;
                runGameplay(f.map, rules, actors, defs.timings, presenter, options, renderer, {},
                            rules.preset(), &launch);
            }
        }
        require(resources.counts().bytes == baseline.bytes, "Renderer retained GPU resources");
    }
    require(!resources.counts().externalBytes, "Presenter retained font resources");
    std::cout << "Cave review complete\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
