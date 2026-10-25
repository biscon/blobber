#include "ui/hud.hpp"
#include "raylib.h"
#include "ui/layout.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
namespace blobber {
namespace {
constexpr Color ink{19, 23, 25, 255}, panel{31, 35, 35, 255}, gold{172, 139, 83, 255};
constexpr Color pale{214, 199, 164, 255}, muted{116, 120, 113, 255}, edge{72, 72, 60, 255};
constexpr Color danger{199, 110, 95, 255};
void text(const char* value, int x, int y, Color color = pale) {
    DrawText(value, x, y, 10, color);
}
void box(UiRect r, Color border = edge) {
    DrawRectangle(r.x, r.y, r.width, r.height, panel);
    DrawRectangleLines(r.x, r.y, r.width, r.height, border);
}
void clipped(const char* value, int x, int y, int width, Color color = pale) {
    char buffer[192];
    std::snprintf(buffer, sizeof(buffer), "%s", value);
    auto n = std::strlen(buffer);
    bool cut = false;
    while (n && MeasureText(buffer, 10) > width) {
        buffer[--n] = '\0';
        cut = true;
    }
    if (cut && n >= 3)
        buffer[n - 1] = buffer[n - 2] = buffer[n - 3] = '.';
    text(buffer, x, y, color);
}
void wrapped(const char* value, UiRect r, Color color = pale) {
    // Bounded stack formatting; wrapping and clipping never construct strings.
    const char* cursor = value;
    for (int y = r.y; *cursor && y + 10 <= r.y + r.height; y += 11) {
        char line[192]{};
        std::size_t n = 0, lastSpace = 0;
        while (cursor[n] && n + 1 < sizeof(line)) {
            line[n] = cursor[n];
            line[n + 1] = '\0';
            if (MeasureText(line, 10) > r.width) {
                line[n] = '\0';
                break;
            }
            if (cursor[n] == ' ')
                lastSpace = n;
            ++n;
        }
        if (!n)
            break;
        if (cursor[n] && lastSpace) {
            n = lastSpace;
            line[n] = '\0';
        }
        text(line, r.x, y, color);
        cursor += n;
        while (*cursor == ' ')
            ++cursor;
    }
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
void card(const HudCard& c, int i, bool selected) {
    const auto r = hudLayout::card(i);
    box(r, selected ? gold : edge);
    clipped(c.name, r.x + 6, r.y + 5, r.width - 12, selected ? gold : pale);
    constexpr Color colors[]{
        {128, 113, 85, 255}, {111, 133, 139, 255}, {112, 135, 98, 255}, {138, 113, 150, 255}};
    const Color color = c.status == DisplayStatus::Dead ? muted : colors[c.id.index % 4];
    DrawRectangle(r.x + 6, r.y + 20, 27, 33, ink);
    DrawCircle(r.x + 19, r.y + 29, 6, color);
    DrawRectangle(r.x + 11, r.y + 36, 17, 15, color);
    clipped(c.className, r.x + 38, r.y + 18, 111, muted);
    char line[64];
    std::snprintf(line, sizeof(line), "HP %d/%d", c.hp, c.maxHp);
    text(line, r.x + 38, r.y + 29, c.hp < c.maxHp ? danger : pale);
    if (c.hasMana)
        std::snprintf(line, sizeof(line), "Mana %d/%d", c.mana, c.maxMana);
    else
        std::snprintf(line, sizeof(line), "Mana --");
    text(line, r.x + 38, r.y + 40, muted);
    constexpr const char* positions[]{"FL", "FR", "BL", "BR"};
    const char* status = c.status == DisplayStatus::Ready  ? "Ready"
                         : c.status == DisplayStatus::Dead ? "Dead"
                                                           : "Unconscious";
    std::snprintf(line, sizeof(line), "%d %s / %s", i + 1, positions[i], status);
    clipped(line, r.x + 6, r.y + 55, r.width - 12, c.status == DisplayStatus::Ready ? muted : danger);
}
} // namespace
void drawUI(const MovementState& state, bool debug, const Map& map, const HudSnapshot* hud) {
    ClearBackground(ink);
    for (int y = 0; y < 360; y += 3)
        for (int x = 0; x < 640; x += 3) {
            const unsigned v = static_cast<unsigned>(x * 73856093u) ^ static_cast<unsigned>(y * 19349663u);
            if ((v % 7) < 2)
                DrawPixel(x, y, {37, 40, 38, 255});
        }
    box({4, 4, 124, 280});
    box({128, 4, 508, 280});
    text("STONE & SHADOW", 12, 10, gold);
    char line[96];
    std::snprintf(line, sizeof(line), "%s  %02d:%02d", headingName(state.player.heading), state.player.cell.x,
                  state.player.cell.y);
    clipped(line, 12, 24, 108, muted);
    if (hud) {
        for (int i = 0; i < 6; ++i) {
            const auto action = static_cast<PartyAction>(i);
            const bool selected = hud->selection.action == action;
            const auto r = hudLayout::action(i);
            box(r, selected ? gold : edge);
            const auto reason = hud->actionAvailability[i];
            const bool unavailable = reason == CommandReason::Unavailable ||
                                     reason == CommandReason::BackRowOnly ||
                                     reason == CommandReason::CannotAct;
            clipped(actionName(action), r.x + 4, r.y + 2, r.width - 8, unavailable ? muted : pale);
        }
        box(hudLayout::previousPage);
        text("<", 15, 137);
        box(hudLayout::nextPage);
        text(">", 110, 137);
        std::snprintf(line, sizeof(line), "TARGETS %zu/%zu", hud->targetPage + 1,
                      std::max(std::size_t{1}, (hud->targets.count + 3) / 4));
        clipped(line, 32, 137, 69, muted);
        for (int i = 0; i < 4; ++i) {
            const auto r = hudLayout::target(i);
            const auto index = hud->targetPage * 4 + i;
            if (index >= hud->targets.count) {
                if (i == 0)
                    text("No visible targets", r.x, r.y + 1, muted);
                continue;
            }
            const auto id = hud->targets.ids[index];
            const bool selected = hud->selection.target && sameActor(*hud->selection.target, id);
            box(r, selected ? gold : edge);
            std::snprintf(line, sizeof(line), "Monster #%u", unsigned(id.slot) + 1);
            text(line, r.x + 3, r.y + 1, hud->suspended ? muted : selected ? gold : pale);
        }
        box(hudLayout::confirm, hud->availability == CommandReason::PreviewAccepted ? gold : edge);
        text("ENTER / PREVIEW", 16, 207, hud->availability == CommandReason::PreviewAccepted ? gold : muted);
        wrapped(shortReason(hud->availability), hudLayout::feedback,
                hud->availability == CommandReason::PreviewAccepted ? pale : danger);
        DrawLine(10, 248, 122, 248, edge);
        text("LOG (click / scroll)", 10, 251, gold);
        clipped(hud->log.newest(hud->logOffset), 10, 264, 112);
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
        BeginScissorMode(12, 62, 108, 188);
        for (int y = 0; y < map.height(); ++y)
            for (int x = 0; x < map.width(); ++x)
                if (map.solid({x, y}))
                    DrawRectangleRec({12 + x * scale, 62 + y * scale, scale, scale}, edge);
        DrawRectangleRec({12 + state.player.cell.x * scale, 62 + state.player.cell.y * scale, scale, scale},
                         gold);
        EndScissorMode();
        text("F1 CLOSE", 15, 260, gold);
    }
}
void drawHudOverlay(const HudSnapshot* hud, const PickingBuffer* picking) {
    if (!hud)
        return;
    if (!hud->suspended && hud->selection.target && picking &&
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
        text("COMMAND LOG / wheel to scroll / click LOG to close", 162, 38, gold);
        for (int i = 0; i < 5; ++i) {
            const auto offset = hud->logOffset + i;
            if (offset >= hud->log.size())
                break;
            wrapped(hud->log.newest(offset), {162, 57 + i * 39, 440, 35});
        }
    }
    if (hud->demo) {
        DrawRectangle(140, 8, 240, 16, ink);
        char line[64];
        std::snprintf(line, sizeof(line), "DEV UI DEMO %d / SAMPLE VALUES", hud->demoScenario + 1);
        clipped(line, 144, 11, 230, gold);
        box(hudLayout::demoNext, gold);
        text("F2 NEXT", 400, 11, gold);
        box(hudLayout::demoReset, gold);
        text("F3 RESET", 515, 11, gold);
    }
}
} // namespace blobber
