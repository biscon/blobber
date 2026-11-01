#include "app/arena_preparation.hpp"
#include <algorithm>
namespace blobber {
void ArenaPreparationFlow::begin(const ArenaScenario& scenario) noexcept {
    active = true;
    commitRequested = false;
    step = ArenaPreparationStep::Level;
    candidate = scenario;
    advancement = {};
    controls = {};
    transition = member = 0;
}
void ArenaPreparationFlow::refresh() noexcept {
    auto& h = advancement;
    h.preview.preview = rules::previewAdvancement(
        *h.catalog, h.identity, h.baseline,
        rules::previewBuild(*h.catalog, candidate.party[member].draft).loadout, h.request.choices);
    h.preview.error = h.preview.preview.valid ? AdvancementError::None : AdvancementError::Choices;
}
void ArenaPreparationFlow::nextMember(const rules::Catalog& c) noexcept {
    while (transition + 1 < candidate.preparation.targetLevel) {
        auto& h = advancement;
        h = {};
        h.open = true;
        h.catalog = &c;
        const auto& draft = candidate.party[member].draft;
        h.identity = rules::draftIdentity(draft);
        h.portrait = draft.portrait;
        h.baseline = draft.spells;
        auto preview = rules::previewBuild(c, draft);
        auto stats = preview.stats;
        for (unsigned previous = 0; previous < transition; ++previous) {
            const auto next = rules::previewAdvancement(c, h.identity, h.baseline, preview.loadout,
                                                        candidate.preparation.advancements[previous][member]);
            if (!next.valid) {
                active = false; // An internally inconsistent draft must never commit.
                return;
            }
            stats = next.stats;
            h.baseline = next.spells;
            ++h.identity.level;
        }
        h.request.level = h.identity.level;
        h.request.choices = candidate.preparation.advancements[transition][member];
        h.oldHp = stats.maxHitPoints;
        h.oldMana = rules::maximumMana(c.characterClass(draft.characterClass)->casting, h.identity.level);
        if (h.applicable(AdvancementStep::Learned) || h.applicable(AdvancementStep::Prepared)) {
            h.step = AdvancementStep::Overview;
            h.sync();
            refresh();
            step = ArenaPreparationStep::Choices;
            return;
        }
        candidate.preparation.advancements[transition][member] = {};
        if (++member == 4) {
            member = 0;
            ++transition;
        }
    }
    step = ArenaPreparationStep::Review;
    controls = {};
}
void ArenaPreparationFlow::selectLevel(const rules::Catalog& c, unsigned level) noexcept {
    if (!active || level < 1 || level > 3)
        return;
    if (level != candidate.preparation.targetLevel)
        candidate.preparation = {level, {}};
    member = transition = 0;
    nextMember(c);
}
bool ArenaPreparationFlow::commit(const rules::Catalog& c, ArenaScenario& target) noexcept {
    if (!active || step != ArenaPreparationStep::Review ||
        arenaPreparationError(c, candidate.party, candidate.preparation))
        return false;
    target.preparation = candidate.preparation;
    active = false;
    return true;
}
void ArenaPreparationFlow::layout() noexcept {
    if (step == ArenaPreparationStep::Choices) {
        layoutAdvancement(advancement.controls, advancement);
        return;
    }
    controls.clear();
    controls.context = {4, unsigned(step), transition, member, candidate.preparation.targetLevel, 0, 0, 0};
    controls.add({20, 320, 100, 25}, FlowAction::Cancel);
    if (step == ArenaPreparationStep::Level)
        for (int level = 1; level <= 3; ++level)
            controls.add({160 + (level - 1) * 110, 150, 100, 36}, FlowAction::Step, level);
    else {
        controls.add({390, 320, 100, 25}, FlowAction::Back);
        controls.add({500, 320, 115, 25}, FlowAction::Done);
    }
}
void ArenaPreparationFlow::route(const rules::Catalog& c, const UiInput& input, const TextInput& text,
                                 float dt) noexcept {
    if (!active)
        return;
    auto& h = advancement;
    if (!input.focused) {
        h.searchEdit.focused = false;
        h.selection.dragging = 0;
        return;
    }
    layout();
    if (input.has(UiKey::Cancel) &&
        !(step == ArenaPreparationStep::Choices && h.searchEdit.focused && text.backspace)) {
        active = false;
        return;
    }
    if (step != ArenaPreparationStep::Choices) {
        const auto action = controls.input(input);
        if (action.action == FlowAction::Cancel)
            active = false;
        else if (action.action == FlowAction::Step)
            selectLevel(c, unsigned(action.value));
        else if (action.action == FlowAction::Back) {
            step = ArenaPreparationStep::Level;
            controls = {};
        } else if (action.action == FlowAction::Done)
            commitRequested = true;
        return;
    }
    h.searchEdit.tick(dt, input.focused);
    if (h.searchEdit.edit(h.selection.search, text)) {
        h.selection.offset = 0;
        h.selection.filter(c);
    }
    auto routed = input;
    if (h.searchEdit.focused)
        routed.pressed[std::size_t(UiKey::Left)] = routed.pressed[std::size_t(UiKey::Right)] = false;
    auto action = h.controls.input(routed);
    const bool choosing = h.step == AdvancementStep::Learned || h.step == AdvancementStep::Prepared;
    if (choosing && scrollSelection(h.selection, c, routed, action)) {
        h.searchEdit.focused = false;
        return;
    }
    if (input.has(UiKey::Tab))
        h.searchEdit.focus(h.controls.entries[h.controls.focus].action == FlowAction::Search,
                           h.selection.search);
    if (action.action != FlowAction::None && action.action != FlowAction::Search)
        h.searchEdit.focused = false;
    switch (action.action) {
    case FlowAction::Cancel:
        active = false;
        return;
    case FlowAction::Done:
        refresh();
        if (h.preview) {
            candidate.preparation.advancements[transition][member] = h.request.choices;
            if (++member == 4) {
                member = 0;
                ++transition;
            }
            nextMember(c);
        }
        return;
    case FlowAction::Back:
    case FlowAction::Next: {
        int next = int(h.step), direction = action.action == FlowAction::Next ? 1 : -1;
        do {
            next += direction;
        } while (next >= 0 && next <= int(AdvancementStep::Review) && !h.applicable(AdvancementStep(next)));
        if (next >= 0 && next <= int(AdvancementStep::Review)) {
            h.step = AdvancementStep(next);
            h.sync();
        }
        break;
    }
    case FlowAction::Inspect:
        h.selection.inspected = {std::uint16_t(action.value)};
        h.selection.detailOffset = 0;
        break;
    case FlowAction::Toggle:
        h.selection.toggle({std::uint16_t(action.value)});
        break;
    case FlowAction::Search:
        h.searchEdit.focus(true, h.selection.search);
        break;
    case FlowAction::Recommend:
        h.selection.selected = rules::recommendedChoices(
            c, h.selection.context,
            h.step == AdvancementStep::Learned ? rules::SpellChoice::Learned : rules::SpellChoice::Prepared);
        h.selection.inspectSelected();
        break;
    default:
        break;
    }
    if (h.step == AdvancementStep::Learned) {
        h.request.choices.learned = h.selection.selected;
        h.request.choices.prepared &=
            rules::advancementChoices(c, h.identity, h.portrait, h.baseline, h.request.choices, false)
                .available;
    } else if (h.step == AdvancementStep::Prepared)
        h.request.choices.prepared = h.selection.selected;
    refresh();
}
} // namespace blobber
