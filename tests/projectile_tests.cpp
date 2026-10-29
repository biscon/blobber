#include "render/frame_pipeline.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <iostream>
using namespace blobber;
namespace {
void composition() {
    const Map map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                   "#.......#", "#########"});
    const Player player{{4, 6}, Heading::North};
    auto appearance = testArcherAppearance();
    const SceneRenderer renderer(testMaterials(), {appearance});
    auto actors = testActors(map, player.cell, {"test"}, {{{0}, {4, 3}, {1, 1}, Heading::South}});
    NpcAnimator animator(actors, {npcTimings(appearance)});
    FramePipeline plain, effects;
    MovementState state;
    state.player = player;
    AttackVisualSnapshot shots;
    shots.world = actors.snapshot().world;
    shots.revision = 1;
    auto& shot = shots.attacks[0];
    shot.id = {shots.world, 1};
    shot.stage = AttackStage::Flight;
    shot.enemy = actors.id(0);
    shot.ranged = true;
    shot.hit = true;
    shot.releaseMs = 0;
    shot.impactMs = 300;
    shot.endMs = 1300;
    shot.npcOrigin = actors.snapshot().actors[0].npc;
    shot.targetPosition = actorPosition(actors.snapshot().actors[0], 0);
    shot.sourcePosition = shot.targetPosition;
    shot.partyOrigin = player;
    shot.formationSlot = 2;
    for (bool incoming : {false, true}) {
        shot.incoming = incoming;
        ++shots.revision;
        animator.setTime(150);
        plain.update(renderer, map, state, &actors, &animator.snapshot());
        check(effects.update(renderer, map, state, &actors, &animator.snapshot(), &shots, 150),
              "Flight invalidates frame");
        check(!same(plain.frame(), effects.frame()), "Both directions visibly compose an arrow");
        check(!effects.update(renderer, map, state, &actors, &animator.snapshot(), &shots, 150),
              "Paused shot retains cached frame");
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x)
                check(plain.picking()->slot(x, y) == effects.picking()->slot(x, y),
                      "Arrow never alters body picking");
        effects.update(renderer, map, state, &actors, &animator.snapshot(), &shots, 160);
        shot.stage = AttackStage::Resolved;
        ++shots.revision;
        animator.setTime(300);
        effects.update(renderer, map, state, &actors, &animator.snapshot(), &shots, 300);
        plain.update(renderer, map, state, &actors, &animator.snapshot());
        check(!same(plain.frame(), effects.frame()), "Impact glint visible at arrival");
        effects.update(renderer, map, state, &actors, &animator.snapshot(), &shots, 420);
        check(same(plain.frame(), effects.frame()), "Expired cosmetics restore clean frame");
        shot.stage = AttackStage::Flight;
    }
    // Transition endpoints and strafe phases continue to draw, without exposing picking.
    for (const auto action :
         {Action::Left, Action::Right, Action::Forward, Action::Back, Action::TurnLeft, Action::TurnRight}) {
        Controller controller(player);
        controller.submit(map, action, true, &actors);
        controller.update(map, .08f, &actors);
        ++shots.revision;
        animator.setTime(150);
        effects.update(renderer, map, controller.state(), &actors, &animator.snapshot(), &shots, 150);
        check(!effects.picking(), "Transition never exposes targeting through effects");
    }
    // Independent wall-depth test suppresses all projectile pixels.
    PreparedNpcs bank({appearance});
    ProjectilePlayer presentation;
    presentation.update(&shots, 150);
    Frame before, after;
    renderer.compose(map, player, before, &actors, &animator.snapshot());
    after.copyFrom(before);
    std::array<float, viewWidth> wall;
    wall.fill(0);
    composeProjectiles(presentation, bank, actors, player, 0, wall, nullptr, after);
    check(same(before, after), "Wall depth occludes projectiles");
    shot.enemy.world++; // Visuals have no ability to mutate actors, even with foreign references.
    const auto hp = actors.snapshot().actors[0].hitPoints;
    presentation.update(&shots, 180);
    check(actors.snapshot().actors[0].hitPoints == hp, "Cosmetic processing never changes health");
}
void groundLayers() {
    const Map map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                   "#.......#", "#########"});
    const Player player{{4, 6}, Heading::North};
    const SceneRenderer renderer(testGreaseMaterials());
    SpellArea area;
    area.ground.revision = 1;
    area.ground.areas[0] = {{1, 1}, {4, 5}, {3}, 12, 60000, true};
    Frame plain, painted;
    for (int phase = 0; phase <= 8; ++phase) {
        const Player to{{5, 6}, Heading::North};
        renderer.composeStrafe(map, player, to, phase / 8.f, plain);
        renderer.composeStrafe(map, player, to, phase / 8.f, painted, nullptr, nullptr, nullptr, nullptr,
                               &area);
        check(!same(plain, painted), "Persistent ground visible throughout prepared strafe phases");
    }
    // One patch is composited once even where multiple gameplay areas overlap.
    renderer.compose(map, player, painted, nullptr, nullptr, nullptr, nullptr, &area);
    area.ground.areas[1] = area.ground.areas[0];
    area.ground.areas[1].id.serial = 2;
    Frame overlap;
    renderer.compose(map, player, overlap, nullptr, nullptr, nullptr, nullptr, &area);
    check(same(painted, overlap), "Overlapping grease patches do not multiply opacity");
    area.ground.areas[1].active = false;
    area.cells[0] = area.ground.areas[0].cell;
    area.eligible[0] = true;
    renderer.compose(map, player, overlap, nullptr, nullptr, nullptr, nullptr, &area);
    check(!same(painted, overlap), "Target preview is a separate layer above persistent art");
    area.eligible[0] = false;
    for (unsigned heading = 0; heading < 4; ++heading) {
        const Player around{Cell{4, 4} + forward(Heading(heading)) * -2, Heading(heading)};
        area.ground.areas[0].cell = {4, 4};
        renderer.compose(map, around, plain);
        renderer.compose(map, around, painted, nullptr, nullptr, nullptr, nullptr, &area);
        unsigned clear = 0, colored = 0;
        for (int y = int(ViewProjection::horizon) + 1; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                const auto hit = SceneRenderer::groundCell(map, around, x, y);
                if (hit && *hit == Cell{4, 4}) {
                    const auto a = plain[y * viewWidth + x], b = painted[y * viewWidth + x];
                    if (a.r == b.r && a.g == b.g && a.b == b.b)
                        ++clear;
                    else
                        ++colored;
                }
            }
        check(clear && colored, "Every heading preserves transparent pool edges and painted interior");
    }
    area.ground.areas[0].cell = {4, 5};
    MovementState state;
    state.player = player;
    FramePipeline frames;
    check(frames.update(renderer, map, state, nullptr, nullptr, nullptr, 0, &area),
          "Persistent ground first frame");
    check(!frames.update(renderer, map, state, nullptr, nullptr, nullptr, 0, &area),
          "Unchanged ground reuses frame");
    area.ground.areas[0].active = false;
    ++area.ground.revision;
    check(frames.update(renderer, map, state, nullptr, nullptr, nullptr, 0, &area),
          "Ground expiry invalidates cached frame");
    renderer.compose(map, player, plain);
    check(same(plain, frames.frame()), "Expiry restores clean floor");
    const Map wall({"#########", "#.......#", "#.......#", "#.......#", "#..###..#", "#.......#", "#.......#",
                    "#.......#", "#########"});
    area.ground.areas[0].active = true;
    area.ground.areas[0].cell = {4, 3};
    renderer.compose(wall, player, plain);
    renderer.compose(wall, player, painted, nullptr, nullptr, nullptr, nullptr, &area);
    check(same(plain, painted), "Walls occlude persistent ground");
    check(!SceneRenderer::groundCell(map, player, 250, 30), "Ground picking excludes ceiling");
    unsigned hits = 0;
    for (int y = 100; y < viewHeight; y += 7)
        for (int x = 0; x < viewWidth; x += 11) {
            const auto cell = SceneRenderer::groundCell(wall, player, x, y);
            if (cell) {
                ++hits;
                check(spellCell(wall, player, *cell), "Ground pick shares authoritative LOS/range query");
            }
        }
    check(hits > 0, "Visible floor provides ground targets");
}
void capacityAndAnchors() {
    AttackVisualSnapshot shots;
    shots.world = 1;
    shots.revision = 1;
    for (std::size_t i = 0; i < attackCapacity; ++i) {
        auto& v = shots.attacks[i];
        v.id = {1, i + 1};
        v.ranged = true;
        v.stage = AttackStage::Flight;
        v.impactMs = 400;
    }
    ProjectilePlayer full, limited(2), none(0);
    full.update(&shots, 100);
    limited.update(&shots, 100);
    none.update(&shots, 100);
    check(full.size() == attackCapacity && limited.size() == 2 && none.size() == 0,
          "Cosmetic capacity is bounded independently");
    for (const auto& v : limited)
        check(v.id.serial >= attackCapacity - 1, "Exhaustion deterministically retains newest shots");
    check(shots.attacks[0].stage == AttackStage::Flight, "Dropping visuals cannot cancel pending attacks");
    full.update(nullptr, 100);
    check(!full.size(), "Restart clears visual owner");
    auto appearance = testArcherAppearance();
    PreparedNpcs bank({appearance});
    const Player p{{4, 4}, Heading::North};
    for (int side = -1; side <= 1; ++side)
        for (int slot = 0; slot < 4; ++slot) {
            NpcInstance npc{{0}, {4 + side, 3}, {slot % 2, slot / 2}, Heading::South};
            const ActorPosition position{npc.cell.x + .25f + npc.subcell.x * .5f,
                                         npc.cell.y + .25f + npc.subcell.y * .5f};
            const auto torso = bank.attachment(npc, position, false, p, 0);
            check(torso.valid && torso.x >= 0 && torso.x < viewWidth,
                  "Diagonal attachment uses inward body placement");
        }
    check(projectilePreparedBytes() < 1024 * 1024, "Bounded immutable raster bank");
}
} // namespace
int main() {
    try {
        composition();
        groundLayers();
        capacityAndAnchors();
        std::cout << "Projectile checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
