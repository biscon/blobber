#include "ui/hud.hpp"
#include "raylib.h"
#include "ui/fonts.hpp"
#include "ui/layout.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
namespace blobber {
namespace {
constexpr Color ink{19, 23, 25, 255}, panel{31, 35, 35, 255}, gold{172, 139, 83, 255};
constexpr Color pale{214, 199, 164, 255}, muted{116, 120, 113, 255}, edge{72, 72, 60, 255};
constexpr Color danger{199, 110, 95, 255};
// The same layout renders geometry at art resolution and text at game resolution.
// A null font set selects the geometry pass; no default font is drawn.
class HudPainter {
  public:
    explicit HudPainter(const UiFonts* fonts) : fonts_(fonts) {}
    void drawUI(const MovementState& state, bool debug, const Map& map, const HudSnapshot* hud);
    void drawOverlay(const HudSnapshot* hud, const PickingBuffer* picking);

  private:
    const UiFonts* fonts_;
    void text(const char* value, float x, float y, Color color = pale,
              UiTextStyle style = UiTextStyle::Label) {
        if (fonts_)
            fonts_->draw(value, x, y, color, style);
    }
    void rectangle(int x, int y, int w, int h, Color color) {
        if (!fonts_)
            DrawRectangle(x, y, w, h, color);
    }
    void rectangleLines(int x, int y, int w, int h, Color color) {
        if (!fonts_)
            DrawRectangleLines(x, y, w, h, color);
    }
    void rectangleRec(Rectangle r, Color color) {
        if (!fonts_)
            DrawRectangleRec(r, color);
    }
    void circle(int x, int y, float r, Color color) {
        if (!fonts_)
            DrawCircle(x, y, r, color);
    }
    void rule(int x, int y, int x2, int y2, Color color) {
        if (!fonts_)
            DrawLine(x, y, x2, y2, color);
    }
    void buttonLabel(const char* value, UiRect r, int inset, Color color = pale,
                     UiTextStyle style = UiTextStyle::Label) {
        if (fonts_)
            clipped(value, r.x + inset, fonts_->centeredY(value, float(r.y), float(r.height), style),
                    r.width - 2 * inset, color, style);
    }
    void box(UiRect r, Color border = edge);
    void clipped(const char* value, int x, float y, int width, Color color = pale,
                 UiTextStyle style = UiTextStyle::Label);
    int wrapped(const char* value, UiRect r, Color color = pale, UiTextStyle style = UiTextStyle::Label);
    void card(const HudCard& c, int i, bool selected);
};
void HudPainter::box(UiRect r, Color border) {
    rectangle(r.x, r.y, r.width, r.height, panel);
    rectangleLines(r.x, r.y, r.width, r.height, border);
}
void HudPainter::clipped(const char* value, int x, float y, int width, Color color, UiTextStyle style) {
    if (!fonts_)
        return;
    char buffer[192];
    std::snprintf(buffer, sizeof(buffer), "%s", value);
    auto n = std::strlen(buffer);
    bool cut = false;
    while (n && fonts_->width(buffer, style) > width) {
        buffer[--n] = '\0';
        cut = true;
    }
    if (cut) {
        n = n > 3 ? n - 3 : 0;
        for (;;) {
            std::memcpy(buffer + n, "...", 4);
            if (fonts_->width(buffer, style) <= width)
                break;
            if (!n) {
                buffer[0] = '\0';
                break;
            }
            --n;
        }
    }
    text(buffer, x, y, color, style);
}
int HudPainter::wrapped(const char* value, UiRect r, Color color, UiTextStyle style) {
    if (!fonts_)
        return 0;
    const int height = fonts_->lineHeight(style);
    // Bounded stack formatting; wrapping and clipping never construct strings.
    const char* cursor = value;
    int used = 0;
    while (*cursor && used + height <= r.height) {
        char line[192]{};
        std::size_t n = 0, lastSpace = 0;
        while (cursor[n] && cursor[n] != '\n' && n + 1 < sizeof(line)) {
            line[n] = cursor[n];
            line[n + 1] = '\0';
            if (fonts_->width(line, style) > r.width) {
                line[n] = '\0';
                break;
            }
            if (cursor[n] == ' ')
                lastSpace = n;
            ++n;
        }
        if (!n && *cursor != '\n')
            break;
        if (cursor[n] && cursor[n] != '\n' && lastSpace) {
            n = lastSpace;
            line[n] = '\0';
        }
        text(line, r.x, r.y + used, color, style);
        used += height;
        cursor += n;
        while (*cursor == ' ')
            ++cursor;
        if (*cursor == '\n')
            ++cursor;
    }
    return used;
}
const char* shortReason(CommandReason r) {
    switch (r) {
    case CommandReason::PreviewAccepted:
        return "Preview only. No costs.";
    case CommandReason::Unavailable:
        return "Unavailable until later.";
    case CommandReason::MapOpen:
        return "Close map to confirm.";
    case CommandReason::CannotAct:
        return "Cannot act: demo status.";
    default:
        return commandMessage(r);
    }
}
void HudPainter::card(const HudCard& c, int i, bool selected) {
    const auto r = hudLayout::card(i);
    box(r, selected ? gold : edge);
    clipped(c.name, r.x + 6, r.y + 5, r.width - 12, selected ? gold : pale, UiTextStyle::Heading);
    constexpr Color colors[]{
        {128, 113, 85, 255}, {111, 133, 139, 255}, {112, 135, 98, 255}, {138, 113, 150, 255}};
    const Color color = c.status == DisplayStatus::Dead ? muted : colors[c.id.index % 4];
    rectangle(r.x + 6, r.y + 20, 27, 33, ink);
    circle(r.x + 19, r.y + 29, 6, color);
    rectangle(r.x + 11, r.y + 36, 17, 15, color);
    clipped(c.className, r.x + 38, r.y + 18, 111, muted, UiTextStyle::Small);
    char line[64];
    std::snprintf(line, sizeof(line), "HP %d/%d", c.hp, c.maxHp);
    text(line, r.x + 38, r.y + 29, c.hp < c.maxHp ? danger : pale);
    if (c.hasMana)
        std::snprintf(line, sizeof(line), "Mana %d/%d", c.mana, c.maxMana);
    else
        std::snprintf(line, sizeof(line), "Mana --");
    text(line, r.x + 38, r.y + 40, muted, UiTextStyle::Small);
    constexpr const char* positions[]{"FL", "FR", "BL", "BR"};
    const char* status = c.status == DisplayStatus::Ready  ? "Ready"
                         : c.status == DisplayStatus::Dead ? "Dead"
                                                           : "Unconscious";
    std::snprintf(line, sizeof(line), "%d %s / %s", i + 1, positions[i], status);
    clipped(line, r.x + 6, r.y + 55, r.width - 12, c.status == DisplayStatus::Ready ? muted : danger,
            UiTextStyle::Small);
}
void HudPainter::drawUI(const MovementState& state, bool debug, const Map& map, const HudSnapshot* hud) {
    if (!fonts_) {
        ClearBackground(ink);
        for (int y = 0; y < 360; y += 3)
            for (int x = 0; x < 640; x += 3) {
                const unsigned v =
                    static_cast<unsigned>(x * 73856093u) ^ static_cast<unsigned>(y * 19349663u);
                if ((v % 7) < 2)
                    DrawPixel(x, y, {37, 40, 38, 255});
            }
    }
    box({4, 4, 124, 280});
    box({128, 4, 508, 280});
    text("STONE & SHADOW", 12, 10, gold, UiTextStyle::Heading);
    char line[96];
    std::snprintf(line, sizeof(line), "%s  %02d:%02d", headingName(state.player.heading), state.player.cell.x,
                  state.player.cell.y);
    clipped(line, 12, 24, 108, muted, UiTextStyle::Small);
    if (hud) {
        if (!debug) {
            for (int i = 0; i < 6; ++i) {
                const auto action = static_cast<PartyAction>(i);
                const bool selected = hud->selection.action == action;
                const auto r = hudLayout::action(i);
                box(r, selected ? gold : edge);
                const auto reason = hud->actionAvailability[i];
                const bool unavailable = reason == CommandReason::Unavailable ||
                                         reason == CommandReason::BackRowOnly ||
                                         reason == CommandReason::CannotAct;
                buttonLabel(actionName(action), r, 4, unavailable ? muted : pale);
            }
            box(hudLayout::previousPage);
            buttonLabel("<", hudLayout::previousPage, 5);
            box(hudLayout::nextPage);
            buttonLabel(">", hudLayout::nextPage, 6);
            std::snprintf(line, sizeof(line), "TARGETS %zu/%zu", hud->targetPage + 1,
                          std::max(std::size_t{1}, (hud->targets.count + 3) / 4));
            buttonLabel(line, {32, 135, 69, 13}, 0, muted, UiTextStyle::Small);
            for (int i = 0; i < 4; ++i) {
                const auto r = hudLayout::target(i);
                const auto index = hud->targetPage * 4 + i;
                if (index >= hud->targets.count) {
                    if (i == 0)
                        buttonLabel("No visible targets", r, 0, muted);
                    continue;
                }
                const auto id = hud->targets.ids[index];
                const bool selected = hud->selection.target && sameActor(*hud->selection.target, id);
                box(r, selected ? gold : edge);
                std::snprintf(line, sizeof(line), "Monster #%u", unsigned(id.slot) + 1);
                buttonLabel(line, r, 3, hud->suspended ? muted : selected ? gold : pale);
            }
            box(hudLayout::confirm, hud->availability == CommandReason::PreviewAccepted ? gold : edge);
            buttonLabel("ENTER / PREVIEW", hudLayout::confirm, 6,
                        hud->availability == CommandReason::PreviewAccepted ? gold : muted);
            wrapped(shortReason(hud->availability), hudLayout::feedback,
                    hud->availability == CommandReason::PreviewAccepted ? pale : danger);
            rule(10, 248, 122, 248, edge);
            text("LOG (click / scroll)", 10, 251, gold, UiTextStyle::Log);
            clipped(hud->log.newest(hud->logOffset), 10, 264, 112, pale, UiTextStyle::Log);
        }
        for (int i = 0; i < 4; ++i)
            card(hud->cards[i], i, hud->cards[i].id.index == hud->selection.character.index);
    } else {
        text("EXPLORATION", 15, 50, gold);
        box({4, 288, 632, 68});
        text("Party HUD unavailable in this diagnostic", 16, 302);
    }
    if (debug) {
        box({8, 36, 116, 244});
        text("SURVEY / DEBUG", 15, 44, gold);
        // Fit any supported map into the sidebar without drawing over the viewport.
        const float scale = std::min(104.0f / map.width(), 184.0f / map.height());
        if (!fonts_) {
            BeginScissorMode(12, 62, 108, 188);
            for (int y = 0; y < map.height(); ++y)
                for (int x = 0; x < map.width(); ++x)
                    if (map.solid({x, y}))
                        rectangleRec({12 + x * scale, 62 + y * scale, scale, scale}, edge);
            rectangleRec({12 + state.player.cell.x * scale, 62 + state.player.cell.y * scale, scale, scale},
                         gold);
            EndScissorMode();
        }
        text("F1 CLOSE", 15, 260, gold);
    }
}
void HudPainter::drawOverlay(const HudSnapshot* hud, const PickingBuffer* picking) {
    if (!hud)
        return;
    if (!fonts_ && !hud->suspended && hud->selection.target && picking &&
        hud->selection.target->world == picking->targets().world) {
        const auto selected = hud->selection.target->slot;
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x)
                if (picking->slot(x, y) == selected &&
                    (picking->slot(x - 1, y) != selected || picking->slot(x + 1, y) != selected ||
                     picking->slot(x, y - 1) != selected || picking->slot(x, y + 1) != selected))
                    DrawPixel(viewX + x, viewY + y, gold);
    }
    if (hud->logExpanded) {
        box(hudLayout::expandedLog, gold);
        text("COMMAND LOG / wheel to scroll / click LOG to close", 162, 38, gold, UiTextStyle::Log);
        const int bottom = hudLayout::expandedLog.y + hudLayout::expandedLog.height - 10;
        int y = 57;
        for (auto offset = hud->logOffset; offset < hud->log.size(); ++offset) {
            const int used =
                wrapped(hud->log.newest(offset), {162, y, 440, bottom - y}, pale, UiTextStyle::Log);
            if (!used)
                break;
            y += used + 2;
        }
    }
    if (hud->demo) {
        rectangle(140, 8, 240, 16, ink);
        char line[64];
        std::snprintf(line, sizeof(line), "DEV UI DEMO %d / SAMPLE VALUES", hud->demoScenario + 1);
        buttonLabel(line, {140, 8, 240, 16}, 4, gold);
        box(hudLayout::demoNext, gold);
        buttonLabel("F2 NEXT", hudLayout::demoNext, 10, gold);
        box(hudLayout::demoReset, gold);
        buttonLabel("F3 RESET", hudLayout::demoReset, 10, gold);
    }
}
} // namespace
void drawUI(const MovementState& state, bool debug, const Map& map, const HudSnapshot* hud,
            const UiFonts* fonts) {
    HudPainter(fonts).drawUI(state, debug, map, hud);
}
void drawHudOverlay(const HudSnapshot* hud, const PickingBuffer* picking, const UiFonts* fonts) {
    HudPainter(fonts).drawOverlay(hud, picking);
}
} // namespace blobber
