#include "assets/blueprint_loader.hpp"
#include "cave_review.hpp"
#include "platform/world_renderer.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace blobber::cave_review {
// Diagnostic path. Stage an empty actor roster on both sides so the
// measured delta describes the environments and common interactive artwork.
void biomeChange(WorldRenderer& renderer, const generation::CaveFloor& cave,
                 const std::filesystem::path& root, std::uint32_t seed) {
    auto underkeep = generation::generate({seed, 3}, blueprints::load(root / "blueprints/underkeep.json"));
    if (!underkeep.floor)
        throw std::runtime_error("Underkeep residency reference generation failed");
    auto report = [&](const char* label) {
        auto m = renderer.memory();
        std::cout << label << " textures=" << m.textureCount << " rgba=" << m.residentTextures
                  << " alpha=" << m.residentAlpha << " meshes=" << m.residentMeshes
                  << " cpu=" << renderer.cpuBytes() << " worldGpu=" << m.worldGpu
                  << " applicationGpu=" << m.applicationGpu << '\n';
        return m;
    };
    MovementState stonePose, cavePose;
    stonePose.player = underkeep.floor->layout.spawn;
    cavePose.player = {cave.sites[generation::CaveSite::Fork], Heading::East};
    const world_render::FrameInput stone{underkeep.floor->map, stonePose}, pool{cave.map, cavePose};
    renderer.stage(stone);
    renderer.commit();
    const auto initial = report("Underkeep");
    renderer.stage(pool);
    report("Underkeep+cave");
    renderer.commit();
    report("Cave");
    renderer.stage(stone);
    report("Cave+Underkeep");
    renderer.commit();
    const auto returned = report("Underkeep-return");
    if (returned.residentTextures != initial.residentTextures ||
        returned.residentAlpha != initial.residentAlpha ||
        returned.residentMeshes != initial.residentMeshes || returned.worldGpu != initial.worldGpu)
        throw std::runtime_error("Cave scenery survived the return to the Underkeep");
    renderer.clear();
}

void checkPreparedDressing(const world_assets::CpuAssets& assets, const generation::CaveFloor& cave) {
    const auto& map = cave.map;
    const auto pack = assets.pack(map.definition().assetPack);
    const auto acquired = assets.acquire(assets.requirements(pack.id));
    unsigned roofs = 0, ledges = 0, wetCells = 0, webs = 0;
    for (const auto& d : map.definition().decorations)
        webs += d.web() || d.kind == DecorationKind::Vine;
    for (const auto& s : map.definition().scenery) {
        roofs += s.clearance > 0;
        ledges += s.model == "flowstone" || s.model == "fungus-ledge";
        const bool water = s.model.compare(0, 5, "pool-") == 0;
        if (water)
            wetCells += s.width * s.depth;
        const auto& model = pack.buildings.at(pack.building(s.model).index);
        for (auto id : model.parts) {
            const auto& mesh = assets.mesh(id);
            if (assets.publication.textures.at(mesh.texture).current != water || (water && mesh.water))
                throw std::runtime_error("Cave mesh has the wrong current material");
            if (!s.clearance)
                continue;
            const auto surface = map.surface(s.anchor);
            const float ceiling = elevation(surface.ceiling) - elevation(surface.floor);
            if (mesh.cutout || mesh.water)
                throw std::runtime_error("Ceiling rock must render as opaque geometry");
            float top = 0;
            for (const auto& v : mesh.vertices) {
                top = std::max(top, v[1]);
                if (v[1] < s.clearance - .00001f || v[1] > ceiling + .00001f)
                    throw std::runtime_error("Cave ceiling cluster clips its clearance");
            }
            if (std::abs(top - ceiling) > .00001f)
                throw std::runtime_error("Floating cave ceiling cluster");
        }
    }
    std::cout << "Overhead groups=" << roofs << ", wall reliefs=" << ledges << ", wet cells=" << wetCells
              << ", silk=" << webs << ", decals=" << map.definition().decorations.size()
              << ", scenery placements=" << map.definition().scenery.size() << '\n';
}
} // namespace blobber::cave_review
