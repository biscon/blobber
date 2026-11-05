#pragma once
#include "support/elemental_assets.hpp"
#include "support/world_fixtures.hpp"
namespace blobber::test {
struct ResidencyFixture {
    WorldFixture storage;
    explicit ResidencyFixture(unsigned unused = 0) {
        world_assets::Writer w(storage.directory);
        std::vector<unsigned char> rgba(4 * 4 * 4, 255);
        const auto opaque = std::uint16_t(w.add("door", 4, 4, false, rgba, 0, {}, false));
        const auto cutout = std::uint16_t(w.add("cutout", 4, 4, true, rgba, 0, {}, false));
        std::vector<world_assets::Appearance> actors;
        for (unsigned i = 0; i < 2; ++i) {
            world_assets::Appearance a;
            a.id = i ? "goblin.archer" : "goblin";
            rgba[0] = static_cast<unsigned char>(40 + 20 * i);
            const auto t = std::uint16_t(w.add(a.id, 4, 4, true, rgba));
            a.clips[0].timing = {{{100, 0, 0, 0}}, 1};
            for (auto& side : a.clips[0].textures)
                side[0] = t;
            actors.push_back(a);
        }
        for (auto name :
             {"loot/bag", "fixture/key-bronze", "fixture/key-silver", "fixture/endpoint", "fixture/threshold",
              "effect/arrow", "effect/tip", "effect/feathers", "effect/slash", "effect/glint",
              "effect/grease", "effect/web", "effect/weapon", "effect/preview", "effect/white"})
            w.add(name, 4, 4, true, rgba);
        std::vector<world_assets::Pack> packs;
        std::vector<world_assets::PropMesh> meshes;
        for (unsigned i = 0; i < 2; ++i) {
            world_assets::Pack p;
            p.id = i ? "second" : "first";
            rgba[1] = static_cast<unsigned char>(50 + 80 * i);
            const auto t = std::uint16_t(w.add(p.id, 4, 4, false, rgba));
            p.materialCount = 3;
            p.materials[0] = p.materials[1] = p.materials[2] = t;
            p.door = p.lockedDoor = p.doorEdge = p.sky = opaque;
            p.crest = p.vine = cutout;
            world_assets::PropMesh m;
            m.name = p.id;
            m.texture = t;
            m.vertices = {
                {{-.2f, 0, 0, 0, 0, 1, 0, 0}}, {{.2f, 0, 0, 0, 0, 1, 1, 0}}, {{0, .4f, 0, 0, 0, 1, .5f, 1}}};
            p.models[0] = std::uint16_t(meshes.size());
            meshes.push_back(m);
            packs.push_back(p);
        }
        for (unsigned i = 0; i < unused; ++i)
            w.add("unused-" + std::to_string(i), 4, 4, true, rgba);
        elementalAssets(w);
        w.finish(actors, 0, packs, meshes);
    }
    world_assets::Publication publication() const { return world_assets::load(storage.directory); }
};
} // namespace blobber::test
