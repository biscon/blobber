#include "render/melee_swipe.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/world_fixtures.hpp"
#include <cmath>
#include <iostream>
using namespace blobber;
using namespace blobber::world_render;
namespace {
float distance(SwipePoint a, SwipePoint b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}
void geometry() {
    const SwipePoint center{250, 140};
    const auto full = sampleSwipe(center, 100, 200, false, 0, 100);
    check(full.alpha == 1 && distance(full.vertices.front(), full.vertices[swipeColumns - 1]) == 0 &&
              distance(full.vertices[swipeSegments * swipeColumns],
                       full.vertices[swipeSegments * swipeColumns + swipeColumns - 1]) == 0,
          "Both endpoints are pointed");
    float thickest = 0;
    unsigned peak = 0;
    for (unsigned i = 0; i <= swipeSegments; ++i) {
        const auto width =
            distance(full.vertices[swipeColumns * i], full.vertices[swipeColumns * i + swipeColumns - 1]);
        if (width > thickest) {
            thickest = width;
            peak = i;
        }
        check(std::isfinite(width), "Finite strip geometry");
    }
    check(peak == 13 && std::abs(thickest - 17) < .02f, "Thickness peaks behind leading tip");
    check(swipeEdgeAlpha.front() == 0 && swipeEdgeAlpha.back() == 0 && swipeEdgeAlpha[1] > 0 &&
              swipeEdgeAlpha[1] < swipeEdgeAlpha[2] && swipeEdgeAlpha[2] == 1 && swipeEdgeAlpha[3] == 1,
          "Solid core fades through translucent bands to transparent outer edges");
    const auto early = sampleSwipe(center, 100, 200, false, 0, 34);
    const auto late = sampleSwipe(center, 100, 200, false, 0, 190);
    check(distance(early.vertices[0], full.vertices[0]) == 0 &&
              distance(early.vertices[swipeSegments * swipeColumns],
                       full.vertices[swipeSegments * swipeColumns]) > 20,
          "Leading tip advances during reveal");
    check(distance(late.vertices[0], full.vertices[0]) > 20 &&
              distance(late.vertices[swipeSegments * swipeColumns],
                       full.vertices[swipeSegments * swipeColumns]) < .001f &&
              late.alpha < 1,
          "Trailing end catches up while fading");
    for (unsigned age : {70u, 100u, 130u})
        check(sampleSwipe(center, 100, 200, false, 0, age).alpha == 1, "Peak interval remains opaque");
    check(sampleSwipe(center, 100, 200, false, 0, 0).alpha == 0 &&
              sampleSwipe(center, 100, 200, false, 0, 240).alpha == 0,
          "Invisible endpoints in time");
    const auto mirror = sampleSwipe(center, 100, 200, true, 0, 100);
    for (unsigned i = 0; i < full.vertices.size(); ++i)
        check(std::abs(full.vertices[i].x + mirror.vertices[i].x - 500) < .001f &&
                  full.vertices[i].y == mirror.vertices[i].y,
              "Mirror preserves shape");
}
void lifecycle() {
    const auto map = combatMap();
    const auto catalog = combatCatalog();
    const Player pose{{3, 3}, Heading::North};
    NpcWorld places(map, pose.cell, {"goblin"}, {{{0}, {3, 2}, {0, 1}, Heading::South}});
    GameSession game(map, catalog, places, 5, pose, false);
    WorldFrame world;
    world.camera = camera(game.movement(), map);
    world.count = 1;
    auto& body = world.billboards[0];
    body.id = 0;
    body.alive = true;
    body.feet = {3.25f, 0, 2.75f};
    body.width = .45f;
    body.height = .65f;
    body.depth = project(world.camera, body.feet).depth;
    body.bodyBounds = {.2f, .1f, .8f, .95f};
    AttackVisualSnapshot attacks;
    attacks.world = game.actors().snapshot().world;
    attacks.revision = 1;
    auto& attack = attacks.attacks[NpcWorld::maxInstances];
    attack.id = {attacks.world, 1};
    attack.enemy = game.actors().id(0);
    attack.character = {0};
    attack.stage = AttackStage::Windup;
    attack.hit = true;
    attack.impactMs = 200;
    FrameInput in{map, game.movement(), &game.actors(), nullptr, nullptr, nullptr, &attacks};
    MeleeSwipePlayer player;
    allocations = 0;
    watching = true;
    player.update(in, world);
    in.now = 100;
    player.update(in, world);
    const bool initial = player.frame().count == 0;
    in.now = 185;
    player.update(in, world);
    const auto peak = player.frame().ribbons[0];
    const bool visible = player.frame().count == 1;
    const bool paused = !player.update(in, world);
    body.feet.x += .5f;
    body.width *= 2; // Recoil and canvas changes cannot move the captured swipe.
    body.alive = false;
    world.count = 0; // Lethal reaction may remove the target billboard entirely.
    attack.stage = AttackStage::Resolved;
    ++attacks.revision;
    in.now = 200;
    player.update(in, world);
    const auto contact = player.frame().ribbons[0];
    in.now = 340;
    const bool expired = player.update(in, world) && !player.frame().count;
    const bool stable = !player.update(in, world);
    watching = false;
    check(allocations == 0, "Spawn, animate, pause, resolve and expire allocate nothing");
    check(initial && visible && paused && expired && stable, "Timeline and cache invalidation");
    check(distance(peak.vertices[0], contact.vertices[0]) == 0, "Contact remains fixed through recoil");
    // A missed deadline never replays an expired stroke.
    in.now = 1000;
    world.count = 1;
    body.alive = true;
    player.update(in, world);
    check(!player.frame().count, "Skipped frames do not replay effects");
    attack.id.serial = 2;
    attack.impactMs = 1200;
    attack.stage = AttackStage::Windup;
    attack.hit = false;
    ++attacks.revision;
    player.update(in, world);
    in.now = 1185;
    player.update(in, world);
    check(player.frame().count == 1, "Miss retains a moving stroke");
    const auto miss = player.frame().ribbons[0];
    const auto torso = project(world.camera, attachment(world.camera, body, body.torso).position);
    check(miss.vertices[0].x < torso.x && miss.vertices[swipeSegments * swipeColumns].x < torso.x,
          "Mirrored miss passes to one side of target");
    attack.stage = AttackStage::Canceled;
    ++attacks.revision;
    check(player.update(in, world) && !player.frame().count, "Cancellation removes stroke");
    attack.stage = AttackStage::Windup;
    ++attacks.revision;
    player.update(in, world);
    check(!player.frame().count, "Canceled identity cannot respawn");
    attack.id.serial = 3;
    attack.impactMs = 1400;
    ++attacks.revision;
    body.alive = false;
    in.now = 1200;
    player.update(in, world);
    body.alive = true;
    in.now = 1385;
    player.update(in, world);
    check(!player.frame().count, "Missing initial geometry never retries against later art");
    in.attacks = nullptr;
    player.update(in, world);
    check(!player.frame().count, "Teardown clears records");
    // Registered body extent ignores a wider attack canvas and weapon/padding envelope.
    in.attacks = &attacks;
    attack.id.serial = 4;
    attack.impactMs = 1600;
    attack.hit = true;
    ++attacks.revision;
    in.now = 1400;
    body.bodyCanvasWidth = .45f;
    MeleeSwipePlayer narrow, padded;
    body.width = .45f;
    narrow.update(in, world);
    body.width = .9f;
    padded.update(in, world);
    in.now = 1585;
    narrow.update(in, world);
    padded.update(in, world);
    check(distance(narrow.frame().ribbons[0].vertices[0], padded.frame().ribbons[0].vertices[0]) == 0,
          "Weapon and transparent canvas enlargement cannot inflate body-based swipe");
    MeleeSwipePlayer late;
    in.now = 1600;
    late.update(in, world);
    check(!late.frame().count, "First observation after impact cannot invent acceptance geometry");
    in.now = 1800;
    for (unsigned i = 0; i < rules::partySize; ++i) {
        auto copy = attack;
        copy.id.serial = 5 + i;
        copy.character = {static_cast<std::uint16_t>(i)};
        copy.impactMs = 2000;
        attacks.attacks[NpcWorld::maxInstances + i] = copy;
    }
    ++attacks.revision;
    allocations = 0;
    watching = true;
    player.update(in, world);
    in.now = 1985;
    player.update(in, world);
    const bool full = player.frame().count == rules::partySize;
    watching = false;
    check(full && allocations == 0, "All four party records fit without allocation or eviction");
    for (unsigned i = 0; i < rules::partySize; ++i) {
        auto& a = attacks.attacks[NpcWorld::maxInstances + i];
        if (i == 0)
            a.ranged = true;
        if (i == 1)
            a.incoming = true;
        if (i == 2)
            a.spell = rules::SpellKind::InflictWounds;
        if (i == 3)
            a.stage = AttackStage::Empty;
    }
    ++attacks.revision;
    player.update(in, world);
    check(!player.frame().count, "Ranged, incoming, spells and empty records have no melee swipe");
}
} // namespace
int main() {
    try {
        test::WorldFixture fixture;
        const auto publication = world_assets::load(fixture.directory);
        check(publication.appearances[0].attachments.bodyBounds[2].left == .2f,
              "Body-only bounds survive prepared publication roundtrip");
        geometry();
        lifecycle();
        std::cout << "Melee swipe geometry and lifecycle passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
