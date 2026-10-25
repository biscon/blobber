#include "ui/interaction.hpp"
namespace blobber {
void routeHudInput(CommandController& commands, const UiInput& input, std::optional<ActorId> viewportHit,
                   bool allowConfirm) noexcept {
    if (!input.focused) {
        commands.cancel();
        return;
    }
    if (input.has(UiKey::Cancel) || input.has(UiKey::RightClick)) {
        commands.cancel();
        return;
    }
    for (int i = 0; i < 4; ++i)
        if (input.has(static_cast<UiKey>(i)))
            commands.selectCharacter(i);
    if (input.has(UiKey::Up))
        commands.cycleAction(-1);
    if (input.has(UiKey::Down))
        commands.cycleAction(1);
    if (input.has(UiKey::Left))
        commands.cycleTarget(-1);
    if (input.has(UiKey::Right))
        commands.cycleTarget(1);
    bool confirm = input.has(UiKey::Confirm);
    if (input.pointer) {
        const auto p = *input.pointer;
        if ((hudLayout::log.contains(p) ||
             (commands.snapshot().logExpanded && hudLayout::expandedLog.contains(p))) &&
            input.scroll)
            commands.scrollLog(input.scroll);
        if (input.has(UiKey::Click)) {
            for (int i = 0; i < 4; ++i)
                if (hudLayout::card(i).contains(p))
                    commands.selectCharacter(i);
            for (int i = 0; i < (commands.snapshot().production ? static_cast<int>(PartyAction::Count) : 6);
                 ++i)
                if (hudLayout::action(i, commands.snapshot().production).contains(p))
                    commands.selectAction(static_cast<PartyAction>(i));
            if (hudLayout::previousPage.contains(p))
                commands.pageTargets(-1);
            if (hudLayout::nextPage.contains(p))
                commands.pageTargets(1);
            const auto& state = commands.snapshot();
            for (int i = 0; i < 4; ++i) {
                const auto index = state.targetPage * 4 + i;
                if (hudLayout::target(i).contains(p)) {
                    if (state.production && state.selection.action == PartyAction::Aid)
                        commands.selectAlly(i);
                    else if (index < state.targets.count)
                        commands.selectTarget(state.targets.ids[index]);
                }
            }
            if (hudLayout::log.contains(p))
                commands.toggleLog();
            if (commands.snapshot().logExpanded && hudLayout::expandedLog.contains(p))
                return;
            if (hudLayout::viewport.contains(p) && viewportHit)
                commands.selectTarget(*viewportHit);
            confirm |= hudLayout::confirm.contains(p);
        }
    }
    if (confirm && allowConfirm)
        commands.confirm();
}
} // namespace blobber
