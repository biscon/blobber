#pragma once
#include "campaign_fixtures.hpp"
#include "persistence/campaign_store.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>

namespace blobber::campaign_test {
// Shorten walks and ordinary fights. Travel, rewards, loot, admission and publication
// still run through their campaign owners.
inline TravelRequest faceStair(CampaignRun& run, FixtureKind kind) {
    auto& game = run.game();
    game.setAnimated(false);
    auto request = stair(game, kind);
    const auto f = *game.fixtures().find(request.id);
    game.restorePose({stairLanding(f), f.facing}, false);
    game.discover();
    return run.request(stair(game, kind));
}
inline void stepFloor(CampaignRun& run, FixtureKind kind) {
    auto ticket = run.prepareTravel(faceStair(run, kind));
    check(run.commitTravel(ticket), "Campaign stair transfer");
}
inline void clearOrdinaryRoute(CampaignRun& run) {
    auto& game = run.game();
    game.setAnimated(false);
    const auto& layout = run.location(run.activeLocation()).generated->layout;
    for (unsigned i = 0; i < layout.encounterCount; ++i) {
        const auto& g = layout.encounters[i];
        if (g.role == generation::EncounterRole::Optional)
            continue;
        for (unsigned slot = g.first; slot < g.first + g.count; ++slot)
            if (game.actors().snapshot().actors[slot].npc.vault == VaultPart::None)
                game.actors().damage(game.actors().id(slot), 10000, game.timeMs());
        game.update(0, false);
    }
}
inline void checkSkippedFloors(const CampaignRun& run) {
    for (unsigned depth = 1; depth < run.dungeonDepth(); ++depth) {
        const auto& location = run.location(floorLocation(depth));
        const auto& layout = location.generated->layout;
        const auto& state = *location.saved;
        check(location.visited && state.objective.completed, "Skipped guardian remains cleared");
        for (unsigned i = 0; i < layout.encounterCount; ++i) {
            const auto& group = layout.encounters[i];
            for (unsigned n = group.first; n < group.first + group.count; ++n)
                check(state.creditedDeaths[n] == (group.role != generation::EncounterRole::Optional),
                      "Skipped floors credit only the ordinary route");
        }
        for (unsigned i = 0; i < location.recovery.groundCount + location.objective.rewardCount; ++i) {
            const auto& stack = state.loot.state.stacks[NpcWorld::maxInstances * lootPerActor + i];
            const bool optional = depth <= 3
                                      ? stack.container == 1
                                      : i < location.recovery.groundCount && !(stack.cell == layout.supply);
            check(stack.active == optional, "Backtracking retains only optional fixed supplies");
        }
    }
}
inline void dampCampaignRoute() {
    const auto c = catalog(false, 50, 7, false, false, true);
    const auto d = definitions();
    const auto& blueprints = blobber::test::campaignBlueprints();
    auto roster = *c.preset();
    roster.characters[1].id = "saved-surveyor";
    roster.characters[1].name = "Ida";
    std::swap(roster.characters[0], roster.characters[1]);
    const CampaignContent content{"dc07-fixture", 0, &blueprints};
    Directory directory;
    CampaignDisk disk(directory.path / "campaign.json");
    CampaignStore store(c, d, content, disk);
    const auto start = std::chrono::steady_clock::now();
    auto run = std::make_unique<CampaignRun>(blueprints, c, roster, d.types, d.timings, 5, 4);
    auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start);
    checkSkippedFloors(*run);
    const auto seepEntryKits = run->party().campSupplies().kits;
    const auto member = [&]() -> const auto& { return *run->party().character({0}); };
    check(member().build.id == "saved-surveyor" && member().build.name == "Ida" &&
              member().build.level == 2 && member().experience == 675 && !member().experienceFraction,
          "Selected roster and actual ordinary-route XP at the Seep entrance");
    const auto camp = std::uint16_t(NpcWorld::maxInstances * lootPerActor);
    auto& game = run->game();
    const auto loot = game.loot().snapshot().stacks[camp];
    game.setAnimated(false);
    game.restorePose({loot.cell, Heading::North}, false);
    check(bool(game.pickup({{game.actors().snapshot().world, camp, loot.generation},
                            {0},
                            run->party().inventory().revision()})),
          "Collect the camp antitoxin");
    game.restorePose(run->location(run->activeLocation()).entry, false);
    game.discover();
    check(store.save(*run, "missing").durable, "Save a settled cave");
    const auto bytes = std::filesystem::file_size(directory.path / "campaign.json");
    check(bytes < 256 * 1024, "Representative late start fits the compact save budget");
    std::cout << "DC07 preparation_ms=" << elapsed.count() << " snapshot_bytes=" << run->snapshotBytes()
              << " seep_save_bytes=" << bytes << '\n';
    run = store.load();
    check(member().build.id == "saved-surveyor" && member().experience == 675 &&
              !run->game().loot().snapshot().stacks[camp].active,
          "Continue retains roster and looted camp");
    const auto kits = run->party().campSupplies().kits;
    auto recall = run->prepareTravel(run->recallRequest());
    check(run->commitTravel(recall) && run->party().campSupplies().kits + 1 == kits, "Paid cave recall");
    for (const auto name : {"inn", "smithy", "emporium"}) {
        auto& city = run->game();
        city.restorePose(sanctuaryCity().doorway(name).approach, false);
        auto enter = run->prepareTravel(run->doorwayRequest());
        check(run->commitTravel(enter) && interiorLocation(run->activeLocationId()) && !run->dungeonDepth(),
              "Town doorway remains a distinct interior");
        run->game().restorePose(run->location(run->activeLocation()).exit, false);
        auto leave = run->prepareTravel(run->doorwayRequest());
        check(run->commitTravel(leave) && run->activeLocation() == 0, "Town interior exits to Sanctuary");
    }
    faceStair(*run, FixtureKind::Downstairs);
    check(!run->previewTravel(run->waypointRequest(5)), "Unknown cave waypoint rejected");
    auto warp = run->prepareTravel(run->waypointRequest(4));
    check(run->commitTravel(warp) && run->dungeonDepth() == 4, "Free return to cave entrance");
    stepFloor(*run, FixtureKind::Upstairs);
    check(run->dungeonDepth() == 3 && run->game().objective().completed, "Prior commander remains cleared");
    stepFloor(*run, FixtureKind::Downstairs);
    clearOrdinaryRoute(*run);
    check(member().experience == 850 && run->game().objective().completed, "One Seep ordinary-route payout");
    stepFloor(*run, FixtureKind::Downstairs);
    stepFloor(*run, FixtureKind::Upstairs);
    check(run->dungeonDepth() == 4 && !run->game().loot().snapshot().stacks[camp].active &&
              member().experience == 850,
          "Cleared gate is a valid return landing; finite loot and XP");
    stepFloor(*run, FixtureKind::Downstairs);
    clearOrdinaryRoute(*run);
    check(member().experience == 1162, "Same party earns Galleries XP");
    stepFloor(*run, FixtureKind::Downstairs);
    stepFloor(*run, FixtureKind::Upstairs);
    stepFloor(*run, FixtureKind::Downstairs);
    const auto vaultCheckpoint = run->checkpoint();
    check(std::all_of(vaultCheckpoint->locations.begin(), vaultCheckpoint->locations.end(),
                      [](const auto& location) { return location.visited; }),
          "All six dungeon floors and four town locations have been visited");
    const auto fullSave = encodeCampaign(c, d, content, *vaultCheckpoint);
    std::cout << "DC07 ten_location_save_bytes=" << fullSave.size() << '\n';
    check(fullSave.size() < 256 * 1024, "All visited locations fit the compact fixture save budget");
    clearOrdinaryRoute(*run);
    auto& vault = run->game();
    const auto mother = vault.combat().vaultMother();
    check(member().experience == 1475 && run->status() == CampaignStatus::Active, "Vault remains active");
    for (unsigned n = 0; n < vault.actors().snapshot().count; ++n)
        if (vault.actors().snapshot().actors[n].npc.sac())
            vault.actors().damage(vault.actors().id(n), 10000, vault.timeMs());
    vault.actors().damage(mother, 10000, vault.timeMs());
    run->update(0, false);
    run->update(1, false);
    check(member().experience == 1525 && member().experienceFraction == 0 && run->status() == CampaignStatus::Victory && run->saveSafety(),
          "Mother pays 200 party XP exactly once and ends the campaign");
    check(store.terminal(*run).durable && store.inspect().terminal, "Saved run receives its terminal marker");

