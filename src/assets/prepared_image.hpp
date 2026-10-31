#pragma once
#include "assets/image.hpp"
#include "assets/preparation_file.hpp"
namespace blobber {
inline OwnedImage preparedImage(const std::filesystem::path& path) {
    const auto bytes = preparationFile(path);
    preparationPoint("Preparing artwork");
    // Decode one image on the window thread; raylib's process-wide callbacks stay there too.
    return OwnedImage(
        LoadImageFromMemory(path.extension().string().c_str(), bytes.data(), int(bytes.size())));
}
} // namespace blobber
