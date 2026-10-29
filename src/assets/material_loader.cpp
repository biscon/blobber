#include "assets/material_loader.hpp"
#include "assets/image.hpp"
#include <memory>
#include <stdexcept>

namespace blobber {
Material loadMaterial(const std::filesystem::path& path) {
    OwnedImage image(LoadImage(path.string().c_str()));
    if (!image.value.data)
        throw std::runtime_error("Missing or invalid texture: " + path.string());
    if (image.value.width <= 0 || image.value.height <= 0 || image.value.width > Material::maxDimension ||
        image.value.height > Material::maxDimension)
        throw std::runtime_error("Material exceeds 4096 x 4096 limit: " + path.string());
    std::unique_ptr<Color, decltype(&UnloadImageColors)> colors(LoadImageColors(image.value),
                                                                UnloadImageColors);
    if (!colors)
        throw std::runtime_error("Cannot decode texture: " + path.string());
    std::vector<Color> pixels(colors.get(), colors.get() + image.value.width * image.value.height);
    return Material(image.value.width, image.value.height, std::move(pixels));
}
Materials loadMaterials(const std::filesystem::path& directory) {
    Materials result{loadMaterial(directory / "wall.png"), loadMaterial(directory / "floor.png"),
                     loadMaterial(directory / "ceiling.png")};
    const auto grease = directory.parent_path() / "effects/grease.png";
    if (std::filesystem::exists(grease))
        result.grease = loadMaterial(grease);
    return result;
}
} // namespace blobber
