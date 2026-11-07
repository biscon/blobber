#include "game/commands.hpp"
#include "game/perception.hpp"
#include "ui/layout.hpp"
#include "ui/player_text.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>

namespace blobber {
namespace {
bool encounterActive(CombatPhase phase) noexcept {
    return phase == CombatPhase::Contact || phase == CombatPhase::Facing || phase == CombatPhase::Turns ||
           phase == CombatPhase::RoundEnd;
}
} // namespace
bool CommandController::pickerTarget(ActorId id) const noexcept {
    const auto* actor = actors_.actor(id);
    return encounterActive(session_.combat().snapshot().phase) && actor && actor->hostile() &&
           actor->life == ActorLife::Alive && session_.combat().member(id);
}
bool CommandController::pickerRecipients() const noexcept {
    const auto& m = state_.magic;
    const auto& c = m.armed ? m.pending : state_.selection;
    if (!m.armed || m.single)
        return c.target && pickerTarget(*c.target) && visible(*c.target);
    if (!c.targetCount || c.targetCount > c.enemies.size())
        return false;
    for (std::size_t i = 0; i < c.targetCount; ++i)
        if (!pickerTarget(c.enemies[i]) || !visible(c.enemies[i]))
            return false;
    return true;
}
bool CommandController::sameTargetContext(const PartyCommand& a, const PartyCommand& b) const noexcept {
    return a.world == b.world && a.turn == b.turn && a.character.index == b.character.index &&
           a.action == b.action && a.spell.index == b.spell.index && a.feature == b.feature;
}
CommandReason CommandController::enemyEligibility(ActorId id) const noexcept {
    auto c = state_.magic.armed ? state_.magic.pending : state_.selection;
    if (state_.magic.armed) {
        const auto reason = magicEligibility(id);
        if (reason != CommandReason::Ready)
            return reason;
        c.targetCount = 1;
        c.enemies[0] = id;
        c.darts[0] = state_.magic.missiles ? 3 : 0;
        if (c.feature == rules::Feature::SpiritualWeapon)
            c.aimedCell = actors_.actor(id)->npc.cell;
    }
    c.target = id;
    return validate(c);
}
void CommandController::refreshTargetPicker() noexcept {
    auto& p = state_.targetPicker;
    const auto& m = state_.magic;
    const auto& c = m.armed ? m.pending : state_.selection;
    const bool wasAvailable = p.available;
    p.available =
        !state_.suspended && !state_.modal() && !state_.inventory.itemTargeting && !state_.campaignTerminal &&
        encounterActive(session_.combat().snapshot().phase) &&
        (m.armed ? m.hostile && !m.area && (!m.ground || m.single) && !m.self
                 : !m.casting() && (c.action == PartyAction::Melee || c.action == PartyAction::BackMelee ||
                                    c.action == PartyAction::Ranged));
    if (!p.available || !sameTargetContext(c, targetContext_)) {
        p.open = false;
        p.feedback = {};
    }
    targetContext_ = c;
    bool changed = wasAvailable != p.available;
    std::size_t count = 0;
    if (p.available && state_.targets.world == actors_.snapshot().world &&
        state_.targets.revision == actors_.snapshot().revision)
        for (std::size_t i = 0; i < state_.targets.candidateCount; ++i) {
            const auto id = state_.targets.candidates[i];
            if (!pickerTarget(id))
                continue;
            changed |= count >= p.candidateCount || !sameActor(p.candidates[count], id);
            p.candidates[count++] = id;
        }
    changed |= count != p.candidateCount;
    p.candidateCount = count;
    if (changed)
        ++p.revision;
    const auto pages = (count + 3) / 4;
    state_.targetPage = pages ? std::min(state_.targetPage, pages - 1) : 0;
    p.canConfirm = p.available && pickerRecipients() &&
                   (m.armed && m.single && c.target ? enemyEligibility(*c.target) == CommandReason::Ready
                                                    : state_.availability == CommandReason::Ready);
    p.rows = {};
    p.count = 0;
    const auto describe = [&](ActorId id, EnemyTargetRow& row) {
        row.id = id;
        const auto& a = *actors_.actor(id);
        std::snprintf(row.name.data(), row.name.size(), "%s #%u",
                      monsterName(actors_.catalog().monster(a.monster)->id), unsigned(id.slot) + 1);
        row.reason = enemyEligibility(id);
        const auto direction = forward(session_.movement().player.heading);
        const bool rear =
            (2 * a.npc.subcell.x - 1) * direction.x + (2 * a.npc.subcell.y - 1) * direction.y > 0;
        std::snprintf(row.detail.data(), row.detail.size(), "%s%s / %s", rear ? "Rear" : "Front",
                      state_.targets.anchors[id.slot].valid ? "" : " / Obscured",
                      row.reason == CommandReason::Ready ? "Ready" : commandMessage(row.reason));
        row.selected = c.target && sameActor(*c.target, id);
    };
    for (std::size_t i = state_.targetPage * 4; i < p.candidateCount && p.count < 4; ++i)
        describe(p.candidates[i], p.rows[p.count++]);
}
void CommandController::toggleTargetPicker() noexcept {
    refreshTargetPicker();
    if (state_.targetPicker.available) {
        state_.targetPicker.open = !state_.targetPicker.open;
        state_.targetPicker.feedback = {};
    }
}
void CommandController::hoverEnemyTarget(ActorId id) noexcept {
    if (!visible(id)) {
        hover(HoverKind::None);
        return;
    }
    const auto a = state_.targets.anchors[id.slot];
    // Hidden targets have no world-space tooltip; their identity stays in the dock.
    hover(HoverKind::Enemy, 0, id, float(a.valid ? viewX + a.x : 12), float(a.valid ? viewY + a.y : 300));
}
void CommandController::activateTargetRow(unsigned row, bool remove) noexcept {
    auto& p = state_.targetPicker;
    if (!p.open || row >= p.count || state_.suspended)
        return;
    const auto id = p.rows[row].id;
    refreshTargetPicker();
    if (!p.open || !p.available || !pickerTarget(id) || !visible(id))
        return;
    // A changed list must never redirect an old row activation to a new actor.
    if (remove && state_.magic.armed && !state_.magic.single) {
        magicEnemy(id, true);
    } else {
        const auto reason = enemyEligibility(id);
        if (reason != CommandReason::Ready) {
            std::snprintf(p.feedback.data(), p.feedback.size(), "%s", commandMessage(reason));
            return;
        }
        p.feedback = {};
        if (state_.magic.armed)
            magicEnemy(id);
        else
            selectTarget(id);
    }
    hoverEnemyTarget(id);
    refreshTargetPicker();
}
void CommandController::cycleEnemyTarget(int delta) noexcept {
    refreshTargetPicker();
    const auto& p = state_.targetPicker;
    if (!p.available || !p.candidateCount)
        return;
    const auto count = p.candidateCount;
    const auto current = state_.magic.armed ? state_.magic.pending.target : state_.selection.target;
    std::size_t index = delta < 0 ? 0 : count - 1;
    if (current)
        for (std::size_t i = 0; i < count; ++i)
            if (sameActor(*current, p.candidates[i]))
                index = i;
    for (std::size_t n = 0; n < count; ++n) {
        index = (index + (delta < 0 ? count - 1 : 1)) % count;
        const auto id = p.candidates[index];
        if (enemyEligibility(id) != CommandReason::Ready)
            continue;
        if (state_.magic.armed) {
            state_.magic.pending.target = id;
            state_.targetPage = index / 4;
            updateAvailability();
        } else
            selectTarget(id);
        hoverEnemyTarget(id);
        state_.targetPicker.feedback = {};
        return;
    }
    std::snprintf(state_.targetPicker.feedback.data(), state_.targetPicker.feedback.size(),
                  "No eligible enemies for this action.");
}
void CommandController::targetPickerConfirm() noexcept {
    refreshTargetPicker();
    if (!state_.targetPicker.available)
        return;
    if (!pickerRecipients()) {
        std::snprintf(state_.targetPicker.feedback.data(), state_.targetPicker.feedback.size(),
                      "Choose an enemy in this encounter.");
        return;
    }
    const auto reason = confirm();
    if (reason != CommandReason::Resolved) {
        std::snprintf(state_.targetPicker.feedback.data(), state_.targetPicker.feedback.size(), "%s",
                      commandMessage(reason));
    }
}
} // namespace blobber
