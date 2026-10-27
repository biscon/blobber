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
inline constexpr Color teal{69, 210, 207, 255};
inline constexpr Color muted{133, 141, 138, 255}, red{190, 51, 36, 255}, blue{108, 166, 188, 255};
} // namespace hudInk
// Layout remains in art units; all UI drawing targets native game pixels.
class HudPainter {
  public:
    HudPainter(const UiFonts* fonts, const UiArt* art, const UiFonts* metrics = nullptr)
        : fonts(fonts), art(art), metrics(metrics ? metrics : fonts) {}
    const UiFonts* fonts;
    const UiArt* art;
    const UiFonts* metrics;
    static constexpr float scale = float(gameWidth) / artWidth;
    static Vector2 native(Vector2 p) { return {p.x * scale, p.y * scale}; }
    static Rectangle native(Rectangle r) {
        return {r.x * scale, r.y * scale, r.width * scale, r.height * scale};
    }
    void fill(UiRect r, Color color) const {
        rectangle({float(r.x), float(r.y), float(r.width), float(r.height)}, color);
    }
    void rectangle(Rectangle r, Color color) const { DrawRectangleRec(native(r), color); }
    void border(UiRect r, Color color) const {
        DrawRectangleLinesEx(native(Rectangle{float(r.x), float(r.y), float(r.width), float(r.height)}), 1,
                             color);
    }
    void line(Vector2 a, Vector2 b, Color color, float pixels = 1) const {
        DrawLineEx(native(a), native(b), pixels, color);
    }
    void triangle(Vector2 a, Vector2 b, Vector2 c, Color color) const {
        DrawTriangle(native(a), native(b), native(c), color);
    }
    void box(UiRect r, Color color = hudInk::edge) const {
        fill(r, hudInk::panel);
        border(r, color);
        if (r.width > 16 && r.height > 16)
            border({r.x + 2, r.y + 2, r.width - 4, r.height - 4}, {29, 36, 33, 255});
    }
    void overlayBox(UiRect r, Color color = hudInk::gold) const { box(r, color); }
    void gear(UiRect r) const {
        const auto center = native(Vector2{r.x + r.width * .5f, r.y + r.height * .5f});
        constexpr float pi = 3.14159265358979323846f;
        const float radius = std::min(r.width, r.height) * scale * .36f;
        const auto point = [&](int i) {
            const float angle = float(i) * pi / 16;
            const float length = radius * ((i % 4 == 1 || i % 4 == 2) ? 1.f : .78f);
            return Vector2{center.x + std::cos(angle) * length, center.y + std::sin(angle) * length};
        };
        for (int i = 0; i < 32; ++i)
            DrawTriangle(center, point(i + 1), point(i), hudInk::gold);
        DrawCircleV(center, radius * .36f, hudInk::panel);
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
    void centered(const char* value, UiRect r, Color color = hudInk::text,
                  UiTextStyle style = UiTextStyle::Label) const {
        if (fonts)
            text(value, r.x + (r.width - fonts->width(value, style)) * .5f,
                 fonts->centeredY(value, float(r.y), float(r.height), style), color, style);
    }
    void rightAligned(const char* value, UiRect r, Color color = hudInk::text,
                      UiTextStyle style = UiTextStyle::Small) const {
        if (fonts)
            text(value, r.x + r.width - 2 - fonts->width(value, style),
                 fonts->centeredY(value, float(r.y), float(r.height), style), color, style);
    }
    void picture(unsigned index, UiRect r, Color tint = WHITE) const {
        if (art)
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
    void characterSheet(const HudSnapshot& h) const;
    void inventory(const HudSnapshot& h) const;
    void itemTargets(const HudSnapshot& h) const;
    void spellbook(const HudSnapshot& h) const;
    void spellTargets(const HudSnapshot& h) const;
    void spellMarkers(const HudSnapshot& h) const;
    void checkmark(UiRect r) const {
        fill(r, hudInk::teal);
        line({float(r.x + 1), float(r.y + r.height / 2)},
             {float(r.x + r.width / 3), float(r.y + r.height - 2)}, WHITE, 2);
        line({float(r.x + r.width / 3), float(r.y + r.height - 2)},
             {float(r.x + r.width - 2), float(r.y + 1)}, WHITE, 2);
    }
    void dart(UiRect r) const {
        line({float(r.x), float(r.y + r.height - 1)}, {float(r.x + r.width - 1), float(r.y)}, hudInk::teal,
             2);
        triangle({float(r.x + r.width - 1), float(r.y)}, {float(r.x + r.width - 4), float(r.y)},
                 {float(r.x + r.width - 1), float(r.y + 4)}, WHITE);
    }
};
} // namespace blobber
