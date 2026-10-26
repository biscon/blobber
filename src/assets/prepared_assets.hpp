#pragma once
#include "render/npc_sprites.hpp"
#include "render/scene_cache.hpp"
#include <filesystem>

namespace blobber {
// Publication metadata is written last. Verify it before loading either bank;
// the bank readers then verify their payloads against these footer hashes.
std::uint64_t preparedBundleChecksum(const std::filesystem::path& path);
void validatePreparedPublication(const std::filesystem::path& directory, std::uint64_t profile,
                                 std::uint64_t definitions);
std::size_t sceneStorageBytes(const PreparedScene& scene) noexcept;
void savePreparedScene(const std::filesystem::path& path, const PreparedScene& scene, std::uint64_t profile);
PreparedScene loadPreparedScene(const std::filesystem::path& path, std::uint64_t profile,
                                PreparationLimits limits = {});
void savePreparedNpcs(const std::filesystem::path& path, const PreparedNpcs& npcs,
                      const std::vector<NpcTimings>& timings, std::uint64_t definitions,
                      std::uint64_t profile);
PreparedNpcs loadPreparedNpcs(const std::filesystem::path& path, const std::vector<NpcTimings>& timings,
                              std::uint64_t definitions, std::uint64_t profile,
                              PreparationLimits limits = {});
} // namespace blobber
