#include "ui/hud_painter.hpp"
#include "ui/inventory_layout.hpp"
namespace blobber {
using namespace hudInk;
namespace {
void wrapped(const HudPainter& p, const char* value, int x, int y, int width, int maxLines) {
    if (!value || !p.fonts)
        return;
    const char* cursor = value;
    for (int row = 0; row < maxLines && *cursor; ++row) {
        while (*cursor == ' ')
            ++cursor;
        char line[192]{};
        std::size_t n = 0, space = 0;
        while (cursor[n] && n < sizeof(line) - 2) {
            line[n] = cursor[n];
            line[n + 1] = 0;
            if (p.fonts->width(line, UiTextStyle::Small) > width) {
                line[n] = 0;
                break;
            }
            if (cursor[n] == ' ')
                space = n;
            ++n;
        }
        if (!n)
            n = 1;
        if (cursor[n] && space) {
            n = space;
            line[n] = 0;
        }
        p.clipped(line, x, float(y + row * 10), width, text, UiTextStyle::Small);
        cursor += n;
    }
}
// Descriptions can contain 512 ASCII bytes. Paginate measured lines without heap
// storage so every accepted description remains readable at a fixed panel size.
void description(const HudPainter& p, const char* value, int page) {
    if (!value || !p.fonts)
        return;
    struct Row {
        const char* first;
        unsigned size;
    };
    std::array<Row, 512> rows{};
    unsigned count = 0;
    const char* cursor = value;
    while (*cursor && count < rows.size()) {
        while (*cursor == ' ')
            ++cursor;
        if (!*cursor)
            break;
        char line[192]{};
        unsigned n = 0, space = 0;
        while (cursor[n] && n < sizeof(line) - 2) {
            line[n] = cursor[n];
            line[n + 1] = 0;
            if (p.fonts->width(line, UiTextStyle::Small) > 136)
                break;
            if (cursor[n] == ' ')
                space = n;
            ++n;
        }
        if (!n)
            n = 1;
        else if (cursor[n] && space)
            n = space;
        rows[count++] = {cursor, n};
        cursor += n;
    }
    const int pages = std::max(1, int((count + 3) / 4));
    const unsigned first = unsigned((page % pages + pages) % pages) * 4;
    for (unsigned r = first; r < std::min(first + 4, count); ++r) {
        char line[192]{};
        std::snprintf(line, sizeof(line), "%.*s", int(rows[r].size), rows[r].first);
        p.clipped(line, 490, float(151 + (r - first) * 10), 136, text, UiTextStyle::Small);
    }
    if (pages > 1) {
        char hint[64];
        std::snprintf(hint, sizeof(hint), "Description %u/%d / mouse wheel", first / 4 + 1, pages);
        p.clipped(hint, 490, 194, 136, muted, UiTextStyle::Small);
    }
}
} // namespace
void HudPainter::inventory(const HudSnapshot& h) const {
    const auto& i = h.inventory;
    fill({0, 0, 640, 360}, background);
    overlayBox(inventoryLayout::panel);
    text("INVENTORY", inventoryLayout::inventoryPage.x, 11,
         i.page == CharacterPage::Inventory ? bright : gold, UiTextStyle::Display);
    line({81, 10}, {81, 25}, edge);
    text("CHARACTER", inventoryLayout::characterPage.x, 11,
         i.page == CharacterPage::Character ? bright : gold, UiTextStyle::Display);
    const auto activePage =
        i.page == CharacterPage::Inventory ? inventoryLayout::inventoryPage : inventoryLayout::characterPage;
    const auto activeLabel = i.page == CharacterPage::Inventory ? "INVENTORY" : "CHARACTER";
    const float tabWidth = fonts ? fonts->width(activeLabel, UiTextStyle::Display) : activePage.width;
    line({float(activePage.x), 23}, {activePage.x + tabWidth, 23}, bright, 2);
    text("PAUSED", 282, 13, teal, UiTextStyle::Small);
    box(inventoryLayout::close);
    centered("X", inventoryLayout::close, gold);
    for (int n = 0; n < 4; ++n) {
        const auto r = inventoryLayout::tab(n);
        const auto& c = h.cards[n];
        box(r, c.id.index == i.character.index ? bright : edge);
        picture(c.portrait, {r.x + 3, r.y + 3, 19, 22});
        char line[96];
        std::snprintf(line, sizeof(line), "%d  %s", n + 1, c.name);
        clipped(line, r.x + 28, r.y + 4, r.width - 32, gold, UiTextStyle::Label);
        clipped(c.className, r.x + 28, r.y + 16, r.width - 32, muted, UiTextStyle::Small);
    }
    if (i.page == CharacterPage::Character) {
        characterSheet(h);
        return;
    }
    text("EQUIPPED", 16, 69, gold, UiTextStyle::Display);
    text("BAG / 48 SLOTS", 178, 69, gold, UiTextStyle::Display);
    text("ITEM DETAILS", 490, 69, gold, UiTextStyle::Display);
    picture(UiArt::silhouetteIcon, {32, 93, 118, 200}, {160, 165, 154, 210});
    const InventoryCell* selected = nullptr;
    auto drawCell = [&](const InventoryCell& c, UiRect r, unsigned index) {
        box(r, i.cursor == index ? teal : c.handle && c.handle == i.selected ? bright : edge);
        if (c.definition) {
            picture(UiArt::itemIcon(*c.definition), {r.x + 2, r.y + 2, r.width - 4, r.height - 4});
            if (c.quantity > 1) {
                char q[12];
                std::snprintf(q, sizeof(q), "%u", c.quantity);
                fill({r.x + r.width - 17, r.y + r.height - 11, 16, 10}, background);
                rightAligned(q, {r.x + r.width - 20, r.y + r.height - 12, 20, 12}, gold, UiTextStyle::Small);
            }
            if (c.handle == i.selected) {
                selected = &c;
                border(r, bright);
            }
        }
    };
    for (int n = 0; n < 48; ++n)
        drawCell(i.cells[n], inventoryLayout::cell(n), n);
    constexpr const char* slots[]{"BODY", "MAIN HAND", "SHIELD", "RANGED"};
    for (int n = 0; n < 4; ++n) {
        const auto r = inventoryLayout::equipment(n);
        drawCell(i.cells[48 + n], r, 48 + n);
        centered(slots[n], {r.x - 8, r.y - 13, r.width + 16, 12}, gold, UiTextStyle::Small);
    }
    char line[128];
    std::snprintf(line, sizeof(line), "AC %d", i.armorClass);
    text(line, 16, 294, gold);
    std::snprintf(line, sizeof(line), "Weight %.2f / %.0f lb", double(i.weight) / 100,
                  double(i.capacity) / 100);
    text(line, 178, 316, i.weight > i.capacity ? red : gold, UiTextStyle::Label);
    if (selected) {
        const auto& d = *selected->definition;
        picture(UiArt::itemIcon(d), {490, 86, 46, 46});
        wrapped(*this, d.name.c_str(), 541, 88, 85, 3);
        std::snprintf(line, sizeof(line), "%u  /  %.2f lb each", selected->quantity, double(d.weight) / 100);
        clipped(line, 490, 136, 136, gold, UiTextStyle::Small);
        description(*this, d.description.c_str(), i.descriptionPage);
        if (d.kind == rules::ItemKind::Weapon) {
            const auto& w = selected->weapon;
            std::snprintf(line, sizeof(line), "Hit %+d / %dd%d%+d%s", w.attackBonus, w.damage.count,
                          w.damage.sides, w.damage.modifier, w.proficient ? "" : " / untrained");
        } else if (d.kind == rules::ItemKind::Armor)
            std::snprintf(line, sizeof(line), "Base AC %d / armor training", d.baseArmorClass);
        else if (d.kind == rules::ItemKind::Shield)
            std::snprintf(line, sizeof(line), "AC +2 / shield training");
        else
            std::snprintf(line, sizeof(line), "Stack limit %u", rules::stackLimit(d));
        clipped(line, 490, 209, 136, teal, UiTextStyle::Small);
    } else
        wrapped(*this,
                "Select an item to inspect it. Drag to move, equip, merge or transfer. Shift-drag splits a "
                "stack.",
                490, 91, 136, 10);
    for (unsigned n = 0; n < i.actions.size(); ++n) {
        const auto& action = i.actions[n];
        if (!action.visible)
            continue;
        const auto rect = inventoryLayout::action(i, static_cast<InventoryAction>(n));
        box(rect, action.enabled ? edge : Color{47, 50, 44, 255});
        centered(action.label, rect, action.enabled ? gold : muted, UiTextStyle::Small);
        if (!action.enabled)
            clipped(action.reason, rect.x, rect.y + 22, rect.width, muted, UiTextStyle::Small);
    }
    clipped("Drag: move / same item: merge / Shift-drag: split", 178, 330, 300, muted, UiTextStyle::Small);
    clipped(i.management == InventoryError::None ? "Equipment & transfers available"
                                                 : "Equipment & transfers locked in danger",
            16, 313, 155, i.management == InventoryError::None ? muted : red, UiTextStyle::Small);
    clipped(i.feedback[0] ? i.feedback.data()
                          : "1-4: character / Arrows: select / Tab: "
                            "equipment / Esc: close",
            14, 342, 610, i.feedback[0] ? teal : muted, UiTextStyle::Small);
    if (i.dragging) {
        for (const auto& c : i.cells)
            if (c.handle == i.source && c.definition) {
                const UiRect r{int(i.pointerX) - 16, int(i.pointerY) - 16, 32, 32};
                picture(UiArt::itemIcon(*c.definition), r, {255, 255, 255, 190});
                border(r, teal);
            }
    }
    if (i.mode == InventoryMode::Quantity) {
        fill({0, 0, 640, 360}, {0, 0, 0, 150});
        overlayBox(inventoryLayout::dialog);
        text("SPLIT STACK", 225, 132, gold, UiTextStyle::Display);
        std::snprintf(line, sizeof(line), "Quantity %u / %u", i.quantity, i.maximum);
        centered(line, {257, 162, 124, 22}, hudInk::text);
        box(inventoryLayout::minus);
        centered("-", inventoryLayout::minus, gold, UiTextStyle::Control);
        box(inventoryLayout::plus);
        centered("+", inventoryLayout::plus, gold, UiTextStyle::Control);
        box(inventoryLayout::accept);
        centered("CONFIRM", inventoryLayout::accept, gold);
        box(inventoryLayout::cancel);
        centered("CANCEL", inventoryLayout::cancel, gold);
    }
}
void HudPainter::itemTargets(const HudSnapshot& h) const {
    const auto& i = h.inventory;
    box({4, 304, 376, 54});
    text(i.usingItem ? i.usingItem->name.c_str() : "USE ITEM", 12, 309, gold, UiTextStyle::Display);
    clipped("Click an eligible ally / Enter confirms selection", 12, 322, 360, hudInk::text,
            UiTextStyle::Small);
    const char* reason = "Choose a recipient.";
    for (int n = 0; n < 4; ++n)
        if (i.itemCommand.ally && i.itemCommand.ally->index == h.cards[n].id.index)
            reason = i.recipients[n] == CommandReason::Ready
                         ? (realTime(h.combat.phase) ? "Ready / one potion / 6s cooldown"
                                                     : "Ready / one potion / one action")
                         : commandMessage(i.recipients[n]);
    clipped(reason, 12, 340, 200, teal, UiTextStyle::Small);
    box(inventoryLayout::useBack);
    centered("BACK / B", inventoryLayout::useBack, gold, UiTextStyle::Small);
    box(inventoryLayout::useCancel);
    centered("CANCEL", inventoryLayout::useCancel, gold, UiTextStyle::Small);
}
} // namespace blobber
