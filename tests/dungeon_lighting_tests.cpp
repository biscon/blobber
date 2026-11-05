#include "render/dungeon_lighting.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include <cmath>
#include <iostream>
using namespace blobber::world_render;
int main() try {
    for (auto preset : {DungeonLighting::Current, DungeonLighting::Gentle, DungeonLighting::Strong,
                        DungeonLighting::Early}) {
        for (const char* pack : {"dungeon", "sanctuary-city", "sanctuary-inn", "future-biome"})
            check(effectiveDungeonLighting(preset, pack) == DungeonLighting::Current,
                  "All other packs retain current lighting");
        check(effectiveDungeonLighting(preset, "underkeep") == preset, "Stone biome opts into selection");
        float previous = -1;
        for (float depth : {0.f, .025f, .9f, 1.999f, 2.f, 2.001f, 4.f, 8.f, 16.f}) {
            const float lit = dungeonLightingDepth(depth, preset);
            check(std::isfinite(lit) && lit >= depth && lit > previous,
                  "Attenuation distance stays finite, monotonic and never brightens");
            if (depth <= 2 && preset != DungeonLighting::Early)
                check(lit == depth, "Protected near field includes exact two-cell boundary");
            if (depth > 0 && preset == DungeonLighting::Early)
                check(lit > depth, "Early variant darkens even close to the camera");
            previous = lit;
        }
    }
    check(dungeonLightingDepth(6, DungeonLighting::Gentle) == 8 &&
              dungeonLightingDepth(6, DungeonLighting::Strong) == 10 &&
              dungeonLightingDepth(6, DungeonLighting::Early) == 9,
          "Trial curves retain their agreed six-cell comparison values");
    for (auto preset : {DungeonLighting::Gentle, DungeonLighting::Strong})
        check(dungeonLightingDepth(2.001f, preset) - dungeonLightingDepth(1.999f, preset) < .0041f,
              "Near-field transition is continuous");
    allocations = 0;
    watching = true;
    const auto selected = effectiveDungeonLighting(DungeonLighting::Strong, "underkeep");
    const auto depth = dungeonLightingDepth(6, selected);
    watching = false;
    check(!allocations && depth == 10, "Profile resolution requires no allocation");
    std::cout << "Dungeon lighting CPU checks passed\n";
} catch (const std::exception& e) {
    watching = false;
    std::cerr << e.what() << '\n';
    return 1;
}
