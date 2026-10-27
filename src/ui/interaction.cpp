#include "ui/interaction.hpp"
namespace blobber {
void routeHudInput(CommandController& commands, const UiInput& input, std::optional<ActorId> viewportHit,
                   bool allowConfirm) noexcept {
    if (!input.focused) {
        commands.cancel();
        commands.hover(HoverKind::None);
        return;
    }
    const auto& state = commands.snapshot();
    const bool moved = input.pointer && commands.pointerMoved(input.pointer->x, input.pointer->y);
    const bool click = input.has(UiKey::Click);
    const auto inside = [&](UiRect r) { return input.pointer && r.contains(*input.pointer); };
    if (input.has(UiKey::Cancel) || input.has(UiKey::RightClick)) {
        if (state.modal())
            commands.closeOverlay();
        else if (state.selection.action != PartyAction::Melee || state.selection.target ||
                 state.selection.ally)
            commands.cancel();
        else if (input.has(UiKey::Cancel))
            commands.toggleMenu();
        commands.hover(HoverKind::None);
        return;
    }
    // An overlay owns the entire input frame, including its closing click.
    if (state.modal()) {
        commands.hover(HoverKind::None);
        if (state.magic.open) {
            if (moved)
                for (int i = 0; i < 10; ++i)
                    if (inside(hudLayout::spellEntry(i)) && state.magic.offset + i < state.magic.count)
                        commands.browseSpellbook(int(state.magic.offset + i) - int(state.magic.cursor));
            if (input.scroll)
                commands.browseSpellbook(-input.scroll);
            if (input.has(UiKey::Up))
                commands.browseSpellbook(-1);
            if (input.has(UiKey::Down))
                commands.browseSpellbook(1);
            if (input.has(UiKey::Confirm))
                commands.chooseSpellbook(state.magic.cursor);
            if (click) {
                if (inside(hudLayout::spellbookClose))
                    commands.closeOverlay();
                else if (inside(hudLayout::spellPrevious))
                    commands.browseSpellbook(-10);
                else if (inside(hudLayout::spellNext))
                    commands.browseSpellbook(10);
                else
                    for (int i = 0; i < 10; ++i)
                        if (inside(hudLayout::spellEntry(i)))
                            commands.chooseSpellbook(state.magic.offset + i);
            }
        } else if (state.logExpanded) {
            if (input.scroll && (inside(hudLayout::expandedLog) || inside(hudLayout::log)))
                commands.scrollLog(input.scroll);
            if (click && (inside(hudLayout::closeOverlay) || inside(hudLayout::logExpand)))
                commands.closeOverlay();
            if (click && inside(hudLayout::logLatest))
                commands.latestLog();
        } else if (state.inspected) {
            if (click && inside(hudLayout::detailsClose))
                commands.closeOverlay();
        } else if (state.menuOpen) {
            if (click && (inside(hudLayout::resume) || inside(hudLayout::gear)))
                commands.closeOverlay();
            else if (click && inside(hudLayout::quit))
                commands.requestQuit();
        }
        return;
    }
    if (click && inside(hudLayout::gear)) {
        commands.toggleMenu();
        return;
    }
    if (click && inside(hudLayout::logExpand)) {
        commands.toggleLog();
        return;
    }
    if (click && inside(hudLayout::logLatest)) {
        commands.latestLog();
        return;
    }
    if (input.scroll && inside(hudLayout::log))
        commands.scrollLog(input.scroll);
    if (click)
        for (int i = 0; i < 4; ++i)
            if (inside(hudLayout::detail(i))) {
                commands.inspect(i);
                return;
            }
    if (state.magic.armed && !input.has(UiKey::Four)) {
        if (input.has(UiKey::Left) || input.has(UiKey::Right)) {
            commands.magicKeyboardTarget(input.has(UiKey::Left) ? -1 : 1);
            return;
        }
        // Up adds/toggles the keyboard cursor, Enter commits the complete selection.
        if (input.has(UiKey::Up)) {
            if (state.magic.hostile && state.selection.target)
                commands.magicEnemy(*state.selection.target);
            else if (state.selection.ally)
                for (std::size_t i = 0; i < state.cards.size(); ++i)
                    if (state.cards[i].id.index == state.selection.ally->index)
                        commands.magicAlly(i);
            return;
        }
        if (allowConfirm && (input.has(UiKey::Confirm) || (click && inside(hudLayout::spellCast)))) {
            commands.confirm();
            return;
        }
        if (click && allowConfirm) {
            if (viewportHit && inside(hudLayout::viewport)) {
                commands.magicEnemy(*viewportHit);
                if (commands.magicSingleTarget())
                    commands.confirm();
                return;
            }
            for (std::size_t i = 0; i < state.cards.size(); ++i)
                if (inside(hudLayout::card(int(i)))) {
                    commands.magicAlly(i);
                    if (commands.magicSingleTarget())
                        commands.confirm();
                    return;
                }
        }
    }
    bool actionInput = false;
    for (int i = 0; i < 6; ++i) {
        const bool activated = input.has(static_cast<UiKey>(i)) || (click && inside(hudLayout::action(i)));
        if (activated) {
            actionInput = true;
            if (i == 3) {
                commands.openSpellbook();
                return;
            }
            if (allowConfirm && !state.suspended && (i == 0 || i == 1 || i == 2 || i == 5)) {
                commands.selectAction(dockActions[i]);
                if (i == 1)
                    commands.confirm();
            }
        }
    }
    if (!actionInput && click) {
        for (int i = 0; i < 4; ++i)
            if (inside(hudLayout::detail(i))) {
                commands.inspect(i);
                return;
            } else if (inside(hudLayout::card(i))) {
                if (state.selection.action == PartyAction::Aid) {
                    if (allowConfirm) {
                        commands.selectCharacter(i);
                        commands.confirm();
                    }
                } else if (state.production && realTime(state.combat.phase))
                    commands.selectCharacter(i);
                else
                    commands.inspect(i);
                return;
            }
        if (allowConfirm && inside(hudLayout::viewport) && viewportHit &&
            (state.selection.action == PartyAction::Melee || state.selection.action == PartyAction::Ranged)) {
            commands.selectTarget(*viewportHit);
            commands.confirm();
        }
    }
    if (!actionInput && (input.has(UiKey::Left) || input.has(UiKey::Right))) {
        commands.cycleTarget(input.has(UiKey::Left) ? -1 : 1);
        if (state.selection.target) {
            const auto id = *state.selection.target;
            const auto anchor = state.targets.anchors[id.slot];
            commands.hover(HoverKind::Enemy, 0, id, float(viewX + anchor.x), float(viewY + anchor.y));
        } else if (state.selection.ally) {
            for (int i = 0; i < 4; ++i)
                if (state.cards[i].id.index == state.selection.ally->index)
                    commands.hover(HoverKind::Portrait, i, {}, 508, float(hudLayout::card(i).y));
        }
        return;
    }
    if (!actionInput && input.has(UiKey::Confirm) && allowConfirm)
        commands.confirm();
    if (!moved && !click && !actionInput && state.hover.kind != HoverKind::None &&
        (state.selection.target || state.selection.ally))
        return;
    // Pointer hover is recomputed after mutation; it never selects a command target.
    if (input.pointer) {
        const auto p = *input.pointer;
        for (int i = 0; i < 6; ++i)
            if (inside(hudLayout::action(i))) {
                commands.hover(HoverKind::Action, i, {}, p.x, p.y);
                return;
            }
        for (int i = 0; i < 4; ++i)
            if (inside(hudLayout::detail(i))) {
                commands.hover(HoverKind::Portrait, i, {}, p.x, p.y);
                return;
            } else if (inside(hudLayout::card(i))) {
                commands.hover(HoverKind::Portrait, i, {}, p.x, p.y);
                return;
            }
        if (inside(hudLayout::viewport) && viewportHit) {
            commands.hover(HoverKind::Enemy, 0, viewportHit, p.x, p.y);
            return;
        }
    }
    commands.hover(HoverKind::None);
}
} // namespace blobber
