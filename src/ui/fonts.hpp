#pragma once
#include "raylib.h"
#include <filesystem>

namespace blobber {
enum class UiTextStyle { Label, Small, Heading, Log, Display };
// Fixed, size-specific atlases. Construct with a live window; destroy before it closes.
// Coordinates/measurements use art units, but glyphs draw at native game-target resolution.
class UiFonts {
  public:
    explicit UiFonts(const std::filesystem::path& directory);
    UiFonts(const UiFonts&) = delete;
    UiFonts& operator=(const UiFonts&) = delete;
    void draw(const char* text, float x, float y, Color color, UiTextStyle style) const;
    float width(const char* text, UiTextStyle style) const;
    int lineHeight(UiTextStyle style) const;
    float centeredY(const char* text, float top, float height, UiTextStyle style) const;

  private:
    struct SizedFont {
        SizedFont(const std::filesystem::path& path, int pixels);
        ~SizedFont();
        SizedFont(const SizedFont&) = delete;
        SizedFont& operator=(const SizedFont&) = delete;
        Font value{};
    };
    const Font& font(UiTextStyle style) const;
    SizedFont label_, small_, heading_, log_, display_;
};
} // namespace blobber
