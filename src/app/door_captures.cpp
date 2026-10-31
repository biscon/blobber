#include "app/door_captures.hpp"
#include "generation/floor.hpp"
#include "platform/game_view.hpp"
namespace blobber {
void captureDoors(const rules::Catalog& catalog, const NpcWorld& source,
                  const std::vector<NpcTimings>& timings, WorldRenderer& world, Presenter& presenter,
                  const std::filesystem::path& dir) {
    std::vector<std::string> names;
    for (unsigned i = 0; i < source.typeCount(); ++i)
        names.push_back(source.typeId({std::uint16_t(i)}));
    for (unsigned heading = 0; heading < 4; ++heading) {
        auto rotate = [&](Cell c) { return generation::rotate(c, 9, heading); };
        std::vector<std::string> rows(9, std::string(9, '#'));
        for (int y = 1; y < 8; ++y)
            for (int x = 1; x < 8; ++x) {
                const auto c = rotate({x, y});
                rows[c.y][c.x] = (y == 4 && x != 4) ? '#' : '.';
            }
        Map map(rows);
        Player spawn{rotate({4, 5}), Heading(heading)};
        NpcWorld npcs(map, spawn.cell, names, {});
        FloorFixtures defs;
        defs.count = 1;
        defs.entries[0] = {rotate({4, 4}), Heading(heading), FixtureKind::Door, 0};
        GameSession game(map, catalog, npcs, 5, spawn, false, timings, {}, defs);
        NpcAnimator animator(game.actors(), timings);
        CommandController commands(game, timings);
        GameView view(nullptr, &world);
        SpellArea area;
        auto capture = [&](const std::string& suffix) {
            animator.setTime(game.timeMs());
            view.update(game, animator.snapshot(), area);
            commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, game.actionBusy(),
                             false, true);
            view.present(presenter, game, map, false, true, &commands.snapshot());
            presenter.capture(dir / ("door-" + std::to_string(heading) + "-" + suffix + ".png"));
        };
        capture("closed");
        if (!game.interact({game.fixtures().id(0), game.fixtures().revision()}))
            throw std::runtime_error("Door capture interaction failed");
        capture("opening-0");
        game.move(Action::Forward);
        for (unsigned ms = 75; ms <= 300; ms += 75) {
            game.update(.075, false);
            capture("opening-" + std::to_string(ms));
        }
        for (unsigned ms = 40; ms <= 160; ms += 40) {
            game.update(.04, false);
            capture("move-" + std::to_string(ms));
        }
        game.setAnimated(false);
        capture("settled");
    }
}
} // namespace blobber
