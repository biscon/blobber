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
    constexpr float width = 180, inset = 6, textWidth = 168, edge = 1;
    const float height = banner.secondLine[0] ? 39.f : 30.f;
    const float x = viewX + 8 + motion.offset * .5f, y = viewY + 8;
    const auto color = [&](Color c) {
        c.a = static_cast<unsigned char>(c.a * motion.alpha);
        return c;
    };
    BeginScissorMode(viewX * HudPainter::scale, viewY * HudPainter::scale, viewWidth * HudPainter::scale,
                     viewHeight * HudPainter::scale);
    p.rectangle({x + edge, y + edge, width, height}, color({0, 0, 0, 130}));
    p.rectangle({x, y, width, height}, color({13, 20, 23, 238}));
    p.rectangle({x, y, edge, height}, color(hudInk::teal));
    p.rectangle({x + edge, y, width - edge, 1 / HudPainter::scale}, color(hudInk::gold));
    // Size-specific atlases draw at native size; UiFonts snaps text after motion.
    p.text("NOW PLAYING", x + inset, y + 2, color(hudInk::teal), UiTextStyle::MusicCaption);
    char first[96], second[96];
    musicBannerLetters(first, banner.title, banner.age, banner.revision);
    musicBannerLetters(second, banner.secondLine, banner.age, banner.revision + 37);
    p.clipped(first, int(x + inset), y + 9, int(textWidth), color(hudInk::text), UiTextStyle::MusicTitle);
    if (second[0])
        p.clipped(second, int(x + inset), y + 18, int(textWidth), color(hudInk::text),
                  UiTextStyle::MusicTitle);
    p.clipped(banner.artist, int(x + inset), y + height - 8, int(textWidth), color(hudInk::gold),
              UiTextStyle::MusicCaption);
    // Small geometric rhythm marks, no flashing or audio-reactive allocations.
    for (unsigned i = 0; i < 4; ++i)
        p.rectangle({x + (330 + i * 5) * .5f, y + (7 - float(i % 3)) * .5f, 1, (3 + float(i % 3)) * .5f},
                    color(hudInk::teal));
    EndScissorMode();
}
} // namespace blobber
