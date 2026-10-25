#include "game/commands.hpp"
#include <algorithm>
#include <cstdio>

namespace blobber {
const char* actionName(PartyAction action) noexcept {
    constexpr const char* names[]{"Melee", "Back-row melee", "Defend", "Ranged", "Spell", "Item"};
    const auto i = static_cast<unsigned>(action);
    return i < static_cast<unsigned>(PartyAction::Count) ? names[i] : "Invalid action";
}
const char* commandMessage(CommandReason reason) noexcept {
    switch (reason) {
    case CommandReason::PreviewAccepted:
        return "Preview only: no costs or combat.";
    case CommandReason::InvalidCharacter:
        return "Invalid character.";
    case CommandReason::InvalidAction:
        return "Invalid action.";
    case CommandReason::Unavailable:
        return "Unavailable: arrives in a later slice.";
    case CommandReason::BackRowOnly:
        return "Select a back-row character.";
    case CommandReason::MissingTarget:
        return "Select a visible target.";
    case CommandReason::InvalidTarget:
        return "Target is gone or cannot act.";
    case CommandReason::NotVisible:
        return "Target is no longer visible.";
    case CommandReason::Moving:
        return "Wait for movement to finish.";
    case CommandReason::MapOpen:
        return "Close the map to target or confirm.";
    case CommandReason::Unfocused:
        return "Focus lost: selection cancelled.";
    case CommandReason::CannotAct:
        return "Character cannot act (demo status).";
    }
    return "Invalid request.";
}
void CombatLog::append(const char* message) noexcept {
    std::snprintf(entries_[next_].data(), messageBytes, "%s", message ? message : "");
    next_ = (next_ + 1) % capacity;
    count_ = std::min(count_ + 1, capacity);
}
const char* CombatLog::newest(std::size_t offset) const noexcept {
    return offset < count_ ? entries_[(next_ + capacity - 1 - offset) % capacity].data() : "";
}
CommandController::CommandController(const Party& party, const ActorWorld& actors)
    : party_(party), actors_(actors) {
    state_.selection.character = party.snapshot().formation[0];
    refresh({}, false, false, true);
    state_.log.append("Choose a character, action and target. Mana is unavailable until the spell slice.");
}
bool CommandController::visible(ActorId id) const noexcept {
    if (state_.targets.world != actors_.snapshot().world ||
        state_.targets.revision != actors_.snapshot().revision)
        return false;
    for (std::size_t i = 0; i < state_.targets.count; ++i)
        if (sameActor(id, state_.targets.ids[i]))
            return true;
    return false;
}
void CommandController::refresh(const TargetSnapshot& targets, bool moving, bool mapOpen, bool focused,
                                const HudDemo* demo) noexcept {
    suspension_ = !focused  ? CommandReason::Unfocused
                  : moving  ? CommandReason::Moving
                  : mapOpen ? CommandReason::MapOpen
                            : CommandReason::PreviewAccepted;
    state_.suspended = suspension_ != CommandReason::PreviewAccepted;
    TargetSnapshot normalized;
    if (targets.count <= targets.ids.size() && targets.world == actors_.snapshot().world &&
        targets.revision == actors_.snapshot().revision) {
        normalized.world = targets.world;
        normalized.revision = targets.revision;
        // Normalize membership in stable slot order, ignoring duplicates and invalid IDs.
        std::array<bool, NpcWorld::maxInstances> included{};
        for (std::size_t i = 0; i < targets.count; ++i) {
            const auto* a = actors_.actor(targets.ids[i]);
            if (a && a->life == ActorLife::Alive)
                included[targets.ids[i].slot] = true;
        }
        for (std::size_t i = 0; i < included.size(); ++i)
            if (included[i])
                normalized.ids[normalized.count++] = actors_.id(i);
    }
    state_.targets = normalized;
    state_.demo = demo && demo->enabled;
    state_.demoScenario = state_.demo ? demo->scenario : 0;
    for (std::size_t i = 0; i < rules::partySize; ++i) {
        const auto id = party_.snapshot().formation[i];
        const auto& member = *party_.character(id);
        auto& card = state_.cards[i];
        card = {};
        card.id = id;
        card.name = member.build.name.c_str();
        card.className = party_.catalog().characterClass(member.build.characterClass)->name.c_str();
        card.hp = member.hitPoints;
        card.maxHp = member.stats.maxHitPoints;
        if (state_.demo) {
            const auto& r = demo->resources[id.index];
            card.hp = std::clamp(r.hp, 0, card.maxHp);
            card.mana = std::max(0, r.mana);
            card.maxMana = std::max(card.mana, r.maxMana);
            card.hasMana = true;
            card.status = r.status;
        }
    }
    if (state_.selection.target && (moving || !focused || !visible(*state_.selection.target))) {
        state_.log.append(commandMessage(moving     ? CommandReason::Moving
                                         : !focused ? CommandReason::Unfocused
                                                    : CommandReason::NotVisible));
        state_.logOffset = 0;
        state_.selection.target.reset();
    }
    const auto pages = (state_.targets.count + 3) / 4;
    state_.targetPage = pages ? std::min(state_.targetPage, pages - 1) : 0;
    updateAvailability();
}
void CommandController::updateAvailability() noexcept {
    state_.availability = validate(state_.selection);
    auto preview = state_.selection;
    for (std::size_t i = 0; i < state_.actionAvailability.size(); ++i) {
        preview.action = static_cast<PartyAction>(i);
        state_.actionAvailability[i] = validate(preview);
    }
}
void CommandController::selectCharacter(std::size_t index) noexcept {
    if (index < rules::partySize)
        state_.selection.character = party_.snapshot().formation[index];
    updateAvailability();
}
void CommandController::selectAction(PartyAction action) noexcept {
    if (static_cast<unsigned>(action) >= static_cast<unsigned>(PartyAction::Count))
        return;
    state_.selection.action = action;
    if (action == PartyAction::Defend)
        state_.selection.target.reset();
    updateAvailability();
}
void CommandController::cycleAction(int delta) noexcept {
    constexpr int count = static_cast<int>(PartyAction::Count);
    selectAction(static_cast<PartyAction>(
        (static_cast<int>(state_.selection.action) + (delta < 0 ? count - 1 : 1)) % count));
}
void CommandController::selectTarget(ActorId target) noexcept {
    if (state_.suspended || !visible(target))
        return;
    state_.selection.target = target;
    for (std::size_t i = 0; i < state_.targets.count; ++i)
        if (sameActor(target, state_.targets.ids[i]))
            state_.targetPage = i / 4;
    updateAvailability();
}
void CommandController::cycleTarget(int delta) noexcept {
    if (state_.suspended || !state_.targets.count)
        return;
    const auto count = state_.targets.count;
    std::size_t next = delta < 0 ? count - 1 : 0;
    if (state_.selection.target)
        for (std::size_t i = 0; i < count; ++i)
            if (sameActor(*state_.selection.target, state_.targets.ids[i]))
                next = (i + (delta < 0 ? count - 1 : 1)) % count;
    selectTarget(state_.targets.ids[next]);
}
void CommandController::pageTargets(int delta) noexcept {
    if (state_.suspended)
        return;
    const auto pages = (state_.targets.count + 3) / 4;
    if (pages)
        state_.targetPage = (state_.targetPage + (delta < 0 ? pages - 1 : 1)) % pages;
}
void CommandController::scrollLog(int delta) noexcept {
    const int maximum = state_.log.size() ? static_cast<int>(state_.log.size()) - 1 : 0;
    state_.logOffset = static_cast<std::size_t>(
        std::clamp<std::int64_t>(static_cast<std::int64_t>(state_.logOffset) + delta, 0, maximum));
}
void CommandController::cancel() noexcept {
    state_.selection.target.reset();
    updateAvailability();
}
void CommandController::movementStarted() noexcept {
    if (state_.selection.target) {
        state_.log.append("Movement cleared the selected target.");
        state_.logOffset = 0;
    }
    cancel();
}
CommandReason CommandController::validate(const PartyCommand& command) const noexcept {
    if (state_.suspended)
        return suspension_;
    if (!party_.character(command.character))
        return CommandReason::InvalidCharacter;
    if (static_cast<unsigned>(command.action) >= static_cast<unsigned>(PartyAction::Count))
        return CommandReason::InvalidAction;
    bool back = false;
    for (std::size_t i = 0; i < rules::partySize; ++i)
        if (state_.cards[i].id.index == command.character.index) {
            if (state_.cards[i].status != DisplayStatus::Ready)
                return CommandReason::CannotAct;
        }
    for (std::size_t i = 2; i < rules::partySize; ++i)
        if (party_.snapshot().formation[i].index == command.character.index)
            back = true;
    if (command.action >= PartyAction::Ranged)
        return CommandReason::Unavailable;
    if (command.action == PartyAction::BackMelee && !back)
        return CommandReason::BackRowOnly;
    if (command.action == PartyAction::Defend)
        return CommandReason::PreviewAccepted;
    if (!command.target)
        return CommandReason::MissingTarget;
    const auto* actor = actors_.actor(*command.target);
    if (!actor || actor->life != ActorLife::Alive)
        return CommandReason::InvalidTarget;
    return visible(*command.target) ? CommandReason::PreviewAccepted : CommandReason::NotVisible;
}
CommandReason CommandController::submit(const PartyCommand& command) noexcept {
    const auto reason = validate(command);
    char line[CombatLog::messageBytes];
    const auto* member = party_.character(command.character);
    if (command.target && command.action != PartyAction::Defend)
        std::snprintf(line, sizeof(line), "%s: %s -> monster #%u - %s",
                      member ? member->build.name.c_str() : "?", actionName(command.action),
                      unsigned(command.target->slot) + 1, commandMessage(reason));
    else
        std::snprintf(line, sizeof(line), "%s: %s - %s", member ? member->build.name.c_str() : "?",
                      actionName(command.action), commandMessage(reason));
    state_.log.append(line);
    state_.logOffset = 0;
    updateAvailability();
    return reason;
}
} // namespace blobber
