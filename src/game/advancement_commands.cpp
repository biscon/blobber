#include "game/commands.hpp"
#include <algorithm>
#include <cstdio>
namespace blobber {
const char* advancementFeatures(rules::ProgressionClass cls, int level) noexcept {
    using C = rules::ProgressionClass;
    if (level == 4) {
        if (cls == C::Bard)
            return "Improve ability scores, choose one new cantrip and learn one spell. Maximum mana rises "
                   "to 17.";
        if (cls == C::Wizard)
            return "Ability Score Improvement. Learn one cantrip and two spells. Maximum mana rises to 17; "
                   "second-level spells remain your highest spell level.";
        if (cls == C::Cleric)
            return "Ability Score Improvement. Learn one cantrip. Maximum mana rises to 17; Preserve Life "
                   "pool rises to 20.";
        return "Ability Score Improvement: raise one score by 2 or two scores by 1, to a maximum of 20. "
               "Existing class features remain available.";
    }
    switch (cls) {
    case C::Barbarian:
        return level == 2 ? "Reckless Attack grants advantage on Strength melee attacks and exposes you. "
                            "Danger Sense grants advantage on Dexterity saves against visible dangers."
                          : "Path of the Berserker. Three Rages per long rest. Choose one extra melee swing "
                            "per Rage after the activation turn.";
    case C::Fighter:
        return level == 2 ? "Action Surge: one extra normal action. One charge, restored on a Short or Long "
                            "Rest. Activate before or after your normal action."
                          : "Champion: weapon attacks critically hit on a natural 19 or 20.";
    case C::Cleric:
        return level == 2 ? "Life Domain / Preserve Life: distribute a pool of 10 healing among allies below "
                            "half maximum HP. Lowest HP percentage first. Uses your action; one charge per "
                            "Short or Long Rest."
                          : "Preserve Life pool increases to 15. Second-level spells unlock. Lesser "
                            "Restoration and Spiritual Weapon are always prepared by your Life Domain.";
    case C::Rogue:
        return level == 2
                   ? "Cunning Action: Defend automatically after your last normal action and advance "
                     "initiative. You may activate it earlier."
                   : "Thief / Fast Hands: use your Cunning Action allowance for a potion instead of Defend. "
                     "Use a quick potion before your last normal action. Sneak Attack increases to 2d6.";
    case C::Paladin:
        return level == 2 ? "Defense grants +1 AC while armored. Charisma spellcasting and Divine Smite unlock; share four mana. Lay on Hands pool rises to ten."
                          : "Oath of Devotion: Sanctuary and Protection from Evil and Good are always prepared. Sacred Weapon uses Channel Divinity, restored on rest. Mana rises to six; Lay on Hands to fifteen.";
    case C::Bard:
        return level == 2
                   ? "Song of Rest adds 1d6 healing for each companion spending Hit Dice on a short rest. "
                     "Learn one spell."
                   : "College of Lore grants Cutting Words. Spend one Inspiration use and a reaction to "
                     "reduce an enemy attack by 1d6. Learn one spell, including second-level choices.";
    case C::Ranger:
        return level == 2
                   ? "Archery adds +2 to ranged weapon attack rolls. Learn Hunter's Mark and one healing "
                     "spell. Cast known spells with Wisdom and four mana."
                   : "Hunter / Colossus Slayer: weapon hits on already wounded enemies add 1d8 once per "
                     "turn. Learn the remaining healing spell. Mana rises to six.";
    case C::Wizard:
        return level == 2 ? "Evocation / Sculpt Spells: Shatter protects your other three companions. You "
                            "remain vulnerable to your own blast. Learn two spells."
                          : "Second-level spells unlock. Learn two spells. Arcane Recovery restores up to "
                            "four mana once per Long Rest.";
    default:
        return "This class cannot level up.";
    }
}
rules::CharacterIdentityView AdvancementHud::candidate() const noexcept {
    auto next = identity;
    ++next.level;
    if (next.level == 4)
        next.levelFourAsi = request.choices.abilityImprovement;
    return next;
}
bool AdvancementHud::choosing() const noexcept {
    return step == AdvancementStep::Cantrips || step == AdvancementStep::Learned ||
           step == AdvancementStep::Prepared;
}
rules::SpellChoice AdvancementHud::choiceKind() const noexcept {
    return step == AdvancementStep::Cantrips  ? rules::SpellChoice::Cantrips
           : step == AdvancementStep::Learned ? rules::SpellChoice::Learned
                                              : rules::SpellChoice::Prepared;
}
bool AdvancementHud::applicable(AdvancementStep s) const noexcept {
    if (s == AdvancementStep::Abilities)
        return identity.level == 3;
    if (s != AdvancementStep::Cantrips && s != AdvancementStep::Learned && s != AdvancementStep::Prepared)
        return true;
    const auto kind = s == AdvancementStep::Cantrips  ? rules::SpellChoice::Cantrips
                      : s == AdvancementStep::Learned ? rules::SpellChoice::Learned
                                                      : rules::SpellChoice::Prepared;
    return rules::advancementChoices(*catalog, identity, portrait, baseline, request.choices, kind).maximum !=
           0;
}
void AdvancementHud::sync() noexcept {
    selection = {};
    searchEdit = {};
    controls.focus = 0;
    selection.context =
        rules::advancementChoices(*catalog, identity, portrait, baseline, request.choices, choiceKind());
    selection.selected = step == AdvancementStep::Cantrips  ? request.choices.cantrips
                         : step == AdvancementStep::Learned ? request.choices.learned
                                                            : request.choices.prepared;
    selection.filter(*catalog);
}
void AdvancementHud::storeSelection() noexcept {
    if (step == AdvancementStep::Cantrips)
        request.choices.cantrips = selection.selected;
    else if (step == AdvancementStep::Learned) {
        request.choices.learned = selection.selected;
        const auto context =
            rules::advancementChoices(*catalog, identity, portrait, baseline, request.choices, false);
        request.choices.prepared &= context.available;
    } else if (step == AdvancementStep::Prepared)
        request.choices.prepared = selection.selected;
}
bool AdvancementHud::canIncrease(unsigned index) const noexcept {
    if (index >= rules::abilityCount || identity.level != 3 ||
        request.choices.abilityImprovement.points() >= 2)
        return false;
    auto next = candidate();
    ++next.levelFourAsi.increases[index];
    return bool(rules::effectiveAbilities(*catalog, next, false));
}
void AdvancementHud::ability(unsigned index, int delta) noexcept {
    if (index >= rules::abilityCount || step != AdvancementStep::Abilities)
        return;
    auto& value = request.choices.abilityImprovement.increases[index];
    const auto before = rules::preparationLimit(*catalog, candidate());
    if (delta == 1 && canIncrease(index))
        ++value;
    else if (delta == -1 && value)
        --value;
    else
        return;
    if (rules::preparationLimit(*catalog, candidate()) < before)
        request.choices.prepared = 0;
}
void layoutAdvancement(FlowControls& w, const AdvancementHud& h) noexcept {
    w.clear();
    w.context = {2,
                 unsigned(h.step),
                 h.request.character.index,
                 unsigned(h.identity.level),
                 h.selection.offset,
                 h.selection.context.available,
                 h.selection.context.locked,
                 0};
    w.add({14, 330, 90, 20}, FlowAction::Cancel);
    w.add({414, 330, 90, 20}, FlowAction::Back, 0, h.step != AdvancementStep::Overview);
    const bool choosing = h.choosing();
    w.add({510, 330, 116, 20}, h.step == AdvancementStep::Review ? FlowAction::Done : FlowAction::Next, 0,
          h.step == AdvancementStep::Review      ? bool(h.preview)
          : h.step == AdvancementStep::Abilities ? bool(rules::effectiveAbilities(*h.catalog, h.candidate()))
                                                 : !choosing || h.selection.valid());
    if (choosing)
        layoutSelection(w, h.selection);
    if (h.step == AdvancementStep::Abilities)
        for (unsigned i = 0; i < rules::abilityCount; ++i) {
            const int y = 108 + int(i) * 27;
            w.add({502, y, 28, 20}, FlowAction::Minus, int(i),
                  h.request.choices.abilityImprovement.increases[i] != 0);
            w.add({571, y, 28, 20}, FlowAction::Plus, int(i), h.canIncrease(i));
        }
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
        m.pending.restoration = RestorationChoice((unsigned(m.pending.restoration) + 1) % 3);
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
    if (h.choosing() && scrollSelection(h.selection, *h.catalog, input, a)) {
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
    case FlowAction::Minus:
        h.ability(unsigned(a.value), -1);
        break;
    case FlowAction::Plus:
        h.ability(unsigned(a.value), 1);
        break;
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
        h.selection.selected = rules::recommendedChoices(*h.catalog, h.selection.context, h.choiceKind());
        h.selection.inspectSelected();
        break;
    default:
        break;
    }
    h.storeSelection();
    h.preview = session_.previewAdvancement(h.request);
}
} // namespace blobber
