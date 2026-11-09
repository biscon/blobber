#include "assets/blueprint_blockouts.hpp"
#include <algorithm>
namespace blobber::blueprints {
namespace {
void box(world_assets::PropMesh& mesh, float x, float y, float z, float w, float h, float d, float u) {
    auto quad = [&](std::array<std::array<float, 3>, 4> p, std::array<float, 3> normal) {
        for (unsigned i : {0u, 1u, 2u, 0u, 2u, 3u})
            mesh.vertices.push_back({p[i][0], p[i][1], p[i][2], normal[0], normal[1], normal[2], u, .5f});
    };
    quad({{{x, y, z}, {x, y + h, z}, {x + w, y + h, z}, {x + w, y, z}}}, {0, 0, -1});
    quad({{{x + w, y, z + d}, {x + w, y + h, z + d}, {x, y + h, z + d}, {x, y, z + d}}}, {0, 0, 1});
    quad({{{x, y, z + d}, {x, y + h, z + d}, {x, y + h, z}, {x, y, z}}}, {-1, 0, 0});
    quad({{{x + w, y, z}, {x + w, y + h, z}, {x + w, y + h, z + d}, {x + w, y, z + d}}}, {1, 0, 0});
    quad({{{x, y + h, z}, {x, y + h, z + d}, {x + w, y + h, z + d}, {x + w, y + h, z}}}, {0, 1, 0});
    quad({{{x, y, z}, {x + w, y, z}, {x + w, y, z + d}, {x, y, z + d}}}, {0, -1, 0});
}
} // namespace
void appendBlockouts(const BlueprintCatalog& c, world_assets::Writer& writer,
                     std::vector<world_assets::Pack>& packs, std::vector<world_assets::PropMesh>& meshes) {
    validate(c);
    const auto source =
        std::find_if(packs.begin(), packs.end(), [](const auto& p) { return p.id == "underkeep"; });
    if (source == packs.end())
        throw Failure(Error::Reference, "Blockouts require Underkeep terrain/door/chest bindings");
    if (packs.size() >= world_assets::packCapacity ||
        meshes.size() + c.kitCount > world_assets::meshCapacity ||
        source->materialCount >= source->materials.size())
        throw Failure(Error::Capacity, "Blockout publication exceeds existing limits");
    if (std::any_of(packs.begin(), packs.end(), [](const auto& p) { return p.id == "blueprint-blockouts"; }))
        throw Failure(Error::Content, "Duplicate blockout publication");
    auto pack = *source;
    pack.id = "blueprint-blockouts";
    pack.buildings.clear();
    // One small atlas; blockout color encodes purpose, not final art direction.
    constexpr std::array<std::array<unsigned char, 3>, 8> colors{{{174, 103, 75},
                                                                  {180, 145, 89},
                                                                  {125, 148, 91},
                                                                  {191, 153, 98},
                                                                  {101, 134, 158},
                                                                  {133, 104, 74},
                                                                  {139, 149, 165},
                                                                  {148, 122, 164}}};
    std::vector<unsigned char> rgba(64 * 8 * 4, 255);
    for (unsigned y = 0; y < 8; ++y)
        for (unsigned x = 0; x < 64; ++x)
            for (unsigned k = 0; k < 3; ++k)
                rgba[(y * 64 + x) * 4 + k] = colors[x / 8][k];
    const auto texture = std::uint16_t(writer.add("blueprint/blockout-colors", 64, 8, false, rgba));
    pack.materials[pack.materialCount++] = texture;
    for (unsigned i = 0; i < c.kitCount; ++i) {
        const auto& k = c.kits[i];
        const std::string_view id = k.id.data();
        world_assets::PropMesh mesh;
        mesh.name = "blueprint/" + std::string(id);
        mesh.architecture = true;
        mesh.texture = texture;
        const float u = (float(i % 8) + .5f) / 8.f, w = float(k.size.x), d = float(k.size.y), h = k.height;
        const auto add = [&](float x, float y, float z, float bw, float bh, float bd) {
            box(mesh, x, y, z, bw, bh, bd, u);
        };
        if (k.blocking)
            add(-.5f, 0, -.5f, w, .035f, d); // Exact visible collision extent.
        if (id == "hearth") {
            add(-.45f, .035f, .18f, w - .1f, h - .035f, .27f);
            add(-.45f, .035f, -.4f, .25f, h * .65f, .58f);
            add(w - .8f, .035f, -.4f, .25f, h * .65f, .58f);
            add(-.45f, h * .65f, -.4f, w - .1f, h * .18f, .58f);
        } else if (id == "work-surface" || id == "table-and-benches") {
            const float width = id == "work-surface" ? w - .2f : std::min(.95f, w * .48f),
                        left = id == "work-surface" ? -.4f : (w - 1.f - width) * .5f;
            add(left, h - .07f, -.35f, width, .07f, d - .3f);
            for (float x : {left, left + width - .1f})
                for (float z : {-.3f, d - .65f})
                    add(x, .035f, z, .1f, h - .105f, .1f);
            if (id == "table-and-benches") {
                add(-.43f, h * .5f, -.35f, .28f, .06f, d - .3f);
                add(w - .85f, h * .5f, -.35f, .28f, .06f, d - .3f);
                for (float x : {-.4f, w - .82f})
                    for (float z : {-.3f, d - .75f})
                        add(x, .035f, z, .12f, h * .5f - .035f, .16f);
            }
        } else if (id == "tapestry") {
            add(-.325f, .65f, .46f, .65f, h - .65f, .025f);
        } else if (id == "brazier") {
            add(-.10f, .035f, .15f, .20f, .25f, .20f);
            add(-.15f, .285f, .10f, .30f, h - .285f, .30f);
        } else if (!k.blocking) {
            add(-.15f, 0, .19f, .30f, h, .30f);
        } else if (id == "sleeping-bay") {
            add(-.4f, .08f, -.4f, .8f, .18f, d - .2f);
            add(-.4f, .26f, d - .95f, .8f, .07f, .3f);
            add(-.43f, .035f, d - .65f, .86f, h - .035f, .12f);
        } else if (id == "weapon-storage") {
            add(-.4f, .035f, .26f, w - .2f, .1f, .15f);
            add(-.4f, h - .15f, .26f, w - .2f, .1f, .15f);
            for (unsigned n = 0; n < 6; ++n)
                add(-.3f + float(n) * (w - .5f) / 6, .035f, .12f, .055f, h - .04f, .09f);
        } else if (id == "armor-display") {
            add(-.06f, .035f, -.05f, .12f, h - .035f, .1f);
            add(-.25f, h * .45f, -.18f, .5f, h * .4f, .36f);
            add(-.13f, h * .85f, -.13f, .26f, h * .15f, .26f);
        } else {
            const auto pieces = id == "provisions" ? 3u : 1u;
            for (unsigned n = 0; n < pieces; ++n)
                add(-.4f + float(n) * (w - .2f) / float(pieces), .035f, -.35f, (w - .25f) / float(pieces),
                    h - .035f, d - .3f);
        }
        for (const auto& v : mesh.vertices)
            if (v[0] < -.50001f || v[0] > w - .49999f || v[2] < -.50001f || v[2] > d - .49999f || v[1] < 0 ||
                v[1] > h + .00001f)
                throw Failure(Error::Bounds, "Blockout mesh exceeds kit contract: " + std::string(id));
        pack.buildings.push_back({std::string(id), {std::uint16_t(meshes.size())}});
        meshes.push_back(std::move(mesh));
    }
    packs.push_back(std::move(pack));
}
} // namespace blobber::blueprints
