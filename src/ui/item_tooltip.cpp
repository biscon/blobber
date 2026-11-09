#include "ui/hud_painter.hpp"

namespace blobber {
using namespace hudInk;
namespace {
const char* name(const rules::ItemDefinition& item) {
    return item.id == "camp_kit" ? "Ration pack" : item.name.c_str();
}
const char* description(const rules::ItemDefinition& item) {
    return item.description.c_str();
}
struct TooltipText {
    std::array<std::array<char, 160>, 48> rows{};
    unsigned count = 0;
    void add(const char* line) {
        if (count < rows.size())
            std::snprintf(rows[count++].data(), rows[0].size(), "%s", line);
    }
    void wrap(const HudPainter& p, const char* cursor, int width) {
        while (cursor && *cursor && count < rows.size()) {
            while (*cursor == ' ' || *cursor == '\n')
                ++cursor;
            if (!*cursor)
                break;
            char line[160]{};
            unsigned n = 0, space = 0;
            while (cursor[n] && cursor[n] != '\n' && n < sizeof(line) - 1) {
                line[n] = cursor[n];
                line[n + 1] = 0;
                if (p.fonts && p.fonts->width(line, UiTextStyle::Small) > width) {
                    line[n] = 0;
                    break;
                }
                if (cursor[n] == ' ')
                    space = n;
                ++n;
            }
            if (!n)
                n = 1;
            else if (cursor[n] && cursor[n] != '\n' && space)
                n = space;
            std::snprintf(line, sizeof(line), "%.*s", int(n), cursor);
            add(line);
            cursor += n;
        }
    }
};
void itemText(TooltipText& lines, const HudPainter& p, const rules::ItemDefinition& item,
              const rules::WeaponPreview& weapon, int armorClass, int width) {
    char value[160];
    std::snprintf(value, sizeof(value), "Weight: %.2f lb each", double(item.weight) / 100);
    lines.add(value);
    if (item.kind == rules::ItemKind::Weapon) {
        std::snprintf(value, sizeof(value), "Hit %+d | Damage %dd%d%+d", weapon.attackBonus,
                      weapon.damage.count, weapon.damage.sides, weapon.damage.modifier);
        lines.add(value);
        std::snprintf(value, sizeof(value), "%s | %s", rules::damageTypeName(item.damageType),
                      weapon.proficient ? "Trained" : "Untrained");
        lines.add(value);
        if (item.ranged)
            lines.add(rules::ammunitionFor(item) == rules::Ammunition::Bolt ? "Ammunition: bolts"
                                                                            : "Ammunition: arrows");
        if (weapon.heavyDisadvantage)
            lines.add("Heavy: disadvantage for Small wielder");
        if (item.loading)
            lines.add("Loading: one shot per action");
        if (item.twoHanded)
            lines.add("Two-handed");
        else if (item.versatileSides)
            lines.add("Versatile: damage reflects current shield");
    } else if (item.kind == rules::ItemKind::Armor) {
        constexpr const char* types[]{"Light", "Medium", "Heavy", "Shield"};
        std::snprintf(value, sizeof(value), "%s armor | Base AC %d", types[unsigned(item.armor)],
                      item.baseArmorClass);
        lines.add(value);
        std::snprintf(value, sizeof(value), "Gear AC if equipped: %d", armorClass);
        lines.add(value);
    } else if (item.kind == rules::ItemKind::Shield) {
        lines.add("Shield | AC +2");
        std::snprintf(value, sizeof(value), "Gear AC if equipped: %d", armorClass);
        lines.add(value);
    }
    lines.add("");
    lines.wrap(p, description(item), width);
}
} // namespace
void HudPainter::itemTooltip(const rules::ItemDefinition& item, const ItemComparison& comparison,
                             const char* heading, const char* characterName, ArtPoint pointer, UiRect bounds,
                             const char* summary, const char* detail) const {
    constexpr int width = 234, gap = 4;
    TooltipText offered, equipped;
    unsigned warningStart = unsigned(offered.rows.size());
    char lineText[160];
    if (summary)
        offered.add(summary);
    if (detail)
        offered.add(detail);
    itemText(offered, *this, item, comparison.offeredWeapon, comparison.offeredArmorClass, width - 16);
    if (item.wearSlot != rules::WearSlot::None) {
        char delta[160];
        const auto& a = comparison.currentStats;
        const auto& b = comparison.offeredStats;
        std::snprintf(delta, sizeof(delta), "If equipped: AC %+d | HP %+d | Mana %+d",
                      b.armorClass - a.armorClass, b.maxHitPoints - a.maxHitPoints,
                      b.maxManaBonus - a.maxManaBonus);
        offered.wrap(*this, delta, width - 16);
        std::snprintf(delta, sizeof(delta), "Spell hit %+d | Initiative %+d | Saves %+d/%+d/%+d/%+d/%+d/%+d",
                      b.spellAttackBonus - a.spellAttackBonus, b.initiative - a.initiative,
                      b.saves[0] - a.saves[0], b.saves[1] - a.saves[1], b.saves[2] - a.saves[2],
                      b.saves[3] - a.saves[3], b.saves[4] - a.saves[4], b.saves[5] - a.saves[5]);
        offered.wrap(*this, delta, width - 16);
    }
    if (comparison.restriction[0]) {
        offered.add("");
        warningStart = offered.count;
        offered.wrap(*this, comparison.restriction, width - 16);
    }
    if (comparison.equippable) {
        if (comparison.equipped)
            itemText(equipped, *this, *comparison.equipped, comparison.equippedWeapon,
                     comparison.equippedArmorClass, width - 16);
        else
            equipped.add("Empty: nothing equipped in this slot.");
    }
    const auto rows =
        std::min(std::max(offered.count, equipped.count), unsigned(std::max(1, (bounds.height - 48) / 10)));
    const int height = 48 + int(rows) * 10;
    const int totalWidth = comparison.equippable ? width * 2 + gap : width;
    const int x = std::clamp(int(pointer.x) + 8, bounds.x, bounds.x + bounds.width - totalWidth);
    const int y = std::clamp(int(pointer.y) + 8, bounds.y, bounds.y + bounds.height - height);
    const auto panel = [&](int px, const char* heading, const rules::ItemDefinition* definition,
                           const TooltipText& lines, bool current, unsigned warningRow) {
        overlayBox({px, y, width, height}, current ? edge : gold);
        clipped(heading, px + 8, y + 7, width - 16, current ? muted : gold, UiTextStyle::Small);
        if (definition) {
            picture(UiArt::itemIcon(*definition), {px + 8, y + 21, 20, 20});
            clipped(name(*definition), px + 34, y + 24, width - 42, gold, UiTextStyle::Display);
        } else
            text("Empty", px + 8, y + 25, gold, UiTextStyle::Display);
        const auto visible = std::min(lines.count, rows);
        const auto warnings = warningRow < lines.count ? std::min(lines.count - warningRow, visible) : 0;
        const auto prefix = visible - warnings;
        for (unsigned i = 0; i < visible; ++i) {
            // Keep equip restrictions visible even when a long description must be clipped.
            const auto source = i < prefix ? i : lines.count - warnings + i - prefix;
            const bool ellipsis = lines.count > visible && i < prefix && i + 1 == prefix;
            clipped(ellipsis ? "..." : lines.rows[source].data(), px + 8, float(y + 45 + int(i) * 10),
                    width - 16, source >= warningRow ? red : hudInk::text, UiTextStyle::Small);
        }
    };
    panel(x, heading, &item, offered, false, warningStart);
    if (comparison.equippable) {
        std::snprintf(lineText, sizeof(lineText), "CURRENTLY EQUIPPED / %s", characterName);
        panel(x + width + gap, lineText, comparison.equipped, equipped, true, unsigned(equipped.rows.size()));
    }
}
void HudPainter::inventoryTooltip(const HudSnapshot& h) const {
    const auto& i = h.inventory;
    if (!i.open || i.page != CharacterPage::Inventory || i.mode != InventoryMode::Browse || i.pointerDown ||
        i.dragging || h.suspended || !i.tooltipVisible || i.hovered < 0 || i.hovered >= int(bagCells))
        return;
    const auto& cell = i.cells[unsigned(i.hovered)];
    if (!cell.definition)
        return;
    const char* characterName = "";
    for (const auto& card : h.cards)
        if (card.id.index == i.character.index)
            characterName = card.name;
    char summary[48];
    std::snprintf(summary, sizeof(summary), "Quantity: %u", cell.quantity);
    itemTooltip(*cell.definition, i.comparison, "BAG ITEM", characterName, {i.pointerX, i.pointerY},
                {8, 65, 624, 289}, summary);
}
} // namespace blobber
