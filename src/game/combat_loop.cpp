#include "game/combat.hpp"
#include "game/perception.hpp"
#include <algorithm>
namespace blobber {
CombatOutcome CombatController::update(Player pose, bool moving, std::uint64_t now) noexcept {
    CombatOutcome out;
    origin_ = pose;
    if (state_.phase == CombatPhase::Victory || state_.phase == CombatPhase::Defeat) {
        out.phase = state_.phase;
        return out;
    }
    if (state_.phase == CombatPhase::Facing) {
        if (!moving && state_.facing && pose.heading == *state_.facing) {
            state_.phase = resumePhase_;
            state_.facing.reset();
            state_.awaitingAction = resumeAwaiting_;
            ++state_.turn;
        }
        out.phase = state_.phase;
        return out;
    }
    if (state_.resolvingAction) {
        if (now >= due_) {
            state_.resolvingAction = false;
            advance(now);
            if (state_.phase == CombatPhase::Victory || state_.phase == CombatPhase::Defeat)
                out.event = CombatEvent::Finished;
        }
        out.phase = state_.phase;
        return out;
    }
    if (realTime(state_.phase)) {
        bool aware = false;
        for (std::size_t i = 0; i < actors_.snapshot().count; ++i)
            aware |= ai_.aware(actors_.id(i));
        if (aware) {
            episode_ = true;
            enroll(pose, false);
            state_.phase = CombatPhase::Approach;
        }
        if (finish(now)) {
            out.event = CombatEvent::Finished;
            out.phase = state_.phase;
            return out;
        }
        bool conscious = false;
        for (const auto& c : party_.snapshot().characters)
            conscious |= c.life == rules::Life::Conscious;
        if (conscious && state_.phase == CombatPhase::Approach) {
            for (std::uint16_t i = 0; i < rules::partySize; ++i) {
                if (party_.character({i})->life != rules::Life::Dying) {
                    saveDue_[i] = 0;
                    continue;
                }
                if (!saveDue_[i])
                    saveDue_[i] = state_.gameplayMs + combatRoundMs;
                out = deathSave({i}, now);
                if (out.event != CombatEvent::None)
                    break;
            }
            if (out.event == CombatEvent::None) {
                const auto count = actors_.snapshot().count;
                for (std::size_t n = 0; n < count; ++n) {
                    const auto i = (autoCursor_ + n) % count;
                    const auto id = actors_.id(i);
                    const auto* a = actors_.actor(id);
                    if (!a || a->life != ActorLife::Alive || !ai_.aware(id) || actorWalking(*a, now) ||
                        now < enemyBusy_[i] || enemyReady_[i] > state_.gameplayMs ||
                        !actors_.catalog().attack(ranged_[i]) || !inBowRange(a->npc.cell, pose.cell) ||
                        !frontal(id))
                        continue;
                    out = enemyAttack(id, true, now, !known_[i]);
                    autoCursor_ = (i + 1) % count;
                    break;
                }
            }
        }
    } else if (state_.phase == CombatPhase::Contact) {
        moving |= now < partyBusy_;
        for (std::size_t i = 0; i < state_.enemyCount; ++i) {
            const auto id = state_.enemies[i];
            const auto* a = actors_.actor(id);
            moving |= a && a->life == ActorLife::Alive && actorWalking(*a, now);
            moving |= now < enemyBusy_[id.slot];
        }
        if (!moving) {
            if (finish(now)) {
                out.event = CombatEvent::Finished;
                out.phase = state_.phase;
                return out;
            }
            if (settleFront(now)) {
                out.phase = state_.phase;
                return out;
            }
            begin(now);
            out.event = CombatEvent::Started;
            if (finish(now))
                out.event = CombatEvent::Finished;
        }
    } else if (state_.phase == CombatPhase::RoundEnd) {
        if (now >= due_) {
            state_.gameplayMs += combatRoundMs;
            refreshReadiness();
            std::array<bool, NpcWorld::maxInstances> frozen{};
            std::array<Cell, NpcWorld::maxInstances> before{};
            for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
                const auto id = actors_.id(i);
                before[i] = actors_.snapshot().actors[i].npc.cell;
                frozen[i] =
                    (members_[i] && frontal(id)) || now < enemyBusy_[i] || enemyReady_[i] > state_.gameplayMs;
            }
            if (enableRoaming_)
                ai_.update(combatRoundMs, now, pose, &frozen);
            for (std::size_t i = 0; i < actors_.snapshot().count; ++i)
                if (!(before[i] == actors_.snapshot().actors[i].npc.cell))
                    enemyReady_[i] = state_.gameplayMs + combatRoundMs;
            observe(pose);
            enroll(pose, false);
            addGroups();
            surprised_.fill(false);
            if (finish(now))
                out.event = CombatEvent::Finished;
            else {
                ++state_.round;
                position_ = enemyCursor_ = 0;
                state_.spent.fill(false);
                state_.phase = CombatPhase::Turns;
                state_.awaitingAction = false;
                state_.activeCharacter.reset();
                state_.activeEnemy.reset();
                ++state_.turn;
                if (!settleFront(now))
                    selectTurn();
                out.event =
                    state_.phase == CombatPhase::Approach ? CombatEvent::Disengaged : CombatEvent::Round;
            }
        }
    } else if (state_.phase == CombatPhase::Turns) {
        if (finish(now))
            out.event = CombatEvent::Finished;
        else if (!moving && settleFront(now)) {
            out.phase = state_.phase;
            return out;
        } else if (now >= due_ && !moving) {
            if (state_.activeCharacter) {
                const auto id = *state_.activeCharacter;
                const auto life = party_.character(id)->life;
                if (life == rules::Life::Conscious && !state_.spent[id.index])
                    state_.awaitingAction = true;
                else {
                    state_.awaitingAction = false;
                    out = deathSave(id, now);
                    if (party_.character(id)->life == rules::Life::Conscious && !state_.spent[id.index])
                        state_.awaitingAction = true;
                    else {
                        if (out.event == CombatEvent::None) {
                            out.event = CombatEvent::Spent;
                            out.character = id;
                        }
                        advance(now);
                    }
                }
            } else if (state_.activeEnemy) {
                const auto* a = actors_.actor(*state_.activeEnemy);
                if (a && (actorWalking(*a, now) || now < enemyBusy_[state_.activeEnemy->slot])) {
                    out.phase = state_.phase;
                    return out;
                }
                out = enemyTurn(*state_.activeEnemy, now);
                if (!state_.resolvingAction)
                    advance(now);
            } else
                selectTurn();
        }
    }
    refreshReadiness();
    out.phase = state_.phase;
    return out;
}
} // namespace blobber
