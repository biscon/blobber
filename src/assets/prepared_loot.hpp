#pragma once
#include "render/loot_sprites.hpp"
#include <filesystem>
namespace blobber {
PreparedLoot prepareLoot(Image sheet, const std::array<Image, PreparedLoot::icons - 1>& fixtures = {});
void savePreparedLoot(const PreparedLoot&, const std::filesystem::path&);
PreparedLoot loadPreparedLoot(const std::filesystem::path&);
} // namespace blobber
