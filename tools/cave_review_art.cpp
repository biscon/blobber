#include "cave_review.hpp"
#include "generation/cave_pool.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace blobber::cave_review {
namespace {
std::uint16_t bindCurrent(world_assets::Publication& art, const std::vector<unsigned char>& source) {
    using namespace world_assets;
    constexpr unsigned side = WaterCurrent::tileSize;
    if (source.size() != waterSourceSize * waterSourceSize * 4 ||
        art.textures.size() >= catalogTextureCapacity || art.current.texture != UINT16_MAX)
        throw std::runtime_error("Invalid cave water material");
    // In the overlap, transition from the continuation of the far edge back to
    // the near edge. Both the sample and its slope agree at each periodic join.
    auto weight = [](unsigned i) {
        const float t = std::min(float(i) / waterOverlap, 1.f);
        return 1.f - t * t * (3.f - 2.f * t);
    };
    std::vector<unsigned char> tile(side * side * 4, 255);
    for (unsigned y = 0; y < side; ++y)
        for (unsigned x = 0; x < side; ++x)
            for (unsigned c = 0; c < 3; ++c) {
                auto row = [&](unsigned sy) {
                    const auto i = (sy * waterSourceSize + x) * 4 + c;
                    return (1.f - weight(x)) * source[i] +
                           (x < waterOverlap ? weight(x) * source[i + side * 4] : 0.f);
                };
                const float value =
                    (1.f - weight(y)) * row(y) + (y < waterOverlap ? weight(y) * row(y + side) : 0.f);
                tile[(y * side + x) * 4 + c] = static_cast<unsigned char>(std::lround(value));
            }
    std::vector<unsigned char> alpha;
    art.current.pixels = preparePixels(tile, side, side, 0, false, alpha);
    art.current.texture = static_cast<std::uint16_t>(art.textures.size());
    Texture t;
    t.name = "cave/water-current";
    t.width = t.height = side;
    t.canvas = {side, side, 0, 0};
    t.levels = mipLevels(side, side);
    t.bytes = art.current.pixels.size();
    art.textureBytes += t.bytes;
    art.textures.push_back(std::move(t));
    art.anchors.emplace_back();
    return art.current.texture;
}
} // namespace
void bindBlockouts(world_assets::Publication& publication, const std::vector<unsigned char>& waterRgba) {
    using namespace world_assets;
    const auto source = std::find_if(publication.packs.begin(), publication.packs.end(),
                                     [](const Pack& p) { return p.id == "underkeep"; });
    if (source == publication.packs.end())
        throw std::runtime_error("Missing prepared terrain for the cave review");
    Pack pack = *source;
    pack.id = "cave-blockout";
    const auto waterTexture = bindCurrent(publication, waterRgba);
    pack.materials.at(pack.materialCount++) = waterTexture;
    auto strip = [&](const char* id, float halfWidth, float halfDepth, float height, bool pool) {
        PropMesh m;
        m.name = std::string("cave/") + id;
        m.architecture = true;
        m.texture = pool ? waterTexture : pack.materials[0];
        for (unsigned i : {0u, 1u, 2u, 0u, 2u, 3u}) {
            const float px = i >= 2 ? halfWidth : -halfWidth;
            const float pz = i == 1 || i == 2 ? halfDepth : -halfDepth;
            m.vertices.push_back({px, height, pz, 0, 1, 0, float(i >= 2), float(pz > 0)});
        }
        pack.buildings.push_back({id, {std::uint16_t(publication.meshes.size())}});
        publication.meshes.push_back(std::move(m));
    };
    for (const auto& patch : generation::cavePool)
        strip(patch.model, .5f * patch.width, .5f, .18f, true);
    strip("survey-rope", .5f, .04f, .012f, false);
    // Fit the existing sack to one blocking cell. Its prepared pivot is against a
    // wall, so recenter its actual painted bounds before enlarging the proxy.
    const auto bag = pack.buildings.at(pack.building("grain-sack").index);
    auto clutch = publication.readMesh(bag.parts.at(0));
    float minX = 4, maxX = -4, minZ = 4, maxZ = -4;
    for (const auto& v : clutch.vertices) {
        minX = std::min(minX, v[0]);
        maxX = std::max(maxX, v[0]);
        minZ = std::min(minZ, v[2]);
        maxZ = std::max(maxZ, v[2]);
    }
    const float span = std::max(maxX - minX, maxZ - minZ);
    if (span <= 0)
        throw std::runtime_error("Empty clutch proxy footprint");
    const float scale = .88f / span;
    for (auto& v : clutch.vertices) {
        v[0] = (v[0] - (minX + maxX) * .5f) * scale;
        v[1] *= scale;
        v[2] = (v[2] - (minZ + maxZ) * .5f) * scale;
    }
    clutch.name = "cave/clutch-blockout";
    pack.buildings.push_back({"clutch-blockout", {std::uint16_t(publication.meshes.size())}});
    publication.meshes.push_back(std::move(clutch));
    publication.packs.push_back(std::move(pack));
}
} // namespace blobber::cave_review
