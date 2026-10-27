#include "game/commands.hpp"
#include "ui/inventory_layout.hpp"
#include <algorithm>
#include <cstdio>
namespace blobber {
void CommandController::inventoryFeedback(const char* s) noexcept {
    std::snprintf(state_.inventory.feedback.data(), state_.inventory.feedback.size(), "%s", s);
}
void CommandController::openInventory(std::optional<rules::CharacterId> character) noexcept {
    if (!session_ || state_.suspended)
        return;
    const auto selected = character.value_or(state_.selection.character);
    closeOverlay();
    cancel();
    auto& i = state_.inventory;
    i = {};
    i.open = true;
    i.character = party_.character(selected) ? selected : rules::CharacterId{0};
    session_->clearPending();
    refreshInventory();
}
void CommandController::inventoryCharacter(rules::CharacterId c) noexcept {
    auto& i = state_.inventory;
    if (!party_.character(c))
        return;
    if (i.mode == InventoryMode::Transfer) {
        inventoryDrop({c, UINT16_MAX});
        return;
    }
    i.character = c;
    i.descriptionPage = 0;
    i.cursor = 0;
    i.selected = {};
    i.source = {};
    i.mode = InventoryMode::Browse;
    i.pointerDown = i.dragging = false;
    i.feedback = {};
    refreshInventory();
}
void CommandController::refreshInventory() noexcept {
    auto& i = state_.inventory;
    const auto& inv = party_.inventory();
    if (i.open) {
        if (i.revision && i.revision != inv.revision()) {
            i.pointerDown = i.dragging = false;
            i.mode = InventoryMode::Browse;
            i.source = {};
        }
        i.revision = inv.revision();
        for (std::uint16_t cell = 0; cell < 52; ++cell) {
            auto& out = i.cells[cell];
            out = {};
            out.handle = inv.at({i.character, cell});
            if (const auto* s = inv.stack(out.handle)) {
                out.quantity = s->quantity;
                out.weapon = party_.character(i.character)->stats.weapons[cell];
                out.definition = party_.catalog().item(s->item);
            }
        }
        if (!inv.stack(i.selected) || inv.locate(i.selected).character.index != i.character.index)
            i.selected = {};
        const auto* c = party_.character(i.character);
        i.weight = c->stats.carriedWeight;
        i.capacity = c->stats.carryingCapacity;
        i.armorClass = session_ ? session_->effects().armorClass(i.character) : c->stats.armorClass;
        i.management = session_ && session_->combat().safeRecovery(session_->movement().player)
                           ? InventoryError::None
                           : InventoryError::Unsafe;
    }
    if (i.itemTargeting && session_) {
        for (std::size_t n = 0; n < 4; ++n) {
            auto command = i.itemCommand;
            command.ally = state_.cards[n].id;
            i.recipients[n] = session_->combat().validate(command, session_->movement().player);
            state_.cards[n].castEligible = i.recipients[n] == CommandReason::Ready;
            state_.cards[n].castRecipient = state_.cards[n].castEligible && i.itemCommand.ally &&
                                            i.itemCommand.ally->index == state_.cards[n].id.index;
            state_.cards[n].castCaster = i.itemCommand.character.index == state_.cards[n].id.index;
            state_.cards[n].castPreview =
                i.usingItem && i.usingItem->consumable == rules::Consumable::Healing ? "2d4+2 HP" : "+2 mana";
        }
    }
}
void CommandController::inventorySelect(std::uint16_t cell) noexcept {
    auto& i = state_.inventory;
    if (!i.open || cell >= 52)
        return;
    i.cursor = cell;
    if (i.mode == InventoryMode::Move || i.mode == InventoryMode::Split) {
        inventoryDrop({i.character, cell}, i.mode == InventoryMode::Split);
        return;
    }
    if (i.selected != i.cells[cell].handle)
        i.descriptionPage = 0;
    i.selected = i.cells[cell].handle;
    i.source = {};
    i.feedback = {};
}
void CommandController::inventoryExecute(const InventoryRequest& r) noexcept {
    if (!session_)
        return;
    const auto result = session_->inventoryChange(r);
    record(session_->events());
    state_.inventory.mode = InventoryMode::Browse;
    state_.inventory.source = {};
    state_.inventory.pointerDown = state_.inventory.dragging = false;
    inventoryFeedback(inventoryMessage(result.error));
    refreshInventory();
}
void CommandController::inventoryDrop(ItemLocation dest, bool split) noexcept {
    auto& i = state_.inventory;
    const auto h = i.source ? i.source : i.selected;
    const auto* s = party_.inventory().stack(h);
    if (!s) {
        inventoryFeedback(inventoryMessage(InventoryError::InvalidItem));
        return;
    }
    InventoryRequest r;
    r.source = h;
    r.revision = i.revision;
    r.destination = dest;
    r.operation = dest.character.index != i.character.index ? InventoryOp::Transfer
                  : split                                   ? InventoryOp::Split
                                                            : InventoryOp::Move;
    if (split) {
        if (s->quantity < 2 || (dest.character.index == i.character.index &&
                                (dest.cell >= bagCells || dest.cell == party_.inventory().locate(h).cell))) {
            inventoryFeedback("Choose another bag cell for a stack of at least two.");
            return;
        }
        r.quantity = 1;
        if (session_->inventoryAccess(r) != InventoryError::None) {
            inventoryFeedback(inventoryMessage(session_->inventoryAccess(r)));
            return;
        }
        const auto valid = party_.previewInventoryChange(r);
        if (!valid) {
            inventoryFeedback(inventoryMessage(valid.error));
            return;
        }
        i.pending = r;
        i.quantity = 1;
        i.maximum = s->quantity - 1;
        i.mode = InventoryMode::Quantity;
        i.pointerDown = i.dragging = false;
        inventoryFeedback("Choose a quantity. Left/Right adjust; Enter confirms.");
    } else
        inventoryExecute(r);
}
void CommandController::inventoryAction(InventoryAction action) noexcept {
    auto& i = state_.inventory;
    if (!i.open || !session_)
        return;
    const auto* stack = party_.inventory().stack(i.selected);
    if (!stack) {
        inventoryFeedback("Select an item first.");
        return;
    }
    const auto* item = party_.catalog().item(stack->item);
    const auto loc = party_.inventory().locate(i.selected);
    i.source = i.selected;
    if (action == InventoryAction::Move) {
        i.mode = InventoryMode::Move;
        inventoryFeedback("Select a destination cell or equipment slot.");
    } else if (action == InventoryAction::Split) {
        if (stack->quantity < 2) {
            inventoryFeedback("This item cannot be split.");
            return;
        }
        i.mode = InventoryMode::Split;
        inventoryFeedback("Choose a bag cell or another character, then a quantity.");
    } else if (action == InventoryAction::Transfer) {
        i.mode = InventoryMode::Transfer;
        inventoryFeedback("Select another character above (or press 1-4).");
    } else if (action == InventoryAction::Equip) {
        std::uint16_t dest = UINT16_MAX;
        if (loc.cell >= bagCells) {
            for (std::uint16_t k = 0; k < bagCells; ++k)
                if (!i.cells[k].handle) {
                    dest = k;
                    break;
                }
            if (dest == UINT16_MAX) {
                inventoryFeedback(inventoryMessage(InventoryError::Full));
                return;
            }
        } else if (item->kind == rules::ItemKind::Armor)
            dest = 48;
        else if (item->kind == rules::ItemKind::Shield)
            dest = 50;
        else if (item->kind == rules::ItemKind::Weapon)
            dest = item->ranged ? 51 : 49;
        else {
            inventoryFeedback("This item cannot be equipped.");
            return;
        }
        inventoryDrop({i.character, dest});
    } else if (action == InventoryAction::Use) {
        if (item->kind != rules::ItemKind::Consumable) {
            inventoryFeedback("Select a healing or mana potion.");
            return;
        }
        PartyCommand c;
        c.character = i.character;
        c.action = PartyAction::Item;
        c.item = i.selected;
        c.world = state_.combat.world;
        c.turn = state_.combat.turn;
        c.inventoryRevision = party_.inventory().revision();
        c.ally = i.character;
        auto reason = session_->combat().validate(c, session_->movement().player);
        if (reason != CommandReason::Ready && reason != CommandReason::FullHealth &&
            reason != CommandReason::FullMana && reason != CommandReason::DeadAlly) {
            inventoryFeedback(commandMessage(reason));
            return;
        }
        if (reason != CommandReason::Ready)
            for (std::uint16_t n = 0; n < 4; ++n) {
                auto candidate = c;
                candidate.ally = rules::CharacterId{n};
                if (session_->combat().validate(candidate, session_->movement().player) ==
                    CommandReason::Ready) {
                    c = candidate;
                    break;
                }
            }
        closeOverlay();
        i.itemCommand = c;
        i.itemTargeting = true;
        i.usingItem = item;
        inventoryFeedback("Click a living ally to use. Escape cancels.");
        refreshInventory();
    }
}
void CommandController::inventoryDescription(int direction) noexcept {
    auto& page = state_.inventory.descriptionPage;
    page = std::clamp(page + (direction > 0 ? 1 : -1), -32767, 32767);
}
void CommandController::inventoryQuantity(int delta) noexcept {
    auto& i = state_.inventory;
    i.quantity = static_cast<unsigned>(std::clamp<int>(int(i.quantity) + delta, 1, int(i.maximum)));
}
void CommandController::inventoryConfirm() noexcept {
    auto& i = state_.inventory;
    if (i.mode == InventoryMode::Quantity) {
        auto r = i.pending;
        r.quantity = i.quantity;
        inventoryExecute(r);
    } else
        inventorySelect(i.cursor);
}
void CommandController::inventoryCancel() noexcept {
    auto& i = state_.inventory;
    if (i.mode != InventoryMode::Browse || i.pointerDown || i.dragging) {
        i.mode = InventoryMode::Browse;
        i.source = {};
        i.pointerDown = i.dragging = false;
        i.feedback = {};
    } else
        closeOverlay();
}
void CommandController::inventoryPointer(float x, float y, bool press, bool held, bool release,
                                         bool shift) noexcept {
    auto& i = state_.inventory;
    i.pointerX = x;
    i.pointerY = y;
    auto hit = [&]() -> std::uint16_t {
        for (std::uint16_t k = 0; k < 48; ++k)
            if (inventoryLayout::cell(k).contains({x, y}))
                return k;
        for (std::uint16_t k = 0; k < 4; ++k)
            if (inventoryLayout::equipment(k).contains({x, y}))
                return 48 + k;
        return UINT16_MAX;
    };
    if (press) {
        const auto k = hit();
        if (k >= 52)
            return;
        if (i.mode == InventoryMode::Move || i.mode == InventoryMode::Split) {
            inventorySelect(k);
            return;
        }
        inventorySelect(k);
        i.source = i.selected;
        i.pointerDown = bool(i.source);
        i.startX = x;
        i.startY = y;
        i.splitDrag = shift;
    }
    if (i.pointerDown && held && ((x - i.startX) * (x - i.startX) + (y - i.startY) * (y - i.startY) >= 9))
        i.dragging = true;
    if (release) {
        const bool dragging = i.dragging;
        i.pointerDown = i.dragging = false;
        if (!dragging)
            return;
        for (int k = 0; k < 4; ++k)
            if (inventoryLayout::tab(k).contains({x, y})) {
                inventoryDrop({state_.cards[k].id, UINT16_MAX}, i.splitDrag);
                return;
            }
        const auto k = hit();
        if (k < 52)
            inventoryDrop({i.character, k}, i.splitDrag);
    }
}
void CommandController::inventoryAlly(std::size_t index, bool commit) noexcept {
    auto& i = state_.inventory;
    if (!i.itemTargeting || index >= 4)
        return;
    i.itemCommand.ally = state_.cards[index].id;
    if (commit) {
        const auto command = i.itemCommand;
        const auto reason = submit(command);
        inventoryFeedback(commandMessage(reason));
    }
    refreshInventory();
}
void CommandController::inventoryBack() noexcept {
    const auto c = state_.inventory.itemCommand.character;
    openInventory(c);
}
} // namespace blobber
