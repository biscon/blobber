#include "game/campaign.hpp"
#include "game/commands.hpp"
#include "support/automation_fixtures.hpp"
#include <iostream>
#include <queue>
using namespace blobber;
using namespace blobber::automation;
using namespace blobber::automation::test;
namespace {
FixtureRequest stair(const GameSession& g, FixtureKind kind) {
    for (unsigned i = 0; i < g.fixtures().definition().count; ++i)
        if (g.fixtures().definition().entries[i].kind == kind)
            return {g.fixtures().id(i), g.fixtures().revision()};
    throw std::runtime_error("Missing fixture");
}
void face(GameSession& g, Heading h) {
    while (g.movement().player.heading != h)
        g.move(Action::TurnRight);
}
void walk(GameSession& g, Cell to) {
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
            if (map.solid(next) || parent[index(next)] >= 0)
                continue;
            const auto f = g.fixtures().at(next);
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
        if (fixture >= 0 && g.fixtures().definition().entries[unsigned(fixture)].kind == FixtureKind::Door)
            check(g.interact({g.fixtures().id(unsigned(fixture)), g.fixtures().revision()}).accepted,
                  "open traversed door");
        g.move(Action::Forward);
        check(g.movement().player.cell == *i, "checked fixture movement");
    }
}
void kill(GameSession& g) {
    for (std::size_t i = 0; i < g.actors().snapshot().count; ++i)
        g.actors().damage(g.actors().id(i), 100000, g.timeMs());
    g.update(0, false);
}
void travel(CampaignRun& run, FixtureKind kind) {
    auto& g = run.game();
    auto r = stair(g, kind);
    const auto c = g.fixtures().find(r.id)->cell;
    walk(g, c);
    r = stair(g, kind);
    auto ticket = run.prepareTravel(run.request(r));
    check(run.commitTravel(ticket), "checked travel commits");
}
void campaign() {
    const auto cat = catalog(false, 50, 7);
    const auto defs = definitions();
    CampaignRun run(cat, *cat.preset(), defs.types, defs.timings, 5);
    check(run.activeLocation() == 0 && run.status() == CampaignStatus::Active, "campaign starts in hub");
    CommandController hubUi(run.game(), defs.timings);
    hubUi.refresh({}, false, false, true);
    check(hubUi.snapshot().campaign &&
              std::string_view(hubUi.snapshot().fixturePrompt.data()).find("R: rest") !=
                  std::string_view::npos,
          "hub exposes sanctuary recovery alongside stair interaction");
    auto* party = &run.party();
    const auto inventory = party->inventory().identity();
    const auto initialHp = party->character({0})->hitPoints;
    party->damage({0}, 2);
    auto request = run.request(run.game().nearbyFixture());
    check(run.previewTravel(request).accepted, "hub entrance preview");
    auto ticket = run.prepareTravel(request);
    check(run.activeLocation() == 0 && party->character({0})->hitPoints == initialHp - 2,
          "preparation leaves source and party intact");
    check(run.commitTravel(ticket), "enter floor one");
    check(run.activeLocation() == 1 && &run.game().party() == party &&
              party->inventory().identity() == inventory,
          "one persistent party and inventory");
    check(!run.commitTravel(ticket) && !run.previewTravel(request).accepted,
          "repeat and stale source rejected");
    const auto enemy = run.game().actors().id(0);
    kill(run.game());
    const auto xp = party->character({0})->experience;
    const auto loot = run.game().loot().snapshot();
    EffectRequest buff;
    buff.world = run.game().actors().snapshot().world;
    buff.kind = EffectKind::Aid;
    buff.caster = {0};
    buff.targets[0] = rules::CharacterId{0};
    buff.count = 1;
    check(bool(run.game().applyEffect(buff)), "party buff fixture applies");
    const auto max = party->character({0})->stats.maxHitPoints;
    const auto now = run.gameplayMs();
    travel(run, FixtureKind::Upstairs);
    check(run.activeLocation() == 0 && run.gameplayMs() == now, "free ascent consumes no time");
    check(run.game().effects().has(rules::CharacterId{0}, EffectKind::Aid) &&
              party->character({0})->stats.maxHitPoints == max,
          "Aid travels without double application");
    run.update(1, false);
    travel(run, FixtureKind::Downstairs);
    check(run.activeLocation() == 1 && run.game().objective().completed, "objective restored");
    check(!run.game().actors().actor(enemy) && run.game().actors().snapshot().actors[0].hasDeath,
          "fresh runtime identity and retained deaths");
    check(party->character({0})->experience == xp, "no repeated XP");
    for (std::size_t i = 0; i < lootCapacity; ++i)
        check(run.game().loot().snapshot().stacks[i].active == loot.stacks[i].active, "loot ledger retained");
    // All deeper travel uses checked movement, permanent doors and the required quest key.
    travel(run, FixtureKind::Downstairs);
    check(run.activeLocation() == 2, "descend to second floor");
    kill(run.game());
    for (unsigned i = 0; i < run.game().fixtures().definition().count; ++i)
        if (run.game().fixtures().definition().entries[i].kind == FixtureKind::Key) {
            walk(run.game(), run.game().fixtures().definition().entries[i].cell);
            check(
                run.game().interact({run.game().fixtures().id(i), run.game().fixtures().revision()}).accepted,
                "collect bronze key");
        }
    travel(run, FixtureKind::Upstairs);
    check(run.activeLocation() == 1 && run.game().landingSafety(1).safe,
          "return to protected objective landing");
    travel(run, FixtureKind::Downstairs);
    travel(run, FixtureKind::Downstairs);
    check(run.activeLocation() == 3 && run.status() == CampaignStatus::Active,
          "final floor is not automatic victory");
    auto& final = run.game();
    const auto ordinaryId = final.actors().id(0);
    const auto ordinary = *final.actors().actor(ordinaryId);
    bool walking = false;
    for (unsigned d = 0; d < 4 && !walking; ++d) {
        const auto next = ordinary.npc.cell + forward(Heading(d));
        if (final.geometry().npcAllowed(next) && !final.actors().occupied(next))
            walking = final.actors()
                          .walk(ordinaryId, next, ordinary.npc.subcell, final.movement().player.cell,
                                final.timeMs())
                          .changed;
    }
    check(walking, "final lifecycle fixture has an existing NPC presentation tail");
    for (unsigned i = 0; i < final.objective().count; ++i)
        final.actors().damage(final.objective().members[i], 100000, final.timeMs());
    final.update(0, false);
    check(run.status() == CampaignStatus::Active, "final result waits for accepted presentation to settle");
    const auto pose = final.movement().player;
    const auto aiTime = final.ai().timeMs;
    final.move(Action::TurnLeft);
    final.update(.1, false);
    check(final.movement().player == pose && final.ai().timeMs == aiTime,
          "closing campaign drains existing presentation without new movement or AI actions");
    final.update(.6, false);
    check(final.actors().snapshot().actors[0].life == ActorLife::Alive &&
              run.status() == CampaignStatus::Victory,
          "final designated guardian completion wins without an unrelated clear");
    check(!run.previewTravel(run.request(stair(run.game(), FixtureKind::Upstairs))).accepted,
          "terminal travel rejects");
    std::cout << "snapshot_bytes=" << run.snapshotBytes() << '\n';
}
void transferFailuresAndEffects() {
    const auto cat = catalog(false, 50, 7);
    const auto defs = definitions();
    CampaignRun run(cat, *cat.preset(), defs.types, defs.timings, 8);
    auto enter = [&] {
        auto t = run.prepareTravel(run.request(run.game().nearbyFixture()));
        check(run.commitTravel(t), "fixture enters adjacent floor");
    };
    run.game().setAnimated(false);
    auto staged = run.prepareTravel(run.request(run.game().nearbyFixture()));
    run.party().damage({0}, 1);
    check(!run.commitTravel(staged), "party mutation invalidates prepared ticket");
    staged = run.prepareTravel(run.request(run.game().nearbyFixture()));
    run.game().move(Action::TurnLeft);
    check(!run.commitTravel(staged), "pose mutation invalidates prepared ticket");
    face(run.game(), Heading::East);
    // face helper requires animation disabled to finish each turn.
    enter();
    auto& g = run.game();
    const auto oldWorld = g.actors().snapshot().world;
    EffectRequest effect;
    effect.world = oldWorld;
    effect.caster = {0};
    effect.count = 1;
    effect.kind = EffectKind::Bless;
    effect.targets[0] = rules::CharacterId{0};
    check(bool(g.applyEffect(effect)), "portable concentration applies");
    const auto oldEffect = g.effects().concentration({0});
    effect.kind = EffectKind::MirrorImage;
    check(bool(g.applyEffect(effect)), "mirror fixture applies");
    effect.kind = EffectKind::SpiritualWeapon;
    const auto weapon = g.applyEffect(effect);
    check(bool(weapon) && !run.previewTravel(run.request(g.nearbyFixture())).accepted,
          "local weapon blocks travel");
    PartyCommand dismiss;
    dismiss.world = oldWorld;
    dismiss.turn = g.combat().snapshot().turn;
    dismiss.character = {0};
    dismiss.action = PartyAction::Feature;
    dismiss.feature = rules::Feature::DismissSpiritualWeapon;
    check(g.submit(dismiss).reason == CommandReason::Resolved, "explicit spectral dismissal is checked");
    // Non-concentration local area and NPC conditions freeze while the party leaves.
    auto& effects = const_cast<EffectController&>(g.effects());
    CombatEvents notices;
    effects.condition(g.actors().id(0), EffectKind::Sleep, {0}, g.timeMs(), notices);
    const auto localCell = run.location(1).generated->layout.optionalSupply;
    check(effects.createArea(localCell, {0}, 10, g.timeMs(), notices).serial != 0, "local grease fixture");
    PartyCommand cast;
    cast.world = oldWorld;
    cast.turn = g.combat().snapshot().turn;
    cast.character = {3};
    cast.action = PartyAction::Spell;
    cast.targetCount = 1;
    cast.allies[0] = {3};
    for (std::uint16_t i = 0; i < cat.spellCount(); ++i)
        if (cat.spell({i})->kind == rules::SpellKind::MageArmor)
            cast.spell = {i};
    check(g.submit(cast).reason == CommandReason::Resolved,
          "legal buff establishes an actual action cooldown");
    run.update(2, false);
    const auto cooldown = g.combat().snapshot().cooldownMs[3];
    check(cooldown > 0, "travel fixture still has a cooldown");
    check(run.party().grantTemporary({2}, 7, g.combat().snapshot().gameplayMs + 60000),
          "owned temporary HP deadline");
    auto before = std::make_unique<FloorState>();
    g.exportFloor(*before);
    travel(run, FixtureKind::Upstairs);
    check(run.game().effects().concentration({0}).serial && !run.game().removeEffect(oldEffect),
          "portable concentration gets fresh identity");
    check(run.game().effects().has(rules::CharacterId{0}, EffectKind::MirrorImage), "mirror follows party");
    check(run.game().combat().snapshot().cooldownMs[3] == cooldown &&
              run.party().character({2})->temporaryHitPoints == 7,
          "action cooldown and temporary HP follow the party without reset");
    run.party().damage({0}, 1);
    RestRequest rest;
    rest.kind = RestKind::Long;
    rest.world = run.game().actors().snapshot().world;
    rest.token = run.game().restToken();
    const auto hp = run.party().character({0})->hitPoints;
    const auto clock = run.gameplayMs();
    check(bool(run.game().rest(rest)), "hub explicit sanctuary long rest");
    check(run.party().character({0})->hitPoints > hp && run.gameplayMs() == clock + 28800000,
          "rest advances campaign time exactly once");
    check(!run.game().effects().has(rules::CharacterId{0}, EffectKind::Bless) &&
              !run.game().effects().has(rules::CharacterId{0}, EffectKind::MirrorImage),
          "party durations expire in hub");
    check(run.party().character({2})->temporaryHitPoints == 0 &&
              run.game().combat().snapshot().cooldownMs[3] == 0,
          "rest expires temporary HP and readiness deadlines once on campaign time");
    enter();
    auto after = std::make_unique<FloorState>();
    run.game().exportFloor(*after);
    check(after->timeMs == before->timeMs && after->effects.gameplayMs == before->effects.gameplayMs,
          "inactive floor clocks frozen through rest");
    check(after->effects.ground.areas[0].untilMs == before->effects.ground.areas[0].untilMs &&
              after->effects.conditions[4][0].active,
          "local area and NPC condition retained");
    std::ostringstream a, b;
    a << before->ai.rng;
    b << after->ai.rng;
    check(a.str() == b.str(), "AI RNG unchanged while inactive");
    a.str("");
    b.str("");
    before->rng.describe(a);
    after->rng.describe(b);
    check(a.str() == b.str(), "combat RNG restored without draws");
    // Malformed destination and threatened landing both fail without changing current run.
    travel(run, FixtureKind::Upstairs);
    auto& saved = *run.location(1).saved;
    const auto count = saved.actors.count;
    saved.actors.count = 257;
    const auto sourceWorld = run.game().actors().snapshot().world;
    bool rejected = false;
    try {
        (void)run.prepareTravel(run.request(run.game().nearbyFixture()));
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected && run.activeLocation() == 0 && run.game().actors().snapshot().world == sourceWorld,
          "invalid snapshot rolls back");
    saved.actors.count = count;
    saved.effects.ground.areas[0].cell = run.location(1).entry.cell;
    rejected = false;
    try {
        (void)run.prepareTravel(run.request(run.game().nearbyFixture()));
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected && run.activeLocation() == 0, "restored landing hazard rejects travel");
    // Defeat wins if final objective and party wipe coexist.
    for (std::uint16_t i = 0; i < 4; ++i)
        run.party().damage({i}, 100000);
    check(run.status() == CampaignStatus::Defeat, "party wipe ends campaign immediately");
}
void inventoryAndLifecycle() {
    const auto cat = catalog(false, 50, 7);
    const auto defs = definitions();
    CampaignRun run(cat, *cat.preset(), defs.types, defs.timings, 5);
    auto* owner = &run.party();
    const auto id = owner->inventory().identity();
    owner->awardExperience(1200);
    const auto advancement = run.game().advancementRequest({0});
    check(bool(run.game().advance(advancement)), "checked warrior advancement before travel");
    const auto level = owner->character({0})->build.level;
    check(owner->swapFormation(FormationSlot::FrontLeft, FormationSlot::BackRight),
          "formation changes outside combat");
    const auto formation = owner->snapshot().formation;
    // Finite supplies can be exhausted; ordinary stairs still work.
    for (std::uint16_t c = 0; c < 4; ++c)
        for (std::uint16_t b = 0; b < bagCells; ++b) {
            const auto h = owner->inventory().at({{c}, b});
            const auto* item = owner->inventory().stack(h);
            if (item && cat.item(item->item)->kind == rules::ItemKind::Supply)
                check(owner->consumeItem(h, item->quantity), "consume owned finite camp supplies");
        }
    walk(run.game(), {4, 5});
    ItemHandle dropped;
    for (std::uint16_t b = 0; b < bagCells; ++b) {
        const auto h = owner->inventory().at({{0}, b});
        if (owner->inventory().stack(h)) {
            dropped = h;
            break;
        }
    }
    check(bool(dropped), "owned drop fixture has gear");
    const auto stack = *owner->inventory().stack(dropped);
    check(bool(run.game().dropItem(dropped, owner->inventory().revision())),
          "checked hub drop outside protected landing");
    LootId oldLoot;
    for (unsigned i = lootReservedCapacity; i < lootCapacity; ++i)
        if (run.game().loot().snapshot().stacks[i].active) {
            oldLoot = {run.game().loot().snapshot().world, std::uint16_t(i),
                       run.game().loot().snapshot().stacks[i].generation};
            break;
        }
    walk(run.game(), {4, 4});
    face(run.game(), Heading::East);
    travel(run, FixtureKind::Downstairs);
    check(owner->inventory().identity() == id && owner->character({0})->build.level == level,
          "advancement and inventory survive descent");
    travel(run, FixtureKind::Upstairs);
    check(!run.game().lootAccess(oldLoot), "old floor loot handle rejects after revisit");
    face(run.game(), Heading::South);
    auto restored = oldLoot;
    restored.world = run.game().loot().snapshot().world;
    check(bool(run.game().pickup({restored, {0}, owner->inventory().revision()})),
          "revisited dropped stack remains collectible exactly once");
    check(!run.game().pickup({restored, {0}, owner->inventory().revision()}),
          "repeated pickup cannot duplicate gear");
    for (unsigned i = 0; i < 4; ++i)
        check(owner->snapshot().formation[i].index == formation[i].index, "formation retained");
    bool found = false;
    for (std::uint16_t b = 0; b < bagCells; ++b) {
        const auto* item = owner->inventory().stack(owner->inventory().at({{0}, b}));
        found |= item && item->item.index == stack.item.index && item->quantity >= stack.quantity;
    }
    check(found, "actual dropped gear returns without regranting starting build");
    owner->damage({0}, 100000); // dead party members are carried, never resurrected by travel/rest.
    face(run.game(), Heading::East);
    travel(run, FixtureKind::Downstairs);
    travel(run, FixtureKind::Upstairs);
    check(owner->character({0})->life == rules::Life::Dead, "travel does not resurrect dead companion");
    for (std::uint16_t c = 1; c < 4; ++c)
        owner->damage({c}, 100000);
    const auto time = run.gameplayMs();
    const auto pos = run.game().movement().player;
    run.game().move(Action::Forward);
    run.update(1, false);
    RestRequest rest;
    rest.world = run.game().actors().snapshot().world;
    rest.token = run.game().restToken();
    check(run.status() == CampaignStatus::Defeat && run.gameplayMs() == time &&
              run.game().movement().player == pos && !run.game().rest(rest),
          "terminal run freezes movement, clock and recovery");
}

