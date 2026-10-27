#pragma once
#include "render/loot_sprites.hpp"
#include <filesystem>
namespace blobber {
PreparedLoot prepareLoot(Image sheet);
void savePreparedLoot(const PreparedLoot&, const std::filesystem::path&);
PreparedLoot loadPreparedLoot(const std::filesystem::path&);
} // namespace blobber
