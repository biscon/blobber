#include "app/travel_fade.hpp"
#include "assets/preparation_file.hpp"
#include "core/preparation.hpp"
#include "render/world_doors.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/fixture_world.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace blobber;
namespace {
template <class F> void rejects(F f, const char* message) {
    bool rejected = false;
    try {
        f();
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected, message);
}
void doors() {
    auto catalog = fixtureCatalog();
    auto map = fixtureMap();
    NpcWorld empty(map, {3, 3}, {"goblin"}, {});
    auto game =
        std::make_unique<GameSession>(map, catalog, empty, 5, Player{{3, 3}, Heading::East}, false,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions(false));
    game->setAnimated(false);
    auto request = [&] { return fixtureRequest(*game, 0); };
    const auto stale = request();
    check(game->interact(request()).changed && game->fixtures().open(0), "Door opens");
    check(!game->interact(stale), "Old request cannot toggle a changed door");
    game->setAnimated(true);
    check(game->interact(request()).changed && game->opening() && game->geometry().solid({4, 3}),
          "Closing changes collision atomically and starts a motion barrier");
    std::array<world_render::DoorBox, world_render::doorBoxCapacity> boxes;
    world_render::doorBoxes(game->geometry(), &game->doors(), boxes, 0, 0, 0);
    const auto begin = boxes[3].x;
    game->update(.15, false);
    world_render::doorBoxes(game->geometry(), &game->doors(), boxes, 0, 0, 0);
    const auto middle = boxes[3].x;
    check(std::abs(begin.x) > .55f && std::abs(middle.x) > .39f && std::abs(middle.z) > .39f,
          "Closing animates from open through a halfway angle");
    check(!game->interact(request()), "Motion barrier rejects repeated interaction");
    game->update(.15, false);
    check(!game->opening() && game->fixtures().completed(0) && !game->fixtures().open(0),
          "Closed state retains first-opening completion");
    auto saved = std::make_unique<FloorState>();
    game->exportFloor(*saved);
    const auto digest = saved->digest();
    game->setAnimated(false);
    check(game->interact(request()).changed, "Door reopens");
    game->importFloor(*saved);
    check(!game->fixtures().open(0) && game->geometry().solid({4, 3}), "Closed state restores");
    game->exportFloor(*saved);
    check(saved->digest() == digest, "Door snapshot roundtrip is canonical");
    check(game->interact(request()).changed, "Restored closed door opens");
    game->move(Action::Forward);
    game->move(Action::Forward);
    game->move(Action::TurnRight);
    game->move(Action::TurnRight);
    check(game->movement().player.cell == Cell{5, 3} && game->interact(request()).changed,
          "Door closes from opposite threshold side");
    check(!game->fixtures().rememberedOpen(0), "Discovery remembers closing independently of completion");
    // Occupancy can change independently of the player's last simulation tick.
    check(game->interact(request()).changed, "Open before occupancy test");
    NpcWorld occupied(map, {3, 3}, {"goblin"}, {{{0}, {6, 3}, {0, 0}, Heading::East}});
    GameSession crowded(map, catalog, occupied, 5, {{3, 3}, Heading::East}, false, {}, {},
                        fixtureDefinitions(false));
    crowded.setAnimated(false);
    check(crowded.interact(fixtureRequest(crowded, 0)).changed, "Open crowded fixture");
    check(crowded.actors().move(crowded.actors().id(0), {5, 3}, {0, 0}, {3, 3}).changed,
          "Actor approaches threshold");
    check(crowded.actors().move(crowded.actors().id(0), {4, 3}, {0, 0}, {3, 3}).changed,
          "Actor occupies threshold");
    const auto revision = crowded.geometry().revision();
    check(!crowded.interact(fixtureRequest(crowded, 0)) && !crowded.geometry().solid({4, 3}) &&
              crowded.geometry().revision() == revision,
          "Occupied threshold cannot close");
}
void stairsAndFrames() {
    test::VisibilityFixture fixture;
    world_assets::CpuAssets assets(fixture.publication());
    auto catalog = fixtureCatalog();
    const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
    for (unsigned h = 0; h < 4; ++h)
        for (auto kind : {FixtureKind::Upstairs, FixtureKind::Downstairs}) {
            FloorFixtures definitions;
            definitions.count = 1;
            definitions.entries[0] = {{3, 3}, Heading(h), kind};
            const auto landing = stairLanding(definitions.entries[0]);
            NpcWorld empty(map, landing, {"goblin"}, {});
            GameSession game(map, catalog, empty, 5, {landing, Heading(h)}, false, {}, {}, definitions);
            game.setAnimated(false);
            check(game.geometry().solid({3, 3}) && game.nearbyFixture().id == game.fixtures().id(0),
                  "Stair entrance is blocked and exposes its faced landing interaction");
            game.move(Action::Forward);
            check(game.movement().player.cell == landing, "Cannot enter stair geometry");
            check(!fixtureReachable(definitions.entries[0], {3, 3}, Heading(h)),
                  "No current-cell stair interaction");
            auto scene = world_render::prepare(game.geometry());
            bool below = false, above = false, black = false;
            unsigned treads = 0;
            for (const auto& range : scene.ranges)
                if (range.id == PickingBuffer::fixtureBase) {
                    for (unsigned i = range.first; i < range.first + range.count; i += 6) {
                        const auto& v = scene.vertices[scene.indices[i]];
                        if (v.normal.y == 1 && range.material == 1)
                            ++treads;
                    }
                    for (unsigned i = range.first; i < range.first + range.count; ++i) {
                        const auto& v = scene.vertices[scene.indices[i]];
                        below |= v.position.y < -.24f;
                        above |= v.position.y > 1.1f;
                        black |= v.shade == 0;
                        check(v.shade >= 0 && v.shade <= 1, "Prepared stair shading is bounded");
                    }
                }
            check(treads == 8 && black && (kind == FixtureKind::Upstairs ? above : below),
                  "Eight steps extend in the requested vertical direction and end in darkness");
            world_render::VisibilityPipeline visibility(scene, assets);
            world_render::FrameInput input{game.geometry(), game.movement(), nullptr,
                                           nullptr,         nullptr,         &game.doors()};
            visibility.update(input);
            bool pickable = false;
            for (int y = 0; y < viewHeight; ++y)
                for (int x = 0; x < viewWidth; ++x)
                    pickable |= visibility.frame().picking.slot(x, y) == PickingBuffer::fixtureBase;
            check(pickable, "Stair triangles publish fixture picking IDs");
            watching = true;
            allocations = 0;
            for (unsigned i = 0; i < 4; ++i) {
                game.move(Action::TurnRight);
                visibility.update(input);
            }
            watching = false;
            check(allocations == 0, "Warmed stair views allocate no engine C++ storage");
            definitions.entries[0].kind = FixtureKind::Door;
            FloorGeometry door(map, definitions);
            std::array<world_render::DoorBox, world_render::doorBoxCapacity> boxes;
            check(world_render::doorBoxes(door.view(), nullptr, boxes, 0, 0, 0) == 4,
                  "Frame has three strips and one leaf");
            for (unsigned i = 0; i < 3; ++i)
                check(boxes[i].worldMapped, "All frame strips use world UV mapping");
            check(!boxes[3].worldMapped, "Leaf uses fitted art independently of frame density");
            const auto uv = world_render::doorUvSpan(boxes[3], 512, 512);
            check(std::abs(std::hypot(boxes[3].x.x, boxes[3].x.z) / uv[0] - boxes[3].y.y / uv[1]) < 1e-5f,
                  "Square door artwork retains physical aspect rather than stretching the ring");
            const auto mesh = world_render::proxyMesh();
            for (unsigned k = 0; k < 8; ++k)
                check(std::abs(mesh.vertices[k].u - mesh.vertices[k].position.x) < 1e-5f,
                      "Both broad faces place the latch at physical free edge x=1");
            const auto length = [](world_render::Vec v) { return std::hypot(v.x, v.z); };
            const float jamb = length(boxes[0].x), leaf = length(boxes[3].x);
            check(std::abs(leaf - .88f * .65f) < 1e-5f && std::abs(boxes[3].y.y - .92f * .875f) < 1e-5f,
                  "Door leaf has reviewed width/height in every heading");
            check(std::abs(2 * jamb + leaf - 1.f) < 1e-5f &&
                      std::abs(boxes[2].origin.y - boxes[3].origin.y - boxes[3].y.y) < 1e-5f &&
                      std::abs(boxes[2].origin.y + boxes[2].y.y - boxes[0].origin.y - boxes[0].y.y) < 1e-5f,
                  "Stone frame fills both sides and the entire space above the smaller leaf");
            world_render::doorBoxes(door.view(), nullptr, boxes, 0, 0, 0, 0, 7, 0);
            for (unsigned i = 0; i < 3; ++i)
                check(boxes[i].emblemTexture == UINT16_MAX, "Frames never receive guardian emblems");
            check(boxes[3].emblemTexture == 7, "Objective approach leaf receives reusable emblem");
            for (bool back : {false, true}) {
                const auto quad = world_render::doorEmblemQuad(boxes[3], back);
                for (const auto& p : quad) {
                    const auto& b = boxes[3];
                    const float plane = ((p.x - b.origin.x) * b.z.x + (p.z - b.origin.z) * b.z.z) /
                                        (b.z.x * b.z.x + b.z.z * b.z.z);
                    check(std::abs(plane - (back ? 1.02f : -.02f)) < .0001f,
                          "Emblem sits just outside the correct door face in every heading");
                    check(p.y > boxes[3].origin.y && p.y < boxes[3].origin.y + boxes[3].y.y,
                          "Emblem remains within the leaf height");
                }
            }
            definitions.entries[0].kind = FixtureKind::GuardianGate;
            FloorGeometry gate(map, definitions);
            world_render::doorBoxes(gate.view(), nullptr, boxes, 0, 0, 0, 0, 7);
            check(boxes[3].emblemTexture == 7, "Guardian gate also receives the same sigil");
        }
}
void packs() {
    test::WorldFixture fixture;
    world_assets::Writer writer(fixture.directory);
    std::vector<unsigned char> pixels(4 * 4 * 4, 255);
    world_assets::Pack pack;
    for (const char* name : {"wall", "floor", "ceiling"})
        pack.materials[pack.materialCount++] = std::uint16_t(writer.add(name, 4, 4, false, pixels));
    pack.door = pack.lockedDoor = pack.doorEdge =
        std::uint16_t(writer.add("door", 4, 4, false, pixels, 0, {}, false));
    pack.guardianEmblem = pack.crest = pack.vine = std::uint16_t(writer.add("decal", 4, 4, true, pixels));
    pack.sky = std::uint16_t(writer.add("sky", 4, 4, false, pixels, 0, {}, false));
    writer.finish({}, 0, {pack});
    world_assets::CpuAssets assets(world_assets::load(fixture.directory));
    check(assets.pack("dungeon").materialCount == 3 && !assets.publication.textures[pack.sky].material,
          "Sky and door images remain outside the terrain palette");
    check(assets.pack("dungeon").guardianEmblem == pack.guardianEmblem,
          "Optional guardian art binding survives publication");
    rejects([&] { assets.pack("missing"); }, "Missing map pack rejects explicitly");
    auto world = Map({"###", "#.#", "###"}).definition();
    const auto previous = world.fingerprint();
    world.assetPack = "future";
    check(previous != world.fingerprint(), "Pack selection participates in world identity");
    world.assetPack = "../escape";
    rejects([&] { Map invalid({"###", "#.#", "###"}, world); }, "Invalid map pack ID rejects");
    const auto metadata = fixture.directory / "metadata.json";
    nlohmann::json corrupt;
    {
        std::ifstream input(metadata);
        input >> corrupt;
    }
    const auto original = corrupt;
    corrupt["packs"][0]["worldScale"] = 2;
    {
        std::ofstream output(metadata);
        output << corrupt;
    }
    check(world_assets::load(fixture.directory).fingerprint != assets.publication.fingerprint,
          "Pack bindings and sampling participate in publication identity even with identical pixels");
    corrupt["packs"][0]["guardianEmblem"] = pack.door;
    {
        std::ofstream output(metadata);
        output << corrupt.dump();
    }
    rejects([&] { world_assets::load(fixture.directory); }, "Opaque guardian emblem binding rejects");
    corrupt = original;
    corrupt["packs"][0]["materials"][1] = pack.sky;
    {
        std::ofstream output(metadata);
        output << corrupt;
    }
    rejects([&] { world_assets::load(fixture.directory); }, "Non-terrain pack palette entry rejects");
}
void lootBags() {
    test::VisibilityFixture fixture;
    world_assets::CpuAssets assets(fixture.publication());
    const auto map = test::worldRoom(9, 9);
    auto scene = world_render::prepare(map);
    world_render::VisibilityPipeline visibility(scene, assets);
    MovementState movement;
    movement.player = {{4, 5}, Heading::North};
    LootSnapshot loot;
    loot.world = 99;
    loot.stacks[0] = {{4, 5}, {0}, 1, true, 7};
    loot.stacks[1] = {{4, 5}, {0}, 2, true, 8};
    world_render::FrameInput input{map, movement, nullptr, nullptr, &loot};
    for (int cellY : {5, 4, 2}) {
        loot.stacks[0].cell.y = loot.stacks[1].cell.y = cellY;
        ++loot.revision;
        visibility.update(input);
        const auto& frame = visibility.frame();
        check(frame.count == 1, "Several loot stacks in a cell share one bag and shadow");
        const auto& bag = frame.billboards[0];
        const float projected = bag.width * world_render::focal() / bag.depth;
        const float oldProjected = std::clamp(world_render::focal() * .25f / bag.depth, 4.f, 64.f);
        check(std::abs(projected - oldProjected * (2.f / 3.f)) < .001f && bag.height == bag.width,
              "Near-clamped and distant bags retain two-thirds of previous size");
        check(bag.shadowRadius > 0 && bag.shadowRadius < bag.width * .5f,
              "Small bag has a proportional soft floor shadow");
        check(visibility.lootVisible({99, 0, 7}), "Reduced bag stays pickable");
    }
    watching = true;
    allocations = 0;
    for (unsigned i = 0; i < 4; ++i) {
        ++loot.revision;
        visibility.update(input);
    }
    watching = false;
    check(!allocations, "Bag/shadow updates do not allocate frame storage");
}
void loading() {
    TravelFade fade;
    fade.prepared();
    check(fade.phase() == TravelFade::Phase::Out, "Cannot skip blackout before preparing");
    for (unsigned i = 0; i < 25; ++i)
        fade.advance(.01f);
    fade.advance(.01f);
    check(fade.phase() == TravelFade::Phase::Black && fade.alpha() == 1, "Fade reaches full black");
    fade.advance(10);
    check(fade.phase() == TravelFade::Phase::Black, "Elapsed time cannot start loading");
    fade.blackPresented();
    check(fade.phase() == TravelFade::Phase::Loading, "Presented black frame admits loading");
    fade.prepared();
    for (unsigned i = 0; i < 26; ++i)
        fade.advance(.01f);
    check(fade.phase() == TravelFade::Phase::Done && fade.alpha() == 0, "Prepared destination fades in");
    PreparationWorker worker;
    unsigned pumps = 0;
    const auto main = std::this_thread::get_id();
    PreparationScope scope(
        {[](void* context, const char*) { ++*static_cast<unsigned*>(context); }, &pumps, &worker});
    auto thread = prepareCpu([&] {
        preparationPoint("Owned CPU fixture");
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
        return std::this_thread::get_id();
    });
    check(thread != main && pumps > 0, "CPU work runs off the pumping thread");
    rejects([&] { prepareCpu([]() -> int { throw std::runtime_error("fixture failure"); }); },
            "Worker propagates failure");
    check(prepareCpu([] { return 17; }) == 17, "Worker remains reusable after failure");
    worker.cancel();
    rejects(
        [&] {
            prepareCpu([] {
                preparationPoint("Cancelled");
                return 0;
            });
        },
        "Cancellation joins work before propagating");
    worker.reset();
    check(prepareCpu([] { return 23; }) == 23, "Worker can begin a new loading boundary");
    test::WorldFixture files;
    const auto path = files.directory / "bounded-art.bin";
    {
        std::ofstream output(path, std::ios::binary);
        output << "asset";
    }
    rejects([&] { preparationFile(path, 4); }, "Oversized file rejects before decode or upload");
    const auto bytes = preparationFile(path, 5);
    check(std::string(bytes.begin(), bytes.end()) == "asset",
          "Exact-limit file transfers through the CPU worker");
    struct Cancel {
        PreparationWorker& worker;
        std::atomic<bool> started{false};
    } cancel{worker};
    PreparationScope cancelling({[](void* owner, const char*) {
                                     auto& c = *static_cast<Cancel*>(owner);
                                     if (c.started)
                                         c.worker.cancel();
                                 },
                                 &cancel, &worker});
    rejects(
        [&] {
            prepareCpu([&] {
                cancel.started = true;
                for (unsigned i = 0; i < 50; ++i) {
                    preparationPoint("Owned cancellable loading");
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            });
        },
        "Window-thread cancellation interrupts a running CPU preparation task");
}
} // namespace
int main() {
    try {
        doors();
        stairsAndFrames();
        packs();
        lootBags();
        loading();
        std::cout << "Dungeon refresh CPU contracts passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
