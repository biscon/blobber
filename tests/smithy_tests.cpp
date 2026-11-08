#include "game/commands.hpp"
#include "game/smithy_interior.hpp"
#include "persistence/campaign_codec.hpp"
#include "support/allocations.hpp"
#include "support/campaign_fixtures.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::campaign_test;
namespace {
void doorway(CampaignRun& run, const char* name) {
    const auto approach = sanctuaryCity().doorway(name).approach;
    walk(run.game(), approach.cell);
    face(run.game(), approach.heading);
}
void commitDoor(CampaignRun& run) {
    auto ticket = run.prepareTravel(run.doorwayRequest());
    check(run.commitTravel(ticket), "Doorway commits");
}
void layout() {
    const auto authored = smithyInterior();
    const auto& map = authored.map;
    check(map.width() == 12 && map.height() == 12 && map.connected(authored.entry.cell),
          "Authored workshop is enclosed and connected");
    for (const auto& s : map.definition().scenery)
        for (unsigned y = 0; y < s.depth; ++y)
            for (unsigned x = 0; x < s.width; ++x)
                check(map.movementBlocked(s.cell(x, y)) && !map.solid(s.cell(x, y)),
                      "Furniture blocks movement without becoming a full sight wall");
    for (int y = 5; y <= 9; ++y)
        for (int x = 5; x <= 6; ++x)
            check(!map.movementBlocked({x, y}), "Two-cell main approach remains open");
    for (int y = 3; y <= 9; ++y)
        check(!map.movementBlocked({8, y}), "Right-hand staff passage remains open");
    const auto cat = catalog();
    const auto defs = definitions();
    Party party(cat, *cat.preset());
    const auto smith = blacksmithPlacement(defs.types);
    check(defs.types[smith.type.index] == "blacksmith" && smith.role == NpcRole::Resident &&
              smith.residentId == dialogueKey("sanctuary.blacksmith"),
          "Duncan binds the dedicated resident appearance and stable identity");
    auto missing = defs.types;
    missing.erase(std::find(missing.begin(), missing.end(), "blacksmith"));
    rejects([&] { blacksmithPlacement(missing); }, "Missing blacksmith appearance rejects");
    NpcWorld npcs(map, authored.entry.cell, defs.types, {smith});
    GameSession game(map, party, npcs, 5, authored.entry, defs.timings, authored.recovery, authored.fixtures,
                     {});
    for (int y = 2; y <= 9; ++y)
        for (int x = 2; x <= 9; ++x)
            if (!map.movementBlocked({x, y}) && !game.actors().occupied({x, y}))
                walk(game, {x, y});
    walk(game, smithyCustomer.cell);
    face(game, Heading::South);
    check(!game.beginConversation(game.actors().id(0)), "Wrong customer facing rejects conversation");
    face(game, smithyCustomer.heading);
    check(game.beginConversation(game.actors().id(0)), "Duncan is in existing conversation range");
    game.endConversation();
}
void campaign() {
    const auto cat = catalog();
    const auto defs = definitions();
    CampaignRun run(cat, *cat.preset(), defs.types, defs.timings, 5);
    const auto approach = sanctuaryCity().doorway("smithy").approach;
    check(!run.previewTravel(run.doorwayRequest()), "No remote doorway entry");
    doorway(run, "smithy");
    auto request = run.doorwayRequest();
    check(request.doorway.slot == 1 && run.previewTravel(request), "Smithy owns second city link");
    check(run.location(0).fixtures.npcExcluded.test(std::size_t(approach.cell.y * 24 + approach.cell.x)),
          "City smithy approach excludes NPC placement");
    for (auto slot : {std::uint16_t(0), std::uint16_t(2), std::uint16_t(UINT16_MAX)}) {
        auto bad = request;
        bad.doorway.slot = slot;
        check(!run.previewTravel(bad), "Wrong or out-of-range doorway slot rejects");
    }
    auto bad = request;
    ++bad.doorway.world;
    check(!run.previewTravel(bad) && !run.doorway(bad.doorway), "Foreign doorway world rejects");
    bad = request;
    ++bad.revision;
    check(!run.previewTravel(bad), "Foreign campaign revision rejects");
    face(run.game(), Heading::West);
    check(!run.previewTravel(request), "Wrong facing rejects a previously valid request");
    face(run.game(), approach.heading);
    CommandController commands(run.game(), defs.timings);
    commands.refresh({}, false, false, true);
    check(std::string_view(commands.snapshot().fixturePrompt.data()) == "[F] Enter smithy",
          "Smithy prompt uses canonical link label");
    commands.interactFixture();
    check(commands.snapshot().travelRequested && commands.snapshot().travelRequested->doorway.slot == 1,
          "F dispatches smithy link");
    const auto source = run.game().movement().player;
    {
        auto canceled = run.prepareTravel(request);
    }
    check(run.activeLocation() == 0 && run.game().movement().player == source,
          "Canceled preparation preserves source");
    bool failed = false;
    allocationsUntilFailure = 0;
    try {
        auto ticket = run.prepareTravel(request);
    } catch (const std::bad_alloc&) {
        failed = true;
    }
    allocationsUntilFailure = -1;
    check(failed && run.activeLocation() == 0 && run.game().movement().player == source,
          "Allocation failure preserves source");
    {
        auto ticket = run.prepareTravel(request);
        face(run.game(), Heading::West);
        check(!run.commitTravel(ticket), "Changed source pose rejects staged commit");
    }
    face(run.game(), approach.heading);
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 32; ++i) {
        (void)run.previewTravel(run.doorwayRequest());
        commands.refresh({}, false, false, true);
    }
    watching = false;
    check(!allocations, "Warmed two-doorway queries and prompts allocate no C++ storage");
    commitDoor(run);
    check(run.activeLocationId() == CampaignLocationId::Smithy && !run.dungeonDepth() &&
              std::string_view(run.name()) == "Sanctuary smithy" && run.game().actors().snapshot().count == 1,
          "Smithy is an authored depth-zero location with one resident");
    const auto& resident = run.game().actors().snapshot().actors[0];
    check(!resident.hostile() && resident.npc.cell == smithyResidentCell &&
              resident.npc.subcell == smithyResidentSubcell && resident.npc.facing == Heading::South &&
              run.game().actors().occupied(smithyResidentCell),
          "Duncan occupies the nearest counter subcell, facing the customer");
    check(!run.previewTravel(request), "City request becomes stale after travel");
    check(!run.previewTravel(run.recallRequest()) && !run.previewTravel(run.waypointRequest(1)),
          "Interior cannot bypass its door with recall or dungeon travel");
    CommandController inside(run.game(), defs.timings);
    inside.openTravel();
    check(!inside.snapshot().travel.open, "Travel overlay stays closed indoors");
    check(!run.game().atSanctuary(), "Smithy grants no inn rest policy");
    for (unsigned i = 0; i < 2; ++i) {
        run.update(.02, false);
        inside.refresh({}, false, false, true);
    }
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 32; ++i) {
        run.update(.02, false);
        inside.refresh({}, false, false, true);
    }
    watching = false;
    check(!allocations, "Warmed smithy simulation and command frames allocate no C++ storage");
    walk(run.game(), smithyCustomer.cell);
    face(run.game(), smithyCustomer.heading);
    inside.refresh({}, false, false, true);
    check(run.game().beginConversation(run.game().actors().id(0)), "Counter conversation admits Duncan");
    const auto talkingPose = run.game().movement().player;
    run.game().move(Action::Forward);
    check(run.game().movement().player == talkingPose, "Conversation cannot move through the counter");
    run.game().endConversation();
    constexpr auto host = dialogueKey("sanctuary.blacksmith");
    run.dialogue().get(host, true)->introduced = true;
    check(run.dialogue().complete(host, dialogueKey("craft"), 0), "Duncan topic history records");
    ShopDefinition shop;
    shop.id = dialogueKey("fixture.smithy.shop");
    shop.key = "fixture.smithy.shop";
    shop.stock.push_back({{0}, 2});
    auto* vendor = run.merchants().visit(host, shop, run.game().gameTime());
    check(vendor && vendor->stock[0] == 2, "Duncan owns a separate merchant ledger");
    vendor->stock[0] = 1; // Isolated persisted stock fixture, not a transaction assertion.
    ItemHandle item;
    for (std::uint16_t b = 0; b < bagCells && !item; ++b) {
        const auto candidate = run.party().inventory().at({{0}, b});
        if (run.party().inventory().stack(candidate))
            item = candidate;
    }
    check(bool(item), "Owned party fixture has an item");
    check(bool(run.game().dropItem(item, run.party().inventory().revision())), "Drop inside smithy");
    const auto loot = run.game().loot().snapshot();
    const auto pose = run.game().movement().player;
    run.game().discover();
    auto checkpoint = run.checkpoint();
    const auto discovered = checkpoint->locations[5].state->fixtures.discovered;
    const auto text = encodeCampaign(cat, defs, {"fixture", 0}, *checkpoint);
    auto decoded = decodeCampaign(cat, defs, {"fixture", 0}, text);
    auto restored = std::make_unique<CampaignRun>(cat, std::move(*decoded), defs.timings);
    check(restored->activeLocationId() == CampaignLocationId::Smithy &&
              restored->game().movement().player == pose,
          "Continue restores smithy and pose");
    check(restored->checkpoint()->locations[5].state->fixtures.discovered == discovered,
          "Smithy discovery survives disk roundtrip");
    const auto* history = restored->dialogue().get(host);
    const auto* merchant = restored->merchants().find(host);
    check(history && history->introduced && history->completedCount == 1 &&
              history->completed[0] == dialogueKey("craft") && merchant && merchant->stock[0] == 1,
          "Duncan conversation history and finite stock survive Continue");
    check(restored->game().actors().snapshot().count == 1 &&
              restored->game().actors().snapshot().actors[0].npc.residentId == host,
          "Continue reconstructs Duncan's canonical resident");
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
    check(drops(restored->game()), "Dropped items survive Continue");
    for (unsigned fault = 0; fault < 14; ++fault) {
        auto invalid = restored->checkpoint();
        auto& location = invalid->locations[5];
        if (fault == 0)
            location.recovery.freeRestEverywhere = true;
        if (fault == 1)
            location.entry.heading = Heading::West;
        if (fault == 2)
            location.placements.push_back({{7}, {6, 3}, {1, 1}, Heading::South});
        if (fault == 3)
            location.rows[6][6] = '#';
        if (fault == 4) {
            location.world.scenery[0].anchor.y = 3;
            location.worldFingerprint = location.world.fingerprint();
        }
        if (fault == 5)
            location.placements.clear();
        if (fault == 6)
            location.placements[0].residentId = dialogueKey("sanctuary.innkeeper");
        if (fault == 7)
            location.placements[0].cell.x = 7;
        if (fault == 8)
            location.placements[0].subcell.y = 0;
        if (fault == 9)
            location.placements[0].facing = Heading::North;
        if (fault == 10)
            location.placements[0].animation = NpcAnimation::Talk;
        if (fault == 11)
            location.placements[0].phaseMs = 1;
        if (fault == 12)
            location.placements[0].role = NpcRole::Hostile;
        if (fault == 13)
            location.state->actors.actors[0].npc.cell.x = 7;
        rejects([&] { CampaignRun rejected(cat, std::move(*invalid), defs.timings); },
                "Noncanonical smithy state rejects");
    }
    auto old = blobber::automation::Json::parse(text);
    old["schema"] = 14;
    old["simulation"] = 17;
    rejects([&] { decodeCampaign(cat, defs, {"fixture", 0}, old.dump()); }, "Old saves reject unchanged");
    const auto exit = restored->location(5).exit;
    walk(restored->game(), exit.cell);
    face(restored->game(), exit.heading);
    CommandController leaving(restored->game(), defs.timings);
    leaving.refresh({}, false, false, true);
    check(std::string_view(leaving.snapshot().fixturePrompt.data()) == "[F] Leave smithy",
          "Interior exit has smithy label");
    commitDoor(*restored);
    check(restored->game().movement().player == Player{approach.cell, Heading::West},
          "Smithy exit faces the street");
    enterInn(*restored);
    leaveInn(*restored);
    doorway(*restored, "smithy");
    commitDoor(*restored);
    check(drops(restored->game()), "Inactive smithy items survive city and inn travel");
    check(restored->game().actors().snapshot().count == 1 && restored->dialogue().get(host)->introduced &&
              restored->merchants().find(host)->stock[0] == 1,
          "Inactive smithy travel retains Duncan, dialogue and stock");
    Client client(cat, defs);
    auto scenario = generatedScenario(cat);
    scenario["kind"] = "campaign";
    scenario.erase("depth");
    check(client.request("start", {{"scenario", scenario}}).at("ok"), "Wire campaign starts");
    const auto move = [&](const char* direction) {
        check(client.act({{"kind", "move"}, {"direction", direction}}).at("data").at("accepted"),
              "Wire city movement accepted");
        client.advance(100, false);
    };
    move("turn_right");
    for (unsigned i = 0; i < 4; ++i)
        move("forward");
    move("turn_right");
    for (unsigned i = 0; i < 4; ++i)
        move("forward");
    move("turn_left");
    check(client.act({{"kind", "doorway"}}).at("data").at("accepted"), "Wire smithy entry");
    const auto observed = client.protocol.state().at("campaign");
    check(observed.at("location") == 5 && observed.at("depth") == 0 &&
              observed.at("location_name") == "Sanctuary smithy" &&
              observed.at("locations").size() == campaignLocationCount &&
              observed.at("locations")[5].at("fingerprint") == smithyInteriorIdentity,
          "Wire diagnostics distinguish smithy from inn and dungeon depth");
}
} // namespace
int main() try {
    layout();
    campaign();
    std::cout << "Smithy layout, Duncan resident, doorway and persistence checks passed\n";
    return 0;
} catch (const std::exception& e) {
    watching = false;
    allocationsUntilFailure = -1;
    std::cerr << e.what() << '\n';
    return 1;
}
