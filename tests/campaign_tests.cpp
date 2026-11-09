#include "app/result_flow.hpp"
#include "game/campaign.hpp"
#include "game/commands.hpp"
#include "support/campaign_fixtures.hpp"
#include "support/generation_blueprints.hpp"
#include "ui/interaction.hpp"
#include "ui/travel_layout.hpp"
#include <iostream>
#include <queue>
using namespace blobber;
using namespace blobber::automation;
using namespace blobber::automation::test;
namespace {
using namespace blobber::campaign_test;
void cityCampaign() {
    const auto cat = catalog();
    const auto defs = definitions();
    CampaignRun run(blobber::test::campaignBlueprints(), cat, *cat.preset(), defs.types, defs.timings, 5);
    check(run.game().movement().player == Player{{11, 8}, Heading::North} &&
              run.map().definition().assetPack == "sanctuary-city",
          "New Game uses square arrival");
    check(!run.game().atSanctuary() && !run.game().fixtures().discovered({12, 20}),
          "Square is not free rest and distant gate is initially unexplored");
    check(!run.previewTravel(run.request(stair(run.game(), FixtureKind::Downstairs))),
          "Square does not offer remote stair travel");
    const auto inventory = run.party().inventory().identity();
    run.party().damage({0}, 1);
    const auto hp = run.party().character({0})->hitPoints;
    walk(run.game(), {12, 20});
    face(run.game(), Heading::South);
    check(run.game().fixtures().discovered({12, 20}) && !run.game().geometry().canStep({12, 20}, {12, 21}) &&
              !run.game().fixtures().find(run.game().nearbyFixture().id),
          "Explored city gate stays closed without an action");
    const auto gatePose = run.game().movement().player;
    auto checkpoint = run.checkpoint();
    const auto discovered = checkpoint->locations[0].state->fixtures.discovered;
    CampaignRun restored(blobber::test::campaignBlueprints(), cat, std::move(*checkpoint), defs.timings);
    check(restored.game().movement().player == gatePose &&
              restored.checkpoint()->locations[0].state->fixtures.discovered == discovered,
          "City pose and partial exploration survive checkpoint restore");
    // Immutable city data must not become an arbitrary user-defined hub, even with a new checksum.
    for (unsigned mutation = 0; mutation < 5; ++mutation) {
        auto bad = run.checkpoint();
        auto& hub = bad->locations[0];
        if (mutation == 0)
            hub.entry.heading = Heading::West;
        if (mutation == 1)
            hub.fixtures.landings[1] = hub.entry.cell;
        if (mutation == 2)
            hub.fixtures.discovery = false;
        if (mutation == 3)
            hub.rows[20][12] = '#';
        if (mutation == 4) {
            hub.world.scenery[0].model = "different-tree";
            hub.worldFingerprint = hub.world.fingerprint();
        }
        bool rejected = false;
        try {
            CampaignRun invalid(blobber::test::campaignBlueprints(), cat, std::move(*bad), defs.timings);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "Changed canonical hub definition rejects");
    }
    entrance(run);
    const auto entrancePose = run.game().movement().player;
    run.game().move(Action::Forward);
    check(run.game().movement().blocked && run.game().movement().player == entrancePose &&
              !run.game().movement().transition.active(),
          "Extra forward at the entrance cannot enter the stair cell or start movement");
    const auto entranceRequest = run.request(run.game().nearbyFixture());
    check(run.previewTravel(entranceRequest).accepted,
          "Blocked entrance movement preserves explicit stair interaction");
    auto entry = run.prepareTravel(entranceRequest);
    check(run.commitTravel(entry), "Enter dungeon after blocked entrance movement");
    auto& saved = *run.location(0).saved;
    auto& hazard = saved.effects.ground.areas[0];
    hazard.active = true;
    hazard.cell = run.location(0).exit.cell;
    hazard.footprint = wholeCellFootprint(hazard.cell);
    hazard.id.serial = saved.effects.nextArea++;
    hazard.untilMs = 60000;
    hazard.caster = rules::CharacterId{0};
    const auto upstairs = stair(run.game(), FixtureKind::Upstairs);
    face(run.game(), run.game().fixtures().find(upstairs.id)->facing);
    bool rejected = false;
    try {
        (void)run.prepareTravel(run.request(upstairs));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && run.activeLocation() == 1, "Dungeon return checks its own landing hazard");
    hazard.active = false;
    travel(run, FixtureKind::Upstairs);
    check(run.game().movement().player == Player{{13, 7}, Heading::South} &&
              run.party().character({0})->hitPoints == hp &&
              run.party().inventory().identity() == inventory && run.game().fixtures().discovered({12, 20}),
          "Dungeon return faces town, preserves identity/discovery and does not rest");
    travel(run, FixtureKind::Downstairs);
    const auto kits = run.party().campSupplies().kits;
    auto recall = run.prepareTravel(run.recallRequest());
    check(run.commitTravel(recall) && !run.commitTravel(recall) &&
              run.game().movement().player == Player{{9, 6}, Heading::North} && !run.game().atSanctuary() &&
              run.party().campSupplies().kits == kits - 1 && run.party().character({0})->hitPoints == hp,
          "Recall arrives at sanctuary and charges once without resting");
    check(!run.previewTravel(run.waypointRequest(1)), "Recall does not enable remote waypoint travel");
}
void campaign() {
    const auto cat = catalog(false, 50, 7);
    const auto defs = definitions();
    CampaignRun run(blobber::test::campaignBlueprints(), cat, *cat.preset(), defs.types, defs.timings, 5);
    entrance(run);
    check(run.activeLocation() == 0 && run.status() == CampaignStatus::Active, "campaign starts in hub");
    walk(run.game(), run.location(0).entry.cell);
    CommandController hubUi(run.game(), defs.timings);
    hubUi.refresh({}, false, false, true);
    check(!hubUi.snapshot().fixturePrompt[0], "Sanctuary arrival no longer exposes free recovery");
    walk(run.game(), {11, 8});
    hubUi.refresh({}, false, false, true);
    check(!hubUi.snapshot().fixturePrompt[0], "Idle campaign has no permanent location strip");
    entrance(run);
    hubUi.refresh({}, false, false, true);
    check(std::string_view(hubUi.snapshot().fixturePrompt.data()) == "[F] Enter dungeon",
          "Entrance exposes contextual stair prompt");
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
    buff.caster = rules::CharacterId{0};
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
    final.update(.7, false);
    check(final.actors().snapshot().actors[0].life == ActorLife::Alive &&
              run.status() == CampaignStatus::Active && final.runActive(),
          "final designated guardian completion retains active gameplay without unrelated clear");
    check(final.objective().completed && final.loot().snapshot().containerCount == 3 &&
              !final.loot().snapshot().containers[2].sealed,
          "final guardian chest exists and unseals");
    kill(final);
    travel(run, FixtureKind::Upstairs);
    check(run.activeLocation() == 2 && run.status() == CampaignStatus::Active,
          "completed final floor permits returning upstairs");
    std::cout << "snapshot_bytes=" << run.snapshotBytes() << '\n';
}
void transferFailuresAndEffects() {
    const auto cat = catalog(false, 50, 7);
    const auto defs = definitions();
    CampaignRun run(blobber::test::campaignBlueprints(), cat, *cat.preset(), defs.types, defs.timings, 8);
    entrance(run);
    auto enter = [&] {
        entrance(run);
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
    effect.caster = rules::CharacterId{0};
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
    enterInn(run);
    run.party().damage({0}, 1);
    RestRequest rest;
    rest.kind = RestKind::Long;
    rest.world = run.game().actors().snapshot().world;
    rest.token = run.game().restToken();
    const auto hp = run.party().character({0})->hitPoints;
    const auto clock = run.gameplayMs();
    check(bool(run.game().rest(rest)), "inn explicit free long rest");
    check(run.party().character({0})->hitPoints > hp && run.gameplayMs() == clock + 28800000,
          "rest advances campaign time exactly once");
    check(!run.game().effects().has(rules::CharacterId{0}, EffectKind::Bless) &&
              !run.game().effects().has(rules::CharacterId{0}, EffectKind::MirrorImage),
          "party durations expire in hub");
    check(run.party().character({2})->temporaryHitPoints == 0 &&
              run.game().combat().snapshot().cooldownMs[3] == 0,
          "rest expires temporary HP and readiness deadlines once on campaign time");
    leaveInn(run);
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
    entrance(run);
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
    saved.effects.ground.areas[0].footprint = wholeCellFootprint(run.location(1).entry.cell);
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
    CampaignRun run(blobber::test::campaignBlueprints(), cat, *cat.preset(), defs.types, defs.timings, 5);
    entrance(run);
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
    walk(run.game(), {12, 8});
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
    entrance(run);
    travel(run, FixtureKind::Downstairs);
    check(owner->inventory().identity() == id && owner->character({0})->build.level == level,
          "advancement and inventory survive descent");
    travel(run, FixtureKind::Upstairs);
    check(!run.game().lootAccess(oldLoot), "old floor loot handle rejects after revisit");
    walk(run.game(), {12, 8});
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
        CampaignRun run(blobber::test::campaignBlueprints(), cat, *cat.preset(), defs.types, defs.timings, 5);
        entrance(run);
        EffectRequest e;
        e.kind = kind;
        e.world = run.game().actors().snapshot().world;
        e.caster = rules::CharacterId{0};
        e.count = 1;
        e.difficulty = 1000;
        const rules::CharacterId target{
            std::uint16_t(kind == EffectKind::MirrorImage || kind == EffectKind::Aid ? 0 : 3)};
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
        for (unsigned transition = 0; transition < 2; ++transition) {
            if (transition == 1)
                entrance(run);
            const auto r = transition == 0 ? run.recallRequest() : run.waypointRequest(1);
            auto next = run.prepareTravel(r);
            check(run.commitTravel(next) && run.game().effects().has(target, kind),
                  "every portable instance survives recall and waypoint return");
            const auto statusAfter = run.game().effects().statuses(target);
            check(statusAfter.count == status.count &&
                      statusAfter.entries[0].remainingMs == status.entries[0].remainingMs,
                  "recall and waypoint preserve portable duration");
        }
    }
    for (const auto kind : {EffectKind::Sleep, EffectKind::Blind, EffectKind::Slow, EffectKind::Knockdown}) {
        CampaignRun run(blobber::test::campaignBlueprints(), cat, *cat.preset(), defs.types, defs.timings, 5);
        entrance(run);
        CombatEvents notices;
        const_cast<EffectController&>(run.game().effects())
            .condition(rules::CharacterId{1}, kind, {0}, 0, notices);
        auto ticket = run.prepareTravel(run.request(run.game().nearbyFixture()));
        check(run.commitTravel(ticket) && run.game().effects().has(rules::CharacterId{1}, kind),
              "party condition transfers");
        for (unsigned transition = 0; transition < 2; ++transition) {
            if (transition == 1)
                entrance(run);
            auto next = run.prepareTravel(transition == 0 ? run.recallRequest() : run.waypointRequest(1));
            check(run.commitTravel(next) && run.game().effects().has(rules::CharacterId{1}, kind),
                  "party conditions survive recall and waypoint return");
        }
    }
    CampaignRun run(blobber::test::campaignBlueprints(), cat, *cat.preset(), defs.types, defs.timings, 5);
    entrance(run);
    auto initial = run.prepareTravel(run.request(run.game().nearbyFixture()));
    check(run.commitTravel(initial), "capacity fixture enters floor");
    EffectRequest e;
    e.world = run.game().actors().snapshot().world;
    e.caster = rules::CharacterId{0};
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
    entrance(run);
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

void recallAndWaypoints() {
    const auto cat = catalog(false, 50, 7);
    const auto defs = definitions();
    CampaignRun run(blobber::test::campaignBlueprints(), cat, *cat.preset(), defs.types, defs.timings, 5);
    entrance(run);
    auto& party = run.party();
    const auto identity = party.inventory().identity();
    auto source = std::make_unique<FloorState>();
    const auto rejects = [&](TravelRequest request) {
        run.game().exportFloor(*source);
        const auto digest = source->digest();
        const auto world = run.game().actors().snapshot().world;
        const auto kits = party.campSupplies().kits;
        const auto revision = party.inventory().revision();
        check(!run.previewTravel(request), "invalid travel preview rejected");
        bool threw = false;
        try {
            (void)run.prepareTravel(request);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        run.game().exportFloor(*source);
        check(threw && source->digest() == digest && run.game().actors().snapshot().world == world &&
                  party.campSupplies().kits == kits && party.inventory().revision() == revision,
              "travel rejection preserves canonical source and resources");
    };
    rejects(run.recallRequest());
    rejects(run.waypointRequest(1));
    rejects(run.waypointRequest(0));
    rejects(run.waypointRequest(4));
    auto first = run.prepareTravel(run.request(run.game().nearbyFixture()));
    check(!run.location(1).visited, "preparation does not unlock entry");
    check(run.commitTravel(first) && run.location(1).visited, "stairs alone unlock entry on commit");
    rejects(run.waypointRequest(1));
    check(bool(party.acquire({{0}, cat.findItem("camp_kit"), 2, party.inventory().revision()})),
          "split-supply fixture owns a multi-kit stack");
    auto stale = run.recallRequest();
    check(bool(party.acquire({{1}, cat.findItem("camp_kit"), 1, party.inventory().revision()})),
          "fixture adds separate supplier stack");
    rejects(stale);
    const auto originalSupply = party.campSupplies().supply;
    ItemLocation freeCell;
    freeCell.character = party.inventory().locate(originalSupply).character;
    for (std::uint16_t i = 0; i < bagCells; ++i)
        if (!party.inventory().at({freeCell.character, i})) {
            freeCell.cell = i;
            break;
        }
    check(bool(party.inventoryChange(
              {InventoryOp::Split, originalSupply, freeCell, 1, party.inventory().revision()})),
          "owned supply split");
    check(party.campSupplies().kits == 4, "shared supply query counts split and separate stacks");
    auto staged = run.prepareTravel(run.recallRequest());
    const auto kitsBefore = party.campSupplies().kits;
    check(party.consumeItem(party.campSupplies().supply), "mutate staged supply");
    check(!run.commitTravel(staged) && run.activeLocation() == 1 &&
              party.campSupplies().kits == kitsBefore - 1,
          "stale commit never additionally charges");
    auto wrongWorld = run.recallRequest();
    ++wrongWorld.world;
    rejects(wrongWorld);
    auto wrongSupply = run.recallRequest();
    ++wrongSupply.supply.generation;
    rejects(wrongSupply);
    // Each depth returns through an entry waypoint; no new rewards or floor simulation.
    for (unsigned depth = 1; depth <= 3; ++depth) {
        check(run.activeLocation() == depth, "owned route reaches each depth");
        if (depth < 3) {
            kill(run.game()); // lifecycle fixture, not encounter calibration
            for (unsigned i = 0; i < run.game().fixtures().definition().count; ++i)
                if (run.game().fixtures().definition().entries[i].kind == FixtureKind::Key) {
                    walk(run.game(), run.game().fixtures().definition().entries[i].cell);
                    check(bool(run.game().interact(
                              {run.game().fixtures().id(i), run.game().fixtures().revision()})),
                          "owned required key collected");
                }
            walk(run.game(), run.location(depth).generated->layout.optionalSupply);
        }
        party.damage({0}, 1);
        if (depth == 1) {
            RestRequest rest;
            rest.kind = RestKind::Long;
            rest.world = run.game().actors().snapshot().world;
            rest.token = run.game().restToken();
            const auto pose = run.game().movement().player;
            const auto kits = party.campSupplies().kits;
            const auto clock = run.gameplayMs();
            check(bool(run.game().rest(rest)) && party.campSupplies().kits == kits - 1 &&
                      run.game().movement().player == pose && run.gameplayMs() == clock + 28800000,
                  "field rest consumes one kit, advances once, preserves position");
            party.damage({0}, 1);
        }
        if (!party.campSupplies().kits)
            check(bool(party.acquire({{1}, cat.findItem("camp_kit"), 1, party.inventory().revision()})),
                  "owned recall supply");
        const auto request = run.recallRequest();
        const auto quantity = party.inventory().stack(request.supply)->quantity;
        const auto supplier = party.inventory().locate(request.supply).character;
        const auto weight = party.character(supplier)->stats.carriedWeight;
        const auto unitWeight = cat.item(party.inventory().stack(request.supply)->item)->weight;
        run.game().exportFloor(*source);
        const auto digest = source->digest();
        const auto hp = party.character({0})->hitPoints;
        const auto xp = party.character({0})->experience;
        const auto kits = party.campSupplies().kits;
        const auto clock = run.gameplayMs();
        for (unsigned i = 0; i < 3; ++i)
            check(bool(run.previewTravel(request)), "repeat preview remains legal");
        auto ticket = run.prepareTravel(request);
        check(run.activeLocation() == depth && party.campSupplies().kits == kits,
              "recall preparation is uncharged");
        check(run.commitTravel(ticket) && !run.commitTravel(ticket), "recall commits once");
        check(run.activeLocation() == 0 && party.campSupplies().kits == kits - 1 &&
                  party.inventory().identity() == identity && party.character({0})->hitPoints == hp &&
                  run.gameplayMs() == clock && run.game().movement().player == run.location(0).entry,
              "recall charges once without recovery, clock advance or new Party");
        const auto* remaining = party.inventory().stack(request.supply);
        check((quantity == 1 ? !remaining : remaining && remaining->quantity == quantity - 1) &&
                  party.character(supplier)->stats.carriedWeight == weight - unitWeight,
              "charge preserves live stack identity or invalidates the last kit and updates weight");
        rejects(request);
        rejects(run.waypointRequest(depth + 1));
        // Wrong facing/proximity cannot turn the selector into an anywhere-in-hub warp.
        run.game().setAnimated(false);
        run.game().move(Action::TurnLeft);
        rejects(run.waypointRequest(depth));
        entrance(run);
        auto back = run.prepareTravel(run.waypointRequest(depth));
        check(run.commitTravel(back) && run.game().movement().player == run.location(depth).entry &&
                  party.campSupplies().kits == kits - 1 && party.character({0})->experience == xp,
              "free waypoint uses original entry and preserves XP and kits");
        run.game().exportFloor(*source);
        check(source->digest() == digest, "recall and waypoint preserve complete canonical floor state");
        if (depth < 3)
            travel(run, FixtureKind::Downstairs);
    }
    // Unsafe local effects and movement use the same checked departure authority.
    auto& g = run.game();
    EffectRequest e;
    e.world = g.actors().snapshot().world;
    e.kind = EffectKind::SpiritualWeapon;
    e.caster = rules::CharacterId{0};
    e.targets[0] = rules::CharacterId{0};
    e.count = 1;
    const auto weapon = g.applyEffect(e);
    check(bool(weapon), "recall summon restriction fixture");
    rejects(run.recallRequest());
    check(bool(g.removeEffect(weapon.id)), "explicit fixture dismissal");
    g.setAnimated(true);
    g.move(Action::TurnLeft);
    rejects(run.recallRequest());
    run.update(1, false);
    g.setAnimated(false);
    while (party.campSupplies().supply)
        check(party.consumeItem(party.campSupplies().supply), "deplete finite kits");
    rejects(run.recallRequest());
    travel(run, FixtureKind::Upstairs);
    travel(run, FixtureKind::Upstairs);
    travel(run, FixtureKind::Upstairs);
    check(run.activeLocation() == 0 && !party.campSupplies().kits, "zero-kit walking remains free");
    enterInn(run);
    party.damage({0}, 1);
    RestRequest rest;
    rest.kind = RestKind::Long;
    rest.world = run.game().actors().snapshot().world;
    rest.token = run.game().restToken();
    const auto clock = run.gameplayMs();
    check(bool(run.game().rest(rest)) && !party.campSupplies().kits && run.gameplayMs() == clock + 28800000,
          "explicit inn rest restores without kit refill");
    leaveInn(run);
    entrance(run);
    CommandController selector(run.game());
    selector.refresh({}, false, false, true);
    selector.openTravel();
    check(selector.snapshot().travel.count == 3, "all legally unlocked entries fit fixed selector");
    UiInput down;
    down.pressed[std::size_t(UiKey::Down)] = true;
    routeHudInput(selector, down);
    routeHudInput(selector, down);
    check(selector.snapshot().travel.rows[selector.snapshot().travel.cursor].depth == 3,
          "keyboard browses deep entries");
    UiInput row;
    row.pressed[std::size_t(UiKey::Click)] = true;
    row.pointer = ArtPoint{float(travelLayout::row(1).x + 1), float(travelLayout::row(1).y + 1)};
    routeHudInput(selector, row);
    check(selector.snapshot().travel.cursor == 1 && !selector.snapshot().travelRequested,
          "row click selects without implicit travel");
    UiInput confirm;
    confirm.pressed[std::size_t(UiKey::Confirm)] = true;
    routeHudInput(selector, confirm);
    check(selector.snapshot().travelRequested && selector.snapshot().travelRequested->depth == 2,
          "selector confirms the selected unlocked entry");
    auto warp = run.prepareTravel(run.waypointRequest(3));
    check(run.commitTravel(warp) && !party.campSupplies().kits, "zero-kit deep entry return is free");
    for (std::uint16_t i = 0; i < 4; ++i)
        party.damage({i}, 100000);
    rejects(run.recallRequest());
    rejects(run.waypointRequest(1));
    std::cout << "PreparedTravel bounded bytes: " << sizeof(CampaignRun::PreparedTravel) << '\n';
}
void recallSafetyAndRollback() {
    const auto cat = catalog();
    const auto defs = definitions();
    CampaignRun run(blobber::test::campaignBlueprints(), cat, *cat.preset(), defs.types, defs.timings, 5);
    entrance(run);
    travel(run, FixtureKind::Downstairs);
    auto& game = run.game();
    const auto denied = [&](const char* reason) {
        const auto result = run.previewTravel(run.recallRequest());
        check(!result && std::string_view(result.reason).find(reason) != std::string_view::npos,
              "recall reports the actual safety restriction");
    };
    run.party().damage({0}, 1);
    PartyCommand wind;
    wind.character = {0};
    wind.action = PartyAction::Feature;
    wind.feature = rules::Feature::SecondWind;
    wind.world = game.actors().snapshot().world;
    wind.turn = game.combat().snapshot().turn;
    check(game.submit(wind).reason == CommandReason::Resolved, "owned feature starts presentation");
    game.update(.7, false);
    denied("Wait until it is safe to travel");
    game.update(.4, false);
    auto& phase = const_cast<CombatSnapshot&>(game.combat().snapshot()).phase;
    for (const auto state : {CombatPhase::Contact, CombatPhase::Turns, CombatPhase::Approach,
                             CombatPhase::Facing, CombatPhase::RoundEnd, CombatPhase::Victory}) {
        phase = state;
        denied("Wait until it is safe to travel");
    }
    phase = CombatPhase::Exploration;
    CombatEvents events;
    auto& effects = const_cast<EffectController&>(game.effects());
    effects.createArea(game.movement().player.cell, {0}, 10, game.timeMs(), events);
    denied("Wait until it is safe to travel");
    const_cast<GroundSnapshot&>(effects.ground()).areas[0].active = false;
    for (std::uint16_t i = 0; i < 4; ++i)
        effects.condition(rules::CharacterId{i}, EffectKind::Sleep, {0}, game.timeMs(), events);
    denied("able conscious");
    for (std::uint16_t i = 0; i < 4; ++i)
        effects.wake(rules::CharacterId{i}, game.timeMs(), events);
    EffectRequest local;
    local.world = game.actors().snapshot().world;
    local.kind = EffectKind::Bane;
    local.caster = rules::CharacterId{0};
    local.count = 2;
    local.difficulty = 1000;
    local.targets[0] = rules::CharacterId{0};
    local.targets[1] = game.actors().id(0);
    const auto concentration = game.applyEffect(local);
    check(bool(concentration), "mixed concentration fixture");
    denied("Dismiss spells affecting enemies");
    check(bool(game.removeEffect(concentration.id)), "explicit mixed concentration dismissal");
    // Seed a malformed hub record, then a real local landing hazard. Neither failure charges.
    auto& hub = *run.location(0).saved;
    const auto count = hub.actors.count;
    const auto kits = run.party().campSupplies().kits;
    const auto inventory = run.party().inventory().revision();
    const auto world = game.actors().snapshot().world;
    auto snapshot = std::make_unique<FloorState>();
    game.exportFloor(*snapshot);
    const auto digest = snapshot->digest();
    const auto fail = [&] {
        bool threw = false;
        try {
            (void)run.prepareTravel(run.recallRequest());
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        game.exportFloor(*snapshot);
        check(threw && run.activeLocation() == 1 && run.party().campSupplies().kits == kits &&
                  run.party().inventory().revision() == inventory &&
                  game.actors().snapshot().world == world && snapshot->digest() == digest,
              "failed recall destination retains source and kit");
    };
    hub.actors.count = 257;
    fail();
    hub.actors.count = count;
    auto& area = hub.effects.ground.areas[0];
    area.active = true;
    area.cell = run.location(0).entry.cell;
    area.footprint = wholeCellFootprint(area.cell);
    area.id.serial = hub.effects.nextArea++;
    area.untilMs = 60000;
    area.caster = rules::CharacterId{0};
    fail();
    area.active = false;
    auto ticket = run.prepareTravel(run.recallRequest());
    ticket.game().setAnimated(false);
    ticket.game().move(Action::TurnLeft);
    check(!run.commitTravel(ticket) && run.party().campSupplies().kits == kits,
          "mutated staged destination is rejected without charging");
}
void travelUi() {
    const auto cat = catalog();
    const auto defs = definitions();
    CampaignRun run(blobber::test::campaignBlueprints(), cat, *cat.preset(), defs.types, defs.timings, 5);
    entrance(run);
    UiInput open, confirm, cancel;
    open.pressed[std::size_t(UiKey::Travel)] = true;
    open.pressed[std::size_t(UiKey::TransferItem)] = true;
    confirm.pressed[std::size_t(UiKey::Confirm)] = true;
    cancel.pressed[std::size_t(UiKey::Cancel)] = true;
    CommandController hub(run.game());
    hub.refresh({}, false, false, true);
    auto both = open;
    both.pressed[std::size_t(UiKey::Confirm)] = true;
    routeHudInput(hub, both);
    check(!hub.snapshot().travel.open && hub.snapshot().travelRequested &&
              hub.snapshot().travelRequested->kind == TravelKind::Stairs,
          "one fresh hub destination immediately emits checked stairs intent");
    hub.cancelTravel();
    hub.interactFixture();
    check(!hub.snapshot().modal() && hub.snapshot().travelRequested,
          "hub F enters the single destination without a selector");
    auto enter = run.prepareTravel(*hub.snapshot().travelRequested);
    check(run.commitTravel(enter), "confirmed first entry commits");
    check(bool(run.party().acquire({{0}, cat.findItem("camp_kit"), 1, run.party().inventory().revision()})),
          "recall UI fixture owns a kit for stale-panel consumption and another for commit");
    CommandController ui(run.game());
    ui.refresh({}, false, false, true);
    const auto entryPose = run.game().movement().player;
    const auto up = stair(run.game(), FixtureKind::Upstairs);
    const auto* upDefinition = run.game().fixtures().find(up.id);
    run.game().restorePose({stairLanding(*upDefinition), upDefinition->facing}, false);
    run.game().update(0, false);
    ui.refresh({}, false, false, true);
    check(std::string_view(ui.snapshot().fixturePrompt.data()) == "[F] Ascend",
          "Usable dungeon exit offers short Ascend prompt");
    run.game().restorePose(entryPose, false);
    ui.refresh({}, false, false, true);
    const auto supply = run.party().inventory().locate(run.party().campSupplies().supply);
    ui.openInventory(supply.character);
    ui.inventorySelect(supply.cell);
    routeHudInput(ui, open);
    check(ui.snapshot().inventory.open && !ui.snapshot().travel.open &&
              ui.snapshot().inventory.mode == InventoryMode::Transfer,
          "physical T retains Inventory transfer without opening Travel");
    ui.closeOverlay();
    routeHudInput(ui, both);
    const auto kits = run.party().campSupplies().kits;
    check(ui.snapshot().travel.open && ui.snapshot().travel.preview && !ui.snapshot().travelRequested,
          "full-health recall can open but does not execute on opening");
    routeHudInput(ui, cancel);
    check(!ui.snapshot().modal() && run.party().campSupplies().kits == kits, "recall cancel is uncharged");
    UiInput click;
    click.pointer = ArtPoint{float(hudLayout::travelButton.x + 1), float(hudLayout::travelButton.y + 1)};
    routeHudInput(ui, click);
    check(ui.snapshot().hover.kind == HoverKind::Travel &&
              std::string(ui.snapshot().hover.lines[0].data()) == "Travel [T]" &&
              !ui.snapshot().travel.open && run.party().campSupplies().kits == kits,
          "travel icon hover identifies its shortcut without opening or spending supplies");
    click.pressed[std::size_t(UiKey::Click)] = true;
    routeHudInput(ui, click);
    check(ui.snapshot().travel.open, "header opens recall panel");
    run.party().consumeItem(run.party().campSupplies().supply);
    ui.refresh({}, false, false, true);
    routeHudInput(ui, confirm);
    check(ui.snapshot().travel.open && !ui.snapshot().travelRequested && !ui.snapshot().travel.preview,
          "stale panel cannot silently select another kit");
    routeHudInput(ui, cancel);
    routeHudInput(ui, open);
    click.pointer = ArtPoint{float(travelLayout::confirm.x + 1), float(travelLayout::confirm.y + 1)};
    routeHudInput(ui, click);
    check(ui.snapshot().travelRequested && !ui.snapshot().modal(), "click confirms recall intent");
    auto ticket = run.prepareTravel(*ui.snapshot().travelRequested);
    check(run.commitTravel(ticket), "UI recall commits");
    entrance(run);
    CommandController returned(run.game());
    returned.refresh({}, false, false, true);
    returned.interactFixture();
    check(!returned.snapshot().travel.open && returned.snapshot().travelRequested &&
              returned.snapshot().travelRequested->kind == TravelKind::Waypoint &&
              returned.snapshot().travelRequested->depth == 1,
          "sole unlocked hub entry immediately emits waypoint intent");
    UiInputEdges edges;
    std::array<bool, uiKeyCount> down{};
    down[std::size_t(UiKey::Travel)] = true;
    check(edges.sample(down, true).has(UiKey::Travel) && !edges.sample(down, true).has(UiKey::Travel),
          "held Travel has one edge");
    check(!edges.sample(down, false).has(UiKey::Travel) && !edges.sample(down, true).has(UiKey::Travel),
          "blur/refocus does not rearm held Travel");
    down.fill(false);
    edges.sample(down, true);
    down[std::size_t(UiKey::Travel)] = true;
    check(edges.sample(down, true).has(UiKey::Travel), "release rearms Travel");
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
    const Json header = {{"fixture", "pc05"}};
    TraceWriter trace(tracePath);
    trace.append("header", header);
    client.trace = &trace;
    const auto start = client.request("start", {{"scenario", campaignScenario(cat)}});
    check(start.at("ok") && start.at("data").at("scenario_kind") == "campaign", "campaign protocol launch");
    entrance(client);
    const auto preview = client.request("preview", {{"action", {{"kind", "interact"}, {"fixture", 0}}}});
    check(preview.at("ok") && preview.at("data").at("accepted"), "wire stair preview");
    check(client.act({{"kind", "interact"}, {"fixture", 0}}).at("data").at("accepted"), "wire stair travel");
    check(client.protocol.state().at("campaign").at("location") == 1, "wire campaign location");
    const auto kitCount = client.protocol.session()->game().party().campSupplies().kits;
    check(client.request("preview", {{"action", {{"kind", "recall"}}}}).at("data").at("accepted"),
          "wire recall preview");
    check(client.act({{"kind", "recall"}}).at("data").at("accepted") &&
              client.protocol.session()->game().party().campSupplies().kits == kitCount - 1,
          "wire recall charges exactly one kit");
    check(!client.act({{"kind", "recall"}}).at("data").at("accepted").get<bool>() &&
              client.protocol.session()->game().party().campSupplies().kits == kitCount - 1,
          "repeated wire recall at hub does not charge again");
    entrance(client);
    check(client.act({{"kind", "waypoint"}, {"depth", 1}}).at("data").at("accepted"), "wire entry return");
    auto upstairs = stair(client.protocol.session()->game(), FixtureKind::Upstairs);
    const auto stairFacing = client.protocol.session()->game().fixtures().find(upstairs.id)->facing;
    while (client.protocol.session()->game().movement().player.heading != stairFacing) {
        check(client.act({{"kind", "move"}, {"direction", "turn_right"}}).at("data").at("accepted"),
              "face staircase");
        client.advance();
    }
    check(client.act({{"kind", "interact"}, {"fixture", upstairs.id.slot}}).at("data").at("accepted"),
          "wire free ascent");
    const auto state = client.protocol.state();
    check(state.at("campaign").at("locations")[1].contains("state_digest"),
          "inactive integrity digest included");
    trace.finish();
    client.trace = nullptr;
    unsigned unlocks = 0, recallEvents = 0, warpEvents = 0;
    std::istringstream wire(client.recordedOutput.str());
    for (std::string line; std::getline(wire, line);) {
        const auto record = Json::parse(line);
        if (record.at("type") != "events")
            continue;
        for (const auto& event : record.at("events")) {
            if (event.at("event") == "entry_unlocked")
                ++unlocks;
            if (event.at("event") != "campaign_transition")
                continue;
            if (event.at("kind") == "recall") {
                ++recallEvents;
                check(event.at("kit_charge") == 1, "recall event reports actual single charge");
            } else if (event.at("kind") == "waypoint") {
                ++warpEvents;
                check(event.at("kit_charge") == 0, "waypoint event reports zero charge");
            }
        }
    }
    check(unlocks == 1 && recallEvents == 1 && warpEvents == 1,
          "only committed first arrival unlocks; rejected recall emits no transition");
    Protocol replayed(blobber::test::campaignBlueprints(), cat, defs, Mode::Diagnostic);
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
    Protocol negative(blobber::test::campaignBlueprints(), cat, defs, Mode::Diagnostic);
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
    entrance(failure);
    const auto original = failure.protocol.state();
    const auto failed = failure.act({{"kind", "interact"}, {"fixture", 0}});
    check(failed.at("ok") && !failed.at("data").at("accepted") &&
              failure.protocol.state().at("campaign") == original.at("campaign"),
          "view preparation failure preserves campaign");
    int recallCalls = -1;
    Client recallFailure(cat, defs, Mode::Visual, factory, &recallCalls);
    check(recallFailure.request("start", {{"scenario", campaignScenario(cat)}}).at("ok"),
          "recall view failure fixture starts");
    entrance(recallFailure);
    check(recallFailure.act({{"kind", "interact"}, {"fixture", 0}}).at("data").at("accepted"),
          "recall failure fixture enters floor");
    const auto recallBefore = recallFailure.protocol.state();
    const auto inv = recallFailure.protocol.session()->game().party().inventory().revision();
    const auto failedRecall = recallFailure.act({{"kind", "recall"}});
    check(!failedRecall.at("data").at("accepted").get<bool>() &&
              recallFailure.protocol.state().at("campaign") == recallBefore.at("campaign") &&
              recallFailure.protocol.session()->game().party().inventory().revision() == inv,
          "recall view allocation failure retains kit and canonical campaign");
}

} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string_view(argv[1]) == "--replay") {
            const auto cat = catalog();
            const auto defs = definitions();
            Protocol protocol(blobber::test::campaignBlueprints(), cat, defs, Mode::Diagnostic);
            replay(protocol, argv[2], Json{{"fixture", "pc05"}});
            std::cout << "campaign fresh-process replay passed\n";
            return 0;
        }
        const std::filesystem::path retain =
            argc == 3 && std::string_view(argv[1]) == "--retain" ? argv[2] : "";
        cityCampaign();
        std::cout << "campaign\n";
        campaign();
        std::cout << "transferFailuresAndEffects\n";
        transferFailuresAndEffects();
        std::cout << "inventoryAndLifecycle\n";
        inventoryAndLifecycle();
        std::cout << "allPortableEffects\n";
        allPortableEffects();
        std::cout << "recallAndWaypoints\n";
        recallAndWaypoints();
        std::cout << "recallSafetyAndRollback\n";
        recallSafetyAndRollback();
        std::cout << "travelUi\n";
        travelUi();
        adapters(retain);
        std::cout << "campaign checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
