#include "automation/trace.hpp"
#include "game/commands.hpp"
#include "support/arena_preparation_fixtures.hpp"
#include "support/automation_fixtures.hpp"
#include "support/bard_fixtures.hpp"
#include "support/generation_blueprints.hpp"
#include "support/paladin_fixtures.hpp"
#include "support/newt_fixtures.hpp"
#include "support/ranger_fixtures.hpp"
#include <iostream>
#include <sstream>
#include <sys/wait.h>
using namespace blobber;
using namespace blobber::automation;
using namespace blobber::automation::test;
namespace {
class CoveredCandidates final : public Visibility {
    TargetSnapshot snapshot_;

  public:
    const TargetSnapshot& sample(const Map&, const GameSession& game) override {
        snapshot_ = {};
        snapshot_.world = game.actors().snapshot().world;
        snapshot_.revision = game.actors().snapshot().revision;
        snapshot_.candidates[snapshot_.candidateCount++] = game.actors().id(0);
        return snapshot_; // A target candidate with no body pixels.
    }
    bool lootVisible(LootId) const noexcept override { return false; }
};
void biteWarningWire() {
    const auto cat = blobber::test::newtCatalog();
    const auto map = combatMap();
    const Player pose{{3, 3}, Heading::North};
    const NpcWorld npcs(map, pose.cell, {"test.newt"}, {{{0}, {3, 2}, {0, 1}, Heading::South}});
    GameSession game(map, cat, npcs, 5, pose, false);
    for (unsigned tick = 0; tick < 300; ++tick) {
        for (const auto& event : game.update(.05, false)) {
            if (event.event != CombatEvent::BiteWarning)
                continue;
            Observation o;
            o.session = &game;
            o.map = &map;
            o.visible[0] = true;
            const auto notice = encodeEvent(event, cat, false);
            const auto state = encodeObservation(o);
            check(notice["event"] == "bite_warning" && notice["enemy"] == 0 &&
                      notice["recipient"] == state["enemies"][0]["heavy_bite_target"],
                  "Playable headless warning names the same victim as the persistent observation");
            return;
        }
        const auto& state = game.combat().snapshot();
        if (state.awaitingAction && state.activeCharacter) {
            auto end = turnCommand(game, PartyAction::Feature);
            end.feature = rules::Feature::EndTurn;
            game.submit(end);
        }
    }
    throw std::runtime_error("No headless warning");
}
void restorationWire() {
    const auto c = catalog();
    const auto d = definitions();
    const auto& blueprints = blobber::test::campaignBlueprints();
    auto run = std::make_unique<CampaignRun>(blueprints, c, *c.preset(), d.types, d.timings, 5);
    Session session(blueprints, c, d, std::move(run), Mode::Diagnostic);
    auto& game = session.game();
    auto floor = std::make_unique<FloorState>();
    game.exportFloor(*floor);
    floor->effects.poisonRemaining[0] = 9500;
    game.importFloor(*floor);
    Observation observation;
    session.observe(observation);
    const auto statuses = encodeObservation(observation)["party"][0]["statuses"];
    check(statuses.size() == 1 && statuses[0]["name"] == "Poisoned" &&
              statuses[0]["remaining_ms"] == "9500" && statuses[0]["harmful"],
          "Headless observation exposes the recipient and exact poison clock");
    Json request{{"kind", "combat"}, {"action", "spell"}, {"character", 1}, {"allies", {0}},
                 {"restore_condition", "poison"}};
    check(decodeAction(request, game).combat.restoration == RestorationChoice::Poison,
          "Named poison cure decodes");
    request["restore_paralysis"] = true;
    rejects([&] { decodeAction(request, game); }, "Ambiguous restoration choices reject");
    request.erase("restore_condition");
    check(decodeAction(request, game).combat.restoration == RestorationChoice::Paralysis,
          "Legacy paralysis choice still decodes");
}
void paladinWire() {
    using namespace rules;
    const auto cat = blobber::test::paladinCatalog(); const auto defs = definitions();
    auto arena = defaultArena(cat); arena.adjust({0}, 1); arenaLevel(cat, arena, 4);
    Session session(blobber::test::campaignBlueprints(), cat, defs, arena, Mode::Diagnostic);
    session.game().party().damage({1}, 6);
    Json request{{"kind", "combat"}, {"action", "feature"}, {"character", 0}, {"feature", "lay_on_hands"}, {"allies", {1}}, {"healing_amount", 3}};
    const auto decoded = decodeAction(request, session.game());
    check(decoded.combat.healingAmount == 3 && session.action(decoded, true).accepted, "Healing wire validates amount without spending");
    check(session.game().party().character({0})->layOnHands == 20, "Wire preview does not mutate healing pool");
    check(session.action(decoded, false).accepted && session.game().party().character({0})->layOnHands == 17, "Wire healing commits exact pool amount");
    check(!session.action(decoded, false).accepted, "Wire healing duplicate rejected");
    auto malformed = request; malformed["healing_amount"] = true;
    rejects([&] { decodeAction(malformed, session.game()); }, "Healing wire rejects boolean amount");
    malformed["healing_amount"] = 21;
    rejects([&] { decodeAction(malformed, session.game()); }, "Healing wire rejects oversized amount");
    const auto smite = decodeAction({{"kind", "combat"}, {"action", "smite"}, {"character", 0}, {"target", 0}}, session.game());
    check(smite.combat.action == PartyAction::Smite, "Direct Smite action is wire-visible");
    Observation observation; session.observe(observation);
    const auto state = encodeObservation(observation);
    check(state["party"][0]["lay_on_hands"] == 17, "Observation exposes healing pool");
    const auto descriptors = describeActions(session.game(), observation);
    check(descriptors["combat"][0]["lay_on_hands"]["maximum"] == 20, "Descriptors expose healing limits");
    bool oath = false, ability = false;
    for (const auto& spell : descriptors["combat"][0]["spells"]) oath |= spell["id"] == "protection_from_evil_and_good";
    for (const auto& feature : descriptors["combat"][0]["features"]) ability |= feature["id"] == "divine_smite";
    check(oath && ability, "Descriptors include high-bit oath spell and Smite");
    CombatOutcome impact; impact.event = CombatEvent::Attack; impact.hit = true; impact.smiteDamage = 9;
    impact.damageComponents[0] = {4, 4, DamageAdjustment::Normal};
    impact.damageComponents[1] = {9, 9, DamageAdjustment::Normal};
    const auto event = encodeEvent(impact, cat, true);
    check(event["damage_components"].size() == 2 && event["damage_components"][1]["type"] == "radiant", "Wire feedback keeps damage types separate");
}
void barbarianWire() {
    auto cat = blobber::test::barbarianCatalog();
    auto arena = defaultArena(cat);
    arena.adjust({0}, 1);
    arenaLevel(cat, arena, 4);
    auto defs = definitions();
    const auto play = [&] {
        Session s(blobber::test::campaignBlueprints(), cat, defs, decodeArena(cat, encodeArena(cat, arena)),
                  Mode::Diagnostic);
        const auto rage = decodeAction(
            {{"kind", "combat"}, {"character", 0}, {"action", "feature"}, {"feature", "rage"}}, s.game());
        check(s.action(rage, true).accepted && s.game().party().rageUses({0}) == 3,
              "Rage preview preserves uses");
        check(s.action(rage, false).accepted && !s.action(rage, false).accepted, "Rage commits once");
        Observation o;
        s.observe(o);
        const auto hero = encodeObservation(o).at("party")[0];
        check(hero.at("rage_uses") == 2 && hero.at("rage_until_ms") == "60000" && hero.at("frenzy_unspent"),
              "Headless prepared Berserker observes Rage");
        return hero;
    };
    check(play() == play(), "Fresh seeded Barbarian sessions reproduce the same state");
}
void bardWire() {
    const auto cat = blobber::test::bardCatalog();
    const auto defs = definitions();
    auto arena = defaultArena(cat);
    arena.adjust({0}, 1);
    arenaLevel(cat, arena, 3);
    arena.preparation.advancements[1][0].learned = rules::spellBit(cat.findSpell("shatter"));
    const auto resolve = [&](const Json& expected) -> Json {
        Session s(blobber::test::campaignBlueprints(), cat, defs, arena, Mode::Diagnostic);
        auto& g = s.game();
        const auto grant = decodeAction({{"kind", "combat"},
                                         {"character", 0},
                                         {"action", "feature"},
                                         {"feature", "bardic_inspiration"},
                                         {"allies", {1}}},
                                        g);
        check(s.action(grant, false).accepted, "Headless Bard grants Inspiration");
        const auto cell = g.movement().player.cell;
        const auto cast = decodeAction({{"kind", "combat"},
                                        {"character", 0},
                                        {"action", "spell"},
                                        {"spell", "shatter"},
                                        {"cell", Json::array({cell.x, cell.y})}},
                                       g);
        check(s.action(cast, false).accepted, "Headless Bard casts known Shatter");
        if (!g.pendingRollDecision().serial)
            return nullptr;
        Observation o;
        s.observe(o);
        const auto roll = encodeObservation(o).at("pending_roll");
        if (!expected.is_null())
            check(roll == expected.at("pending"), "Same seed reproduces pending save");
        check(roll.at("purpose") == "save" && roll.at("serial").is_string() && roll.at("character") == 1 &&
                  describeActions(g, o).contains("resolve_roll"),
              "Headless player can discover the pending save");
        const auto answer =
            decodeAction({{"kind", "resolve_roll"}, {"serial", roll.at("serial")}, {"spend", true}}, g);
        const auto before = s.ticks();
        const auto now = g.timeMs();
        s.tick();
        check(s.ticks() == before && g.timeMs() == now && s.action(answer, true).accepted &&
                  g.party().character({1})->inspirationUntil,
              "Preview and tick preserve held roll");
        check(s.action(answer, false).accepted && !s.action(answer, false).accepted,
              "Wire answer commits once");
        s.observe(o);
        return {{"pending", roll}, {"spent", encodeObservation(o).at("last_inspiration")}};
    };
    for (arena.seed = 1; arena.seed <= 48; ++arena.seed) {
        const auto result = resolve(nullptr);
        if (!result.is_null()) {
            check(resolve(result) == result, "Same seed and answer reproduce the full Bard decision");
            return;
        }
    }
    check(false, "Headless Bard fixture reaches an eligible save");
}
void rangerWire() {
    const auto cat = blobber::test::rangerCatalog();
    const auto defs = definitions();
    auto arena = defaultArena(cat);
    arena.adjust({0}, 1);
    arenaLevel(cat, arena, 3);
    Session session(blobber::test::campaignBlueprints(), cat, defs, arena, Mode::Diagnostic);
    session.game().party().damage({0}, 5);
    const auto action = decodeAction({{"kind", "combat"}, {"action", "spell"}, {"character", 0},
                                      {"spell", "goodberry"}}, session.game());
    check(session.action(action, true).accepted && session.game().party().character({0})->mana == 6,
          "Headless Goodberry preview does not mutate resources");
    check(session.action(action, false).accepted && session.game().party().character({0})->mana == 4 &&
              !session.action(action, false).accepted, "Headless Goodberry commits once");
    Observation observation;
    session.observe(observation);
    const auto hero = encodeObservation(observation)["party"][0];
    check(hero["learned"].size() == 3 && hero["prepared"].empty() &&
              hero["passive_features"] == Json::array({"archery", "colossus_slayer"}),
          "Headless observation exposes known spells and Ranger passive features");
    const auto actions = describeActions(session.game(), observation);
    bool goodberry = false, mark = false;
    for (const auto& spell : actions["combat"][0]["spells"]) {
        goodberry |= spell["id"] == "goodberry" && spell.value("party", false);
        mark |= spell["id"] == "hunters_mark" && spell["hostile"] == true && spell["max_targets"] == 1;
    }
    check(goodberry && mark, "Headless descriptors expose the party heal and single-enemy mark");
}
void preciseAimProtocol() {
    const auto cat = catalog();
    const auto defs = definitions();
    Session runner(blobber::test::campaignBlueprints(), cat, defs, scenario(cat), Mode::Diagnostic);
    Json request = {{"kind", "combat"},
                    {"action", "spell"},
                    {"character", 3},
                    {"spell", "sleep"},
                    {"aim_subcell", {{"x", 20}, {"y", 20}}}};
    const auto decoded = decodeAction(request, runner.game());
    check(decoded.combat.aimedSubcell == std::optional<SpellAim>{{20, 20}} && !decoded.combat.aimedCell,
          "Protocol preserves precise grid intersections");
    request["cell"] = Json::array({10, 10});
    rejects([&] { decodeAction(request, runner.game()); }, "Protocol rejects ambiguous mixed aiming");
    request.erase("cell");
    request["aim_subcell"]["x"] = -1;
    rejects([&] { decodeAction(request, runner.game()); }, "Protocol rejects invalid subcell coordinates");
    CombatOutcome event;
    event.event = CombatEvent::Wake;
    event.enemy = runner.game().actors().id(0);
    event.awakened = {1, event.enemy.world};
    const auto encoded = encodeEvent(event, cat, true);
    check(encoded.at("event") == "wake" && encoded.at("enemy") == 0 && encoded.at("awakened") == 1,
          "Wake event exposes rescuer and recipient identities");
}
void coveredObservations() {
    const auto c = catalog();
    const auto d = definitions();
    Session session(blobber::test::campaignBlueprints(), c, d, scenario(c), Mode::Visual,
                    [](void*, const Map&, const GameSession&) -> std::unique_ptr<Visibility> {
                        return std::make_unique<CoveredCandidates>();
                    });
    Observation o;
    session.observe(o);
    check(o.targets.count == 0 && o.targets.candidateCount == 1 && o.visible[0],
          "Visual observations retain covered command candidates separately from body pixels");
    const auto encoded = encodeObservation(o);
    check(encoded.at("party_gold") == std::to_string(session.game().party().gold()),
          "Player observations expose lossless shared gold outside inventories");
    check(encoded.at("targets") == Json::array({0}) && encoded.at("enemies").size() == 1 &&
              encoded.at("enemies")[0].at("targetable"),
          "Covered target is discoverable in player observations");
    const auto actions = describeActions(session.game(), o);
    check(actions.at("targets") == Json::array({0}), "Headless action targets include covered candidate");
}
void preparedArena() {
    const auto c = catalog();
    const auto d = definitions();
    auto s = scenario(c);
    arenaLevel(c, s, 3);
    const auto input = Json::parse(encodeArena(c, s));
    Client client(c, d);
    auto started = client.request("start", {{"scenario", input}});
    check(started.at("ok"), "prepared scenario starts through protocol");
    for (const auto& member : started.at("data").at("party"))
        check(member.at("level") == 3 && member.at("hp") == member.at("max_hp") &&
                  member.at("mana") == member.at("max_mana"),
              "headless uses prepared full resources");
    const auto previous = client.protocol.state();
    auto invalid = input;
    invalid["preparation"]["advancements"][0][3]["learned"] = Json::array();
    check(!client.request("start", {{"scenario", invalid}}).at("ok"),
          "invalid preparation rejects replacement");
    check(client.protocol.state() == previous, "failed preparation preserves active session");
    const auto reset = client.request("reset");
    check(reset.at("ok"), "prepared arena reset");
    check(reset.at("data").at("party") == previous.at("party"), "reset reproduces prepared party exactly");
}
void casterSourceEvents() {
    const auto cat = catalog();
    CombatOutcome e;
    e.event = CombatEvent::Effect;
    e.effect.kind = EffectKind::Bless;
    e.effect.caster = ActorId{2, 42};
    e.effect.target = ActorId{1, 42};
    const auto npc = encodeEvent(e, cat, false);
    check(npc.at("effect").at("caster").is_null() &&
              npc.at("effect").at("caster_source") == Json{{"kind", "npc"}, {"actor", 2}} &&
              npc.at("effect").at("target") == 1,
          "NPC effect source is tagged and never masquerades as a party caster");
    e.effect.caster = rules::CharacterId{2};
    const auto party = encodeEvent(e, cat, false);
    check(party.at("effect").at("caster") == 2 &&
              party.at("effect").at("caster_source") == Json{{"kind", "party"}, {"character", 2}},
          "Party events retain numeric caster and additive tagged source");
    e.event = CombatEvent::Cast;
    e.enemyAttack = true;
    e.enemy = {2, 42};
    e.spell = rules::SpellKind::SacredFlame;
    const auto cast = encodeEvent(e, cat, false);
    check(cast.at("caster_source") == Json{{"kind", "npc"}, {"actor", 2}} && cast.at("roll") == 0 &&
              cast.at("save_total") == 0 && cast.at("damage") == 0,
          "Normal enemy cast event identifies actor without invented party identity or rolls");
}

void packEvents() {
    const auto cat = catalog();
    CombatOutcome event;
    event.event = CombatEvent::Attack;
    event.enemyAttack = true;
    event.packTactics = true;
    event.attackRollMode = rules::RollMode::Advantage;
    const auto encoded = encodeEvent(event, cat, false);
    check(encoded.at("pack_tactics") == true && encoded.at("attack_roll_mode") == "advantage",
          "Resolved NPC weapon event exposes accepted Pack Tactics context");
    event.attackRollMode = rules::RollMode::Normal;
    check(encodeEvent(event, cat, true).at("attack_roll_mode") == "normal",
          "Diagnostic event reports cancellation as a normal roll");
    event.attackRollMode = rules::RollMode::Disadvantage;
    event.packTactics = false;
    check(encodeEvent(event, cat, false).at("attack_roll_mode") == "disadvantage",
          "Ordinary NPC weapon disadvantage is explicit");
    event.event = CombatEvent::AttackStarted;
    check(encodeEvent(event, cat, false).is_null() && !encodeEvent(event, cat, true).contains("pack_tactics"),
          "Acceptance does not publish hidden pack result");
    event.event = CombatEvent::AttackCanceled;
    check(!encodeEvent(event, cat, false).contains("attack_roll_mode"),
          "Canceled attacks do not publish roll context");
    event.event = CombatEvent::Attack;
    event.enemyAttack = false;
    check(!encodeEvent(event, cat, false).contains("pack_tactics"), "Party event wire shape stays unchanged");
    event.enemyAttack = true;
    event.spell = rules::SpellKind::FireBolt;
    check(!encodeEvent(event, cat, false).contains("pack_tactics"), "NPC spell event has no weapon metadata");
}

void fortitudeEvents() {
    const auto cat = catalog();
    CombatOutcome event;
    event.event = CombatEvent::Attack;
    event.fortitude = {FortitudeStatus::Saved, 13, 16, 9};
    const auto encoded = encodeEvent(event, cat, false);
    check(encoded.at("undead_fortitude") ==
              Json{{"outcome", "saved"}, {"roll", 13}, {"total", 16}, {"difficulty", 9}},
          "Resolved impact exposes Fortitude save independently of attack/spell save rolls");
    event.fortitude.status = FortitudeStatus::Failed;
    event.fortitude.difficulty = INT64_MAX;
    check(encodeEvent(event, cat, true).at("undead_fortitude").at("difficulty") == INT64_MAX,
          "Fortitude DC retains 64-bit precision");
    event.fortitude = {FortitudeStatus::RadiantBypass};
    check(encodeEvent(event, cat, false).at("undead_fortitude") == Json{{"outcome", "radiant_bypass"}},
          "Bypass reports its reason without an invented save");
    for (const auto type : {CombatEvent::AttackStarted, CombatEvent::ProjectileReleased,
                            CombatEvent::AttackCanceled, CombatEvent::SpellSave}) {
        event.event = type;
        check(!encodeEvent(event, cat, true).contains("undead_fortitude"),
              "Unresolved events never reveal Fortitude context");
    }
    event.event = CombatEvent::Attack;
    event.fortitude = {};
    check(!encodeEvent(event, cat, false).contains("undead_fortitude"),
          "Unaffected event wire shape remains unchanged");
}
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
    c.start(cat, 2);
    // One distant survivor keeps the arena active without making this progression
    // contract depend on a pursuing pack's search/return timing.
    auto& fixture = c.protocol.replaySession()->game();
    for (int x = 9; x >= 1; --x)
        check(bool(fixture.actors().move(fixture.actors().id(1), {x, 7}, {0, 0},
                                         fixture.movement().player.cell)),
              "progression fixture isolates surviving guard");
    for (int y = 6; y >= 1; --y)
        check(bool(fixture.actors().move(fixture.actors().id(1), {1, y}, {0, 0},
                                         fixture.movement().player.cell)),
              "progression fixture keeps survivor outside alert range");
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
    for (unsigned attempt = 0; attempt < 3; ++attempt) {
        c.advance(700, false); // Settle the shot/corpse tail, which may publish a local victory.
        if (!c.protocol.session()->result()) break;
        check(c.act({{"kind", "continue"}}).at("data").at("accepted"),
              "resume active arena after local victory");
    }
    auto result = c.act({{"kind", "advancement"}, {"character", 0}});
    if (!result.at("data").at("accepted"))
        throw std::runtime_error("advancement fixture: " + result.dump());
    check(c.protocol.session()->game().party().character({0})->build.level == 2,
          "earned fighter advancement commits through runner");
}
void levelFourWire() {
    const auto cat = catalog();
    const auto defs = definitions();
    auto arena = scenario(cat);
    arenaLevel(cat, arena, 3);
    Session runner(blobber::test::campaignBlueprints(), cat, defs, arena, Mode::Diagnostic);
    auto& game = runner.game();
    game.party().grantNextLevelExperience();
    Observation observed;
    runner.observe(observed, true);
    const auto descriptor = describeActions(game, observed).at("combat").at(3).at("advancement");
    check(descriptor.at("ability_points") == 2 && descriptor.at("cantrip_min") == 1,
          "wire descriptors expose mandatory level-four choices");
    const auto& c = *game.party().character({3});
    rules::AdvancementChoices choices;
    choices.abilityImprovement.increases[3] = 2;
    auto context =
        rules::advancementChoices(cat, c.build.view(), c.build.portrait, c.build.spells, choices, true);
    choices.learned = rules::recommendedChoices(cat, context, rules::SpellChoice::Learned);
    context = rules::advancementChoices(cat, c.build.view(), c.build.portrait, c.build.spells, choices,
                                        rules::SpellChoice::Cantrips);
    choices.cantrips = rules::recommendedChoices(cat, context, rules::SpellChoice::Cantrips);
    Json input = {{"kind", "advancement"},
                  {"character", 3},
                  {"learned", spellList(cat, choices.learned)},
                  {"prepared", Json::array()},
                  {"cantrips", spellList(cat, choices.cantrips)},
                  {"ability_improvement", {0, 0, 0, 2, 0, 0}}};
    for (const auto& bad : {Json::array({0, 0}), Json::array({0, 0, 0, 3, 0, 0}),
                            Json::array({0, 0, 0, -1, 0, 0}), Json::array({0, 0, 0, true, 0, 0})}) {
        auto invalid = input;
        invalid["ability_improvement"] = bad;
        rejects([&] { decodeAction(invalid, game); }, "wire rejects malformed ability arrays");
    }
    auto incomplete = input;
    incomplete.erase("ability_improvement");
    check(!runner.action(decodeAction(incomplete, game), false).accepted && c.build.level == 3,
          "omitted ASI cannot bypass authoritative advancement");
    const auto decoded = decodeAction(input, game);
    check(runner.action(decoded, true).accepted && c.build.level == 3,
          "wire preview accepts complete choices without advancing");
    check(runner.action(decoded, false).accepted && c.build.level == 4,
          "wire commits full level-four caster advancement");
    runner.observe(observed, true);
    const auto result = encodeObservation(observed).at("party").at(3);
    check(result.at("level_four_asi") == input.at("ability_improvement") && result.at("max_mana") == 17,
          "wire observation retains earned improvements and mana cap");
    check(!runner.action(decoded, false).accepted, "wire cannot replay the fourth-level commit");
}
void timingParity() {
    const auto cat = catalog();
    const auto defs = definitions();
    const auto s = scenario(cat);
    Session runner(blobber::test::campaignBlueprints(), cat, defs, s, Mode::Diagnostic);
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
        for (std::size_t i = 0; i < targets.candidateCount; ++i)
            targets.candidates[i] = normal.actors().id(targets.candidates[i].slot);
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
    Protocol p(blobber::test::campaignBlueprints(), cat, defs, Mode::Diagnostic);
    replay(p, path, header);
    struct Observer final : ReplayObserver {
        std::size_t calls = 0;
        std::string firstSession;
        bool reset = false, mutate = false;
        void boundary(Session* session, std::size_t operation, const Json& state) override {
            check(operation == ++calls, "observer receives consecutive checked operation boundaries");
            if (firstSession.empty())
                firstSession = state.at("session").get<std::string>();
            reset |= state.at("session") != firstSession;
            if (mutate && session)
                session->replayGame().move(Action::TurnLeft);
        }
    } observer;
    Protocol observed(blobber::test::campaignBlueprints(), cat, defs, Mode::Diagnostic);
    replay(observed, path, header, &observer);
    check(observer.calls > 4 && observer.reset, "observer covers reset across a complete recording");
    Observer mutation;
    mutation.mutate = true;
    Protocol guarded(blobber::test::campaignBlueprints(), cat, defs, Mode::Diagnostic);
    bool mutated = false;
    try {
        replay(guarded, path, header, &mutation);
    } catch (const std::runtime_error& e) {
        mutated = std::string(e.what()).find("observer mutated state") != std::string::npos;
    }
    check(mutated, "observer cannot mutate replay state unnoticed");
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
        Protocol q(blobber::test::campaignBlueprints(), cat, defs, Mode::Diagnostic);
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
        Protocol q(blobber::test::campaignBlueprints(), cat, defs, Mode::Diagnostic);
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
        Protocol q(blobber::test::campaignBlueprints(), cat, defs, Mode::Diagnostic);
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
            Protocol p(blobber::test::campaignBlueprints(), c, d, Mode::Diagnostic);
            replay(p, argv[2], {{"protocol", 1}, {"timing", "test"}, {"content", "owned-fixture"}});
            return 0;
        }
        biteWarningWire();
        restorationWire();
        paladinWire();
        rangerWire();
        barbarianWire();
        bardWire();
        preciseAimProtocol();
        coveredObservations();
        preparedArena();
        casterSourceEvents();
        packEvents();
        fortitudeEvents();
        boundaries();
        tickBudget();
        management();
        outcomes();
        spellAndPartialVictory();
        earnedProgression();
        levelFourWire();
        timingParity();
        recordings(argv[0]);
        std::cout << "Automation protocol, lifecycle, management, outcomes and replay passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
