#include "game/creator.hpp"
#include <algorithm>
#include <cstdio>
namespace blobber {
using namespace rules;
namespace {
bool equal(const CharacterDraft& a, const CharacterDraft& b) noexcept {
    return a.name == b.name && a.race.index == b.race.index &&
           a.characterClass.index == b.characterClass.index && a.portrait.index == b.portrait.index &&
           a.abilities == b.abilities && a.defense == b.defense && a.lifeDomain == b.lifeDomain &&
           a.gear == b.gear && a.spells.cantrips == b.spells.cantrips && a.spells.racial == b.spells.racial &&
           a.spells.learned == b.spells.learned && a.spells.prepared == b.spells.prepared &&
           a.spells.domain == b.spells.domain;
}
bool sameOption(const GearOption& a, const GearOption& b) noexcept {
    if (a.count != b.count)
        return false;
    for (std::size_t i = 0; i < a.count; ++i)
        if (a.items[i].item.index != b.items[i].item.index || a.items[i].quantity != b.items[i].quantity ||
            a.items[i].slot != b.items[i].slot)
            return false;
    return true;
}
} // namespace
const char* creatorStepName(CreatorStep s) noexcept {
    constexpr const char* names[]{"Identity",       "Class",     "Abilities",   "Skills",    "Cantrips",
                                  "Racial cantrip", "Spellbook", "Preparation", "Equipment", "Review"};
    return unsigned(s) < 10 ? names[unsigned(s)] : "";
}
void CreatorModel::notify(const char* s) noexcept {
    std::snprintf(message.data(), message.size(), "%s", s);
}
void CreatorModel::begin(const Catalog& c) noexcept {
    *this = {};
    draft.race = c.findRace("human");
    draft.portrait = defaultPortrait("human");
    baseline = draft;
    refresh(c);
    nameEdit.focus(true, draft.name);
}
bool CreatorModel::dirty() const noexcept {
    return !equal(draft, baseline);
}
SpellChoice CreatorModel::choice() const noexcept {
    return step == CreatorStep::Cantrips  ? SpellChoice::Cantrips
           : step == CreatorStep::Racial  ? SpellChoice::Racial
           : step == CreatorStep::Learned ? SpellChoice::Learned
                                          : SpellChoice::Prepared;
}
bool CreatorModel::applicable(const Catalog& c, CreatorStep s) const noexcept {
    const auto* cls = c.characterClass(draft.characterClass);
    const auto* r = c.race(draft.race);
    if (s == CreatorStep::Racial)
        return r && r->wizardCantrip;
    if (s == CreatorStep::Cantrips || s == CreatorStep::Prepared)
        return cls && cls->casting != Casting::None;
    if (s == CreatorStep::Learned)
        return cls && cls->casting == Casting::Intelligence;
    return s < CreatorStep::Count;
}
void CreatorModel::syncSelection(const Catalog& c) noexcept {
    if (step < CreatorStep::Cantrips || step > CreatorStep::Prepared)
        return;
    const auto previous = selection.context;
    selection.context = choiceContext(c, draftIdentity(draft), draft.portrait, draft.spells, choice());
    selection.selected = selection.context.baseline;
    if (previous.available != selection.context.available || previous.locked != selection.context.locked)
        selection.offset = 0;
    selection.filter(c);
}
void CreatorModel::refresh(const Catalog& c) noexcept {
    preview = previewBuild(c, draft);
    syncSelection(c);
}
bool CreatorModel::validStep(const Catalog& c, CreatorStep s) const noexcept {
    const auto p = previewBuild(c, draft);
    switch (s) {
    case CreatorStep::Identity:
        return !p.has(BuildError::Name) && !p.has(BuildError::Race) && !p.has(BuildError::Portrait);
    case CreatorStep::Class:
        return !p.has(BuildError::Class) && !p.has(BuildError::Features);
    case CreatorStep::Abilities:
        return !p.has(BuildError::Abilities) && !p.has(BuildError::Budget);
    case CreatorStep::Skills:
        return true;
    case CreatorStep::Equipment:
        return !p.has(BuildError::Gear) && !p.has(BuildError::Derived) && !p.has(BuildError::Content);
    case CreatorStep::Review:
        return bool(p);
    default: {
        const auto k = s == CreatorStep::Cantrips  ? SpellChoice::Cantrips
                       : s == CreatorStep::Racial  ? SpellChoice::Racial
                       : s == CreatorStep::Learned ? SpellChoice::Learned
                                                   : SpellChoice::Prepared;
        SpellSelection query;
        query.context = choiceContext(c, draftIdentity(draft), draft.portrait, draft.spells, k);
        query.selected = query.context.baseline;
        return query.valid();
    }
    }
}
void CreatorModel::go(const Catalog& c, CreatorStep to) noexcept {
    if (!applicable(c, to))
        return;
    step = to;
    visited |= 1u << unsigned(to);
    selection = {};
    nameEdit.focused = searchEdit.focused = false;
    message = {};
    reviewOffset = 0;
    refresh(c);
}
void CreatorModel::next(const Catalog& c, int direction) noexcept {
    if (direction > 0 && !validStep(c, step)) {
        notify("Complete the highlighted choices before continuing.");
        return;
    }
    for (int i = int(step) + direction; i >= 0 && i < int(CreatorStep::Count); i += direction)
        if (applicable(c, CreatorStep(i))) {
            go(c, CreatorStep(i));
            return;
        }
}
void CreatorModel::prune(const Catalog& c) noexcept {
    const auto* cls = c.characterClass(draft.characterClass);
    const auto* r = c.race(draft.race);
    draft.spells.cantrips &= classSpells(c, draft.characterClass, 0);
    draft.spells.learned &=
        cls && cls->casting == Casting::Intelligence ? classSpells(c, draft.characterClass, 1) : 0;
    draft.spells.racial &= r && r->wizardCantrip ? racialCantrips(c) & ~draft.spells.cantrips : 0;
    SpellGrants recommended;
    recommendSpells(c, draftIdentity(draft), recommended);
    draft.spells.domain = recommended.domain;
    draft.spells.prepared &= preparationOptions(c, draftIdentity(draft), draft.spells);
    refresh(c);
}
void CreatorModel::race(const Catalog& c, RaceId id) noexcept {
    if (!c.race(id) || id.index == draft.race.index)
        return;
    draft.race = id;
    draft.portrait = defaultPortrait(c.race(id)->id);
    prune(c);
    notify("Race changed: portrait reset; invalid grants removed. Review abilities and spells.");
}
void CreatorModel::characterClass(const Catalog& c, ClassId id) noexcept {
    const auto* cls = c.characterClass(id);
    if (!cls || id.index == draft.characterClass.index)
        return;
    const auto old = draft;
    const bool first = !c.characterClass(draft.characterClass);
    const auto recommended = recommendedDraft(c, draft.race, id);
    draft.characterClass = id;
    draft.defense = recommended.defense;
    draft.lifeDomain = recommended.lifeDomain;
    if (first) {
        draft.abilities = recommended.abilities;
        draft.spells = recommended.spells;
        draft.gear = recommended.gear;
    } else {
        draft.gear = {};
        const auto* oldOptions = c.creation(old.characterClass);
        const auto* newOptions = c.creation(id);
        if (oldOptions && newOptions)
            for (std::size_t g = 0; g < newOptions->groupCount; ++g)
                for (std::size_t o = 0; o < newOptions->groups[g].count; ++o)
                    for (std::size_t h = 0; h < oldOptions->groupCount; ++h)
                        if (old.gear[h] < oldOptions->groups[h].count &&
                            sameOption(newOptions->groups[g].options[o],
                                       oldOptions->groups[h].options[old.gear[h]]))
                            draft.gear[g] = std::uint8_t(o);
    }
    prune(c);
    notify(first ? "Recommended abilities, spells and starting gear selected. You can change every choice."
                 : "Class changed: valid choices retained; other gear reset. Review spells and equipment.");
}
void CreatorModel::ability(const Catalog& c, unsigned i, int delta) noexcept {
    if (i >= 6)
        return;
    auto next = draft.abilities;
    next[i] += delta;
    if (pointBuyCost(next) < 0 || pointBuyCost(next) > 27) {
        notify("Increase unavailable: base limit 15 or insufficient points.");
        return;
    }
    draft.abilities = next;
    refresh(c);
    notify(preview.has(BuildError::Prepared) ? "Preparation needs review after the ability change." : "");
}
void CreatorModel::recommend(const Catalog& c) noexcept {
    if (step == CreatorStep::Abilities) {
        if (const auto* cls = c.creation(draft.characterClass))
            draft.abilities = cls->recommended;
    } else {
        auto bits = recommendedChoices(c, selection.context, choice());
        if (step == CreatorStep::Cantrips)
            draft.spells.cantrips = bits;
        if (step == CreatorStep::Racial)
            draft.spells.racial = bits;
        if (step == CreatorStep::Learned)
            draft.spells.learned = bits;
        if (step == CreatorStep::Prepared)
            draft.spells.prepared = bits;
        prune(c);
    }
    refresh(c);
    notify("Recommended choices applied to this step.");
}
void CreatorModel::toggleSpell(const Catalog& c, SpellId id) noexcept {
    if (!selection.toggle(id))
        return;
    if (step == CreatorStep::Cantrips)
        draft.spells.cantrips = selection.selected;
    if (step == CreatorStep::Racial)
        draft.spells.racial = selection.selected;
    if (step == CreatorStep::Learned)
        draft.spells.learned = selection.selected;
    if (step == CreatorStep::Prepared)
        draft.spells.prepared = selection.selected;
    prune(c);
}
void CreatorModel::cancel() noexcept {
    if (dirty())
        discard = true;
    else
        canceled = true;
}
void FlowControls::add(UiRect r, FlowAction a, int value, bool enabled) noexcept {
    if (count == entries.size()) {
        overflow = true;
        return;
    }
    entries[count++] = {r, a, value, enabled};
}
FlowControl FlowControls::input(const UiInput& in) noexcept {
    if (!count || overflow || !in.focused)
        return {};
    focus = std::min(focus, count - 1);
    if (in.has(UiKey::Tab) || in.has(UiKey::Down) || in.has(UiKey::Right))
        focus = (focus + (in.shift ? count - 1 : 1)) % count;
    if (in.has(UiKey::Up) || in.has(UiKey::Left))
        focus = (focus + count - 1) % count;
    if (in.has(UiKey::Click) && in.pointer)
        for (unsigned i = 0; i < count; ++i)
            if (entries[i].rect.contains(*in.pointer)) {
                focus = i;
                return entries[i].enabled ? entries[i] : FlowControl{};
            }
    if (in.has(UiKey::Confirm) && entries[focus].enabled)
        return entries[focus];
    return {};
}
void layoutSelection(FlowControls& w, const SpellSelection& s) noexcept {
    w.add({174, 79, 266, 18}, FlowAction::Search);
    for (std::size_t j = 0; j < 6 && j + s.offset < s.count; ++j) {
        const int id = s.visible[j + s.offset].index;
        const UiRect r{174 + int(j % 2) * 137, 109 + int(j / 2) * 53, 129, 47};
        w.add({r.x, r.y, r.width - 23, r.height}, FlowAction::Inspect, id);
        const auto bit = spellBit({std::uint16_t(id)});
        w.add({r.x + r.width - 23, r.y, 23, r.height}, FlowAction::Toggle, id,
              !(s.context.locked & bit) &&
                  ((s.selected & bit) || spellCount(s.selected) < s.context.maximum));
    }
    w.add({174, 276, 40, 18}, FlowAction::Scroll, -2, s.offset > 0);
    w.add({400, 276, 40, 18}, FlowAction::Scroll, 2, s.offset + 6 < s.count);
    w.add({458, 282, 164, 20}, FlowAction::Recommend);
    if (s.count > 6)
        w.add({444, 109, 7, 153}, FlowAction::Scrollbar);
    w.add({593, 233, 25, 17}, FlowAction::DetailsScroll, -1, s.detailOffset > 0);
    w.add({593, 252, 25, 17}, FlowAction::DetailsScroll, 1);
}
void layoutCreator(FlowControls& w, const CreatorModel& m, const Catalog& c) noexcept {
    w.clear();
    if (m.discard) {
        w.add({218, 204, 114, 24}, FlowAction::Keep);
        w.add({344, 204, 114, 24}, FlowAction::Discard);
        return;
    }
    int x = 170;
    for (auto s : {CreatorStep::Identity, CreatorStep::Class, CreatorStep::Abilities, CreatorStep::Skills,
                   CreatorStep::Cantrips, CreatorStep::Equipment, CreatorStep::Review}) {
        if (s == CreatorStep::Cantrips) {
            if (m.applicable(c, CreatorStep::Cantrips))
                s = CreatorStep::Cantrips;
            else if (m.applicable(c, CreatorStep::Racial))
                s = CreatorStep::Racial;
            else {
                x += 65;
                continue;
            }
        }
        w.add({x, 36, 62, 18}, FlowAction::Step, int(s), (m.visited & (1u << unsigned(s))) != 0);
        x += 65;
    }
    w.add({14, 330, 90, 20}, FlowAction::Cancel);
    w.add({440, 330, 76, 20}, FlowAction::Back, 0, m.step != CreatorStep::Identity);
    w.add({528, 330, 98, 20}, m.step == CreatorStep::Review ? FlowAction::Save : FlowAction::Next, 0,
          m.validStep(c, m.step) && !m.saved);
    switch (m.step) {
    case CreatorStep::Identity:
        w.add({174, 94, 448, 24}, FlowAction::Name);
        for (int i = 0; i < 4; ++i)
            w.add({174 + i * 114, 147, 106, 30}, FlowAction::Race, i);
        {
            int i = 0;
            for (unsigned p = 0; p < portraitCount; ++p)
                if (const auto* r = c.race(m.draft.race); r && r->id == portrait({std::uint8_t(p)})->race)
                    w.add({174 + i++ * 91, 207, 82, 98}, FlowAction::Portrait, int(p));
        }
        break;
    case CreatorStep::Class:
        for (int i = 0; i < 4; ++i)
            w.add({174 + i * 114, 92, 106, 90}, FlowAction::Class, i);
        break;
    case CreatorStep::Abilities:
        for (unsigned i = 0; i < 6; ++i) {
            w.add({280, 103 + int(i) * 28, 19, 19}, FlowAction::Minus, int(i), m.draft.abilities[i] > 8);
            auto next = m.draft.abilities;
            ++next[i];
            w.add({332, 103 + int(i) * 28, 19, 19}, FlowAction::Plus, int(i),
                  pointBuyCost(next) >= 0 && pointBuyCost(next) <= 27);
        }
        w.add({174, 282, 178, 22}, FlowAction::Recommend);
        w.add({364, 282, 104, 22}, FlowAction::Reset);
        break;
    case CreatorStep::Cantrips:
    case CreatorStep::Racial:
    case CreatorStep::Learned:
    case CreatorStep::Prepared:
        layoutSelection(w, m.selection);
        {
            int sx = 174;
            for (auto s :
                 {CreatorStep::Cantrips, CreatorStep::Racial, CreatorStep::Learned, CreatorStep::Prepared})
                if (m.applicable(c, s)) {
                    w.add({sx, 59, 109, 17}, FlowAction::Step, int(s),
                          (m.visited & (1u << unsigned(s))) != 0);
                    sx += 112;
                }
        }
        break;
    case CreatorStep::Equipment:
        if (const auto* options = c.creation(m.draft.characterClass))
            for (std::size_t g = 0; g < options->groupCount; ++g)
                for (std::size_t o = 0; o < options->groups[g].count; ++o)
                    w.add({174 + int(o) * 137, 92 + int(g) * 58, 129, 49}, FlowAction::Gear, int(g * 2 + o));
        break;
    case CreatorStep::Review:
        w.add({174, 291, 94, 20}, FlowAction::Step, int(CreatorStep::Identity));
        w.add({278, 291, 94, 20}, FlowAction::Step, int(CreatorStep::Abilities));
        if (m.applicable(c, CreatorStep::Cantrips) || m.applicable(c, CreatorStep::Racial))
            w.add({382, 291, 94, 20}, FlowAction::Step,
                  int(m.applicable(c, CreatorStep::Cantrips) ? CreatorStep::Cantrips : CreatorStep::Racial));
        w.add({486, 291, 94, 20}, FlowAction::Step, int(CreatorStep::Equipment));
        w.add({584, 135, 37, 18}, FlowAction::Scroll, -1, m.reviewOffset > 0);
        w.add({584, 265, 37, 18}, FlowAction::Scroll, 1);
        break;
    default:
        break;
    }
    if (w.focus >= w.count)
        w.focus = 0;
}
void routeCreator(CreatorModel& m, FlowControls& w, const Catalog& c, const UiInput& input,
                  const TextInput& text, float dt) noexcept {
    m.nameEdit.tick(dt, input.focused);
    m.searchEdit.tick(dt, input.focused);
    if (!input.focused)
        return;
    layoutCreator(w, m, c);
    if (m.discard) {
        const auto a = w.input(input).action;
        if (input.has(UiKey::Cancel) || a == FlowAction::Keep) {
            m.discard = false;
            w.focus = 0;
        } else if (a == FlowAction::Discard) {
            m.discard = false;
            m.canceled = true;
        }
        return;
    }
    if (m.nameEdit.edit(m.draft.name, text))
        m.refresh(c);
    if (m.searchEdit.edit(m.selection.search, text)) {
        m.selection.offset = 0;
        m.selection.filter(c);
    }
    UiInput routed = input;
    if (m.nameEdit.focused || m.searchEdit.focused) {
        routed.pressed[std::size_t(UiKey::Left)] = routed.pressed[std::size_t(UiKey::Right)] = false;
        if (text.backspace)
            routed.pressed[std::size_t(UiKey::Cancel)] = false;
    }
    if (routed.has(UiKey::Cancel)) {
        m.cancel();
        w.focus = 0;
        return;
    }
    auto a = w.input(routed);
    if (input.has(UiKey::Tab)) {
        m.nameEdit.focus(w.entries[w.focus].action == FlowAction::Name, m.draft.name);
        m.searchEdit.focus(w.entries[w.focus].action == FlowAction::Search, m.selection.search);
    }
    if (input.scroll) {
        a.action = m.step >= CreatorStep::Cantrips && m.step <= CreatorStep::Prepared && input.pointer &&
                           UiRect{458, 104, 164, 169}.contains(*input.pointer)
                       ? FlowAction::DetailsScroll
                       : FlowAction::Scroll;
        a.value =
            -input.scroll * (a.action == FlowAction::DetailsScroll || m.step == CreatorStep::Review ? 1 : 2);
    }
    if (input.pointerHeld && input.pointer && UiRect{444, 109, 7, 153}.contains(*input.pointer) &&
        m.selection.count > 6)
        a.action = FlowAction::Scrollbar;
    if (a.action == FlowAction::None) {
        if (input.has(UiKey::Click))
            m.nameEdit.focused = m.searchEdit.focused = false;
        return;
    }
    if (a.action != FlowAction::Name && a.action != FlowAction::Search)
        m.nameEdit.focused = m.searchEdit.focused = false;
    switch (a.action) {
    case FlowAction::Cancel:
        m.cancel();
        w.focus = 0;
        break;
    case FlowAction::Back:
        m.next(c, -1);
        w.focus = 0;
        break;
    case FlowAction::Next:
        m.next(c, 1);
        w.focus = 0;
        break;
    case FlowAction::Step:
        m.go(c, CreatorStep(a.value));
        w.focus = 0;
        break;
    case FlowAction::Name:
        m.nameEdit.focus(true, m.draft.name);
        break;
    case FlowAction::Search:
        m.searchEdit.focus(true, m.selection.search);
        break;
    case FlowAction::Race:
        m.race(c, {std::uint16_t(a.value)});
        break;
    case FlowAction::Portrait:
        m.draft.portrait = {std::uint8_t(a.value)};
        m.refresh(c);
        break;
    case FlowAction::Class:
        m.characterClass(c, {std::uint16_t(a.value)});
        break;
    case FlowAction::Minus:
        m.ability(c, unsigned(a.value), -1);
        break;
    case FlowAction::Plus:
        m.ability(c, unsigned(a.value), 1);
        break;
    case FlowAction::Recommend:
        m.recommend(c);
        break;
    case FlowAction::Reset:
        m.draft.abilities.fill(8);
        m.refresh(c);
        break;
    case FlowAction::Gear:
        m.draft.gear[unsigned(a.value) / 2] = std::uint8_t(a.value % 2);
        m.refresh(c);
        break;
    case FlowAction::Inspect:
        m.selection.inspected = {std::uint16_t(a.value)};
        m.selection.detailOffset = 0;
        break;
    case FlowAction::Toggle:
        m.toggleSpell(c, {std::uint16_t(a.value)});
        break;
    case FlowAction::Scroll:
        if (m.step == CreatorStep::Review)
            m.reviewOffset = unsigned(std::clamp(int(m.reviewOffset) + a.value, 0, 48));
        else {
            m.selection.offset = std::size_t(std::max(0, int(m.selection.offset) + a.value));
            m.selection.filter(c);
        }
        break;
    case FlowAction::Scrollbar:
        if (input.pointer && m.selection.count > 6) {
            m.selection.offset = std::size_t(std::clamp((input.pointer->y - 109) / 153.f, 0.f, 1.f) *
                                             float(m.selection.count - 6));
            m.selection.filter(c);
        }
        break;
    case FlowAction::DetailsScroll:
        m.selection.detailOffset = unsigned(std::clamp(int(m.selection.detailOffset) + a.value, 0, 24));
        break;
    case FlowAction::Save:
        if (!m.saved && m.preview)
            m.saveRequested = true;
        break;
    default:
        break;
    }
}
} // namespace blobber
