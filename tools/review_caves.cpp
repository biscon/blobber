#include "app/gameplay.hpp"
#include "assets/blueprint_loader.hpp"
#include "game/campaign.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "cave_review.hpp"
#include "generation/cave_population.hpp"
#include "platform/audio.hpp"
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
void face(GameSession& game, Heading wanted, bool stopAtContact = false) {
    for (unsigned i = 0; i < 3 && game.movement().player.heading != wanted; ++i) {
        game.move(Action::TurnRight);
        game.update(.2, false);
        if (stopAtContact && !realTime(game.combat().snapshot().phase))
            return;
    }
    require(game.movement().player.heading == wanted, "Failed to turn");
}
// Route planning is review-only. Replay each edge through ordinary game commands,
// including discovering and opening closed doors from their reachable side.
} // namespace
void cave_review::walkTo(GameSession& game, Cell goal, std::optional<Cell> avoid, std::vector<Cell>* trace,
                         bool stopAtContact, const CellMask* forbidden) {
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
            if (!map.canStep(c, next) || (avoid && next == *avoid) || previous[index(next)] != -1 ||
                (forbidden && (*forbidden)[index(next)]))
                continue;
            const auto slot = game.fixtures().at(next);
            if (slot >= 0 && game.fixtures().definition().entries[unsigned(slot)].kind != FixtureKind::Door &&
                !game.fixtures().open(unsigned(slot)))
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
                face(game, Heading(h), stopAtContact);
                break;
            }
        if (stopAtContact && !realTime(game.combat().snapshot().phase))
            return;
        const auto slot = game.fixtures().at(*it);
        if (slot >= 0 && !game.fixtures().open(unsigned(slot))) {
            game.move(Action::Forward);
            require(game.movement().player.cell == from, "Closed door admitted movement");
            game.discover();
            require(bool(game.interact(game.nearbyFixture())), "Review door interaction failed");
            if (game.opening())
                game.update(DoorSnapshot::durationMs / 1000., false);
        }
        game.move(Action::Forward);
        game.update(.2, false);
        if (trace && game.movement().player.cell == *it)
            trace->push_back(*it);
        if (stopAtContact && (!realTime(game.combat().snapshot().phase) || game.actors().occupied(*it)))
            return;
        if (!(game.movement().player.cell == *it)) {
            std::cerr << "Blocked review step (" << from.x << "," << from.y << ") to ("
                      << it->x << "," << it->y << "), phase " << unsigned(game.combat().snapshot().phase)
                      << ", goal (" << goal.x << "," << goal.y << ")\n";
            throw std::runtime_error("Walkthrough failed to reach next cell");
        }
    }
}
namespace {
using cave_review::walkTo;
Player reviewRoute(GameSession& game, const CaveFloor& f) {
    const auto& s = f.sites;
    game.restorePose(f.layout.spawn, false);
    Player photo;
    if (f.layout.depth == 4) {
        walkTo(game, s[Site::Camp]);
        walkTo(game, s[Site::SpiderArch]);
        photo = {f.layout.rooms[1].anchor, Heading::West};
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
                       "ASSETS PREPARED] [--encounter|--populated] [--full-clear-entry] [--fallback]\n"
                       "       blobber_review_caves attrition 5 SEED ASSETS");
    bool forced = false, encounter = false, populated = false, fullEntry = false;
    while (argc > 4 && std::string_view(argv[argc - 1]).substr(0, 2) == "--") {
        const std::string_view option = argv[--argc];
        if (option == "--fallback")
            forced = true;
        else if (option == "--encounter")
            encounter = true;
        else if (option == "--populated")
            populated = true;
        else if (option == "--full-clear-entry")
            fullEntry = true;
        else
            throw std::invalid_argument("Unknown cave review option");
    }
    require(!(populated && encounter) && (!fullEntry || populated), "Invalid cave review mode combination");
    std::string mode = argv[1];
    const unsigned depth = number(argv[2]), seed = number(argv[3]);
    if (mode == "attrition") {
        require(argc == 5 && depth == 5 && !forced && !populated && !encounter,
                "Usage: blobber_review_caves attrition 5 SEED ASSETS");
        const std::filesystem::path root = argv[4];
        const auto catalog = loadRules(root / "rules/catalog.json");
        const auto defs = loadNpcDefinitions(root / "npcs/level.json");
        const auto rooms = blueprints::load(root / "blueprints/underkeep.json");
        for (bool originalNest : {true, false}) {
            std::cout << (originalNest ? "Three-spider comparison" : "Five-spider trial") << '\n';
            CampaignRun campaign(rooms, catalog, *catalog.preset(), defs.types, defs.timings, seed, 4);
            cave_review::checkNestApproach(campaign, originalNest);
        }
        return 0;
    }
    const bool vault = encounter && depth == 6;
    auto result = generateCave(seed, depth, forced,
                               populated ? CaveContent::Populated
                               : vault   ? CaveContent::VaultEncounter
                                         : CaveContent::Walkthrough);
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
    // Keep the combat stream independent of the cave layout stream.
    const auto combatSeed =
        encounter || populated ? std::uint32_t(seedFor({seed, depth}, Domain::Runtime)) : seed;
    RecoveryLevel recovery;
    std::vector<NpcInstance> occupants;
    const bool galleries = encounter && depth == 5;
    require(rules.preset() != nullptr, "Cave review requires a starting party");
    const auto& starting = *rules.preset();
    std::optional<ArenaScenario> vaultParty;
    FloorObjectiveDefinition objective;
    std::optional<Party::Saved> preparedParty;
    if (populated) {
        const auto population = prepareCavePopulation(f, seed, rules, defs.types);
        occupants = population.placements.instances();
        recovery = population.recovery;
        objective = population.objective;
        preparedParty = cave_review::populationParty(rules, depth, fullEntry);
    } else if (vault) {
        vaultParty = cave_review::prepareVault(f.sites, defs.types, rules, occupants);
    } else if (galleries) {
        cave_review::placeGalleries(f.sites, defs.types, occupants);
    } else if (encounter) {
        const auto type = std::find(defs.types.begin(), defs.types.end(), "venomous.spider");
        require(type != defs.types.end(), "Missing spider appearance");
        occupants.push_back({{std::uint16_t(type - defs.types.begin())}, f.sites[Site::SpiderArch],
                             {0, 1}, Heading::West});
        const auto item = rules.findItem("antitoxin");
        require(rules.item(item), "Missing camp antitoxin");
        recovery.ground[recovery.groundCount++] = {f.sites[Site::Camp], item, 1};
    }
    const NpcWorld actors(f.map, f.layout.spawn.cell, defs.types, occupants);
    auto walkGalleries = [&](bool meleeOnly, WorldRenderer* renderer = nullptr, Presenter* presenter = nullptr) {
        auto party = starting;
        if (meleeOnly)
            cave_review::equipMeleeProofParty(party);
        for (bool north : {true, false}) {
            GameSession game(f.map, rules, party, actors, combatSeed, f.layout.spawn, true, defs.timings,
                             recovery, f.layout.fixtures);
            cave_review::checkGalleries(game, f, defs.timings, north, meleeOnly, output, renderer, presenter);
        }
    };
    auto walkVault = [&](WorldRenderer* renderer = nullptr, Presenter* presenter = nullptr) {
        for (auto route : {cave_review::VaultRoute::Suppress, cave_review::VaultRoute::Burst,
                           cave_review::VaultRoute::OpeningVolley, cave_review::VaultRoute::DoorwayVolley,
                           cave_review::VaultRoute::Replacement}) {
            GameSession game(f.map, prepareArenaParty(rules, *vaultParty), actors, combatSeed, f.layout.spawn,
                             true, defs.timings, recovery, f.layout.fixtures, {});
            cave_review::checkVault(game, f, defs.timings, route, output, renderer, presenter);
        }
    };
    if (check && populated) {
        for (bool full : {false, true}) {
            GameSession game(f.map,
                             std::make_unique<Party>(rules, cave_review::populationParty(rules, depth, full)),
                             actors, combatSeed, f.layout.spawn, true, defs.timings, recovery,
                             f.layout.fixtures, objective);
            cave_review::checkPopulation(game, f, recovery, full);
        }
        {
            GameSession shortcut(f.map, std::make_unique<Party>(rules, *preparedParty), actors, combatSeed,
                                 f.layout.spawn, true, defs.timings, recovery, f.layout.fixtures, objective);
            cave_review::checkShortcut(shortcut, f);
        }
        if (depth == 5) {
            for (bool north : {true, false}) {
                GameSession game(f.map, std::make_unique<Party>(rules, *preparedParty), actors, combatSeed,
                                 f.layout.spawn, true, defs.timings, recovery, f.layout.fixtures, objective);
                cave_review::checkGalleries(game, f, defs.timings, north, false, output, nullptr, nullptr);
                GameSession bite(f.map,std::make_unique<Party>(rules,*preparedParty),actors,combatSeed,
                                 f.layout.spawn,true,defs.timings,recovery,f.layout.fixtures,objective);
                cave_review::checkGalleries(bite,f,defs.timings,north,false,output,nullptr,nullptr,true);
            }
        }
        if (depth == 6) {
            // Isolate the guardian with the actual populated party, geometry and gate.
            std::vector<NpcInstance> vaultOnly;
            placeBroodVault(f.sites, defs.types, rules, vaultOnly);
            NpcWorld boss(f.map, f.layout.spawn.cell, defs.types, std::move(vaultOnly));
            auto vaultObjective = objective;
            vaultObjective.awardCount = 0;
            const auto atVault = cave_review::guardianParty(rules,*preparedParty,depth);
            for (auto tactic : {cave_review::VaultRoute::Suppress, cave_review::VaultRoute::Burst}) {
                GameSession game(f.map, std::make_unique<Party>(rules, atVault), boss, combatSeed,
                                 f.layout.spawn, true, defs.timings, recovery, f.layout.fixtures,
                                 vaultObjective);
                cave_review::checkVault(game, f, defs.timings, tactic, output, nullptr, nullptr);
            }
        }
        return 0;
    }
    if (check && vault) {
        walkVault();
        return 0;
    }
    if (check && galleries) {
        walkGalleries(false);
        walkGalleries(true);
        return 0;
    }
    if (check) {
        GameSession game(f.map, rules, actors, combatSeed, f.layout.spawn, encounter, defs.timings, recovery,
                         f.layout.fixtures);
        if (encounter) {
            cave_review::checkSpider(game, f, defs.timings, output);
            return 0;
        }
        reviewRoute(game, f);
        std::cout << "Walked the required route, optional pocket and return landing.\n";
        return 0;
    }
    auto pub = world_assets::load(prepared / "world");
    require(pub.definitions == defs.definitionFingerprint, "Prepared world and NPC definitions differ");
    SetTraceLogLevel(LOG_WARNING);
    Window window(1920, 1080, hidden, hidden);
    require(!hidden || IsWindowState(FLAG_WINDOW_HIDDEN), "Capture window must be hidden");
    GpuResources resources;
    {
        AudioRuntime audio;
        if (!hidden)
            audio.prepare(root, false, output / "settings.json");
        Presenter presenter(resources, root, nullptr, prepared);
        presenter.audio = hidden ? nullptr : &audio;
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, std::move(pub));
            if (hidden && populated) {
                // One native inspection view per floor, from a walkable northern approach.
                const auto& room = f.layout.rooms[4];
                const Cell stance{room.anchor.x, room.origin.y};
                require(!f.map.movementBlocked(stance) && !actors.occupied(stance),
                        "Blocked guardian viewpoint");
                const auto atGuardian = cave_review::guardianParty(rules,*preparedParty,depth);
                GameSession game(f.map, std::make_unique<Party>(rules, atGuardian), actors, combatSeed,
                                 {stance, Heading::South}, true, defs.timings, recovery, f.layout.fixtures,
                                 objective);
                NpcAnimator animator(game.actors(), defs.timings, &game.combat());
                GameView view(renderer, &f.layout);
                CommandController commands(game, defs.timings);
                animator.setTime(game.timeMs(), &game.effects());
                view.update(game, animator.snapshot(), {});
                commands.refresh(view.picking()->targets(), false, false, true);
                view.present(presenter, game, f.map, false, true, &commands.snapshot());
                presenter.capture(output / "guardian.png");
                const auto glError = reinterpret_cast<unsigned (*)()>(rlGetProcAddress("glGetError"));
                require(glError && glError() == 0, "OpenGL error in populated guardian view");
            } else if (hidden && vault) {
                walkVault(&renderer, &presenter);
                for (const auto* frame : {"vault-nest.png", "stirring-sac.png", "mother-and-young.png"})
                    require(std::filesystem::is_regular_file(output / frame),
                            "Missing native vault review frame");
                using GetError = unsigned (*)();
                const auto glError = reinterpret_cast<GetError>(rlGetProcAddress("glGetError"));
                require(glError && glError() == 0, "OpenGL error in vault encounter");
                require(!renderer.memory().textureCount, "Vault review retained textures");
            } else if (hidden && galleries) {
                walkGalleries(false, &renderer, &presenter);
                require(!renderer.memory().textureCount, "Galleries review retained scene textures");
            } else if (hidden && encounter) {
                GameSession game(f.map, rules, actors, combatSeed, f.layout.spawn, true, defs.timings,
                                 recovery, f.layout.fixtures);
                cave_review::checkSpider(game, f, defs.timings, output, &renderer, &presenter);
                require(!renderer.memory().textureCount, "Spider review retained scene textures");
            } else if (hidden) {
                cave_review::checkPreparedDressing(renderer.assets(), f);
                if (depth == 5)
                    cave_review::biomeChange(renderer, f, root, seed);
                GameSession game(f.map, rules, actors, seed, f.layout.spawn, false, defs.timings, {},
                                 f.layout.fixtures);
                auto photo = reviewRoute(game, f);
                walkTo(game, photo.cell);
                face(game, photo.heading);
                NpcAnimator animator(game.actors(), defs.timings);
                {
                    GameView view(renderer, &f.layout);
                    CommandController commands(game, defs.timings);
                    auto capture = [&](const std::string& file, bool mapOpen = false) {
                        animator.setTime(game.timeMs(), &game.effects());
                        view.update(game, animator.snapshot(), {});
                        commands.refresh(view.picking()->targets(), false, false, true);
                        view.present(presenter, game, f.map, mapOpen, true, &commands.snapshot());
                        presenter.capture(output / file);
                        using GetError = unsigned (*)();
                        const auto glError = reinterpret_cast<GetError>(rlGetProcAddress("glGetError"));
                        require(glError && glError() == 0, "OpenGL error in cave capture");
                    };
                    capture("view.png");
                    auto photograph = [&](Cell cell, Heading heading, const char* file) {
                        walkTo(game, cell);
                        face(game, heading);
                        capture(file);
                    };
                    auto facingFeature = [&](Cell feature, unsigned room, const char* file) {
                        for (int distance : {2, 1})
                            for (unsigned h = 0; h < 4; ++h) {
                                const auto facing = Heading(h);
                                const auto step = forward(facing);
                                const auto stance = feature + step * -distance;
                                if (regionAt(f.layout, stance) != room || f.map.movementBlocked(stance) ||
                                    (distance == 2 && f.map.movementBlocked(stance + step)))
                                    continue;
                                photograph(stance, facing, file);
                                std::cout << file << " from (" << stance.x << "," << stance.y
                                          << ") heading=" << h << '\n';
                                return true;
                            }
                        return false;
                    };
                    // Use the first exploration junction even when its shoreline
                    // changes orientation with the room shape or seed.
                    const auto& scenery = f.map.definition().scenery;
                    const auto shore = std::find_if(scenery.begin(), scenery.end(), [&](const auto& patch) {
                        return patch.model == "pool-east" && regionAt(f.layout, patch.anchor) == 9;
                    });
                    require(shore != scenery.end() && facingFeature(shore->anchor, 9, "shore.png"),
                            "No dry viewpoint for the exploration pool");
                    if (depth == 4) {
                        photograph(f.layout.connections[0].socketA + Cell{-1, 0}, Heading::East,
                                   "threshold.png");
                        const auto camp = f.layout.rooms[1].anchor;
                        photograph(camp + Cell{-2, 1}, Heading::North, "bed-end.png");
                        photograph(camp + Cell{-1, 0}, Heading::West, "bed-side.png");
                        for (const auto& ledge : f.map.definition().scenery) {
                            if (ledge.model != "fungus-ledge")
                                continue;
                            const Cell stance = ledge.anchor + forward(ledge.facing) * -1;
                            if (!f.map.movementBlocked(stance) && game.fixtures().at(stance) < 0) {
                                photograph(stance, ledge.facing, "wall-fungi.png");
                                break;
                            }
                        }
                        for (const auto& store : f.map.definition().scenery) {
                            if (store.model != "crate-large")
                                continue;
                            for (int distance : {2, 1}) {
                                const Cell stance = store.anchor + forward(store.facing) * -distance;
                                if (regionAt(f.layout, stance) == 1 && !f.map.movementBlocked(stance)) {
                                    photograph(stance, store.facing, "wall-crate.png");
                                    break;
                                }
                            }
                        }
                        photograph(camp + Cell{-2, -1}, Heading::West, "moss-close.png");
                        photograph(camp + Cell{-1, -1}, Heading::East, "camp-roof.png");
                        const auto arch = f.sites[Site::ArchWeb];
                        photograph({arch.x, f.sites[Site::SpiderArch].y}, Heading::North, "arch-in.png");
                        photograph(arch + Cell{2, 0}, Heading::West, "arch-out.png");
                        photograph(f.sites[Site::SpiderArch], Heading::West, "near-ceiling.png");
                        photograph(f.sites[Site::SpiderArch] + Cell{-4, 0}, Heading::East, "arch-roof.png");
                        photograph(f.sites[Site::HighGallery] + Cell{0, -3}, Heading::South,
                                   "gallery-roof.png");
                    }
                    if (depth == 6) {
                        photograph(f.sites[Site::Mother], Heading::North, "vault-silk.png");
                        photograph(f.sites[Site::EastMelee], Heading::South, "clutch-east.png");
                        photograph(f.sites[Site::WestMelee] + Cell{0, -1}, Heading::South, "clutch-cord.png");
                    }
                    if (depth == 5) {
                        for (const auto& d : f.map.definition().decorations)
                            if ((d.web() || d.kind == DecorationKind::Vine) &&
                                regionAt(f.layout, d.cell) == 1 && facingFeature(d.cell, 1, "early-web.png"))
                                break;
                        walkTo(game, photo.cell);
                        face(game, photo.heading);
                        capture("discovery.png", true);
                        photograph(f.sites[Site::HighGallery] + Cell{1, 0}, Heading::South, "column.png");
                        walkTo(game, f.sites[Site::Lookout] + Cell{0, 2});
                        face(game, Heading::South);
                        // Sample the wall/floor at a fixed bright and dim phase.
                        const auto toBright = (60150 - game.timeMs() % 60000) % 60000;
                        game.update(double(toBright) / 1000., false);
                        capture("glow-high.png");
                        game.update(.95, false);
                        capture("glow-low.png");
                        // Compare equal-distance views without advancing the fire animation.
                        const auto bowl = f.sites[Site::Lookout] + Cell{0, 3};
                        for (auto heading : {Heading::West, Heading::East}) {
                            const auto side = bowl + forward(heading) * -1;
                            if (!f.map.movementBlocked(side)) {
                                game.restorePose({side, heading}, false);
                                capture("fire-side.png");
                                break;
                            }
                        }
                        const auto door = f.layout.connections[0].socketA;
                        walkTo(game, door + Cell{1, 0});
                        face(game, Heading::West);
                        require(bool(game.interact(game.nearbyFixture())), "Could not close survey door");
                        game.update(.5, false);
                        capture("door.png");
                        // The player's reference faces west, one step back from the rope.
                        walkTo(game, f.sites[Site::FarBank] + Cell{1, 0});
                        face(game, Heading::West);
                        const auto toDrip = (20960 - game.timeMs() % 16000) % 16000;
                        game.update(double(toDrip) / 1000., false);
                        capture("basin-west.png");
                        game.update(.38, false);
                        capture("basin-west-ring.png");
                        photograph(f.sites[Site::Fork] + Cell{0, -2}, Heading::East, "rope-bank.png");
                        walkTo(game, f.sites[Site::Fork] + Cell{3, -2});
                        face(game, Heading::South);
                        capture("north-bank.png");
                        walkTo(game, f.sites[Site::Fork] + Cell{2, 2});
                        face(game, Heading::North);
                        // Cross the repeat boundary in a continuous four-second sample.
                        constexpr unsigned period = world_assets::WaterCurrent::periodMs;
                        const auto untilStart = (period - 2000 + period - game.timeMs() % period) % period;
                        game.update(double(untilStart) / 1000, false);
                        // The first and last images are four seconds apart and straddle the wrap.
                        animator.setTime(game.timeMs(), &game.effects());
                        view.update(game, animator.snapshot(), {});
                        const auto reads = renderer.assets().residency().reads;
                        const auto uploads = resources.transfers().uploads;
                        const auto pickRevision = renderer.visibility()->frame().revision;
                        for (unsigned i = 0; i <= 4; ++i) {
                            capture("south-bank-" + std::to_string(i) + ".png");
                            game.update(1., false);
                        }
                        require(reads == renderer.assets().residency().reads &&
                                    uploads == resources.transfers().uploads,
                                "Warmed water reloaded or uploaded scene resources");
                        require(renderer.visibility()->frame().revision == pickRevision,
                                "Water animation changed picking at a fixed pose");
                    }
                }
                require(!renderer.memory().textureCount && !renderer.assets().residency().meshes,
                        "Cave scene retained assets");
            } else {
                Options options;
                options.assets = root;
                options.preparedAssets = prepared;
                options.settings = output / "settings.json";
                GameplayLaunch launch;
                launch.spawn = f.layout.spawn;
                launch.seed = combatSeed;
                launch.enableAi = encounter || populated;
                launch.objective = &objective;
                if (preparedParty)
                    launch.preparedParty = &*preparedParty;
                if (vaultParty)
                    launch.arenaScenario = &*vaultParty;
                launch.rooms = &f.layout;
                launch.fixtures = &f.layout.fixtures;
                runGameplay(f.map, rules, actors, defs.timings, presenter, options, renderer, recovery,
                            &starting, &launch);
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
