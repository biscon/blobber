#include "ui/music_banner.hpp"
#include "ui/hud_painter.hpp"
namespace blobber {
void drawMusicBanner(const MusicBanner& banner, const UiFonts& fonts) {
    if (!banner.visible)
        return;
    const auto motion = musicBannerMotion(banner.age);
    if (motion.alpha <= 0)
        return;
    const HudPainter p(&fonts, nullptr);
    const float height = banner.secondLine[0] ? 72.f : 54.f;
    const float x = 12 + motion.offset, y = 292 - height;
    const auto color = [&](Color c) {
        c.a = static_cast<unsigned char>(c.a * motion.alpha);
        return c;
    };
    BeginScissorMode(viewX * HudPainter::scale, viewY * HudPainter::scale, viewWidth * HudPainter::scale,
                     viewHeight * HudPainter::scale);
    p.rectangle({x + 3, y + 3, 360, height}, color({0, 0, 0, 130}));
    p.rectangle({x, y, 360, height}, color({13, 20, 23, 238}));
    p.rectangle({x, y, 3, height}, color(hudInk::teal));
    p.rectangle({x + 3, y, 357, 1}, color(hudInk::gold));
    p.text("NOW PLAYING", x + 12, y + 5, color(hudInk::teal), UiTextStyle::Small);
    char first[96], second[96];
    musicBannerLetters(first, banner.title, banner.age, banner.revision);
    musicBannerLetters(second, banner.secondLine, banner.age, banner.revision + 37);
    p.clipped(first, int(x + 12), y + 15, 335, color(hudInk::text), UiTextStyle::Control);
    if (second[0])
        p.clipped(second, int(x + 12), y + 32, 335, color(hudInk::text), UiTextStyle::Control);
    p.clipped(banner.artist, int(x + 12), y + height - 13, 335, color(hudInk::gold), UiTextStyle::Small);
    // Small geometric rhythm marks, no flashing or audio-reactive allocations.
    for (unsigned i = 0; i < 4; ++i)
        p.rectangle({x + 330 + i * 5, y + 7 - float(i % 3), 2, 3 + float(i % 3)}, color(hudInk::teal));
    EndScissorMode();
}
} // namespace blobber
