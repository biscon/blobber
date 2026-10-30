#include "ui/hud_painter.hpp"
#include "ui/travel_layout.hpp"
namespace blobber {
void HudPainter::travel(const HudSnapshot& h) const {
    using namespace hudInk;
    const auto& t = h.travel;
    overlayBox(travelLayout::panel);
    text(t.hub ? "RETURN TO THE DUNGEON" : "RECALL TO SANCTUARY", 136, 76, gold, UiTextStyle::Display);
    char line[192];
    if (t.hub) {
        const bool fresh = t.rows[0].request.kind == TravelKind::Stairs;
        text(fresh ? "No entries unlocked yet. Enter floor 1 by stairs."
                   : "Choose an unlocked entry. Travel is free.",
             136, 98, hudInk::text, UiTextStyle::Small);
        for (unsigned i = 0; i < t.count; ++i) {
            const auto r = travelLayout::row(i);
            box(r, i == t.cursor ? bright : edge);
            std::snprintf(line, sizeof(line), "%s floor %u%s",
                          t.rows[i].request.kind == TravelKind::Stairs ? "Enter" : "Return to",
                          t.rows[i].depth, t.rows[i].request.kind == TravelKind::Stairs ? "" : " entry");
            label(line, r, i == t.cursor ? bright : hudInk::text);
        }
        text("Arrival is at the entrance, never your departure cell.", 136, 202, muted, UiTextStyle::Small);
    } else {
        std::snprintf(line, sizeof(line), "Camp kits: %u  |  Cost: 1  |  After recall: %u", t.kits,
                      t.kits ? t.kits - 1 : 0);
        text(line, 136, 106, gold, UiTextStyle::Small);
        text("Destination: Sanctuary hub", 136, 133, hudInk::text);
        text("Recovery is a separate rest at the sanctuary.", 136, 159, muted, UiTextStyle::Small);
        text("Return from the hub uses an unlocked floor entrance.", 136, 182, muted, UiTextStyle::Small);
        text("Your departure position is not a return waypoint.", 136, 202, muted, UiTextStyle::Small);
    }
    clipped(t.preview.reason, 136, 225, 366, t.preview ? teal : red, UiTextStyle::Small);
    box(travelLayout::cancel, gold);
    centered("Cancel", travelLayout::cancel, gold);
    box(travelLayout::confirm, t.preview ? bright : edge);
    centered(t.hub ? "Travel / Enter" : "Recall - 1 camp kit / Enter", travelLayout::confirm,
             t.preview ? bright : muted);
}
} // namespace blobber
