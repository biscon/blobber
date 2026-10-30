#include "automation/trace.hpp"
#include "game/commands.hpp"
#include "support/automation_fixtures.hpp"
#include <iostream>
#include <sstream>
#include <sys/wait.h>
using namespace blobber;
using namespace blobber::automation;
using namespace blobber::automation::test;
namespace {
void boundaries() {
    const auto cat = catalog();
    const auto defs = definitions();
    Client c(cat, defs);
    check(!c.request("observe").at("ok"), "no session query rejects");
    check(c.start(cat).at("ok"), "start validates existing arena format");
    const auto first = c.protocol.state();
    const auto actions = c.request("actions");
    check(actions.at("ok") && actions.at("data").at("combat").size() == 4,
          "discover every party member's actions");
    check(c.protocol.state() == first, "observation and discovery are non-mutating");
    auto invalid = Json::parse(encodeArena(cat, scenario(cat)));
    invalid["monsters"][0]["quantity"] = 257;
    check(!c.request("start", {{"scenario", invalid}}).at("ok"), "invalid replacement rejects");
    check(c.protocol.state() == first, "invalid replacement preserves running session");
    Json stale = {{"version", 1},    {"id", "stale"}, {"op", "advance"}, {"session", first["session"]},
                  {"revision", "0"}, {"ticks", 10}};
    check(!c.protocol.request(stale, c.sink).at("ok") && c.protocol.state() == first,
          "stale revision cannot advance");
    check(!c.request("advance", {{"ticks", 1001}}).at("ok"), "per-request tick bound");
    check(!c.request("advance", {{"ticks", -1}}).at("ok"), "negative ticks reject");
    check(!c.act({{"kind", "move"}, {"direction", "teleport"}}).at("ok"), "unknown direction rejects");
    check(c.protocol.state() == first, "bad requests have no costs, clocks or revision changes");
    check(c.act({{"kind", "move"}, {"direction", "forward"}}).at("data").at("accepted"), "move accepted");
    check(!c.act({{"kind", "move"}, {"direction", "forward"}}).at("data").at("accepted"),
          "no command queue during transition");
    const auto elapsed = c.advance(1000, true).at("data").at("ticks").get<unsigned>();
    check(elapsed > 0 && elapsed < 1000, "decision advance stops when movement settles");
    const auto old = c.protocol.state();
    check(c.request("reset").at("ok"), "reset succeeds");
    stale["session"] = old["session"];
    stale["revision"] = old["revision"];
    check(!c.protocol.request(stale, c.sink).at("ok"), "reset invalidates old requests");
    check(c.protocol.session()->game().movement().player == arenaSpawn(), "reset restores initial pose");
    const auto before = c.protocol.state();
    c.request("inspect");
    check(c.protocol.state().at("diagnostic_inspection_used"), "inspection provenance recorded");
    check(c.protocol.state().at("time_ms") == before.at("time_ms"), "inspection freezes time");
    bool bad = false;
    try {
        parseRequest(std::string(33, '[') + std::string(33, ']'));
    } catch (...) {
        bad = true;
    }
    check(bad, "nesting rejected before DOM parsing");
    bad = false;
    try {
        parseRequest(std::string(maxRequestBytes + 1, ' '));
    } catch (...) {
        bad = true;
    }
    check(bad, "request byte limit");
    bad = false;
    try {
        parseRequest("{\"a\":1,\"a\":2}");
    } catch (...) {
        bad = true;
    }
    check(bad, "duplicate request fields reject");
    bad = false;
    try {
        serial(Json("18446744073709551616"));
    } catch (...) {
        bad = true;
    }
    check(bad, "uint64 overflow rejects");
}
void tickBudget() {
    const auto cat = catalog();
    const auto defs = definitions();
    Client c(cat, defs);
    c.start(cat);
    for (unsigned i = 0; i < maxRunTicks / maxRequestTicks; ++i) {
        const auto result = c.advance(maxRequestTicks, false);
        check(result.at("data").at("ticks") == maxRequestTicks,
              "explicit waiting consumes exactly requested ticks");
    }
    const auto result = c.advance(1, false).at("data");
    check(result.at("ticks") == 0 && result.at("stop") == "tick_limit",
          "run tick limit reports zero additional progress");
    check(c.protocol.session()->ticks() == maxRunTicks, "tick ceiling cannot be exceeded");
    c.request("reset");
    check(c.advance(1, false).at("data").at("ticks") == 1, "reset starts a new tick budget");
}
void management() {
    const auto cat = catalog();
    const auto defs = definitions();
    Client c(cat, defs);
    c.start(cat);
    auto state = c.protocol.state();
    const auto inventory = state["party"][0]["inventory"];
    Json stack;
    for (const auto& s : inventory)
        if (s["item"] == "healing_potion")
            stack = s;
    check(!stack.is_null(), "fixture owns potion stack");
    const Json split = {{"kind", "inventory"}, {"operation", "split"}, {"item", stack["handle"]},
                        {"character", 0},      {"cell", 40},           {"quantity", 1}};
    const auto before = c.protocol.state();
    check(c.request("preview", {{"action", split}}).at("data").at("accepted"),
          "inventory preview accepts legal split");
    check(c.protocol.state() == before, "preview does not mutate inventory or revisions");
    check(c.act(split).at("data").at("accepted"), "inventory split commits");
    Json splitHandle;
    const auto afterSplit = c.protocol.state();
    for (const auto& s : afterSplit["party"][0]["inventory"])
        if (s["cell"] == 40)
            splitHandle = s["handle"];
    check(c.act({{"kind", "drop"}, {"item", splitHandle}}).at("data").at("accepted"), "drop commits");
    check(!c.act({{"kind", "drop"}, {"item", splitHandle}}).at("data").at("accepted"),
          "stale item handle cannot drop twice");
    const auto loot = c.protocol.state()["loot"][0]["handle"];
    check(c.act({{"kind", "pickup"}, {"loot", loot}, {"character", 1}}).at("data").at("accepted"),
          "checked pickup commits");
    check(!c.act({{"kind", "pickup"}, {"loot", loot}, {"character", 1}}).at("data").at("accepted"),
          "loot generation/reuse rejects repeat");
    check(!c.act({{"kind", "rest"}, {"type", "short"}, {"dice", {0, 0, 0, 0}}}).at("data").at("accepted"),
          "rest requires benefit");
    check(!c.act({{"kind", "advancement"}, {"character", 0}}).at("data").at("accepted"),
          "no free advancement");
    const auto& g = c.protocol.session()->game();
    bool cast = false;
    for (std::uint16_t i = 0; i < cat.spellCount(); ++i) {
        const auto* s = cat.spell({i});
        if (s->kind != rules::SpellKind::MageArmor || !g.party().spellAccess({3}, {i}))
            continue;
        check(c.act({{"kind", "combat"},
                     {"character", 3},
                     {"action", "spell"},
                     {"spell", s->id},
                     {"allies", {3}}})
                  .at("data")
                  .at("accepted"),
              "legal pre-combat buff casts through runner");
        cast = true;
        c.advance(1000, false);
        check(c.act({{"kind", "combat"},
                     {"action", "feature"},
                     {"character", 3},
                     {"feature", "arcane_recovery"}})
                  .at("data")
                  .at("accepted"),
              "checked feature restores spent mana");
        c.advance(1000, false);
        check(c.act({{"kind", "rest"}, {"type", "long"}}).at("data").at("accepted"),
              "long rest restores spent mana normally");
    }
    check(cast, "fixture exercises a legal buff and long rest");
}
void outcomes() {
    const auto defs = definitions();
    for (bool deadly : {false, true}) {
        const auto cat = catalog(deadly);
        Client c(cat, defs);
        c.start(cat);
        approach(c);
        fight(c, !deadly);
        check(c.protocol.session()->game().combat().snapshot().phase ==
                  (deadly ? CombatPhase::Defeat : CombatPhase::Victory),
              "expected scripted outcome");
        check(!c.act({{"kind", "move"}, {"direction", "forward"}}).at("data").at("accepted"),
              "terminal state rejects actions");
        const auto ticks = c.protocol.session()->ticks();
        c.advance(1000, false);
        check(ticks == c.protocol.session()->ticks(), "terminal does not advance");
        if (!deadly)
            check(c.protocol.session()->game().party().character({0})->experience == 300,
                  "earned XP reaches ordinary advancement threshold");
    }
}
void spellAndPartialVictory() {
    const auto cat = catalog(false, 1200, 1);
    const auto defs = definitions();
    Client c(cat, defs);
    c.start(cat, 4);
    for (unsigned i = 0; i < 8; ++i) {
        c.act({{"kind", "move"}, {"direction", "forward"}});
        c.advance();
    }
    const Json cast = {{"kind", "combat"},         {"action", "spell"},    {"character", 3},
                       {"spell", "magic_missile"}, {"enemies", {0, 1, 2}}, {"darts", {1, 1, 1}}};
    auto invalid = cast;
    invalid["darts"] = {1, 1, 0};
    const auto mana = c.protocol.session()->game().party().character({3})->mana;
    check(!c.act(invalid).at("data").at("accepted"), "invalid compound spell has no cost");
    check(c.protocol.session()->game().party().character({3})->mana == mana, "invalid darts preserve mana");
    check(c.act(cast).at("data").at("accepted"), "compound spell accepts three explicit recipients");
    c.advance(100, false);
    for (unsigned i = 0; i < 3; ++i)
        check(c.protocol.session()->game().actors().snapshot().actors[i].hasDeath,
              "each dart recipient resolves through simulation");
    c.start(cat, 2);
    // Isolate the local-victory boundary with a fixture-owned distant guard. This is
    // test setup through ActorWorld, not an exposed automation mutation or replay input.
    auto& fixture = const_cast<GameSession&>(c.protocol.session()->game());
    for (int x = 9; x >= 1; --x)
        check(bool(fixture.actors().move(fixture.actors().id(1), {x, 7}, {0, 0},
                                         fixture.movement().player.cell)),
              "fixture separates the second encounter");
    for (int y = 6; y >= 1; --y)
        check(bool(fixture.actors().move(fixture.actors().id(1), {1, y}, {0, 0},
                                         fixture.movement().player.cell)),
              "fixture moves guard beyond alert range");
    approach(c);
    fight(c, true, 2000, true);
    check(c.protocol.session()->result() && !c.protocol.session()->terminal(),
          "local victory retains distant guard");
    check(c.advance().at("data").at("stop") == "encounter_result", "partial result pauses advancement");
    check(c.act({{"kind", "continue"}}).at("data").at("accepted"), "continue resumes incomplete arena");
    check(realTime(c.protocol.session()->game().combat().snapshot().phase),
          "continuation resumes exploration");
}
void earnedProgression() {
    const auto cat = catalog(false, 1200, 1);
    const auto defs = definitions();
    Client c(cat, defs);
    c.start(cat, 8);
    for (unsigned i = 0; i < 8; ++i) {
        check(c.act({{"kind", "move"}, {"direction", "forward"}}).at("data").at("accepted"),
              "ranged approach");
        c.advance();
    }
    bool killed = false;
    for (unsigned i = 0; i < 20 && !killed; ++i) {
        auto shot = Json{{"kind", "combat"}, {"character", 2}, {"action", "ranged"}, {"target", 0}};
        auto result = c.act(shot);
        if (!result.at("data").at("accepted"))
            throw std::runtime_error("ranged fixture: " + result.dump());
        c.advance(60, false);
        killed = c.protocol.session()->game().party().character({0})->experience >= 300;
    }
    check(killed, "normal ranged kill earns level-two XP");
    for (unsigned i = 0; i < 10; ++i) {
        const auto moved = c.act({{"kind", "move"}, {"direction", "back"}});
        if (!moved.at("data").at("accepted"))
            throw std::runtime_error("retreat fixture: " + moved.dump());
        c.advance();
    }
    c.advance(1000, false); // Let pursuing enemies finish searching and the shot cooldown settle.
    auto result = c.act({{"kind", "advancement"}, {"character", 0}});
    if (!result.at("data").at("accepted"))
        throw std::runtime_error("advancement fixture: " + result.dump());
    check(c.protocol.session()->game().party().character({0})->build.level == 2,
          "earned fighter advancement commits through runner");
}
void timingParity() {
    const auto cat = catalog();
    const auto defs = definitions();
    const auto s = scenario(cat);
    Session runner(cat, defs, s, Mode::Diagnostic);
    const auto map = arenaMap();
    const auto placements = arenaPlacements(map, cat, s, defs.types);
    GameSession normal(map, cat, arenaParty(cat, s), placements, s.seed, arenaSpawn(), true, defs.timings);
    CommandController commands(normal, defs.timings);
    for (unsigned cycle = 0; cycle < 6000 && !runner.terminal(); ++cycle) {
        const auto& a = runner.tick();
        const auto& b = normal.update(.01, false);
        check(a.count == b.count, "normal scheduling event count parity");
        for (std::size_t i = 0; i < a.count; ++i)
            check(encodeEvent(a.values[i], cat, true) == encodeEvent(b.values[i], cat, true),
                  "normal scheduling normalized event parity");
        Observation obs;
        runner.observe(obs);
        auto targets = obs.targets;
        targets.world = normal.actors().snapshot().world;
        for (std::size_t i = 0; i < targets.count; ++i)
            targets.ids[i] = normal.actors().id(targets.ids[i].slot);
        commands.record(b);
        commands.refresh(targets, normal.movement().transition.active(), false, true);
        const auto& st = runner.game().combat().snapshot();
        if (realTime(st.phase) && !runner.game().movement().transition.active()) {
            ActionRequest action;
            action.operation = Operation::Move;
            action.movement = Action::Forward;
            if (runner.action(action, true).accepted) {
                runner.action(action, false);
                normal.move(Action::Forward);
            }
        } else if (st.awaitingAction) {
            PartyCommand command;
            command.character = *st.activeCharacter;
            command.world = st.world;
            command.turn = st.turn;
            command.target = runner.game().actors().id(0);
            command.action = PartyAction::Melee;
            ActionRequest action;
            action.combat = command;
            if (!runner.action(action, true).accepted) {
                action.combat.action = PartyAction::Defend;
                action.combat.target.reset();
            }
            check(runner.action(action, false).accepted, "script accepts shared player command");
            action.combat.world = normal.actors().snapshot().world;
            action.combat.turn = normal.combat().snapshot().turn;
            if (action.combat.target)
                action.combat.target = normal.actors().id(action.combat.target->slot);
            check(commands.submit(action.combat) == CommandReason::Resolved,
                  "normal command controller accepts matching command");
        }
    }
    check(runner.terminal(), "parity trace completes fight");
    Observation a, b;
    runner.observe(a, true);
    b = a;
    b.session = &normal;
    b.map = &map;
    check(encodeObservation(a) == encodeObservation(b), "normal/controller final state matches runner");
}
void recordings(const char* executable) {
    Directory dir;
    const auto path = dir.path / "run.jsonl";
    const auto cat = catalog();
    const auto defs = definitions();
    const Json header = {{"protocol", 1}, {"timing", "test"}, {"content", "owned-fixture"}};
    {
        TraceWriter writer(path);
        writer.append("header", header);
        Client c(cat, defs);
        c.trace = &writer;
        check(c.start(cat).at("ok"), "recorded start succeeds");
        approach(c);
        fight(c);
        const auto nested = parseRequest(std::string(31, '[') + "0" + std::string(31, ']'));
        check(!c.request("observe", {{"unknown", nested}}).at("ok"), "record maximum-depth rejected request");
        check(!c.request("observe", {{"unknown", std::string(maxRequestBytes - 128, 'x')}}).at("ok"),
              "record near-limit rejected request");
        check(c.request("reset").at("ok"), "recorded reset succeeds");
        check(c.request("close").at("ok"), "recorded close succeeds");
        writer.finish();
    }
    Protocol p(cat, defs, Mode::Diagnostic);
    replay(p, path, header);
    const auto child = fork();
    check(child >= 0, "fork replay subprocess");
    if (child == 0) {
        execl(executable, executable, "--replay-fixture", path.c_str(), nullptr);
        _exit(127);
    }
    int result = 0;
    waitpid(child, &result, 0);
    check(WIFEXITED(result) && WEXITSTATUS(result) == 0, "fresh-process replay");
    bool rejected = false;
    try {
        TraceWriter w(path);
    } catch (...) {
        rejected = true;
    }
    check(rejected, "recording never overwrites");
    rejected = false;
    try {
        Protocol q(cat, defs, Mode::Diagnostic);
        replay(q, path, {{"protocol", 2}});
    } catch (...) {
        rejected = true;
    }
    check(rejected, "compatibility mismatch rejects");
    std::ifstream f(path);
    std::string text((std::istreambuf_iterator<char>(f)), {});
    const auto end = text.rfind("{\"data\":{\"complete\":true}");
    check(end != std::string::npos, "trace footer exists");
    {
        std::ofstream x(dir.path / "truncated");
        x << text.substr(0, end);
    }
    rejected = false;
    try {
        Protocol q(cat, defs, Mode::Diagnostic);
        replay(q, dir.path / "truncated", header);
    } catch (...) {
        rejected = true;
    }
    check(rejected, "incomplete trace rejects");
    auto changed = text;
    const auto time = changed.find("\"time_ms\":\"0\"");
    check(time != std::string::npos, "recorded clock exists");
    changed.replace(time, 13, "\"time_ms\":\"1\"");
    {
        std::ofstream x(dir.path / "changed");
        x << changed;
    }
    rejected = false;
    try {
        Protocol q(cat, defs, Mode::Diagnostic);
        replay(q, dir.path / "changed", header);
    } catch (...) {
        rejected = true;
    }
    check(rejected, "tampered boundary detected");
    rejected = false;
    try {
        TraceWriter w(dir.path / "limited", 4);
        w.append("header", header);
    } catch (...) {
        rejected = true;
    }
    check(rejected, "trace byte exhaustion stops recording");
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string(argv[1]) == "--replay-fixture") {
            const auto c = catalog();
            const auto d = definitions();
            Protocol p(c, d, Mode::Diagnostic);
            replay(p, argv[2], {{"protocol", 1}, {"timing", "test"}, {"content", "owned-fixture"}});
            return 0;
        }
        boundaries();
        tickBudget();
        management();
        outcomes();
        spellAndPartialVictory();
        earnedProgression();
        timingParity();
        recordings(argv[0]);
        std::cout << "Automation protocol, lifecycle, management, outcomes and replay passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
