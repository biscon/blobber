#include "ui/hud_painter.hpp"
#include <cstdio>
namespace blobber {
void HudPainter::enemyTargets(const HudSnapshot& h, bool popover) const {
    using namespace hudInk;
    const auto& p = h.targetPicker;
    if (!p.available || h.modal() || h.suspended)
        return;
    if (!popover) {
        const auto b = hudLayout::enemyTargetButton(h.magic.armed);
        box(b, p.open ? bright : gold);
        centered("Targets", b, gold, UiTextStyle::Small);
        return;
    }
    if (!p.open)
        return;
    const auto panel = hudLayout::enemyPicker(p.count);
    overlayBox(panel, gold);
    text("TARGETS", 14, panel.y + 4, gold, UiTextStyle::Display);
    char pages[64];
    std::snprintf(pages, sizeof(pages), "%zu / %zu  / Tab next", h.targetPage + 1,
                  (p.candidateCount + 3) / 4 ? (p.candidateCount + 3) / 4 : 1);
    clipped(pages, 165, panel.y + 5, 133, muted, UiTextStyle::Small);
    if (!p.count)
        text("No enemies in view.", 20, panel.y + 30, muted);
    for (unsigned i = 0; i < p.count; ++i) {
        const auto& row = p.rows[i];
        const auto r = hudLayout::enemyTargetRow(i, h.targetPicker.count);
        const bool hovered = h.hover.enemy && sameActor(*h.hover.enemy, row.id);
        if (row.selected || hovered)
            fill(r, {37, 45, 40, 255});
        border(r, row.selected ? bright : hovered ? gold : edge);
        clipped(row.name.data(), r.x + 4, r.y + 1, r.width - 8,
                row.reason == CommandReason::Ready ? gold : muted, UiTextStyle::Small);
        clipped(row.detail.data(), r.x + 4, r.y + 9, r.width - 8, muted, UiTextStyle::Small);
    }
    box(hudLayout::enemyPrevious, edge);
    label("Previous", hudLayout::enemyPrevious, p.candidateCount > 4 ? gold : muted, UiTextStyle::Small);
    box(hudLayout::enemyNext, edge);
    label("Next", hudLayout::enemyNext, p.candidateCount > 4 ? gold : muted, UiTextStyle::Small);
    box(hudLayout::enemyConfirm, gold);
    label(h.magic.armed                               ? "Cast [Enter]"
          : h.selection.action == PartyAction::Ranged ? "Fire [Enter]"
                                                      : "Attack [Enter]",
          hudLayout::enemyConfirm, p.canConfirm ? bright : muted, UiTextStyle::Small);
    const char* hint = h.magic.armed && !h.magic.single ? "Click/Up: add / Shift-click/Down: remove"
                                                        : "Select a target, then confirm. Esc: close";
    clipped(p.feedback[0] ? p.feedback.data() : hint, 14, 286, 284, muted, UiTextStyle::Small);
}
} // namespace blobber
