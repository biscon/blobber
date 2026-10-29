#include "app/arena_setup.hpp"
#include "app/menu_model.hpp"
#include "game/commands.hpp"
#include "game/perception.hpp"
#include "game/session.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "ui/interaction.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <unistd.h>
using namespace blobber;
namespace {
int failing = -1;
bool fail(ScenarioStage s) {
    return int(s) == failing;
}
struct Temp {
    std::filesystem::path path;
    Temp() {
        char pattern[] = "/tmp/blobber-arena-XXXXXX";
        const auto* p = mkdtemp(pattern);
        if (!p)
            throw std::runtime_error("mkdtemp");
        path = p;
    }
    ~Temp() {
        std::error_code e;
        std::filesystem::remove_all(path, e);
    }
};
rules::Catalog catalog() {
    auto c = creationCatalog();
    std::vector<rules::RaceDefinition> races;
    std::vector<rules::ClassDefinition> classes;
    std::vector<rules::ItemDefinition> items;
    std::vector<rules::SpellDefinition> spells;
    std::vector<rules::CreationClass> creation;
    for (std::uint16_t i = 0; i < c.raceCount(); ++i)
        races.push_back(*c.race({i}));
    for (std::uint16_t i = 0; i < c.classCount(); ++i) {
        classes.push_back(*c.characterClass({i}));
        creation.push_back(*c.creation({i}));
    }
    for (std::uint16_t i = 0; i < c.itemCount(); ++i)
        items.push_back(*c.item({i}));
    for (std::uint16_t i = 0; i < c.spellCount(); ++i)
        spells.push_back(*c.spell({i}));
    auto second = *c.monster({0});
    second.id = "second";
    return {{*c.attack({0})}, {*c.monster({0}), second}, races, classes, items, *c.preset(), spells,
            creation};
}
void placement(const rules::Catalog& c, unsigned count) {
    auto s = defaultArena(c);
    s.adjust({0}, 1);
    s.roster[0].quantity = count;
    const auto m = arenaMap();
    const std::vector<std::string> types{c.monster({0})->id, c.monster({1})->id};
    const auto w = arenaPlacements(m, c, s, types);
    const auto again = arenaPlacements(m, c, s, types);
    check(w.instances().size() == count, "exact roster count");
    std::set<std::tuple<int, int, int, int>> occupied;
    for (unsigned i = 0; i < count; ++i) {
        const auto& n = w.instances()[i];
        check(!m.solid(n.cell) && !npcSees(m, n.cell, n.facing, arenaSpawn().cell),
              "safe floor outside perception");
        check(occupied.insert({n.cell.x, n.cell.y, n.subcell.x, n.subcell.y}).second,
              "unique subcell occupancy");
        check(n.cell == again.instances()[i].cell && n.subcell == again.instances()[i].subcell,
              "deterministic placement");
        if (i < 4)
            check(n.cell == Cell{10, 7}, "first cell packs four");
        if (i == 4)
            check(n.cell == Cell{10, 6}, "overflow adjacent cell");
    }
    const auto party = arenaParty(c, s);
    GameSession session(m, c, party, w, s.seed, arenaSpawn(), true);
    for (int tick = 0; tick < 300; ++tick)
        session.update(.1, false);
    check(session.combat().snapshot().phase == CombatPhase::Exploration, "prep remains out of combat");
    for (unsigned i = 0; i < count; ++i)
        check(session.actors().snapshot().actors[i].npc.cell == w.instances()[i].cell,
              "guards stay at safe spawn");
}
void lifecycle(const rules::Catalog& c) {
    auto s = defaultArena(c);
    s.adjust({0}, 1);
    s.adjust({1}, 1);
    const auto m = arenaMap();
    const auto w = arenaPlacements(m, c, s, {c.monster({0})->id, c.monster({1})->id});
    check(w.instances()[0].monster->index == 0 && w.instances()[1].monster->index == 1,
          "mixed types preserve roster order");
    const auto party = arenaParty(c, s);
    GameSession game(m, c, party, w, s.seed, arenaSpawn(), true);
    game.setAnimated(false);
    for (int i = 0; i < 8; ++i) {
        game.move(Action::Forward);
        game.update(.1, false);
    }
    for (int i = 0; i < 30; ++i)
        game.update(.1, false);
    check(game.combat().snapshot().phase != CombatPhase::Exploration ||
              !(game.actors().snapshot().actors[0].npc.cell == Cell{10, 7}),
          "normal approach alerts guards");
    GameSession opener(m, c, party, w, s.seed, {{10, 13}, Heading::North}, false);
    auto shot = turnCommand(opener, PartyAction::Ranged);
    shot.character = {2};
    shot.target = opener.actors().id(0);
    check(opener.submit(shot).event == CombatEvent::AttackStarted, "arena supports normal ranged openers");
    GameSession prep(m, c, party, w, s.seed, arenaSpawn(), true);
    auto armor = spellCommand(prep, rules::SpellKind::MageArmor);
    armor.targetCount = 1;
    armor.allies[0] = {3};
    check(prep.submit(armor).reason == CommandReason::Resolved,
          "normal preparation spells work before combat");
    prep.update(6, false);
    check(prep.combat().snapshot().phase == CombatPhase::Exploration, "preparation does not force combat");
    GameSession caster(m, c, party, w, s.seed, {{10, 13}, Heading::North}, false);
    auto fire = spellCommand(caster, rules::SpellKind::FireBolt);
    fire.targetCount = 1;
    fire.enemies[0] = caster.actors().id(0);
    check(caster.submit(fire).event == CombatEvent::AttackStarted, "arena supports spell openers");
    game.actors().damage(game.actors().id(0), 10000, game.timeMs());
    check(!arenaCleared(game.actors().snapshot()), "partial victory cannot complete arena");
    game.actors().damage(game.actors().id(1), 10000, game.timeMs());
    check(arenaCleared(game.actors().snapshot()), "whole roster killed");
    game.party().damage({0}, 1);
    GameSession retry(m, c, party, w, s.seed, arenaSpawn(), true);
    check(retry.party().character({0})->hitPoints == retry.party().character({0})->stats.maxHitPoints,
          "retry restores HP");
    check(!arenaCleared(retry.actors().snapshot()) && retry.movement().player == arenaSpawn(),
          "retry restores roster and spawn");
    CommandController commands(retry);
    commands.arenaContext(false);
    commands.toggleMenu();
    UiInput click;
    click.pressed[std::size_t(UiKey::Click)] = true;
    click.pointer = ArtPoint{250, 180};
    routeHudInput(commands, click, {});
    check(commands.snapshot().retryConfirmation && !commands.snapshot().retryRequested,
          "pause rematch requires confirmation");
    UiInput cancel;
    cancel.pressed[std::size_t(UiKey::Confirm)] = true;
    routeHudInput(commands, cancel, {});
    check(!commands.snapshot().retryConfirmation, "default confirmation keeps attempt");
    routeHudInput(commands, click, {});
    click.pointer = ArtPoint{250, 220};
    routeHudInput(commands, click, {});
    check(commands.snapshot().retryRequested && !commands.snapshot().menuRequested,
          "confirmed retry distinct from edit");
}
void persistence(const rules::Catalog& c) {
    Temp temp;
    ArenaScenarioStore store(c, temp.path / "scenarios", {fail});
    check(store.list().count == 0, "missing folder is empty");
    auto s = defaultArena(c);
    s.adjust({0}, 1);
    s.adjust({1}, 1);
    s.seed = UINT32_MAX;
    auto encoded = encodeArena(c, s);
    auto decoded = decodeArena(c, encoded);
    check(encodeArena(c, decoded) == encoded, "lossless portable scenario");
    auto written = store.save(s);
    check(written.durable, "durable save");
    check(store.list().count == 1 && encodeArena(c, store.load(written.entry)) == encoded, "save/list/load");
    Temp other;
    std::filesystem::copy_file(store.directory() / written.entry.file.data(), other.path / "shared.json");
    ArenaScenarioStore imported(c, other.path);
    check(encodeArena(c, imported.load(imported.list().entries[0])) == encoded,
          "share without local character library");
    for (int stage = 0; stage < 3; ++stage) {
        failing = stage;
        rejects([&] { store.save(s, &written.entry); }, "write failure surfaced");
        check(encodeArena(c, store.load(written.entry)) == encoded, "failed replacement preserves file");
    }
    failing = int(ScenarioStage::DirectoryFlush);
    auto warning = store.save(s, &written.entry);
    check(!warning.durable && store.list().count == 1, "post-commit warning does not create duplicate");
    failing = -1;
    s.seed = 10;
    auto replaced = store.save(s, &written.entry);
    rejects([&] { store.save(s, &written.entry); }, "stale replacement rejected");
    rejects([&] { store.load(written.entry); }, "stale selection rejected");
    check(store.load(replaced.entry).seed == 10, "replacement committed");
    auto j = nlohmann::json::parse(encoded);
    for (const char* field : {"schema_version", "layout_version"}) {
        auto bad = j;
        bad[field] = 2;
        rejects([&] { decodeArena(c, bad.dump()); }, "unknown version");
    }
    for (const auto value : {-1ll, 4294967296ll}) {
        auto bad = j;
        bad["seed"] = value;
        rejects([&] { decodeArena(c, bad.dump()); }, "seed bound");
    }
    auto bad = j;
    bad["party"][0]["level"] = 3;
    rejects([&] { decodeArena(c, bad.dump()); }, "unsupported character level");
    bad = j;
    bad["party"][0]["id"] = bad["party"][1]["id"];
    rejects([&] { decodeArena(c, bad.dump()); }, "duplicate identity");
    bad = j;
    bad["monsters"][0]["monster"] = "missing";
    rejects([&] { decodeArena(c, bad.dump()); }, "missing content");
    bad = j;
    bad["monsters"][0]["quantity"] = 256;
    rejects([&] { decodeArena(c, bad.dump()); }, "total exceeds actor capacity");
    rejects([&] { decodeArena(c, "{\"name\":1,\"name\":2}"); }, "duplicate JSON key");
    rejects([&] { decodeArena(c, std::string(arenaMaxBytes + 1, ' ')); }, "file size bound");
    std::ofstream(store.directory() / "broken.json") << "{";
    const auto listing = store.list();
    bool invalid = false;
    for (std::size_t i = 0; i < listing.count; ++i)
        invalid |= listing.entries[i].error[0] != 0;
    check(invalid, "corrupt files remain visible with errors");
    ScenarioEntry traversal;
    std::snprintf(traversal.file.data(), traversal.file.size(), "../shared.json");
    rejects([&] { store.load(traversal); }, "path traversal rejected");
    for (int i = 0; i < 126; ++i)
        std::ofstream(store.directory() / ("fixture-" + std::to_string(i) + ".json")) << encoded;
    check(store.list().count == 128, "bounded listing full");
    rejects([&] { store.save(s); }, "full listing cannot create more");
}
void picker(const rules::Catalog& c) {
    auto s = defaultArena(c);
    s.adjust({0}, 1);
    MenuModel model;
    LibrarySnapshot empty;
    model.beginArenaPicker(s, c);
    check(model.ready(empty), "snapshot party does not require library");
    model.activeSlot = 0;
    model.assign(model.choices[3]);
    model.activeSlot = 1;
    model.assign({{}, 3});
    check(model.choices[1].snapshot == 3 && model.choices[0].snapshot == 1,
          "preset matches imported snapshot identity and swaps");
    model.activeSlot = 0;
    model.assign(model.choices[1]);
    auto party = model.resolve(c, empty);
    check(party.characters[0].id == s.party[3].id.data() && party.characters[3].id == s.party[0].id.data(),
          "snapshot formation swaps");
    ArenaSetup setup;
    setup.scenario = s;
    setup.syncSeed();
    setup.available.fill(true);
    setup.filter(c);
    check(setup.visibleCount == 2 && !setup.error(c), "available monster catalog");
    std::snprintf(setup.search.data(), 65, "SECOND");
    setup.filter(c);
    check(setup.visibleCount == 1, "case-insensitive filter");
    std::snprintf(setup.seedText.data(), 65, "4294967296");
    check(!setup.readSeed(), "invalid seed input");
    s.roster[0].quantity = 256;
    check(!s.adjust({1}, 1), "plus cannot overflow actor capacity");
    s.roster[0].quantity = 1;
    check(s.adjust({0}, -1) && s.count == 0, "minus removes zero rows");
}
} // namespace
int main(int argc, char**) {
    try {
        const auto c = catalog();
        if (argc > 1) {
            placement(c, 256);
            std::cout << "Arena maximum-roster stress passed\n";
            return 0;
        }
        for (unsigned n : {1u, 4u, 5u})
            placement(c, n);
        lifecycle(c);
        persistence(c);
        picker(c);
        std::cout << "Arena tests passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
