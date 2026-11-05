#include "game/campaign.hpp"
#include "render/spell_lights.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/se02_checks.hpp"
#include "support/spell_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace blobber;
using namespace blobber::world_render;
namespace {
void commands() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog(1000, 1, 30);
    for (int count : {1, 12}) {
        const auto places = spellPlacements(map, count, true);
        GameSession game(map, catalog, places, 5, spellSpawn, false);
        spellReady(game, 3);
        auto command = spellCommand(game, rules::SpellKind::BurningHands);
        const auto now = game.timeMs();
        const auto out = game.submit(command);
        check(out.reason == CommandReason::Resolved, "Area spell accepted normally");
        const auto& snapshot = game.combat().spellVisuals();
        unsigned casts = 0;
        for (const auto& c : snapshot.casts)
            if (c.id.serial) {
                ++casts;
                check(c.count == unsigned(count) && c.impactMs == now,
                      "One cast carries all targets at instant deadline");
                for (unsigned i = 0; i < c.count; ++i) {
                    check(c.targets[i].hit && c.targets[i].damage >= 0,
                          "Resolved contact and damage published");
                    check(c.targets[i].actor.world == snapshot.world,
                          "Target identity belongs to cast world");
                }
                check(c.eligible[0] && c.eligible[1] && c.eligible[2],
                      "Footprint includes unoccupied eligible cells");
            }
        check(casts == 1, "No duplicate cone per victim");
        const auto revision = snapshot.revision;
        check(game.submit(command).reason != CommandReason::Resolved && snapshot.revision == revision,
              "Rejected repeat emits no cue");
    }
    for (auto kind : {rules::SpellKind::FireBolt, rules::SpellKind::RayOfFrost}) {
        const auto places = spellPlacements(map, 1);
        GameSession game(map, catalog, places, 5, spellSpawn, false);
        auto command = spellCommand(game, kind);
        command.targetCount = 1;
        command.enemies[0] = game.actors().id(0);
        check(game.submit(command).event == CombatEvent::AttackStarted, "Projectile accepted");
        const auto first = game.combat().spellVisuals().casts[0];
        check(first.stage == AttackStage::Flight && first.impactMs > first.releaseMs,
              "Flight uses authoritative deadline");
        game.update(.7, false);
        const auto& resolved = game.combat().spellVisuals().casts[0];
        check(resolved.id.serial == first.id.serial && resolved.stage == AttackStage::Resolved,
              "Large step retains resolved cue");
        check(resolved.impactMs == first.impactMs && resolved.targets[0].damage > 0,
              "Retained cue has impact time and actual damage");
        if (kind == rules::SpellKind::RayOfFrost)
            check(game.effects().visualConditions().slow[0],
                  "Slow visual comes from authoritative condition");
    }
    for (unsigned scenario = 0; scenario < 3; ++scenario) {
        const auto outcomeCatalog = spellCatalog(scenario == 1 ? 1 : 1000, scenario == 0 ? 30 : 1);
        const auto places = spellPlacements(map, 1);
        GameSession session(map, outcomeCatalog, places, 5, spellSpawn, false);
        auto command = spellCommand(session, rules::SpellKind::FireBolt);
        command.targetCount = 1;
        command.enemies[0] = session.actors().id(0);
        allocations = 0;
        watching = true;
        const auto accepted = session.submit(command);
        if (scenario == 2)
            session.actors().remove(command.enemies[0]);
        session.update(.7, false);
        watching = false;
        check(!allocations && accepted.event == CombatEvent::AttackStarted,
              "Cast/outcome publication allocates nothing");
        const auto& target = session.combat().spellVisuals().casts[0].targets[0];
        if (scenario == 0)
            check(!target.hit && !target.damage, "Miss outcome cannot create hit aftermath");
        if (scenario == 1)
            check(target.hit && target.died, "Lethal outcome retains dying-body identity");
        if (scenario == 2)
            check(target.gone, "Removed target is recorded without fabricated contact");
    }
    const auto empty = spellPlacements(map, 0, true);
    GameSession game(map, catalog, empty, 5, spellSpawn, false);
    check(game.submit(spellCommand(game, rules::SpellKind::BurningHands)).reason ==
              CommandReason::MissingTarget,
          "Entirely empty cast retains existing command rejection");
    check(!game.combat().spellVisuals().revision, "Rejected empty cast emits nothing");
}
void playback() {
    test::VisibilityFixture fixture;
    world_assets::CpuAssets assets(fixture.publication());
    auto lease = assets.acquire(world_assets::allAssets(assets.publication));
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map, 1, true);
    ActorWorld actors(map, catalog, places);
    MovementState movement;
    movement.player = spellSpawn;
    WorldFrame world;
    world.camera = camera(movement, map);
    world.revision = 1;
    world.count = 1;
    auto& body = world.billboards[0];
    body.id = 0;
    body.alive = true;
    body.feet = {5.5f, 0, 7.5f};
    body.width = .5f;
    body.height = .7f;
    body.depth = 1;
    body.torso = {.5f, .45f};
    SpellVisualJournal journal;
    journal.clear(actors.snapshot().world);
    auto* cast = journal.begin(rules::SpellKind::BurningHands, {3}, spellSpawn, 100);
    cast->cells = {Cell{5, 7}, Cell{6, 7}, Cell{7, 7}};
    cast->eligible = {true, true, false};
    FrameInput in{map, movement, &actors};
    in.spells = &journal.snapshot();
    in.now = 100;
    EffectsPipeline pipeline(assets);
    check(pipeline.update(in, world) && pipeline.frame().count > 0,
          "Zero-recipient cue still draws eligible cone cells");
    for (unsigned i = 0; i < pipeline.frame().count; ++i)
        if (pipeline.frame().packets[i].footprint)
            check(pipeline.frame().packets[i].footprint == (512 | 1024 | 3),
                  "Cone retains exact eligible mask");
    const auto revision = pipeline.frame().revision;
    check(!pipeline.update(in, world) && pipeline.frame().revision == revision,
          "Pause/repeated snapshot reuses colour");
    in.now = 580;
    pipeline.update(in, world);
    check(pipeline.frame().count == 0, "Cone ends exactly at 480 ms");
    in.now = 1200;
    pipeline.update(in, world);
    in.now = 1400;
    check(!pipeline.update(in, world), "Expired cast resumes idle caching");
    auto* fire = journal.begin(rules::SpellKind::FireBolt, {3}, spellSpawn, 1500);
    fire->count = 1;
    fire->targets[0] = {actors.id(0), {5.5f, 7.5f}, 0, 5, true, false, false, false};
    in.now = 1500;
    pipeline.update(in, world);
    check(pipeline.frame().count > 0, "Hit creates independent aftermath");
    in.now = 1800;
    pipeline.update(in, world);
    check(pipeline.frame().count > 0, "Aftermath outlives original 120 ms retention");
    body.alive = false;
    in.now = 1900;
    ++world.revision;
    pipeline.update(in, world);
    check(pipeline.frame().count > 0, "Visible dying pose retains cosmetic flames");
    world.count = 0;
    ++world.revision;
    in.now = 1950;
    pipeline.update(in, world);
    check(!pipeline.frame().count, "No detached effect after body disappears");
    world.count = 1;
    body.alive = true;
    ++world.revision;
    in.now = 2300;
    pipeline.update(in, world);
    check(!pipeline.frame().count, "Expired hit never replays on visibility return");
    ActorConditionVisualSnapshot conditions;
    conditions.world = actors.snapshot().world;
    conditions.slow[0] = true;
    in.conditions = &conditions;
    pipeline.update(in, world);
    check(pipeline.frame().frost[0] > 0, "Restored Slow needs no cast burst");
    in.now = 2400;
    check(!pipeline.update(in, world), "Sustained frost permits idle colour caching");
    conditions.slow.reset();
    pipeline.update(in, world);
    check(pipeline.frame().frost[0] == 0, "Authoritative expiry clears coating immediately");
    // Force bounded overload through meaningful distinct cast/target identities.
    for (unsigned i = 0; i < 20; ++i) {
        auto* c = journal.begin(rules::SpellKind::BurningHands, {3}, spellSpawn, 2500);
        c->cells = {Cell{5, 7}, Cell{6, 7}, Cell{7, 7}};
        c->eligible = {true, true, true};
        c->count = 36;
        for (auto& t : c->targets)
            t = {actors.id(0), {5.5f, 7.5f}, 0, 5, true, false, false, false};
    }
    check(journal.snapshot().dropped > 0, "Journal overload is explicit and bounded");
    in.now = 2500;
    pipeline.update(in, world);
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 45; ++i) {
        in.now = 2500 + i * 20;
        pipeline.update(in, world);
    }
    watching = false;
    check(allocations == 0, "Overlapping elemental frame processing allocates nothing");
    check(pipeline.frame().count <= effectCapacity && pipeline.frame().lightCount <= spellLightCapacity,
          "Exhaustion stays bounded");
    in.now = 4000;
    pipeline.update(in, world);
    check(!pipeline.frame().count && !pipeline.frame().lightCount,
          "Repeated overflow snapshots cannot resurrect dropped effects");
    journal.clear(actors.snapshot().world);
    auto* restored = journal.begin(rules::SpellKind::BurningHands, {3}, spellSpawn, 4100);
    restored->cells = {Cell{5, 7}, Cell{6, 7}, Cell{7, 7}};
    restored->eligible = {true, true, true};
    in.now = 4100;
    pipeline.update(in, world);
    check(pipeline.frame().count > 0, "Same-world restore resets cosmetic consumer epoch");
    const auto newId = restored->id.serial;
    journal.clear(actors.snapshot().world);
    pipeline.update(in, world);
    check(!pipeline.frame().count, "Restore clears live bursts immediately");
    check(journal.begin(rules::SpellKind::FireBolt, {3}, spellSpawn, 4200)->id.serial > newId,
          "Same-world cast identities never recycle");
    in.now = 9000;
    pipeline.update(in, world);
    check(!pipeline.frame().count, "Large jump does not replay an old application burst");
    AttackVisual pending;
    pending.id = {actors.snapshot().world, 1};
    pending.stage = AttackStage::Flight;
    pending.spell = rules::SpellKind::FireBolt;
    pending.partyOrigin = spellSpawn;
    pending.releaseMs = 9100;
    pending.impactMs = 9300;
    SpellTargetVisual target{actors.id(0), {5.5f, 7.5f}, 0, 0, true};
    journal.attack(pending, target);
    in.now = 9150;
    pipeline.update(in, world);
    check(pipeline.frame().count > 0, "Pending cast draws flight");
    journal.resolve(pending.id, target, 9150, true);
    pipeline.update(in, world);
    check(!pipeline.frame().count && !pipeline.frame().lightCount,
          "Canceled cast clears flight without target aftermath");
    auto* immune = journal.begin(rules::SpellKind::FireBolt, {3}, spellSpawn, 9200);
    immune->count = 1;
    immune->targets[0] = target;
    in.now = 9200;
    pipeline.update(in, world);
    check(pipeline.frame().count > 0, "Zero-damage hit retains compact contact burst");
    in.now = 9440;
    pipeline.update(in, world);
    check(!pipeline.frame().count && !pipeline.frame().lightCount,
          "Zero damage cannot leave damaging-looking flames after contact");

    journal.clear(actors.snapshot().world);
    pending.spell = rules::SpellKind::MagicMissile;
    pending.id.serial = 2;
    pending.releaseMs = 10000;
    pending.impactMs = 10300;
    pending.darts = 3;
    target.hit = true;
    target.damage = 4;
    journal.attack(pending, target);
    in.now = 10120;
    pipeline.update(in, world);
    const auto& force = assets.publication.effects[unsigned(world_assets::EffectClipKind::ForceFlight)];
    unsigned darts = 0;
    for (unsigned i = 0; i < pipeline.frame().count; ++i) {
        const auto& p = pipeline.frame().packets[i];
        darts += !p.additive && p.texture == force.body && p.uv[1] == force.frames[1][1];
    }
    check(darts == 3, "One target renders all three assigned force darts");

    journal.clear(actors.snapshot().world);
    pending.spell = rules::SpellKind::ScorchingRay;
    pending.id.serial = 3;
    pending.darts = 1;
    journal.attack(pending, target);
    auto extra = pending;
    extra.id.serial = 4;
    extra.impactMs = 10450;
    auto miss = target;
    miss.hit = false;
    miss.damage = 0;
    journal.attack(extra, miss, pending.id);
    allocations = 0;
    watching = true;
    for (unsigned now = 10120; now <= 10900; now += 30) {
        if (now == 10300)
            journal.resolve(pending.id, target, now);
        if (now == 10450)
            journal.resolve(extra.id, miss, now);
        in.now = now;
        pipeline.update(in, world);
    }
    watching = false;
    check(!allocations, "Independent ray flight, mixed contacts and trails allocate nothing on CPU");
    in.now = 11200;
    pipeline.update(in, world);
    check(!pipeline.frame().count && !pipeline.frame().lightCount,
          "Mixed same-target rays expire without replaying either outcome");
    conditions.guiding[0] = true;
    pipeline.update(in, world);
    check(pipeline.frame().guiding[0] > 0, "Guiding snapshot restores quiet coating without a cast");
    in.now = 11300;
    check(!pipeline.update(in, world), "Guiding alone permits idle colour caching");
    conditions.guiding.reset();
    pipeline.update(in, world);
    check(pipeline.frame().guiding[0] == 0, "Authoritative guiding removal clears coating");
}
void lightBlockers() {
    const Map map({"#######", "#.....#", "#.....#", "#..#..#", "#.....#", "#.....#", "#######"});
    MovementState movement;
    movement.player = {{3, 5}, Heading::North};
    FrameInput in{map, movement};
    WorldFrame world;
    EffectFrame effects;
    effects.lightCount = 1;
    effects.lights[0] = {{3.5f, .5f, 4.1f}, {1, .3f, .05f}, 1.5f, 1, 3};
    const auto data = prepareSpellLights(in, world, effects);
    check(spellLightVisible(data, 0, {3.5f, .01f, 4.2f}, {0, 1, 0}), "Light reaches nearby floor");
    check(spellLightVisible(data, 0, {3.5f, .5f, 4.f}, {0, 0, 1}), "Near wall face is illuminated");
    check(!spellLightVisible(data, 0, {3.5f, .5f, 2.9f}), "Light cannot cross wall");
    check(!spellLightVisible(data, 0, {3.5f, -.2f, 4.2f}), "Light cannot cross floor");
    check(!spellLightVisible(data, 0, {3.5f, 1.2f, 4.2f}), "Light cannot cross ceiling");
    const Map corner({"#######", "#.....#", "#.....#", "#.#...#", "#.....#", "#.....#", "#######"});
    FrameInput cornerInput{corner, movement};
    effects.lights[0].position = {2.5f, .5f, 2.5f};
    check(!spellLightVisible(prepareSpellLights(cornerInput, world, effects), 0, {3.5f, .5f, 3.5f}),
          "Exact diagonal corner cannot leak through the unvisited adjacent wall");
    effects.lights[0].position = {3.5f, .5f, 4.1f};
    const Map room({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
    FloorFixtures fixtures;
    fixtures.count = 1;
    fixtures.entries[0] = {{3, 3}, Heading::North, FixtureKind::Door, 0};
    const auto catalog = spellCatalog();
    NpcWorld places(room, {3, 4}, {"goblin"}, {});
    GameSession game(room, catalog, places, 5, {{3, 4}, Heading::North}, false, {}, {}, fixtures);
    DoorSnapshot doors = game.doors();
    FrameInput doorInput{game.geometry(), movement};
    doorInput.doors = &doors;
    auto sample = [&] {
        world.doorCount = doorBoxes(game.geometry(), &doors, world.doors, 0, 0, 0);
        return prepareSpellLights(doorInput, world, effects);
    };
    check(!spellLightVisible(sample(), 0, {3.5f, .5f, 3.5f}), "Closed leaf blocks light");
    const auto changed = game.interact({game.fixtures().id(0), game.fixtures().revision()});
    check(changed.changed, "Owned door opens");
    doors.until[0] = 300;
    doors.now = 150;
    const auto middle = sample();
    doors.now = 300;
    const auto open = sample();
    check(middle.texels != open.texels, "Occlusion uses interpolated door boxes");
    check(spellLightVisible(open, 0, {3.5f, .5f, 3.5f}), "Open leaf reveals light through doorway");
}
void publication() {
    test::VisibilityFixture fixture;
    const auto original = fixture.publication();
    check(original.effects[0].count == 8, "Prepared clips round trip");
    const auto path = fixture.location.directory / "metadata.json";
    nlohmann::json json;
    {
        std::ifstream f(path);
        f >> json;
    }
    auto invalid = json;
    invalid["effects"][0]["frames"][0] = {.99, 0, .5, .5};
    {
        std::ofstream f(path);
        f << invalid;
    }
    rejects([&] { world_assets::load(fixture.location.directory); },
            "Out-of-bounds frame rejects publication");
    invalid = json;
    invalid["effects"][0]["body"] = 0;
    {
        std::ofstream f(path);
        f << invalid;
    }
    rejects([&] { world_assets::load(fixture.location.directory); },
            "Material texture cannot be an effect atlas");
    invalid = json;
    invalid["effects"].erase(invalid["effects"].end() - 1);
    {
        std::ofstream f(path);
        f << invalid;
    }
    rejects([&] { world_assets::load(fixture.location.directory); },
            "Incomplete offensive clip catalogue rejects publication");
    invalid = json;
    invalid["effects"][unsigned(world_assets::EffectClipKind::Electricity)]["frameMs"] = 0;
    {
        std::ofstream f(path);
        f << invalid;
    }
    rejects([&] { world_assets::load(fixture.location.directory); },
            "Invalid offensive clip timing rejects before playback");
    {
        std::ofstream f(path);
        f << json;
    }
}
} // namespace
int main() {
    try {
        test::se02Commands();
        test::se02Journal();
        test::se02Portraits();
        commands();
        playback();
        lightBlockers();
        publication();
        std::cout << "SE01/SE02 CPU checks passed; effect_pipeline_bytes=" << sizeof(EffectsPipeline)
                  << " light_scratch_bytes=" << sizeof(SpellLightData)
                  << " journal_bytes=" << sizeof(SpellVisualJournal) << '\n';
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
