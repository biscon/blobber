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
        unsigned differences = 0;
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
            check(renderer.cpuBytes() < 352u * world_assets::MiB &&
                      renderer.gpuBytes() < 352u * world_assets::MiB,
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
        std::cout << "World billboard GPU parity/lifecycle passed; boundary pixel differences=" << differences
                  << '\n';
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
