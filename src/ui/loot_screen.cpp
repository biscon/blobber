#include "ui/hud_painter.hpp"
#include "ui/loot_layout.hpp"
namespace blobber {
void HudPainter::loot(const HudSnapshot& h) const {
    using namespace hudInk;
    const auto& l = h.loot;
    overlayBox(lootLayout::panel);
    text("LOOT BAG", 60, 56, gold, UiTextStyle::Display);
    text("PAUSED", 240, 59, muted, UiTextStyle::Small);
    box(lootLayout::close);
    centered("X", lootLayout::close, gold);
    line({60, 77}, {452, 77}, edge);
    picture(l.portrait, {60, 84, 23, 27});
    char buffer[192];
    std::snprintf(buffer, sizeof(buffer), "Taking items as %s", l.name);
    text(buffer, 90, 84, gold);
    std::snprintf(buffer, sizeof(buffer), "Carry weight: %.2f / %.0f lb", l.weight / 100., l.capacity / 100.);
    text(buffer, 90, 99, muted, UiTextStyle::Small);
    for (unsigned i = 0; i < LootHud::visibleCount; ++i) {
        const auto r = lootLayout::cell(i);
        const auto& e = l.entries[i];
        const bool selected = int(i) == l.hovered;
        itemSlot(e.definition, e.quantity, r, selected ? bright : edge, selected);
        if (e.definition)
            clipped(e.definition->name.c_str(), r.x, r.y + r.height + 2, r.width, muted, UiTextStyle::Small);
    }
    box(lootLayout::info);
    if (l.hovered >= 0 && unsigned(l.hovered) < l.entries.size() && l.entries[l.hovered].definition) {
        const auto& e = l.entries[l.hovered];
        const auto& d = *e.definition;
        picture(UiArt::itemIcon(d), {330, 122, 38, 38});
        clipped(d.name.c_str(), 374, 124, 72, gold);
        constexpr const char* kinds[] = {"Weapon", "Armor",      "Shield",   "Ammunition",
                                         "Gear",   "Consumable", "Currency", "Supply"};
        clipped(kinds[static_cast<unsigned>(d.kind)], 374, 140, 72, muted, UiTextStyle::Small);
        std::snprintf(buffer, sizeof(buffer), "Quantity %u / %.2f lb each", e.quantity, d.weight / 100.);
        clipped(buffer, 330, 166, 116, gold, UiTextStyle::Small);
        using K = rules::ItemKind;
        if (d.kind == K::Weapon)
            std::snprintf(buffer, sizeof(buffer), "Damage %dd%d%s", d.damage.count, d.damage.sides,
                          d.finesse ? " / finesse" : "");
        else if (d.kind == K::Armor)
            std::snprintf(buffer, sizeof(buffer), "Base AC %d", d.baseArmorClass);
        else if (d.kind == K::Shield)
            std::snprintf(buffer, sizeof(buffer), "AC +2");
        else if (d.kind == K::Consumable)
            std::snprintf(buffer, sizeof(buffer), "%s",
                          d.consumable == rules::Consumable::Healing ? "Restores 2d4 + 2 HP"
                                                                     : "Restores 2 mana");
        else
            std::snprintf(buffer, sizeof(buffer), "Stack limit %u", rules::stackLimit(d));
        clipped(buffer, 330, 180, 116, teal, UiTextStyle::Small);
        clipped(d.description.c_str(), 330, 197, 116, muted, UiTextStyle::Small);
        clipped(l.reason, 330, 218, 116, l.canTake ? gold : red, UiTextStyle::Small);
    } else {
        clipped(l.count ? "Hover an item for stats." : "This bag is empty.", 330, 128, 116, muted,
                UiTextStyle::Small);
        clipped("Click a portrait to change", 330, 148, 116, muted, UiTextStyle::Small);
        clipped("who collects the items.", 330, 159, 116, muted, UiTextStyle::Small);
    }
    const unsigned pages = std::max(1u, (l.count + LootHud::visibleCount - 1) / LootHud::visibleCount);
    box(lootLayout::previous, l.page ? gold : edge);
    centered("<", lootLayout::previous, l.page ? gold : muted);
    box(lootLayout::next, l.page + 1 < pages ? gold : edge);
    centered(">", lootLayout::next, l.page + 1 < pages ? gold : muted);
    std::snprintf(buffer, sizeof(buffer), "%u stacks / Page %u of %u / Wheel", l.count, l.page + 1, pages);
    centered(buffer, {88, 234, 194, 15}, muted, UiTextStyle::Small);
    const char* feedback = l.feedback[0] ? l.feedback.data() : "Click to take. Leftovers stay in the bag.";
    // Two measured lines keep partial-collection reasons readable beside the buttons.
    char firstLine[192]{};
    std::snprintf(firstLine, sizeof(firstLine), "%s", feedback);
    auto length = std::strlen(firstLine);
    if (fonts && fonts->width(firstLine, UiTextStyle::Small) > 222) {
        while (length && fonts->width(firstLine, UiTextStyle::Small) > 222)
            firstLine[--length] = 0;
        while (length && firstLine[length - 1] != ' ')
            firstLine[--length] = 0;
        clipped(feedback + length, 60, 266, 222, gold, UiTextStyle::Small);
    }
    clipped(firstLine, 60, 255, 222, gold, UiTextStyle::Small);
    box(lootLayout::takeAll, l.canTakeAny ? gold : edge);
    centered("Take All", lootLayout::takeAll, l.canTakeAny ? gold : muted);
    box(lootLayout::dismiss);
    centered("Close [Esc]", lootLayout::dismiss, gold);
}
} // namespace blobber
