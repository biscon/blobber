#pragma once
#include <filesystem>
namespace blobber {
// Offline only: reads authored sheets and writes registered frames plus frames.json.
// Loading/export may allocate; malformed registration throws with sheet context.
void prepareNpcAnimation(const std::filesystem::path& configPath);
} // namespace blobber
