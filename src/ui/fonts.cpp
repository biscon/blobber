#include "ui/fonts.hpp"
#include "render/layout.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace blobber {
namespace {
constexpr float textScale = float(gameWidth) / artWidth;
static_assert(gameWidth / artWidth == gameHeight / artHeight);
} // namespace
UiFonts::SizedFont::SizedFont(const std::filesystem::path& path, int pixels) {
    value = LoadFontEx(path.string().c_str(), pixels, nullptr, 0);
    // LoadFontEx falls back to the shared default on failure; never own or accept that fallback.
    if (value.texture.id == GetFontDefault().texture.id)
        throw std::runtime_error("Cannot load UI font: " + path.string());
    if (!IsFontValid(value) || !value.texture.id || value.baseSize != pixels) {
        UnloadFont(value);
        throw std::runtime_error("Invalid UI font: " + path.string());
    }
    SetTextureFilter(value.texture, TEXTURE_FILTER_POINT);
}
UiFonts::SizedFont::~SizedFont() {
    UnloadFont(value);
}
UiFonts::UiFonts(const std::filesystem::path& directory)
    : label_(directory / "IBMPlexSans-Regular.ttf", 24), small_(directory / "IBMPlexSans-Regular.ttf", 21),
      heading_(directory / "IBMPlexSans-Bold.ttf", 27), log_(directory / "Inconsolata.otf", 24),
      display_(directory / "DejaVuSerif-Bold.ttf", 27), control_(directory / "IBMPlexSans-Bold.ttf", 48) {}
const Font& UiFonts::font(UiTextStyle style) const {
    switch (style) {
    case UiTextStyle::Control:
        return control_.value;
    case UiTextStyle::Display:
        return display_.value;
    case UiTextStyle::Label:
        return label_.value;
    case UiTextStyle::Small:
        return small_.value;
    case UiTextStyle::Heading:
        return heading_.value;
    case UiTextStyle::Log:
        return log_.value;
    }
    return label_.value;
}
void UiFonts::draw(const char* text, float x, float y, Color color, UiTextStyle style) const {
    const auto& face = font(style);
    DrawTextEx(face, text, {std::round(x * textScale), std::round(y * textScale)}, float(face.baseSize), 1.0f,
               color);
}
float UiFonts::width(const char* text, UiTextStyle style) const {
    const auto& face = font(style);
    return MeasureTextEx(face, text, float(face.baseSize), 1.0f).x / textScale;
}
float UiFonts::centeredY(const char* text, float top, float height, UiTextStyle style) const {
    const auto& face = font(style);
    float inkTop = std::numeric_limits<float>::infinity(), inkBottom = 0;
    // HUD labels follow the catalog's printable-ASCII contract and are single-line.
    for (const auto* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
        if (*p <= ' ')
            continue;
        const int i = GetGlyphIndex(face, *p);
        if (face.recs[i].height <= 0)
            continue;
        inkTop = std::min(inkTop, float(face.glyphs[i].offsetY));
        inkBottom = std::max(inkBottom, face.glyphs[i].offsetY + face.recs[i].height);
    }
    if (!std::isfinite(inkTop))
        return top;
    return top + height * 0.5f - (inkTop + inkBottom) / (2 * textScale);
}
int UiFonts::lineHeight(UiTextStyle style) const {
    return static_cast<int>(std::ceil(font(style).baseSize / textScale)) + 2;
}
} // namespace blobber
