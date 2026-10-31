#include "assets/npc_metadata.hpp"
#include "assets/world_assets.hpp"
#include <iostream>
int main(int argc, char** argv) {
    try {
        if (argc != 2)
            return 2;
        const std::filesystem::path root = argv[1];
        const auto defs = blobber::loadNpcDefinitions(root / "npcs/level.json");
        blobber::world_assets::Writer writer(root / "prepared/world");
        std::vector<unsigned char> pixels(8 * 12 * 4, 255);
        for (const auto* name : {"wall", "floor", "ceiling"})
            writer.add(name, 8, 12, false, pixels);
        std::vector<blobber::world_assets::Appearance> appearances;
        for (unsigned i = 0; i < defs.types.size(); ++i) {
            blobber::world_assets::Appearance a;
            a.id = defs.types[i];
            a.attachments = defs.attachments[i];
            for (unsigned c = 0; c < blobber::npcAnimationCount; ++c) {
                a.clips[c].timing = defs.timings[i][c];
                for (unsigned side = 0; side < 4; ++side)
                    for (unsigned f = 0; f < defs.timings[i][c].count; ++f)
                        a.clips[c].textures[side][f] =
                            std::uint16_t(writer.add(a.id + "/" + std::to_string(c) + "/" +
                                                         std::to_string(side) + "/" + std::to_string(f),
                                                     8, 12, true, pixels));
            }
            appearances.push_back(a);
        }
        for (const auto* name : {"loot/bag", "fixture/key-bronze", "fixture/key-silver", "fixture/upstairs",
                                 "fixture/downstairs", "fixture/endpoint", "fixture/threshold"})
            writer.add(name, 8, 12, true, pixels);
        for (const auto* name : {"door", "door-locked"})
            writer.add(name, 8, 8, false, std::vector<unsigned char>(8 * 8 * 4, 255));
        for (const auto* name :
             {"arrow", "tip", "feathers", "slash", "glint", "grease", "web", "weapon", "preview", "white"})
            writer.add(std::string("effect/") + name, 8, 8, true, std::vector<unsigned char>(8 * 8 * 4, 255));
        writer.finish(appearances, defs.definitionFingerprint);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
