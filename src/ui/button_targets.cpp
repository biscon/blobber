#include "ui/button_targets.hpp"
#include "ui/camp_layout.hpp"
#include "ui/dialogue_layout.hpp"
#include "ui/inventory_layout.hpp"
#include "ui/loot_layout.hpp"
#include "ui/save_layout.hpp"
#include "ui/trade_layout.hpp"
#include "ui/travel_layout.hpp"
namespace blobber {
ButtonTargets hudButtonTargets(const HudSnapshot& h) noexcept {
    ButtonTargets t;
    t.context = {h.combat.world,
                 h.combat.turn,
                 0,
                 unsigned(h.combat.phase),
                 h.inventory.revision,
                 h.selection.character.index,
                 0,
                 0};
    auto panel = [&](unsigned n) { t.context[2] = n; };
    auto cards = [&] {
        for (int i = 0; i < 4; ++i)
            t.add(hudLayout::card(i));
    };
    if (h.trade.open) {
        const auto& trade = h.trade;
        panel(16);
        t.context[4] = trade.inventoryRevision;
        t.context[5] = trade.merchantRevision;
        t.context[6] = trade.character.index | (std::uint64_t(trade.page) << 16);
        t.context[7] =
            trade.selected | (std::uint64_t(trade.side) << 8) | (std::uint64_t(trade.quantity) << 16);
        cards();
        t.add(tradeLayout::close);
        t.add(tradeLayout::back);
        t.add(tradeLayout::previous, trade.page > 0);
        t.add(tradeLayout::next, trade.page + 1 < trade.pages);
        t.add(tradeLayout::minus, trade.selected < 48 && trade.quantity > 1);
        t.add(tradeLayout::plus, trade.selected < 48 && trade.quantity < trade.maximum);
        t.add(tradeLayout::accept, trade.canTrade);
        for (const auto side : {TradeSide::Merchant, TradeSide::Player})
            for (unsigned i = 0; i < 48; ++i)
                t.add(tradeLayout::cell(side, i), true, true);
        return t;
    }
    if (h.dialogue.open) {
        panel(15);
        t.context[4] = h.dialogue.revision;
        t.context[5] = h.dialogue.offset;
        t.context[6] = unsigned(h.dialogue.introduction) + 2u * unsigned(h.dialogue.revealing);
        t.context[7] = h.dialogue.leaving;
        if (!h.dialogue.leaving) {
            for (unsigned i = 0; i < h.dialogue.count; ++i)
                t.add(dialogueLayout::row(i));
            if (h.dialogue.pages > 1)
                t.add(dialogueLayout::advance, dialogueLayout::advanceEnabled(h.dialogue));
            t.add(dialogueLayout::previous, h.dialogue.offset > 0);
            t.add(dialogueLayout::next, h.dialogue.offset + h.dialogue.count < h.dialogue.totalTopics);
        }
        return t;
    }
    if (h.targetPicker.open && h.targetPicker.available && !h.modal()) {
        panel(14);
        t.context[4] = h.targetPicker.revision;
        t.context[6] = h.targetPage;
        t.context[7] = unsigned(h.selection.action) + 256u * unsigned(h.magic.kind);
        for (unsigned i = 0; i < h.targetPicker.count; ++i)
            t.add(hudLayout::enemyTargetRow(i, h.targetPicker.count), true, false,
                  0x20000u + h.targetPicker.rows[i].id.slot);
        t.add(hudLayout::enemyPrevious, h.targetPicker.candidateCount > 4);
        t.add(hudLayout::enemyNext, h.targetPicker.candidateCount > 4);
        t.add(hudLayout::enemyConfirm, h.targetPicker.canConfirm);
        t.add(hudLayout::enemyTargetButton(h.magic.armed));
        t.add({0, 0, artWidth, artHeight}, true, true); // Dismiss outside without a world action.
        return t;
    }
    if (h.menuOpen && h.save.enabled) {
        panel(1);
        t.context[6] = unsigned(h.save.confirmation);
        t.context[7] = h.save.terminalWarning;
        for (unsigned i = 0; i < saveLayout::rowCount(h.save); ++i)
            t.add(saveLayout::row(h.save, i), h.save.confirmation != SaveConfirmation::None ||
                                                  h.save.terminalWarning || (i != 1 && i != 2) ||
                                                  h.save.allowed);
    } else if (h.travel.open) {
        panel(2);
        t.context[5] = h.travel.cursor;
        t.context[6] = h.travel.hub;
        t.context[7] = h.travel.count;
        t.add(travelLayout::cancel);
        t.add(travelLayout::confirm, h.travel.preview.accepted);
        if (h.travel.hub)
            for (unsigned i = 0; i < h.travel.count; ++i)
                t.add(travelLayout::row(i));
    } else if (h.advancement.open || (h.camp.open && h.camp.preparing)) {
        // These panels have their own FlowControls capture and stable action IDs.
        panel(3);
    } else if (h.leaveConfirmation) {
        panel(4);
        t.add({242, 286, 156, 24}, !h.leaveConfirmation);
        t.add(hudLayout::resume);
        t.add(hudLayout::mainMenu);
    } else if (h.loot.open) {
        panel(5);
        t.context[6] = h.loot.page;
        t.context[7] = h.loot.recipient.index;
        t.add(lootLayout::close);
        t.add(lootLayout::dismiss);
        cards();
        t.add(lootLayout::takeAll, h.loot.canTakeAny);
        t.add(lootLayout::previous, h.loot.page > 0);
        t.add(lootLayout::next, (h.loot.page + 1) * LootHud::visibleCount < h.loot.count);
        for (unsigned i = 0; i < LootHud::visibleCount; ++i)
            t.add(lootLayout::cell(i), h.loot.entries[i].quantity > 0);
    } else if (h.camp.open) {
        panel(6);
        t.context[6] = h.camp.playing;
        t.context[7] = unsigned(h.camp.request.kind);
        t.add(campLayout::cancel);
        if (!h.camp.playing) {
            t.add(campLayout::shortRest);
            t.add(campLayout::longRest);
            t.add(campLayout::start, bool(h.camp.preview));
            t.add(campLayout::prepare, h.camp.request.kind == RestKind::Long && h.camp.canPrepare);
            for (unsigned i = 0; i < 4; ++i) {
                t.add(campLayout::minus(i), h.camp.request.dice[i] > 0);
                t.add(campLayout::plus(i), h.camp.request.dice[i] < h.camp.members[i].available);
            }
        }
    } else if (h.inventory.itemTargeting) {
        panel(7);
        t.add(inventoryLayout::useBack);
        t.add(inventoryLayout::useCancel);
        for (int i = 0; i < 4; ++i)
            t.add(hudLayout::card(i), h.inventory.recipients[i] == CommandReason::Ready);
    } else if (h.inventory.open) {
        panel(8);
        t.context[5] = h.inventory.character.index;
        t.context[6] = unsigned(h.inventory.mode);
        t.context[7] = unsigned(h.inventory.page) | (std::uint64_t(h.inventory.quantity) << 8) |
                       (std::uint64_t(h.inventory.cursor) << 40);
        t.add(inventoryLayout::close);
        t.add(inventoryLayout::inventoryPage);
        t.add(inventoryLayout::characterPage);
        if (h.inventory.mode == InventoryMode::Quantity) {
            t.add(inventoryLayout::minus, h.inventory.quantity > 1);
            t.add(inventoryLayout::plus, h.inventory.quantity < h.inventory.maximum);
            t.add(inventoryLayout::accept);
            t.add(inventoryLayout::cancel);
        } else {
            for (int i = 0; i < 4; ++i)
                t.add(inventoryLayout::tab(i));
            if (h.inventory.page == CharacterPage::Character)
                t.add(inventoryLayout::levelUp);
            else {
                for (unsigned i = 0; i < h.inventory.actions.size(); ++i)
                    if (h.inventory.actions[i].visible)
                        t.add(inventoryLayout::action(h.inventory, InventoryAction(i)),
                              h.inventory.actions[i].enabled, false, 0x10000u + i);
                for (int i = 0; i < 48; ++i)
                    t.add(inventoryLayout::cell(i), true, true);
                for (int i = 0; i < 4; ++i)
                    t.add(inventoryLayout::equipment(i), true, true);
            }
        }
    } else if (h.magic.open) {
        panel(9);
        t.context[6] = h.magic.tab;
        t.context[7] = h.magic.offset;
        t.add(hudLayout::spellbookClose);
        t.add(hudLayout::spellPrevious, h.magic.offset > 0);
        t.add(hudLayout::spellNext, h.magic.offset + 7 < h.magic.visibleCount);
        int x = 14;
        for (unsigned i = 0; i < h.magic.tabs.size(); ++i)
            if (h.magic.tabs[i]) {
                const auto r = hudLayout::spellTab(int(i), x);
                t.add(r);
                x += r.width + 2;
            }
        for (int i = 0; i < hudLayout::spellRows; ++i)
            if (h.magic.offset + i < h.magic.visibleCount)
                t.add(hudLayout::spellEntry(i));
        // Outside-click dismissal is a surface interaction, not a button.
        t.add({0, 0, artWidth, artHeight}, true, true);
    } else if (h.logExpanded) {
        panel(10);
        t.add(hudLayout::closeOverlay);
        t.add(hudLayout::logExpand);
        t.add(hudLayout::logLatest);
    } else if (h.menuOpen) {
        panel(11);
        t.add({242, 286, 156, 24}, !h.leaveConfirmation);
        t.add(hudLayout::resume);
        t.add(hudLayout::gear);
        t.add(hudLayout::rest);
        t.add(hudLayout::quit);
        t.add(hudLayout::mainMenu, h.canReturnToMenu);
        if (h.development)
            t.add(hudLayout::developmentXp);
    } else if (h.campaignTerminal || h.combat.phase == CombatPhase::Victory ||
               h.combat.phase == CombatPhase::Defeat) {
        panel(13);
        if (h.arena && (h.arenaComplete || h.combat.phase == CombatPhase::Defeat)) {
            t.add(hudLayout::arenaRematch);
            t.add(hudLayout::arenaEdit);
        } else
            t.add(hudLayout::resultButton);
    } else {
        panel(12);
        t.context[6] = unsigned(h.magic.state);
        t.context[7] = unsigned(h.magic.kind);
        t.add(hudLayout::gear);
        t.add(hudLayout::travelButton, h.campaign);
        t.add(hudLayout::restButton, h.restAvailable);
        t.add(hudLayout::logExpand);
        t.add(hudLayout::logLatest);
        if (h.targetPicker.available)
            t.add(hudLayout::enemyTargetButton(h.magic.armed));
        for (int i = 0; i < 4; ++i)
            t.add(hudLayout::detail(i));
        if (h.magic.casting()) {
            t.add(hudLayout::spellBack, h.magic.state != CastingState::Resolving);
            t.add(hudLayout::spellCancel, h.magic.state != CastingState::Resolving);
            t.add(hudLayout::spellCast,
                  (h.availability == CommandReason::Ready && (!h.magic.single || h.magic.ground)) ||
                      h.targetPicker.canConfirm);
            if (h.magic.kind == rules::SpellKind::LesserRestoration)
                t.add(hudLayout::restorationChoice);
            for (unsigned i = 0; i < h.magic.pending.targetCount && !h.magic.hostile; ++i)
                t.add(hudLayout::recipientChip(int(i)));
            for (int i = 0; i < 4; ++i)
                t.add(hudLayout::card(i), !h.magic.hostile && h.magic.allyReasons[i] == CommandReason::Ready);
        } else {
            t.add(hudLayout::endTurn, h.combat.canEndTurn && !h.suspended);
            for (int i = 0; i < 6; ++i) {
                const auto reason = h.actionAvailability[unsigned(dockActions[i])];
                const bool available = !h.suspended && (i == 3 || i == 4 || reason == CommandReason::Ready ||
                                                        reason == CommandReason::MissingTarget ||
                                                        reason == CommandReason::InvalidAlly);
                t.add(hudLayout::action(i), available);
            }
            cards();
        }
    }
    return t;
}
} // namespace blobber
