#include "support/allocations.hpp"
#include "support/blueprint_fixtures.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/fixture_world.hpp"
#include "world/blueprint_report.hpp"
#include <iostream>
#include <sstream>
using namespace blobber;
namespace bp = blueprints;
namespace {
void malformed() {
    const auto original = test::blueprintJson();
    auto bad = [&](auto edit) {
        auto j = original;
        edit(j);
        rejects([&] { bp::decode(j.dump()); }, "Malformed blueprint must reject");
    };
    bad([](auto& j) { j["schema_version"] = 2; });
    bad([](auto& j) { j["unexpected"] = true; });
    bad([](auto& j) { j["rooms"][0]["size"][0] = 10; });
    bad([](auto& j) { j["rooms"][0]["size"][0] = 5.5; });
    bad([](auto& j) { j["kits"][0]["height"] = -1; });
    bad([](auto& j) { j["kits"][0]["blocking"] = 0; });
    for (const auto clearance : {-1.f, .5f, 1.f, 3.f})
        bad([&](auto& j) { j["kits"][0]["clearance"] = clearance; });
    bad([](auto& j) { j["kits"][0]["clearance"] = "high"; });
    bad([](auto& j) { j["kits"][0]["fire"] = {0, .2}; });
    bad([](auto& j) { j["kits"][0]["fire"] = {0, .8, 0}; });
    for (const auto scale : {0.f, -1.f, 1.1f})
        bad([&](auto& j) { j["kits"][0]["fire"] = {0, .2, 0, scale}; });
    bad([](auto& j) { j["kits"][0]["fire"] = {0, .2, 0, "small"}; });
    bad([](auto& j) { j["kits"][0]["fire"] = {0, .2, 0, .5, 1}; });
    bad([](auto& j) { j["kits"].push_back(j["kits"][0]); });
    bad([](auto& j) { j["rooms"][0]["placements"][0]["kit"] = "absent"; });
    bad([](auto& j) { j["rooms"][0]["placements"][0]["facing"] = "mirror"; });
    bad([](auto& j) { j["rooms"][0]["placements"].push_back(j["rooms"][0]["placements"][0]); });
    bad([](auto& j) { j["rooms"][0]["placements"][0]["anchor"] = {2, 0}; });
    bad([](auto& j) { j["rooms"][0]["rewards"][0]["access"] = {0, 0}; });
    bad([](auto& j) { j["rooms"][0]["encounters"][0]["origin"] = {0, 0}; });
    bad([](auto& j) { j["rooms"][0]["sockets"][0]["outward"] = "south"; });
    bad([](auto& j) { j["rooms"][0]["sockets"][0]["cell"] = {0, -1}; });
    bad([](auto& j) { j["rooms"][0]["rotations"] = {0, 45}; });
    bad([](auto& j) { j["rooms"][0]["rotations"] = {0, 0}; });
    bad([](auto& j) { j["rooms"][0]["solid"] = {{8, 8}}; });
    bad([](auto& j) {
        j["rooms"][0]["placements"] = nlohmann::json::array();
        for (int i = 0; i < 17; ++i)
            j["rooms"][0]["placements"].push_back({});
    });
    rejects([] { bp::decode(std::string(bp::maxJsonBytes + 1, ' ')); }, "Oversize source rejected");
    rejects([] { bp::decode("{\"schema_version\":1,\"schema_version\":1}"); }, "Duplicate key rejected");
    auto c = test::blueprintCatalog();
    c.rooms[0].solid[bp::index({0, 3})] = true;
    c.rooms[0].solid[bp::index({1, 4})] = true;
    rejects([&] { bp::validate(c); }, "Disconnected corner cannot rely on an outside detour");
    c = test::blueprintCatalog();
    c.rooms[0].sockets[1].cell.y = 1;
    rejects([&] { bp::assemble(c, "kitchen-hall"); }, "Mismatching pair sockets reject");
    c = test::blueprintCatalog();
    c.kitCount = bp::maxKits + 1;
    rejects([&] { bp::validate(c); }, "In-memory catalog capacity rejects before indexing");
}
void overhead() {
    auto j = test::blueprintJson();
    j["kits"].push_back(
        {{"id", "overhead"}, {"size", {3, 3}}, {"height", 1.5}, {"clearance", 1.1}, {"blocking", false}});
    j["rooms"][0]["placements"].push_back({{"kit", "overhead"}, {"anchor", {0, 0}}, {"facing", "north"}});
    const auto c = bp::decode(j.dump()); // Overlaps a floor kit, lane and encounter region.
    for (unsigned t = 0; t < 4; ++t) {
        const auto scene = bp::assemble(c, "kitchen", t);
        const auto& ceiling = scene.map.definition().scenery[1];
        check(ceiling.clearance == 1.1f && !ceiling.blocking, "Ceiling placement retains clearance");
        check(scene.map.movementBlocked(ceiling.cell(0, 0)),
              "Floor furniture still blocks under ceiling kit");
        check(!scene.map.movementBlocked(ceiling.cell(1, 1)) && !scene.map.npcBlocked(ceiling.cell(1, 1)),
              "Overhead footprint leaves unused floor cells walkable in every rotation");
    }
    auto invalid = j;
    invalid["kits"][1]["blocking"] = true;
    rejects([&] { bp::decode(invalid.dump()); }, "Overhead geometry cannot block the floor");
    invalid = j;
    invalid["kits"][0]["height"] = 1.2;
    rejects([&] { bp::decode(invalid.dump()); }, "Tall floor furniture cannot intersect overhead volume");
    invalid = j;
    invalid["rooms"][0]["placements"].push_back(invalid["rooms"][0]["placements"][1]);
    rejects([&] { bp::decode(invalid.dump()); }, "Overhead visual overlap rejects");
    invalid = j;
    invalid["rooms"][0]["ceiling"] = 4;
    rejects([&] { bp::decode(invalid.dump()); }, "Low ceiling cannot host suspended kit");
}
void closedWallKits() {
    auto j = test::blueprintJson();
    j["kits"].push_back({{"id", "wall-fixture"},
                         {"size", {1, 1}},
                         {"height", .8},
                         {"blocking", false},
                         {"closedWall", true}});
    j["rooms"][0]["placements"].push_back({{"kit", "wall-fixture"}, {"anchor", {2, 0}}, {"facing", "south"}});
    j["rooms"][0]["placements"].push_back({{"kit", "wall-fixture"}, {"anchor", {0, 2}}, {"facing", "east"}});
    const auto catalog = bp::decode(j.dump());
    for (unsigned turn = 0; turn < 4; ++turn) {
        for (unsigned sockets : {1u, 4u, 8u, 9u, 15u}) {
            const auto scene = bp::assemble(catalog, "kitchen", turn, sockets);
            const unsigned expected = 3 - bool(sockets & 1) - bool(sockets & 8);
            check(scene.map.definition().scenery.size() == expected &&
                      scene.rooms[0].blueprint.placementCount == expected,
                  "Conditional scenery and plan omit only kits attached to selected doorways");
            for (const auto& prop : scene.map.definition().scenery)
                if (prop.model == "wall-fixture") {
                    check(!scene.map.movementBlocked(prop.anchor) && !scene.map.npcBlocked(prop.anchor),
                          "Wall shelves have no party or NPC collision footprint");
                    check(scene.map.solid(prop.anchor + forward(prop.facing) * -1),
                          "Retained conditional kits have actual solid wall backing");
                }
        }
        const auto pair = bp::assemble(catalog, "kitchen-hall", turn);
        check(pair.rooms[0].blueprint.placementCount == 3,
              "Pair retains kitchen shelving on unused north and west walls");
    }
    auto invalid = j;
    invalid["kits"][1]["closedWall"] = "yes";
    rejects([&] { bp::decode(invalid.dump()); }, "Wall condition requires a boolean");
    invalid = j;
    invalid["kits"][1]["blocking"] = true;
    rejects([&] { bp::decode(invalid.dump()); }, "Conditional wall kit cannot reserve movement");
    invalid = j;
    invalid["kits"][1]["size"] = {1, 2};
    rejects([&] { bp::decode(invalid.dump()); }, "Conditional wall kits must fit one backing cell");
    invalid = j;
    invalid["kits"][1]["closedWall"] = false;
    rejects([&] { bp::decode(invalid.dump()); }, "Ordinary nonblocking furnishings cannot occupy sockets");
    invalid = j;
    invalid["rooms"][0]["placements"][1]["facing"] = "north";
    rejects([&] { bp::decode(invalid.dump()); }, "Wall kit must face into room with backing wall");
}
void transforms() {
    {
        auto json = test::blueprintJson();
        json["kits"][0]["blocking"] = false;
        json["kits"][0]["fire"] = {.12, .2, .25};
        check(bp::decode(json.dump()).kits[0].fire->scale == 1.f, "Legacy anchors retain default flame size");
        json["kits"][0]["fire"] = {.12, .2, .25, .5};
        const auto decorative = bp::decode(json.dump());
        for (unsigned turn = 0; turn < 4; ++turn) {
            const auto scene = bp::assemble(decorative, "kitchen-hall", turn);
            check(scene.map.definition().ambientFires.size() == 2, "Pair assembles both local fire anchors");
            unsigned i = 0;
            for (const auto& s : scene.map.definition().scenery) {
                check(!s.blocking && !scene.map.movementBlocked(s.anchor) && !scene.map.npcBlocked(s.anchor),
                      "Decorative blueprint footprint does not reserve movement");
                const auto& fire = scene.map.definition().ambientFires[i++];
                const auto r = right(s.facing), f = forward(s.facing);
                check(std::abs(fire.x - (s.anchor.x + .5f + .12f * r.x - .25f * f.x)) < .0001f &&
                          std::abs(fire.z - (s.anchor.y + .5f + .12f * r.y - .25f * f.y)) < .0001f &&
                          fire.y == .2f && fire.scale == .5f,
                      "Fire anchors rotate and translate with their models");
            }
        }
        WorldDefinition excessive;
        excessive.ambientFires.resize(ambientFireCapacity);
        rejects([&] { bp::appendFurnishings(decorative, decorative.rooms[0], {0, 0}, excessive); },
                "Assembled fire capacity rejects");
        auto overlap = json;
        overlap["rooms"][0]["placements"].push_back(overlap["rooms"][0]["placements"][0]);
        rejects([&] { bp::decode(overlap.dump()); }, "Decorative blueprint visual overlap rejects");
    }
    const auto c = test::blueprintCatalog();
    const auto& r = c.rooms[c.room("barracks").index];
    const auto t = bp::transform(r, 1);
    check(t.size == Cell{7, 5} && t.sockets[0].cell == Cell{7, 2} && t.sockets[0].outward == Heading::East,
          "Rectangular room and north socket rotate clockwise");
    auto round = r;
    for (unsigned i = 0; i < 4; ++i)
        round = bp::transform(round, 1);
    check(round.size == r.size && round.placements[0].anchor == r.placements[0].anchor &&
              round.sockets[1].cell == r.sockets[1].cell && round.rewards[0].access == r.rewards[0].access,
          "Four quarter turns restore placement/socket/reward coordinates");
    unsigned variants = 0;
    for (unsigned n = 0; n < c.roomCount; ++n)
        for (unsigned turn = 0; turn < 4; ++turn)
            for (unsigned mask = 1; mask < 16; ++mask) {
                auto scene = bp::assemble(c, c.rooms[n].id.data(), turn, mask);
                check(scene.map.connected(scene.spawn.cell), "Every doorway subset has connected movement");
                for (const auto& s : scene.map.definition().scenery)
                    for (unsigned y = 0; y < s.depth; ++y)
                        for (unsigned x = 0; x < s.width; ++x)
                            check(scene.map.movementBlocked(s.cell(x, y)) &&
                                      scene.map.npcBlocked(s.cell(x, y)) && !scene.map.solid(s.cell(x, y)),
                                  "Scenery blocks movement without terrain walls");
                ++variants;
            }
    for (unsigned turn = 0; turn < 4; ++turn) {
        const auto pair = bp::assemble(c, "kitchen-hall", turn);
        check(pair.fixtures.count == 3 && pair.roomCount == 2,
              "Pair owns exactly one shared door and two outside doors");
    }
    rejects([&] { bp::assemble(c, "kitchen", 0, 16); }, "Unknown socket bits reject");
    std::ostringstream svg;
    const auto scene = bp::assemble(c, "kitchen-hall");
    bp::writePlan(svg, c, scene, "kitchen-hall");
    check(svg.str().find("fixture") != std::string::npos && svg.str().find("viewBox") != std::string::npos,
          "Diagram comes from catalog");
    std::cout << variants << " owned rotation/socket assemblies checked\n";
}
void gameplay() {
    const auto c = test::blueprintCatalog();
    const auto rules = fixtureCatalog();
    for (const auto* id : {"kitchen", "mead-hall", "barracks", "armory", "kitchen-hall"}) {
        const auto scene = bp::assemble(c, id);
        const NpcWorld empty(scene.map, scene.spawn.cell, {"goblin"}, {});
        auto owner =
            std::make_unique<GameSession>(scene.map, rules, empty, 5, scene.spawn, false,
                                          std::vector<NpcTimings>{}, RecoveryLevel{}, scene.fixtures);
        auto& game = *owner;
        for (unsigned i = 0; i < scene.fixtures.count; ++i) {
            const auto& f = scene.fixtures.entries[i];
            game.restorePose({f.cell + forward(f.facing), bp::rotate(f.facing, 2)}, true);
            game.discover();
            check(!game.geometry().canStep(game.movement().player.cell, f.cell), "Closed door blocks entry");
            check(bool(game.interact(fixtureRequest(game, i))), "Open through normal checked command");
            check(game.actionBusy(), "Door opening retains movement barrier");
            game.update(.31, false);
            check(!game.actionBusy() && game.geometry().canStep(game.movement().player.cell, f.cell),
                  "Settled door allows traversal");
            game.move(Action::Forward);
            game.update(.2, false);
            check(game.movement().player.cell == f.cell, "Player traverses selected socket");
            game.move(Action::Forward);
            game.update(.2, false);
            check(game.movement().player.cell == f.cell + forward(f.facing) * -1,
                  "Player reaches other doorway side");
            game.move(Action::TurnRight);
            game.update(.2, false);
            game.move(Action::TurnRight);
            game.update(.2, false);
            check(bool(game.interact(fixtureRequest(game, i))), "Door closes from other side");
            game.update(.31, false);
        }
        const auto& rr = scene.rooms[0];
        const auto reward = rr.blueprint.rewards[0];
        const Cell a = rr.origin + reward.access, b = rr.origin + reward.cell;
        Heading h = Heading::North;
        for (unsigned i = 0; i < 4; ++i)
            if (a + forward(Heading(i)) == b)
                h = Heading(i);
        game.restorePose({a, h}, false);
        const auto container = game.nearbyContainer();
        check(bool(container) && bool(game.openContainer(container)),
              "Authored reward access opens empty chest");
        allocations = 0;
        watching = true;
        for (unsigned i = 0; i < 8; ++i) {
            game.move(Action::TurnRight);
            game.update(.2, false);
        }
        watching = false;
        check(!allocations, "Engine-owned review movement processing allocates nothing");
    }
}
void combatSpace() {
    const auto c = test::blueprintCatalog();
    const auto rules = hybridCatalog(false);
    for (const auto* name : {"kitchen", "mead-hall", "barracks", "armory", "kitchen-hall"}) {
        const auto scene = bp::assemble(c, name);
        const auto& room = scene.rooms[scene.roomCount - 1];
        const Player pose{room.origin + Cell{2, 4}, Heading::North};
        NpcWorld placements(scene.map, pose.cell, {"swordsman", "archer"},
                            {{{0}, scene.combatCells[0], {1, 1}, Heading::South},
                             {{1}, scene.combatCells[1], {1, 1}, Heading::South}});
        auto game = std::make_unique<GameSession>(scene.map, rules, placements, 5, pose, true,
                                                  std::vector<NpcTimings>{}, RecoveryLevel{}, scene.fixtures);
        bool melee = false, ranged = false;
        for (unsigned tick = 0; tick < 240 && !(melee && ranged); ++tick) {
            allocations = 0;
            watching = true;
            if (game->combat().snapshot().awaitingAction)
                game->submit(turnCommand(*game));
            else
                for (const auto& event : game->update(.5, false))
                    if (event.event == CombatEvent::AttackStarted && event.enemyAttack) {
                        melee |= event.kind == rules::AttackKind::Melee;
                        ranged |= event.kind == rules::AttackKind::Ranged;
                    }
            watching = false;
            check(!allocations, "Blueprint encounter processing reuses engine-owned storage");
            for (unsigned i = 0; i < game->actors().snapshot().count; ++i) {
                const auto& actor = game->actors().snapshot().actors[i];
                if (actor.life == ActorLife::Alive)
                    check(!scene.map.npcBlocked(actor.npc.cell), "Encounter never enters authored furniture");
            }
        }
        check(melee && ranged, "Melee and ranged actors can attack within bounded review rounds");
    }
}
} // namespace
int main() try {
    malformed();
    overhead();
    closedWallKits();
    transforms();
    gameplay();
    combatSpace();
    std::cout << "Blueprint loading, circulation, rotation, doors, rewards and allocations passed\n";
    return 0;
} catch (const std::exception& e) {
    watching = false;
    std::cerr << e.what() << '\n';
    return 1;
}
