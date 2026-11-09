#include "assets/blueprint_art.hpp"
#include <algorithm>
#include <cmath>

namespace blobber::blueprints {
void validateKitArt(const Kit& kit, const world_assets::PropMesh& mesh) {
    const auto fail = [&](const char* reason) {
        throw Failure(Error::Bounds, std::string(kit.id.data()) + ": " + reason);
    };
    if (!mesh.architecture || mesh.cutout || mesh.water || mesh.vertices.empty() ||
        mesh.vertices.size() > 6000 || mesh.vertices.size() % 3)
        fail("expected opaque static kit mesh, at most 2000 triangles");
    // Export coordinates are rounded to six decimal places. Permit rounding only.
    constexpr float epsilon = .00001f;
    for (const auto& v : mesh.vertices) {
        for (float f : v)
            if (!std::isfinite(f))
                fail("nonfinite vertex");
        if (v[0] < -.5f - epsilon || v[0] > float(kit.size.x) - .5f + epsilon || v[2] < -.5f - epsilon ||
            v[2] > float(kit.size.y) - .5f + epsilon || v[1] < -epsilon || v[1] > kit.height + epsilon)
            fail("mesh exceeds blueprint footprint or height");
    }
}

void validatePublishedArt(const BlueprintCatalog& catalog, const ReviewScene& scene,
                          const world_assets::Publication& publication) {
    const auto& packId = scene.map.definition().assetPack;
    const auto pack = std::find_if(publication.packs.begin(), publication.packs.end(),
                                   [&](const auto& p) { return p.id == packId; });
    if (pack == publication.packs.end())
        throw Failure(Error::Reference, "Missing blueprint art pack: " + packId);
    std::array<bool, maxKits> checked{};
    for (const auto& placement : scene.map.definition().scenery) {
        const auto id = catalog.kit(placement.model);
        if (checked[id.index])
            continue;
        checked[id.index] = true;
        const auto& kit = catalog.kits[id.index];
        const auto model = std::find_if(pack->buildings.begin(), pack->buildings.end(),
                                        [&](const auto& b) { return b.id == placement.model; });
        if (model == pack->buildings.end())
            throw Failure(Error::Reference, "Missing finished kit: " + placement.model);
        if (model->parts.size() != 1)
            throw Failure(Error::Content, placement.model + ": expected one grouped kit mesh");
        validateKitArt(kit, publication.readMesh(model->parts.front()));
    }
}
} // namespace blobber::blueprints
