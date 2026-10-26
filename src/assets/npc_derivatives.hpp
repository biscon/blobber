#pragma once
#include <filesystem>
namespace blobber {
// Offline only. Definitions remain byte-identical; their image paths resolve to
// profile-C derivatives. Returns a generated level file for the source loader.
std::filesystem::path prepareNpcDerivatives(const std::filesystem::path& level,
                                            const std::filesystem::path& destination,
                                            std::size_t* loadingHighWater = nullptr);
} // namespace blobber
