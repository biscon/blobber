#include "assets/image.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <iostream>
using namespace blobber;
namespace {
unsigned compare(GpuResources& r, WorldRenderer& renderer, TargetHandle ids) {
    renderer.diagnosticIds(ids);
    OwnedImage image(LoadImageFromTexture(r.target(ids).texture));
    check(image.value.data, "Diagnostic ID readback");
    ImageFlipVertical(&image.value);
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* pixels = static_cast<const unsigned char*>(image.value.data);
    const auto& picking = renderer.visibility()->frame().picking;
    std::array<bool, PickingBuffer::fixtureBase + fixtureCapacity> cpu{}, gpu{};
    unsigned mismatch = 0, interior = 0;
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x) {
            const auto p = (y * viewWidth + x) * 4;
            const auto actual = std::uint16_t(pixels[p] + 256 * pixels[p + 1]), expected = picking.slot(x, y);
            if (actual < gpu.size())
                gpu[actual] = true;
            if (expected < cpu.size())
                cpu[expected] = true;
            if (actual == expected)
                continue;
            ++mismatch;
            bool edge = false;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    edge |= picking.slot(x + dx, y + dy) == actual;
            if (!edge)
                ++interior;
        }
    if (interior)
        std::cerr << "Interior mismatches " << interior << " / total " << mismatch << '\n';
    check(interior == 0, "CPU/GPU ID interiors agree within one-pixel geometric boundary");
    check(cpu == gpu, "CPU/GPU exact visible object membership");
    return mismatch;
}
unsigned creatureDoorClipping(GpuResources& resources) {
    test::VisibilityFixture fixture(1, 2, true);
    auto publication = fixture.publication();
    publication.appearances[0].clips[unsigned(NpcAnimation::Attack)].width = .9f;
    const auto assets = std::make_shared<const world_assets::CpuAssets>(std::move(publication));
    const auto catalog = creationCatalog(true);
    const auto map = test::worldRoom(9, 9);
    unsigned differences = 0;
    for (unsigned h = 0; h < 4; ++h)
        for (int slot : {0, 3}) {
            const auto heading = Heading(h);
            const auto f = forward(heading);
            const Cell doorCell{4, 4}, subcell{slot % 2, slot / 2};
            const Player pose{doorCell + f * -1, heading};
            FloorFixtures fixtures;
            fixtures.count = 1;
            fixtures.entries[0] = {doorCell, heading, FixtureKind::Door};
            NpcWorld places(map, pose.cell, {"goblin"}, {{{0}, doorCell + f, subcell, Heading((h + 2) % 4)}});
            GameSession game(map, catalog, places, 5, pose, false, fixture.timings, {}, fixtures);
            game.setAnimated(false);
            check(bool(game.interact(game.nearbyFixture())), "GPU clipping fixture opens door");
            const auto id = game.actors().id(0);
            check(bool(game.actors().walk(id, doorCell, subcell, pose.cell, 0, 600)),
                  "GPU clipping creature enters doorway");
            WorldRenderer renderer(resources, assets);
            NpcAnimator animator(game.actors(), fixture.timings);
            auto doors = game.doors();
            auto movement = game.movement();
            auto input = [&] {
                return world_render::FrameInput{game.geometry(),      movement, &game.actors(),
                                                &animator.snapshot(), nullptr,  &doors};
            };
            renderer.stage(input());
            renderer.commit();
            GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
            const auto transfers = resources.transfers();
            for (unsigned angle : {0u, 150u, 300u}) {
                doors.now = angle;
                doors.until[0] = 300;
                ++doors.revision;
                for (unsigned time : {0u, 300u, 600u}) {
                    animator.setTime(time);
                    // Include wide registered attack art at the middle walk sample.
                    auto animation = animator.snapshot();
                    if (time == 300)
                        animation.frames[0].clip = NpcAnimation::Attack;
                    auto in = input();
                    in.animation = &animation;
                    renderer.supersampling(angle != 150);
                    allocations = 0;
                    watching = true;
                    renderer.render(in);
                    watching = false;
                    check(allocations == 0, "Scenery-clipped GPU frame allocates no engine C++ storage");
                    differences += compare(resources, renderer, ids.get());
                }
            }
            check(bool(game.actors().walk(id, doorCell + f, subcell, pose.cell, 1000, 600)),
                  "GPU clipping creature leaves doorway in reverse");
            for (unsigned time : {1000u, 1300u, 1600u}) {
                animator.setTime(time);
                renderer.render(input());
                differences += compare(resources, renderer, ids.get());
            }
            movement.transition = {pose, {pose.cell, Heading((h + 1) % 4)}, Action::TurnRight, .04f, .16f};
            renderer.render(input());
            differences += compare(resources, renderer, ids.get());
            check(resources.transfers().uploads == transfers.uploads &&
                      resources.transfers().bytes == transfers.bytes,
                  "Clipping masks require no texture or mesh resource uploads");
        }
    {
        // A wide body crosses a neighboring raised floor/low ceiling. A nearer
        // bag is drawn afterward, checking that the body masks are reset.
        WorldDefinition definition;
        definition.surfaces.resize(81);
        for (auto& surface : definition.surfaces)
            surface.ceiling = 8;
        definition.surfaces[4 * 9 + 3] = {1, 5};
        const Map terrain({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                           "#.......#", "#.......#", "#########"},
                          definition);
        MovementState movement;
        movement.player = {{4, 6}, Heading::North};
        NpcWorld places(terrain, movement.player.cell, {"goblin"}, {{{0}, {4, 4}, {0, 0}, Heading::South}});
        ActorWorld actors(terrain, catalog, places);
        NpcAnimationSnapshot animation;
        animation.world = actors.snapshot().world;
        animation.actorRevision = actors.snapshot().revision;
        animation.count = 1;
        animation.frames[0].clip = NpcAnimation::Attack;
        LootSnapshot loot;
        loot.world = 12;
        loot.stacks[0] = {{4, 5}, {0}, 1, true, 8};
        const world_render::FrameInput input{terrain, movement, &actors, &animation, &loot};
        WorldRenderer renderer(resources, assets);
        renderer.stage(input);
        renderer.commit();
        GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
        for (float elevation : {0.f, .6f})
            for (bool quality : {false, true}) {
                animation.frames[0].elevation = elevation;
                ++animation.revision;
                renderer.supersampling(quality);
                renderer.render(input);
                differences += compare(resources, renderer, ids.get());
            }
        movement.transition = {movement.player, {{4, 6}, Heading::East}, Action::TurnRight, .025f, .16f};
        renderer.render(input);
        differences += compare(resources, renderer, ids.get());
    }
    return differences;
}
} // namespace
int main() {
    try {
        test::VisibilityFixture fixture;
        auto assets = std::make_shared<const world_assets::CpuAssets>(fixture.publication());
        const auto catalog = creationCatalog(true);
        const auto map = test::worldRoom(9, 9);
        MovementState movement;
        movement.player = {{4, 6}, Heading::North};
        const NpcWorld placements(
            map, movement.player.cell, {"goblin"},
            {{{0}, {4, 4}, {0, 0}, Heading::South}, {{0}, {4, 3}, {0, 0}, Heading::South}});
        ActorWorld actors(map, catalog, placements);
        NpcAnimationSnapshot animation;
        animation.count = 2;
        animation.world = actors.snapshot().world;
        animation.actorRevision = actors.snapshot().revision;
        animation.frames[1].active = 1;
        LootSnapshot loot;
        loot.world = 12;
        loot.stacks[0] = {{4, 6}, {0}, 1, true, 8};
        Window window(640, 360, true);
        check(IsWindowState(FLAG_WINDOW_HIDDEN), "GPU review remains hidden");
        GpuResources resources;
        unsigned differences = creatureDoorClipping(resources);
        {
            WorldRenderer renderer(resources, assets);
            world_render::FrameInput input{map, movement, &actors, &animation, &loot};
            const auto shared = resources.counts();
            for (int point = 0; point < 7; ++point) {
                resources.failAfter(point);
                rejects([&] { renderer.stage(input); },
                        "Every initial interactive acquisition failure rolls back");
                resources.failAfter(-1);
                const auto count = resources.counts();
                check(!renderer.staged() && count.textures == shared.textures &&
                          count.targets == shared.targets && count.meshes == shared.meshes &&
                          count.shaders == shared.shaders,
                      "Failed first stage releases newly acquired shared billboard resources");
            }
            renderer.stage(input);
            renderer.commit();
            GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
            for (bool quality : {false, true}) {
                renderer.supersampling(quality);
                for (unsigned clip = 0; clip < npcAnimationCount; ++clip) {
                    animation.frames[0].clip = NpcAnimation(clip);
                    animation.revision++;
                    renderer.render(input);
                    differences += compare(resources, renderer, ids.get());
                    check(renderer.picking() && renderer.picking()->targets().count == 2,
                          "Both quality paths retain interaction");
                }
            }
            // IDs do not exercise the premultiplied colour blend transition.
            // A fully opaque flashed body has a known white interior in both qualities.
            animation.frames[0].clip = NpcAnimation::Idle;
            animation.frames[0].active = 1;
            animation.frames[0].flash = true;
            ++animation.revision;
            for (bool quality : {false, true}) {
                renderer.supersampling(quality);
                renderer.render(input);
                OwnedImage color(LoadImageFromTexture(resources.target(renderer.image().target).texture));
                ImageFlipVertical(&color.value);
                unsigned white = 0;
                const auto& picking = renderer.visibility()->frame().picking;
                for (int y = 2; y < viewHeight - 2; ++y)
                    for (int x = 2; x < viewWidth - 2; ++x) {
                        if (picking.slot(x, y) != 0 || picking.slot(x - 2, y) != 0 ||
                            picking.slot(x + 2, y) != 0 || picking.slot(x, y - 2) != 0 ||
                            picking.slot(x, y + 2) != 0)
                            continue;
                        const auto c = GetImageColor(color.value, x, y);
                        white += c.r > 240 && c.g > 240 && c.b > 240;
                    }
                check(white > 20, "Composited colour shows opaque flashed body interiors at both qualities");
            }
            animation.frames[0].flash = false;
            ++animation.revision;
            renderer.render(input);
            const auto canonicalRevision = renderer.visibility()->frame().revision;
            const auto* canonicalPicking = renderer.picking();
            renderer.supersampling(false);
            renderer.render(input);
            check(renderer.visibility()->frame().revision == canonicalRevision &&
                      renderer.picking() == canonicalPicking,
                  "Quality-only changes reuse exact canonical interaction snapshot");
            renderer.render(input);
            check(renderer.stats.cached && renderer.stats.draws == 0,
                  "Unchanged interactive world caches all GPU draws");
            const auto normalCamera = world_render::camera(input.movement, input.geometry);
            for (float weight : {0.f, .5f, 1.f}) {
                auto dialogueCamera = normalCamera;
                dialogueCamera.position.z -= weight * .45f;
                dialogueCamera.horizon -= weight * .2f;
                input.cameraOverride = dialogueCamera;
                renderer.render(input);
                check(world_render::sameCamera(renderer.visibility()->frame().camera, dialogueCamera),
                      "GPU visibility consumes shared dialogue camera");
                differences += compare(resources, renderer, ids.get());
            }
            input.cameraOverride.reset();
            renderer.render(input);
            check(world_render::sameCamera(renderer.visibility()->frame().camera, normalCamera),
                  "Dialogue camera restores original projection");
            actors.face(actors.id(0), Heading::North);
            animation.actorRevision = actors.snapshot().revision;
            animation.frames[0].clip = NpcAnimation::Hurt;
            animation.frames[0].active = 1;
            animation.revision++;
            renderer.render(input);
            differences += compare(resources, renderer, ids.get());
            bool noncentral = false;
            for (unsigned i = 0; i < renderer.visibility()->frame().count; ++i)
                if (renderer.visibility()->frame().billboards[i].id == 0)
                    noncentral = renderer.visibility()->frame().billboards[i].anchorX == .25f;
            check(noncentral, "Noncentral registered pivot reaches CPU and GPU transforms");
            animation.frames[0].clip = NpcAnimation::Idle;
            animation.frames[0].active = 0;
            animation.revision++;
            for (auto facing : {Heading::North, Heading::East, Heading::South, Heading::West}) {
                actors.face(actors.id(0), facing);
                animation.actorRevision = actors.snapshot().revision;
                animation.revision++;
                renderer.render(input);
                differences += compare(resources, renderer, ids.get());
            }
            actors.face(actors.id(0), Heading::South);
            animation.actorRevision = actors.snapshot().revision;
            animation.revision++;
            for (float depth : {.024f, .026f}) {
                animation.frames[0].offsetY = 2.65f - depth;
                animation.revision++;
                renderer.render(input);
                differences += compare(resources, renderer, ids.get());
                bool front = false;
                for (unsigned i = 0; i < renderer.picking()->targets().count; ++i)
                    front |= renderer.picking()->targets().ids[i].slot == 0;
                check(front == (depth > .025f),
                      "Billboard crossing near plane clips without stale interaction");
            }
            animation.frames[0].offsetY = 0;
            animation.revision++;
            const auto before = resources.transfers();
            watching = true;
            allocations = 0;
            for (unsigned i = 0; i < 12; ++i) {
                animation.revision++;
                animation.frames[1].active = std::uint8_t(i % 2);
                renderer.supersampling(i % 2 != 0);
                renderer.render(input);
            }
            watching = false;
            check(allocations == 0, "Warmed CPU/GPU interaction submission has no C++ allocations");
            check(resources.transfers().uploads == before.uploads &&
                      resources.transfers().bytes == before.bytes,
                  "No frame mesh/texture resource upload");
            // Interpolated turns and traveling silhouettes use one shared camera/model.
            movement.transition = {movement.player, {{4, 6}, Heading::East}, Action::TurnRight, .035f, .16f};
            animation.frames[0].offsetX = .14f;
            animation.revision++;
            renderer.render(input);
            differences += compare(resources, renderer, ids.get());
            check(!renderer.picking(), "Moving GPU view suspends interaction");
            movement.transition.elapsed = .16f;
            animation.frames[0].offsetX = 0;
            animation.revision++;
            FloorFixtures fixtures;
            fixtures.count = 1;
            fixtures.entries[0] = {{4, 5}, Heading::North, FixtureKind::Door};
            FloorGeometry geometry(map, fixtures);
            renderer.render({geometry.view(), movement, &actors, &animation, &loot});
            differences += compare(resources, renderer, ids.get());
            check(renderer.picking()->targets().count == 0, "Closed proxy occludes GPU/CPU actors");
            // Failed staging preserves both published pixels/IDs and the active world.
            const auto revision = renderer.image().revision;
            const auto* picking = renderer.picking();
            resources.failAfter(0);
            rejects([&] { renderer.stage(input); }, "Stage failure injection");
            resources.failAfter(-1);
            check(renderer.image().revision == revision && renderer.picking() == picking,
                  "Failed stage retains image and picking atomically");
            check(renderer.cpuBytes() < world_assets::worldCpuBudget &&
                      renderer.gpuBytes() < world_assets::worldGpuBudget,
                  "Active/staged subsystem budgets");
            unsigned cpuFailures = 0;
            const auto retained = resources.counts();
            for (long point = 0; point < 20; ++point) {
                allocationsUntilFailure = point;
                try {
                    renderer.stage(input);
                } catch (const std::bad_alloc&) {
                    ++cpuFailures;
                }
                allocationsUntilFailure = -1;
                renderer.cancel();
                const auto after = resources.counts();
                check(renderer.image().revision == revision && renderer.picking() == picking &&
                          after.targets == retained.targets && after.meshes == retained.meshes,
                      "CPU preparation failure/cancel retains image, picking and resources");
            }
            check(cpuFailures >= 10, "Preparation failure guard reaches geometry and visibility buffers");
            // Native 2D state remains usable after billboard/fringe/ID passes.
            BeginTextureMode(resources.target(ids.get()));
            ClearBackground(BLACK);
            DrawRectangle(10, 10, 20, 20, RED);
            EndTextureMode();
            OwnedImage checkImage(LoadImageFromTexture(resources.target(ids.get()).texture));
            ImageFlipVertical(&checkImage.value);
            check(GetImageColor(checkImage.value, 15, 15).r == RED.r,
                  "Native 2D state restored after world drawing");
        }
        check(resources.counts().textures == 0 && resources.counts().meshes == 0 &&
                  resources.counts().shaders == 0 && resources.counts().targets == 0,
              "All world billboard resources release before context");
        {
            test::VisibilityFixture cropped(1, 6, true);
            WorldRenderer renderer(resources, cropped.publication());
            movement = {};
            movement.player = {{4, 6}, Heading::North};
            animation.frames = {};
            animation.actorRevision = actors.snapshot().revision;
            world_render::FrameInput input{map, movement, &actors, &animation};
            renderer.stage(input);
            renderer.commit();
            GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
            for (unsigned clip = 0; clip < npcAnimationCount; ++clip)
                for (bool quality : {false, true}) {
                    animation.frames[0].clip = NpcAnimation(clip);
                    ++animation.revision;
                    renderer.supersampling(quality);
                    renderer.render(input);
                    differences += compare(resources, renderer, ids.get());
                }
            const auto transfers = resources.transfers();
            watching = true;
            allocations = 0;
            for (unsigned i = 0; i < 4; ++i) {
                animation.frames[0].clip = NpcAnimation::Idle;
                animation.frames[0].active = std::uint8_t(i % 2);
                ++animation.revision;
                renderer.render(input);
            }
            watching = false;
            check(allocations == 0 && resources.transfers().uploads == transfers.uploads,
                  "Cropped animation frames allocate no engine C++ storage or texture uploads");
        }
        check(resources.counts().textures == 0 && resources.counts().targets == 0,
              "Cropped textures release with their original world owner");
        std::cout << "World billboard GPU parity/lifecycle passed; boundary pixel differences=" << differences
                  << '\n';
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
