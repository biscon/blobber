#include "game/movement.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include <iostream>
#include <limits>
#include <type_traits>
using namespace blobber;
namespace {
const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
ActorWorld group() {
    return testActors(map, {3, 5}, {"test"},
                      {{{0}, {3, 3}, {0, 0}, Heading::South},
                       {{0}, {3, 3}, {1, 0}},
                       {{0}, {3, 3}, {0, 1}},
                       {{0}, {3, 3}, {1, 1}}});
}
void initialization() {
    static_assert(!std::is_move_constructible_v<ActorWorld>);
    static_assert(!std::is_copy_constructible_v<ActorWorld>);
    auto actors = group();
    auto other = group();
    const auto id = actors.id(0);
    check(actors.actor(id)->hitPoints == 7 && actors.actor(id)->maxHitPoints == 7,
          "HP comes from fixture catalog");
    check(actors.catalog().monster(actors.actor(id)->monster)->id == "test", "Typed monster binding");
    check(!actors.actor({}) && !actors.actor(other.id(0)) && !actors.actor(actors.id(256)),
          "Invalid, out of bounds and foreign handles fail");
    check(actors.move(other.id(0), {2, 3}, {0, 0}, {3, 5}).error == ActorError::InvalidHandle,
          "Foreign mutation rejected");
    ActorId expired;
    {
        auto temporary = group();
        expired = temporary.id(0);
    }
    check(!actors.actor(expired), "Destroyed-world handle cannot alias another owner");
    const NpcWorld missing(map, {3, 5}, {"unknown"}, {{{0}, {3, 3}, {0, 0}}});
    rejects([&] { ActorWorld bad(map, testActorCatalog(), missing); }, "Unknown monster binding");
    auto empty = testActors(map, {1, 1}, {}, {});
    check(empty.snapshot().count == 0 && !empty.occupied({1, 1}), "Empty world");
    std::vector<std::string> rows(20, std::string(20, '.'));
    rows.front() = rows.back() = std::string(20, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    const Map large(rows);
    std::vector<NpcInstance> placements;
    for (int i = 0; i < 256; ++i)
        placements.push_back({{0}, {1 + i % 16, 1 + i / 16}, {0, 0}});
    auto full = testActors(large, {18, 18}, {"test"}, placements);
    check(full.snapshot().count == 256 && full.actor(full.id(255)), "Full actor capacity supported");
    placements.push_back({{0}, {17, 17}, {0, 0}});
    rejects([&] { auto overflow = testActors(large, {18, 18}, {"test"}, placements); },
            "Over capacity rejected");
    std::vector<std::string> names;
    for (int i = 0; i < 32; ++i)
        names.push_back("type" + std::to_string(i));
    const auto catalog = testMonsterCatalog(names);
    const NpcWorld many(map, {1, 1}, names, {{{31}, {3, 3}, {0, 0}}});
    ActorWorld types(map, catalog, many);
    check(types.actor(types.id(0))->monster.index == 31, "Full appearance binding capacity");
}
void distinctAppearanceBindings() {
    // Catalog order intentionally differs from appearance order.
    const auto catalog = testMonsterCatalog({"fixture.archer", "fixture.melee"});
    const NpcWorld placements(map, {3, 5}, {"fixture.melee", "fixture.archer"},
                              {{{0}, {2, 3}, {0, 0}}, {{1}, {4, 3}, {0, 0}}});
    ActorWorld actors(map, catalog, placements);
    check(actors.actor(actors.id(0))->monster.index == 1 && actors.actor(actors.id(1))->monster.index == 0,
          "Distinct appearances resolve by name, not coincident catalog indices");
    auto missingCatalog = testMonsterCatalog({"fixture.melee"});
    rejects([&] { ActorWorld missing(map, missingCatalog, placements); },
            "An unbound second appearance fails at construction");
    auto instances = placements.instances();
    instances[1].monster = catalog.findMonster("fixture.melee");
    const NpcWorld overridden(map, {3, 5}, {"fixture.melee", "fixture.archer"}, instances);
    ActorWorld overrideWorld(map, catalog, overridden);
    check(overrideWorld.actor(overrideWorld.id(1))->monster.index == 1 &&
              overrideWorld.actor(overrideWorld.id(1))->npc.type.index == 1,
          "Explicit rules overrides remain independent of appearance selection");
}
void movementAndLifecycle() {
    auto actors = group();
    const auto id = actors.id(0);
    for (const auto cell :
         {Cell{-1, 3}, Cell{0, 3}, Cell{3, 1}, Cell{2, 2}, Cell{std::numeric_limits<int>::max(), 3}})
        check(actors.move(id, cell, {0, 0}, {3, 5}).error == ActorError::InvalidDestination,
              "Walls, bounds, diagonal and distant moves rejected");
    check(actors.move(id, {3, 3}, {-1, 0}, {3, 5}).error == ActorError::InvalidDestination,
          "Bad subslot rejected");
    check(actors.move(id, {3, 4}, {0, 0}, {3, 4}).error == ActorError::PartyCell,
          "Current party cell excluded");
    for (Cell sub : {Cell{1, 0}, Cell{0, 1}, Cell{1, 1}})
        check(actors.move(id, {3, 3}, sub, {3, 5}).error == ActorError::Occupied,
              "All occupied subslots rejected");
    check(!actors.move(id, {3, 3}, {0, 0}, {3, 5}).changed && !actors.face(id, Heading::South).changed,
          "Unchanged pose is a no-op");
    check(actors.face(id, static_cast<Heading>(4)).error == ActorError::InvalidFacing &&
              actors.snapshot().revision == 0 && actors.occupied({3, 3}) && !actors.occupied({3, 4}),
          "Rejected operations preserve transform, occupancy and revision");
    check(actors.move(id, {3, 4}, {0, 0}, {3, 5}).changed && actors.occupied({3, 3}) &&
              actors.occupied({3, 4}),
          "Moving one of four leaves original cell blocked");
    check(actors.move(id, {3, 4}, {1, 1}, {3, 5}).changed, "Same-cell subslot move");
    check(actors.face(id, Heading::West).changed && actors.actor(id)->npc.facing == Heading::West,
          "Facing changes independently");
    const auto revision = actors.snapshot().revision;
    check(!actors.damage(id, 0, 0).changed && actors.damage(id, -1, 0).error == ActorError::InvalidDamage &&
              actors.snapshot().revision == revision,
          "Zero/negative damage does not mutate");
    const auto hit = actors.damage(id, 2, 500);
    check(hit.damage == 2 && !hit.died && actors.actor(id)->hitPoints == 5, "Resolved surviving damage");
    check(actors.damage(id, 1, 499).error == ActorError::InvalidTime && actors.actor(id)->hitPoints == 5,
          "Backwards damage time rejected atomically");
    const auto killed = actors.damage(id, std::numeric_limits<std::int64_t>::max(), 500);
    check(killed.died && killed.damage == 5 && actors.actor(id)->hitPoints == 0 && !actors.occupied({3, 4}),
          "Lethal overflow-safe damage frees occupancy immediately");
    const auto deathRevision = actors.snapshot().revision;
    check(actors.damage(id, 1, 600).error == ActorError::NotAlive &&
              actors.move(id, {3, 3}, {0, 0}, {3, 5}).error == ActorError::NotAlive &&
              actors.face(id, Heading::North).error == ActorError::NotAlive &&
              actors.snapshot().revision == deathRevision,
          "Death resolves once and terminal actors cannot act");
    const auto replacement = actors.id(1);
    check(actors.move(replacement, {3, 4}, {1, 1}, {3, 5}).changed,
          "Released dead subslot reusable by survivor");
    check(actors.remove(id).changed && actors.occupied({3, 4}) && !actors.actor(id),
          "Removing old dead actor must not vacate new occupant");
    check(actors.remove(id).error == ActorError::InvalidHandle, "Removed handle stays invalid");
    check(actors.remove(replacement).changed && !actors.occupied({3, 4}),
          "Last live occupant removal frees cell");
    check(actors.snapshot().count == 4 && actors.actor(actors.id(3)),
          "Removal does not compact identity slots");
}
void queuedMovement() {
    auto actors = testActors(map, {3, 5}, {"test"}, {{{0}, {2, 3}, {0, 0}}});
    Controller player({{3, 5}, Heading::North});
    player.submit(map, Action::Forward, true, &actors);
    player.submit(map, Action::Forward, true, &actors);
    check(actors.move(actors.id(0), {3, 3}, {0, 0}, player.state().player.cell).changed,
          "Actor can enter queued destination before command executes");
    player.update(map, 1, &actors);
    check(player.state().blocked && player.state().player.cell == Cell{3, 4},
          "Queued command rechecks live occupancy");
    player.submit(map, Action::TurnLeft, true, &actors);
    player.submit(map, Action::Right, true, &actors); // North after turning west.
    check(actors.remove(actors.id(0)).changed, "Remove blocker while party transition active");
    player.update(map, 1, &actors);
    check(!player.state().blocked && player.state().player.cell == Cell{3, 3},
          "Queued movement enters newly freed cell");
}
} // namespace
int main() {
    try {
        initialization();
        distinctAppearanceBindings();
        movementAndLifecycle();
        queuedMovement();
        std::cout << "Actor checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
