#include "render/frame_pipeline.hpp"
#include "render/transitions.hpp"
#include "support/actor_fixtures.hpp"
#include "support/combat_fixtures.hpp"
#include "support/check.hpp"
#include "support/hybrid_fixtures.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <chrono>
#include <iostream>
#include <limits>
using namespace blobber;
namespace {
const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
ActorWorld world(NpcAnimation animation, std::uint32_t phase = 0) {
    return testActors(map, {3, 5}, {"test"}, {{{0}, {3, 3}, {0, 0}, Heading::South, animation, phase}});
}
void timing() {
    const auto metadata = npcTimings(testAnimatedNpcAppearance());
    const auto npcs = world(NpcAnimation::Attack);
    NpcAnimator animator(npcs, {metadata});
    for (const auto& pair : std::array<std::array<std::uint64_t, 2>, 10>{{{0, 0},
                                                                          {69, 0},
                                                                          {70, 1},
                                                                          {94, 1},
                                                                          {95, 2},
                                                                          {119, 2},
                                                                          {120, 3},
                                                                          {154, 3},
                                                                          {155, 0},
                                                                          {1000, 1}}}) {
        animator.setTime(pair[0]);
        check(animator.snapshot().frames[0].active == pair[1], "Attack timing boundaries and wraparound");
        check(animator.snapshot().frames[0].idle == (pair[0] % 120 >= 60),
              "Fallback has independent idle timeline");
    }
    animator.setTime(0);
    const auto revision = animator.snapshot().revision;
    animator.setTime(59);
    check(animator.snapshot().revision == revision, "No revision change within held frames");
    animator.setTime(60);
    check(animator.snapshot().revision == revision + 1, "Fallback change publishes new selection");
    const auto phased = world(NpcAnimation::Walk, std::numeric_limits<std::uint32_t>::max());
    NpcAnimator phase(phased, {metadata});
    const auto huge = std::numeric_limits<std::uint64_t>::max();
    phase.setTime(huge);
    check(phase.snapshot().frames[0].active ==
              ((huge % 60 + phased.snapshot().actors[0].npc.phaseMs % 60) % 60 >= 30),
          "Large elapsed time and phase cannot overflow or require catch-up loops");
    phase.setTime(0);
    check(phase.snapshot().frames[0].active == (phased.snapshot().actors[0].npc.phaseMs % 60 >= 30),
          "Deterministic seek resets time");
    const auto idle = world(NpcAnimation::Idle);
    NpcAnimator still(idle, {npcTimings(testNpcAppearance())});
    still.setTime(huge);
    check(still.snapshot().revision == 0 && still.snapshot().frames[0].active == 0,
          "Legacy static idle never advances");
    rejects([&] { NpcAnimator bad(npcs, {}); }, "Metadata count mismatch");
    rejects([&] { NpcAnimator bad(npcs, {npcTimings(testNpcAppearance())}); },
            "Absent selected clip rejected");
    auto bad = metadata;
    bad[0].durationMs[0] = 0;
    rejects([&] { NpcAnimator invalid(idle, {bad}); }, "Zero duration rejected");
    bad = metadata;
    bad[2].count = 5;
    rejects([&] { NpcAnimator invalid(idle, {bad}); }, "Bounded frame count");
    auto empty = testActors(map, {1, 1}, {}, {});
    NpcAnimator none(empty, {});
    none.setTime(huge);
    check(none.snapshot().count == 0 && none.snapshot().revision == 0, "Empty world clock");
}
void actualWalking() {
    auto actors = world(NpcAnimation::Idle);
    const auto id = actors.id(0);
    auto timings = npcTimings(testReactionAppearance());
    NpcAnimator animator(actors, {timings});
    check(actors.walk(id, {3, 2}, {0, 0}, {3, 5}, 100).changed, "Successful cross-cell walk commits");
    animator.setTime(100);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Walk,
          "Successful move triggers walk at frame zero");
    animator.setTime(699);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Walk, "Walk lasts 600 ms");
    animator.setTime(700);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Idle, "Walk expires to idle");
    check(!actors.walk(id, {0, 0}, {0, 0}, {3, 5}, 800), "Wall step rejected");
    animator.setTime(800);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Idle, "Rejected step cannot animate walking");
    actors.walk(id, {3, 3}, {0, 0}, {3, 5}, 900);
    actors.attack(id, 950);
    animator.setTime(950);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Attack, "Attack interrupts walk");
    animator.setTime(1200);
    check(animator.snapshot().frames[0].clip != NpcAnimation::Walk,
          "Canceled walk cannot resume after attack");
    actors.walk(id, {3, 2}, {0, 0}, {3, 5}, 1500);
    actors.damage(id, 1, 1550);
    animator.setTime(1550);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Hurt, "Hurt interrupts walk");
    animator.setTime(2050);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Idle, "Damage canceled old walking");
    NpcAnimator legacy(actors, {npcTimings(testNpcAppearance())});
    actors.walk(id, {3, 3}, {0, 0}, {3, 5}, 2200);
    legacy.setTime(2200);
    check(legacy.snapshot().frames[0].clip == NpcAnimation::Idle, "Missing walk art remains idle");
    actors.damage(id, 10000, 2250);
    animator.setTime(2250);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Death, "Lethal damage interrupts walk");
    check(!actors.walk(id, {3, 2}, {0, 0}, {3, 5}, 2300), "Dead actor cannot walk");
    auto old = world(NpcAnimation::Idle);
    NpcAnimator oldAnimator(old, {npcTimings(testAnimatedNpcAppearance())});
    old.walk(old.id(0), {3, 2}, {0, 0}, {3, 5}, 100);
    old.damage(old.id(0), 10000, 120);
    oldAnimator.setTime(150);
    check(oldAnimator.snapshot().frames[0].clip == NpcAnimation::Walk &&
              oldAnimator.snapshot().frames[0].flash,
          "Legacy lethal flash freezes the walking pose");
}
void travelTiming() {
    auto actors = world(NpcAnimation::Idle);
    const auto id = actors.id(0);
    NpcAnimator animator(actors, {npcTimings(testNpcAppearance())});
    actors.walk(id, {4, 3}, {0, 0}, {3, 5}, 100);
    check(actors.occupied({4, 3}) && !actors.occupied({3, 3}), "Travel commits grid collision immediately");
    animator.setTime(100);
    check(animator.snapshot().frames[0].offsetX == -1, "Travel starts at source, including without walk art");
    const auto revision = animator.snapshot().revision;
    animator.setTime(101);
    check(animator.snapshot().revision > revision, "Position invalidates a held animation frame");
    animator.setTime(400);
    check(animator.snapshot().frames[0].offsetX == -.5f, "Halfway position is halfway between cells");
    const auto held = animator.snapshot().revision;
    animator.setTime(400);
    check(animator.snapshot().revision == held, "Paused presentation reuses the same position");
    animator.setTime(700);
    check(animator.snapshot().frames[0].offsetX == 0, "Arrival has exact destination position");
    actors.walk(id, {4, 2}, {1, 1}, {3, 5}, 700);
    const auto midway = actorPosition(*actors.actor(id), 1000);
    check(midway.x == 4.5f && midway.y == 3.f, "Travel includes changed subcell coordinates");
    actors.walk(id, {3, 2}, {0, 0}, {3, 5}, 1000);
    const auto chained = actorPosition(*actors.actor(id), 1000);
    check(chained.x == midway.x && chained.y == midway.y, "Early chained step starts without snapping");
    actors.move(id, {3, 3}, {0, 0}, {3, 5});
    check(actorPosition(*actors.actor(id), 1001).y == 3.25f, "Explicit move cancels travel");
    actors.walk(id, {3, 2}, {0, 0}, {3, 5}, 1100);
    actors.damage(id, 10000, 1400);
    const auto dead = actorPosition(*actors.actor(id), 1400);
    check(dead.y == 2.75f && actorPosition(*actors.actor(id), 5000).y == dead.y,
          "Lethal damage freezes travel where the actor died");
}
void travelRendering() {
    const auto appearance = testNpcAppearance();
    const PreparedNpcs prepared({appearance});
    auto actors = world(NpcAnimation::Idle);
    NpcAnimator animator(actors, {npcTimings(appearance)});
    auto halfway = testActors(map, {3, 5}, {"test"}, {{{0}, {3, 3}, {1, 0}, Heading::South}});
    const std::array<Player, 4> cameras{{{{3, 5}, Heading::North},
                                         {{3, 1}, Heading::South},
                                         {{5, 3}, Heading::West},
                                         {{1, 3}, Heading::East}}};
    std::array<float, viewWidth> walls;
    walls.fill(100);
    Frame moving, reference;
    PickingBuffer pick, expected;
    actors.walk(actors.id(0), {4, 3}, {0, 0}, {3, 5}, 0);
    animator.setTime(300);
    for (const auto camera : cameras) {
        pick.clear();
        expected.clear();
        prepared.compose(actors, camera, 0, walls, moving, &animator.snapshot(), &pick);
        prepared.compose(halfway, camera, 0, walls, reference, nullptr, &expected);
        std::size_t body = 0;
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                check(pick.slot(x, y) == expected.slot(x, y),
                      "Moving body projects exactly at the intermediate subslot from every heading");
                body += pick.slot(x, y) == 0;
            }
        check(body > 0, "Travel projection fixture is visible");
    }
    const Player camera{{3, 5}, Heading::North};
    for (float invalidOffset : {std::numeric_limits<float>::quiet_NaN(),
                                std::numeric_limits<float>::infinity(), float(Map::maxDimension + 1)}) {
        auto invalid = animator.snapshot();
        invalid.frames[0].offsetX = invalidOffset;
        rejects([&] { prepared.compose(actors, camera, 0, walls, moving, &invalid); },
                "Malformed travel offsets cannot reach projection arithmetic");
    }
    walls.fill(.1f);
    pick.clear();
    std::fill(moving.begin(), moving.end(), BLACK);
    prepared.compose(actors, camera, 0, walls, moving, &animator.snapshot(), &pick);
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x)
            check(pick.slot(x, y) == PickingBuffer::empty && moving[y * viewWidth + x].r == 0,
                  "Wall depth clips traveling bodies, shadows and picking");
    const SceneRenderer renderer(testMaterials(), {appearance});
    FramePipeline pipeline;
    MovementState movement;
    movement.player = camera;
    animator.setTime(310);
    check(pipeline.update(renderer, map, movement, &actors, &animator.snapshot()), "First travel frame");
    animator.setTime(320);
    check(pipeline.update(renderer, map, movement, &actors, &animator.snapshot()),
          "Held pose still refreshes continuous travel");
    animator.setTime(600);
    pipeline.update(renderer, map, movement, &actors, &animator.snapshot());
    renderer.compose(map, camera, reference, &actors);
    check(same(pipeline.frame(), reference), "Arrival restores exact stationary prepared rendering");
}
// Explicit benchmark, excluded from routine CTest. All fixtures are generated.
void travelStress() {
    std::vector<std::string> rows(20, std::string(20, '.'));
    rows.front() = rows.back() = std::string(20, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    const Map crowdedMap(rows);
    std::vector<NpcInstance> placements;
    for (int y = 5; y < 13; ++y)
        for (int x = 2; x < 18; x += 2)
            for (int slot = 0; slot < 4; ++slot)
                placements.push_back({{0}, {x, y}, {slot % 2, slot / 2}, Heading::East});
    auto actors = testActors(crowdedMap, {9, 14}, {"test"}, placements);
    const auto appearance = testNpcAppearance();
    const PreparedNpcs prepared({appearance});
    NpcAnimator animator(actors, {npcTimings(appearance)});
    for (std::size_t i = 0; i < placements.size(); ++i) {
        const auto& npc = placements[i];
        check(actors.walk(actors.id(i), {npc.cell.x + 1, npc.cell.y}, npc.subcell, {9, 14}, 0).changed,
              "Every stress actor has a real legal travel segment");
    }
    Frame frame;
    PickingBuffer picking;
    std::array<float, viewWidth> walls;
    walls.fill(100);
    const auto start = std::chrono::steady_clock::now();
    for (std::uint64_t time = 0; time < 600; time += 5) {
        animator.setTime(time);
        picking.clear();
        prepared.compose(actors, {{9, 14}, Heading::North}, 0, walls, frame, &animator.snapshot(), &picking);
        picking.finish(&actors);
    }
    const auto ms =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    check(picking.targets().count > 0, "Stress actually presents visible bodies");
    std::cout << "256 traveling actors, 120 compositions with picking: " << ms << " ms total, " << ms / 120
              << " ms/frame (CPU only, generated solid art)\n";
}
void staggeredIdle() {
    const std::vector<NpcInstance> placements{{{0}, {3, 3}, {0, 0}, Heading::South},
                                              {{0}, {3, 3}, {1, 0}, Heading::South},
                                              {{0}, {3, 3}, {0, 1}, Heading::South},
                                              {{0}, {3, 3}, {1, 1}, Heading::South, NpcAnimation::Idle, 60},
                                              {{0}, {4, 3}, {0, 0}, Heading::South, NpcAnimation::Attack}};
    auto actors = testActors(map, {3, 5}, {"test"}, placements);
    auto rebuilt = testActors(map, {3, 5}, {"test"}, placements);
    const auto metadata = npcTimings(testAnimatedNpcAppearance());
    NpcAnimator animator(actors, {metadata}), repeat(rebuilt, {metadata});
    std::array<int, 3> firstChange{{-1, -1, -1}};
    const auto initial = animator.snapshot().frames;
    for (std::uint64_t time = 0; time <= 120; ++time) {
        animator.setTime(time);
        repeat.setTime(time);
        for (std::size_t i = 0; i < placements.size(); ++i) {
            const auto& frame = animator.snapshot().frames[i];
            const auto& again = repeat.snapshot().frames[i];
            check(frame.active == again.active && frame.idle == again.idle,
                  "Reconstructed worlds have deterministic phases despite different world IDs");
            if (i < 3 && frame.active != initial[i].active && firstChange[i] < 0)
                firstChange[i] = static_cast<int>(time);
            if (i < 4)
                check(frame.active == frame.idle, "Idle playback and directional fallback share phase");
        }
        check(animator.snapshot().frames[3].active == ((time + 60) % 120 >= 60),
              "Explicit idle phase overrides automatic staggering");
        const auto attack = time < 70 ? 0 : time < 95 ? 1 : time < 120 ? 2 : 3;
        check(animator.snapshot().frames[4].active == attack,
              "Automatic idle staggering does not alter authored attack timing");
    }
    check(firstChange[0] > 0 && firstChange[1] > 0 && firstChange[2] > 0 &&
              firstChange[0] != firstChange[1] && firstChange[0] != firstChange[2] &&
              firstChange[1] != firstChange[2],
          "Default idle actors transition at different times within one cycle");
    check(bool(actors.remove(actors.id(0))), "Remove earlier slot");
    check(bool(actors.move(actors.id(1), {2, 3}, {0, 0}, {3, 5})), "Move phased actor");
    for (std::uint64_t time = 0; time < 120; ++time) {
        animator.setTime(time);
        repeat.setTime(time);
        for (std::size_t i = 1; i < placements.size(); ++i)
            check(animator.snapshot().frames[i].idle == repeat.snapshot().frames[i].idle,
                  "Movement and removal preserve surviving idle phases, including attack fallback");
    }
    animator.setTime(53);
    const auto frozen = animator.snapshot().frames[1];
    check(actors.damage(actors.id(1), 7, 53).died, "Lethal damage fixture");
    for (std::uint64_t time : {53u, 100u, 152u}) {
        animator.setTime(time);
        const auto& frame = animator.snapshot().frames[1];
        check(frame.visible && frame.flash && frame.active == frozen.active && frame.idle == frozen.idle,
              "Death flash holds the staggered pose from hit time");
    }
    animator.setTime(153);
    check(!animator.snapshot().frames[1].visible && !animator.snapshot().frames[1].flash,
          "Idle phase does not shift the 100 ms death flash lifetime");
}
void triggeredAttacks() {
    const auto appearance = testAnimatedNpcAppearance();
    auto actors = world(NpcAnimation::Idle);
    const auto idleActors = world(NpcAnimation::Idle);
    NpcAnimator animator(actors, {npcTimings(appearance)}), idle(idleActors, {npcTimings(appearance)});
    const SceneRenderer renderer(testMaterials(), {appearance});
    Frame attackFrame, idleFrame;
    const auto id = actors.id(0);
    const auto hp = actors.actor(id)->hitPoints;
    check(actors.attack(id, 1000).changed, "Attack cue independent of damage");
    for (const auto elapsed : {0u, 69u, 70u, 94u, 95u, 119u, 120u, 154u, 155u, 310u}) {
        animator.setTime(1000 + elapsed);
        const auto& frame = animator.snapshot().frames[0];
        check(frame.clip == (elapsed < 155 ? NpcAnimation::Attack : NpcAnimation::Idle),
              "One-shot attack plays to completion then returns to idle without looping");
        if (elapsed < 155)
            check(frame.active == (elapsed < 70    ? 0
                                   : elapsed < 95  ? 1
                                   : elapsed < 120 ? 2
                                                   : 3),
                  "Triggered clip uses relative time without placement phase");
    }
    animator.setTime(1070);
    idle.setTime(1070);
    renderer.compose(map, {{3, 5}, Heading::North}, attackFrame, &actors, &animator.snapshot());
    renderer.compose(map, {{3, 5}, Heading::North}, idleFrame, &idleActors, &idle.snapshot());
    check(!same(attackFrame, idleFrame) && actors.actor(id)->hitPoints == hp,
          "Miss animation changes actual prepared pixels without health mutation");
    renderer.compose(map, {{5, 3}, Heading::West}, attackFrame, &actors, &animator.snapshot());
    renderer.compose(map, {{5, 3}, Heading::West}, idleFrame, &idleActors, &idle.snapshot());
    check(same(attackFrame, idleFrame), "Missing attack direction uses current idle frame");
    const auto revision = actors.snapshot().revision;
    check(actors.attack(id, 999).error == ActorError::InvalidTime &&
              actors.attack(idleActors.id(0), 1100).error == ActorError::InvalidHandle &&
              actors.snapshot().revision == revision,
          "Invalid cues leave actor state unchanged");
    check(actors.attack(id, 1100).changed, "A new attack restarts the clip");
    animator.setTime(1100);
    check(animator.snapshot().frames[0].active == 0, "Retrigger starts on frame zero");
    actors.damage(id, hp, 1170);
    animator.setTime(1190);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Attack &&
              animator.snapshot().frames[0].active == 1 && animator.snapshot().frames[0].flash,
          "Death freezes triggered attack pose at the lethal timestamp");
    check(actors.attack(id, 1200).error == ActorError::NotAlive, "Dead actor cannot start an attack");
    animator.setTime(1270);
    check(!animator.snapshot().frames[0].visible, "Death expiry still hides attacking body");
    auto noArt = world(NpcAnimation::Idle);
    NpcAnimator fallback(noArt, {npcTimings(testNpcAppearance())});
    noArt.attack(noArt.id(0), 0);
    fallback.setTime(10);
    check(fallback.snapshot().frames[0].clip == NpcAnimation::Idle,
          "Missing entire attack clip safely retains idle");
}
void sideContactAttacks() {
    const auto room = hybridRoom();
    const auto catalog = hybridCatalog(false);
    const auto appearance = testAnimatedNpcAppearance();
    const auto timings = npcTimings(appearance);
    const SceneRenderer renderer(testMaterials(), {appearance});
    Frame attack, idle;
    for (int heading = 0; heading < 4; ++heading) {
        const Player contact{{6, 6}, static_cast<Heading>(heading)};
        const auto f = forward(contact.heading), r = right(contact.heading);
        const Player start{contact.cell + f * -1, contact.heading};
        for (int side : {-1, 0, 1}) {
            for (int slot = 0; slot < 4; ++slot) {
                NpcInstance npc{{0}, contact.cell + f + r * side, {slot % 2, slot / 2},
                                static_cast<Heading>((heading + 1) % 4)};
                npc.monster = catalog.findMonster("swordsman");
                NpcWorld placements(room, start.cell, {"goblin"}, {npc});
                GameSession session(room, catalog, placements, 5, start, false, {timings});
                session.setAnimated(false);
                session.move(Action::Forward);
                check(session.combat().snapshot().phase == CombatPhase::Contact,
                      "Side approach enters contact");
                NpcAnimator animator(session.actors(), {timings});
                bool attacked = false;
                for (int step = 0; step < 400 && !attacked; ++step) {
                    if (session.combat().snapshot().awaitingAction) {
                        session.submit(turnCommand(session));
                        continue;
                    }
                    const auto event = session.update(.05, false).step;
                    if (event.event != CombatEvent::AttackStarted)
                        continue;
                    check(event.enemyAttack && event.kind == rules::AttackKind::Melee,
                          "Contact enemy starts melee attack");
                    animator.setTime(session.timeMs());
                    const auto& pose = animator.snapshot();
                    check(pose.frames[0].clip == NpcAnimation::Attack,
                          "Accepted melee attack selects its animation");
                    renderer.compose(room, session.movement().player, attack, &session.actors(), &pose);
                    auto idlePose = pose;
                    idlePose.frames[0].clip = NpcAnimation::Idle;
                    idlePose.frames[0].active = idlePose.frames[0].idle;
                    renderer.compose(room, session.movement().player, idle, &session.actors(), &idlePose);
                    check(!same(attack, idle),
                          "Every contact heading/cell/subcell renders attack pixels, not side idle fallback");
                    attacked = true;
                }
                check(attacked, "Side-approached enemy takes its attack turn");
            }
        }
    }
}
void rendering() {
    const auto appearance = testAnimatedNpcAppearance();
    const auto npcs = world(NpcAnimation::Attack);
    const auto idleWorld = world(NpcAnimation::Idle);
    NpcAnimator animator(npcs, {npcTimings(appearance)}), idle(idleWorld, {npcTimings(appearance)});
    const SceneRenderer renderer(testMaterials(), {appearance});
    Frame a, b, source, destination;
    const std::array<Player, 4> cameras{{{{3, 5}, Heading::North},
                                         {{3, 1}, Heading::South},
                                         {{5, 3}, Heading::West},
                                         {{1, 3}, Heading::East}}};
    animator.setTime(70);
    idle.setTime(70);
    for (std::size_t side = 0; side < cameras.size(); ++side) {
        renderer.compose(map, cameras[side], a, &npcs, &animator.snapshot());
        renderer.compose(map, cameras[side], b, &idleWorld, &idle.snapshot());
        check(same(a, b) == (side != 0),
              "Front attack differs; absent directions exactly match current idle");
    }
    MovementState state;
    state.player = cameras[0];
    FramePipeline pipeline;
    animator.setTime(0);
    check(pipeline.update(renderer, map, state, &npcs, &animator.snapshot()),
          "First animation frame rendered");
    a.copyFrom(pipeline.frame());
    animator.setTime(59);
    check(!pipeline.update(renderer, map, state, &npcs, &animator.snapshot()),
          "Stationary held frame reused");
    animator.setTime(70);
    check(pipeline.update(renderer, map, state, &npcs, &animator.snapshot()) && !same(a, pipeline.frame()),
          "Stationary player sees NPC frame changes");
    // Every movement effect must use one shared time for both endpoints, including
    // ticks during an already active transition and animation-mode switching.
    for (int action = 0; action < 6; ++action) {
        const auto kind = static_cast<Action>(action);
        Player from{{3, 4}, Heading::North}, to = from;
        check(tryAction(map, to, kind), "Fixture transition possible");
        state.player = to;
        state.revision++;
        for (std::uint64_t time : {0u, 70u, 95u, 120u, 155u}) {
            state.transition = {from, to, kind, Transition::duration * 0.5f};
            animator.setTime(time);
            pipeline.update(renderer, map, state, &npcs, &animator.snapshot());
            if (isStrafe(kind))
                renderer.composeStrafe(map, from, to, 0.5f, b, &npcs, &animator.snapshot());
            else {
                renderer.compose(map, from, source, &npcs, &animator.snapshot());
                renderer.compose(map, to, destination, &npcs, &animator.snapshot());
                composeTransition(source, destination, kind, 0.5f, b);
            }
            check(same(pipeline.frame(), b), "Transition endpoints share current NPC snapshot");
        }
        state.transition.elapsed = Transition::duration;
        pipeline.update(renderer, map, state, &npcs, &animator.snapshot());
        renderer.compose(map, to, b, &npcs, &animator.snapshot());
        check(same(pipeline.frame(), b), "Completion does not restore stale animation frame");
    }
    state.player = cameras[0];
    state.transition.elapsed = Transition::duration;
    ++state.revision;
    animator.setTime(70);
    pipeline.update(renderer, map, state, &npcs, &animator.snapshot());
    check(pipeline.update(renderer, map, state, &npcs), "Removing snapshot invalidates held frame");
    renderer.compose(map, state.player, b, &npcs);
    check(same(pipeline.frame(), b), "Null snapshot displays first selected clip frame");
    check(pipeline.update(renderer, map, state, &npcs, &animator.snapshot()),
          "Restoring snapshot invalidates held frame");
    auto oversized = appearance;
    oversized.width = 0.5f;
    oversized.height = 1;
    rejects([&] { PreparedNpcs bad({oversized}, {1024, 512u * 1024u * 1024u}); },
            "All animation frames count toward prepared pixel budget");
    auto invalid = animator.snapshot();
    invalid.frames[0].active = 4;
    rejects([&] { renderer.compose(map, cameras[0], a, &npcs, &invalid); },
            "Invalid frame selection rejected");
    invalid = animator.snapshot();
    invalid.count = 0;
    rejects([&] { renderer.compose(map, cameras[0], a, &npcs, &invalid); },
            "Snapshot count mismatch rejected");
    auto malformed = appearance;
    malformed.animations[1].views[2].clear();
    rejects([&] { PreparedNpcs bad({malformed}); }, "Walk requires all directions");
    malformed = appearance;
    malformed.animations[0].views[0][1].anchorY = 0.5f;
    rejects([&] { PreparedNpcs bad({malformed}); }, "Every frame respects ground anchor");
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--stress") {
            travelStress();
            return 0;
        }
        timing();
        actualWalking();
        travelTiming();
        travelRendering();
        staggeredIdle();
        triggeredAttacks();
        sideContactAttacks();
        rendering();
        std::cout << "NPC animation checks passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
