#pragma once
#include "game/session.hpp"
#include "inventory_fixtures.hpp"
#include <memory>
namespace blobber {
inline Map fixtureMap() {
    return Map({"#########", "#...#...#", "#...#...#", "#.......#", "#...#...#", "#...#...#", "#########"});
}
inline FloorFixtures fixtureDefinitions(bool locked = true) {
    FloorFixtures f;
    f.discovery = true;
    f.entries[f.count++] = {{4, 3}, Heading::East, FixtureKind::Door, std::uint8_t(locked ? 2 : 0)};
    if (locked)
        f.entries[f.count++] = {{2, 2}, Heading::North, FixtureKind::Key, 2};
    f.entries[f.count++] = {{6, 3}, Heading::North, FixtureKind::Downstairs, 0};
    return f;
}
inline rules::Catalog fixtureCatalog() {
    auto f = inventoryFixture();
    rules::ItemDefinition token;
    token.id = "token";
    token.name = token.description = "Token";
    token.kind = rules::ItemKind::Gear;
    f.items.push_back(token);
    return inventoryCatalog(f);
}
inline FixtureRequest fixtureRequest(const GameSession& s, unsigned slot) {
    return {s.fixtures().id(slot), s.fixtures().revision()};
}
inline void collectFixtureKey(GameSession& s) {
    s.setAnimated(false);
    s.move(Action::TurnLeft);
    (void)s.interact(fixtureRequest(s, 1));
    s.move(Action::TurnRight);
    s.move(Action::Forward);
}
} // namespace blobber
