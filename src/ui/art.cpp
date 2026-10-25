#include "ui/art.hpp"
#include "assets/image.hpp"
#include <stdexcept>
namespace blobber {
UiArt::UiArt(GpuResources& resources, const std::filesystem::path& directory) : resources_(resources) {
    constexpr const char* names[]{"alden",  "brunna", "merrin", "lethiel", "goblin", "melee",
                                  "defend", "ranged", "spell",  "item",    "aid"};
    OwnedImage atlas(GenImageColor(256, 192, BLANK));
    if (!atlas.value.data)
        throw std::runtime_error("Cannot allocate UI atlas");
    for (unsigned i = 0; i < regions_.size(); ++i) {
        const auto path = directory / (std::string(names[i]) + ".png");
        OwnedImage image(LoadImage(path.string().c_str()));
        if (!image.value.data)
            throw std::runtime_error("Missing UI art: " + path.string());
        const int w = i < 4 ? 48 : i == 4 ? 24 : 30, h = i < 4 ? 56 : i == 4 ? 24 : 28;
        ImageResize(&image.value, w, h);
        regions_[i] = {float((i % 4) * 64), float((i / 4) * 64), float(w), float(h)};
        ImageDrawImage(&atlas.value, image.value, int(regions_[i].x), int(regions_[i].y), WHITE);
    }
    atlas_ = resources_.createTexture(atlas.value);
}
void UiArt::draw(unsigned index, UiRect r, Color tint) const {
    if (index >= regions_.size()) {
        DrawRectangle(r.x, r.y, r.width, r.height, {19, 26, 25, 255});
        DrawCircle(r.x + r.width / 2, r.y + r.height / 3, float(r.width) / 5, {101, 115, 108, 255});
        DrawRectangle(r.x + r.width / 5, r.y + r.height / 2, r.width * 3 / 5, r.height * 2 / 5,
                      {101, 115, 108, 255});
        return;
    }
    DrawTexturePro(resources_.texture(atlas_), regions_[index],
                   {float(r.x), float(r.y), float(r.width), float(r.height)}, {0, 0}, 0, tint);
}
} // namespace blobber
