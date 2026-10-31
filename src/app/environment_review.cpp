#include "app/environment_review.hpp"
#include "assets/image.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "game/commands.hpp"
#include "platform/game_view.hpp"
#include "platform/input.hpp"
#include "platform/window.hpp"
#include "ui/interaction.hpp"
#include "world/environment_maps.hpp"
#include <fstream>
#include <iostream>
namespace blobber {
namespace {
RecoveryLevel reviewLoot(const Map& map, const rules::Catalog& catalog) {
    RecoveryLevel r;
    if (!map.flat()) {
        r.groundCount = 1;
        r.ground[0] = {{3, 3},
                       catalog.item(catalog.findItem("dagger")) ? catalog.findItem("dagger")
                                                                : rules::ItemId{0},
                       1};
    }
    return r;
}
struct Review {
    const Map& map;
    NpcWorld placements;
    GameSession game;
    NpcAnimator animator;
    CommandController commands;
    Review(const Map& m, const rules::Catalog& c, const NpcDefinitions& d, bool actors = false)
        : map(m), placements(m, {3, 5}, d.types,
                             actors ? std::vector<NpcInstance>{{{0}, {3, 2}, {0, 0}, Heading::South}}
                                    : std::vector<NpcInstance>{}),
          game(m, c, placements, 5, {{3, 5}, Heading::North}, false, d.timings, reviewLoot(m, c),
               m.flat() ? FloorFixtures{} : rendererHeightFixtures()),
          animator(game.actors(), d.timings), commands(game, d.timings) {}
};
} // namespace
int runEnvironmentReview(const Options& o) {
    const bool capture = !o.captureDirectory.empty();
    if (capture) {
        if (std::filesystem::exists(o.captureDirectory) && !std::filesystem::is_empty(o.captureDirectory))
            throw std::runtime_error("Environment capture directory must be empty");
        std::filesystem::create_directories(o.captureDirectory);
    }
    const auto catalog = loadRules(o.assets / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(o.assets / "npcs/level.json");
    const Map height = rendererHeightMap(), hub = sanctuaryMap();
    Window window(o.width, o.height, capture);
    GpuResources resources;
    Presenter presenter(resources, o.assets);
    WorldRenderer renderer(resources, world_assets::load(o.assets / "prepared/world"));
    renderer.supersampling(o.supersampling);
    auto review = std::make_unique<Review>(height, catalog, definitions);
    auto view = std::make_unique<GameView>(nullptr, &renderer);
    SpellArea area;
    auto present = [&] {
        review->animator.setTime(review->game.timeMs(), &review->game.effects());
        area.ground = review->game.effects().ground();
        view->update(review->game, review->animator.snapshot(), area);
        review->commands.refresh(view->picking() ? view->picking()->targets() : TargetSnapshot{},
                                 review->game.actionBusy(), false, true);
        view->present(presenter, review->game, review->map, false, true, &review->commands.snapshot());
    };
    std::ofstream manifest;
    if (capture) {
        manifest.open(o.captureDirectory / "frames.csv");
        manifest << "name,time_ms,cell_x,cell_y,heading,progress,moving\n";
        if (!manifest)
            throw std::runtime_error("Cannot create environment frame manifest");
    }
    auto save = [&](const std::string& name) {
        present();
        const auto& m = review->game.movement();
        manifest << name << ',' << review->game.timeMs() << ',' << m.player.cell.x << ',' << m.player.cell.y
                 << ',' << unsigned(m.player.heading) << ',' << m.transition.progress() << ','
                 << review->game.actionBusy() << '\n';
        presenter.capture(o.captureDirectory / (name + ".png"));
        OwnedImage image(LoadImageFromTexture(resources.target(renderer.image().target).texture));
        if (!image.value.data)
            throw std::runtime_error("Environment capture readback failed");
        ImageFlipVertical(&image.value);
        if (!ExportImage(image.value, (o.captureDirectory / (name + "-logical.png")).string().c_str()))
            throw std::runtime_error("Environment capture export failed");
    };
    auto replace = [&](const Map& map, bool actors) {
        auto next = std::make_unique<Review>(map, catalog, definitions, actors);
        auto nextView = std::make_unique<GameView>(nullptr, &renderer);
        next->animator.setTime(0);
        nextView->update(next->game, next->animator.snapshot(), area);
        nextView->activate();
        view.swap(nextView);
        nextView.reset();
        review.swap(next);
    };
    if (capture) {
        for (bool quality : {false, true}) {
            renderer.supersampling(quality);
            const std::string q = quality ? "ss" : "native";
            review->game.restorePose({{3, 4}, Heading::North}, true);
            save(q + "-step-start");
            review->game.move(Action::Forward);
            for (unsigned i = 1; i <= 8; ++i) {
                review->game.update(.02, false);
                save(q + "-step-" + std::to_string(i));
            }
            review->game.restorePose({{5, 3}, Heading::North}, true);
            save(q + "-high-ledge");
            review->game.restorePose({{3, 6}, Heading::East}, true);
            review->game.move(Action::Forward);
            if (!review->game.movement().blocked)
                throw std::runtime_error("Review low clearance unexpectedly traversable");
            save(q + "-rejected-clearance");
            review->game.restorePose({{3, 3}, Heading::East}, true);
            save(q + "-raised-door-closed");
            if (!review->game.interact(review->game.nearbyFixture()))
                throw std::runtime_error("Raised review door cannot open");
            save(q + "-raised-door-opening-0");
            review->game.update(.15, false);
            save(q + "-raised-door-opening-150");
            review->game.update(.15, false);
            save(q + "-raised-door-open");
            replace(hub, false);
            review->game.restorePose({{6, 4}, Heading::West}, true);
            save(q + "-hub-covered");
            for (unsigned step = 0; step < 3; ++step) {
                review->game.move(Action::Forward);
                for (unsigned i = 1; i <= 8; ++i) {
                    review->game.update(.02, false);
                    save(q + "-hub-exit-" + std::to_string(step * 8 + i));
                }
                review->game.update(.001, false);
            }
            for (unsigned h = 0; h < 4; ++h) {
                review->game.restorePose({{4, 3}, Heading(h)}, true);
                save(q + "-hub-turn-" + std::to_string(h));
            }
            review->game.restorePose({{3, 2}, Heading::West}, true);
            save(q + "-wall-art");
            review->game.restorePose({{2, 6}, Heading::West}, true);
            save(q + "-bench");
            review->game.restorePose({{6, 6}, Heading::East}, true);
            save(q + "-urn");
            replace(height, true);
            review->game.restorePose({{3, 4}, Heading::North}, true);
            save(q + "-raised-actor");
            replace(height, false);
        }
        std::cout
            << "R05 application-owned captures exported; diagnostic poses and actual step transitions.\n";
        return 0;
    }
    Input input;
    SetWindowTitle("R05 heights | F1 heights, F2 outdoor hub | F5 native, F6 supersampling");
    while (!WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), .1f);
        auto sample = input.poll(dt);
        sample.ui.text = pollTextInput();
        if (IsKeyPressed(KEY_ESCAPE) && !review->commands.snapshot().modal())
            break;
        if (sample.ui.focused) {
            if (IsKeyPressed(KEY_F1))
                replace(height, false);
            if (IsKeyPressed(KEY_F2))
                replace(hub, false);
            if (IsKeyPressed(KEY_F5))
                renderer.supersampling(false);
            if (IsKeyPressed(KEY_F6))
                renderer.supersampling(true);
        }
        auto& r = *review;
        if (sample.clearPending)
            r.game.clearPending();
        const bool paused = !sample.ui.focused || r.commands.snapshot().modal();
        const auto before = r.commands.snapshot().combat;
        r.commands.record(r.game.update(dt, paused));
        if (!paused && sample.action)
            r.game.move(*sample.action, sample.fresh);
        present();
        const auto* p = view->picking();
        const auto pointer = sample.ui.pointer;
        const int x = pointer ? int(pointer->x) - viewX : -1, y = pointer ? int(pointer->y) - viewY : -1;
        routeHudInput(r.commands, sample.ui, p ? p->pick(x, y) : std::nullopt,
                      confirmationMatches(before, r.game.combat().snapshot()),
                      p ? p->pickLoot(x, y) : std::nullopt, view->groundCell(r.game, x, y),
                      p ? p->pickFixture(x, y) : std::nullopt);
    }
    return 0;
}
} // namespace blobber
