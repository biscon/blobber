#pragma once
#include "assets/npc_metadata.hpp"
#include "render/npc_sprites.hpp"
#include <filesystem>
namespace blobber {
struct LoadedNpcs {
    NpcWorld world;
    std::vector<NpcAppearance> appearances;
    std::uint64_t definitionFingerprint = 0;
    std::size_t sourceDecodeHighWater = 0; // Managed image copies; excludes decoder internals.
};
LoadedNpcs loadNpcs(const std::filesystem::path& levelFile, const Map& map, Cell playerSpawn);
} // namespace blobber
