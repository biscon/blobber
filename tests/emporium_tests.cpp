#include "game/commands.hpp"
#include "game/emporium_interior.hpp"
#include "persistence/campaign_codec.hpp"
#include "support/allocations.hpp"
#include "support/campaign_fixtures.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::campaign_test;
namespace {
void door(CampaignRun& run, const char* name) {
    const auto approach = sanctuaryCity().doorway(name).approach;
    walk(run.game(), approach.cell);
    face(run.game(), approach.heading);
}
void commit(CampaignRun& run) {
    auto ticket = run.prepareTravel(run.doorwayRequest());
    check(run.commitTravel(ticket), "Authored doorway commits");
}
void layout() {
    const auto authored = emporiumInterior();
    const auto& map = authored.map;
    check(map.connected(authored.entry.cell) && map.width() == 12 && map.height() == 12,
          "Emporium is an enclosed connected authored room");
    for (const auto& s : map.definition().scenery)
        for (unsigned y = 0; y < s.depth; ++y)
            for (unsigned x = 0; x < s.width; ++x)
                check(map.movementBlocked(s.cell(x, y)) && !map.solid(s.cell(x, y)),
                      "Furniture blocks whole cells without becoming full-height walls");
    for (int y = 5; y <= 9; ++y)
        for (int x = 5; x <= 6; ++x)
            check(!map.movementBlocked({x, y}), "Two-cell customer approach stays open");
    for (int y = 3; y <= 9; ++y)
        check(!map.movementBlocked({8, y}), "Staff bypass stays open");
    const auto cat = catalog();
    const auto defs = definitions();
    Party party(cat, *cat.preset());
    NpcInstance future{{7}, emporiumResidentCell, emporiumResidentSubcell, Heading::South};
    future.role = NpcRole::Resident;
    future.residentId = dialogueKey("fixture.emporium");
    NpcWorld npcs(map, authored.entry.cell, defs.types, {future});
    GameSession game(map, party, npcs, 5, authored.entry, defs.timings, authored.recovery, authored.fixtures,
                     {});
    for (int y = 2; y <= 9; ++y)
        for (int x = 2; x <= 9; ++x)
            if (!map.movementBlocked({x, y}) && !game.actors().occupied({x, y}))
                walk(game, {x, y});
    walk(game, emporiumCustomer.cell);
    face(game, Heading::South);
    check(!game.beginConversation(game.actors().id(0)), "Wrong customer facing rejects conversation");
    face(game, emporiumCustomer.heading);
    check(game.beginConversation(game.actors().id(0)), "Reserved merchant is in dialogue range");
    game.endConversation();
}
void campaign() {
    const auto cat = catalog();
    const auto defs = definitions();
    CampaignRun run(cat, *cat.preset(), defs.types, defs.timings, 5);
    const auto approach = sanctuaryCity().doorway("emporium").approach;
    check(run.location(0).doorwayCount == 3 && run.location(4).doorwayCount == 1 &&
              run.location(5).doorwayCount == 1 && run.location(6).doorwayCount == 1,
          "Three canonical city links and one return per interior");
    check(!run.previewTravel(run.doorwayRequest()), "No remote interior entry");
    door(run, "emporium");
    const auto request = run.doorwayRequest();
    check(request.doorway.slot == 2 && run.previewTravel(request), "Emporium is the third city doorway");
    check(run.location(0).fixtures.npcExcluded.test(std::size_t(approach.cell.y * 24 + approach.cell.x)),
          "Emporium approach excludes NPC placement");
    for (auto slot : {std::uint16_t(0), std::uint16_t(1), std::uint16_t(3), std::uint16_t(UINT16_MAX)}) {
        auto bad = request;
        bad.doorway.slot = slot;
        check(!run.previewTravel(bad), "Wrong or out-of-range city slot rejects");
    }
    auto bad = request;
    ++bad.doorway.world;
    check(!run.previewTravel(bad) && !run.doorway(bad.doorway), "Foreign doorway rejects");
    bad = request;
    ++bad.revision;
    check(!run.previewTravel(bad), "Stale travel revision rejects");
    face(run.game(), Heading::West);
    check(!run.previewTravel(request), "Wrong facing rejects");
    face(run.game(), approach.heading);
    CommandController outside(run.game(), defs.timings);
    outside.refresh({}, false, false, true);
    check(std::string_view(outside.snapshot().fixturePrompt.data()) == "[F] Enter Arcane Emporium",
          "City prompt names the emporium");
    outside.interactFixture();
    check(outside.snapshot().travelRequested && outside.snapshot().travelRequested->doorway.slot == 2,
          "F routes the third city doorway");
    const auto source = run.game().movement().player;
    {
        auto canceled = run.prepareTravel(request);
    }
    check(run.activeLocation() == 0 && run.game().movement().player == source,
          "Cancel preserves source city and pose");
    bool failed = false;
    allocationsUntilFailure = 0;
    try {
        auto ticket = run.prepareTravel(request);
    } catch (const std::bad_alloc&) {
        failed = true;
    }
    allocationsUntilFailure = -1;
    check(failed && run.activeLocation() == 0 && run.game().movement().player == source,
          "Preparation failure preserves source");
    {
        auto ticket = run.prepareTravel(request);
        face(run.game(), Heading::West);
        check(!run.commitTravel(ticket), "Changed source pose rejects staged commit");
    }
    face(run.game(), approach.heading);
    commit(run);
    check(run.activeLocationId() == CampaignLocationId::Emporium && !run.dungeonDepth() &&
              std::string_view(run.name()) == "Arcane Emporium" && run.game().actors().snapshot().count == 0,
          "Emporium is a separate depth-zero room with future merchant space");
    check(!run.previewTravel(request), "Source doorway handle is stale after travel");
    check(!run.previewTravel(run.recallRequest()) && !run.previewTravel(run.waypointRequest(1)) &&
              !run.game().atSanctuary(),
          "Ordinary rest and no interior travel shortcuts");
    CommandController inside(run.game(), defs.timings);
    inside.openTravel();
    check(!inside.snapshot().travel.open, "No travel overlay inside emporium");
    for (unsigned i = 0; i < 2; ++i) {
        run.update(.02, false);
        inside.refresh({}, false, false, true);
    }
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 32; ++i) {
        run.update(.02, false);
        inside.refresh({}, false, false, true);
        (void)run.previewTravel(run.doorwayRequest());
    }
    watching = false;
    check(!allocations, "Warmed room frames and door queries allocate no engine C++ storage");
    walk(run.game(), emporiumCustomer.cell);
    face(run.game(), emporiumCustomer.heading);
    inside.refresh({}, false, false, true);
    check(!inside.snapshot().fixturePrompt[0], "Future merchant space has no fake interaction");
    ItemHandle item;
    for (std::uint16_t i = 0; i < bagCells && !item; ++i) {
        const auto candidate = run.party().inventory().at({{0}, i});
        if (run.party().inventory().stack(candidate))
            item = candidate;
    }
    check(bool(item) && bool(run.game().dropItem(item, run.party().inventory().revision())),
          "Owned fixture drops an item inside emporium");
    const auto loot = run.game().loot().snapshot();
    run.game().discover();
    const auto pose = run.game().movement().player;
    const auto checkpoint = run.checkpoint();
    const auto discovery = checkpoint->locations[6].state->fixtures.discovered;
    const auto text = encodeCampaign(cat, defs, {"fixture", 0}, *checkpoint);
    auto decoded = decodeCampaign(cat, defs, {"fixture", 0}, text);
    auto restored = std::make_unique<CampaignRun>(cat, std::move(*decoded), defs.timings);
    const auto drops = [&](const GameSession& game) {
        for (unsigned i = 0; i < loot.stacks.size(); ++i) {
            const auto& a = loot.stacks[i];
            const auto& b = game.loot().snapshot().stacks[i];
            if (a.active != b.active || (a.active && (!(a.cell == b.cell) || a.quantity != b.quantity ||
                                                      a.item.index != b.item.index)))
                return false;
        }
        return true;
    };
    check(restored->activeLocationId() == CampaignLocationId::Emporium &&
              restored->game().movement().player == pose && drops(restored->game()) &&
              restored->checkpoint()->locations[6].state->fixtures.discovered == discovery,
          "Continue retains emporium, pose, drops and discovery");
    for (unsigned fault = 0; fault < 6; ++fault) {
        auto invalid = restored->checkpoint();
        auto& location = invalid->locations[6];
        if (fault == 0)
            location.recovery.freeRestEverywhere = true;
        if (fault == 1)
            location.entry.heading = Heading::West;
        if (fault == 2)
            location.placements.push_back({{7}, {6, 3}, {1, 1}, Heading::South});
        if (fault == 3)
            location.rows[6][6] = '#';
        if (fault == 4) {
            ++location.world.scenery[0].anchor.y;
            location.worldFingerprint = location.world.fingerprint();
        }
        if (fault == 5)
            location.fixtures.npcExcluded.reset();
        rejects([&] { CampaignRun rejected(cat, std::move(*invalid), defs.timings); },
                "Noncanonical emporium state rejects");
    }
    auto old = blobber::automation::Json::parse(text);
    old["schema"] = 15;
    old["simulation"] = 18;
    rejects([&] { decodeCampaign(cat, defs, {"fixture", 0}, old.dump()); }, "Six-location saves reject");
    const auto exit = restored->location(6).exit;
    walk(restored->game(), exit.cell);
    face(restored->game(), exit.heading);
    CommandController leaving(restored->game(), defs.timings);
    leaving.refresh({}, false, false, true);
    check(std::string_view(leaving.snapshot().fixturePrompt.data()) == "[F] Leave Arcane Emporium",
          "Exit prompt names the emporium");
    commit(*restored);
    check(restored->game().movement().player == Player{approach.cell, Heading::West},
          "Exit returns to the same street facing away");
    enterInn(*restored);
    leaveInn(*restored);
    door(*restored, "smithy");
    check(restored->doorwayRequest().doorway.slot == 1, "Smithy slot stays stable");
    commit(*restored);
    check(restored->game().actors().snapshot().count == 1, "Duncan remains in the smithy");
    const auto smithyExit = restored->location(5).exit;
    walk(restored->game(), smithyExit.cell);
    face(restored->game(), smithyExit.heading);
    commit(*restored);
    door(*restored, "emporium");
    commit(*restored);
    check(drops(restored->game()), "Inactive emporium retains drops through inn and smithy travel");