void allPortableEffects() {
    const auto cat = catalog(false, 50, 7);
    const auto defs = definitions();
    for (const auto kind :
         {EffectKind::Bless, EffectKind::Bane, EffectKind::ShieldOfFaith, EffectKind::MageArmor,
          EffectKind::HoldPerson, EffectKind::Guidance, EffectKind::Resistance, EffectKind::Sanctuary,
          EffectKind::Aid, EffectKind::MirrorImage}) {
        CampaignRun run(cat, *cat.preset(), defs.types, defs.timings, 5);
        EffectRequest e;
        e.kind = kind;
        e.world = run.game().actors().snapshot().world;
        e.caster = {0};
        e.count = 1;
        e.difficulty = 1000;
        const rules::CharacterId target{std::uint16_t(kind == EffectKind::MirrorImage ? 0 : 3)};
        e.targets[0] = target;
        const auto applied = run.game().applyEffect(e);
        check(bool(applied) && applied.id.serial, "portable effect fixture actually applies");
        const auto status = run.game().effects().statuses(target);
        auto ticket = run.prepareTravel(run.request(run.game().nearbyFixture()));
        check(run.commitTravel(ticket) && run.game().effects().has(target, kind),
              "every supported portable instance transfers");
        const auto restored = run.game().effects().statuses(target);
        check(restored.count == status.count &&
                  restored.entries[0].remainingMs == status.entries[0].remainingMs,
              "transfer preserves effect duration");
    }
    for (const auto kind : {EffectKind::Sleep, EffectKind::Blind, EffectKind::Slow, EffectKind::Knockdown}) {
        CampaignRun run(cat, *cat.preset(), defs.types, defs.timings, 5);
        CombatEvents notices;
        const_cast<EffectController&>(run.game().effects())
            .condition(rules::CharacterId{1}, kind, {0}, 0, notices);
        auto ticket = run.prepareTravel(run.request(run.game().nearbyFixture()));
        check(run.commitTravel(ticket) && run.game().effects().has(rules::CharacterId{1}, kind),
              "party condition transfers");
    }
    CampaignRun run(cat, *cat.preset(), defs.types, defs.timings, 5);
    auto initial = run.prepareTravel(run.request(run.game().nearbyFixture()));
    check(run.commitTravel(initial), "capacity fixture enters floor");
    EffectRequest e;
    e.world = run.game().actors().snapshot().world;
    e.caster = {0};
    e.count = 2;
    e.kind = EffectKind::Bless;
    e.targets[0] = rules::CharacterId{1};
    e.targets[1] = run.game().actors().id(0);
    check(bool(run.game().applyEffect(e)) &&
              !run.previewTravel(run.request(run.game().nearbyFixture())).accepted,
          "mixed concentration blocks travel");
    check(bool(run.game().endConcentration(e.world, {0})), "local concentration explicitly dismissed");
    e.count = 1;
    e.kind = EffectKind::Web;
    e.targets[0] = rules::CharacterId{0};
    check(bool(run.game().applyEffect(e)) &&
              !run.previewTravel(run.request(run.game().nearbyFixture())).accepted,
          "Web blocks travel");
    check(bool(run.game().endConcentration(e.world, {0})), "Web explicitly dismissed");
    e.kind = EffectKind::Sanctuary;
    e.targets[0] = run.game().actors().id(0);
    check(bool(run.game().applyEffect(e)), "one floor-local slot occupied");
    travel(run, FixtureKind::Upstairs);
    e.world = run.game().actors().snapshot().world;
    e.kind = EffectKind::MageArmor;
    e.targets[0] = rules::CharacterId{3};
    EffectId last;
    for (unsigned i = 0; i < EffectController::capacity; ++i) {
        const auto out = run.game().applyEffect(e);
        check(bool(out), "fill portable pool");
        last = out.id;
    }
    const auto hp = run.party().character({3})->hitPoints;
    bool rejected = false;
    try {
        (void)run.prepareTravel(run.request(run.game().nearbyFixture()));
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected && run.activeLocation() == 0 && run.party().character({3})->hitPoints == hp,
          "combined effect capacity rejects atomically");
    check(bool(run.game().removeEffect(last)), "free one portable slot");
    auto fits = run.prepareTravel(run.request(run.game().nearbyFixture()));
    check(run.commitTravel(fits), "exact combined effect capacity succeeds");
}

