#pragma once
#include "support/allocations.hpp"
#include "support/creation_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <cmath>
namespace blobber::test {
inline void movingVisibilityWork() {
    VisibilityFixture fixture;
    world_assets::CpuAssets assets(fixture.publication());
    const auto catalog = creationCatalog(true);
    const auto map = worldRoom(9, 9);
    const Player start{{4, 6}, Heading::North}, end{{4, 5}, Heading::North};
    NpcWorld places(map, start.cell, {"goblin"}, {{{0}, {4, 3}, {0, 0}, Heading::South}});
    ActorWorld actors(map, catalog, places);
    NpcAnimator animator(actors, fixture.timings);
    animator.setTime(100);
    auto scene = world_render::prepare(map);
    auto fast = std::make_unique<world_render::VisibilityPipeline>(scene, assets);
    auto reference = std::make_unique<world_render::VisibilityPipeline>(scene, assets);
    MovementState movement;
    movement.player = start;
    auto input = [&] { return world_render::FrameInput{map, movement, &actors, &animator.snapshot()}; };
    fast->update(input(), false);
    check(fast->picking() && fast->frame().rasterized, "Settled preparation produces interaction");
    movement.transition = {start, end, Action::Forward, .08f, .16f};
    allocations = 0;
    watching = true;
    fast->update(input(), false);
    watching = false;
    check(allocations == 0 && !fast->picking() && !fast->frame().rasterized && fast->frame().count == 1,
          "Moving presentation updates without allocations or a software picking raster");
    const auto revision = fast->frame().revision;
    fast->rasterForDiagnostics();
    reference->update(input());
    check(fast->frame().rasterized && !fast->picking() && fast->frame().revision == revision,
          "On-demand diagnostics neither publish moving interaction nor change image identity");
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x)
            check(fast->frame().picking.slot(x, y) == reference->frame().picking.slot(x, y),
                  "Requested moving raster agrees with the full diagnostic pipeline");
    movement.transition.elapsed = std::nextafter(.16f, 0.f);
    fast->update(input(), false);
    const auto finalCamera = fast->frame().camera;
    check(!fast->frame().rasterized, "Later movement invalidates the diagnostic raster");
    movement.player = end;
    movement.transition = {};
    fast->update(input(), false);
    reference->update(input());
    check(world_render::sameCamera(finalCamera, fast->frame().camera),
          "Fixture exercises settling at an already reached camera pose");
    check(fast->picking() && fast->frame().rasterized, "Settling restores current interaction");
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x) {
            const auto p = std::size_t(y) * viewWidth + x;
            check(fast->picking()->slot(x, y) == reference->picking()->slot(x, y) &&
                      fast->picking()->opaqueDepth(p) == reference->picking()->opaqueDepth(p),
                  "Settling rebuilds stale scenery even when the final camera pose is unchanged");
        }
    std::array<std::uint8_t, NpcWorld::maxInstances> marks{};
    const auto empty = fast->picking()->markedBounds(marks);
    check(empty.left == empty.right && empty.top == empty.bottom,
          "An unhighlighted HUD has no outline pixels to scan");
    marks[0] = 3;
    const auto bounds = fast->picking()->markedBounds(marks);
    check(bounds.left < bounds.right && bounds.top < bounds.bottom &&
              (bounds.right - bounds.left) * (bounds.bottom - bounds.top) < viewWidth * viewHeight / 2,
          "A highlighted small creature bounds its outline work to visible pixels");
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x)
            if (fast->picking()->slot(x, y) == 0)
                check(x >= bounds.left && x < bounds.right && y >= bounds.top && y < bounds.bottom,
                      "Cropped outline bounds retain every visible silhouette pixel");
    check(!fast->update(input(), false), "Unchanged settled view reuses its raster");
    FloorFixtures fixtures;
    fixtures.count = 1;
    fixtures.entries[0] = {{4, 4}, Heading::North, FixtureKind::Door};
    FloorGeometry geometry(map, fixtures);
    DoorSnapshot doors;
    doors.world = geometry.world();
    doors.until[0] = 300;
    doors.now = 100;
    fast->update({geometry.view(), movement, &actors, &animator.snapshot(), nullptr, &doors}, false);
    check(!fast->picking() && !fast->frame().rasterized, "Moving doors also skip unavailable interaction");
    doors.now = 300;
    ++doors.revision;
    fast->update({geometry.view(), movement, &actors, &animator.snapshot(), nullptr, &doors}, false);
    check(fast->picking() && fast->frame().rasterized, "Settled doors restore current picking");
}
} // namespace blobber::test