    auto commander = std::make_unique<CampaignRun>(blueprints, c, roster, d.types, d.timings, 5, 3);
    check(!commander->previewTravel(faceStair(*commander, FixtureKind::Downstairs)),
          "Even a party at the downstairs cannot descend past living commanders");
    clearOrdinaryRoute(*commander);
    stepFloor(*commander, FixtureKind::Downstairs);
    check(commander->dungeonDepth() == 4, "Commander unlocks the Seep");
    auto final = std::make_unique<CampaignRun>(blueprints, c, roster, d.types, d.timings, 5, 6);
    checkSkippedFloors(*final);
    check(final->party().campSupplies().kits == seepEntryKits + 2,
          "Direct Vault start carries exactly one kit from each preceding cave guardian");
    check(final->party().character({0})->build.level == 3 &&
              final->party().character({0})->experience == 1162,
          "Direct Vault start uses the ordinary entry budget");
    // Construct a settled previously hatched young. Its generation must survive disk
    // transport and the ordinary mother-first victory check.
    auto checkpoint = final->checkpoint();
    auto& floor = *checkpoint->locations[floorLocation(6)].state;
    unsigned child = 0;
    while (child < floor.actors.count && floor.actors.actors[child].npc.vault != VaultPart::WestYoung)
        ++child;
    check(child < floor.actors.count, "West young reservation");
    floor.actors.actors[child].life = ActorLife::Alive;
    floor.actors.actors[child].generation = 2;
    floor.objective.members[floor.objective.count++] = {std::uint16_t(child), 0, 2};
    const auto text = encodeCampaign(c, d, content, *checkpoint);
    auto stale = decodeCampaign(c, d, content, text);
    stale->locations[floorLocation(6)].state->objective.members[1].generation = 1;
    bool rejected = false;
    try {
        CampaignRun bad(blueprints, c, std::move(*stale), d.timings);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "A stale young incarnation cannot complete the objective");
    auto loaded = decodeCampaign(c, d, content, text);
    final = std::make_unique<CampaignRun>(blueprints, c, std::move(*loaded), d.timings);
    auto& end = final->game();
    check(end.actors().id(child).generation == 2, "Young incarnation survives restore");
    end.actors().damage(end.combat().vaultMother(), 10000, 0);
    final->update(0, false);
    check(final->status() == CampaignStatus::Active, "Surviving young delays mother-first victory");
    check(final->party().character({0})->experience == 1212 &&
              final->party().character({0})->experienceFraction == 6, "Mother death credits XP immediately");
    auto between = decodeCampaign(c, d, content, encodeCampaign(c, d, content, *final->checkpoint()));
    final = std::make_unique<CampaignRun>(blueprints, c, std::move(*between), d.timings);
    final->game().actors().damage(final->game().actors().id(child), 10000, 0);
    final->update(0, false);
    check(final->status() == CampaignStatus::Victory && final->party().character({0})->experience == 1212,
          "Direct Vault run uses the same final victory");
}
} // namespace blobber::campaign_test
