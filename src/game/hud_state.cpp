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
std::size_t CombatLog::rewrapOffset(std::size_t offset, int from, int to) const noexcept {
    if (!offset || from == to || from <= 0 || to <= 0)
        return offset;
    const auto count = lineCount(from);
    const auto end = count - std::min(offset, count);
    const auto anchorLine = end ? end - 1 : 0;
    std::size_t ordinal = 0, newOrdinal = 0;
    for (std::size_t i = size(); i > 0; --i) {
        const auto* message = newest(i - 1);
        const auto* cursor = message;
        while (*cursor) {
            const char* start;
            nextLine(cursor, from, start);
            if (ordinal++ == anchorLine) {
                const auto* anchor = cursor > message ? cursor - 1 : message;
                const auto* wrapped = message;
                while (*wrapped) {
                    const char* lineStart;
                    nextLine(wrapped, to, lineStart);
                    if (wrapped > anchor)
                        return lineCount(to) - newOrdinal - 1;
                    ++newOrdinal;
                }
            }
        }
        newOrdinal += lines(message, to);
    }
    return 0;
}
void CommandController::appendLog(const char* message) noexcept {
    const int columns = state_.logExpanded       ? hudLayout::expandedLogColumns
                        : state_.magic.casting() ? hudLayout::castingLogColumns
                                                 : hudLayout::logColumns;
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
    if (!state_.magic.casting())
        cancel();
    if (open)
        state_.logOffset = state_.log.rewrapOffset(
            state_.logOffset, state_.magic.casting() ? hudLayout::castingLogColumns : hudLayout::logColumns,
            hudLayout::expandedLogColumns);
    state_.logExpanded = open;
    state_.hover = {};
}
void CommandController::inspect(std::size_t index) noexcept {
    if (index >= rules::partySize || state_.suspended)
        return;
    openInventory(state_.cards[index].id, CharacterPage::Character);
    state_.hover = {};
}
void CommandController::toggleMenu() noexcept {
    const bool open = !state_.menuOpen;
    closeOverlay();
    if (!state_.magic.casting())
        cancel();
    state_.menuOpen = open;
    state_.hover = {};
}
void CommandController::closeOverlay() noexcept {
    state_.advancement.open = false;
    state_.loot.open = false;
    state_.camp.open = false;
    state_.inventory.open = false;
    state_.inventory.pointerDown = state_.inventory.dragging = false;
    state_.inventory.mode = InventoryMode::Browse;
    if (state_.magic.open)
        setCastingState(CastingState::Normal);
    if (state_.logExpanded)
        state_.logOffset = state_.log.rewrapOffset(state_.logOffset, hudLayout::expandedLogColumns,
                                                   state_.magic.casting() ? hudLayout::castingLogColumns
                                                                          : hudLayout::logColumns);
    state_.logExpanded = false;
    state_.menuOpen = false;
    state_.leaveConfirmation = state_.retryConfirmation = false;
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
    if ((state_.modal() &&
         !(state_.magic.open && (h.kind == HoverKind::Status || h.kind == HoverKind::Concentration))) ||
        state_.suspended) {
        h.kind = HoverKind::None;
        return;
    }
    auto command = state_.magic.armed ? state_.magic.pending : state_.selection;
    auto put = [&](int line, const char* text) {
        std::snprintf(h.lines[line].data(), h.lines[line].size(), "%s", text);
    };
    if (h.kind == HoverKind::Concentration) {
        if (unsigned(h.index) >= state_.cards.size() || !state_.cards[h.index].concentration) {
            h.kind = HoverKind::None;
            return;
        }
        const auto& e = *state_.cards[h.index].concentration;
        put(0, "Concentration");
        put(1, e.name);
        statusDuration(e, h.lines[2].data(), h.lines[2].size());
        put(3, e.ending);
        return;
    }
    if (h.kind == HoverKind::Status) {
        const unsigned card = unsigned(h.index) / 6, slot = unsigned(h.index) % 6;
        if (card >= state_.cards.size() || slot >= state_.cards[card].effects.count) {
            h.kind = HoverKind::None;
            return;
        }
        const auto& effects = state_.cards[card].effects;
        if (slot == 5 && effects.count > 6) {
            put(0, "More active effects");
            for (std::size_t i = 5; i < effects.count; ++i) {
                put(int(1 + (i - 5) * 2), effects.entries[i].name);
                statusDuration(effects.entries[i], h.lines[2 + (i - 5) * 2].data(), h.lines[0].size());
            }
        } else {
            const auto& e = effects.entries[slot];
            put(0, e.name);
            statusDuration(e, h.lines[1].data(), h.lines[1].size());
            put(2, e.ending);
        }
        return;
    }
    if (h.kind == HoverKind::Rest) {
        std::snprintf(h.lines[0].data(), h.lines[0].size(), "Rest [R]");
        std::snprintf(h.lines[1].data(), h.lines[1].size(), "%s", state_.restReason);
    } else if (h.kind == HoverKind::Enemy) {
        if (!h.enemy || !visible(*h.enemy)) {
            h.kind = HoverKind::None;
            return;
        }
        const auto& a = *actors_.actor(*h.enemy);
        std::snprintf(h.lines[0].data(), h.lines[0].size(), "%s #%u",
                      actors_.catalog().monster(a.monster)->id.c_str(), unsigned(h.enemy->slot) + 1);
        h.lines[0][0] = static_cast<char>(std::toupper(static_cast<unsigned char>(h.lines[0][0])));
        h.health = a.maxHitPoints ? float(a.hitPoints) / a.maxHitPoints : 0;
        if (state_.magic.armed) {
            const auto& m = state_.magic;
            std::snprintf(h.lines[0].data(), h.lines[0].size(), "%s",
                          actors_.catalog().monster(a.monster)->id.c_str());
            h.lines[0][0] = static_cast<char>(std::toupper(static_cast<unsigned char>(h.lines[0][0])));
            h.reason = magicEligibility(*h.enemy);
            unsigned darts = 0;
            bool selected = false;
            for (std::size_t i = 0; i < m.pending.targetCount; ++i)
                if (sameActor(m.pending.enemies[i], *h.enemy)) {
                    darts = m.pending.darts[i];
                    selected = true;
                }
            put(1, m.name);
            if (m.area) {
                bool affected = false;
                for (std::size_t i = 0; i < m.affectedCount; ++i)
                    affected |= sameActor(m.affected[i], *h.enemy);
                put(2, affected ? "In the affected area." : "Outside the affected area.");
                put(3, "Inspect the preview, then Cast.");
            } else if (h.reason != CommandReason::Ready)
                put(2, commandMessage(h.reason));
            else if (m.single)
                put(2, "Click to cast.");
            else if (m.missiles) {
                std::snprintf(h.lines[2].data(), h.lines[2].size(), "%u %s assigned", darts,
                              darts == 1 ? "dart" : "darts");
                put(3, m.assigned == 3
                           ? (darts ? "Right/Shift-click removes a dart." : "Remove a dart to reassign it.")
                           : "Left: add / Right or Shift-click: remove");
            } else
                put(2, selected ? "Click to remove." : "Click to select.");
            return;
        }
        command.action =
            state_.selection.action == PartyAction::Ranged ? PartyAction::Ranged : PartyAction::Melee;
        command.target = h.enemy;
        h.reason = validate(command);
        const auto* c = party_.character(command.character);
        const auto slot =
            command.action == PartyAction::Ranged ? EquipmentSlot::Ranged : EquipmentSlot::MainHand;
        const auto* item = party_.equipped(command.character, slot);
        if (c && item) {
            const auto i = static_cast<unsigned>(slot);
            const auto& weapon = c->stats.weapons[i];
            const auto& dice = weapon.damage;
            std::snprintf(h.lines[1].data(), h.lines[1].size(), "%s: %lld-%lld damage", item->name.c_str(),
                          static_cast<long long>(std::max<std::int64_t>(0, dice.count + dice.modifier)),
                          static_cast<long long>(std::max<std::int64_t>(
                              0, std::int64_t(dice.count) * dice.sides + dice.modifier)));
        }
        put(2, realTime(state_.combat.phase) ? "Cost: 6-second cooldown" : "Cost: 1 action");
        if (session_ && session_->combat().sneakPreview(command.character, *h.enemy,
                                                        command.action == PartyAction::Ranged,
                                                        session_->movement().player))
            put(2, "Sneak Attack: +1d6 on hit / one action");
        put(3, h.reason == CommandReason::Ready ? "Click to attack." : commandMessage(h.reason));
        if (session_)
            for (unsigned e = unsigned(EffectKind::Sleep); e < unsigned(EffectKind::Count); ++e)
                if (session_->effects().has(*h.enemy, EffectKind(e))) {
                    put(2, effectName(EffectKind(e)));
                    break;
                }
    } else if (h.kind == HoverKind::Action && h.index >= 0 && h.index < 6) {
        if (h.index == 4 && session_) {
            h.reason = CommandReason::Ready;
            put(0, "Inventory [I / 5]");
            put(1, "Browse bags, equipment and potions.");
            put(2, "Browsing pauses play.");
            put(3, "Click or press I to open.");
            return;
        }
        const auto action = dockActions[h.index];
        command.action = action;
        h.reason = validate(command);
        std::snprintf(h.lines[0].data(), h.lines[0].size(), "%s [%d]", actionName(action), h.index + 1);
        constexpr const char* effects[]{
            "Attack with your equipped weapon.",       "Attacks against you: disadvantage until next turn.",
            "Ready bow: six-cell range, clear sight.", "Open the paused spellbook and class abilities.",
            "Usable items are not available yet.",     "Wake a sleeper, or DC 10 Wisdom to stabilize."};
        put(1, effects[h.index]);
        put(2, h.index == 3   ? "Browsing pauses; targeting resumes play."
               : h.index == 4 ? "Unavailable"
                              : "Cost: 1 action");
        if (h.index == 3 && session_) {
            put(0, state_.magic.spellcaster ? "Spells [4]" : "Class Features [4]");
            put(1, state_.magic.spellcaster ? "Browse spells and activated class features."
                                            : "Browse activated and automatic class features.");
            put(2, "Browsing pauses; choosing a feature spends nothing.");
            put(3, "Click or press 4 to browse.");
            return;
        }
        if (h.index == 0 && party_.hasFeature(command.character, rules::ClassFeature::SneakAttack))
            put(1, "Rogue: melee can reach from the rear row.");
        if (h.reason != CommandReason::Ready && h.reason != CommandReason::MissingTarget &&
            h.reason != CommandReason::InvalidAlly && h.reason != CommandReason::PreviewAccepted)
            put(3, commandMessage(h.reason));
        else if (action == PartyAction::Aid) {
            put(2, "Cost: 1 action / no healing");
            put(3,
                state_.allyCount ? "Then click the ally's portrait." : "No sleeping or dying allies to aid.");
        } else if (action == PartyAction::Defend)
            put(3, "Click or press 2 to defend.");
        else
            put(3, "Click a reachable enemy.");

    } else if (h.kind == HoverKind::Portrait && h.index >= 0 && h.index < 4) {
        const auto& card = state_.cards[h.index];
        put(0, card.name);
        if (state_.magic.armed) {
            const auto& m = state_.magic;
            h.reason = m.allyReasons[h.index];
            if (m.pending.action == PartyAction::Feature)
                put(1, "This feature needs no recipient selection.");
            else if (m.hostile)
                put(1, m.selectedAllies[h.index] ? "WARNING: ally in affected area."
                                                 : "This ally is outside the affected area.");
            else if (m.selectedAllies[h.index]) {
                put(1, "Selected recipient");
                put(2, "Click to remove.");
            } else if (h.reason != CommandReason::Ready)
                put(1, commandMessage(h.reason));
            else
                put(1, m.single                             ? "Click to cast."
                       : m.pending.targetCount == m.maximum ? "Remove a target first."
                                                            : "Click to select.");
        } else if (state_.selection.action == PartyAction::Aid) {
            command.ally = card.id;
            h.reason = validate(command);
            put(1, h.reason == CommandReason::Ready ? "Click to aid. Cost: 1 action."
                                                    : commandMessage(h.reason));
        } else {
            put(1, realTime(state_.combat.phase) ? "Click to select. [i] opens details."
                                                 : "Click to inspect character.");
            put(2, "Read-only details; pauses combat.");
        }
    }
}
} // namespace blobber
