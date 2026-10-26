#include "game/combat.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace blobber {
CombatController::CombatController(Party& party, ActorWorld& actors, rules::DiceRng& rng,
                                   const std::vector<NpcTimings>& timings)
    : party_(party), actors_(actors), rng_(rng) {
    if (&party.catalog() != &actors.catalog())
        throw std::runtime_error("Combat owners must share one catalog");
    if (!timings.empty() && timings.size() != actors.snapshot().typeCount)
        throw std::invalid_argument("Combat animation/world type counts differ");
    for (std::size_t i = 0; i < timings.size(); ++i) {
        validateNpcTimings(timings[i]);
        const auto& t = timings[i];
        const auto& attack = t[static_cast<std::size_t>(NpcAnimation::Attack)];
        presentation_[i] = {npcClipDuration(attack), npcAttackImpact(attack),
                            npcClipDuration(t[static_cast<std::size_t>(NpcAnimation::Hurt)]),
                            npcClipDuration(t[static_cast<std::size_t>(NpcAnimation::Death)])};
    }
    state_.world = actors.snapshot().world;
    for (std::size_t i = 0; i < actors.snapshot().count; ++i) {
        const auto& actor = actors.snapshot().actors[i];
        const auto& monster = *actors.catalog().monster(actor.monster);
        for (std::size_t j = 0; j < monster.attackCount; ++j)
            if (actors.catalog().attack(monster.attacks[j])->kind == rules::AttackKind::Melee) {
                melee_[i] = monster.attacks[j];
                break;
            }
        if (!actors.catalog().attack(melee_[i]))
            throw std::runtime_error("Stationary combat requires a melee attack for every monster");
    }
}
void CombatController::previewTurns(CombatSnapshot& destination) const noexcept {
    destination.upcomingCount = 0;
    if (state_.phase != CombatPhase::Turns)
        return;
    auto occupied = occupiedSlots();
    for (std::size_t position = position_; position < orderCount_; ++position) {
        const auto entry = order_[position];
        if (entry.party) {
            const auto life = party_.character({entry.index})->life;
            if (life == rules::Life::Conscious || life == rules::Life::Dying)
                destination.upcoming[destination.upcomingCount++] = {
                    rules::CharacterId{entry.index}, {}, life == rules::Life::Dying};
        } else {
            for (std::size_t i = position == position_ ? enemyCursor_ : 0; i < state_.enemyCount; ++i) {
                const auto id = state_.enemies[i];
                const auto* actor = actors_.actor(id);
                if (actor && actor->monster.index == groups_[entry.index].index) {
                    if (const auto slot = meleeSlot(id, occupied)) {
                        destination.upcoming[destination.upcomingCount++] = {{}, id, false};
                        // Predict vacancy claims in turn order without mutating ActorWorld.
                        occupied |= 1u << (slot->x + 2 * slot->y);
                    }
                }
            }
        }
    }
}
bool CombatController::member(ActorId id) const noexcept {
    for (std::size_t i = 0; i < state_.enemyCount; ++i)
        if (sameActor(state_.enemies[i], id))
            return true;
    return false;
}
bool CombatController::near(const ActorState& actor) const noexcept {
    switch (origin_.heading) {
    case Heading::North:
        return actor.npc.subcell.y == 1;
    case Heading::East:
        return actor.npc.subcell.x == 0;
    case Heading::South:
        return actor.npc.subcell.y == 0;
    case Heading::West:
        return actor.npc.subcell.x == 1;
    }
    return false;
}
bool CombatController::exposed(rules::CharacterId id) const noexcept {
    const auto& f = party_.snapshot().formation;
    if (f[0].index == id.index || f[1].index == id.index)
        return true;
    for (std::size_t i = 2; i < f.size(); ++i)
        if (f[i].index == id.index)
            return party_.character(f[i - 2])->life != rules::Life::Conscious;
    return false;
}
std::uint8_t CombatController::occupiedSlots() const noexcept {
    std::uint8_t occupied = 0;
    for (std::size_t i = 0; i < state_.enemyCount; ++i) {
        const auto* actor = actors_.actor(state_.enemies[i]);
        if (actor && actor->life == ActorLife::Alive && actor->npc.cell == cell_)
            occupied |= 1u << (actor->npc.subcell.x + 2 * actor->npc.subcell.y);
    }
    return occupied;
}
std::optional<Cell> CombatController::meleeSlot(ActorId id, std::uint8_t occupied) const noexcept {
    const auto* actor = actors_.actor(id);
    if (!actor || actor->life != ActorLife::Alive || !member(id) || !(actor->npc.cell == cell_))
        return {};
    if (near(*actor))
        return actor->npc.subcell;
    auto candidate = actor->npc.subcell + forward(origin_.heading) * -1;
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!(occupied & (1u << (candidate.x + 2 * candidate.y))))
            return candidate;
        if (origin_.heading == Heading::North || origin_.heading == Heading::South)
            candidate.x = 1 - candidate.x;
        else
            candidate.y = 1 - candidate.y;
    }
    return {};
}
bool CombatController::exposed(ActorId id) const noexcept {
    const auto* actor = actors_.actor(id);
    if (!actor || !member(id))
        return false;
    if (near(*actor))
        return true;
    for (std::size_t i = 0; i < state_.enemyCount; ++i) {
        const auto* front = actors_.actor(state_.enemies[i]);
        const bool sameColumn =
            front && ((origin_.heading == Heading::North || origin_.heading == Heading::South)
                          ? front->npc.subcell.x == actor->npc.subcell.x
                          : front->npc.subcell.y == actor->npc.subcell.y);
        if (sameColumn && front->life == ActorLife::Alive && near(*front))
            return false;
    }
    return true;
}
bool CombatController::contact(Player pose) noexcept {
    if (state_.phase != CombatPhase::Exploration)
        return false;
    const auto ahead = pose.cell + forward(pose.heading);
    if (!actors_.occupied(ahead))
        return false;
    origin_ = pose;
    cell_ = ahead;
    state_.enemyCount = 0;
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto& actor = actors_.snapshot().actors[i];
        if (actor.life == ActorLife::Alive && actor.npc.cell == cell_) {
            // ActorWorld occupancy guarantees at most four distinct subslots per cell.
            if (state_.enemyCount == state_.enemies.size())
                return false;
            state_.enemies[state_.enemyCount++] = actors_.id(i);
        }
    }
    if (!state_.enemyCount)
        return false;
    state_.phase = CombatPhase::Contact;
    party_.lockFormation(true);
    return true;
}
bool CombatController::finish() noexcept {
    bool livingEnemy = false, conscious = false;
    for (std::size_t i = 0; i < state_.enemyCount; ++i) {
        const auto* actor = actors_.actor(state_.enemies[i]);
        livingEnemy |= actor && actor->life == ActorLife::Alive;
    }
    for (const auto& c : party_.snapshot().characters)
        conscious |= c.life == rules::Life::Conscious;
    if (livingEnemy && conscious)
        return false;
    state_.phase = conscious ? CombatPhase::Victory : CombatPhase::Defeat;
    state_.awaitingAction = false;
    state_.activeCharacter.reset();
    state_.activeEnemy.reset();
    state_.defending.fill(false);
    if (conscious)
        for (std::uint16_t i = 0; i < rules::partySize; ++i)
            if (party_.character({i})->life == rules::Life::Dying)
                party_.stabilize({i});
    return true;
}
void CombatController::begin(std::uint64_t now) noexcept {
    const auto facing = static_cast<Heading>((static_cast<unsigned>(origin_.heading) + 2) % 4);
    for (std::size_t i = 0; i < state_.enemyCount; ++i)
        actors_.face(state_.enemies[i], facing);
    state_.phase = CombatPhase::Turns;
    if (finish())
        return;
    state_.round = 1;
    groupCount_ = orderCount_ = position_ = enemyCursor_ = 0;
    for (std::uint16_t i = 0; i < rules::partySize; ++i) {
        const auto& c = *party_.character({i});
        order_[orderCount_++] = {true, i, rng_.d20().value.selected + c.stats.initiative,
                                 c.stats.abilities[static_cast<std::size_t>(rules::Ability::Dexterity)]};
    }
    for (std::size_t i = 0; i < state_.enemyCount; ++i) {
        const auto* actor = actors_.actor(state_.enemies[i]);
        if (!actor || actor->life != ActorLife::Alive)
            continue;
        const auto monster = actor->monster;
        bool found = false;
        for (std::size_t j = 0; j < groupCount_; ++j)
            found |= groups_[j].index == monster.index;
        if (!found)
            groups_[groupCount_++] = monster;
    }
    for (std::uint16_t i = 0; i < groupCount_; ++i) {
        const auto& m = *actors_.catalog().monster(groups_[i]);
        const int dex = m.abilities[static_cast<std::size_t>(rules::Ability::Dexterity)];
        order_[orderCount_++] = {false, i, rng_.d20().value.selected + rules::abilityModifier(dex).value,
                                 dex};
    }
    std::sort(order_.begin(), order_.begin() + orderCount_, [](const Entry& a, const Entry& b) {
        if (a.initiative != b.initiative)
            return a.initiative > b.initiative;
        if (a.dexterity != b.dexterity)
            return a.dexterity > b.dexterity;
        if (a.party != b.party)
            return a.party;
        return a.index < b.index;
    });
    due_ = now + actionDelayMs;
    if (!finish())
        selectTurn();
}
void CombatController::selectTurn() noexcept {
    state_.awaitingAction = false;
    state_.activeCharacter.reset();
    state_.activeEnemy.reset();
    const auto occupied = occupiedSlots();
    // A conscious party member guarantees a selectable turn within one full cycle.
    for (std::size_t skipped = 0; skipped <= orderCount_; ++skipped) {
        const auto& entry = order_[position_];
        if (entry.party) {
            state_.defending[entry.index] = false;
            const auto& c = *party_.character({entry.index});
            if (c.life == rules::Life::Conscious || c.life == rules::Life::Dying) {
                state_.activeCharacter = rules::CharacterId{entry.index};
                state_.awaitingAction = c.life == rules::Life::Conscious;
                ++state_.turn;
                return;
            }
        } else {
            for (; enemyCursor_ < state_.enemyCount; ++enemyCursor_) {
                const auto id = state_.enemies[enemyCursor_];
                const auto* a = actors_.actor(id);
                if (a && a->life == ActorLife::Alive && a->monster.index == groups_[entry.index].index &&
                    meleeSlot(id, occupied)) {
                    state_.activeEnemy = id;
                    ++state_.turn;
                    return;
                }
            }
        }
        enemyCursor_ = 0;
        if (++position_ == orderCount_) {
            position_ = 0;
            ++state_.round;
        }
    }
}
void CombatController::holdAttack(const CombatOutcome& out, std::uint64_t now) noexcept {
    const auto& t = presentation_[actors_.actor(out.enemy)->npc.type.index];
    const std::uint64_t duration =
        out.enemyAttack
            ? std::max<std::uint64_t>(t.attack, t.impact + combatFeedbackDurationMs)
            : std::max<std::uint64_t>(combatFeedbackDurationMs, out.hit ? (out.died ? t.death : t.hurt) : 0);
    state_.awaitingAction = false;
    state_.resolvingAction = true;
    due_ = now + std::max(actionDelayMs, duration);
}
void CombatController::advance(std::uint64_t now) noexcept {
    due_ = now + actionDelayMs;
    if (finish())
        return;
    if (state_.activeEnemy)
        ++enemyCursor_;
    else if (++position_ == orderCount_) {
        position_ = 0;
        ++state_.round;
    }
    selectTurn();
}
const rules::WeaponPreview* CombatController::weapon(rules::CharacterId id) const noexcept {
    const auto* c = party_.character(id);
    if (!c)
        return nullptr;
    const auto i = c->build.equipment.mainHand.index;
    if (i >= c->build.itemCount)
        return nullptr;
    const auto& w = c->stats.weapons[i];
    return w.available && !w.ranged ? &w : nullptr;
}
CommandReason CombatController::validate(const PartyCommand& c, Player pose) const noexcept {
    if (!party_.character(c.character))
        return CommandReason::InvalidCharacter;
    if (static_cast<unsigned>(c.action) >= static_cast<unsigned>(PartyAction::Count))
        return CommandReason::InvalidAction;
    if (c.action == PartyAction::BackMelee)
        return CommandReason::FeatureRequired;
    if (c.action == PartyAction::Ranged || c.action == PartyAction::Spell || c.action == PartyAction::Item)
        return CommandReason::Unavailable;
    if (state_.phase != CombatPhase::Turns)
        return CommandReason::NoEncounter;
    if (c.world != state_.world || c.turn != state_.turn)
        return CommandReason::StaleTurn;
    if (!state_.awaitingAction || !state_.activeCharacter ||
        state_.activeCharacter->index != c.character.index)
        return CommandReason::NotYourTurn;
    if (party_.character(c.character)->life != rules::Life::Conscious)
        return CommandReason::CannotAct;
    if (!(pose == origin_))
        return CommandReason::Moving;
    if (c.action == PartyAction::Defend)
        return CommandReason::Ready;
    if (c.action == PartyAction::Aid) {
        const auto* ally = c.ally ? party_.character(*c.ally) : nullptr;
        return ally && c.ally->index != c.character.index && ally->life == rules::Life::Dying
                   ? CommandReason::Ready
                   : CommandReason::InvalidAlly;
    }
    if (c.action != PartyAction::Melee)
        return CommandReason::Unavailable;
    if (!exposed(c.character))
        return CommandReason::RearBlocked;
    if (!weapon(c.character))
        return CommandReason::NoWeapon;
    if (!c.target)
        return CommandReason::MissingTarget;
    const auto* a = actors_.actor(*c.target);
    if (!a || a->life != ActorLife::Alive || !member(*c.target) || !(a->npc.cell == cell_))
        return CommandReason::InvalidTarget;
    return exposed(*c.target) ? CommandReason::Ready : CommandReason::FrontProtected;
}
CombatOutcome CombatController::submit(const PartyCommand& c, Player pose, std::uint64_t now) noexcept {
    CombatOutcome out;
    out.reason = validate(c, pose);
    if (out.reason != CommandReason::Ready)
        return out;
    out.reason = CommandReason::Resolved;
    out.character = c.character;
    if (c.action == PartyAction::Defend) {
        out.event = CombatEvent::Defend;
        state_.defending[c.character.index] = true;
    } else if (c.action == PartyAction::Aid) {
        out.event = CombatEvent::Aid;
        out.recipient = *c.ally;
        out.roll = rng_.d20().value.selected;
        out.hit = out.roll + party_.character(c.character)
                                 ->stats.modifiers[static_cast<std::size_t>(rules::Ability::Wisdom)] >=
                  10;
        if (out.hit)
            party_.stabilize(out.recipient);
    } else {
        out.event = CombatEvent::Attack;
        out.enemy = *c.target;
        const auto& w = *weapon(c.character);
        const auto roll = rng_.d20().value;
        out.roll = roll.selected;
        const auto attack =
            rules::resolveAttack(roll, w.attackBonus,
                                 actors_.catalog().monster(actors_.actor(out.enemy)->monster)->armorClass)
                .value;
        out.hit = attack.outcome != rules::AttackOutcome::Miss;
        out.critical = attack.outcome == rules::AttackOutcome::Critical;
        if (out.hit) {
            out.damage = rng_.damage(w.damage, out.critical).value.total;
            out.died = actors_.damage(out.enemy, out.damage, now).died;
        }
    }
    if (out.event == CombatEvent::Attack)
        holdAttack(out, now);
    else
        advance(now);
    out.phase = state_.phase;
    return out;
}
CombatOutcome CombatController::update(Player pose, bool moving, std::uint64_t now) noexcept {
    CombatOutcome out;
    if (state_.phase == CombatPhase::Contact)
        for (std::size_t i = 0; i < state_.enemyCount; ++i) {
            const auto* actor = actors_.actor(state_.enemies[i]);
            moving |= actor && actorWalking(*actor, now);
        }
    if (state_.phase == CombatPhase::Contact && !moving) {
        begin(now);
        out.event = CombatEvent::Started;
    } else if (state_.phase == CombatPhase::Turns) {
        if (state_.resolvingAction) {
            if (now >= due_) {
                state_.resolvingAction = false;
                advance(now);
                if (state_.phase == CombatPhase::Victory || state_.phase == CombatPhase::Defeat)
                    out.event = CombatEvent::Finished;
            }
            out.phase = state_.phase;
            return out; // Never execute the next automatic action in this update.
        }
        if (finish())
            out.event = CombatEvent::Finished;
        else {
            if (state_.activeCharacter &&
                party_.character(*state_.activeCharacter)->life != rules::Life::Conscious)
                state_.awaitingAction = false;
            if (state_.awaitingAction || now < due_ || !(pose == origin_)) {
                out.phase = state_.phase;
                return out;
            }
            if (state_.activeCharacter) {
                out.character = *state_.activeCharacter;
                // A diagnostic or later healing consumer may have changed health since turn selection.
                const auto life = party_.character(out.character)->life;
                if (life == rules::Life::Dying) {
                    out.event = CombatEvent::DeathSave;
                    out.roll = rng_.d20().value.selected;
                    out.died = party_.deathSave(out.character, out.roll).died;
                }
                if (party_.character(out.character)->life == rules::Life::Conscious)
                    state_.awaitingAction = true;
                else
                    advance(now);
            } else if (state_.activeEnemy) {
                out.enemy = *state_.activeEnemy;
                const auto* a = actors_.actor(out.enemy);
                const auto slot = meleeSlot(out.enemy, occupiedSlots());
                if (slot &&
                    (a->npc.subcell == *slot || actors_.move(out.enemy, a->npc.cell, *slot, origin_.cell))) {
                    actors_.attack(out.enemy, now);
                    out.event = CombatEvent::Attack;
                    out.enemyAttack = true;
                    std::array<rules::CharacterId, 2> targets{};
                    std::size_t count = 0;
                    for (const auto id : party_.snapshot().formation)
                        if (party_.character(id)->life == rules::Life::Conscious && exposed(id))
                            targets[count++] = id;
                    // At most two targets; a d4 parity gives an unbiased choice.
                    const auto index = count == 2 ? (rng_.roll({1, 4, 0}).value.total - 1) % 2 : 0;
                    out.recipient = targets[static_cast<std::size_t>(index)];
                    const auto& attack = *actors_.catalog().attack(melee_[out.enemy.slot]);
                    const auto roll =
                        rng_.d20(state_.defending[out.recipient.index] ? rules::RollMode::Disadvantage
                                                                       : rules::RollMode::Normal)
                            .value;
                    out.roll = roll.selected;
                    const auto hit = rules::resolveAttack(roll, attack.bonus,
                                                          party_.character(out.recipient)->stats.armorClass)
                                         .value;
                    out.hit = hit.outcome != rules::AttackOutcome::Miss;
                    out.critical = hit.outcome == rules::AttackOutcome::Critical;
                    if (out.hit) {
                        out.damage = rng_.damage(attack.damage, out.critical).value.total;
                        out.died = party_.damage(out.recipient, out.damage, out.critical).died;
                        if (party_.character(out.recipient)->life != rules::Life::Conscious)
                            state_.defending[out.recipient.index] = false;
                    }
                }
                if (out.event == CombatEvent::Attack)
                    holdAttack(out, now);
                else
                    advance(now);
            }
        }
    }
    out.phase = state_.phase;
    return out;
}
bool CombatController::continueExploring() noexcept {
    if (state_.phase != CombatPhase::Victory)
        return false;
    state_.phase = CombatPhase::Exploration;
    state_.enemyCount = 0;
    party_.lockFormation(false);
    return true;
}
} // namespace blobber
