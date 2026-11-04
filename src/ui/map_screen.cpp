#include "ui/hud.hpp"
#include "ui/hud_painter.hpp"
#include "ui/map_model.hpp"
#include <cmath>
namespace blobber {
namespace {
using namespace hudInk;
// The same normalized pictograms serve padded map cells and legend samples.
void marker(const HudPainter& p, MapMarker kind, Rectangle box, Heading facing = Heading::North) {
    const auto point = [&](float x, float y) {
        return Vector2{box.x + x * box.width, box.y + y * box.height};
    };
    const Color ink = kind == MapMarker::Party || kind == MapMarker::OpenDoor ? teal : bright;
    const float stroke = std::max(1.f, box.width * HudPainter::scale * .065f);
    const auto line = [&](float x, float y, float xx, float yy, Color color) {
        p.line(point(x, y), point(xx, yy), color, stroke);
    };
    const auto triangle = [&](Vector2 a, Vector2 b, Vector2 c, Color color) {
        if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0)
            std::swap(b, c);
        p.triangle(a, b, c, color);
    };
    const auto rect = [&](float x, float y, float w, float h, Color color) {
        p.rectangle({box.x + x * box.width, box.y + y * box.height, w * box.width, h * box.height}, color);
    };
    const auto circle = [&](float x, float y, float radius, Color color) {
        DrawCircleV(HudPainter::native(point(x, y)), radius * box.width * HudPainter::scale, color);
    };
    const auto arch = [&] {
        constexpr Vector2 points[]{{.13f, .94f}, {.13f, .35f}, {.20f, .18f}, {.36f, .08f},
                                   {.64f, .08f}, {.80f, .18f}, {.87f, .35f}, {.87f, .94f}};
        for (unsigned i = 1; i < std::size(points); ++i)
            line(points[i - 1].x, points[i - 1].y, points[i].x, points[i].y, ink);
        line(.13f, .94f, .87f, .94f, ink);
    };
    switch (kind) {
    case MapMarker::None:
        break;
    case MapMarker::Party: {
        const auto f = forward(facing), r = right(facing);
        const auto arrow = [&](float x, float y) {
            return point(.5f + r.x * x + f.x * y, .5f + r.y * x + f.y * y);
        };
        const auto tip = arrow(0, .48f), left = arrow(-.35f, -.4f), rightPoint = arrow(.35f, -.4f),
                   notch = arrow(0, -.22f);
        triangle(tip, left, notch, ink);
        triangle(tip, notch, rightPoint, ink);
        break;
    }
    case MapMarker::ClosedDoor:
        arch();
        for (float x : {.32f, .5f, .68f})
            line(x, .32f, x, .9f, ink);
        circle(.72f, .62f, .05f, ink);
        break;
    case MapMarker::OpenDoor:
        line(.16f, .22f, .16f, .94f, ink);
        line(.16f, .22f, .81f, .08f, ink);
        line(.81f, .08f, .81f, .80f, ink);
        line(.81f, .80f, .16f, .94f, ink);
        line(.35f, .25f, .35f, .83f, ink);
        break;
    case MapMarker::GuardianGate:
        arch();
        for (float x : {.29f, .5f, .71f})
            line(x, .12f, x, .94f, ink);
        line(.14f, .42f, .86f, .42f, ink);
        rect(.32f, .5f, .36f, .29f, background);
        line(.32f, .5f, .68f, .5f, ink);
        line(.32f, .79f, .68f, .79f, ink);
        line(.32f, .5f, .32f, .79f, ink);
        line(.68f, .5f, .68f, .79f, ink);
        circle(.5f, .62f, .055f, ink);
        break;
    case MapMarker::Key:
        circle(.22f, .5f, .18f, ink);
        circle(.22f, .5f, .10f, background);
        line(.39f, .5f, .94f, .5f, ink);
        line(.72f, .5f, .72f, .3f, ink);
        line(.9f, .5f, .9f, .3f, ink);
        break;
    case MapMarker::Upstairs:
    case MapMarker::Downstairs: {
        const bool up = kind == MapMarker::Upstairs;
        for (int i = 0; i < 3; ++i) {
            const float y = .76f - i * .22f;
            rect(up ? .12f + i * .14f : .32f, y, .58f - i * .14f, .10f, ink);
        }
        const float x = up ? .79f : .16f;
        line(x, up ? .61f : .28f, x, up ? .15f : .85f, ink);
        const float tip = up ? .04f : .96f, base = up ? .25f : .75f;
        triangle(point(x, tip), point(x - .16f, base), point(x + .16f, base), ink);
        break;
    }
    case MapMarker::Objective:
        for (float inset : {.02f, .22f}) {
            line(.5f, inset, 1 - inset, .5f, ink);
            line(1 - inset, .5f, .5f, 1 - inset, ink);
            line(.5f, 1 - inset, inset, .5f, ink);
            line(inset, .5f, .5f, inset, ink);
        }
        break;
    }
}
} // namespace
void drawMapOverlay(const MovementState& state, const Map& map, const UiFonts* fonts,
                    const generation::DiagnosticView* generation, const HudSnapshot* hud, bool reveal,
                    const MapOverlayModel* model) {
    const HudPainter p(fonts, nullptr);
    const auto* floor = hud ? hud->floor : nullptr;
    MapOverlayModel fallback;
    if (!model) {
        fallback.prepare(map);
        fallback.refresh(floor);
        model = &fallback;
    }
    p.overlayBox(hudLayout::viewport, gold);
    char title[32];
    const auto depth =
        floor && (floor->definition().depth || (hud && hud->campaign)) ? floor->definition().depth : 1u;
    std::snprintf(title, sizeof(title), "FLOOR %u", depth);
    p.text(title, 12, 24, gold, UiTextStyle::Display);
    p.rightAligned("M CLOSE", {420, 22, 76, 16}, gold, UiTextStyle::Small);
    p.line({5, 39}, {503, 39}, edge);
    const auto canvas = hudLayout::mapCanvas;
    p.fill(canvas, {9, 15, 15, 255});
    const auto view = MapGridView::fit(map.width(), map.height(), state.player.cell, canvas.x, canvas.y,
                                       canvas.width, canvas.height);
    // A subtle background grid uses exactly the same phase as the centered map.
    const float firstX =
        view.x - std::floor((view.x - canvas.x) / MapGridView::cellSize) * MapGridView::cellSize;
    const float firstY =
        view.y - std::floor((view.y - canvas.y) / MapGridView::cellSize) * MapGridView::cellSize;
    for (float x = firstX; x < canvas.x + canvas.width; x += MapGridView::cellSize)
        p.rectangle({x, float(canvas.y), 1.f / HudPainter::scale, float(canvas.height)}, {18, 25, 24, 255});
    for (float y = firstY; y < canvas.y + canvas.height; y += MapGridView::cellSize)
        p.rectangle({float(canvas.x), y, float(canvas.width), 1.f / HudPainter::scale}, {18, 25, 24, 255});
    for (int y = 0; y < view.rows; ++y)
        for (int x = 0; x < view.columns; ++x) {
            const Cell cell{view.first.x + x, view.first.y + y};
            const auto tile = model->tile(cell, reveal);
            if (tile == MapTile::Hidden)
                continue;
            p.rectangle({view.left(cell), view.top(cell), MapGridView::cellSize - 1.f / HudPainter::scale,
                         MapGridView::cellSize - 1.f / HudPainter::scale},
                        tile == MapTile::Wall ? edge : Color{64, 79, 73, 255});
        }
    if (floor)
        for (unsigned i = 0; i < floor->definition().count; ++i) {
            const auto& f = floor->definition().entries[i];
            if (view.contains(f.cell))
                marker(p, mapFixtureMarker(*floor, i, reveal),
                       {view.left(f.cell) + .5f, view.top(f.cell) + .5f, 5, 5});
        }
    if (view.contains(state.player.cell))
        marker(p, MapMarker::Party,
               {view.left(state.player.cell) + .5f, view.top(state.player.cell) + .5f, 5, 5},
               state.player.heading);
    p.line({386, 46}, {386, 292}, edge);
    p.text("LEGEND", 394, 50, gold, UiTextStyle::Display);
    constexpr const char* labels[]{"Party", "Closed door", "Open door",   "Guardian gate",
                                   "Key",   "Stairs up",   "Stairs down", "Objective"};
    constexpr MapMarker markers[]{MapMarker::Party,        MapMarker::ClosedDoor, MapMarker::OpenDoor,
                                  MapMarker::GuardianGate, MapMarker::Key,        MapMarker::Upstairs,
                                  MapMarker::Downstairs,   MapMarker::Objective};
    for (unsigned i = 0; i < std::size(markers); ++i) {
        const int y = 70 + int(i) * 17;
        marker(p, markers[i], {398, float(y - 5), 10, 10});
        p.label(labels[i], {414, y - 6, 82, 12});
    }
    p.line({394, 210}, {496, 210}, edge);
    p.text("QUEST KEYS", 394, 218, gold, UiTextStyle::Display);
    int keyY = 236;
    for (unsigned key : {2u, 3u})
        if (hud && (hud->questKeys & (1u << key))) {
            marker(p, MapMarker::Key, {394, float(keyY), 7, 7});
            p.text(key == 2 ? "Bronze seal key" : "Silver seal key", 405, float(keyY), text,
                   UiTextStyle::Small);
            keyY += 12;
        }
    if (keyY == 236)
        p.text("None collected", 394, 236, muted);
    if (hud && hud->development) {
        p.box(hudLayout::diagnosticMap);
        p.label(reveal ? "Hide full map" : "Reveal full map", hudLayout::diagnosticMap, gold,
                UiTextStyle::Small);
    }
    (void)generation; // The player legend remains visible in diagnostic full-map mode too.
}
} // namespace blobber
