#include "game/commands.hpp"
#include "ui/layout.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace blobber {
namespace {
// ASCII is the catalog/log contract. Word wrapping is bounded by messageBytes.
std::size_t nextLine(const char*& cursor, int columns, const char*& start) noexcept {
    while (*cursor == ' ')
        ++cursor;
    start = cursor;
    std::size_t n = 0, space = 0;
    while (cursor[n] && cursor[n] != '\n' && n < static_cast<std::size_t>(columns)) {
        if (cursor[n] == ' ')
            space = n;
        ++n;
    }
    if (cursor[n] && cursor[n] != '\n' && cursor[n] != ' ' && space)
        n = space;
    cursor += n;
    while (*cursor == ' ')
        ++cursor;
    if (*cursor == '\n')
        ++cursor;
    return n;
}
std::size_t lines(const char* cursor, int columns) noexcept {
    std::size_t count = 0;
    while (*cursor) {
        const char* start;
        nextLine(cursor, columns, start);
        ++count;
    }
    return count;
}
} // namespace
std::size_t CombatLog::lineCount(int columns) const noexcept {
    if (columns <= 0)
        return 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < size(); ++i)
        count += lines(newest(i), columns);
    return count;
}
bool CombatLog::line(std::size_t index, int columns, char* output, std::size_t bytes) const noexcept {
    if (!output || !bytes || columns <= 0)
        return false;
    output[0] = '\0';
    for (std::size_t i = size(); i > 0; --i) {
        const char* cursor = newest(i - 1);
        while (*cursor) {
            const char* start;
            const auto length = nextLine(cursor, columns, start);
            if (!index) {
                const auto n = std::min(length, bytes - 1);
                std::memcpy(output, start, n);
                output[n] = '\0';
                return true;
            }
            --index;
        }
    }
    return false;
}
void CommandController::appendLog(const char* message) noexcept {
    const int columns = state_.logExpanded ? hudLayout::expandedLogColumns : hudLayout::logColumns;
    const auto added = message ? lines(message, columns) : 0;
    state_.log.append(message);
    if (state_.logOffset) {
        const auto count = state_.log.lineCount(columns);
        state_.logOffset = std::min(state_.logOffset + added, count ? count - 1 : 0);
    }
}
void CommandController::toggleLog() noexcept {
    const bool open = !state_.logExpanded;
    closeOverlay();
    cancel();
    state_.logExpanded = open;
    state_.hover = {};
}
void CommandController::inspect(std::size_t index) noexcept {
    if (index >= rules::partySize || state_.suspended)
        return;
    closeOverlay();
    cancel();
    state_.inspected = state_.cards[index].id;
    state_.hover = {};
}
void CommandController::toggleMenu() noexcept {
    const bool open = !state_.menuOpen;
    closeOverlay();
    cancel();
    state_.menuOpen = open;
    state_.hover = {};
}
void CommandController::closeOverlay() noexcept {
    state_.inspected.reset();
    state_.logExpanded = false;
    state_.menuOpen = false;
    updateAvailability();
}
void CommandController::hover(HoverKind kind, int index, std::optional<ActorId> enemy, float x,
                              float y) noexcept {
    state_.hover = {};
    state_.hover.kind = kind;
    state_.hover.index = index;
    state_.hover.enemy = enemy;
    state_.hover.x = x;
    state_.hover.y = y;
    updateHover();
}
void CommandController::updateHover() noexcept {
    auto& h = state_.hover;
    h.lines = {};
    if (state_.modal() || state_.suspended) {
        h.kind = HoverKind::None;
        return;
    }
    auto command = state_.selection;
    auto put = [&](int line, const char* text) {
        std::snprintf(h.lines[line].data(), h.lines[line].size(), "%s", text);
    };
    if (h.kind == HoverKind::Enemy) {
        if (!h.enemy || !visible(*h.enemy)) {
            h.kind = HoverKind::None;
            return;
        }
        const auto& a = *actors_.actor(*h.enemy);
        std::snprintf(h.lines[0].data(), h.lines[0].size(), "%s #%u",
                      actors_.catalog().monster(a.monster)->id.c_str(), unsigned(h.enemy->slot) + 1);
        h.lines[0][0] = static_cast<char>(std::toupper(static_cast<unsigned char>(h.lines[0][0])));
        h.health = a.maxHitPoints ? float(a.hitPoints) / a.maxHitPoints : 0;
        command.action = PartyAction::Melee;
        command.target = h.enemy;
        h.reason = validate(command);
        const auto* c = party_.character(command.character);
        if (c && c->build.equipment.mainHand.index < c->build.itemCount) {
            const auto i = c->build.equipment.mainHand.index;
            const auto& weapon = c->stats.weapons[i];
            const auto& dice = weapon.damage;
            const auto* item = party_.catalog().item(c->build.items[i].item);
            std::snprintf(h.lines[1].data(), h.lines[1].size(), "%s: %lld-%lld damage", item->name.c_str(),
                          static_cast<long long>(std::max<std::int64_t>(0, dice.count + dice.modifier)),
                          static_cast<long long>(std::max<std::int64_t>(
                              0, std::int64_t(dice.count) * dice.sides + dice.modifier)));
        }
        put(2, "Cost: 1 action");
        put(3, h.reason == CommandReason::Ready ? "Click to attack." : commandMessage(h.reason));
    } else if (h.kind == HoverKind::Action && h.index >= 0 && h.index < 6) {
        const auto action = dockActions[h.index];
        command.action = action;
        h.reason = validate(command);
        std::snprintf(h.lines[0].data(), h.lines[0].size(), "%s [%d]", actionName(action), h.index + 1);
        constexpr const char* effects[]{
            "Attack with your equipped weapon.",   "Attacks against you: disadvantage until next turn.",
            "Ranged combat is not available yet.", "Spells are not available yet.",
            "Usable items are not available yet.", "DC 10 Wisdom: stabilize a dying ally."};
        put(1, effects[h.index]);
        put(2, (h.index >= 2 && h.index <= 4) ? "Unavailable" : "Cost: 1 action");
        if (h.reason != CommandReason::Ready && h.reason != CommandReason::MissingTarget &&
            h.reason != CommandReason::InvalidAlly && h.reason != CommandReason::PreviewAccepted)
            put(3, commandMessage(h.reason));
        else if (action == PartyAction::Aid) {
            put(2, "Cost: 1 action / no healing");
            put(3, state_.allyCount ? "Then click a dying ally's portrait." : "No dying allies to aid.");
        } else if (action == PartyAction::Defend)
            put(3, "Click or press 2 to defend.");
        else
            put(3, "Click a reachable enemy.");

    } else if (h.kind == HoverKind::Portrait && h.index >= 0 && h.index < 4) {
        const auto& card = state_.cards[h.index];
        put(0, card.name);
        if (state_.selection.action == PartyAction::Aid) {
            command.ally = card.id;
            h.reason = validate(command);
            put(1, h.reason == CommandReason::Ready ? "Click to aid. Cost: 1 action."
                                                    : commandMessage(h.reason));
        } else {
            put(1, "Click to inspect character.");
            put(2, "Read-only details; pauses combat.");
        }
    }
}
} // namespace blobber
