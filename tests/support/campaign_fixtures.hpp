#pragma once
#include "automation_fixtures.hpp"
#include <queue>
namespace blobber::campaign_test {
using namespace blobber::automation::test;
inline FixtureRequest stair(const GameSession& g, FixtureKind kind) {
    for (unsigned i = 0; i < g.fixtures().definition().count; ++i)
        if (g.fixtures().definition().entries[i].kind == kind)
            return {g.fixtures().id(i), g.fixtures().revision()};
    throw std::runtime_error("Missing fixture");
}
inline void face(GameSession& g, Heading h) {
    for (unsigned tries = 0; tries < 4 && g.movement().player.heading != h; ++tries)
        g.move(Action::TurnRight);
    if (g.movement().player.heading != h)
        throw std::runtime_error("Fixture turn blocked at " + std::to_string(g.movement().player.cell.x) +
                                 "," + std::to_string(g.movement().player.cell.y) +
                                 " phase=" + std::to_string(unsigned(g.combat().snapshot().phase)));
}
inline void walk(GameSession& g, Cell to) {
    g.setAnimated(false);
    const auto& map = g.geometry().base();
    const auto n = std::size_t(map.width() * map.height());
    std::vector<int> parent(n, -1);
    auto index = [&](Cell c) { return c.y * map.width() + c.x; };
    auto cell = [&](int i) { return Cell{i % map.width(), i / map.width()}; };
    std::queue<Cell> q;
    const auto start = g.movement().player.cell;
    parent[index(start)] = index(start);
    q.push(start);
    while (!q.empty()) {
        const auto c = q.front();
        q.pop();
        for (unsigned d = 0; d < 4; ++d) {
            const auto next = c + forward(Heading(d));
            if (!map.canStep(c, next) || parent[index(next)] >= 0)
                continue;
            const auto f = g.fixtures().at(next);
            if (f >= 0 && stairFixture(g.fixtures().definition().entries[unsigned(f)].kind))
                continue;
            if (f >= 0 && g.fixtures().definition().entries[unsigned(f)].kind == FixtureKind::Door &&
                g.fixtures().definition().entries[unsigned(f)].key &&
                !(g.party().questKeys() & (1u << g.fixtures().definition().entries[unsigned(f)].key)))
                continue;
            if (g.actors().occupied(next))
                continue;
            if (f >= 0 && g.fixtures().definition().entries[unsigned(f)].kind == FixtureKind::GuardianGate &&
                !g.objective().completed)
                continue;
            parent[index(next)] = index(c);
            q.push(next);
        }
    }
    check(parent[index(to)] >= 0, "fixture path exists");
    std::vector<Cell> path;
    for (auto c = to; !(c == start); c = cell(parent[index(c)]))
        path.push_back(c);
    for (auto i = path.rbegin(); i != path.rend(); ++i) {
        const auto here = g.movement().player.cell;
        const auto delta = *i + here * -1;
        Heading h = delta.x > 0   ? Heading::East
                    : delta.x < 0 ? Heading::West
                    : delta.y > 0 ? Heading::South
                                  : Heading::North;
        face(g, h);
        auto fixture = g.fixtures().at(*i);
        if (fixture >= 0 && g.fixtures().definition().entries[unsigned(fixture)].kind == FixtureKind::Door &&
            !g.fixtures().open(unsigned(fixture)))
            check(g.interact({g.fixtures().id(unsigned(fixture)), g.fixtures().revision()}).accepted,
                  "open traversed door");
        g.move(Action::Forward);
        check(g.movement().player.cell == *i, "checked fixture movement");
    }
}
inline void entrance(CampaignRun& run) {
    auto& g = run.game();
    const auto f = *g.fixtures().find(stair(g, FixtureKind::Downstairs).id);
    walk(g, stairLanding(f));
    face(g, f.facing);
}
inline void entrance(Client& client) {
    const auto& game = client.protocol.session()->game();
    const auto f = *game.fixtures().find(stair(game, FixtureKind::Downstairs).id);
    const auto move = [&](const char* direction) {
        check(client.act({{"kind", "move"}, {"direction", direction}}).at("data").at("accepted"),
              "Wire city navigation accepted");
        client.advance(100, false);
    };
    const auto turn = [&](Heading h) {
        while (game.movement().player.heading != h)
            move("turn_right");
    };
    const auto target = stairLanding(f);
    // Both town arrivals reach the clear landing row before crossing the square.
    while (game.movement().player.cell.y != target.y) {
        turn(game.movement().player.cell.y < target.y ? Heading::South : Heading::North);
        move("forward");
    }
    while (game.movement().player.cell.x != target.x) {
        turn(game.movement().player.cell.x < target.x ? Heading::East : Heading::West);
        move("forward");
    }
    turn(f.facing);
}
inline void kill(GameSession& g) {
    for (std::size_t i = 0; i < g.actors().snapshot().count; ++i)
        g.actors().damage(g.actors().id(i), 100000, g.timeMs());
    g.update(0, false);
}
inline void travel(CampaignRun& run, FixtureKind kind) {
    auto& g = run.game();
    auto r = stair(g, kind);
    const auto f = *g.fixtures().find(r.id);
    walk(g, stairLanding(f));
    face(g, f.facing);
    r = stair(g, kind);
    auto ticket = run.prepareTravel(run.request(r));
    check(run.commitTravel(ticket), "checked travel commits");
}
} // namespace blobber::campaign_test
