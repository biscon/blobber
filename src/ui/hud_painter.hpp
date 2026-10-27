#pragma once
#include "game/commands.hpp"
#include "ui/art.hpp"
#include "ui/fonts.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
namespace blobber {
namespace hudInk {
inline constexpr Color background{12, 18, 18, 255}, panel{17, 24, 24, 255}, edge{96, 83, 57, 255};
inline constexpr Color gold{204, 166, 94, 255}, bright{244, 203, 103, 255}, text{220, 216, 199, 255};
inline constexpr Color muted{133, 141, 138, 255}, red{190, 51, 36, 255}, blue{108, 166, 188, 255};
} // namespace hudInk
// Geometry and artwork render on the art target; fonts render on the native text target.
class HudPainter {
  public:
    HudPainter(const UiFonts* fonts, const UiArt* art, const UiFonts* metrics = nullptr)
        : fonts(fonts), art(art), metrics(metrics ? metrics : fonts) {}
    const UiFonts* fonts;
    const UiArt* art;
    const UiFonts* metrics;
    void fill(UiRect r, Color color) const {
        if (!fonts)
            DrawRectangle(r.x, r.y, r.width, r.height, color);
    }
    void border(UiRect r, Color color) const {
        if (!fonts)
            DrawRectangleLines(r.x, r.y, r.width, r.height, color);
    }
    void box(UiRect r, Color color = hudInk::edge) const {
        fill(r, hudInk::panel);
        border(r, color);
        if (r.width > 16 && r.height > 16)
            border({r.x + 2, r.y + 2, r.width - 4, r.height - 4}, {29, 36, 33, 255});
    }
    void overlayBox(UiRect r, Color color = hudInk::gold) const {
        box(r, color);
        if (fonts) {
            constexpr int scale = gameWidth / artWidth;
            BeginScissorMode(r.x * scale, r.y * scale, r.width * scale, r.height * scale);
            ClearBackground(BLANK);
            EndScissorMode();
        }
    }
    void text(const char* value, float x, float y, Color color = hudInk::text,
              UiTextStyle style = UiTextStyle::Label) const {
        if (fonts)
            fonts->draw(value, x, y, color, style);
    }
    void clipped(const char* value, int x, float y, int width, Color color = hudInk::text,
                 UiTextStyle style = UiTextStyle::Label) const {
        if (!fonts)
            return;
        char copy[192];
        std::snprintf(copy, sizeof(copy), "%s", value);
        auto n = std::strlen(copy);
        if (fonts->width(copy, style) > width) {
            while (n && fonts->width(copy, style) > width - fonts->width("...", style))
                copy[--n] = '\0';
            if (n + 3 < sizeof(copy))
                std::memcpy(copy + n, "...", 4);
        }
        text(copy, x, y, color, style);
    }
    void label(const char* value, UiRect r, Color color = hudInk::text,
               UiTextStyle style = UiTextStyle::Label) const {
        if (fonts)
            clipped(value, r.x + 4, fonts->centeredY(value, float(r.y), float(r.height), style), r.width - 8,
                    color, style);
    }
    void picture(unsigned index, UiRect r, Color tint = WHITE) const {
        if (!fonts && art)
            art->draw(index, r, tint);
    }
    void health(UiRect r, float ratio) const {
        fill(r, {38, 21, 19, 255});
        fill({r.x + 1, r.y + 1, int((r.width - 2) * std::clamp(ratio, 0.0f, 1.0f)), r.height - 2},
             hudInk::red);
        border(r, hudInk::edge);
    }
    void card(const HudCard& c, int i, bool aidTarget, bool inspected) const;
    void log(const HudSnapshot& h, bool expanded) const;
    void tooltip(const HudSnapshot& h) const;
    void details(const HudSnapshot& h) const;
    void spellbook(const HudSnapshot& h) const;
    void spellTargets(const HudSnapshot& h) const;
};
} // namespace blobber
