#include "ui/interaction.hpp"
#include "ui/camp_layout.hpp"
#include "ui/inventory_interaction.hpp"
#include "ui/loot_layout.hpp"
#include "ui/travel_layout.hpp"
namespace blobber {
void routeHudInput(CommandController& commands, const UiInput& input, std::optional<ActorId> viewportHit,
                   bool allowConfirm, std::optional<LootId> lootHit, std::optional<Cell> groundHit,
                   std::optional<FixtureId> fixtureHit) noexcept {
    if (!input.focused) {
        commands.hover(HoverKind::None);
        return;
    }
    const auto insideCamp = [&](UiRect r) { return input.pointer && r.contains(*input.pointer); };
    if (commands.snapshot().travel.open) {
        if (input.has(UiKey::Cancel) || (input.has(UiKey::Click) && insideCamp(travelLayout::cancel))) {
            commands.closeOverlay();
            return;
        }
        const auto& h = commands.snapshot().travel;
        if (h.hub) {
            if (input.has(UiKey::Up))
                commands.selectTravel(h.cursor ? h.cursor - 1 : h.count - 1);
            if (input.has(UiKey::Down))
                commands.selectTravel((h.cursor + 1) % h.count);
            for (unsigned i = 0; i < h.count; ++i)
                if (input.has(UiKey::Click) && insideCamp(travelLayout::row(i))) {
                    commands.selectTravel(i);
                    return;
                }
        }
        if (input.has(UiKey::Confirm) || (input.has(UiKey::Click) && insideCamp(travelLayout::confirm)))
            commands.confirmTravel();
        return;
    }
    if (!commands.snapshot().modal() &&
        (input.has(UiKey::Travel) || (input.has(UiKey::Click) && insideCamp(hudLayout::travelButton))) &&
        commands.snapshot().campaign) {
        commands.openTravel();
        return;
    }
    if (commands.snapshot().advancement.open) {
        commands.routeAdvancement(input);
        return;
    }
    if (commands.snapshot().leaveConfirmation) {
        if (input.has(UiKey::Cancel) || input.has(UiKey::Confirm) ||
            (input.has(UiKey::Click) && insideCamp(hudLayout::resume)))
            commands.confirmLeave(false);
        else if (input.has(UiKey::Click) && insideCamp(hudLayout::mainMenu))
            commands.confirmExit();
        return;
    }
    if (commands.snapshot().loot.open) {
        commands.hover(HoverKind::None);
        if (input.has(UiKey::Cancel) ||
            (input.has(UiKey::Click) && (insideCamp(lootLayout::close) || insideCamp(lootLayout::dismiss)))) {
            commands.closeOverlay();
            return;
        }
        if (input.has(UiKey::Click)) {
            for (unsigned i = 0; i < 4; ++i)
                if (insideCamp(hudLayout::card(i))) {
                    commands.lootRecipient(i);
                    return;
                }
            if (insideCamp(lootLayout::takeAll)) {
                if (commands.snapshot().loot.canTakeAny)
                    commands.lootTakeAll();
                return;
            }
            if (insideCamp(lootLayout::previous)) {
                commands.lootPage(-1);
                return;
            }
            if (insideCamp(lootLayout::next)) {
                commands.lootPage(1);
                return;
            }
        }
        if (input.scroll && insideCamp(lootLayout::panel)) {
            commands.lootPage(-input.scroll);
            return;
        }
        int hovered = -1;
        for (unsigned i = 0; i < LootHud::visibleCount; ++i)
            if (insideCamp(lootLayout::cell(i)))
                hovered = int(i);
        commands.lootHover(hovered);
        if (input.has(UiKey::Click) && hovered >= 0)
            commands.lootTake(unsigned(hovered));
        return;
    }
    if (commands.snapshot().camp.open) {
        if (commands.snapshot().camp.preparing) {
            commands.routePreparation(input);
            return;
        }
        if (!commands.snapshot().camp.playing && commands.snapshot().camp.request.kind == RestKind::Long &&
            input.has(UiKey::Click) && insideCamp(campLayout::prepare)) {
            commands.openPreparation();
            return;
        }
        if (!commands.snapshot().camp.playing && commands.snapshot().camp.request.kind == RestKind::Long &&
            input.has(UiKey::Tab)) {
            commands.openPreparation();
            return;
        }
        if (input.has(UiKey::Cancel) || (input.has(UiKey::Click) && insideCamp(campLayout::cancel))) {
            commands.closeOverlay();
            return;
        }
        if (commands.snapshot().camp.playing)
            return;
        if (input.has(UiKey::Left))
            commands.campKind(RestKind::Short);
        if (input.has(UiKey::Right))
            commands.campKind(RestKind::Long);
        if (input.has(UiKey::Click)) {
            if (insideCamp(campLayout::shortRest))
                commands.campKind(RestKind::Short);
            if (insideCamp(campLayout::longRest))
                commands.campKind(RestKind::Long);
            for (unsigned c = 0; c < 4; ++c) {
                if (insideCamp(campLayout::minus(c)))
                    commands.campDice(c, -1);
                if (insideCamp(campLayout::plus(c)))
                    commands.campDice(c, 1);
            }
        }
        if (input.has(UiKey::Confirm) || (input.has(UiKey::Click) && insideCamp(campLayout::start)))
            commands.startRest();
        return;
    }
    if (input.has(UiKey::Rest) && (!commands.snapshot().modal() || commands.snapshot().menuOpen)) {
        commands.openCamp();
        return;
    }
    commands.floorHover({});
    if (routeInventoryInput(commands, input, allowConfirm))
        return;
    const auto& state = commands.snapshot();
    const bool moved = input.pointer && commands.pointerMoved(input.pointer->x, input.pointer->y);
    const bool click = input.has(UiKey::Click);
    const auto inside = [&](UiRect r) { return input.pointer && r.contains(*input.pointer); };
    if (input.has(UiKey::Cancel) || (input.has(UiKey::RightClick) && !state.magic.casting())) {
        if (state.modal())
            commands.closeOverlay();
        else if (state.magic.armed || state.selection.action != PartyAction::Melee ||
                 state.selection.target || state.selection.ally)
            commands.cancel();
        else if (input.has(UiKey::Cancel))
            commands.toggleMenu();
        commands.hover(HoverKind::None);
        return;
    }
    const auto statusHover = [&]() {
        if (!input.pointer || state.suspended)
            return false;
        for (unsigned c = 0; c < state.cards.size(); ++c) {
            if (state.cards[c].concentration && inside(hudLayout::concentration(int(c)))) {
                commands.hover(HoverKind::Concentration, int(c), {}, input.pointer->x, input.pointer->y);
                return true;
            }
            for (unsigned n = 0;
                 n < std::min(state.cards[c].effects.count, std::size_t(hudLayout::statusSlots)); ++n)
                if (inside(hudLayout::status(int(c), int(n)))) {
                    commands.hover(HoverKind::Status, int(c * 6 + n), {}, input.pointer->x, input.pointer->y);
                    return true;
                }
        }
        return false;
    };
    // Read-only status hover does not select a target or consume a keyboard action.
    if (!click && std::none_of(input.pressed.begin(), input.pressed.end(), [](bool v) { return v; }) &&
        (!state.modal() || state.magic.open) && statusHover())
        return;
    // An overlay owns the entire input frame, including its closing click.
    if (state.modal()) {
        commands.hover(HoverKind::None);
        if (state.magic.open) {
            if (moved)
                for (int i = 0; i < hudLayout::spellRows; ++i)
                    if (inside(hudLayout::spellEntry(i)) &&
                        state.magic.offset + i < state.magic.visibleCount) {
                        std::size_t row = 0;
                        while (row < state.magic.visibleCount &&
                               state.magic.visible[row] != state.magic.cursor)
                            ++row;
                        commands.browseSpellbook(int(state.magic.offset + i) - int(row));
                    }
            if (input.scroll && inside(hudLayout::spellbook))
                commands.scrollSpellbook(-input.scroll);
            if (input.has(UiKey::Left))
                commands.cycleSpellbookTab(-1);
            if (input.has(UiKey::Right))
                commands.cycleSpellbookTab(1);
            if (input.has(UiKey::Up))
                commands.browseSpellbook(-1);
            if (input.has(UiKey::Down))
                commands.browseSpellbook(1);
            if (input.has(UiKey::Confirm))
                commands.chooseSpellbook(state.magic.cursor);
            if (click) {
                int tabX = 14;
                for (unsigned t = 0; t < state.magic.tabs.size(); ++t)
                    if (state.magic.tabs[t]) {
                        const auto r = hudLayout::spellTab(int(t), tabX);
                        if (inside(r)) {
                            commands.selectSpellbookTab(t);
                            return;
                        }
                        tabX += r.width + 2;
                    }
                if (inside(hudLayout::spellbookClose) || !inside(hudLayout::spellbook))
                    commands.closeOverlay();
                else if (inside(hudLayout::spellPrevious))
                    commands.scrollSpellbook(-7);
                else if (inside(hudLayout::spellNext))
                    commands.scrollSpellbook(7);
                else
                    for (int i = 0; i < hudLayout::spellRows; ++i)
                        if (inside(hudLayout::spellEntry(i)) &&
                            state.magic.offset + i < state.magic.visibleCount)
                            commands.chooseSpellbook(state.magic.visible[state.magic.offset + i]);
            }
        } else if (state.logExpanded) {
            if (input.scroll && (inside(hudLayout::expandedLog) || inside(hudLayout::log)))
                commands.scrollLog(input.scroll);
            if (click && (inside(hudLayout::closeOverlay) || inside(hudLayout::logExpand)))
                commands.closeOverlay();
            if (click && inside(hudLayout::logLatest))
                commands.latestLog();
        } else if (state.menuOpen) {
            if (state.development && click && inside(hudLayout::developmentXp)) {
                commands.developmentExperience();
                return;
            }
            if (state.canReturnToMenu && click && inside(hudLayout::mainMenu)) {
                if (state.leaveConfirmation)
                    commands.requestMenu();
                else
                    commands.confirmLeave(true);
                return;
            }
            if (click && (inside(hudLayout::resume) || inside(hudLayout::gear))) {
                commands.confirmLeave(false);
                commands.closeOverlay();
            } else if (click && inside(hudLayout::rest)) {
                if (state.arena)
                    commands.confirmRetry();
                else
                    commands.openCamp();
            } else if (click && inside(hudLayout::quit))
                commands.requestQuit();
        }
        return;
    }
    if (!state.modal() && !state.magic.casting() && state.combat.canEndTurn && click &&
        inside(hudLayout::endTurn)) {
        commands.endTurn();
        return;
    }
    if (state.suspended) {
        commands.hover(HoverKind::None);
        return;
    }
    if (allowConfirm && (input.has(UiKey::Interact) || (click && fixtureHit)) && !state.magic.casting() &&
        !state.magic.armed && !state.inventory.itemTargeting) {
        commands.interactFixture(input.has(UiKey::Interact) ? std::nullopt : fixtureHit);
        return;
    }
    if (lootHit && !state.magic.casting() && !state.magic.armed && !state.inventory.itemTargeting) {
        commands.floorHover(lootHit);
        if (click && allowConfirm) {
            commands.openLoot(*lootHit);
            return;
        }
    }
    if (inside(hudLayout::restButton)) {
        commands.hover(HoverKind::Rest, 0, {}, input.pointer->x, input.pointer->y);
        if (click && state.restAvailable)
            commands.openCamp();
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
    if (input.scroll && inside(state.magic.casting() ? hudLayout::castingLog : hudLayout::log))
        commands.scrollLog(input.scroll);
    if (click)
        for (int i = 0; i < 4; ++i)
            if (inside(hudLayout::detail(i))) {
                commands.inspect(i);
                return;
            }
    if (state.magic.armed && state.magic.ground && input.pointer && (moved || click))
        commands.magicGround(inside(hudLayout::viewport) ? groundHit : std::nullopt,
                             click && inside(hudLayout::viewport));
    if (state.magic.casting()) {
        if (state.magic.state == CastingState::Resolving) {
            commands.hover(HoverKind::None);
            return;
        }
        if (state.magic.kind == rules::SpellKind::LesserRestoration &&
            (input.has(UiKey::Tab) || (click && inside(hudLayout::restorationChoice)))) {
            commands.restorationChoice();
            return;
        }
        if (input.has(UiKey::Back) || (click && inside(hudLayout::spellBack))) {
            commands.backToSpellbook();
            return;
        }
        if (click && inside(hudLayout::spellCancel)) {
            commands.cancel();
            return;
        }
        if (input.has(UiKey::Left) || input.has(UiKey::Right)) {
            commands.magicKeyboardTarget(input.has(UiKey::Left) ? -1 : 1);
            const auto& c = state.magic.pending;
            if (state.magic.hostile && c.target) {
                const auto anchor = state.targets.anchors[c.target->slot];
                commands.hover(HoverKind::Enemy, 0, c.target, float(viewX + anchor.x),
                               float(viewY + anchor.y));
            } else if (c.ally) {
                for (int i = 0; i < 4; ++i)
                    if (state.cards[i].id.index == c.ally->index)
                        commands.hover(HoverKind::Portrait, i, {}, 508, float(hudLayout::card(i).y));
            }
            return;
        }
        if (input.has(UiKey::Up) || input.has(UiKey::Down)) {
            const auto c = state.magic.pending;
            if (state.magic.hostile && c.target)
                commands.magicEnemy(*c.target, input.has(UiKey::Down));
            else if (c.ally) {
                if (input.has(UiKey::Down)) {
                    for (std::size_t j = 0; j < c.targetCount; ++j)
                        if (c.allies[j].index == c.ally->index) {
                            commands.removeMagicRecipient(j);
                            break;
                        }
                } else
                    for (std::size_t i = 0; i < state.cards.size(); ++i)
                        if (state.cards[i].id.index == c.ally->index)
                            commands.magicAlly(i);
            }
            return;
        }
        if (allowConfirm &&
            (input.has(UiKey::Confirm) || (!state.magic.single && click && inside(hudLayout::spellCast)))) {
            if (state.availability == CommandReason::Ready || state.magic.single)
                commands.confirm();
            return;
        }
        if (click && !state.magic.hostile && state.magic.pending.targetCount > 0)
            for (std::size_t i = 0; i < state.magic.pending.targetCount; ++i)
                if (inside(hudLayout::recipientChip(int(i)))) {
                    commands.removeMagicRecipient(i);
                    return;
                }
        if ((click || input.has(UiKey::RightClick)) && allowConfirm) {
            if (viewportHit && inside(hudLayout::viewport)) {
                const bool commit = click && !input.shift && commands.magicSingleTarget() &&
                                    commands.magicEligibility(*viewportHit) == CommandReason::Ready;
                commands.magicEnemy(*viewportHit, input.has(UiKey::RightClick) || input.shift);
                if (commit)
                    commands.confirm();
            } else if (click)
                for (std::size_t i = 0; i < state.cards.size(); ++i)
                    if (inside(hudLayout::card(int(i)))) {
                        const bool commit = commands.magicSingleTarget() && !state.magic.hostile &&
                                            state.magic.allyReasons[i] == CommandReason::Ready;
                        commands.magicAlly(i);
                        if (commit)
                            commands.confirm();
                        break;
                    }
        }
        // Casting owns the whole frame. Hover is read-only, including after mutations.
        if (input.pointer && (moved || click || input.has(UiKey::RightClick))) {
            const auto p = *input.pointer;
            commands.hover(HoverKind::None);
            if (inside(hudLayout::viewport) && viewportHit)
                commands.hover(HoverKind::Enemy, 0, viewportHit, p.x, p.y);
            for (int i = 0; i < 4; ++i)
                if (inside(hudLayout::card(i)))
                    commands.hover(HoverKind::Portrait, i, {}, p.x, p.y);
        }
        return;
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
