#pragma once
#include "game/trade.hpp"
#include <filesystem>
namespace blobber {
ShopCatalog loadShops(const std::filesystem::path&, const rules::Catalog&);
void validateShops(const DialogueCatalog&, const ShopCatalog&);
} // namespace blobber
