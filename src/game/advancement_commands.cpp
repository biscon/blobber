#include "game/commands.hpp"
#include <algorithm>
#include <cstdio>
namespace blobber {
const char* advancementFeatures(rules::ProgressionClass cls, int level) noexcept {
    using C = rules::ProgressionClass;
    switch (cls) {
    case C::Fighter:
        return level == 2 ? "Action Surge: one extra normal action. One charge, restored on a Short or Long "
                            "Rest. Activate before or after your normal action."
                          : "Champion: weapon attacks critically hit on a natural 19 or 20.";
    case C::Cleric:
        return level == 2 ? "Life Domain / Preserve Life: distribute a pool of 10 healing among allies below "
                            "half maximum HP. Lowest HP percentage first. Uses your action; one charge per "
                            "Short or Long Rest. Turn Undead is deferred."
                          : "Preserve Life pool increases to 15. Second-level spells unlock. Lesser "
                            "Restoration and Spiritual Weapon are always prepared by your Life Domain.";
    case C::Rogue:
        return level == 2
                   ? "Cunning Action: Defend once per own turn without spending your normal action. Activate "
                     "before or after it; End Turn skips an unused allowance."
                   : "Thief / Fast Hands: use your Cunning Action allowance for a potion instead of Defend. "
                     "Sneak Attack increases to 2d6. Expertise, locks, traps and climbing remain deferred.";
    case C::Wizard:
        return level == 2 ? "Evocation / Sculpt Spells: Shatter protects your other three companions. You "
                            "remain vulnerable to your own blast. Learn two spells. Spell copying and "
                            "Evocation Savant remain deferred."
                          : "Second-level spells unlock. Learn two spells. Arcane Recovery restores up to "
                            "four mana once per Long Rest.";
    default:
        return "No supported progression.";
    }
}
bool AdvancementHud::applicable(AdvancementStep s) const noexcept {
    if (s != AdvancementStep::Learned && s != AdvancementStep::Prepared)
        return true;
    return rules::advancementChoices(*catalog, identity, portrait, baseline, request.choices,
                                     s == AdvancementStep::Learned)
               .maximum != 0;
}
void AdvancementHud::sync() noexcept {
    selection = {};
    searchEdit = {};
    controls.focus = 0;
    const bool learned = step == AdvancementStep::Learned;
    selection.context =
        rules::advancementChoices(*catalog, identity, portrait, baseline, request.choices, learned);
    selection.selected = learned ? request.choices.learned : request.choices.prepared;
    selection.filter(*catalog);
}
void layoutAdvancement(FlowControls& w, const AdvancementHud& h) noexcept {
    w.clear();
    w.add({14, 330, 90, 20}, FlowAction::Cancel);
    w.add({414, 330, 90, 20}, FlowAction::Back, 0, h.step != AdvancementStep::Overview);
    const bool choosing = h.step == AdvancementStep::Learned || h.step == AdvancementStep::Prepared;
    w.add({510, 330, 116, 20}, h.step == AdvancementStep::Review ? FlowAction::Done : FlowAction::Next, 0,
          h.step == AdvancementStep::Review ? bool(h.preview) : !choosing || h.selection.valid());
    if (choosing)
        layoutSelection(w, h.selection);
}
void CommandController::openAdvancement(rules::CharacterId id) noexcept {
    const auto request = session_.advancementRequest(id);
    const auto error = session_.advancementEligibility(request);
    if (error != AdvancementError::None) {
        appendLog(advancementMessage(error));
        return;
    }
    closeOverlay();
    cancel();
    auto& h = state_.advancement;
    h = {};
    h.open = true;
    h.catalog = &party_.catalog();
    h.request = request;
    const auto& c = *party_.character(id);
    h.identity = c.build.view();
    h.portrait = c.build.portrait;
    h.baseline = c.build.spells;
    h.oldHp = c.stats.maxHitPoints;
    h.oldMana = c.maxMana;
    h.sync();
    h.preview = session_.previewAdvancement(h.request);
}
void CommandController::restorationChoice() noexcept {
    auto& m = state_.magic;
    if (m.armed && m.kind == rules::SpellKind::LesserRestoration) {
        m.pending.restoreParalysis = !m.pending.restoreParalysis;
        refreshMagic();
        updateAvailability();
    }
}
void CommandController::endTurn() noexcept {
    if (!state_.combat.activeCharacter)
        return;
    PartyCommand c;
    c.character = *state_.combat.activeCharacter;
    c.world = state_.combat.world;
    c.turn = state_.combat.turn;
    c.action = PartyAction::Feature;
    c.feature = rules::Feature::EndTurn;
    submit(c);
}
void CommandController::developmentExperience() noexcept {
    if (session_.grantNextLevelExperience())
        appendLog("Development: XP granted to the next level threshold.");
}
void CommandController::routeAdvancement(const UiInput& input) noexcept {
    auto& h = state_.advancement;
    if (!h.open)
        return;
    if (!input.focused) {
        h.searchEdit.focused = false;
        h.selection.dragging = 0;
        return;
    }
    if (h.searchEdit.edit(h.selection.search, input.text)) {
        h.selection.offset = 0;
        h.selection.filter(*h.catalog);
    }
    auto routed = input;
    if (h.searchEdit.focused) {
        routed.pressed[std::size_t(UiKey::Left)] = routed.pressed[std::size_t(UiKey::Right)] = false;
        if (input.text.backspace)
            routed.pressed[std::size_t(UiKey::Cancel)] = false;
    }
    h.preview = session_.previewAdvancement(h.request);
    layoutAdvancement(h.controls, h);
    auto a = h.controls.input(routed);
    if (routed.has(UiKey::Cancel))
        a.action = FlowAction::Cancel;
    if ((h.step == AdvancementStep::Learned || h.step == AdvancementStep::Prepared) &&
        scrollSelection(h.selection, *h.catalog, input, a)) {
        h.searchEdit.focused = false;
        return;
    }
    if (input.has(UiKey::Tab))
        h.searchEdit.focus(h.controls.entries[h.controls.focus].action == FlowAction::Search,
                           h.selection.search);
    if (a.action != FlowAction::None && a.action != FlowAction::Search)
        h.searchEdit.focused = false;
    switch (a.action) {
    case FlowAction::Cancel:
        h.open = false;
        return;
    case FlowAction::Done: {
        const auto result = session_.advance(h.request);
        if (!result) {
            std::snprintf(h.message.data(), h.message.size(), "%s", advancementMessage(result.error));
            return;
        }
        char line[128];
        std::snprintf(line, sizeof(line), "%s reached level %d.", h.identity.name.data(),
                      h.request.level + 1);
        appendLog(line);
        h.open = false;
        openInventory(h.request.character, CharacterPage::Character);
        return;
    }
    case FlowAction::Back:
    case FlowAction::Next: {
        const int direction = a.action == FlowAction::Next ? 1 : -1;
        int step = int(h.step);
        do {
            step += direction;
        } while (step >= 0 && step <= int(AdvancementStep::Review) && !h.applicable(AdvancementStep(step)));
        if (step >= 0 && step <= int(AdvancementStep::Review)) {
            h.step = AdvancementStep(step);
            h.sync();
        }
        break;
    }
    case FlowAction::Inspect:
        h.selection.inspected = {std::uint16_t(a.value)};
        h.selection.detailOffset = 0;
        break;
    case FlowAction::Toggle:
        h.selection.toggle({std::uint16_t(a.value)});
        break;
    case FlowAction::Search:
        h.searchEdit.focus(true, h.selection.search);
        break;
    case FlowAction::Recommend:
        h.selection.selected = rules::recommendedChoices(
            *h.catalog, h.selection.context,
            h.step == AdvancementStep::Learned ? rules::SpellChoice::Learned : rules::SpellChoice::Prepared);
        h.selection.inspectSelected();
        break;
    default:
        break;
    }
    if (h.step == AdvancementStep::Learned) {
        h.request.choices.learned = h.selection.selected;
        // Returning to learned choices may invalidate a newly prepared selection.
        auto context = rules::advancementChoices(*h.catalog, h.identity, h.portrait, h.baseline,
                                                 h.request.choices, false);
        h.request.choices.prepared &= context.available;
    } else if (h.step == AdvancementStep::Prepared)
        h.request.choices.prepared = h.selection.selected;
    h.preview = session_.previewAdvancement(h.request);
}
} // namespace blobber
