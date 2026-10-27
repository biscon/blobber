#pragma once
#include "game/recovery.hpp"
#include "world/map.hpp"
#include <filesystem>
namespace blobber {
RecoveryLevel loadRecoveryLevel(const std::filesystem::path&, const Map&, const rules::Catalog&);
}
