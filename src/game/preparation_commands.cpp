#include "game/commands.hpp"
#include <algorithm>
#include <cstdio>
namespace blobber {
void layoutPreparation(FlowControls& w, const CampHud& h) noexcept {
    w.clear();
    for (unsigned i = 0; i < 4; ++i)
        if (h.casters[i])
            w.add({174 + int(i) * 112, 36, 107, 20}, FlowAction::Member, int(i));
    w.add({14, 330, 90, 20}, FlowAction::Cancel);
    bool valid = true;
    for (unsigned i = 0; i < 4; ++i)
        if (h.casters[i]) {
            rules::SpellSelection s;
            s.context = h.contexts[i];
            s.selected = h.editing[i];
            valid &= s.valid();
        }
    w.add({510, 330, 116, 20}, FlowAction::Done, 0, valid);
    layoutSelection(w, h.selection);
}
void CommandController::openPreparation() noexcept {
    auto& h = state_.camp;
    if (!session_ || !h.open || h.playing || h.request.kind != RestKind::Long)
        return;
    refreshCamp();
    if (!h.canPrepare)
        return;
    h.editingRequest = h.request.preparation.enabled ? h.request.preparation : party_.preparationRequest();
    h.editing = h.editingRequest.prepared;
    bool found = false;
    for (std::uint16_t i = 0; i < 4; ++i) {
        const auto& c = *party_.character({i});
        h.contexts[i] = rules::choiceContext(party_.catalog(), c.build.view(), c.build.portrait,
                                             c.build.spells, rules::SpellChoice::Prepared);
        h.casters[i] = c.life != rules::Life::Dead && h.contexts[i].maximum > 0;
        if (h.casters[i] && !found) {
            h.preparationMember = i;
            found = true;
        }
    }
    if (!found)
        return;
    h.preparing = true;
    h.selection = {};
    h.controls = {};
    h.searchEdit = {};
    h.preparationMessage = {};
    h.selection.context = h.contexts[h.preparationMember];
    h.selection.selected = h.editing[h.preparationMember];
    h.selection.filter(party_.catalog());
}
void CommandController::routePreparation(const UiInput& input) noexcept {
    auto& h = state_.camp;
    if (!h.preparing || !input.focused) {
        if (!input.focused) {
            h.searchEdit.focused = false;
            h.selection.dragging = 0;
        }
        return;
    }
    if (h.searchEdit.edit(h.selection.search, input.text)) {
        h.selection.offset = 0;
        h.selection.filter(party_.catalog());
    }
    auto routed = input;
    if (h.searchEdit.focused) {
        routed.pressed[std::size_t(UiKey::Left)] = routed.pressed[std::size_t(UiKey::Right)] = false;
        if (input.text.backspace)
            routed.pressed[std::size_t(UiKey::Cancel)] = false;
    }
    layoutPreparation(h.controls, h);
    auto a = h.controls.input(routed);
    if (routed.has(UiKey::Cancel))
        a.action = FlowAction::Cancel;
    if (scrollSelection(h.selection, party_.catalog(), input, a)) {
        h.searchEdit.focused = false;
        return;
    }
    if (input.has(UiKey::Tab))
        h.searchEdit.focus(h.controls.entries[h.controls.focus].action == FlowAction::Search,
                           h.selection.search);
    if ((a.action != FlowAction::None && a.action != FlowAction::Search) ||
        (input.has(UiKey::Click) && a.action == FlowAction::None))
        h.searchEdit.focused = false;
    switch (a.action) {
    case FlowAction::Cancel:
        h.preparing = false;
        return;
    case FlowAction::Done: {
        auto candidate = h.request;
        candidate.preparation = h.editingRequest;
        candidate.preparation.prepared = h.editing;
        const auto p = session_->previewRest(candidate);
        // Unchanged preparation may be staged; Start Rest remains unavailable without a benefit.
        if (!p && p.error != RestError::NoBenefit) {
            std::snprintf(h.preparationMessage.data(), h.preparationMessage.size(), "%s",
                          restMessage(p.error));
            return;
        }
        h.request = candidate;
        h.staged = candidate.preparation;
        h.preparing = false;
        refreshCamp();
        return;
    }
    case FlowAction::Member:
        h.preparationMember = unsigned(a.value);
        h.selection = {};
        h.selection.context = h.contexts[h.preparationMember];
        h.selection.selected = h.editing[h.preparationMember];
        h.selection.filter(party_.catalog());
        h.controls.focus = 0;
        break;
    case FlowAction::Inspect:
        h.selection.inspected = {std::uint16_t(a.value)};
        h.selection.detailOffset = 0;
        break;
    case FlowAction::Toggle:
        h.selection.toggle({std::uint16_t(a.value)});
        break;
    case FlowAction::Recommend:
        h.selection.selected =
            rules::recommendedChoices(party_.catalog(), h.selection.context, rules::SpellChoice::Prepared);
        h.selection.inspectSelected();
        break;
    case FlowAction::Search:
        h.searchEdit.focus(true, h.selection.search);
        break;
    default:
        break;
    }
    h.editing[h.preparationMember] = h.selection.selected;
}
} // namespace blobber
