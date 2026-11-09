#include "assets/blueprint_loader.hpp"
#include "assets/rules_loader.hpp"
#include "generation/blueprint_rooms.hpp"
#include "generation/population.hpp"
#include "generation/report.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include <fstream>
#include <iostream>
using namespace blobber;
namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
} // namespace
int main(int argc, char** argv) try {
    if (argc != 4)
        throw std::runtime_error("Usage: blobber_review_blueprint_campaign ASSETS PREPARED NEW_OUTPUT");
    const std::filesystem::path root = argv[1], prepared = argv[2], output = argv[3];
    require(std::filesystem::create_directory(output), "Choose a new review output directory");
    const auto rooms = blueprints::load(root / "blueprints/underkeep.json");
    const auto rules = loadRules(root / "rules/catalog.json");
    const auto npcs = loadNpcDefinitions(root / "npcs/level.json");
    blueprints::checkPrepared(root, prepared / "world");
    Window window(1920, 1080, true);
    GpuResources resources;
    {
        Presenter presenter(resources, root, nullptr, prepared);
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, world_assets::load(prepared / "world"));
            bool rollback = false;
            for (unsigned sample = 0; sample < 4; ++sample) {
                const bool fallback = sample >= 2;
                const unsigned depth = sample % 2 ? 3 : 1;
                auto generated = generation::generate({5, depth}, rooms, fallback);
                require(bool(generated), "Production generated floor rejected");
                auto& floor = *generated.floor;
                const auto prefix = std::string(fallback ? "fallback-" : "normal-") +
                                    generation::profileName(floor.layout.profile);
                std::ofstream(output / (prefix + ".json")) << generation::report(floor).dump(2) << '\n';
                auto population = generation::preparePopulation(floor, rules, npcs.types);
                GameSession game(floor.map, rules, population.placements, floor.runtimeSeed,
                                 floor.layout.spawn, false, npcs.timings, population.recovery,
                                 floor.layout.fixtures, population.objective);
                // This visual review isolates furniture; CPU campaign checks retain real encounters.
                for (unsigned i = 0; i < game.actors().snapshot().count; ++i)
                    game.actors().remove(game.actors().id(i));
                for (unsigned i = 0; i < floor.layout.fixtures.count; ++i) {
                    const auto& fixture = floor.layout.fixtures.entries[i];
                    if (fixture.kind != FixtureKind::Key)
                        continue;
                    game.restorePose({fixture.cell, Heading::North}, false);
                    game.discover();
                    require(bool(game.interact({game.fixtures().id(i), game.fixtures().revision()})),
                            "Review key collection failed");
                }
                NpcAnimator animator(game.actors(), npcs.timings);
                {
                    GameView view(renderer);
                    auto shot = [&](const std::string& name) {
                        animator.setTime(game.timeMs(), &game.effects());
                        view.update(game, animator.snapshot(), {});
                        CommandController commands(game, npcs.timings);
                        commands.refresh(view.picking()->targets(), false, false, true);
                        view.present(presenter, game, floor.map, false, true, &commands.snapshot());
                        presenter.capture(output / (prefix + "-" + name + ".png"));
                        using Error = unsigned (*)();
                        require(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                                "Generated room GL error");
                        const auto memory = renderer.memory();
                        require(memory.textureCount > 0, "Missing prepared room resources");
                        if (!rollback) {
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
                            require(rejected && !renderer.staged() &&
                                        renderer.image().revision == active.revision &&
                                        resources.counts().bytes == held.bytes,
                                    "Generated-room staging rollback changed active resources");
                            rollback = true;
                        }
                    };
                    for (unsigned n = 0; n < floor.layout.blueprintCount; ++n) {
                        const auto& instance = floor.layout.blueprints[n];
                        const auto& r = floor.layout.rooms[instance.room];
                        const auto& authored = rooms.rooms[rooms.room(instance.id.data()).index];
                        auto local = Cell{authored.size.x / 2, authored.size.y - 1};
                        auto pose =
                            Player{r.origin + blueprints::rotate(local, authored.size, instance.rotation),
                                   blueprints::rotate(Heading::North, instance.rotation)};
                        require(!floor.map.movementBlocked(pose.cell), "Review room pose is blocked");
                        game.restorePose(pose, false);
                        game.discover();
                        shot(instance.id.data());
                    }
                    const generation::BlueprintInstance *kitchen = nullptr, *hall = nullptr;
                    for (const auto& b : floor.layout.blueprints) {
                        if (b.purpose == blueprints::Purpose::Kitchen)
                            kitchen = &b;
                        if (b.purpose == blueprints::Purpose::MeadHall)
                            hall = &b;
                    }
                    const auto* pair = generation::connectionBetween(floor.layout, kitchen->room, hall->room);
                    require(pair, "Missing kitchen/hall connection");
                    const bool reversed = pair->a != kitchen->room;
                    const auto first = reversed ? pair->socketB : pair->socketA;
                    const auto heading = reversed ? pair->facingB : pair->facingA;
                    game.restorePose({first + forward(heading) * -1, heading}, false);
                    game.discover();
                    for (unsigned step = 0; step <= pair->count; ++step) {
                        const auto door = game.fixtures().at(game.movement().player.cell + forward(heading));
                        if (door >= 0 && !game.fixtures().open(unsigned(door))) {
                            require(bool(game.interact(
                                        {game.fixtures().id(unsigned(door)), game.fixtures().revision()})),
                                    "Pair doorway failed to open");
                            game.update(.31, false);
                        }
                        if (step + 1 == pair->count)
                            shot("pair-transition");
                        game.move(Action::Forward);
                        game.update(.2, false);
                        require(!game.movement().blocked, "Checked kitchen/hall traversal blocked");
                    }
                    require(generation::regionAt(floor.layout, game.movement().player.cell) == hall->room,
                            "Pair traversal did not reach hall");
                }
                require(!renderer.memory().textureCount && !renderer.assets().residency().meshes,
                        "Generated floor leases retained after view release");
            }
        }
        require(resources.counts().bytes == baseline.bytes, "Renderer did not return to presenter baseline");
    }
    std::cout << "Four generated/fallback floors, twenty views, checked pair traversal and GPU "
                 "rollback/lifetime passed\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
