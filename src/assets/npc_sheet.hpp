#pragma once
#include "render/image_sampling.hpp"
#include <filesystem>
namespace blobber {
struct NpcSheetOptions {
    ImageSampling sampling = ImageSampling::Nearest;
    double resolution = 1;
    std::filesystem::path outputDirectory;   // Empty retains the legacy destination.
    std::size_t* loadingHighWater = nullptr; // Optional offline managed image/scratch diagnostic.
};
// Offline only: reads authored sheets and writes registered frames plus frames.json.
// Loading/export may allocate; malformed registration throws with sheet context.
void prepareNpcAnimation(const std::filesystem::path& configPath, const NpcSheetOptions& options = {});
} // namespace blobber
