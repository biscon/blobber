#include "ui/inventory_interaction.hpp"
#include "ui/inventory_layout.hpp"
#include <algorithm>
namespace blobber {
bool routeInventoryInput(CommandController& commands, const UiInput& in, bool allowConfirm) noexcept {
    const auto& h = commands.snapshot();
    const auto& i = h.inventory;
    const auto inside = [&](UiRect r) { return in.pointer && r.contains(*in.pointer); };
    const bool click = in.has(UiKey::Click);
    if (!in.focused)
        return i.open || i.itemTargeting;
    if (i.itemTargeting) {
        if (in.has(UiKey::Cancel) || in.has(UiKey::RightClick) ||
            (click && inside(inventoryLayout::useCancel)))
            commands.cancel();
        else if (in.has(UiKey::Back) || (click && inside(inventoryLayout::useBack)))
            commands.inventoryBack();
        else {
            for (std::size_t n = 0; n < 4; ++n)
                if (click && inside(hudLayout::card(int(n))))
                    commands.inventoryAlly(n, allowConfirm);
            if (in.has(UiKey::Left) || in.has(UiKey::Right)) {
                int index = 0;
                for (int n = 0; n < 4; ++n)
                    if (i.itemCommand.ally && h.cards[n].id.index == i.itemCommand.ally->index)
                        index = n;
                commands.inventoryAlly((index + (in.has(UiKey::Left) ? 3 : 1)) % 4, false);
            }
            if (allowConfirm && in.has(UiKey::Confirm))
                for (int n = 0; n < 4; ++n)
                    if (i.itemCommand.ally && h.cards[n].id.index == i.itemCommand.ally->index) {
                        commands.inventoryAlly(n, true);
                        break;
                    }
        }
        return true;
    }
    if (!i.open) {
        if (in.has(UiKey::Inventory) && !h.suspended) {
            commands.openInventory();
            return true;
        }
        if (!h.modal() && !h.magic.casting() && !h.suspended &&
            (in.has(UiKey::Five) || (click && inside(hudLayout::action(4))))) {
            commands.openInventory();
            return true;
        }
        return false;
    }
    if (in.has(UiKey::Cancel) || in.has(UiKey::RightClick)) {
        commands.inventoryCancel();
        return true;
    }
    if (click && inside(inventoryLayout::close)) {
        commands.closeOverlay();
        return true;
    }
    if (in.has(UiKey::Inventory) || (click && inside(inventoryLayout::inventoryPage))) {
        commands.inventoryPage(CharacterPage::Inventory);
        return true;
    }
    if (click && inside(inventoryLayout::characterPage)) {
        commands.inventoryPage(CharacterPage::Character);
        return true;
    }
    if (i.mode == InventoryMode::Quantity) {
        if (in.has(UiKey::Left) || in.has(UiKey::Down) || (click && inside(inventoryLayout::minus)))
            commands.inventoryQuantity(-1);
        if (in.has(UiKey::Right) || in.has(UiKey::Up) || (click && inside(inventoryLayout::plus)))
            commands.inventoryQuantity(1);
        if (in.scroll)
            commands.inventoryQuantity(in.scroll);
        if (in.has(UiKey::Confirm) || (click && inside(inventoryLayout::accept)))
            commands.inventoryConfirm();
        else if (click && inside(inventoryLayout::cancel))
            commands.inventoryCancel();
        return true;
    }
    for (int n = 0; n < 4; ++n) {
        if (in.has(static_cast<UiKey>(n)) || (click && inside(inventoryLayout::tab(n)))) {
            commands.inventoryCharacter(h.cards[n].id);
            return true;
        }
    }
    if (i.page == CharacterPage::Character) {
        if (click && inside(inventoryLayout::levelUp))
            commands.openAdvancement(i.character);
        return true;
    }
    if (in.scroll)
        commands.inventoryDescription(in.scroll > 0 ? -1 : 1);
    if (in.has(UiKey::PreviousPage))
        commands.inventoryDescription(-1);
    if (in.has(UiKey::NextPage))
        commands.inventoryDescription(1);
    constexpr UiKey keys[]{UiKey::Equip, UiKey::TransferItem, UiKey::UseItem};
    for (unsigned n = 0; n < i.actions.size(); ++n) {
        const auto action = static_cast<InventoryAction>(n);
        if (i.actions[n].visible && ((n < std::size(keys) && in.has(keys[n])) ||
                                     (click && inside(inventoryLayout::action(i, action))))) {
            commands.inventoryAction(action);
            return true;
        }
    }
    if (in.has(UiKey::Tab))
        commands.inventorySelect(i.cursor < 48 ? 48 : 0);
    const int delta = in.has(UiKey::Left)    ? -1
                      : in.has(UiKey::Right) ? 1
                      : in.has(UiKey::Up)    ? (i.cursor < 48 ? -8 : -1)
                      : in.has(UiKey::Down)  ? (i.cursor < 48 ? 8 : 1)
                                             : 0;
    if (delta) {
        const int cell = std::clamp(int(i.cursor) + delta, 0, 51);
        commands.inventorySelect(static_cast<std::uint16_t>(cell));
    }
    if (in.has(UiKey::Confirm))
        commands.inventoryConfirm();
    if (in.pointer)
        commands.inventoryPointer(in.pointer->x, in.pointer->y, click, in.pointerHeld, in.pointerReleased,
                                  in.shift);
    else if (in.pointerReleased)
        commands.inventoryPointer(-100, -100, false, false, true, false);
    return true;
}
} // namespace blobber
