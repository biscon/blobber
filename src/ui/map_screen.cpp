#include "ui/hud.hpp"
#include "ui/hud_painter.hpp"
#include "ui/map_model.hpp"
#include <cmath>
namespace blobber {
using namespace hudInk;
void drawMapOverlay(const MovementState& state, const Map& map, const UiFonts* fonts,
                    const generation::DiagnosticView* generation, const HudSnapshot* hud, bool reveal,
                    const MapOverlayModel* model, const UiArt* art) {
    const HudPainter p(fonts, art);
    const auto marker = [&](MapMarker kind, Rectangle box, Heading facing = Heading::North) {
        if (art)
            art->mapMarker(kind, box, facing);
    };
    constexpr float iconInset = 1.f / HudPainter::scale;
    constexpr float iconSize = MapGridView::cellSize - 2 * iconInset;
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
    if (hud && hud->campaign && depth == 0)
        std::snprintf(title, sizeof(title), "%s", hud->locationName);
    else
        std::snprintf(title, sizeof(title), "FLOOR %u", depth);
    p.text(title, 12, 24, gold, UiTextStyle::Display);
    p.rightAligned("M CLOSE", {420, 22, 76, 16}, gold, UiTextStyle::Small);
    p.line({5, 39}, {503, 39}, edge);
    const auto canvas = hudLayout::mapCanvas;
    p.fill(canvas, {9, 15, 15, 255});
    const auto view =
        MapGridView::fit(model->bounds(), state.player.cell, canvas.x, canvas.y, canvas.width, canvas.height);
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
                marker(mapFixtureMarker(*floor, i, reveal),
                       {view.left(f.cell) + iconInset, view.top(f.cell) + iconInset, iconSize, iconSize});
        }
    if (view.contains(state.player.cell))
        marker(MapMarker::Party,
               {view.left(state.player.cell) + iconInset, view.top(state.player.cell) + iconInset, iconSize,
                iconSize},
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
        marker(markers[i], {397, float(y - 6), 12, 12});
        p.label(labels[i], {414, y - 6, 82, 12});
    }
    p.line({394, 210}, {496, 210}, edge);
    p.text("QUEST KEYS", 394, 218, gold, UiTextStyle::Display);
    int keyY = 236;
    for (unsigned key : {2u, 3u})
        if (hud && (hud->questKeys & (1u << key))) {
            marker(MapMarker::Key, {394, float(keyY - 1), 9, 9});
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
