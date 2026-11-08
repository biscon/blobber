#include "game/perception.hpp"
#include "game/sanctuary_hub.hpp"
#include "generation/population.hpp"
#include "support/automation_fixtures.hpp"
#include <iostream>
#include <queue>
using namespace blobber;
using namespace blobber::generation;
using namespace blobber::automation::test;
namespace {
class HiddenVisibility final : public blobber::automation::Visibility {
    TargetSnapshot targets_;

  public:
    const TargetSnapshot& sample(const Map&, const GameSession& game) override {
        targets_ = {};
        targets_.world = game.actors().snapshot().world;
        targets_.revision = game.actors().snapshot().revision;
        return targets_;
    }
    bool lootVisible(LootId) const noexcept override { return false; }
};
std::unique_ptr<blobber::automation::Visibility> hiddenVisibility(void*, const Map&, const GameSession&) {
    return std::make_unique<HiddenVisibility>();
}
void knowledge() {
    const auto c = catalog();
    const auto d = definitions();
    auto scenario = generatedScenario(c);
    scenario["kind"] = "campaign";
    scenario.erase("depth");
    Client player(c, d, blobber::automation::Mode::Visual, hiddenVisibility);
    check(player.request("start", {{"scenario", scenario}}).at("ok"), "owned player campaign starts");
    const auto stairs = sanctuaryHub().fixtures.entries[0];
    player.protocol.replaySession()->replayGame().restorePose({stairLanding(stairs), stairs.facing}, false);
    player.protocol.replaySession()->replayGame().update(0, false);
    const auto entry = player.act({{"kind", "interact"}, {"fixture", 0}}).at("data");
    check(entry.at("accepted"),
          (std::string("player legally enters generated floor: ") + entry.dump()).c_str());
    const auto before = player.request("observe").at("data");
    check(!before.at("generation").contains("route") && !before.at("generation").contains("encounters"),
          "player generation identity does not expose route or recipe metadata");
    const auto full = player.request("inspect").at("data");
    check(full.at("generation").at("encounters").size() == 6 && full.at("generation").contains("route"),
          "explicit diagnostic campaign inspection exposes active encounter definition");
    const auto after = player.request("observe").at("data");
    check(!after.at("generation").contains("route") && !after.at("generation").contains("encounters") &&
              before.at("discovered") == after.at("discovered") &&
              before.at("fixtures") == after.at("fixtures"),
          "diagnostic inspection cannot teach hidden campaign routes to a player");
}
void budgets() {
    const auto c = catalog(false, 50, 7, false, false, true);
    const auto d = definitions();
    unsigned beforeFinal = 0, optional = 0;
    for (unsigned depth = 1; depth <= 3; ++depth) {
        for (bool fallback : {false, true}) {
            const auto floor = generate({5, depth}, fallback);
            check(bool(floor), "population and fallback generate");
            const auto& f = floor.floor->layout;
            const auto p = resolvePopulation(f, floor.floor->request, c, d.types);
            unsigned directXp = 0, reserveXp = 0, guardianXp = 0;
            for (unsigned i = 0; i < f.encounterCount; ++i) {
                const auto& e = f.encounters[i];
                unsigned xp = 0;
                for (unsigned n = 0; n < e.count; ++n)
                    xp += c.monster(*p.actors[e.first + n].monster)->experience;
                if (e.role == EncounterRole::Direct)
                    directXp += xp;
                if (e.role == EncounterRole::Optional)
                    reserveXp += xp;
                if (e.role == EncounterRole::Guardian)
                    guardianXp += xp;
            }
            check(directXp == (depth == 3 ? 600u : 500u) && guardianXp == (depth == 1   ? 250u
                                                                           : depth == 2 ? 400u
                                                                                        : 450u),
                  "catalog XP matches intended route without milestone grants");
            check(reserveXp == (depth == 1 ? 75u : 200u), "finite accessible reserve XP");
            check(p.count <= 32 &&
                      f.encounterCount == (depth == 1   ? 6u
                                           : depth == 2 ? 7u
                                                        : 8u) &&
                      f.fixtures.count <= 32,
                  "population stays within fixed content admission bounds");
            check(p.recovery.groundCount == 6 && p.recovery.ground[0].quantity == 1 &&
                      p.recovery.ground[1].quantity == 2 && p.recovery.ground[2].quantity == 20 &&
                      p.recovery.ground[3].quantity == 1 && p.recovery.ground[4].quantity == 1 &&
                      p.recovery.ground[5].quantity == 10 && p.objective.rewardCount == (depth < 3 ? 1u : 0u),
                  "main/reserve caches and final victory have explicit finite budgets");
            const auto lead = c.monster(*p.actors[p.objective.members[0]].monster);
            check(lead->hitPoints == (depth == 1 ? 20 : depth == 2 ? 34 : 42), "guardian lead durability");
            for (unsigned i = 0; i < f.encounterCount; ++i) {
                const auto& e = f.encounters[i];
                unsigned distinct = 0;
                for (unsigned n = 0; n < e.count; ++n) {
                    bool unique = true;
                    for (unsigned k = 0; k < n; ++k)
                        unique &= !(f.population[e.first + n].cell == f.population[e.first + k].cell);
                    distinct += unique;
                }
                check(distinct >= (unsigned(e.count) + 1) / 2,
                      "groups use multiple cells without overfilling");
            }
            auto bad = f;
            bad.encounters[0].count = 32;
            check(validate(bad).code == Error::Population, "invalid member range rejects");
            bad = f;
            bad.population[0].recipe = ActorRecipe::Bugbear;
            check(validate(bad).code == Error::Population, "wrong member species rejects");
            bad = f;
            bad.supply = bad.spawn.cell;
            check(validate(bad).code == Error::Population, "cache cannot move into protected arrival");
            bad = f;
            bad.routeCount = unsigned(bad.route.size()) + 1;
            check(validate(bad).code == Error::Population, "route count checked before indexing");
            bad = f;
            --bad.fixtures.count;
            check(bool(validate(bad)), "missing population isolation door rejects");
            if (!fallback) {
                beforeFinal += directXp + (depth < 3 ? guardianXp : 0);
                optional += reserveXp;
            }
        }
    }
    check(beforeFinal == 2250 && optional == 475, "revised direct-route XP and finite optional margin");
}
void approaches() {
    const auto c = catalog(false, 50, 7, true);
    const auto d = definitions();
    for (unsigned depth = 1; depth <= 3; ++depth) {
        const auto result = generate({8, depth});
        const auto& f = *result.floor;
        const auto p = preparePopulation(f, c, d.types);
        for (unsigned i = 0; i < f.layout.encounterCount; ++i) {
            const auto& e = f.layout.encounters[i];
            const auto& actor = f.layout.population[e.first];

            unsigned previous = maxRooms;
            if (e.role == EncounterRole::Direct) {
                for (unsigned j = 1; j < f.layout.routeCount; ++j)
                    if (f.layout.route[j] == e.room) {
                        previous = f.layout.route[j - 1];
                        break;
                    }
            } else if (e.role == EncounterRole::Guardian) {
                previous = sectorJunction(f.layout);
            } else {
                for (unsigned j = 0; j < f.layout.connectionCount; ++j) {
                    const auto& edge = f.layout.connections[j];
                    if (edge.a == e.room)
                        previous = edge.b;
                    if (edge.b == e.room)
                        previous = edge.a;
                }
            }
            const auto* edge = connectionBetween(f.layout, previous, e.room);
            check(edge, "encounter has its recorded incoming connection");
            int door = -1;
            for (unsigned j = 0; j < f.layout.fixtures.count; ++j)
                if (f.layout.fixtures.entries[j].kind == FixtureKind::Door &&
                    f.layout.fixtures.entries[j].cell == threshold(*edge))
                    door = int(j);
            check(door >= 0, "incoming encounter connection has an isolation door");
            const auto towardDoor =
                edge->a == e.room ? Heading((unsigned(edge->facingA) + 2) % 4) : edge->facingA;
            const auto outside = threshold(*edge) + forward(towardDoor) * -1;
            Player spawn{outside, towardDoor};
            int key = -1;
            for (unsigned j = 0; j < f.layout.fixtures.count; ++j)
                if (f.layout.fixtures.entries[unsigned(door)].key &&
                    f.layout.fixtures.entries[j].kind == FixtureKind::Key) {
                    key = int(j);
                    spawn.cell = f.layout.fixtures.entries[j].cell;
                }
            Party party(c);
            GameSession game(f.map, party, p.placements, f.runtimeSeed, spawn, d.timings, p.recovery,
                             f.layout.fixtures, p.objective);
            // Arrival from a previously cleared room must not create an unrelated encounter behind us.
            for (unsigned n = 0; n < game.actors().snapshot().count; ++n)
                if (regionAt(f.layout, game.actors().snapshot().actors[n].npc.cell) == previous ||
                    (key >= 0 && regionAt(f.layout, game.actors().snapshot().actors[n].npc.cell) ==
                                     regionAt(f.layout, f.layout.fixtures.entries[unsigned(key)].cell)))
                    game.actors().damage(game.actors().id(n), 100000, 0);
            game.update(0, false);
            if (key >= 0)
                check(game.interact({game.fixtures().id(unsigned(key)), game.fixtures().revision()}).accepted,
                      "checked key acquisition before sector approach");
            game.restorePose({outside, towardDoor}, false);
            game.update(0, false);
            const auto opened =
                game.interact({game.fixtures().id(unsigned(door)), game.fixtures().revision()});
            if (!opened.accepted) {
                std::cerr << "depth=" << depth << " encounter=" << i << " previous=" << previous
                          << " room=" << unsigned(e.room) << " pose=" << outside.x << ',' << outside.y
                          << " key=" << key << " reason=" << opened.reason << '\n';
                for (unsigned n = 0; n < game.actors().snapshot().count; ++n)
                    if (game.ai().actors[n].awareness != Awareness::Routine)
                        std::cerr << n << " awareness=" << unsigned(game.ai().actors[n].awareness)
                                  << " pos=" << game.actors().snapshot().actors[n].npc.cell.x << ','
                                  << game.actors().snapshot().actors[n].npc.cell.y
                                  << " hp=" << game.actors().snapshot().actors[n].hitPoints << " region="
                                  << regionAt(f.layout, game.actors().snapshot().actors[n].npc.cell) << '\n';
            }
            check(opened.accepted, "ordinary approach opens by checked interaction");
            // Walk only through this room's opened port toward a legal contact cell.
            // Pillar/bent interiors require an actual turn rather than an assumed straight sightline.
            std::array<int, maxCells> parent;
            parent.fill(-1);
            std::queue<Cell> queue;
            const auto start = game.movement().player.cell;
            auto index = [&](Cell c) { return c.y * f.map.width() + c.x; };
            queue.push(start);
            parent[index(start)] = index(start);
            Cell target = start;
            while (!queue.empty()) {
                const auto here = queue.front();
                queue.pop();
                if (combatAdjacent(game.geometry(), here, actor.cell)) {
                    target = here;
                    break;
                }
                for (unsigned h = 0; h < 4; ++h) {
                    const auto next = here + forward(Heading(h));
                    if (!game.geometry().canStep(here, next) || game.actors().occupied(next) ||
                        parent[index(next)] >= 0)
                        continue;
                    parent[index(next)] = index(here);
                    queue.push(next);
                }
            }
            std::vector<Cell> path;
            for (auto at = target; !(at == start);) {
                path.push_back(at);
                const auto n = parent[index(at)];
                at = {n % f.map.width(), n / f.map.width()};
            }
            for (auto step = path.rbegin(); step != path.rend() && realTime(game.combat().snapshot().phase);
                 ++step) {
                for (unsigned attempt = 0; attempt < 100 && realTime(game.combat().snapshot().phase) &&
                                           !(game.movement().player.cell == *step);
                     ++attempt) {
                    const auto delta = *step + game.movement().player.cell * -1;
                    unsigned heading = 0;
                    while (heading < 4 && !(forward(Heading(heading)) == delta))
                        ++heading;
                    check(heading < 4, "approach path uses cardinal cells");
                    game.move(unsigned(game.movement().player.heading) == heading ? Action::Forward
                                                                                  : Action::TurnRight);
                    game.update(.1, false);
                }
            }
            for (unsigned tick = 0; tick < 30; ++tick)
                game.update(.1, false);
            unsigned aware = 0;
            for (unsigned n = 0; n < game.actors().snapshot().count; ++n)
                if (game.ai().actors[n].awareness == Awareness::Pursuing ||
                    game.ai().actors[n].awareness == Awareness::Investigating) {
                    check(n >= e.first && n < e.first + e.count,
                          "other closed rooms do not merge into approach");
                    ++aware;
                }
            check(aware > 0, "real AI sees and approaches the opened encounter");
        }
    }
}
} // namespace
int main() {
    try {
        knowledge();
        budgets();
        approaches();
        std::cout << "Population budgets and actual AI approaches passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
