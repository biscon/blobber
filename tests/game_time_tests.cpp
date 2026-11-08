#include "game/commands.hpp"
#include "persistence/campaign_codec.hpp"
#include "render/outdoor_lighting.hpp"
#include "support/allocations.hpp"
#include "support/campaign_fixtures.hpp"
#include "support/combat_fixtures.hpp"
#include "support/recovery_fixtures.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
using namespace blobber;
namespace {
void calendar() {
    check(std::string_view(GameTime{}.text().data()) == "Day 1 08:00", "Morning origin");
    check(std::string_view(GameTime{16 * gameDuration::hour - 1}.text().data()) == "Day 1 23:59",
          "Minute display floors partial minutes");
    check(std::string_view(GameTime{16 * gameDuration::hour}.text().data()) == "Day 2 00:00",
          "Midnight rolls day");
    check(std::string_view(GameTime{30 * gameDuration::hour}.text().data()) == "Day 2 14:00",
          "Sequential days and 24-hour clock");
    check(std::string_view(GameTime{maxCheckpointClock}.text().data()) == "Day 106751991168 15:12" &&
              std::string_view(GameTime{UINT64_MAX}.text().data()) == "Day 213503982335 22:25",
          "Large supported and uint64 boundary clocks do not overflow");
    const auto deadline = GameTime{42}.after(1000);
    check(!GameTime{1041}.reached(deadline) && GameTime{1042}.reached(deadline) &&
              GameTime{1043}.reached(deadline) && GameTime{UINT64_MAX - 10}.after(11).elapsedMs == UINT64_MAX,
          "Deadlines are inclusive and arithmetic saturates");
    allocations = 0;
    watching = true;
    const auto formatted = GameTime{maxCheckpointClock}.text();
    const auto light = world_render::outdoorLighting(GameTime{10 * gameDuration::hour});
    watching = false;
    check(!allocations && formatted[0] == 'D' && light.ambient[0] > .3f,
          "Clock formatting and lighting sampling allocate no C++ storage");
}
GameTime at(unsigned hour, unsigned minute = 0) {
    return {((hour + 24 - 8) % 24 * 60 + minute) * gameDuration::minute};
}
void lighting() {
    using world_render::outdoorLighting;
    const world_render::OutdoorLighting day;
    check(outdoorLighting(at(7)) == day && outdoorLighting(at(12)) == day && outdoorLighting(at(17)) == day,
          "Daytime is exactly neutral");
    const auto night = outdoorLighting(at(0));
    check(night == outdoorLighting(at(5)) && night == outdoorLighting(at(19)) &&
              night == outdoorLighting(at(23, 59)) && night.ambient == std::array<float, 3>{.3f, .36f, .5f},
          "Night is steady through midnight and transition endpoints");
    for (unsigned minute = 0; minute <= 120; ++minute) {
        const auto dawn = outdoorLighting(at(5 + minute / 60, minute % 60));
        const auto dusk = outdoorLighting(at(19 - (minute + 59) / 60, (60 - minute % 60) % 60));
        for (unsigned c = 0; c < 3; ++c)
            check(std::abs(dawn.ambient[c] - dusk.ambient[c]) < .00001f &&
                      dawn.ambient[c] >= night.ambient[c] && dawn.ambient[c] <= 1,
                  "Dawn and dusk are symmetric and bounded");
        if (minute) {
            const auto prior = outdoorLighting(at(5 + (minute - 1) / 60, (minute - 1) % 60));
            check(dawn.ambient[0] >= prior.ambient[0] && dawn.ambient[0] - prior.ambient[0] < .009,
                  "Minute steps are monotonic and visually small");
        }
    }
    check(outdoorLighting(at(18)) == outdoorLighting(GameTime{at(18).elapsedMs + 59999}),
          "Profiles are stable throughout a game minute");
}
void simulation() {
    const auto cat = recoveryCatalog();
    const auto map = combatMap();
    const Player pose{{3, 3}, Heading::North};
    const auto empty = combatPlacements(map, pose, 0);
    GameSession game(map, cat, empty, 5, pose, false);
    for (unsigned i = 0; i < 4; ++i)
        game.update(.00025, false);
    check(game.gameTime().elapsedMs == 1, "Fractional milliseconds accumulate once");
    game.update(60, true);
    check(game.gameTime().elapsedMs == 1, "Pause freezes clock");
    game.update(std::numeric_limits<double>::quiet_NaN(), false);
    game.update(-1, false);
    check(game.gameTime().elapsedMs == 1, "Invalid deltas add no time");
    check(!game.rest(restRequest(game)) && game.gameTime().elapsedMs == 1,
          "Rejected rest adds no time");
    game.party().damage({0}, 2);
    auto shortRest = restRequest(game, RestKind::Short);
    shortRest.dice[0] = 1;
    check(bool(game.rest(shortRest)) && game.gameTime().elapsedMs == 1 + gameDuration::hour,
          "Short rest advances one hour");
    game.party().damage({0}, 1);
    const auto longRest = restRequest(game);
    check(bool(game.rest(longRest)) && game.gameTime().elapsedMs == 1 + 9 * gameDuration::hour,
          "Long rest advances eight hours");
    check(!game.rest(longRest) && game.gameTime().elapsedMs == 1 + 9 * gameDuration::hour,
          "Repeated rest adds no time");
    CommandController commands(game);
    commands.refresh({}, false, true, true);
    check(commands.snapshot().gameTime.elapsedMs == game.gameTime().elapsedMs,
          "Map HUD snapshots game time");

    // A durable enemy and harmless attacks allow one full round without a kill.
    const auto combatCat = combatCatalog(10000, -100, {1, 4, -100});
    const auto enemies = combatPlacements(map, pose, 1);
    GameSession fight(map, combatCat, enemies, 5, pose, false);
    for (unsigned i = 0; i < 100 && fight.combat().snapshot().phase != CombatPhase::Turns; ++i)
        fight.update(.1, false);
    check(fight.combat().snapshot().phase == CombatPhase::Turns, "Owned fixture reaches turns");
    const auto initial = fight.gameTime().elapsedMs;
    const auto round = fight.combat().snapshot().round;
    fight.update(10, false);
    check(fight.gameTime().elapsedMs == initial, "Thinking in a turn does not advance game time");
    for (unsigned i = 0; i < 200 && fight.combat().snapshot().round == round; ++i) {
        if (fight.combat().snapshot().awaitingAction)
            fight.submit(turnCommand(fight));
        else
            fight.update(.2, false);
    }
    check(fight.gameTime().elapsedMs == initial + gameDuration::round,
          "Full round adds six seconds regardless of animation or thinking time");
}
void campaign(const char* legacy) {
    using namespace blobber::automation::test;
    using namespace blobber::campaign_test;
    const auto cat = catalog();
    const auto defs = definitions();
    CampaignRun run(cat, *cat.preset(), defs.types, defs.timings, 5);
    run.update(37.25, false);
    if (legacy) {
        std::ifstream in(legacy);
        const std::string bytes((std::istreambuf_iterator<char>(in)), {});
        auto state = decodeCampaign(cat, defs, {"time-legacy", 0}, bytes);
        CampaignRun loaded(cat, std::move(*state), defs.timings);
        check(loaded.gameTime().elapsedMs == 37250 && loaded.gameTime().elapsedMs == run.gameTime().elapsedMs,
              "Pre-change codec save restores identical elapsed time");
        check(encodeCampaign(cat, defs, {"time-legacy", 0}, *loaded.checkpoint()) == bytes,
              "Pre-change save remains byte-identical after restore and re-encode");
    }
    const auto before = run.gameTime();
    run.update(5, true);
    check(run.game().gameTime().elapsedMs == before.elapsedMs, "Campaign delegates pause to session");
    enterInn(run);
    check(run.gameTime().elapsedMs == before.elapsedMs && run.game().gameTime().elapsedMs == before.elapsedMs,
          "Travel preserves global time despite fresh local session clock");
    run.party().damage({0}, 1);
    check(bool(run.game().rest(restRequest(run.game()))) &&
              run.gameTime().elapsedMs == before.elapsedMs + gameDuration::longRest,
          "Campaign clock includes rests in another location");
    auto state = run.checkpoint();
    const auto bytes = encodeCampaign(cat, defs, {"time", 0}, *state);
    auto decoded = decodeCampaign(cat, defs, {"time", 0}, bytes);
    CampaignRun loaded(cat, std::move(*decoded), defs.timings);
    check(loaded.gameTime().elapsedMs == run.gameTime().elapsedMs &&
              loaded.gameTime().calendar().hour == 16,
          "Save/load retains accumulated calendar without wall-clock progression");
    leaveInn(run);
    const auto clock = run.gameTime().elapsedMs;
    entrance(run);
    const auto afterWalk = run.gameTime().elapsedMs;
    { const auto cancelled = run.prepareTravel(run.request(run.game().nearbyFixture())); }
    check(run.gameTime().elapsedMs == afterWalk && afterWalk >= clock && run.activeLocation() == 0,
          "Cancelled preparation adds no time and never commits a location");
    travel(run, FixtureKind::Downstairs);
    check(run.gameTime().elapsedMs == afterWalk && run.game().gameTime().elapsedMs == afterWalk,
          "Global time remains continuous on dungeon entry");
}
} // namespace
int main(int argc, char** argv) try {
    calendar();
    lighting();
    simulation();
    campaign(argc == 2 ? argv[1] : nullptr);
    std::cout << "Game time checks passed\n";
} catch (const std::exception& e) {
    watching = false;
    std::cerr << e.what() << '\n';
    return 1;
}
