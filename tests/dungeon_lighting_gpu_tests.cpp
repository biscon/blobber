#include "assets/image.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <cmath>
#include <iostream>
using namespace blobber;
using world_render::DungeonLighting;
namespace {
constexpr DungeonLighting presets[]{DungeonLighting::Current, DungeonLighting::Gentle,
                                    DungeonLighting::Strong, DungeonLighting::Early};
std::vector<unsigned char> pixels(GpuResources& r, TargetHandle target) {
    OwnedImage im(LoadImageFromTexture(r.target(target).texture));
    check(im.value.data, "Read lighting fixture output");
    ImageFlipVertical(&im.value);
    ImageFormat(&im.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* p = static_cast<const unsigned char*>(im.value.data);
    return {p, p + im.value.width * im.value.height * 4};
}
Map room(const char* pack) {
    WorldDefinition world;
    world.assetPack = pack;
    return Map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                "#.......#", "#.......#", "#########"},
               world);
}
float light(float depth) {
    return 1.f / (1.f + .115f * depth + .015f * depth * depth);
}
void surfaceChecks(GpuResources& resources, const world_assets::Publication& publication) {
    const auto map = room("underkeep");
    WorldRenderer renderer(resources, publication, true, false);
    MovementState movement;
    movement.player = {{4, 1}, Heading::North};
    renderer.stage(map, movement);
    renderer.commit();
    const auto defaultImage = pixels(resources, renderer.image().target);
    renderer.dungeonLighting(DungeonLighting::Current);
    renderer.render(movement);
    check(pixels(resources, renderer.image().target) != defaultImage,
          "Default stone lighting differs from legacy lighting even nearby");
    renderer.dungeonLighting(DungeonLighting::Early);
    renderer.render(movement);
    check(pixels(resources, renderer.image().target) == defaultImage,
          "Default stone lighting matches accepted C / Early");
    for (bool quality : {false, true}) {
        renderer.supersampling(quality);
        for (int y : {1, 2, 3, 6}) {
            movement.player.cell.y = y;
            // Facing north: eye setback adds .4 to the half-cell wall distance.
            const float depth = float(y) - .1f;
            for (auto preset : presets) {
                renderer.dungeonLighting(preset);
                renderer.render(movement);
                const auto image = pixels(resources, renderer.image().target);
                const auto p = (100 * viewWidth + 250) * 4;
                const float distance = preset == DungeonLighting::Early
                                           ? depth * 1.5f
                                           : depth + (preset == DungeonLighting::Gentle   ? .5f
                                                      : preset == DungeonLighting::Strong ? 1.f
                                                                                          : 0.f) *
                                                         std::max(depth - 2, 0.f);
                for (unsigned c = 0; c < 3; ++c) {
                    const float material[]{180, 80, 40};
                    check(std::abs(float(image[p + c]) - material[c] * light(distance)) <= 2,
                          "Surface GPU shading matches expected distance at near and far poses");
                }
                renderer.render(movement);
                check(renderer.stats.cached, "Repeated preset reuses prepared image");
            }
        }
    }
    const auto counts = resources.counts();
    const auto transfers = resources.transfers();
    allocations = 0;
    watching = true;
    for (auto preset : presets) {
        renderer.dungeonLighting(preset);
        renderer.render(movement);
    }
    watching = false;
    check(!allocations && resources.counts().bytes == counts.bytes &&
              resources.transfers().uploads == transfers.uploads,
          "Preset switches allocate no C++ storage or GPU uploads");
    const auto active = pixels(resources, renderer.image().target);
    const auto city = room("sanctuary-city");
    renderer.stage(city, movement);
    check(pixels(resources, renderer.image().target) == active,
          "Staging another pack preserves active image");
    renderer.cancel();
    renderer.render(movement);
    check(renderer.stats.cached, "Cancelled staging preserves active lighting cache");
    renderer.stage(city, movement);
    renderer.commit();
    const auto cityImage = pixels(resources, renderer.image().target);
    for (auto preset : presets) {
        renderer.dungeonLighting(preset);
        renderer.render(movement);
        check(renderer.stats.cached && pixels(resources, renderer.image().target) == cityImage,
              "City staging resolves neutral lighting and ignores trial changes");
    }
}
void creatureAndIsolationChecks(GpuResources& resources, const test::VisibilityFixture& fixture,
                                const world_assets::Publication& publication) {
    const auto catalog = creationCatalog(true);
    for (const char* pack : {"underkeep", "sanctuary-city", "sanctuary-inn", "dungeon"}) {
        const auto map = room(pack);
        MovementState movement;
        movement.player = {{4, 7}, Heading::North};
        const NpcWorld placements(map, movement.player.cell, {"goblin"},
                                  {{{0}, {4, 3}, {0, 0}, Heading::South}});
        ActorWorld actors(map, catalog, placements);
        NpcAnimator animator(actors, fixture.timings);
        animator.setTime(0);
        WorldRenderer renderer(resources, publication, false, false);
        renderer.dungeonLighting(DungeonLighting::Current);
        const world_render::FrameInput input{map, movement, &actors, &animator.snapshot()};
        renderer.stage(input);
        renderer.commit();
        const auto baseline = pixels(resources, renderer.image().target);
        const auto visibilityRevision = renderer.visibility()->frame().revision;
        GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
        renderer.diagnosticIds(ids.get());
        const auto originalIds = pixels(resources, ids.get());
        const auto& b = renderer.visibility()->frame().billboards[0];
        check(b.depth > 2 && b.depth < 9, "Creature exercises unfogged far field");
        for (auto preset : presets) {
            renderer.dungeonLighting(preset);
            renderer.render(input);
            const auto image = pixels(resources, renderer.image().target);
            const bool stone = std::string_view(pack) == "underkeep";
            check(renderer.visibility()->frame().revision == visibilityRevision,
                  "Lighting leaves picking/visibility preparation unchanged");
            if (!stone)
                check(renderer.stats.cached && image == baseline, "Other packs are pixel-identical");
            else if (preset != DungeonLighting::Current)
                check(!renderer.stats.cached && image != baseline, "Trial switch invalidates stone color");
            renderer.diagnosticIds(ids.get());
            check(pixels(resources, ids.get()) == originalIds, "Diagnostic IDs ignore lighting");
            const float distance =
                world_render::dungeonLightingDepth(b.depth, stone ? preset : DungeonLighting::Current);
            const float ratio = light(distance) / light(b.depth);
            unsigned body = 0;
            for (int y = 2; y < viewHeight - 2; ++y)
                for (int x = 2; x < viewWidth - 2; ++x) {
                    bool interior = true;
                    for (int dy = -2; dy <= 2; ++dy)
                        for (int dx = -2; dx <= 2; ++dx) {
                            const auto i = ((y + dy) * viewWidth + x + dx) * 4;
                            interior &= originalIds[i] == 0 && originalIds[i + 1] == 0;
                        }
                    if (!interior)
                        continue;
                    const auto i = (y * viewWidth + x) * 4;
                    for (unsigned c = 0; c < 3; ++c)
                        check(std::abs(float(image[i + c]) - baseline[i + c] * ratio) <= 2,
                              "Opaque creature follows same attenuation curve as surfaces");
                    ++body;
                }
            check(body > 20, "Creature comparison covers visible interior pixels");
        }
    }
}
} // namespace
int main() try {
    test::VisibilityFixture fixture(1, 0, false, true, true);
    auto publication = fixture.publication();
    auto inn = publication.packs[0];
    inn.id = "sanctuary-inn";
    publication.packs.push_back(inn);
    Window window(640, 360, true, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Lighting tests remain hidden");
    GpuResources resources;
    const auto baseline = resources.counts();
    surfaceChecks(resources, publication);
    creatureAndIsolationChecks(resources, fixture, publication);
    const auto after = resources.counts();
    check(after.bytes == baseline.bytes && after.targets == baseline.targets &&
              after.textures == baseline.textures && after.meshes == baseline.meshes &&
              after.shaders == baseline.shaders,
          "Lighting resources fully released");
    using Error = unsigned (*)();
    check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0, "Valid final OpenGL state");
    std::cout << "Dungeon lighting GPU checks passed\n";
} catch (const std::exception& e) {
    watching = false;
    std::cerr << e.what() << '\n';
    return 1;
}
