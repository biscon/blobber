#include "game/actors.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/residency_fixtures.hpp"
#include <iostream>
using namespace blobber;
int main() {
    try {
        test::ResidencyFixture fixture(40);
        auto publication = fixture.publication();
        check(publication.meshes[0].vertices.empty(), "Catalog opening retains no mesh vertices");
        world_assets::CpuAssets assets(publication);
        check(!assets.residency().reads && !assets.residency().textures, "Opening loads no masks or models");
        ActorSnapshot actors;
        actors.typeCount = 2;
        actors.count = 1;
        actors.actors[0].npc.type.index = 0;
        const auto first = assets.requirements("first", &actors, true);
        actors.actors[0].npc.type.index = 1;
        actors.actors[0].life = ActorLife::Dead;
        const auto second = assets.requirements("second", &actors, true);
        check(first.textures[2] && !first.textures[3] && !second.textures[2] && second.textures[3],
              "Requirements select present types including corpse records without renumbering");
        auto source = assets.acquire(first);
        const auto before = assets.residency();
        check(before.textures == first.textures.count() && before.meshes == 1,
              "Only source dependencies are loaded");
        check(assets.mesh(0).vertices.size() == 3, "Acquisition decodes checked model payload");
        {
            auto same = assets.acquire(first);
            check(assets.residency().reads == before.reads, "Shared CPU dependencies are read once");
            auto destination = assets.acquire(second);
            check(assets.residency().textures == (first.textures | second.textures).count(),
                  "Staging retains the union exactly once");
        }
        check(assets.residency().alphaBytes == before.alphaBytes && assets.residency().meshes == 1,
              "Cancellation releases destination-only payloads");
        for (unsigned i = 0; i < 3; ++i) {
            auto destination = assets.acquire(second);
            source = std::move(destination);
            check(assets.residency().textures == second.textures.count() && assets.masks[2].empty(),
                  "Commit releases departed artwork");
            source = assets.acquire(first);
        }
        auto invalid = first;
        invalid.textures.set(world_assets::catalogTextureCapacity - 1);
        rejects([&] { assets.acquire(invalid); }, "Invalid required ID rejects before acquisition");
        const auto reads = assets.residency().reads;
        watching = true;
        allocations = 0;
        for (unsigned i = 0; i < 32; ++i) {
            (void)assets.mesh(0);
            (void)assets.residency();
        }
        watching = false;
        check(!allocations && assets.residency().reads == reads,
              "Resident lookups allocate and read nothing");
        // Corruption in unused content does not force a full-file read at startup.
        const auto broken = publication.textures[3];
        {
            std::fstream f(publication.payload, std::ios::in | std::ios::out | std::ios::binary);
            f.seekp(std::streamoff(broken.alphaOffset));
            f.put(0);
        }
        (void)fixture.publication();
        rejects([&] { assets.acquire(second); }, "Corrupt destination alpha fails during acquisition");
        check(assets.residency().textures == first.textures.count() && !assets.masks[2].empty(),
              "Failed acquisition keeps source and releases partial destination");
        rejects([&] { publication.verify(); }, "Full offline verification detects unused corruption");
        {
            std::fstream f(publication.payload, std::ios::in | std::ios::out | std::ios::binary);
            f.seekp(std::streamoff(publication.meshes[1].offset));
            f.put(0);
        }
        rejects([&] { assets.acquire(assets.requirements("second")); },
                "Corrupt model payload rejects during preparation");
        check(assets.residency().textures == first.textures.count(), "Model failure releases partial leases");
        source = {};
        check(!assets.residency().alphaBytes && !assets.residency().meshBytes && !assets.residency().textures,
              "Last lease returns payload residency to zero");
        test::ResidencyFixture small;
        world_assets::CpuAssets smallAssets(small.publication());
        auto smallLease = smallAssets.acquire(smallAssets.requirements("first", nullptr, true));
        auto largeLease = assets.acquire(assets.requirements("first", nullptr, true));
        check(smallAssets.residency().alphaBytes == assets.residency().alphaBytes &&
                  smallAssets.residency().meshBytes == assets.residency().meshBytes &&
                  smallAssets.residency().textures == assets.residency().textures,
              "Unused catalog growth does not grow resident payloads");
        test::ResidencyFixture capacity(480);
        world_assets::CpuAssets capacityAssets(capacity.publication());
        rejects([&] { capacityAssets.acquire(world_assets::allAssets(capacityAssets.publication)); },
                "Installed catalog can exceed resident capacity; oversized acquisition rejects");
        check(!capacityAssets.residency().reads, "Capacity rejected before payload I/O");
        std::cout << "Location residency, publication and rollback checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
