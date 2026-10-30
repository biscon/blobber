#include "ui/creator_screen.hpp"
#include <algorithm>
#include <cstring>
namespace blobber {
using namespace hudInk;
using namespace rules;
namespace {
unsigned wrap(const HudPainter& p, const char* value, UiRect r, Color color = text,
              UiTextStyle style = UiTextStyle::Label, unsigned skip = 0, int spacing = 12) {
    int y = r.y;
    unsigned lines = 0;
    while (*value) {
        char line[192]{};
        unsigned n = 0, space = 0;
        while (value[n] && value[n] != '\n' && n < sizeof(line) - 1) {
            line[n] = value[n];
            line[n + 1] = 0;
            if (p.fonts->width(line, style) > r.width) {
                line[n] = 0;
                break;
            }
            if (value[n] == ' ')
                space = n;
            ++n;
        }
        if (value[n] && value[n] != '\n' && space)
            n = space;
        if (!n)
            n = 1;
        line[n] = 0;
        ++lines;
        if (skip)
            --skip;
        else {
            if (y + spacing <= r.y + r.height)
                p.text(line, r.x, y, color, style);
            y += spacing;
        }
        value += n;
        while (*value == ' ' || *value == '\n')
            ++value;
    }
    return lines;
}
void button(const HudPainter& p, const FlowControls& controls, unsigned i, const char* label,
            bool selected = false) {
    const auto& b = controls.entries[i];
    p.box(b.rect, selected ? teal : i == controls.focus ? gold : edge);
    if (selected)
        p.fill({b.rect.x + 2, b.rect.y + 2, b.rect.width - 4, b.rect.height - 4}, {20, 58, 53, 255});
    const auto color = b.enabled ? (selected ? WHITE : text) : muted;
    if (p.fonts->width(label, UiTextStyle::Label) > b.rect.width - 8)
        p.label(label, b.rect, color);
    else
        p.centered(label, b.rect, color);
    if (i == controls.focus)
        p.border({b.rect.x - 1, b.rect.y - 1, b.rect.width + 2, b.rect.height + 2}, gold);
}
void textField(const HudPainter& p, UiRect r, const std::array<char, 65>& textValue, const TextEdit& edit,
               const char* placeholder) {
    p.box(r, edit.focused ? teal : edge);
    auto n = std::strlen(textValue.data()), cursor = std::min(edit.cursor, n);
    std::size_t begin = 0;
    char prefix[65]{};
    auto prefixWidth = [&] {
        std::memcpy(prefix, textValue.data() + begin, cursor - begin);
        prefix[cursor - begin] = 0;
        return p.fonts->width(prefix, UiTextStyle::Heading);
    };
    while (begin < cursor && prefixWidth() > r.width - 18)
        ++begin;
    if (!n && !edit.focused)
        p.label(placeholder, r, muted);
    else
        p.clipped(textValue.data() + begin, r.x + 6, r.y + 6, r.width - 12, text, UiTextStyle::Heading);
    if (edit.focused && edit.blink < .5f) {
        const float x = r.x + 6 + prefixWidth();
        p.line({x, float(r.y + 4)}, {x, float(r.y + r.height - 4)}, teal, 1);
    }
}
void sidebar(const HudPainter& p, const Catalog& c, const CharacterIdentityView& b, PortraitId portraitId,
             const char* subtitle = "") {
    p.box({10, 62, 144, 258});
    p.picture(64 + portraitId.index, {11, 63, 142, 213});
    p.clipped(b.name.empty() ? "Your character" : b.name.data(), 17, 283, 130, gold, UiTextStyle::Display);
    if (const auto* race = c.race(b.race))
        p.clipped(race->name.c_str(), 17, 298, 130);
    if (const auto* cls = c.characterClass(b.characterClass)) {
        char line[96];
        std::snprintf(line, sizeof(line), "Level %d %s", b.level, cls->name.c_str());
        p.clipped(line, 17, 310, 130, muted, UiTextStyle::Small);
    }
    if (*subtitle)
        p.clipped(subtitle, 174, 310, 448, muted, UiTextStyle::Small);
}
void frame(const HudPainter& p, const char* title) {
    p.fill({0, 0, 640, 360}, background);
    p.border({6, 6, 628, 348}, edge);
    p.text(title, 14, 13, gold, UiTextStyle::Control);
    p.line({12, 57}, {628, 57}, edge);
    p.line({12, 324}, {628, 324}, edge);
}
void drawSelection(const HudPainter& p, const Catalog& c, const SpellSelection& s, const TextEdit& edit,
                   const FlowControls& controls, const char* title) {
    textField(p, {174, 79, 266, 18}, s.search, edit, "Search spells...");
    p.text(title, 458, 66, gold, UiTextStyle::Heading);
    char line[128];
    std::snprintf(line, sizeof(line), "%u / %u selected", spellCount(s.selected), s.context.maximum);
    if (!s.context.showLocked && s.context.minimum == 0)
        std::snprintf(line, sizeof(line), "%u / %u new slots filled", spellCount(s.selected),
                      s.context.maximum);
    p.text(line, 458, 82, s.valid() ? teal : red, UiTextStyle::Heading);
    p.box({174, 109, 266, 153});
    for (std::size_t j = 0; j < 6 && j + s.offset < s.count; ++j) {
        const auto id = s.visible[j + s.offset];
        const auto& spell = *c.spell(id);
        const auto bit = spellBit(id);
        const bool selected = (s.selected & bit) != 0, locked = (s.context.locked & bit) != 0;
        const UiRect r{174 + int(j % 2) * 137, 109 + int(j / 2) * 53, 129, 47};
        p.box(r, selected ? teal : edge);
        if (selected)
            p.border({r.x + 1, r.y + 1, r.width - 2, r.height - 2}, teal);
        p.art->creator(unsigned(spell.kind), {r.x + 3, r.y + 6, 33, 33});
        wrap(p, spell.name.c_str(), {r.x + 40, r.y + 5, 84, 28}, selected ? WHITE : gold);
        const auto* rule = spellRules(spell.kind);
        char cost[48];
        std::snprintf(cost, sizeof(cost), "Level %d / %d mana", rule->level, spellManaCost(rule->level));
        p.text(locked        ? ((s.context.granted & bit) ? "Granted" : s.context.lockedLabel)
               : rule->level ? cost
                             : "Cantrip / 0 mana",
               r.x + 40, r.y + 33, locked ? gold : muted, UiTextStyle::Small);
    }
    if (!s.count)
        wrap(p, "No matching spells. Clear the search to see all choices.", {174, 120, 266, 40}, muted);
    p.box(selectionLayout::detailPanel);
    auto scrollbar = [&](UiRect track, unsigned count, unsigned page, unsigned offset) {
        if (count <= page)
            return;
        p.fill(track, edge);
        auto thumb = selectionThumb(track, count, page, offset);
        ++thumb.x;
        thumb.width -= 2;
        p.fill(thumb, teal);
    };
    if (const auto* spell = c.spell(s.inspected)) {
        p.art->creator(unsigned(spell->kind), {492, 114, 96, 96});
        p.clipped(spell->name.c_str(), 466, 214, 148, gold, UiTextStyle::Heading);
        const UiRect description{466, 230, 142, 27};
        const int spacing = p.fonts->lineHeight(UiTextStyle::Small);
        s.detailPageLines = unsigned(description.height / spacing);
        s.detailLines =
            wrap(p, spell->description.c_str(), {466, 230, 142, 0}, text, UiTextStyle::Small, 0, spacing);
        const auto last = s.detailLines > s.detailPageLines ? s.detailLines - s.detailPageLines : 0;
        const auto offset = std::min(s.detailOffset, last);
        wrap(p, spell->description.c_str(), description, text, UiTextStyle::Small, offset, spacing);
        scrollbar(selectionLayout::detailTrack, s.detailLines, s.detailPageLines, offset);
    } else {
        s.detailLines = 0;
    }
    scrollbar(selectionLayout::listTrack, unsigned(s.count), 6, unsigned(s.offset));
    std::snprintf(line, sizeof(line), "%zu-%zu / %zu", s.count ? s.offset + 1 : 0,
                  std::min(s.count, s.offset + 6), s.count);
    if (s.context.showLocked)
        p.centered(line, {216, 276, 182, 18}, muted, UiTextStyle::Small);
    if (s.context.showLocked && s.context.maximum != s.context.minimum)
        p.text(s.context.minimum ? "Prepare at least one; the allowance is a maximum."
                                 : "Optional preparation; existing choices stay locked.",
               174, 299, muted, UiTextStyle::Small);
    for (unsigned i = 0; i < controls.count; ++i) {
        const auto& b = controls.entries[i];
        if (b.action == FlowAction::Recommend)
            button(p, controls, i, "Recommended picks");
    }
}
} // namespace
unsigned drawBuildReview(const HudPainter& p, const Catalog& c, const CharacterDraft& d, unsigned offset) {
    const auto preview = previewBuild(c, d);
    sidebar(p, c, draftIdentity(d), d.portrait);
    const auto* cls = c.characterClass(d.characterClass);
    const int mana = cls ? maximumMana(cls->casting, 1) : 0;
    const char* labels[]{"Hit points", "Armor Class", "Mana", "Point buy"};
    const int values[]{preview.stats.maxHitPoints, preview.stats.armorClass, mana, preview.points};
    for (int i = 0; i < 4; ++i) {
        const UiRect r{174 + i * 114, 65, 106, 46};
        p.box(r);
        p.centered(labels[i], {r.x, r.y + 4, r.width, 12}, muted);
        char v[32];
        std::snprintf(v, sizeof(v), i == 3 ? "%d / 27" : "%d", values[i]);
        p.centered(v, {r.x, r.y + 18, r.width, 23}, gold, UiTextStyle::Control);
    }
    p.box({174, 118, 448, 167});
    unsigned row = 0;
    auto detail = [&](const char* label, const char* value) {
        if (row >= offset && row < offset + 13) {
            const int y = 123 + int(row - offset) * 12;
            p.clipped(label, 182, y, 109, gold, UiTextStyle::Small);
            p.clipped(value, 296, y, 280, text, UiTextStyle::Small);
        }
        ++row;
    };
    char line[192];
    detail("Class", cls ? cls->name.c_str() : "Not selected");
    std::snprintf(line, sizeof(line), "STR %d  DEX %d  CON %d  INT %d  WIS %d  CHA %d",
                  preview.stats.abilities[0], preview.stats.abilities[1], preview.stats.abilities[2],
                  preview.stats.abilities[3], preview.stats.abilities[4], preview.stats.abilities[5]);
    detail("Final abilities", line);
    detail("Skills", "Not available in this demo");
    if (d.defense)
        detail("Fighter", "Defense / Second Wind");
    if (d.lifeDomain)
        detail("Cleric", "Life Domain / Disciple of Life");
    if (cls && hasFeature(cls->features, ClassFeature::SneakAttack))
        detail("Rogue", "Sneak Attack / back-row reach");
    if (cls && hasFeature(cls->features, ClassFeature::ArcaneRecovery))
        detail("Wizard", "Arcane Recovery");
    if (cls && cls->casting != Casting::None) {
        const int mod = preview.stats.modifiers[cls->casting == Casting::Wisdom ? 4 : 3];
        std::snprintf(line, sizeof(line), "%s / attack %+d / DC %d / prepare 1..%d",
                      cls->casting == Casting::Wisdom ? "WIS" : "INT", preview.stats.proficiency + mod,
                      8 + preview.stats.proficiency + mod, preview.preparation);
        detail("Class casting", line);
    }
    if (d.spells.racial) {
        const int mod = preview.stats.modifiers[3];
        std::snprintf(line, sizeof(line), "INT / attack %+d / DC %d", preview.stats.proficiency + mod,
                      8 + preview.stats.proficiency + mod);
        detail("Racial casting", line);
    }
    const std::uint32_t masks[]{d.spells.cantrips, d.spells.racial, d.spells.learned, d.spells.prepared,
                                d.spells.domain};
    const char* categories[]{"Cantrip", "Racial (INT)", "Spellbook", "Prepared", "Domain grant"};
    for (unsigned category = 0; category < 5; ++category) {
        const auto mask = masks[category];
        const char* label = categories[category];
        for (std::uint16_t i = 0; i < c.spellCount(); ++i)
            if (mask & spellBit({i}))
                detail(label, c.spell({i})->name.c_str());
    }
    std::snprintf(line, sizeof(line), "%.2f / %.2f lb", preview.stats.carriedWeight / 100.0,
                  preview.stats.carryingCapacity / 100.0);
    detail("Weight", line);
    const auto& load = preview.loadout;
    for (std::size_t i = 0; i < load.itemCount; ++i) {
        const auto& item = load.items[i];
        const auto* def = c.item(item.item);
        if (!def)
            continue;
        std::snprintf(line, sizeof(line), "%s x%u", def->name.c_str(), unsigned(item.quantity));
        const bool equipped = load.equipment.body.index == i || load.equipment.mainHand.index == i ||
                              load.equipment.offHand.index == i || load.equipment.ranged.index == i;
        detail(equipped ? "Equipped" : "Carried", line);
    }
    if (offset >= row)
        p.text("End of summary. Scroll up to review choices.", 182, 130, muted);
    return row;
}
void drawCreator(void* context, const UiFonts& fonts, const UiArt& art) {
    auto& view = *static_cast<CreatorView*>(context);
    auto& m = view.model;
    const auto& c = view.catalog;
    HudPainter p(&fonts, &art);
    layoutCreator(view.controls, m, c);
    // Render the underlying screen behind the exclusive discard prompt.
    FlowControls normal;
    const FlowControls* controls = &view.controls;
    if (m.discard) {
        auto copy = m;
        copy.discard = false;
        layoutCreator(normal, copy, c);
        controls = &normal;
    }
    const auto& w = *controls;
    frame(p, "CREATE CHARACTER");
    sidebar(p, c, draftIdentity(m.draft), m.draft.portrait);
    char line[192];
    switch (m.step) {
    case CreatorStep::Identity: {
        p.text("Name, race & portrait", 174, 65, gold, UiTextStyle::Display);
        textField(p, {174, 94, 448, 24}, m.draft.name, m.nameEdit, "Character name");
        std::snprintf(line, sizeof(line), "%zu / 64", std::strlen(m.draft.name.data()));
        p.text(line, 577, 121, muted, UiTextStyle::Small);
        if (m.preview.has(BuildError::Name))
            p.text("Enter 1-64 printable characters, including a non-space.", 174, 121, red,
                   UiTextStyle::Small);
        p.text("Race", 174, 133, gold);
        if (const auto* r = c.race(m.draft.race)) {
            std::snprintf(line, sizeof(line),
                          "Racial bonuses: STR %+d  DEX %+d  CON %+d  INT %+d  WIS %+d  CHA %+d",
                          r->abilityIncreases[0], r->abilityIncreases[1], r->abilityIncreases[2],
                          r->abilityIncreases[3], r->abilityIncreases[4], r->abilityIncreases[5]);
            p.text(line, 174, 181, muted, UiTextStyle::Small);
        }
        p.text("Choose a portrait / appearance never restricts class", 174, 194, gold);
        break;
    }
    case CreatorStep::Class: {
        p.text("Choose your class", 174, 66, gold, UiTextStyle::Display);
        if (const auto* cls = c.characterClass(m.draft.characterClass)) {
            p.text(cls->name.c_str(), 174, 198, gold, UiTextStyle::Display);
            const bool fighter = cls->allowsDefense,
                       cleric = hasFeature(cls->features, ClassFeature::LifeDomain),
                       rogue = hasFeature(cls->features, ClassFeature::SneakAttack);
            wrap(p,
                 fighter  ? "Armored front line. Defense adds 1 AC while armored. Second Wind spends an "
                            "action to heal; rests refresh its charge."
                 : cleric ? "Healing and protection. Life Domain grants Bless and Cure Wounds. Disciple of "
                            "Life strengthens healing. Prepare spells from the cleric list."
                 : rogue  ? "Precision and agility. Sneak Attack adds damage on a qualifying hit once per "
                            "turn. The rogue can strike from the back row."
                          : "Damage and control. Choose a spellbook and prepare spells from it. Arcane "
                            "Recovery restores mana during safe exploration.",
                 {174, 218, 448, 44});
            std::snprintf(line, sizeof(line), "Hit Die d%d / Primary ability: %s / Armor training: %s",
                          cls->hitDie,
                          cleric    ? "Wisdom"
                          : fighter ? "Strength"
                          : rogue   ? "Dexterity"
                                    : "Intelligence",
                          cls->armor ? "Class proficiencies" : "None");
            p.text(line, 174, 273, muted);
            p.text("Only implemented features are offered. Backgrounds and skills are reserved.", 174, 296,
                   muted, UiTextStyle::Small);
        } else
            p.text("Select a class to see its supported features.", 174, 211, muted);
        break;
    }
    case CreatorStep::Abilities: {
        std::snprintf(line, sizeof(line), "Build your abilities / %d of 27 points remaining",
                      27 - m.preview.points);
        p.text(line, 174, 65, gold, UiTextStyle::Display);
        p.text("Ability", 174, 87, muted, UiTextStyle::Small);
        p.centered("Base", {301, 83, 29, 18}, muted, UiTextStyle::Small);
        p.centered("Race", {362, 83, 24, 18}, muted, UiTextStyle::Small);
        p.centered("Final", {393, 83, 24, 18}, muted, UiTextStyle::Small);
        p.centered("Mod", {424, 83, 24, 18}, muted, UiTextStyle::Small);
        constexpr const char* names[]{"Strength",     "Dexterity", "Constitution",
                                      "Intelligence", "Wisdom",    "Charisma"};
        const auto* race = c.race(m.draft.race);
        for (int i = 0; i < 6; ++i) {
            const int y = 107 + i * 28;
            p.text(names[i], 174, y, gold);
            std::snprintf(line, sizeof(line), "%d", m.draft.abilities[i]);
            p.centered(line, {301, y - 4, 29, 19});
            const int bonus = race ? race->abilityIncreases[i] : 0;
            const int values[]{bonus, m.draft.abilities[i] + bonus,
                               abilityModifier(m.draft.abilities[i] + bonus).value};
            for (int col = 0; col < 3; ++col) {
                std::snprintf(line, sizeof(line), col == 1 ? "%d" : "%+d", values[col]);
                p.centered(line, {362 + col * 31, y - 4, 24, 19});
            }
        }
        p.box({493, 92, 129, 186});
        p.text("LIVE PREVIEW", 504, 103, gold);
        std::snprintf(line, sizeof(line), "HP %d\nAC %d\nInitiative %+d\nPrepared allowance %d",
                      m.preview.stats.maxHitPoints, m.preview.stats.armorClass, m.preview.stats.initiative,
                      m.preview.preparation);
        wrap(p, line, {504, 128, 108, 72});
        wrap(p, "Base scores 8-15. Scores 14 and 15 cost two points per increase. Spend all 27 points.",
             {504, 214, 108, 58}, muted, UiTextStyle::Small);
        break;
    }
    case CreatorStep::Skills:
        p.text("Skills", 174, 66, gold, UiTextStyle::Display);
        p.box({174, 99, 448, 201});
        p.centered("Skill selection is coming later", {180, 155, 436, 28}, gold, UiTextStyle::Control);
        p.centered("Skills are not used in this demo. No selection required.", {180, 195, 436, 24});
        break;
    case CreatorStep::Cantrips:
    case CreatorStep::Racial:
    case CreatorStep::Learned:
    case CreatorStep::Prepared:
        drawSelection(p, c, m.selection, m.searchEdit, w, "");
        if (m.step == CreatorStep::Racial)
            p.text("High Elf / Intelligence / separate racial choice", 174, 310, teal, UiTextStyle::Small);
        else if (m.step == CreatorStep::Prepared) {
            std::snprintf(line, sizeof(line), "Level 1 + %s modifier / domain grants do not count",
                          m.draft.lifeDomain ? "WIS" : "INT");
            p.text(line, 174, 310, teal, UiTextStyle::Small);
        }
        break;
    case CreatorStep::Equipment: {
        p.text("Choose starting equipment", 174, 66, gold, UiTextStyle::Display);
        const auto* cfg = c.creation(m.draft.characterClass);
        if (cfg) {
            p.text("Carried supplies", 174, 261, gold);
            for (std::size_t i = 0; i < cfg->supplyCount; ++i) {
                const auto& supply = cfg->supplies[i];
                const auto* item = c.item(supply.item);
                const int x = 174 + int(i % 2) * 137, y = 275 + int(i / 2) * 20;
                p.itemSlot(item, supply.quantity, {x, y, 18, 18}, edge);
                p.clipped(item->name.c_str(), x + 22, y + 5, 108, text, UiTextStyle::Small);
            }
        }
        p.box({458, 92, 164, 212});
        p.text("STARTING LOADOUT", 466, 100, gold);
        const auto& load = m.preview.loadout;
        const OwnedItemId ids[]{load.equipment.body, load.equipment.mainHand, load.equipment.offHand,
                                load.equipment.ranged};
        const char* labels[]{"Body", "Main hand", "Off hand", "Ranged"};
        for (int i = 0; i < 4; ++i) {
            const int x = 466 + (i % 2) * 76, y = 117 + (i / 2) * 50;
            p.text(labels[i], x, y, muted, UiTextStyle::Small);
            const auto* def = ids[i].index < load.itemCount ? c.item(load.items[ids[i].index].item) : nullptr;
            p.itemSlot(def, 1, {x, y + 11, 32, 32}, edge);
            if (def)
                wrap(p, def->name.c_str(), {x + 34, y + 12, 40, 30}, text, UiTextStyle::Small);
        }
        std::snprintf(line, sizeof(line), "AC %d / Weight %.2f / %.2f lb", m.preview.stats.armorClass,
                      m.preview.stats.carriedWeight / 100.0, m.preview.stats.carryingCapacity / 100.0);
        wrap(p, line, {466, 224, 148, 26});
        const auto weapon = load.equipment.mainHand.index;
        if (weapon < m.preview.stats.weapons.size() && m.preview.stats.weapons[weapon].available) {
            const auto& wpn = m.preview.stats.weapons[weapon];
            std::snprintf(line, sizeof(line), "Attack %+d / %dd%d%+d", wpn.attackBonus, wpn.damage.count,
                          wpn.damage.sides, wpn.damage.modifier);
            p.clipped(line, 466, 254, 148, gold, UiTextStyle::Small);
        }
        wrap(p, "Three party camp kits are added once at dungeon launch.", {466, 274, 148, 27}, muted,
             UiTextStyle::Small);
        break;
    }
    case CreatorStep::Review:
        drawBuildReview(p, c, m.draft, m.reviewOffset);
        break;
    default:
        break;
    }
    for (unsigned i = 0; i < w.count; ++i) {
        const auto& b = w.entries[i];
        switch (b.action) {
        case FlowAction::Cancel:
            button(p, w, i, "Cancel");
            break;
        case FlowAction::Back:
            button(p, w, i, "Back");
            break;
        case FlowAction::Next:
            button(p, w, i, "Continue");
            break;
        case FlowAction::Save:
            button(p, w, i, "Save character");
            break;
        case FlowAction::Step: {
            const auto s = CreatorStep(b.value);
            bool isSpell = s >= CreatorStep::Cantrips && s <= CreatorStep::Prepared;
            const char* label = b.rect.y == 36 && isSpell ? "Spells" : creatorStepName(s);
            char change[64];
            if (m.step == CreatorStep::Review && b.rect.y != 36) {
                std::snprintf(change, sizeof(change), "Change %s", label);
                label = change;
            }
            button(p, w, i, label,
                   s == m.step || (b.rect.y == 36 && isSpell && m.step >= CreatorStep::Cantrips &&
                                   m.step <= CreatorStep::Prepared));
            break;
        }
        case FlowAction::Race:
            button(p, w, i, c.race({std::uint16_t(b.value)})->name.c_str(), m.draft.race.index == b.value);
            break;
        case FlowAction::Portrait:
            p.picture(64 + unsigned(b.value), b.rect);
            p.border(b.rect, m.draft.portrait.index == b.value ? teal : i == w.focus ? gold : edge);
            if (m.draft.portrait.index == b.value)
                p.checkmark({b.rect.x + b.rect.width - 16, b.rect.y + 3, 12, 12});
            break;
        case FlowAction::Class: {
            p.box(b.rect, m.draft.characterClass.index == b.value ? teal : i == w.focus ? gold : edge);
            art.creator(32 + unsigned(b.value), {b.rect.x + 25, b.rect.y + 6, 56, 56});
            p.centered(c.characterClass({std::uint16_t(b.value)})->name.c_str(),
                       {b.rect.x, b.rect.y + 65, b.rect.width, 18}, gold, UiTextStyle::Heading);
            break;
        }
        case FlowAction::Minus:
            button(p, w, i, "-");
            break;
        case FlowAction::Plus:
            button(p, w, i, "+");
            break;
        case FlowAction::Reset:
            button(p, w, i, "Reset points");
            break;
        case FlowAction::Recommend:
            if (m.step == CreatorStep::Abilities)
                button(p, w, i, "Recommended allocation");
            break;
        case FlowAction::Gear: {
            const auto* cfg = c.creation(m.draft.characterClass);
            const unsigned group = unsigned(b.value) / 2, option = unsigned(b.value) % 2;
            const auto& o = cfg->groups[group].options[option];
            p.box(b.rect, m.draft.gear[group] == option ? teal : i == w.focus ? gold : edge);
            if (o.count)
                p.picture(UiArt::itemIcon(*c.item(o.items[0].item)), {b.rect.x + 4, b.rect.y + 7, 34, 34});
            wrap(p, o.name.c_str(), {b.rect.x + 43, b.rect.y + 8, 79, 36}, text, UiTextStyle::Small);
            break;
        }
        case FlowAction::Scroll:
            if (m.step == CreatorStep::Review)
                button(p, w, i, b.value < 0 ? "Up" : "Down");
            break;
        default:
            break;
        }
    }
    if (m.step == CreatorStep::Review && !m.preview) {
        for (unsigned e = 1; e <= unsigned(BuildError::Content); ++e)
            if (m.preview.has(BuildError(e))) {
                p.clipped(buildErrorMessage(BuildError(e)), 174, 313, 448, red, UiTextStyle::Small);
                break;
            }
    } else if (m.step == CreatorStep::Review)
        p.text("All required choices complete / Save adds one library entry", 174, 313, teal,
               UiTextStyle::Small);
    if (m.message[0]) {
        p.fill({110, 330, 320, 20}, background);
        wrap(p, m.message.data(), {110, 330, 320, 23}, gold, UiTextStyle::Small);
    }
    if (m.discard) {
        p.fill({0, 0, 640, 360}, {0, 0, 0, 210});
        p.box({185, 126, 300, 114}, gold);
        p.centered("Discard this character?", {195, 139, 280, 24}, gold, UiTextStyle::Control);
        p.centered("Your unsaved choices will be lost.", {195, 175, 280, 18});
        for (unsigned i = 0; i < view.controls.count; ++i)
            button(p, view.controls, i,
                   view.controls.entries[i].action == FlowAction::Keep ? "Keep creating" : "Discard draft");
    }
}
void drawPreparation(const HudPainter& p, const CampHud& h) {
    if (!h.catalog)
        return;
    const auto& c = *h.catalog;
    frame(p, "PREPARE SPELLS");
    sidebar(p, c, h.selection.context.identity, h.selection.context.portrait,
            "Long Rest / learned spells, cantrips and domain grants stay unchanged");
    FlowControls controls = h.controls;
    layoutPreparation(controls, h);
    drawSelection(p, c, h.selection, h.searchEdit, controls, "Prepared spells");
    for (unsigned i = 0; i < controls.count; ++i) {
        const auto& b = controls.entries[i];
        if (b.action == FlowAction::Member)
            button(p, controls, i, h.members[unsigned(b.value)].name,
                   h.preparationMember == unsigned(b.value));
        if (b.action == FlowAction::Done)
            button(p, controls, i, "Done / back to camp");
        if (b.action == FlowAction::Cancel)
            button(p, controls, i, "Cancel changes");
    }
    p.clipped(h.preparationMessage.data(), 174, 350, 448, red, UiTextStyle::Small);
}
void drawAdvancement(const HudPainter& p, const AdvancementHud& h) {
    if (!h.catalog)
        return;
    frame(p, "LEVEL UP");
    sidebar(p, *h.catalog, h.identity, h.portrait);
    constexpr const char* titles[]{"1 Overview", "2 Class features", "3 Spells", "4 Review"};
    const unsigned step = h.step == AdvancementStep::Review ? 3
                          : h.step == AdvancementStep::Learned || h.step == AdvancementStep::Prepared
                              ? 2
                              : unsigned(h.step);
    for (unsigned i = 0; i < 4; ++i)
        p.text(titles[i], 174 + int(i) * 113, 38, i == step ? gold : muted, UiTextStyle::Heading);
    FlowControls controls = h.controls;
    layoutAdvancement(controls, h);
    const auto* cls = h.catalog->characterClass(h.identity.characterClass);
    char line[192];
    if (h.step == AdvancementStep::Learned || h.step == AdvancementStep::Prepared) {
        drawSelection(p, *h.catalog, h.selection, h.searchEdit, controls,
                      h.step == AdvancementStep::Learned ? "Learn new spells" : "Preparation slot");
        const bool learning = h.step == AdvancementStep::Learned;
        if (learning) {
            p.text("Learn 2 spells for your spellbook", 174, 66, gold, UiTextStyle::Heading);
            std::snprintf(line, sizeof(line), "%u known spells stay in your spellbook.",
                          spellCount(h.baseline.learned));
        } else {
            p.text("You gained 1 preparation slot", 174, 66, gold, UiTextStyle::Heading);
            std::snprintf(line, sizeof(line), "%u prepared spells unchanged; %u domain spells always ready.",
                          spellCount(h.baseline.prepared), spellCount(h.selection.context.granted));
        }
        p.text(line, 174, 278, muted, UiTextStyle::Small);
        const auto available = spellCount(h.selection.context.available);
        if (!learning && available == 1) {
            for (unsigned i = 0; i < h.catalog->spellCount(); ++i)
                if (h.selection.context.available & spellBit({std::uint16_t(i)})) {
                    std::snprintf(line, sizeof(line), "%s is the only remaining spell you can add.",
                                  h.catalog->spell({std::uint16_t(i)})->name.c_str());
                    break;
                }
        } else
            std::snprintf(line, sizeof(line), "%u spells available%s", available,
                          learning ? ". Choose 2; preparation comes next."
                                   : "; choose up to 1, or leave the slot empty.");
        wrap(p, line, {174, 289, 266, 18}, text, UiTextStyle::Small, 0, 9);
        wrap(p,
             learning ? "Only new choices are listed; known spells are kept."
             : h.request.level == 1
                 ? "Swap spells during Long Rest. Second-level spells unlock at character level 3."
                 : "Only new choices are listed. Swap preparations during Long Rest.",
             {174, 307, 266, 18}, muted, UiTextStyle::Small, 0, 9);
    } else {
        std::snprintf(line, sizeof(line), "Level %d -> %d / %s", h.request.level, h.request.level + 1,
                      cls->name.c_str());
        p.text(line, 174, 75, gold, UiTextStyle::Display);
        if (h.step == AdvancementStep::Features) {
            wrap(p, advancementFeatures(cls->progression, h.request.level + 1), {174, 105, 440, 170}, text,
                 UiTextStyle::Heading);
        } else {
            const auto* race = h.catalog->race(h.identity.race);
            const int con =
                rules::abilityModifier(h.identity.baseAbilities[2] + race->abilityIncreases[2]).value;
            const int hp = std::max(1, cls->hitDie / 2 + 1 + con) + race->hitPointsPerLevel;
            std::snprintf(line, sizeof(line), "Maximum HP: %d -> %d", h.oldHp, h.oldHp + hp);
            p.text(line, 174, 108);
            std::snprintf(line, sizeof(line), "Maximum mana: %d -> %d", h.oldMana,
                          rules::maximumMana(cls->casting, h.request.level + 1));
            p.text(line, 174, 128);
            p.text("One additional available Hit Die", 174, 148);
            p.text(rules::subclassName(cls->progression, h.request.level + 1, h.identity.lifeDomain), 174,
                   170, gold);
            wrap(p,
                 "Current HP and mana rise with their maximums, preserving deficits. Existing spent charges "
                 "stay spent. Newly earned charges start ready.",
                 {174, 194, 440, 60}, muted);
            if (h.step == AdvancementStep::Review) {
                int rowY = 229;
                const auto selections = [&](const char* prefix, std::uint32_t mask) {
                    if (!mask)
                        return;
                    char names[192]{};
                    std::snprintf(names, sizeof(names), "%s: ", prefix);
                    bool first = true;
                    for (std::uint16_t i = 0; i < h.catalog->spellCount(); ++i)
                        if (mask & spellBit({i})) {
                            const auto used = std::strlen(names);
                            std::snprintf(names + used, sizeof(names) - used, "%s%s", first ? "" : ", ",
                                          h.catalog->spell({i})->name.c_str());
                            first = false;
                        }
                    wrap(p, names, {174, rowY, 440, 24}, text, UiTextStyle::Small);
                    rowY += 15;
                };
                selections("Learn", h.request.choices.learned);
                selections("Prepare", h.request.choices.prepared);
                if (h.preview.preview.valid)
                    selections("Domain", h.preview.preview.spells.domain & ~h.baseline.domain);
                std::snprintf(line, sizeof(line), "%u newly learned / %u newly prepared spells",
                              rules::spellCount(h.request.choices.learned),
                              rules::spellCount(h.request.choices.prepared));
                p.text(line, 174, 265, teal);
                wrap(p, advancementMessage(h.preview.error), {174, 286, 440, 30}, h.preview ? teal : red);
            }
        }
    }
    for (unsigned i = 0; i < controls.count; ++i) {
        const auto a = controls.entries[i].action;
        if (a == FlowAction::Cancel || a == FlowAction::Back || a == FlowAction::Next ||
            a == FlowAction::Done)
            button(p, controls, i,
                   a == FlowAction::Cancel ? "Cancel"
                   : a == FlowAction::Back ? "Back"
                   : a == FlowAction::Next
                       ? (h.step == AdvancementStep::Prepared && !h.selection.selected ? "Leave slot empty"
                                                                                       : "Next")
                       : "Confirm level up");
    }
    p.clipped(h.message.data(), 174, 350, 448, red, UiTextStyle::Small);
}
} // namespace blobber