    Client client(cat, defs);
    auto scenario = generatedScenario(cat);
    scenario["kind"] = "campaign";
    scenario.erase("depth");
    check(client.request("start", {{"scenario", scenario}}).at("ok"), "Wire campaign starts");
    const auto move = [&](const char* direction) {
        check(client.act({{"kind", "move"}, {"direction", direction}}).at("data").at("accepted"),
              "Wire city movement accepts");
        client.advance(100, false);
    };
    move("turn_right");
    for (unsigned i = 0; i < 5; ++i)
        move("forward");
    move("turn_right");
    move("forward");
    move("turn_left");
    check(client.act({{"kind", "doorway"}}).at("data").at("accepted"), "Wire emporium entry accepts");
    const auto observed = client.protocol.state().at("campaign");
    check(observed.at("location") == 6 && observed.at("depth") == 0 &&
              observed.at("location_name") == "Arcane Emporium" &&
              observed.at("locations").size() == campaignLocationCount &&
              observed.at("locations")[6].at("fingerprint") == emporiumInteriorIdentity,
          "Wire diagnostics identify the seventh authored location");
}
} // namespace
int main() try {
    layout();
    campaign();
    std::cout << "Emporium layout, third doorway, future resident and persistence checks passed\n";
    return 0;
} catch (const std::exception& error) {
    watching = false;
    allocationsUntilFailure = -1;
    std::cerr << error.what() << '\n';
    return 1;
}
