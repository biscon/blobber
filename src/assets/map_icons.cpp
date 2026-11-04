#include "assets/map_icons.hpp"
#include "render/image_sampling.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace blobber {
OwnedImage prepareMapIconAtlas(const Image& sheet) {
    if (!sheet.data || sheet.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 || sheet.width < 32 ||
        sheet.height < 16 || sheet.width > 4096 || sheet.height > 2048 || sheet.width != sheet.height * 2)
        throw std::invalid_argument("Map icons require a bounded transparent 4x2 RGBA sheet");
    OwnedImage atlas(GenImageColor(mapIconAtlasWidth, mapIconAtlasHeight, BLANK));
    if (!atlas.value.data)
        throw std::runtime_error("Cannot allocate map icon atlas");
    const auto* pixels = static_cast<const Color*>(sheet.data);
    for (int icon = 0; icon < mapIconCount; ++icon) {
        // Fractional grid boundaries permit odd-sized imagegen canvases.
        const int col = icon % mapIconColumns, row = icon / mapIconColumns;
        const int x0 = col * sheet.width / mapIconColumns, x1 = (col + 1) * sheet.width / mapIconColumns;
        const int y0 = row * sheet.height / mapIconRows, y1 = (row + 1) * sheet.height / mapIconRows;
        int left = x1, top = y1, right = x0 - 1, bottom = y0 - 1;
        bool transparent = false;
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x) {
                const auto alpha = pixels[y * sheet.width + x].a;
                transparent |= alpha == 0;
                if (alpha <= 16)
                    continue;
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = std::max(bottom, y);
            }
        if (right < left || bottom < top || !transparent)
            throw std::invalid_argument("Map icon cell must contain artwork on transparency");
        const int w = right - left + 1, h = bottom - top + 1;
        const float fit = std::min(float(mapIconSize) / w, float(mapIconSize) / h);
        const int width = std::max(1, int(std::round(w * fit))),
                  height = std::max(1, int(std::round(h * fit)));
        const int ox = col * mapIconStride + mapIconPadding, oy = row * mapIconStride + mapIconPadding;
        AreaSampler sampler(pixels + top * sheet.width + left, w, h, sheet.width, 16);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                ImageDrawPixel(&atlas.value, ox + (mapIconSize - width) / 2 + x,
                               oy + (mapIconSize - height) / 2 + y,
                               sampler.sample(double(x) * w / width, double(y) * h / height,
                                              double(x + 1) * w / width, double(y + 1) * h / height));
        // Extrude the tile boundary into its own gutter for bilinear sampling.
        for (int y = -mapIconPadding; y < mapIconSize + mapIconPadding; ++y)
            for (int x = -mapIconPadding; x < mapIconSize + mapIconPadding; ++x)
                if (x < 0 || y < 0 || x >= mapIconSize || y >= mapIconSize)
                    ImageDrawPixel(&atlas.value, ox + x, oy + y,
                                   GetImageColor(atlas.value, ox + std::clamp(x, 0, mapIconSize - 1),
                                                 oy + std::clamp(y, 0, mapIconSize - 1)));
    }
    return OwnedImage(std::exchange(atlas.value, {}));
}
} // namespace blobber
