#include "app/distribution_smoke.hpp"
#include "support/campaign_fixtures.hpp"
#include "world/sanctuary_city.hpp"
#include <iostream>
using namespace blobber;
int main() try {
    const auto catalog = automation::test::catalog();
    const auto definitions = automation::test::definitions();
    CampaignRun run(test::campaignBlueprints(), catalog, *catalog.preset(), definitions.types,
                    definitions.timings, 5);
    auto scenario = defaultArena(catalog);
    check(scenario.adjust(catalog.findMonster("goblin"), 1) && !arenaError(catalog, scenario),
          "Smoke scenario fixture includes a valid monster roster");
    const auto initial = distributionSmokeState(run);
    requireDistributionSmokeState(initial, run);
    auto wrong = initial;
    wrong["seed"] = 99;
    rejects([&] { requireDistributionSmokeState(wrong, run); }, "Continue rejects another seed/run");
    unsigned actions = 0;
    const auto door = sanctuaryCity().doorway("inn").approach;
    distributionSmokeWalk(run.game(), door, actions);
    check(run.game().movement().player == door && actions > 0, "Smoke follows legal discrete city movements");
    rejects([&] { requireDistributionSmokeState(initial, run); }, "Continue detects a changed pose");
    auto ticket = run.prepareTravel(run.doorwayRequest());
    check(run.commitTravel(ticket), "Smoke navigation reaches a usable inn doorway");
    const auto exit = run.location(unsigned(CampaignLocationId::Inn)).exit;
    distributionSmokeWalk(run.game(), exit, actions);
    auto back = run.prepareTravel(run.doorwayRequest());
    check(run.commitTravel(back), "Smoke interior route reaches the real exit");
    rejects([&] { distributionSmokeWalk(run.game(), {{-1, 0}, Heading::North}, actions); },
            "Out-of-map navigation fails");
    actions = 4096;
    rejects([&] { distributionSmokeWalk(run.game(), door, actions); }, "Navigation respects action budget");
    std::cout << "Distribution smoke movement, restore and bounds checks passed\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
