#pragma once
#include "render/npc_sprites.hpp"
#include <filesystem>
namespace blobber {
struct LoadedNpcs {
    NpcWorld world;
    std::vector<NpcAppearance> appearances;
    std::uint64_t definitionFingerprint = 0;
    std::size_t sourceDecodeHighWater = 0; // Managed image copies; excludes decoder internals.
};
struct LoadedNpcMetadata {
    NpcWorld world;
    std::vector<NpcTimings> timings;
    std::vector<NpcAttachments> attachments;
    std::uint64_t definitionFingerprint = 0;
};
struct NpcDefinitions {
    std::vector<std::string> types;
    std::vector<NpcTimings> timings;
    std::vector<NpcAttachments> attachments;
    std::uint64_t definitionFingerprint = 0;
};
// Definition order is shared with the baker; authored placements are not read.
NpcDefinitions loadNpcDefinitions(const std::filesystem::path&);
LoadedNpcs loadNpcs(const std::filesystem::path& levelFile, const Map& map, Cell playerSpawn);
// Reads bounded JSON only. Image paths are validated syntactically, never opened.
LoadedNpcMetadata loadNpcMetadata(const std::filesystem::path& levelFile, const Map& map, Cell playerSpawn);
} // namespace blobber