Json campaignScenario(const rules::Catalog& cat) {
    auto j = generatedScenario(cat);
    j["kind"] = "campaign";
    j.erase("depth");
    return j;
}
void adapters(const std::filesystem::path& retain) {
    const auto cat = catalog();
    const auto defs = definitions();
    Client client(cat, defs);
    Directory directory;
    const auto tracePath = directory.path / "campaign.jsonl";
    const Json header = {{"fixture", "pc04"}};
    TraceWriter trace(tracePath);
    trace.append("header", header);
    client.trace = &trace;
    const auto start = client.request("start", {{"scenario", campaignScenario(cat)}});
    check(start.at("ok") && start.at("data").at("scenario_kind") == "campaign", "campaign protocol launch");
    const auto preview = client.request("preview", {{"action", {{"kind", "interact"}, {"fixture", 0}}}});
    check(preview.at("ok") && preview.at("data").at("accepted"), "wire stair preview");
    check(client.act({{"kind", "interact"}, {"fixture", 0}}).at("data").at("accepted"), "wire stair travel");
    check(client.protocol.state().at("campaign").at("location") == 1, "wire campaign location");
    auto upstairs = stair(client.protocol.session()->game(), FixtureKind::Upstairs);
    check(client.act({{"kind", "interact"}, {"fixture", upstairs.id.slot}}).at("data").at("accepted"),
          "wire free ascent");
    const auto state = client.protocol.state();
    check(state.at("campaign").at("locations")[1].contains("state_digest"),
          "inactive integrity digest included");
    trace.finish();
    client.trace = nullptr;
    Protocol replayed(cat, defs, Mode::Diagnostic);
    replay(replayed, tracePath, header);
    check(replayed.state() == state, "strict campaign replay restores active and inactive state");
    if (!retain.empty())
        std::filesystem::copy_file(tracePath, retain, std::filesystem::copy_options::overwrite_existing);
    const auto corruptedPath = directory.path / "corrupted.jsonl";
    std::ifstream recorded(tracePath);
    std::ofstream corrupted(corruptedPath);
    bool changed = false;
    for (std::string line; std::getline(recorded, line);) {
        auto record = Json::parse(line);
        if (!changed && record.at("type") == "state" && record.at("data").contains("campaign")) {
            auto& campaign = record["data"]["campaign"];
            if (campaign["location"] == 0 && campaign["locations"][1].contains("state_digest")) {
                campaign["locations"][1]["state_digest"] = "corrupted";
                changed = true;
            }
        }
        corrupted << record.dump() << '\n';
    }
    corrupted.close();
    Protocol negative(cat, defs, Mode::Diagnostic);
    bool rejected = false;
    try {
        replay(negative, corruptedPath, header);
    } catch (const std::exception&) {
        rejected = true;
    }
    check(changed && rejected, "strict replay rejects corrupted inactive location integrity");
    // View allocation failure is injected at the existing factory boundary.
    struct View final : Visibility {
        TargetSnapshot targets;
        const TargetSnapshot& sample(const Map&, const GameSession&) override { return targets; }
        bool lootVisible(LootId) const noexcept override { return false; }
    };
    int calls = 0;
    auto factory = +[](void* context, const Map&, const GameSession&) -> std::unique_ptr<Visibility> {
        auto& count = *static_cast<int*>(context);
        if (++count > 1)
            throw std::bad_alloc();
        return std::make_unique<View>();
    };
    Client failure(cat, defs, Mode::Visual, factory, &calls);
    check(failure.request("start", {{"scenario", campaignScenario(cat)}}).at("ok"),
          "view failure fixture starts");
    const auto original = failure.protocol.state();
    const auto failed = failure.act({{"kind", "interact"}, {"fixture", 0}});
    check(failed.at("ok") && !failed.at("data").at("accepted") &&
              failure.protocol.state().at("campaign") == original.at("campaign"),
          "view preparation failure preserves campaign");
}

} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string_view(argv[1]) == "--replay") {
            const auto cat = catalog();
            const auto defs = definitions();
            Protocol protocol(cat, defs, Mode::Diagnostic);
            replay(protocol, argv[2], Json{{"fixture", "pc04"}});
            std::cout << "campaign fresh-process replay passed\n";
            return 0;
        }
        const std::filesystem::path retain =
            argc == 3 && std::string_view(argv[1]) == "--retain" ? argv[2] : "";
        campaign();
        transferFailuresAndEffects();
        inventoryAndLifecycle();
        allPortableEffects();
        adapters(retain);
        std::cout << "campaign checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
