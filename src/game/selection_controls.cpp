#include "game/creator.hpp"
#include <algorithm>
#include <cmath>
namespace blobber {
UiRect selectionThumb(UiRect track, unsigned count, unsigned page, unsigned offset) noexcept {
    if (count <= page)
        return track;
    const int height = std::max(8, int(track.height * page / count));
    const int top =
        track.y +
        int(std::lround(float(std::min(offset, count - page)) / (count - page) * (track.height - height)));
    return {track.x, top, track.width, height};
}
void layoutSelection(FlowControls& w, const rules::SpellSelection& s) noexcept {
    w.add({174, 79, 266, 18}, FlowAction::Search);
    for (std::size_t j = 0; j < 6 && j + s.offset < s.count; ++j) {
        const auto id = s.visible[j + s.offset];
        const UiRect r{174 + int(j % 2) * 137, 109 + int(j / 2) * 53, 129, 47};
        w.add(r, s.context.locked & rules::spellBit(id) ? FlowAction::Inspect : FlowAction::Toggle, id.index);
    }
    w.add({458, 282, 164, 20}, FlowAction::Recommend);
    if (s.count > 6)
        w.add(selectionLayout::listTrack, FlowAction::Scrollbar);
    if (s.detailLines > s.detailPageLines)
        w.add(selectionLayout::detailTrack, FlowAction::DetailsScrollbar);
}
bool scrollSelection(rules::SpellSelection& s, const rules::Catalog& c, const UiInput& in,
                     FlowControl action) noexcept {
    if (!in.focused || in.has(UiKey::Cancel)) {
        s.dragging = 0;
        return false;
    }
    const auto detailMax = s.detailLines > s.detailPageLines ? s.detailLines - s.detailPageLines : 0;
    s.detailOffset = std::min(s.detailOffset, detailMax);
    if (!in.focused || in.pointerReleased || (!in.pointerHeld && !in.has(UiKey::Click)))
        s.dragging = 0;
    auto adjust = [&](bool detail, int delta) {
        if (detail)
            s.detailOffset = unsigned(std::clamp(int(s.detailOffset) + delta, 0, int(detailMax)));
        else {
            s.offset = std::size_t(std::max(0, int(s.offset) + delta));
            s.filter(c);
        }
    };
    if (in.scroll) {
        const bool detail = in.pointer && selectionLayout::detailPanel.contains(*in.pointer);
        if (detail || !in.pointer || selectionLayout::listPanel.contains(*in.pointer)) {
            adjust(detail, -in.scroll * (detail ? 1 : 2));
            return true;
        }
    }
    if (in.pointer && in.has(UiKey::Click)) {
        for (unsigned kind : {1u, 2u}) {
            const bool detail = kind == 2;
            const auto track = detail ? selectionLayout::detailTrack : selectionLayout::listTrack;
            const auto count = detail ? s.detailLines : unsigned(s.count),
                       page = detail ? s.detailPageLines : 6u,
                       offset = detail ? s.detailOffset : unsigned(s.offset);
            if (count <= page || !track.contains(*in.pointer))
                continue;
            const auto thumb = selectionThumb(track, count, page, offset);
            if (thumb.contains(*in.pointer)) {
                s.dragging = kind;
                s.dragGrab = in.pointer->y - thumb.y;
            } else
                adjust(detail, in.pointer->y < thumb.y ? -int(page) : int(page));
            return true;
        }
    }
    if (s.dragging && in.pointerHeld && in.pointer) {
        const bool detail = s.dragging == 2;
        const auto track = detail ? selectionLayout::detailTrack : selectionLayout::listTrack;
        const auto count = detail ? s.detailLines : unsigned(s.count), page = detail ? s.detailPageLines : 6u;
        if (count > page) {
            const int travel = track.height - selectionThumb(track, count, page, 0).height;
            const float fraction = std::clamp((in.pointer->y - track.y - s.dragGrab) / travel, 0.f, 1.f);
            const auto offset = unsigned(std::lround(fraction * (count - page)));
            if (detail)
                s.detailOffset = offset;
            else {
                s.offset = offset;
                s.filter(c);
            }
        }
        return true;
    }
    if (in.has(UiKey::Confirm) &&
        (action.action == FlowAction::Scrollbar || action.action == FlowAction::DetailsScrollbar)) {
        adjust(action.action == FlowAction::DetailsScrollbar, in.shift ? -3 : 3);
        return true;
    }
    return false;
}
} // namespace blobber
