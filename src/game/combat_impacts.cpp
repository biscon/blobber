#include "game/combat.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace blobber {
std::uint32_t arrowFlightMs(Cell from, Cell to) noexcept {
    const double x = double(from.x) - to.x, y = double(from.y) - to.y;
    return static_cast<std::uint32_t>(
        std::clamp(std::round(180 + 54 * (std::sqrt(x * x + y * y) - 1)), 180., 450.));
}
AttackVisual& CombatController::visual(std::size_t i) noexcept {
    return i < attackCapacity ? visuals_.attacks[i] : visuals_.spellExtras[i - attackCapacity];
}
const AttackVisual& CombatController::visual(std::size_t i) const noexcept {
    return i < attackCapacity ? visuals_.attacks[i] : visuals_.spellExtras[i - attackCapacity];
}
std::size_t CombatController::ownerSlot(std::size_t i) noexcept {
    return i < attackCapacity ? i : NpcWorld::maxInstances + (i - attackCapacity) / 2;
}
bool CombatController::attackBusy(std::size_t slot) const noexcept {
    return slot < attackCapacity && visuals_.attacks[slot].stage != AttackStage::Empty;
}
bool CombatController::pendingImpacts() const noexcept {
    for (std::size_t i = 0; i < timelineCapacity; ++i) {
        const auto& v = visual(i);
        if (v.stage == AttackStage::Windup || v.stage == AttackStage::Flight)
            return true;
    }
    return false;
}
void CombatController::setAttackHold(std::size_t slot, std::uint64_t until) noexcept {
    auto& v = visuals_.attacks[slot];
    v.endMs = std::max(v.endMs, until);
    if (v.incoming)
        enemyBusy_[v.enemy.slot] = std::max(enemyBusy_[v.enemy.slot], v.endMs);
    else
        partyBusy_ = std::max(partyBusy_, v.endMs);
    if (pending_[slot].turn && state_.resolvingAction)
        due_ = std::max(due_, v.endMs);
}
CombatOutcome CombatController::queueAttack(CombatOutcome out, Player pose, std::uint64_t now) noexcept {
    const auto slot = out.enemyAttack ? out.enemy.slot : NpcWorld::maxInstances + out.character.index;
    assert(!attackBusy(slot));
    auto& p = pending_[slot];
    auto& v = visuals_.attacks[slot];
    p = {};
    v = {};
    const auto* a = actors_.actor(out.enemy);
    assert(a);
    v.id = {state_.world, nextAttack_++};
    v.stage = AttackStage::Windup;
    v.enemy = out.enemy;
    v.incoming = out.enemyAttack;
    v.character = out.enemyAttack ? out.recipient : out.character;
    v.ranged = out.kind == rules::AttackKind::Ranged;
    v.hit = out.hit;
    v.spell = out.spell;
    v.darts = out.darts;
    v.startedMs = now;
    v.partyOrigin = pose;
    v.npcOrigin = a->npc;
    v.sourcePosition = actorPosition(*a, now);
    v.targetPosition = v.sourcePosition;
    for (std::size_t i = 0; i < rules::partySize; ++i)
        if (party_.snapshot().formation[i].index == v.character.index)
            v.formationSlot = i;
    const auto& timing = presentation_[a->npc.type.index];
    const auto windup = out.enemyAttack ? out.impactMs : (v.ranged ? 0u : 200u);
    v.releaseMs = now + windup;
    v.impactMs = v.releaseMs + (v.ranged ? arrowFlightMs(pose.cell, a->npc.cell) : 0);
    p.clipEndMs = now + (out.enemyAttack ? (v.ranged ? timing.bow : timing.attack) : windup);
    p.turn = state_.phase == CombatPhase::Turns;
    if (v.ranged && !windup) {
        v.stage = AttackStage::Flight;
        p.releaseNotice = true;
    }
    out.attackId = v.id;
    out.occurredMs = now;
    p.result = out;
    if (p.turn) {
        state_.awaitingAction = false;
        state_.resolvingAction = true;
        due_ = now;
    }
    setAttackHold(slot, std::max(p.clipEndMs, v.impactMs + combatFeedbackDurationMs));
    ++visuals_.revision;
    out.event = CombatEvent::AttackStarted;
    out.impactMs = windup;
    out.hit = out.critical = out.died = false;
    out.damage = out.roll = 0;
    return out;
}
void CombatController::interruptWindup(ActorId id, std::uint64_t when) noexcept {
    if (id.world != state_.world || id.slot >= NpcWorld::maxInstances)
        return;
    auto& v = visuals_.attacks[id.slot];
    if (v.stage != AttackStage::Windup)
        return;
    const auto* a = actors_.actor(id);
    if (!a || a->life != ActorLife::Alive)
        return; // Cancellation is drained before the next deadline.
    auto& p = pending_[id.slot];
    const auto duration = presentation_[a->npc.type.index].hurt;
    if (!duration)
        return;
    const auto until = when + duration;
    const auto extra = until > std::max(when, p.hurtUntilMs) ? until - std::max(when, p.hurtUntilMs) : 0;
    p.hurtUntilMs = std::max(p.hurtUntilMs, until);
    v.releaseMs += extra;
    v.impactMs += extra;
    p.clipEndMs += extra;
    actors_.suspendAttack(id, when, duration);
    setAttackHold(id.slot, v.endMs + extra);
    ++visuals_.revision;
}
void CombatController::cancelAttack(std::size_t slot, std::uint64_t when, CombatEvents& events,
                                    bool targetGone) noexcept {
    auto& v = visuals_.attacks[slot];
    auto out = pending_[slot].result;
    out.event = CombatEvent::AttackCanceled;
    out.occurredMs = when;
    out.targetGone = targetGone;
    out.hit = out.critical = out.died = false;
    out.damage = out.roll = 0;
    out.phase = state_.phase;
    events.append(out);
    v.stage = AttackStage::Canceled;
    v.targetGone = targetGone;
    if (v.incoming)
        actors_.cancelAttack(v.enemy);
    drainedThisUpdate_ = true;
    ++visuals_.revision;
}
void CombatController::reconcileEffects(std::uint64_t now, CombatEvents& events) noexcept {
    effects_.reconcile(now, events);
    for (std::uint16_t i = 0; i < rules::partySize; ++i)
        if (!effects_.canAct(rules::CharacterId{i}))
            state_.defending[i] = false;
    if (state_.activeCharacter && !effects_.canAct(*state_.activeCharacter))
        state_.awaitingAction = false;
    for (std::size_t i = 0; i < attackCapacity; ++i) {
        const auto& v = visuals_.attacks[i];
        if (v.stage == AttackStage::Windup &&
            !(v.incoming ? effects_.canAct(v.enemy) : effects_.canAct(v.character)))
            cancelAttack(i, now, events, false);
    }
}
void CombatController::resolveImpacts(Player pose, std::uint64_t now, CombatEvents& events,
                                      std::uint64_t elapsed) noexcept {
    drainedThisUpdate_ = false;
    auto cursor = now - elapsed;
    const bool realtime = realTime(state_.phase);
    reconcileEffects(cursor, events);
    const auto advanceEffects = [&](std::uint64_t when) {
        if (when > cursor) {
            elapse(when - cursor);
            cursor = when;
        }
        effects_.advance(state_.gameplayMs, when, realtime, events);
    };
    for (std::size_t i = 0; i < attackCapacity; ++i) {
        auto& p = pending_[i];
        const auto& v = visuals_.attacks[i];
        if (p.releaseNotice) {
            auto out = p.result;
            out.event = CombatEvent::ProjectileReleased;
            out.occurredMs = v.releaseMs;
            out.hit = out.critical = false;
            out.damage = out.roll = 0;
            events.append(out);
            p.releaseNotice = false;
        }
        if (v.stage != AttackStage::Windup || v.releaseMs <= now)
            continue;
        const auto* a = actors_.actor(v.enemy);
        const auto* c = party_.character(v.character);
        const bool sourceAlive = v.incoming ? effects_.canAct(v.enemy) : effects_.canAct(v.character);
        const bool targetAlive =
            v.incoming ? c && c->life != rules::Life::Dead : a && a->life == ActorLife::Alive;
        if (!sourceAlive || !targetAlive)
            cancelAttack(i, cursor, events, !targetAlive);
    }
    // Each record has at most release + impact. Rescanning bounded contiguous
    // storage avoids heap queues and tolerates deadline changes caused by hurt.
    for (;;) {
        std::size_t next = timelineCapacity;
        std::uint64_t earliest = std::numeric_limits<std::uint64_t>::max();
        bool nextFlying = false;
        for (std::size_t i = 0; i < timelineCapacity; ++i) {
            const auto& v = visual(i);
            if (v.stage != AttackStage::Windup && v.stage != AttackStage::Flight)
                continue;
            const bool flying = v.stage == AttackStage::Flight;
            const auto due = flying ? v.impactMs : v.releaseMs;
            if (due > now)
                continue;
            if (next == timelineCapacity || due < earliest ||
                (due == earliest &&
                 (flying > nextFlying || (flying == nextFlying && v.id.serial < visual(next).id.serial)))) {
                next = i;
                earliest = due;
                nextFlying = flying;
            }
        }
        if (next == timelineCapacity)
            break;
        advanceEffects(earliest);
        auto& v = visual(next);
        const auto owner = ownerSlot(next);
        auto& p = pending_[owner];
        const auto* a = actors_.actor(v.enemy);
        const auto* character = party_.character(v.character);
        const bool targetAlive =
            v.incoming ? character && character->life != rules::Life::Dead : a && a->life == ActorLife::Alive;
        if (v.stage == AttackStage::Windup) {
            const bool sourceAlive = v.incoming ? effects_.canAct(v.enemy) : effects_.canAct(v.character);
            if (!sourceAlive || !targetAlive) {
                cancelAttack(next, earliest, events, !targetAlive);
                continue;
            }
            if (v.ranged) {
                v.partyOrigin = pose;
                if (v.incoming) {
                    v.npcOrigin = a->npc;
                    v.sourcePosition = actorPosition(*a, earliest);
                }
                v.impactMs = earliest + arrowFlightMs(pose.cell, a->npc.cell);
                setAttackHold(owner, std::max(p.clipEndMs, v.impactMs + combatFeedbackDurationMs));
                v.stage = AttackStage::Flight;
                auto out = p.result;
                out.event = CombatEvent::ProjectileReleased;
                out.occurredMs = earliest;
                out.hit = out.critical = out.died = false;
                out.damage = out.roll = 0;
                out.phase = state_.phase;
                events.append(out);
                ++visuals_.revision;
                continue;
            }
        }
        auto out =
            next < attackCapacity
                ? p.result
                : spellPayloads_[(next - attackCapacity) / 2].outcomes[1 + (next - attackCapacity) % 2];
        out.event = CombatEvent::Attack;
        out.occurredMs = earliest;
        out.phase = state_.phase;
        out.targetGone = !targetAlive;
        // An accepted attack does not reveal its source. Contact (including a miss)
        // alerts before damage so a lethal impact still warns nearby survivors.
        if (!v.incoming && targetAlive)
            alert(out.enemy, pose);
        out.phase = state_.phase;
        applyDamage(out, earliest, events);
        if (!v.incoming && !out.targetGone) {
            const auto* victim = actors_.actor(out.enemy);
            v.targetPosition = actorPosition(*victim, earliest);
            const auto& timing = presentation_[victim->npc.type.index];
            if (out.hit)
                setAttackHold(owner, earliest + (out.died ? timing.death : timing.hurt));
        }
        v.stage = AttackStage::Resolved;
        v.targetGone = out.targetGone;
        drainedThisUpdate_ = true;
        ++visuals_.revision;
        // Cancel incapacitated windups immediately at this impact's timestamp,
        // including shots with later release deadlines. Flying arrows survive.
        for (std::size_t i = 0; i < attackCapacity; ++i) {
            const auto& other = visuals_.attacks[i];
            if (other.stage != AttackStage::Windup)
                continue;
            const auto* npc = actors_.actor(other.enemy);
            const auto* ch = party_.character(other.character);
            const bool sourceAlive =
                other.incoming ? effects_.canAct(other.enemy) : effects_.canAct(other.character);
            const bool liveTarget =
                other.incoming ? ch && ch->life != rules::Life::Dead : npc && npc->life == ActorLife::Alive;
            if (!sourceAlive || !liveTarget)
                cancelAttack(i, earliest, events, !liveTarget);
        }
    }
    advanceEffects(now);
    for (std::size_t i = 0; i < timelineCapacity; ++i) {
        auto& v = visual(i);
        if ((v.stage == AttackStage::Resolved || v.stage == AttackStage::Canceled) && now >= v.endMs) {
            v.stage = AttackStage::Empty;
            ++visuals_.revision;
        }
    }
}
void CombatController::applyDamage(CombatOutcome& out, std::uint64_t when, CombatEvents& events) noexcept {
    const auto* target = actors_.actor(out.enemy);
    const auto* recipient = party_.character(out.recipient);
    out.targetGone = out.enemyAttack ? !recipient || recipient->life == rules::Life::Dead
                                     : !target || target->life != ActorLife::Alive;
    if (out.targetGone) {
        out.hit = out.critical = out.died = false;
        out.damage = 0;
    } else if (out.hit) {
        if (out.enemyAttack) {
            out.died = party_.damage(out.recipient, out.damage, out.critical).died;
            if (party_.character(out.recipient)->life != rules::Life::Conscious)
                state_.defending[out.recipient.index] = false;
        } else {
            out.died = actors_.damage(out.enemy, out.damage, when).died;
            if (out.damage > 0)
                interruptWindup(out.enemy, when);
        }
    }
    events.append(out);
    if (out.hit && out.enemyAttack)
        effects_.damaged(out.recipient, out.damage, when, events);
    else
        effects_.reconcile(when, events);
    if (out.hit && !out.targetGone && !out.died && out.spell == rules::SpellKind::GuidingBolt)
        effects_.guide(out.enemy, when, events);
}
} // namespace blobber
