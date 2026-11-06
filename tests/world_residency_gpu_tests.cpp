#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/residency_fixtures.hpp"
#include <iostream>
using namespace blobber;
int main() {
    try {
        test::ResidencyFixture fixture(40);
        auto assets = std::make_shared<const world_assets::CpuAssets>(fixture.publication());
        WorldDefinition a, b;
        a.assetPack = "first";
        b.assetPack = "second";
        const std::vector<std::string> rows{"#####", "#...#", "#...#", "#...#", "#####"};
        const Map first(rows, a), second(rows, b);
        MovementState state;
        state.player = {{2, 2}, Heading::North};
        Window window(640, 360, true);
        check(IsWindowState(FLAG_WINDOW_HIDDEN), "Residency verification stays hidden");
        GpuResources resources;
        const auto base = resources.counts();
        {
            WorldRenderer renderer(resources, assets);
            check(renderer.supersampling() && !renderer.memory().textureCount,
                  "Default quality unchanged; renderer construction uploads no world textures");
            renderer.stage(first, state);
            renderer.commit();
            const auto source = renderer.image().target;
            const auto before = renderer.memory();
            check(before.textureCount == 3 && before.residentTextures < before.installedTextures,
                  "Only selected environment uploads; unused biome and creature art stay absent");
            for (int failure : {0, 1, 2}) {
                const auto transfers = resources.transfers();
                resources.failAfter(failure);
                rejects([&] { renderer.stage(second, state); }, "Destination failure injected");
                resources.failAfter(-1);
                check(renderer.image().target.owner == source.owner &&
                          renderer.image().target.slot == source.slot &&
                          renderer.image().target.generation == source.generation && !renderer.staged(),
                      "Source survives failed staging");
                check(renderer.memory().textureCount == before.textureCount &&
                          renderer.memory().applicationGpu == before.applicationGpu,
                      "Failed destination releases every partial allocation");
                check(resources.transfers().uploads - transfers.uploads <= 2,
                      "Failure does not reupload shared artwork");
                (void)renderer.render(state);
            }
            auto checkpoint = resources.transfers();
            renderer.stage(second, state);
            check(renderer.memory().textureCount == 4 &&
                      resources.transfers().uploads - checkpoint.uploads == 2,
                  "Transition uploads one new texture and one scene mesh; shared artwork reused");
            renderer.cancel();
            check(renderer.memory().applicationGpu == before.applicationGpu,
                  "Cancel returns to source residency");
            for (unsigned i = 0; i < 3; ++i) {
                renderer.stage(second, state);
                renderer.commit();
                check(renderer.memory().textureCount == 3 &&
                          renderer.memory().applicationGpu == before.applicationGpu,
                      "Commit unloads departed biome with no retained cache");
                renderer.stage(first, state);
                renderer.commit();
            }
            checkpoint = resources.transfers();
            const auto reads = assets->residency().reads;
            watching = true;
            allocations = 0;
            for (unsigned i = 0; i < 8; ++i) {
                state.player.heading = Heading(i % 4);
                renderer.render(state);
            }
            watching = false;
            check(!allocations && assets->residency().reads == reads &&
                      resources.transfers().uploads == checkpoint.uploads,
                  "Frames perform no allocations, asset I/O or resource uploads");
            const auto destinationId = assets->publication.packs[1].materials[0];
            {
                std::fstream payload(assets->publication.payload,
                                     std::ios::in | std::ios::out | std::ios::binary);
                payload.seekp(std::streamoff(assets->publication.textures[destinationId].offset));
                payload.put(1);
            }
            rejects([&] { renderer.stage(second, state); }, "Corrupt destination color fails before upload");
            check(renderer.memory().applicationGpu == before.applicationGpu && !renderer.staged(),
                  "Corrupt color leaves active GPU residency intact");
            renderer.render(state);
            renderer.clear();
            check(!renderer.memory().textureCount && !assets->residency().alphaBytes &&
                      !assets->residency().meshBytes,
                  "Clearing world releases all location payloads");
        }
        check(resources.counts().bytes == base.bytes && !resources.counts().textures,
              "Renderer shutdown releases all GPU resources");
        {
            WorldRenderer native(resources, assets, false);
            check(!native.supersampledTarget(), "Explicit native mode does not allocate high scratch");
            native.supersampling(true);
            check(bool(native.supersampledTarget()), "Explicit quality preparation creates high scratch");
        }
        std::cout << "Hidden GPU residency, sharing, rollback and frame checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
