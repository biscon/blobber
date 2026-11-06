#include "assets/image.hpp"
#include "platform/window.hpp"
#include "platform/world_projection.hpp"
#include "platform/world_renderer.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/architecture_fixtures.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <algorithm>
#include <iostream>

using namespace blobber;
namespace {
void projectionChecks() {
    for (float far : {16.f, 40.f}) {
        const auto projection = worldProjection(far), inverse = MatrixInvert(projection);
        for (float distance : {.026f, .3f, 3.f, far * .9f})
            for (float x : {-.8f, 0.f, .8f})
                for (float y : {-.5f, 0.f, .3f}) {
                    const Vector4 p{x * distance, y * distance, -distance, 1};
                    auto clip = QuaternionTransform(p, projection);
                    clip = Vector4Scale(clip, 1 / clip.w);
                    for (unsigned heading = 0; heading < 4; ++heading) {
                        MovementState movement;
                        movement.player = {{0, 0}, Heading(heading)};
                        const auto c = world_render::camera(movement);
                        const auto cpu = world_render::project(
                            c, {c.position.x + c.right.x * p.x + c.forward.x * distance, c.position.y + p.y,
                                c.position.z + c.right.z * p.x + c.forward.z * distance});
                        check(std::abs(cpu.x - (clip.x * .5f + .5f) * viewWidth) < .003f &&
                                  std::abs(cpu.y - (.5f - clip.y * .5f) * viewHeight) < .003f,
                              "CPU picking projection matches GPU frustum across headings and depths");
                    }
                    auto restored = QuaternionTransform(clip, inverse);
                    restored = Vector4Scale(restored, 1 / restored.w);
                    check(std::abs(restored.x - p.x) < .0002f * distance &&
                              std::abs(restored.y - p.y) < .0002f * distance &&
                              std::abs(restored.z - p.z) < .0002f * distance,
                          "Actual asymmetric projection reconstructs positions across near/far ranges");
                }
        const auto horizon = QuaternionTransform({0, 0, -1, 1}, projection);
        check(std::abs((horizon.y / horizon.w * .5f + .5f) - .60f) < .00001f,
              "Reconstruction uses offset horizon, not a centered perspective approximation");
    }
}
std::vector<unsigned char> pixels(GpuResources& r, TargetHandle target) {
    OwnedImage im(LoadImageFromTexture(r.target(target).texture));
    check(im.value.data, "Read AO test image");
    ImageFlipVertical(&im.value);
    ImageFormat(&im.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* p = static_cast<const unsigned char*>(im.value.data);
    return {p, p + im.value.width * im.value.height * 4};
}
bool same(GpuCounts a, GpuCounts b) {
    return a.textures == b.textures && a.targets == b.targets && a.meshes == b.meshes &&
           a.shaders == b.shaders && a.bytes == b.bytes;
}
void save(GpuResources& r, TargetHandle target, const std::filesystem::path& output) {
    OwnedImage im(LoadImageFromTexture(r.target(target).texture));
    ImageFlipVertical(&im.value);
    check(ExportImage(im.value, output.c_str()), "Export AO fixture capture");
}
void environmentChecks(GpuResources& resources, const std::filesystem::path& output) {
    test::WorldFixture fixture;
    const auto map = test::worldRoom(7, 7);
    MovementState movement;
    movement.player = {{2, 2}, Heading::North};
    WorldRenderer renderer(resources, world_assets::load(fixture.directory));
    renderer.stage(map, movement);
    renderer.commit();
    const auto ao = pixels(resources, renderer.ambientOcclusionTarget());
    unsigned shaded = 0;
    for (std::size_t i = 0; i < ao.size(); i += 4) {
        check(ao[i] >= 159 && ao[i] == ao[i + 1] && ao[i] == ao[i + 2], "AO is bounded and neutral");
        shaded += ao[i] < 249;
    }
    check(shaded > 1000, "Wall/floor junction produces contact shading");
    // Interior of a flat wall, well outside the AO radius of any corner.
    const auto center = world_render::project(world_render::camera(movement, map), {2.5f, .6f, 1.f});
    const auto index = (std::size_t(int(center.y * 2)) * 1000 + int(center.x * 2)) * 4;
    check(ao.at(index) >= 253, "Exposed flat wall remains unoccluded");
    const auto on = pixels(resources, renderer.image().target);
    if (!output.empty()) {
        save(resources, renderer.image().target, output / "corner-on.png");
        save(resources, renderer.ambientOcclusionTarget(), output / "corner-ao.png");
    }
    renderer.ambientOcclusion(false);
    renderer.render(movement);
    check(!renderer.stats.cached, "AO toggle invalidates color");
    const auto off = pixels(resources, renderer.image().target);
    check(on != off, "AO changes rendered environment");
    unsigned darkened = 0;
    for (std::size_t i = 0; i < on.size(); i += 4) {
        check(on[i] <= off[i] + 1, "AO never brightens environment");
        darkened += on[i] + 2 < off[i];
    }
    check(darkened > 100, "Contact shading reaches published color");
    if (!output.empty())
        save(resources, renderer.image().target, output / "corner-off.png");
    renderer.ambientOcclusion(true);
    renderer.render(movement);
    check(pixels(resources, renderer.image().target) == on, "AO is deterministic across toggles");
    renderer.render(movement);
    check(renderer.stats.cached, "Unchanged AO image is cached");
    const auto resident = resources.counts();
    MovementState destination = movement;
    destination.player = {{4, 4}, Heading::East};
    resources.failAfter(0);
    rejects([&] { renderer.stage(map, destination); }, "Failed staging rejects with AO enabled");
    resources.failAfter(-1);
    check(!renderer.staged() && same(resident, resources.counts()), "Failed staging preserves resources");
    renderer.stage(map, destination);
    check(pixels(resources, renderer.image().target) == on, "AO scratch staging preserves active color");
    renderer.cancel();
    check(same(resident, resources.counts()), "Cancelled stage releases candidate resources");
    allocations = 0;
    const auto transfers = resources.transfers();
    watching = true;
    for (unsigned i = 0; i < 4; ++i) {
        renderer.supersampling(i % 2);
        renderer.ambientOcclusion(i != 1);
        renderer.render(i < 2 ? destination : movement);
    }
    watching = false;
    check(!allocations && same(resident, resources.counts()) &&
              resources.transfers().uploads == transfers.uploads,
          "AO rendering and quality/toggle changes allocate and upload nothing");
    renderer.supersampling(false);
    renderer.ambientOcclusion(true);
    renderer.render(movement);
    check(pixels(resources, renderer.ambientOcclusionTarget()) == ao, "Quality modes share identical AO");
}
void creatureChecks(GpuResources& resources) {
    test::VisibilityFixture fixture(1, 0, false, true);
    const auto map = test::worldRoom(7, 7);
    const auto catalog = creationCatalog(true);
    MovementState movement;
    movement.player = {{3, 4}, Heading::North};
    const NpcWorld placements(map, movement.player.cell, {"goblin"}, {{{0}, {3, 3}, {0, 0}, Heading::South}});
    ActorWorld actors(map, catalog, placements);
    NpcAnimator animator(actors, fixture.timings);
    animator.setTime(0);
    WorldRenderer renderer(resources, fixture.publication(), false);
    const world_render::FrameInput input{map, movement, &actors, &animator.snapshot()};
    renderer.stage(input);
    renderer.commit();
    const auto ao = pixels(resources, renderer.ambientOcclusionTarget());
    const auto on = pixels(resources, renderer.image().target);
    GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
    renderer.diagnosticIds(ids.get());
    const auto onIds = pixels(resources, ids.get());
    renderer.ambientOcclusion(false);
    renderer.render(input);
    const auto off = pixels(resources, renderer.image().target);
    renderer.diagnosticIds(ids.get());
    check(pixels(resources, ids.get()) == onIds, "AO does not change diagnostic IDs");
    unsigned body = 0;
    auto isBody = [&](int x, int y) {
        const auto i = (y * viewWidth + x) * 4;
        return onIds[i] == 0 && onIds[i + 1] == 0;
    };
    for (int y = 2; y < viewHeight - 2; ++y)
        for (int x = 2; x < viewWidth - 2; ++x) {
            bool interior = true;
            for (int dy = -2; dy <= 2; ++dy)
                for (int dx = -2; dx <= 2; ++dx)
                    interior &= isBody(x + dx, y + dy);
            if (!interior)
                continue;
            const auto i = (y * viewWidth + x) * 4;
            for (int c = 0; c < 3; ++c)
                check(std::abs(int(on[i + c]) - int(off[i + c])) <= 1, "Opaque creature receives no AO");
            ++body;
        }
    check(body > 20, "Creature exclusion checks visible interior pixels");
    renderer.ambientOcclusion(true);
    animator.setTime(80);
    renderer.render(input);
    check(pixels(resources, renderer.ambientOcclusionTarget()) == ao, "Creature animation contributes no AO");
    const world_render::FrameInput empty{map, movement};
    renderer.render(empty);
    check(pixels(resources, renderer.ambientOcclusionTarget()) == ao,
          "Removing creature and shadow leaves AO unchanged");
}
void sceneryChecks(GpuResources& resources) {
    test::ArchitectureFixture fixture(true);
    auto world = test::sceneryWorld();
    const Map map(test::sceneryRows(), world);
    MovementState movement;
    movement.player = {{7, 4}, Heading::South};
    WorldRenderer renderer(resources, fixture.publication());
    renderer.stage(map, movement);
    renderer.commit();
    const auto on = pixels(resources, renderer.image().target);
    const auto ao = pixels(resources, renderer.ambientOcclusionTarget());
    renderer.ambientOcclusion(false);
    renderer.render(movement);
    const auto off = pixels(resources, renderer.image().target);
    // No geometry above the canopy: sky must remain unchanged and unoccluded.
    for (unsigned y = 0; y < 20; ++y)
        for (unsigned x = 0; x < 500; ++x) {
            const auto i = (y * 500 + x) * 4;
            check(on[i] == off[i] && on[i + 1] == off[i + 1] && on[i + 2] == off[i + 2], "AO excludes sky");
            check(ao[(y * 2 * 1000 + x * 2) * 4] == 255, "Background AO is neutral");
        }
    // Turn/near-plane/screen-edge samples must retain bounded factors and valid GL state.
    renderer.ambientOcclusion(true);
    Controller controller(movement.player);
    controller.submit(map, Action::TurnLeft);
    controller.update(map, .08f);
    renderer.render(controller.state());
    movement.player = {{7, 6}, Heading::South};
    renderer.render(movement);
    const auto close = pixels(resources, renderer.ambientOcclusionTarget());
    for (std::size_t i = 0; i < close.size(); i += 4)
        check(close[i] >= 159, "Near-plane/cutout boundary AO remains finite and bounded");
}
void fogChecks(GpuResources& resources) {
    test::ArchitectureFixture fixture;
    auto publication = fixture.publication();
    publication.packs[0].daylightFog = {0, .1f, 1};
    publication.packs[0].horizon = {.2f, .4f, .8f};
    const Map map(test::architectureRows(), test::architectureWorld());
    MovementState movement;
    movement.player = {{7, 4}, Heading::South};
    WorldRenderer renderer(resources, std::move(publication));
    renderer.stage(map, movement);
    renderer.commit();
    const auto on = pixels(resources, renderer.image().target);
    const auto ao = pixels(resources, renderer.ambientOcclusionTarget());
    check(*std::min_element(ao.begin(), ao.end()) < 249, "Fog fixture exercises actual contact shading");
    renderer.ambientOcclusion(false);
    renderer.render(movement);
    check(pixels(resources, renderer.image().target) == on, "Fully fogged surfaces are unaffected by AO");
}
} // namespace
int main(int argc, char** argv) {
    try {
        projectionChecks();
        if (argc == 2 && std::string_view(argv[1]) == "--cpu-only") {
            std::cout << "AO projection checks passed without a window\n";
            return 0;
        }
        std::filesystem::path output;
        if (argc == 2) {
            output = argv[1];
            check(std::filesystem::create_directory(output), "Use new AO capture directory");
        }
        Window window(640, 360, true, true);
        check(IsWindowState(FLAG_WINDOW_HIDDEN), "AO tests use a hidden window");
        GpuResources resources;
        const auto baseline = resources.counts();
        for (int failure = 0; failure < 5; ++failure) {
            resources.failAfter(failure);
            rejects([&] { EnvironmentAo ao(resources); }, "AO resource preparation failure rejects");
            check(same(baseline, resources.counts()), "AO partial preparation releases resources");
        }
        resources.failAfter(-1);
        environmentChecks(resources, output);
        creatureChecks(resources);
        sceneryChecks(resources);
        fogChecks(resources);
        check(same(baseline, resources.counts()), "AO and view resources fully released");
        using Error = unsigned (*)();
        check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
              "AO restores valid OpenGL state");
        std::cout << "Environment AO GPU checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
