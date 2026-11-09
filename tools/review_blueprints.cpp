#include "app/gameplay.hpp"
#include "assets/blueprint_art.hpp"
#include "assets/blueprint_loader.hpp"
#include "assets/image.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "rules/creation.hpp"
#include "world/blueprint_report.hpp"
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace blobber;
namespace bp = blueprints;
namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
struct Arguments {
    std::string mode, room = "kitchen-hall", sockets, art = "blockout";
    std::filesystem::path catalog = "assets/blueprints/underkeep.json", assets, prepared, uiPrepared, output;
    unsigned turns = 0;
    bool combat = false;
};
Arguments arguments(int argc, char** argv) {
    require(argc >= 2, "Usage: blobber_review_blueprints validate|plans|walk|capture --catalog FILE --room "
                       "ID --assets ASSETS --prepared REVIEW_PREPARED --output NEW_OUTPUT [--rotation "
                       "0|90|180|270] [--sockets south,west] [--art blockout|finished] [--combat]");
    Arguments a;
    a.mode = argv[1];
    require(a.mode == "validate" || a.mode == "plans" || a.mode == "walk" || a.mode == "capture",
            "Unknown review mode");
    for (int i = 2; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--combat") {
            a.combat = true;
            continue;
        }
        require(i + 1 < argc, "Missing review option value");
        const std::string value = argv[++i];
        if (key == "--catalog")
            a.catalog = value;
        else if (key == "--room")
            a.room = value;
        else if (key == "--assets")
            a.assets = value;
        else if (key == "--prepared")
            a.prepared = value;
        else if (key == "--ui-prepared")
            a.uiPrepared = value;
        else if (key == "--output")
            a.output = value;
        else if (key == "--sockets")
            a.sockets = value;
        else if (key == "--art") {
            require(value == "blockout" || value == "finished", "Expected blockout or finished art");
            a.art = value;
        } else if (key == "--rotation") {
            unsigned degrees = 0;
            const auto r = std::from_chars(value.data(), value.data() + value.size(), degrees);
            require(r.ec == std::errc{} && r.ptr == value.data() + value.size() && degrees <= 270 &&
                        degrees % 90 == 0,
                    "Expected quarter-turn degrees");
            a.turns = degrees / 90;
        } else
            throw std::runtime_error("Unknown review option: " + key);
    }
    return a;
}
unsigned sockets(const bp::BlueprintCatalog& c, const Arguments& a) {
    if (a.sockets.empty())
        return 0;
    require(a.room != "kitchen-hall", "Pair socket selection is fixed");
    const auto& r = c.rooms[c.room(a.room).index];
    unsigned mask = 0;
    std::istringstream stream(a.sockets);
    std::string name;
    while (std::getline(stream, name, ',')) {
        unsigned bit = 0;
        for (unsigned i = 0; i < r.socketCount; ++i)
            if (name == r.sockets[i].id.data())
                bit = 1u << i;
        require(bit && !(mask & bit), "Unknown or repeated socket ID");
        mask |= bit;
    }
    require(mask && a.sockets.back() != ',', "Empty socket ID");
    return mask;
}
// Review owns a catalog copy: normal combat mechanics with no loot/XP awards.
rules::Catalog reviewRules(const rules::Catalog& c) {
    std::vector<rules::AttackDefinition> attacks;
    std::vector<rules::MonsterDefinition> monsters;
    std::vector<rules::RaceDefinition> races;
    std::vector<rules::ClassDefinition> classes;
    std::vector<rules::ItemDefinition> items;
    std::vector<rules::SpellDefinition> spells;
    std::vector<rules::CreationClass> creation;
    for (std::uint16_t i = 0; i < c.attackCount(); ++i)
        attacks.push_back(*c.attack({i}));
    for (std::uint16_t i = 0; i < c.monsterCount(); ++i) {
        auto m = *c.monster({i});
        m.experience = 0;
        m.lootCount = 0;
        monsters.push_back(std::move(m));
    }
    for (std::uint16_t i = 0; i < c.raceCount(); ++i)
        races.push_back(*c.race({i}));
    for (std::uint16_t i = 0; i < c.classCount(); ++i) {
        classes.push_back(*c.characterClass({i}));
        if (c.creation({i}))
            creation.push_back(*c.creation({i}));
    }
    for (std::uint16_t i = 0; i < c.itemCount(); ++i)
        items.push_back(*c.item({i}));
    for (std::uint16_t i = 0; i < c.spellCount(); ++i)
        spells.push_back(*c.spell({i}));
    require(c.preset(), "Review requires preset party");
    return {std::move(attacks), std::move(monsters), std::move(races),  std::move(classes),
            std::move(items),   *c.preset(),         std::move(spells), std::move(creation)};
}
NpcWorld actors(const bp::ReviewScene& scene, const NpcDefinitions& definitions, const rules::Catalog& rules,
                bool combat) {
    std::vector<NpcInstance> placements;
    if (combat) {
        unsigned i = 0;
        for (const auto* name : {"goblin", "goblin.archer"}) {
            const auto it = std::find(definitions.types.begin(), definitions.types.end(), name);
            const auto monster = rules.findMonster(name);
            require(it != definitions.types.end() && rules.monster(monster), "Review monster unavailable");
            NpcInstance npc{{std::uint16_t(it - definitions.types.begin())},
                            scene.combatCells[i++],
                            {1, 1},
                            scene.combatFacing};
            npc.monster = monster;
            placements.push_back(npc);
        }
    }
    return {scene.map, scene.spawn.cell, definitions.types, std::move(placements)};
}
unsigned parity(GpuResources& resources, WorldRenderer& renderer) {
    GpuLease<TargetHandle> target(resources, resources.createTarget(viewWidth, viewHeight));
    renderer.diagnosticIds(target.get());
    OwnedImage image(LoadImageFromTexture(resources.target(target.get()).texture));
    require(image.value.data, "Cannot read diagnostic IDs");
    ImageFlipVertical(&image.value);
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* pixels = static_cast<const unsigned char*>(image.value.data);
    const auto& picking = renderer.visibility()->frame().picking;
    unsigned interior = 0;
    for (int y = 1; y < viewHeight - 1; ++y)
        for (int x = 1; x < viewWidth - 1; ++x) {
            const auto p = (y * viewWidth + x) * 4;
            const auto actual = std::uint16_t(pixels[p] + 256 * pixels[p + 1]);
            if (actual == picking.slot(x, y))
                continue;
            bool edge = false;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    edge |= picking.slot(x + dx, y + dy) == actual;
            interior += !edge;
        }
    require(interior < 100, "Blockout CPU/GPU interior picking disagreement");
    return interior;
}
void capture(const Arguments& a, const bp::ReviewScene& scene, const rules::Catalog& rules,
             const NpcDefinitions& definitions, const NpcWorld& placements, GpuResources& resources,
             Presenter& presenter, WorldRenderer& renderer) {
    auto owner =
        std::make_unique<GameSession>(scene.map, rules, *rules.preset(), placements, 5, scene.spawn, a.combat,
                                      definitions.timings, RecoveryLevel{}, scene.fixtures);
    auto& game = *owner;
    NpcAnimator animator(game.actors(), definitions.timings);
    const auto before = resources.counts();
    std::ofstream report(a.output / "review.txt");
    report << "Blueprint art=" << a.art << "; room=" << a.room << "; seed=5; rotation=" << a.turns * 90
           << "; combat=" << a.combat << "; 1920x1080\n";
    report << "Publication fingerprint=" << renderer.assets().publication.fingerprint << '\n';
    {
        GameView view(renderer);
        CommandController commands(game, definitions.timings);
        bool rollbackChecked = false;
        auto shot = [&](const std::string& name) {
            game.discover();
            animator.setTime(game.timeMs(), &game.effects());
            view.update(game, animator.snapshot(), {});
            view.activate();
            commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, game.actionBusy(),
                             false, true);
            view.present(presenter, game, scene.map, false, true, &commands.snapshot());
            presenter.capture(a.output / (name + ".png"));
            report << name << " pose=" << game.movement().player.cell.x << ','
                   << game.movement().player.cell.y << ' ' << headingName(game.movement().player.heading)
                   << " interior_id_differences=" << parity(resources, renderer) << '\n';
            const auto memory = renderer.memory();
            report << "resident_textures=" << memory.textureCount
                   << " texture_bytes=" << memory.residentTextures << " alpha_bytes=" << memory.residentAlpha
                   << " mesh_bytes=" << memory.residentMeshes << " world_gpu=" << memory.worldGpu
                   << " application_gpu=" << memory.applicationGpu
                   << " peak_application_gpu=" << memory.peakApplicationGpu << " cpu=" << renderer.cpuBytes()
                   << '\n';
            if (const auto* prepared = renderer.scene())
                report << "scene_cpu=" << prepared->bytes() << " scene_mesh=" << prepared->meshBytes()
                       << '\n';
            using Error = unsigned (*)();
            const auto get = reinterpret_cast<Error>(rlGetProcAddress("glGetError"));
            require(get && get() == 0, "Blockout capture OpenGL error");
            if (!rollbackChecked) {
                const auto active = renderer.image();
                const auto held = resources.counts();
                bool rejected = false;
                {
                    GameView staged(renderer);
                    resources.failAfter(0);
                    try {
                        staged.update(game, animator.snapshot(), {});
                    } catch (const std::exception&) {
                        rejected = true;
                    }
                    resources.failAfter(-1);
                }
                require(rejected && !renderer.staged() && renderer.image().revision == active.revision &&
                            resources.counts().bytes == held.bytes,
                        "Failed blueprint preparation must preserve the active view and resource owners");
                rollbackChecked = true;
                report << "Injected GPU preparation rollback passed\n";
            }
        };
        // Checked doorway traversal, including the pair's shared doorway.
        for (unsigned i = 0; i < scene.fixtures.count; ++i) {
            const auto& f = scene.fixtures.entries[i];
            game.restorePose({f.cell + forward(f.facing), bp::rotate(f.facing, 2)}, true);
            game.discover();
            if (!a.combat) {
                require(bool(game.interact({game.fixtures().id(i), game.fixtures().revision()})),
                        "Review door did not open");
                game.update(.15, false);
                shot("door-" + std::to_string(i) + "-swing");
                game.update(.16, false);
                game.move(Action::Forward);
                game.update(.2, false);
                require(game.movement().player.cell == f.cell, "Review threshold traversal failed");
                game.move(Action::Forward);
                game.update(.2, false);
                require(game.movement().player.cell == f.cell + forward(f.facing) * -1,
                        "Review doorway exit failed");
            }
        }
        // Camera at the south interior edge looking toward the authored composition.
        const auto& rr = scene.rooms[scene.roomCount - 1];
        Cell pose =
            rr.origin +
            bp::rotate({a.room == "mead-hall" || a.room == "kitchen-hall" ? 3 : 2,
                        a.room == "mead-hall" || a.room == "kitchen-hall" || a.room == "barracks" ? 6 : 4},
                       bp::rotatedSize(rr.blueprint.size, a.turns), a.turns);
        game.restorePose({pose, bp::rotate(Heading::North, a.turns)}, true);
        shot("overview");
        if (!a.combat) {
            const auto reward = rr.blueprint.rewards[0];
            const auto from = rr.origin + reward.access, to = rr.origin + reward.cell;
            Heading face = Heading::North;
            for (unsigned h = 0; h < 4; ++h)
                if (from + forward(Heading(h)) == to)
                    face = Heading(h);
            game.restorePose({from, face}, true);
            require(bool(game.openContainer(game.nearbyContainer())), "Review chest access failed");
            shot("reward-access");
        }
        if (scene.roomCount == 2 && !a.combat) {
            // Shared door is index 0: its kitchen-outward facing is east before rotation.
            const auto& f = scene.fixtures.entries[0];
            game.restorePose({f.cell + forward(f.facing) * -1, f.facing}, true);
            shot("connected-pair");
        }
        if (a.art == "finished" && !a.combat) {
            // Room-local poses rotate with the accepted assembly, never OS input.
            for (unsigned i = 0; i < scene.roomCount; ++i) {
                const auto& room = scene.rooms[i];
                const bool kitchen = room.blueprint.purpose == bp::Purpose::Kitchen;
                const bool hall = room.blueprint.purpose == bp::Purpose::MeadHall;
                if (!kitchen && !hall)
                    continue;
                const auto place = [&](Cell cell, Heading facing) {
                    game.restorePose(
                        {room.origin +
                             bp::rotate(cell, bp::rotatedSize(room.blueprint.size, a.turns), a.turns),
                         bp::rotate(facing, a.turns)},
                        true);
                };
                const std::string name = kitchen ? "kitchen" : "hall";
                place(kitchen ? Cell{2, 4} : Cell{3, 4}, Heading::North);
                shot(name + "-composition");
                place({1, 2}, kitchen ? Heading::North : Heading::West);
                shot(name + "-close");
                game.move(Action::TurnRight);
                game.update(.08, false);
                shot(name + "-turn");
                game.update(.09, false);
                if (kitchen) {
                    place({0, 1}, Heading::North);
                    shot("hearth-detail");
                    place({3, 0}, Heading::East);
                    shot("kitchen-work-surface");
                    place({1, 3}, Heading::West);
                    shot("kitchen-provisions");
                    place({3, 1}, Heading::North);
                    shot("kitchen-barrel");
                    place({2, 3}, Heading::North);
                    shot("kitchen-shelves-north");
                    place({3, 2}, Heading::West);
                    shot("kitchen-shelves-west");
                    for (const auto target : {Cell{0, 1}, Cell{0, 2}, Cell{2, 0}}) {
                        const auto heading = target.x == 0 ? Heading::West : Heading::North;
                        place(target + forward(heading) * -1, heading);
                        game.move(Action::Forward);
                        game.update(.2, false);
                        const auto cell =
                            room.origin +
                            bp::rotate(target, bp::rotatedSize(room.blueprint.size, a.turns), a.turns);
                        require(game.movement().player.cell == cell && !scene.map.npcBlocked(cell),
                                "Wall shelves must have no party or NPC collision footprint");
                    }
                    place({1, 1}, Heading::North);
                    game.move(Action::Forward);
                    game.update(.2, false);
                    const auto storageCell =
                        room.origin +
                        bp::rotate({1, 0}, bp::rotatedSize(room.blueprint.size, a.turns), a.turns);
                    require(game.movement().player.cell == storageCell && !scene.map.npcBlocked(storageCell),
                            "Decorative storage must allow party traversal and NPC placement");
                    shot("storage-walkthrough");
                } else if (hall) {
                    // Walk a closed loop around all four sides of the central table.
                    const std::array<Cell, 11> loop{{{2, 0},
                                                     {3, 0},
                                                     {4, 0},
                                                     {4, 1},
                                                     {4, 2},
                                                     {4, 3},
                                                     {3, 3},
                                                     {2, 3},
                                                     {2, 2},
                                                     {2, 1},
                                                     {2, 0}}};
                    for (unsigned step = 1; step < loop.size(); ++step) {
                        Heading heading = Heading::North;
                        for (unsigned h = 0; h < 4; ++h)
                            if (loop[step - 1] + forward(Heading(h)) == loop[step])
                                heading = Heading(h);
                        place(loop[step - 1], heading);
                        game.move(Action::Forward);
                        game.update(.2, false);
                        const auto target =
                            room.origin +
                            bp::rotate(loop[step], bp::rotatedSize(room.blueprint.size, a.turns), a.turns);
                        require(game.movement().player.cell == target && !scene.map.npcBlocked(target),
                                "Central table loop, armor and overhead cells must remain walkable");
                    }
                    place({3, 5}, Heading::North);
                    shot("hall-furnished");
                    place({3, 3}, Heading::North);
                    shot("central-table");
                    place({2, 1}, Heading::North);
                    shot("armor-detail");
                    place({2, 0}, Heading::North);
                    shot("armor-near");
                    place({3, 4}, Heading::North);
                    shot("chandelier-detail");
                    place({0, 3}, Heading::North);
                    shot("table-end");
                    place({1, 2}, Heading::North);
                    shot("tapestry-detail");
                    place({1, 0}, Heading::West);
                    shot("brazier-detail");
                    game.update(.15, false);
                    shot("brazier-animated");
                }
            }
        }
    }
    require(!renderer.memory().textureCount && !renderer.assets().residency().meshes,
            "Review retained scene asset leases");
    const auto after = resources.counts();
    // Renderer-owned AO/noise/supersampling resources are created on first draw.
    // The view asset checks above cover scenery; main checks the full registry
    // against the Presenter baseline after the renderer itself is destroyed.
    report << "Renderer retained preparation bytes=" << after.bytes - before.bytes << '\n';
    report << "GPU view cleanup passed\n";
    require(bool(report), "Cannot write review report");
}
} // namespace
int main(int argc, char** argv) try {
    const auto a = arguments(argc, argv);
    const auto catalog = bp::load(a.catalog);
    if (a.mode == "validate") {
        unsigned count = 0;
        for (unsigned i = 0; i < catalog.roomCount; ++i)
            for (unsigned t = 0; t < 4; ++t)
                if (catalog.rooms[i].rotations & (1u << t))
                    for (unsigned s = 1; s < (1u << catalog.rooms[i].socketCount); ++s) {
                        (void)bp::assemble(catalog, catalog.rooms[i].id.data(), t, s);
                        ++count;
                    }
        for (unsigned t = 0; t < 4; ++t)
            (void)bp::assemble(catalog, "kitchen-hall", t);
        std::cout << count << " room variants and four pair orientations validated\n";
        if (a.art == "finished") {
            require(!a.prepared.empty(), "Finished validation requires --prepared");
            const auto scene = bp::assemble(catalog, a.room, a.turns, sockets(catalog, a), "underkeep");
            bp::validatePublishedArt(catalog, scene, world_assets::load(a.prepared / "world"));
            std::cout << "Finished bindings validated: " << a.room << '\n';
        }
        return 0;
    }
    const auto scene = bp::assemble(catalog, a.room, a.turns, sockets(catalog, a),
                                    a.art == "finished" ? "underkeep" : "blueprint-blockouts");
    require(!a.output.empty() && std::filesystem::create_directory(a.output),
            "Use a new review output directory");
    {
        std::ofstream svg(a.output / "plan.svg");
        bp::writePlan(svg, catalog, scene, a.room);
    }
    if (a.mode == "plans")
        return 0;
    require(!a.assets.empty() && !a.prepared.empty(), "walk/capture require --assets and --prepared");
    const auto rules = reviewRules(loadRules(a.assets / "rules/catalog.json"));
    const auto definitions = loadNpcDefinitions(a.assets / "npcs/level.json");
    const auto placements = actors(scene, definitions, rules, a.combat);
    auto publication = world_assets::load(a.prepared / "world");
    if (a.art == "finished")
        bp::validatePublishedArt(catalog, scene, publication);
    require(publication.definitions == definitions.definitionFingerprint, "Review asset definitions differ");
    const bool hidden = a.mode == "capture";
    SetTraceLogLevel(LOG_WARNING);
    Window window(1920, 1080, hidden, hidden);
    require(!hidden || IsWindowState(FLAG_WINDOW_HIDDEN), "Capture must remain hidden");
    GpuResources resources;
    {
        const auto uiPrepared = a.uiPrepared.empty()
                                    ? std::filesystem::path(GetApplicationDirectory()) / "assets/prepared"
                                    : a.uiPrepared;
        Presenter presenter(resources, a.assets, nullptr, uiPrepared);
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, std::move(publication));
            if (hidden)
                capture(a, scene, rules, definitions, placements, resources, presenter, renderer);
            else {
                Options options;
                options.assets = a.assets;
                options.preparedAssets = uiPrepared;
                options.noAudio = true;
                options.settings = a.output / "settings.json";
                GameplayLaunch launch;
                launch.spawn = scene.spawn;
                launch.enableAi = a.combat;
                launch.fixtures = &scene.fixtures;
                runGameplay(scene.map, rules, placements, definitions.timings, presenter, options, renderer,
                            {}, rules.preset(), &launch);
            }
        }
        require(resources.counts().bytes == baseline.bytes,
                "Renderer did not return GPU resources to presenter baseline");
    }
    // Presenter atlases/targets are registry-owned, not Presenter leases. The
    // registry destroys them before Window; fonts are separate external owners.
    require(!resources.counts().externalBytes, "Review retained font atlas ownership");
    std::cout << "Blueprint " << a.art << ' ' << a.mode << " complete: " << a.output << '\n';
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
