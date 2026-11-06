#pragma once
#include "support/creation_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <cmath>

namespace blobber::test {
inline void groundAnchoredLeaves() {
    using namespace world_render;
    auto frame = std::make_unique<WorldFrame>();
    frame->camera = {{6, .5f, 6}, {0, 0, -1}, {1, 0, 0}, 0};
    frame->doorCount = 1;
    auto& leaf = frame->doors[0];
    leaf.origin = {4, 0, 3.8f};
    leaf.x = {0, 0, 1};
    leaf.y = {0, 1, 0};
    leaf.z = {.1f, 0, 0};
    leaf.leaf = true;
    Billboard body;
    body.id = 0;
    body.width = .5f;
    body.height = 1;
    body.feet = {4.15f, 0, 4.2f};
    body.depth = 1.8f;
    LeafQueries query;
    auto hidden = [&](Vec point, float depth = 1.8f) {
        const auto& c = frame->camera.position;
        return query.hides(frame->camera,
                           {(point.x - c.x) / depth, (point.y - c.y) / depth, (point.z - c.z) / depth},
                           depth);
    };
    prepareLeafQueries(*frame, nullptr, query);
    check(hidden({4.05f, .3f, 4.2f}), "Physical leaf covers a sample inside its slab");
    prepareLeafQueries(*frame, &body, query);
    check(!hidden({4.05f, .3f, 4.2f}),
          "A camera-side anchor preserves the body across the old leaf-volume cut");
    // Before the card touches the free tip, its artwork already overlaps the
    // leaf on screen. The same ground-side relation must hold on both sides of
    // that contact; otherwise a large strip appears in a single animation tick.
    for (float z : {3.7f, 3.79f, 3.81f, 3.9f}) {
        body.feet.z = z;
        prepareLeafQueries(*frame, &body, query);
        check(!hidden({3.95f, .3f, z}, 6.f - z),
              "The camera-side silhouette does not pop at first contact with the finite leaf tip");
    }
    body.feet.z = 4.2f;
    BillboardClips clips;
    billboardClips(*frame, body, clips);
    check(clips.count == 0, "A moving leaf never deletes a billboard UV strip");
    body.width = .4f;
    body.crop = {.05f, .1f, .95f, 1};
    prepareLeafQueries(*frame, &body, query);
    check(!hidden({4.05f, .3f, 4.2f}), "Supported artwork/crop changes preserve anchor-side ordering");
    body.width = .5f;
    body.crop = {};
    body.feet.x = 3.95f;
    prepareLeafQueries(*frame, &body, query);
    check(hidden({4.05f, .3f, 4.2f}), "Opposite-side creature is behind the finite leaf");
    check(!hidden({4.05f, 2, 4.2f}) && !hidden({4.05f, .3f, 3.2f}),
          "Leaf ordering cannot hide pixels above the top or beyond the free end");
    body.feet.x = 4.05f;
    prepareLeafQueries(*frame, &body, query);
    check(hidden({4.05f, .3f, 4.2f}), "An anchor in the slab conservatively remains behind it");
    body.feet.x = 4.100005f;
    prepareLeafQueries(*frame, &body, query);
    check(hidden({4.05f, .3f, 4.2f}), "Slab-boundary epsilon resolves conservatively");
    body.feet.x = 4.15f;
    frame->camera.position.x = 4.05f;
    prepareLeafQueries(*frame, &body, query);
    check(!hidden({4.05f, .3f, 5.2f}, .8f) && hidden({4.05f, .3f, 4.2f}),
          "Camera in the extended slab retains geometric depth before and behind the finite leaf");
    frame->camera.position.x = 6;
    // A second leaf must still block a body classified in front of the first.
    body.feet.x = 4.15f;
    frame->doorCount = 2;
    frame->doors[1] = leaf;
    frame->doors[1].origin = {4.8f, 0, 4.8f};
    prepareLeafQueries(*frame, &body, query);
    check(hidden({4.05f, .3f, 4.2f}), "Being in front of one leaf cannot bypass another leaf");
    frame->doorCount = 1;
    // Rotate the entire construction: relation must not depend on world heading.
    for (unsigned turn = 0; turn < 4; ++turn) {
        prepareLeafQueries(*frame, &body, query);
        Vec sample = body.feet;
        sample.x -= frame->camera.right.x * .1f;
        sample.z -= frame->camera.right.z * .1f;
        sample.y = .3f;
        check(!hidden(sample), "Ground-side classification survives cardinal rotation");
        auto rotate = [](Vec p) { return Vec{-p.z, p.y, p.x}; };
        frame->camera.position = rotate(frame->camera.position);
        frame->camera.right = rotate(frame->camera.right);
        frame->camera.forward = rotate(frame->camera.forward);
        body.feet = rotate(body.feet);
        leaf.origin = rotate(leaf.origin);
        leaf.x = rotate(leaf.x);
        leaf.z = rotate(leaf.z);
    }
}
inline void creatureIntersectionMasks() {
    groundAnchoredLeaves();
    auto frame = std::make_unique<world_render::WorldFrame>();
    frame->camera.right = {1, 0, 0};
    world_render::Billboard body;
    body.id = 0;
    body.feet = {4.5f, 0, 4.95f};
    body.width = body.height = 1;
    frame->doorCount = 1;
    frame->doors[0] = {{4.4f, 0, 4.9f}, {.2f, 0, 0}, {0, .5f, 0}, {0, 0, .1f}};
    world_render::BillboardClips clips;
    world_render::billboardClips(*frame, body, clips);
    check(clips.contains(.5f, .75f) && !clips.contains(.5f, .25f) && !clips.contains(.2f, .75f),
          "A low static frame hides intersecting legs, preserving the head and both sides");
    // The same physical points must be masked after changing stored artwork crop.
    body.crop = {.25f, .125f, .75f, .875f};
    world_render::billboardClips(*frame, body, clips);
    check(clips.contains(.5f, 5.f / 6) && !clips.contains(.5f, 1.f / 6),
          "Static-frame masks preserve physical registration with an asymmetric stored crop");
    body.feet.y = .6f;
    world_render::billboardClips(*frame, body, clips);
    check(clips.count == 0, "Vertically disjoint door geometry leaves the whole body alone");
    body.id = PickingBuffer::fixtureBase;
    body.feet.y = 0;
    world_render::billboardClips(*frame, body, clips);
    check(clips.count == 0, "Body masks do not affect fixture or loot draws");

    std::vector<std::string> rows(9, std::string(9, '.'));
    rows.front() = rows.back() = std::string(9, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    WorldDefinition definition;
    definition.surfaces.resize(81);
    definition.surfaces[4 * 9 + 3] = {1, 5};
    Map raised(rows, definition);
    body.id = 0;
    body.feet = {4.25f, 0, 4.5f};
    body.crop = {};
    world_render::prepareTerrainClips(raised, frame->camera, body, frame->terrainClips[0]);
    frame->doorCount = 0;
    world_render::billboardClips(*frame, body, clips);
    check(clips.contains(.1f, .9f) && !clips.contains(.1f, .5f) && !clips.contains(.5f, .9f),
          "Raised neighboring floor clips only the intersecting lower body");
    body.feet.y = .5f;
    world_render::prepareTerrainClips(raised, frame->camera, body, frame->terrainClips[0]);
    world_render::billboardClips(*frame, body, clips);
    check(clips.contains(.1f, .1f) && !clips.contains(.1f, .5f),
          "Neighboring ceiling clips the head without removing the torso");
    definition.surfaces[4 * 9 + 3].ceiling = noCeiling;
    Map outdoor(rows, definition);
    world_render::prepareTerrainClips(outdoor, frame->camera, body, frame->terrainClips[0]);
    world_render::billboardClips(*frame, body, clips);
    check(!clips.contains(.1f, .1f), "An outdoor cell has no implicit ceiling mask");
}

inline void creatureMaskExposure() {
    VisibilityFixture fixture;
    auto publication = fixture.publication();
    publication.appearances[0].height = 1;
    world_assets::CpuAssets assets(std::move(publication));
    const auto assetsLease = assets.acquire(world_assets::allAssets(assets.publication));
    auto rows = std::vector<std::string>(9, std::string(9, '.'));
    rows.front() = rows.back() = std::string(9, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    WorldDefinition world;
    world.surfaces.resize(81);
    for (auto& surface : world.surfaces)
        surface.ceiling = noCeiling;
    const Map map(rows, world);
    const auto catalog = creationCatalog(true);
    const Player pose{{4, 6}, Heading::North};
    FloorFixtures fixtures;
    fixtures.count = 1;
    fixtures.entries[0] = {{4, 4}, Heading::North, FixtureKind::Door};
    FloorGeometry geometry(map, fixtures);
    NpcWorld places(map, pose.cell, {"goblin"}, {{{0}, {4, 4}, {0, 0}, Heading::South}});
    ActorWorld actors(map, catalog, places);
    NpcAnimationSnapshot animation;
    animation.world = actors.snapshot().world;
    animation.actorRevision = actors.snapshot().revision;
    animation.count = 1;
    animation.frames[0].offsetX = .25f;
    animation.frames[0].offsetY = .67f; // Inside the actual closed leaf, not just its gameplay cell.
    MovementState movement;
    movement.player = pose;
    auto scene = world_render::prepare(geometry.view());
    auto pipeline = std::make_unique<world_render::VisibilityPipeline>(scene, assets);
    pipeline->update({geometry.view(), movement, &actors, &animation});
    check(pipeline->picking()->targets().count == 0 && pipeline->picking()->targets().candidateCount == 0 &&
              !pipeline->picking()->targets().anchors[0].valid,
          "A body fully embedded in closed door geometry has no pixels, candidates or anchors");
    animation.frames[0].elevation = .6f;
    ++animation.revision;
    pipeline->update({geometry.view(), movement, &actors, &animation});
    const auto& picking = *pipeline->picking();
    check(picking.targets().count == 1 && picking.targets().candidateCount == 1,
          "The head above a low door remains visible and selectable at full size");
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x)
            if (picking.slot(x, y) == 0) {
                const float height =
                    pipeline->frame().camera.position.y +
                    (viewHeight * .40f - y - .5f) * picking.depth(y * viewWidth + x) / world_render::focal();
                check(height >= 1.f - 1e-5f,
                      "Only body samples above the closed leaf survive its intersection mask");
            }
    // No scenery nearby: the frustum must clip partial bodies without changing size,
    // including a body whose axis has already left the viewport.
    movement.player.cell = {6, 6};
    animation.frames[0].offsetY = animation.frames[0].elevation = 0;
    for (float offset : {0.f, -.23f}) {
        animation.frames[0].offsetX = offset;
        ++animation.revision;
        pipeline->update({map, movement, &actors, &animation});
        const auto& b = pipeline->frame().billboards[0];
        const auto foot = world_render::project(pipeline->frame().camera, b.feet);
        check(b.width == .5f && b.height == 1 && b.shadowRadius == .225f &&
                  std::abs(b.feet.x - (4.25f + offset)) < 1e-6f &&
                  foot.x < .5f * b.width * world_render::focal() / b.depth,
              "Viewport edge preserves creature dimensions and the original partial-body position");
        check(pipeline->picking()->targets().count == 1 && pipeline->picking()->targets().candidateCount == 1,
              "The exposed fragment of a partial body remains clickable and selectable");
    }
    movement.player.cell = {7, 6};
    pipeline->update({map, movement, &actors, &animation});
    check(pipeline->picking()->targets().count == 0 && pipeline->picking()->targets().candidateCount == 0,
          "A body fully outside the viewport cannot remain a target candidate");
}
} // namespace blobber::test
