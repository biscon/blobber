#include "app/gameplay.hpp"
#include "support/campaign_fixtures.hpp"
#include "support/combat_fixtures.hpp"
#include <iostream>
using namespace blobber;
namespace {
void restoredCampaignAtOriginalSpawn() {
    const auto catalog = automation::test::catalog();
    const auto definitions = automation::test::definitions();
    const CampaignContent content{"owned-startup", 0};
    CampaignRun original(catalog, *catalog.preset(), definitions.types, definitions.timings, 5);
    auto travel = original.prepareTravel(original.request(original.game().nearbyFixture()));
    check(original.commitTravel(travel), "Startup fixture enters dungeon");
    auto& game = original.game();
    game.setAnimated(false);
    const auto& location = original.location(original.activeLocation());
    const auto oldSpawn = location.placements->instances().front().cell;
    for (unsigned i = 0; i < game.actors().snapshot().count; ++i)
        game.actors().damage(game.actors().id(i), 10000, game.timeMs());
    original.update(.01, false);
    game.restorePose({oldSpawn, Heading::West}, false);
    original.update(.01, false);
    if (const auto* error = original.saveSafety())
        throw std::runtime_error(std::string("Old NPC spawn is not save-safe: ") + error);
    const auto bytes = encodeCampaign(catalog, definitions, content, *original.checkpoint());
    auto checkpoint = decodeCampaign(catalog, definitions, content, bytes);
    CampaignRun restored(catalog, std::move(*checkpoint), definitions.timings);
    const auto& current = restored.location(restored.activeLocation());
    const auto pose = restored.game().movement().player;
    check(pose.cell == oldSpawn && current.placements->occupied(pose.cell) &&
              !restored.game().actors().occupied(pose.cell),
          "Restored party legitimately occupies an original spawn, not a live actor cell");
    rejects([&] { gameplayPlacements(restored.map(), *current.placements, pose.cell); },
            "Rebuilding initial NPC placements at the saved pose reproduces the reported failure");
    const auto prepared = gameplayPlacements(restored.map(), *current.placements, pose.cell, &restored);
    check(!prepared, "Graphical campaign startup uses the restored session without a placement rebuild");
    check(encodeCampaign(catalog, definitions, content, *restored.checkpoint()) == bytes,
          "Startup preserves saved pose, actors, loot, party, RNG and clocks exactly");
}
void standalonePlacements() {
    const auto map = combatMap();
    const Cell spawn{3, 3};
    NpcInstance actor{{0}, {3, 2}, {0, 1}, Heading::South};
    actor.animation = NpcAnimation::Walk;
    const NpcWorld source(map, spawn, {"goblin"}, {actor});
    const auto idle = gameplayPlacements(map, source, spawn);
    check(idle && idle->instances().size() == 1 && idle->instances()[0].animation == NpcAnimation::Idle &&
              idle->instances()[0].cell == actor.cell &&
              source.instances()[0].animation == NpcAnimation::Walk,
          "Standalone startup normalizes a copy of validated initial placements");
    rejects([&] { gameplayPlacements(map, source, actor.cell); },
            "Standalone party/NPC overlap still rejects");
}
} // namespace
int main() {
    try {
        restoredCampaignAtOriginalSpawn();
        standalonePlacements();
        std::cout << "Restored campaign and standalone graphical startup checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
