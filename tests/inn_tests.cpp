#include "game/commands.hpp"
#include "game/inn_interior.hpp"
#include "persistence/campaign_codec.hpp"
#include "support/allocations.hpp"
#include "support/campaign_fixtures.hpp"
#include "world/sanctuary_city.hpp"
#include <climits>
#include <iostream>
using namespace blobber;
using namespace blobber::campaign_test;
int main() {
    try {
        const auto cat = catalog();
        const auto defs = definitions();
        CampaignRun run(cat, *cat.preset(), defs.types, defs.timings, 5);
        const auto approach = sanctuaryCity().doorway("inn").approach;
        check(!run.previewTravel(run.doorwayRequest()), "No remote inn entry");
        walk(run.game(), approach.cell);
        face(run.game(), Heading::West);
        check(!run.previewTravel(run.doorwayRequest()), "Wrong facing rejected");
        face(run.game(), approach.heading);
        auto request = run.doorwayRequest();
        check(bool(run.previewTravel(request)), "Facing inn admits entry");
        auto foreign = request;
        ++foreign.doorway.world;
        check(!run.previewTravel(foreign), "Foreign doorway rejected");
        CommandController controls(run.game(), defs.timings);
        controls.refresh({}, false, false, true);
        check(std::string_view(controls.snapshot().fixturePrompt.data()) == "[F] Enter inn",
              "F entry prompt");
        controls.interactFixture();
        check(controls.snapshot().travelRequested &&
                  controls.snapshot().travelRequested->kind == TravelKind::Doorway,
              "F dispatches doorway travel");
        const auto original = run.game().movement().player;
        auto irrelevantDepth = request;
        irrelevantDepth.depth = UINT_MAX;
        {
            auto prepared = run.prepareTravel(irrelevantDepth);
            check(prepared.destination() == unsigned(CampaignLocationId::Inn),
                  "Doorway ignores stair depth field");
        }
        bool allocationFailed = false;
        allocationsUntilFailure = 0;
        try {
            auto failed = run.prepareTravel(request);
        } catch (const std::bad_alloc&) {
            allocationFailed = true;
        }
        allocationsUntilFailure = -1;
        check(allocationFailed && run.game().movement().player == original,
              "Failed preparation preserves source");
        {
            auto canceled = run.prepareTravel(request);
        }
        check(run.activeLocation() == 0 && run.game().movement().player == original,
              "Canceled preparation keeps city pose");
        allocations = 0;
        watching = true;
        for (unsigned i = 0; i < 64; ++i)
            (void)run.previewTravel(run.doorwayRequest());
        watching = false;
        check(allocations == 0, "Doorway frame queries allocate no storage");
        auto ticket = run.prepareTravel(request);
        check(run.commitTravel(ticket), "Inn entry commits");
        check(run.activeLocationId() == CampaignLocationId::Inn && run.dungeonDepth() == 0,
              "Inn is not dungeon depth four");
        check(!run.previewTravel(request), "Old city request rejected after entry");
        check(!run.previewTravel(run.recallRequest()), "No paid recall from inn");
        check(!run.previewTravel(run.waypointRequest(1)), "No dungeon shortcut from inn");
        check(run.game().actors().snapshot().count == 1 &&
                  !run.game().actors().snapshot().actors[0].hostile() &&
                  run.game().actors().snapshot().actors[0].npc.cell == Cell{6, 3} &&
                  run.game().actors().snapshot().actors[0].npc.subcell == Cell{1, 1} &&
                  run.game().actors().snapshot().actors[0].npc.facing == Heading::South,
              "One peaceful innkeeper occupies the southern half-subcell nearest the counter");
        {
            CommandController innControls(run.game(), defs.timings);
            innControls.refresh({}, false, false, true);
            check(!innControls.snapshot().fixturePrompt[0], "Free inn rest has no persistent prompt");
            check(innControls.snapshot().restAvailable, "Inn rest icon remains enabled");
            innControls.openCamp();
            innControls.campKind(RestKind::Long);
            check(innControls.snapshot().camp.open, "Rest control still opens camp inside inn");
        }
        const auto& map = run.map();
        for (const auto& scenery : map.definition().scenery)
            if (scenery.model == "table-group") {
                check(map.movementBlocked(scenery.anchor), "Seating center blocks movement");
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        if (dx || dy)
                            check(!map.movementBlocked(scenery.anchor + Cell{dx, dy}),
                                  "All eight cells around seating remain walkable");
            }
        for (int y = 1; y < map.height() - 1; ++y)
            for (int x = 1; x < map.width() - 1; ++x)
                if (!map.movementBlocked({x, y}) && !run.game().actors().occupied({x, y})) {
                    walk(run.game(), {x, y});
                    check(run.game().atSanctuary(), "Every reachable inn cell grants free rest");
                }
        while (run.party().campSupplies().supply)
            check(run.party().consumeItem(run.party().campSupplies().supply), "Consume owned test supplies");
        const auto kits = run.party().campSupplies().kits;
        run.party().damage({0}, 1);
        RestRequest rest;
        rest.kind = RestKind::Long;
        rest.world = run.game().actors().snapshot().world;
        rest.token = run.game().restToken();
        check(run.game().previewRest(rest).sanctuary && bool(run.game().rest(rest)),
              "Free inn rest succeeds");
        check(run.party().campSupplies().kits == kits, "Inn rest consumes no kit");
        walk(run.game(), {5, 7});
        ItemHandle item;
        for (std::uint16_t b = 0; b < bagCells && !item; ++b) {
            const auto candidate = run.party().inventory().at({{0}, b});
            if (run.party().inventory().stack(candidate))
                item = candidate;
        }
        check(bool(item), "Owned drop fixture has gear");
        const auto dropped = *run.party().inventory().stack(item);
        check(bool(run.game().dropItem(item, run.party().inventory().revision())), "Drop item inside inn");
        const auto hasDropped = [&](const GameSession& game) {
            for (const auto& stack : game.loot().snapshot().stacks)
                if (stack.active && stack.cell == Cell{5, 7} && stack.item.index == dropped.item.index &&
                    stack.quantity == dropped.quantity)
                    return true;
            return false;
        };
        const auto pose = run.game().movement().player;
        run.dialogue().get(dialogueKey("sanctuary.innkeeper"), true)->introduced = true;
        check(run.dialogue().complete(dialogueKey("sanctuary.innkeeper"), dialogueKey("dungeon"),
                                      dialogueKey("heard_dungeon_advice")),
              "Dialogue history records fact before save");
        auto saved = run.checkpoint();
        const auto bytes = encodeCampaign(cat, defs, {"fixture", 0}, *saved);
        saved = decodeCampaign(cat, defs, {"fixture", 0}, bytes);
        CampaignRun restored(cat, std::move(*saved), defs.timings);
        check(restored.activeLocationId() == CampaignLocationId::Inn &&
                  restored.game().movement().player == pose,
              "Continue restores inn and pose");
        check(hasDropped(restored.game()), "Inn dropped items survive disk roundtrip");
        check(restored.dialogue().get(dialogueKey("sanctuary.innkeeper")) &&
                  restored.dialogue().get(dialogueKey("sanctuary.innkeeper"))->introduced &&
                  restored.dialogue()
                      .get(dialogueKey("sanctuary.innkeeper"))
                      ->knows(dialogueKey("heard_dungeon_advice")),
              "Dialogue introductions/facts survive save Continue");
        check(restored.game().actors().snapshot().count == 1 &&
                  !restored.game().actors().snapshot().actors[0].hostile() &&
                  restored.game().actors().snapshot().actors[0].npc.cell == Cell{6, 3},
              "Continue preserves the peaceful innkeeper's canonical placement");
        for (unsigned fault = 0; fault < 6; ++fault) {
            auto bad = restored.checkpoint();
            auto& actor = bad->locations[4].state->actors.actors[0];
            if (fault == 0)
                actor.npc.role = NpcRole::Hostile;
            if (fault == 1)
                actor.npc.cell = {5, 3};
            if (fault == 2)
                actor.hitPoints = 1;
            if (fault == 3)
                actor.hasAttack = true;
            if (fault == 4)
                bad->locations[4].state->creditedDeaths[0] = true;
            if (fault == 5)
                bad->locations[4].placements.clear();
            rejects([&] { CampaignRun invalid(cat, std::move(*bad), defs.timings); },
                    "Forged resident role, body, combat state or authored placement rejects");
        }
        for (const auto oldCeiling : {6, 8}) {
            auto previous = restored.checkpoint();
            auto& inn = previous->locations[4];
            for (auto& surface : inn.world.surfaces)
                surface.ceiling = oldCeiling;
            for (auto& scenery : inn.world.scenery)
                if (scenery.model == "table-group")
                    scenery.width = scenery.depth = 2;
            inn.worldFingerprint = inn.world.fingerprint();
            const auto oldBytes = encodeCampaign(cat, defs, {"fixture", 0}, *previous);
            auto decoded = decodeCampaign(cat, defs, {"fixture", 0}, oldBytes);
            CampaignRun upgraded(cat, std::move(*decoded), defs.timings);
            check(upgraded.map().surface(pose.cell).ceiling == 6 &&
                      upgraded.game().movement().player == pose && hasDropped(upgraded.game()),
                  "Previous inn layout loads at current height with pose and loot preserved");
            check(!upgraded.map().movementBlocked({4, 6}) && !upgraded.map().movementBlocked({8, 8}),
                  "Previous saves adopt smaller seating footprints");
        }
        {
            auto malformed = restored.checkpoint();
            malformed->locations[4].world.surfaces[0].ceiling = 8;
            malformed->locations[4].worldFingerprint = malformed->locations[4].world.fingerprint();
            bool mixedRejected = false;
            try {
                CampaignRun invalid(cat, std::move(*malformed), defs.timings);
            } catch (const std::invalid_argument&) {
                mixedRejected = true;
            }
            check(mixedRejected, "Compatibility does not accept arbitrary ceiling changes");
            malformed = restored.checkpoint();
            malformed->locations[4].world.scenery[2].width = 2;
            malformed->locations[4].worldFingerprint = malformed->locations[4].world.fingerprint();
            mixedRejected = false;
            try {
                CampaignRun invalid(cat, std::move(*malformed), defs.timings);
            } catch (const std::invalid_argument&) {
                mixedRejected = true;
            }
            check(mixedRejected, "Compatibility does not accept mixed seating footprints");
        }
        check(restored.checkpoint()->locations[4].state->fixtures.discovered ==
                  run.checkpoint()->locations[4].state->fixtures.discovered,
              "Inn discovery survives disk roundtrip");
        auto bad = restored.checkpoint();
        bad->locations[4].recovery.freeRestEverywhere = false;
        bool rejected = false;
        try {
            CampaignRun invalid(cat, std::move(*bad), defs.timings);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "Changed canonical inn rejected");
        const auto exit = restored.location(4).exit;
        walk(restored.game(), exit.cell);
        face(restored.game(), exit.heading);
        {
            CommandController exitControls(restored.game(), defs.timings);
            exitControls.refresh({}, false, false, true);
            check(std::string_view(exitControls.snapshot().fixturePrompt.data()) == "[F] Leave inn",
                  "Exit keeps only its interaction prompt");
        }
        auto leaving = restored.prepareTravel(restored.doorwayRequest());
        check(restored.commitTravel(leaving), "Exit commits");
        check(restored.game().movement().player.cell == approach.cell &&
                  restored.game().movement().player.heading == Heading::West,
              "Exit faces street");
        walk(restored.game(), restored.location(0).entry.cell);
        check(!restored.game().atSanctuary(), "Old city rest cell removed");
        restored.party().damage({0}, 1);
        rest.world = restored.game().actors().snapshot().world;
        rest.token = restored.game().restToken();
        check(restored.game().previewRest(rest).error == RestError::NoSupplies,
              "Outdoor sanctuary cannot grant free long rest with zero kits");
        enterInn(restored);
        check(hasDropped(restored.game()), "Inn dropped items survive inactive travel snapshot");
        check(restored.dialogue().get(dialogueKey("sanctuary.innkeeper"))->introduced,
              "Dialogue history survives location travel");
        // Exercise the public wire action and transition events through real movement.
        Client client(cat, defs);
        auto scenario = generatedScenario(cat);
        scenario["kind"] = "campaign";
        scenario.erase("depth");
        check(client.request("start", {{"scenario", scenario}}).at("ok"), "Wire campaign starts");
        const auto move = [&](const char* direction) {
            check(client.act({{"kind", "move"}, {"direction", direction}}).at("data").at("accepted"),
                  "Wire movement accepted");
            client.advance(100, false);
        };
        move("turn_right");
        for (unsigned i = 0; i < 5; ++i)
            move("forward");
        move("turn_left");
        for (unsigned i = 0; i < 3; ++i)
            move("forward");
        move("turn_right");
        check(client.protocol.state().at("campaign").at("travel").at("doorway").at("ready"),
              "Wire doorway descriptor reflects facing");
        check(client.act({{"kind", "doorway"}}).at("data").at("accepted"), "Wire inn entry");
        check(client.protocol.state().at("residents").size() == 1 &&
                  client.protocol.state().at("residents")[0].at("type") == "innkeeper" &&
                  !client.protocol.state().at("residents")[0].at("targetable").get<bool>() &&
                  client.protocol.state().at("enemies").empty() &&
                  client.protocol.state().at("targets").empty(),
              "Wire observation reports peaceful resident separately from enemies and targets");
        const auto observation = client.protocol.state().at("campaign");
        check(observation.at("location") == 4 && observation.at("depth") == 0 &&
                  observation.at("locations").size() == campaignLocationCount,
              "Wire location differs from depth");
        move("turn_right");
        move("turn_right");
        check(client.act({{"kind", "doorway"}}).at("data").at("accepted"), "Wire inn exit");
        unsigned transitions = 0;
        for (const auto& event : client.sink.events) {
            check(event.at("event") != "entry_unlocked", "Inn does not unlock a dungeon entry");
            if (event.at("event") == "campaign_transition") {
                check(event.at("kind") == "doorway" && event.at("kit_charge") == 0,
                      "Wire transitions report free doorway travel");
                ++transitions;
            }
        }
        check(transitions == 2, "One transition event per doorway action");
        std::cout << "Inn travel, rest, authored layout and checkpoints passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
