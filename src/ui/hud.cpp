#include "ui/hud.hpp"
#include "raylib.h"
#include <array>
#include <cstdio>
namespace blobber {
namespace {
constexpr Color ink{19, 23, 25, 255}, panel{31, 35, 35, 255}, gold{172, 139, 83, 255};
constexpr Color pale{214, 199, 164, 255}, muted{116, 120, 113, 255}, edge{72, 72, 60, 255};
void text(const char* value, int x, int y, Color color = pale, int size = 10) {
    DrawText(value, x, y, size, color);
}
void centered(const char* value, int x, int y, Color color = pale, int size = 10) {
    text(value, x - MeasureText(value, size) / 2, y, color, size);
}
void box(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, ink);
    DrawRectangleLines(x, y, w, h, edge);
    DrawLine(x + 1, y + 1, x + w - 2, y + 1, {100, 91, 68, 255});
    DrawLine(x + 1, y + 1, x + 1, y + h - 2, {100, 91, 68, 255});
    DrawRectangle(x + 3, y + 3, w - 6, h - 6, panel);
    DrawRectangleLines(x + 4, y + 4, w - 8, h - 8, {23, 27, 28, 255});
}
void diamond(int x, int y, int radius, Color color) {
    DrawTriangle({float(x), float(y - radius)}, {float(x - radius), float(y)}, {float(x), float(y + radius)},
                 color);
    DrawTriangle({float(x), float(y - radius)}, {float(x), float(y + radius)}, {float(x + radius), float(y)},
                 color);
}
} // namespace
void drawUI(const MovementState& state, bool debug, const Map& map) {
    ClearBackground(ink);
    // Small deterministic stone-grain marks in the frame, kept off the viewport.
    for (int y = 0; y < 360; y += 3)
        for (int x = 0; x < 640; x += 3) {
            const unsigned v = static_cast<unsigned>(x * 73856093u) ^ static_cast<unsigned>(y * 19349663u);
            if ((v % 7) < 2)
                DrawPixel(x, y, {37, 40, 38, 255});
        }
    box(4, 4, 124, 280);
    centered("STONE", 66, 17, gold, 20);
    centered("& SHADOW", 66, 39, pale, 10);
    DrawLine(18, 58, 114, 58, edge);
    diamond(66, 58, 3, gold);
    centered("THE LOWER VAULTS", 66, 70, muted);

    const int cx = 66, cy = 122;
    DrawCircleLines(cx, cy, 29, edge);
    DrawCircleLines(cx, cy, 24, {54, 58, 54, 255});
    DrawLine(cx - 19, cy, cx + 19, cy, edge);
    DrawLine(cx, cy - 19, cx, cy + 19, edge);
    centered("N", cx, 84, gold);
    centered("S", cx, 153, muted);
    centered("W", 29, cy - 4, muted);
    centered("E", 103, cy - 4, muted);
    const Cell f = forward(state.player.heading);
    const Vector2 tip{float(cx + f.x * 21), float(cy + f.y * 21)};
    const Vector2 base1{float(cx - f.y * 6), float(cy + f.x * 6)};
    const Vector2 base2{float(cx + f.y * 6), float(cy - f.x * 6)};
    DrawTriangle(tip, base1, base2, gold);
    DrawTriangle(tip, base2, base1, gold);
    diamond(cx, cy, 3, pale);
    centered(headingName(state.player.heading), cx, 174, pale);
    DrawLine(18, 192, 114, 192, edge);
    centered("EXPLORATION", cx, 205, gold);
    centered("LEVEL 01", cx, 221, muted);
    char coordinates[32];
    std::snprintf(coordinates, sizeof(coordinates), "%02i : %02i", state.player.cell.x, state.player.cell.y);
    text(coordinates, 43, 241, pale);
    centered(state.animated ? "STEPS: ANIMATED" : "STEPS: INSTANT", cx, 263, muted);

    // Inset aperture: the actual dungeon texture is drawn over its dark center.
    box(128, 4, 508, 280);
    for (int x : {129, 632})
        for (int y : {5, 280})
            DrawRectangle(x, y, 3, 3, gold);
    box(4, 288, 632, 68);
    text("THE FORGOTTEN HALLS", 16, 299, gold);
    text(state.blocked ? "The stone will not yield." : "Cold stone. Still air. A passage ahead.", 16, 315,
         state.blocked ? Color{199, 140, 105, 255} : pale);
    text("TAB  STEP EFFECTS    F1  MAP    ESC  LEAVE", 16, 339, muted);
    DrawLine(291, 298, 291, 346, edge);
    const std::array<const char*, 6> keys{"W", "S", "A", "D", "Q", "E"};
    const std::array<const char*, 6> captions{"FWD", "BACK", "LEFT", "RIGHT", "TURN", "TURN"};
    for (int i = 0; i < 6; ++i) {
        const int x = 305 + i * 53;
        box(x, 299, 27, 24);
        centered(keys[i], x + 13, 306, pale);
        centered(captions[i], x + 13, 330, muted);
    }
    if (debug) {
        box(8, 64, 116, 199);
        text("SURVEY / DEBUG", 17, 75, gold);
        for (int y = 0; y < map.height(); ++y)
            for (int x = 0; x < map.width(); ++x)
                DrawRectangle(16 + x * 4, 96 + y * 4, 3, 3,
                              map.solid({x, y}) ? edge : Color{24, 29, 29, 255});
        const Cell p = state.player.cell;
        DrawRectangle(16 + p.x * 4, 96 + p.y * 4, 3, 3, pale);
        DrawPixel(17 + p.x * 4 + f.x * 3, 97 + p.y * 4 + f.y * 3, gold);
        text("#  WALL", 17, 207, muted);
        text("+  YOUR POSITION", 17, 222, muted);
        text("F1  CLOSE", 17, 245, gold);
    }
}
} // namespace blobber
