#pragma once
#include "render/npc_sprites.hpp"
#include <filesystem>
namespace blobber {
struct LoadedNpcs {
    NpcWorld world;
    std::vector<NpcAppearance> appearances;
};
LoadedNpcs loadNpcs(const std::filesystem::path& levelFile, const Map& map, Cell playerSpawn);
} // namespace blobber
