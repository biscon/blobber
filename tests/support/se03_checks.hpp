#pragma once
#include "render/world_effects.hpp"
#include "support/se02_checks.hpp"
#include "support/world_visibility_fixtures.hpp"
namespace blobber::test {
inline void se03Commands() {
    using K = rules::SpellKind;
    const auto map = hybridRoom();
    for (auto kind :
         {K::CureWounds, K::SpareTheDying, K::LesserRestoration, K::Aid, K::FalseLife, K::Bless, K::Bane,
          K::Guidance, K::Resistance, K::ShieldOfFaith, K::MageArmor, K::Sanctuary, K::MirrorImage, K::Sleep,
          K::ColorSpray, K::HoldPerson, K::Grease, K::Web, K::SpiritualWeapon}) {
        const bool advanced = rules::spellRules(kind)->level == 2;
        const auto catalog = advanced ? creationCatalog(true) : spellCatalog(1, 1, 1);
        const bool close = kind == K::ColorSpray;
        const auto places = spellPlacements(map, 3, close);
        GameSession game(map, catalog, places, 5, spellSpawn, false);
        if (advanced)
            levelParty(game, 3, kind);
        if (close)
            spellReady(game, 3);
        auto command = spellCommand(game, kind);
        const auto& rule = *rules::spellRules(kind);
        if (rule.ground)
            command.aimedCell = close ? Cell{6, 7} : Cell{6, 4};
        if (!rule.area && !rule.self) {
            command.targetCount = kind == K::Aid || kind == K::Bless ? 3 : 1;
            command.allies = {{{0}, {1}, {2}}};
            command.enemies[0] = game.actors().id(0);
        }
        if (kind == K::MageArmor)
            command.allies[0] = {3};
        if (kind == K::CureWounds)
            game.party().damage({0}, 3);
        if (kind == K::SpareTheDying)
            game.party().damage({0}, game.party().character({0})->hitPoints);
        if (kind == K::LesserRestoration) {
            EffectRequest r;
            r.world = game.actors().snapshot().world;
            r.caster = rules::CharacterId{3};
            r.kind = EffectKind::HoldPerson;
            r.targets[0] = rules::CharacterId{0};
            r.count = 1;
            r.difficulty = 100;
            check(bool(game.applyEffect(r)), "Restore setup applies paralysis");
            command.restoreParalysis = true;
        }
        const auto before = game.supportVisuals().revision;
        CommandController hud(game);
        hud.refresh({}, false, false, true);
        allocations = 0;
        watching = true;
        const auto result = game.submit(command);
        hud.record(game.events());
        hud.refresh({}, false, false, true);
        game.update(.3, false);
        const auto state = game.effects().visualConditions();
        watching = false;
        check(!allocations, "SE03 command, journal, status snapshot and HUD allocate nothing");
        check(result.reason == CommandReason::Resolved,
              (std::string(rule.id) + ": " + commandMessage(result.reason)).c_str());
        if (kind == K::CureWounds)
            check(hud.snapshot().portraitFeedback[0][0].kind == PortraitFeedbackKind::Healing,
                  "Accepted green healing path remains");
        else if (kind == K::Sleep || kind == K::ColorSpray) {
            const auto& cue = game.combat().spellVisuals().casts[0];
            check(cue.id.serial && cue.spell == kind && cue.count == 0,
                  "One support area release independent of recipient pulses");
            check(std::count(cue.eligible.begin(), cue.eligible.end(), true) == (kind == K::Sleep ? 9 : 3),
                  "Support release retains eligible empty cells");
        } else
            check(game.supportVisuals().revision > before,
                  "Resolved support command publishes cosmetic facts");
        if (kind == K::MirrorImage)
            check(state.duplicates[3] == 7, "Snapshot exposes all actual duplicate bits");
        if (kind == K::SpareTheDying)
            check(hud.snapshot().portraitFeedback[0][0].support == kind, "Stabilization has its own pulse");
        if (kind == K::SpiritualWeapon) {
            game.update(6, false);
            auto move = featureCommand(game, 1, rules::Feature::SpiritualWeapon);
            move.spell = command.spell;
            move.aimedCell = Cell{7, 4};
            check(game.submit(move).reason == CommandReason::Resolved, "Weapon endpoint move accepted");
            bool found = false;
            for (const auto& cue : game.supportVisuals().records)
                found |= cue.id.serial && cue.phase == SupportPhase::Move && cue.cell == Cell{7, 4} &&
                         cue.from == Cell{6, 4};
            check(found, "Weapon motion retains actual old and new cells");
        }
        if (game.combat().snapshot().phase != CombatPhase::Exploration)
            continue;
        auto save = std::make_unique<FloorState>();
        game.exportFloor(*save);
        const auto oldEpoch = game.supportVisuals().epoch;
        try {
            game.importFloor(*save);
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string(rule.id) + " restore: " + e.what());
        }
        hud.refresh({}, false, false, true);
        check(game.supportVisuals().epoch > oldEpoch && !game.supportVisuals().revision,
              "Restore clears support bursts without adding save state");
        check(game.effects().visualConditions().duplicates == state.duplicates,
              "Restore reconstructs actual duplicate state");
    }
}
inline void se03Lifecycle() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const auto places = spellPlacements(map, 2);
    GameSession game(map, catalog, places, 5, spellSpawn, false);
    levelParty(game, 3, rules::SpellKind::MirrorImage);
    rules::DiceRng rng(5);
    EffectController effects(game.party(), game.actors(), rng);
    SupportVisualJournal journal;
    journal.clear(game.actors().snapshot().world);
    journal.bind(effects);
    CombatEvents events;
    events.supportJournal = &journal;
    effects.bindEvents(events, 100);
    EffectRequest r;
    r.world = game.actors().snapshot().world;
    r.caster = rules::CharacterId{3};
    r.kind = EffectKind::MirrorImage;
    r.targets[0] = rules::CharacterId{3};
    r.count = 1;
    const auto mirror = effects.apply(r, 100, events);
    check(bool(mirror), "Mirror fixture applies");
    effects.resolveMirror({mirror.id, 1, true}, true, 200, events);
    check(effects.visualConditions().duplicates[3] == 7, "Canceled duplicate hit preserves image");
    effects.resolveMirror({mirror.id, 1, true}, false, 300, events);
    check(effects.visualConditions().duplicates[3] == 6, "Resolved loss removes only its actual image");
    bool loss = false;
    for (const auto& cue : journal.snapshot().records)
        loss |= cue.phase == SupportPhase::ImageLost && cue.duplicate == 1 && cue.occurredMs == 300;
    check(loss, "Partial duplicate loss emits one timed cosmetic event");
    r.kind = EffectKind::Bless;
    r.caster = rules::CharacterId{1};
    r.count = 2;
    r.targets[0] = game.actors().id(0);
    r.targets[1] = game.actors().id(1);
    const auto bless = effects.apply(r, 400, events);
    check(bool(bless), "Multi-recipient boon applies");
    effects.remove(bless.id, 500, events);
    unsigned ends = 0;
    for (const auto& cue : journal.snapshot().records)
        ends += cue.phase == SupportPhase::End && cue.effect.serial == bless.id.serial && cue.target.npc;
    check(ends == 2, "Whole-effect end retains each actual recipient before deactivation");
    check(effects.visualConditions().actors[0] == 0, "Ended boon disappears from sustained snapshot");
    for (unsigned i = 0; i < 140; ++i) {
        SupportVisual cue;
        cue.phase = SupportPhase::Placement;
        cue.spell = rules::SpellKind::Web;
        cue.occurredMs = 600;
        journal.emit(cue);
    }
    check(journal.snapshot().dropped > 0, "Bounded cosmetic overwrite reports exhaustion");
}
inline void se03Playback() {
    using namespace world_render;
    VisibilityFixture fixture;
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
    SupportVisualJournal journal;
    journal.clear(actors.snapshot().world);
    SpellVisualJournal spells;
    spells.clear(actors.snapshot().world);
    ActorConditionVisualSnapshot conditions;
    conditions.world = actors.snapshot().world;
    conditions.actors[0] = 1u << unsigned(EffectKind::Bless);
    SpellArea area;
    FrameInput input{map, movement, &actors};
    input.support = &journal.snapshot();
    input.conditions = &conditions;
    input.spells = &spells.snapshot();
    input.area = &area;
    input.now = 100;
    EffectsPipeline pipeline(assets);
    pipeline.update(input, world);
    check(pipeline.frame().count == 1, "Quiet NPC boon has one restrained packet");
    const auto& boon = pipeline.frame().packets[0];
    check(std::abs((boon.points[0].y + boon.points[2].y) * .5f - body.height * .8f * .4375f - body.feet.y) <
              .0001f,
          "Bless's authored lower edge meets registered feet, independent of torso");
    input.now = 200;
    check(!pipeline.update(input, world), "Static boon reuses idle colour");
    SupportVisual cue;
    cue.spell = rules::SpellKind::Bless;
    cue.target = actors.id(0);
    cue.occurredMs = 200;
    journal.emit(cue);
    pipeline.update(input, world);
    check(pipeline.frame().count == 2, "New application outranks quiet mark");
    input.now = 840;
    pipeline.update(input, world);
    check(pipeline.frame().count == 1, "Application expires exactly");
    conditions.actors[0] = (1u << unsigned(EffectKind::Sleep)) | (1u << unsigned(EffectKind::HoldPerson)) |
                           (1u << unsigned(EffectKind::Bless));
    conditions.slow[0] = conditions.guiding[0] = true;
    pipeline.update(input, world);
    check(pipeline.frame().count == 2 && !pipeline.frame().frost[0] && !pipeline.frame().guiding[0],
          "Two disabling cues cap overlap and suppress quieter coatings");
    auto& patch = area.ground.areas[0];
    patch.active = patch.web = true;
    patch.burnMs = 6000;
    patch.cell = {6, 7};
    area.ground.areas[1] = patch;
    ++area.ground.revision;
    pipeline.update(input, world);
    check(pipeline.frame().lightCount == 1, "Co-located burning Web shares one light");
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 24; ++i) {
        input.now = 900 + i * 40;
        pipeline.update(input, world);
    }
    watching = false;
    check(!allocations, "Support overlap and Web playback allocate nothing");
    conditions = {};
    conditions.world = actors.snapshot().world;
    area.ground = {};
    ++area.ground.revision;
    journal.clear(actors.snapshot().world);
    input.now = 2500;
    pipeline.update(input, world);
    check(!pipeline.frame().count && !pipeline.frame().lightCount,
          "Restore/removal clears support colour and light");
    input.now = 2600;
    check(!pipeline.update(input, world), "Idle caching resumes after support cleanup");

    area.ground.weapons[0] = {{6, 7}, true};
    ++area.ground.revision;
    input.now = 2400;
    pipeline.update(input, world);
    const auto baseline = pipeline.frame().packets[0];
    check(!pipeline.update(input, world), "Paused hover retains the same colour frame");
    // Predict following from the same settled cells used by the command owner.
    // The camera and weapon must advance together, not create a near-plane close-up.
    movement.player.cell = {6, 7};
    movement.transition = {spellSpawn, movement.player, Action::Forward, 0, .16f};
    float initialDepth = 0;
    for (float elapsed : {0.f, .04f, .08f, .12f}) {
        movement.transition.elapsed = elapsed;
        world.camera = camera(movement, map);
        ++world.revision;
        pipeline.update(input, world);
        check(pipeline.frame().count == 1, "Following retains one weapon body through movement");
        const auto depth = pipeline.frame().packets[0].depth;
        if (!elapsed)
            initialDepth = depth;
        check(std::abs(depth - initialDepth) < .00001f,
              "Weapon follows camera easing at constant relative depth");
    }
    movement.transition.elapsed = .16f;
    area.ground.weapons[0].cell = {6, 6}; // Authoritative completion commits the same destination.
    ++area.ground.revision;
    world.camera = camera(movement, map);
    ++world.revision;
    pipeline.update(input, world);
    check(std::abs(pipeline.frame().packets[0].depth - initialDepth) < .00001f,
          "Settling the party step cannot pop or enlarge the weapon");
    movement = {};
    movement.player = spellSpawn;
    area.ground.weapons[0].cell = {6, 7};
    ++area.ground.revision;
    world.camera = camera(movement, map);
    ++world.revision;
    pipeline.update(input, world);
    input.now = 2420;
    check(!pipeline.update(input, world), "Hover reuses colour within its 40ms sample");
    input.now = 3000;
    check(pipeline.update(input, world), "Visible idle weapon animates without ground changes");
    const auto high = pipeline.frame().packets[0];
    check(high.points[0].y > baseline.points[0].y && high.points[0].y - baseline.points[0].y <= .0251f &&
              high.points[0].x == baseline.points[0].x && high.points[0].z == baseline.points[0].z,
          "Hover is small vertical-only presentation motion");
    input.now = 4800;
    pipeline.update(input, world);
    check(std::abs(pipeline.frame().packets[0].points[0].y - baseline.points[0].y) < .0001f,
          "Hover returns to its initial position each cycle");
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 60; ++i) {
        input.now += 40;
        pipeline.update(input, world);
    }
    watching = false;
    check(!allocations, "Idle hover allocates nothing");
    AttackVisualSnapshot attacks;
    attacks.world = actors.snapshot().world;
    auto& attack = attacks.attacks[NpcWorld::maxInstances];
    attack.id = {attacks.world, 1};
    attack.stage = AttackStage::Windup;
    attack.spell = rules::SpellKind::SpiritualWeapon;
    attack.startedMs = input.now;
    attack.impactMs = input.now + 200;
    attack.targetPosition = {6.25f, 7.25f};
    attack.npcOrigin.cell = {6, 7};
    attack.enemy = actors.id(0);
    input.attacks = &attacks;
    auto weaponCenter = [&] {
        const auto& packet = pipeline.frame().packets[0];
        return Vec{(packet.points[0].x + packet.points[2].x) * .5f,
                   (packet.points[0].y + packet.points[2].y) * .5f,
                   (packet.points[0].z + packet.points[2].z) * .5f};
    };
    pipeline.update(input, world);
    input.now += 100;
    check(pipeline.update(input, world), "Weapon windup animates before melee impact");
    const auto halfway = weaponCenter();
    check(halfway.x < 6.5f && halfway.x > 6.25f, "Hammer flies toward recorded victim subcell");
    input.now = attack.impactMs;
    attack.stage = AttackStage::Resolved;
    attack.targetGone = true;
    ++attacks.revision;
    pipeline.update(input, world);
    const auto impact = weaponCenter();
    check(std::abs(impact.x - 6.25f) < .0001f && std::abs(impact.z - 7.25f) < .0001f,
          "Attack reaches recorded contact even if the target disappears");
    input.now += 240;
    pipeline.update(input, world);
    const auto returned = weaponCenter();
    const auto hovering = spiritualWeaponPosition(map, {6, 7}, input.now);
    check(std::abs(returned.x - hovering.x) < .0001f && std::abs(returned.y - hovering.y) < .0001f &&
              std::abs(returned.z - hovering.z) < .0001f && area.ground.weapons[0].cell == Cell{6, 7},
          "Hammer returns to current hover without changing authoritative ground position");
    input.now = attack.startedMs + 100;
    attack.stage = AttackStage::Canceled;
    ++attacks.revision;
    pipeline.update(input, world);
    check(std::abs(weaponCenter().x - 6.5f) < .0001f, "Canceled attack resumes hover immediately");
    input.attacks = nullptr;
    area.ground.weapons[0].cell = {6, 9}; // Behind the north-facing camera.
    ++area.ground.revision;
    pipeline.update(input, world);
    input.now += 400;
    check(!pipeline.update(input, world), "Offscreen weapon preserves idle caching");
    area.ground.weapons[0].active = false;
    ++area.ground.revision;
    pipeline.update(input, world);
    input.now += 400;
    check(!pipeline.update(input, world) && !pipeline.frame().count,
          "Dismissal clears weapon and restores idle caching");
}
} // namespace blobber::test
