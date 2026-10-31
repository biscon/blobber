#pragma once
#include "app/loading.hpp"
#include "assets/image.hpp"
#include "game/campaign.hpp"
#include "platform/game_view.hpp"
#include "support/check.hpp"
#include "world/environment_maps.hpp"
#include <iostream>
namespace blobber::test {
inline void dungeonArtReview(const std::filesystem::path& source, const std::filesystem::path& output,
                             const rules::Catalog& rules) {
    PreparationWorker worker;
    GpuResources resources;
    LoadingScreen loading(&worker);
    Presenter presenter(resources, source, &worker);
    loading.attach(presenter);
    const auto definitions = prepareCpu([&] { return loadNpcDefinitions(source / "npcs/level.json"); });
    WorldRenderer renderer(resources,
                           prepareCpu([&] { return world_assets::load(source / "prepared/world"); }));
    std::cout << "Production loading presentations=" << loading.presentations()
              << " longest gap ms=" << loading.longestGapSeconds() * 1000 << "\n";
    auto capture = [&](GameSession& game, const Map& map, const char* name, float fade = 0) {
        NpcAnimator animation(game.actors(), definitions.timings);
        animation.setTime(game.timeMs());
        GameView view(renderer);
        view.update(game, animation.snapshot(), {});
        CommandController commands(game, definitions.timings);
        commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, game.actionBusy(),
                         false, true);
        PreparationScope noLoading({});
        presenter.fade(fade);
        view.present(presenter, game, map, false, true, &commands.snapshot());
        presenter.capture(output / (std::string(name) + ".png"));
        if (std::string(name).find("stairs-") == 0) {
            OwnedImage image(LoadImageFromTexture(resources.target(renderer.image().target).texture));
            ImageFlipVertical(&image.value);
            check(ExportImage(image.value, (output / (std::string(name) + "-world.png")).string().c_str()),
                  "Export staircase review");
        }
    };
    const Map corridor(
        {"#########", "#...#...#", "#...#...#", "#.......#", "#...#...#", "#...#...#", "#########"});
    NpcWorld empty(corridor, {5, 3}, definitions.types, {});
    FloorFixtures door;
    door.count = 1;
    door.entries[0] = {{4, 3}, Heading::East, FixtureKind::Door};
    GameSession game(corridor, rules, empty, 5, {{5, 3}, Heading::West}, false, definitions.timings, {},
                     door);
    capture(game, corridor, "door-closed");
    check(game.interact({game.fixtures().id(0), game.fixtures().revision()}).changed, "Review opens door");
    game.update(.15, false);
    capture(game, corridor, "door-opening");
    game.update(.15, false);
    capture(game, corridor, "door-open");
    check(game.interact({game.fixtures().id(0), game.fixtures().revision()}).changed, "Review closes door");
    game.update(.15, false);
    capture(game, corridor, "door-closing");
    game.update(.15, false);
    capture(game, corridor, "door-closed-again");
    const Map stairs({"#########", "#########", "#########", "####.####", "####.####", "####.####",
                      "###...###", "###...###", "#########"});
    for (auto kind : {FixtureKind::Upstairs, FixtureKind::Downstairs}) {
        NpcWorld none(stairs, {4, 4}, definitions.types, {});
        FloorFixtures f;
        f.count = 1;
        f.entries[0] = {{4, 3}, Heading::North, kind};
        GameSession steps(stairs, rules, none, 5, {{4, 4}, Heading::North}, false, definitions.timings, {},
                          f);
        capture(steps, stairs, kind == FixtureKind::Upstairs ? "stairs-up" : "stairs-down");
    }
    const auto hub = sanctuaryMap();
    NpcWorld noHostiles(hub, {3, 4}, definitions.types, {});
    for (const auto& pose :
         {Player{{3, 4}, Heading::North}, Player{{2, 2}, Heading::West}, Player{{6, 2}, Heading::East}}) {
        GameSession courtyard(hub, rules, noHostiles, 5, pose, false, definitions.timings);
        capture(courtyard, hub,
                pose.heading == Heading::North  ? "hub-sky"
                : pose.heading == Heading::West ? "hub-crest"
                                                : "hub-vine");
    }
    CampaignRun campaign(rules, *rules.preset(), definitions.types, definitions.timings, 5);
    capture(campaign.game(), campaign.map(), "campaign-stairs-down");
    auto ticket =
        prepareCpu([&] { return campaign.prepareTravel(campaign.request(campaign.game().nearbyFixture())); });
    check(campaign.commitTravel(ticket), "Review campaign uses production stairs travel");
    for (unsigned i = 0; i < campaign.game().fixtures().definition().count; ++i) {
        const auto& f = campaign.game().fixtures().definition().entries[i];
        if (f.kind == FixtureKind::Upstairs)
            campaign.game().restorePose({stairLanding(f), f.facing}, true);
    }
    campaign.game().update(0, false); // Reveal the faced stairs after the diagnostic pose restore.
    capture(campaign.game(), campaign.map(), "campaign-stairs-up");
    capture(game, corridor, "travel-fade-half", .5f);
    capture(game, corridor, "travel-black", 1);
    presenter.fade(0);
}
} // namespace blobber::test
