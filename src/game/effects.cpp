#include "game/effects.hpp"
#include "game/combat.hpp"
#include <algorithm>
#include <limits>

namespace blobber {
namespace {
bool concentrates(EffectKind kind) {
    return kind != EffectKind::MageArmor;
}
std::uint64_t duration(EffectKind kind) {
    return kind == EffectKind::MageArmor ? 28800000 : kind == EffectKind::ShieldOfFaith ? 600000 : 60000;
}
bool targetLess(EffectTarget a, EffectTarget b) {
    if (a.npc != b.npc)
        return a.npc < b.npc;
    return a.npc ? a.enemy.slot < b.enemy.slot : a.character.index < b.character.index;
}
} // namespace
bool operator==(EffectTarget a, EffectTarget b) noexcept {
    return a.npc == b.npc && (a.npc ? a.enemy.slot == b.enemy.slot && a.enemy.world == b.enemy.world
                                    : a.character.index == b.character.index);
}
EffectController::EffectController(Party& party, ActorWorld& actors, rules::DiceRng& rng) noexcept
    : party_(party), actors_(actors), rng_(rng) {}
const EffectInstance* EffectController::find(EffectId id) const noexcept {
    if (!id.serial || id.world != actors_.snapshot().world)
        return nullptr;
    for (const auto& e : instances_)
        if (e.active && e.id.serial == id.serial)
            return &e;
    return nullptr;
}
EffectId EffectController::concentration(rules::CharacterId caster) const noexcept {
    for (const auto& e : instances_)
        if (e.active && concentrates(e.kind) && e.caster.index == caster.index)
            return e.id;
    return {};
}
bool EffectController::living(EffectTarget t) const noexcept {
    if (t.npc) {
        const auto* a = actors_.actor(t.enemy);
        return a && a->life == ActorLife::Alive;
    }
    const auto* c = party_.character(t.character);
    return c && c->life != rules::Life::Dead;
}
bool EffectController::unarmored(rules::CharacterId id) const noexcept {
    const auto* c = party_.character(id);
    return c && c->build.equipment.body.index >= c->build.itemCount;
}
bool EffectController::has(EffectTarget target, EffectKind kind) const noexcept {
    if (!living(target))
        return false;
    for (const auto& e : instances_)
        if (e.active && e.kind == kind)
            for (std::size_t i = 0; i < e.count; ++i)
                if (e.targets[i] == target)
                    return true;
    return false;
}
bool EffectController::canAct(EffectTarget t) const noexcept {
    if (!living(t) || has(t, EffectKind::HoldPerson))
        return false;
    return t.npc || party_.character(t.character)->life == rules::Life::Conscious;
}
bool EffectController::mobileParty() const noexcept {
    for (std::uint16_t i = 0; i < rules::partySize; ++i)
        if (canAct(rules::CharacterId{i}))
            return true;
    return false;
}
std::array<bool, NpcWorld::maxInstances> EffectController::incapacitatedActors() const noexcept {
    std::array<bool, NpcWorld::maxInstances> result{};
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i)
        result[i] = !canAct(actors_.id(i));
    return result;
}
int EffectController::armorClass(EffectTarget t) const noexcept {
    if (!living(t))
        return 0;
    int ac;
    if (t.npc)
        ac = actors_.catalog().monster(actors_.actor(t.enemy)->monster)->armorClass;
    else {
        const auto& c = *party_.character(t.character);
        ac = c.stats.armorClass;
        if (unarmored(t.character) && has(t, EffectKind::MageArmor))
            ac += 3; // Current unarmored base is 10 + Dex; preserve the shield.
    }
    return ac + (has(t, EffectKind::ShieldOfFaith) ? 2 : 0);
}
int EffectController::rollModifier(EffectTarget t) noexcept {
    int bonus = 0;
    if (has(t, EffectKind::Bless))
        bonus += static_cast<int>(rng_.roll({1, 4, 0}).value.total);
    if (has(t, EffectKind::Bane))
        bonus -= static_cast<int>(rng_.roll({1, 4, 0}).value.total);
    return bonus;
}
rules::SaveResult EffectController::save(EffectTarget target, rules::Ability ability,
                                         std::int64_t dc) noexcept {
    rules::SaveResult result;
    const auto index = static_cast<std::size_t>(ability);
    if (!living(target) || index >= rules::abilityCount)
        return result;
    if ((ability == rules::Ability::Strength || ability == rules::Ability::Dexterity) &&
        has(target, EffectKind::HoldPerson))
        return result; // Automatic failure consumes no dice.
    const int bonus =
        target.npc ? rules::abilityModifier(
                         actors_.catalog().monster(actors_.actor(target.enemy)->monster)->abilities, ability)
                         .value
                   : party_.character(target.character)->stats.saves[index];
    result.roll = rng_.d20().value;
    result.total = std::int64_t(result.roll.selected) + bonus + rollModifier(target);
    result.succeeded = result.total >= dc;
    return result;
}
void EffectController::emit(const EffectInstance& e, EffectNotice notice, EffectEnd reason,
                            EffectTarget target, const rules::SaveResult& save, std::int64_t dc,
                            std::uint64_t session, CombatEvents& events) const noexcept {
    CombatOutcome out;
    out.event = CombatEvent::Effect;
    out.occurredMs = session;
    out.effect = {notice,     reason, e.id,          e.kind,
                  e.caster,   target, gameplayMs_,   save.roll.selected,
                  save.total, dc,     save.succeeded};
    events.append(out);
}
void EffectController::end(EffectInstance& e, EffectEnd why, std::uint64_t now,
                           CombatEvents& events) noexcept {
    emit(e, EffectNotice::Ended, why, {}, {}, 0, now, events);
    e.active = false;
}
EffectResult EffectController::remove(EffectId id, std::uint64_t now, CombatEvents& events) noexcept {
    if (!find(id))
        return {EffectError::InvalidHandle};
    for (auto& e : instances_)
        if (e.active && e.id.serial == id.serial) {
            end(e, EffectEnd::Dismissed, now, events);
            break;
        }
    return {EffectError::None, id};
}
EffectResult EffectController::apply(const EffectRequest& request, std::uint64_t now,
                                     CombatEvents& events) noexcept {
    if (request.world != actors_.snapshot().world)
        return {EffectError::InvalidWorld};
    if (!canAct(request.caster))
        return {EffectError::InvalidCaster};
    if (request.kind >= EffectKind::Count)
        return {EffectError::InvalidKind};
    if (request.difficulty < 1 || request.difficulty > 1000)
        return {EffectError::InvalidDifficulty};
    const auto maxTargets = request.kind == EffectKind::Bless || request.kind == EffectKind::Bane ? 3u : 1u;
    if (!request.count || request.count > maxTargets)
        return {EffectError::InvalidTargets};
    for (std::size_t i = 0; i < request.count; ++i) {
        const auto target = request.targets[i];
        if (!living(target) ||
            (request.kind == EffectKind::MageArmor && (target.npc || !unarmored(target.character))))
            return {EffectError::InvalidTargets};
        for (std::size_t j = 0; j < i; ++j)
            if (request.targets[j] == target)
                return {EffectError::InvalidTargets};
    }
    const auto old = concentrates(request.kind) ? concentration(request.caster) : EffectId{};
    std::size_t slot = capacity;
    for (std::size_t i = 0; i < capacity; ++i)
        if (!instances_[i].active || (old.serial && instances_[i].id.serial == old.serial)) {
            slot = i;
            break;
        }
    if (slot == capacity || nextSerial_ == std::numeric_limits<std::uint64_t>::max() ||
        gameplayMs_ > std::numeric_limits<std::uint64_t>::max() - duration(request.kind))
        return {EffectError::Capacity};
    if (old.serial)
        for (auto& e : instances_)
            if (e.active && e.id.serial == old.serial)
                end(e, EffectEnd::Replaced, now, events);
    EffectInstance candidate;
    candidate.id = {request.world, nextSerial_++};
    candidate.kind = request.kind;
    candidate.caster = request.caster;
    candidate.difficulty = request.difficulty;
    candidate.expiresMs = gameplayMs_ + duration(request.kind);
    candidate.retryMs = gameplayMs_ + retryIntervalMs;
    auto targets = request.targets;
    std::sort(targets.begin(), targets.begin() + request.count, targetLess);
    for (std::size_t i = 0; i < request.count; ++i) {
        bool accepted = true;
        if (request.kind == EffectKind::Bane || request.kind == EffectKind::HoldPerson) {
            const auto saved =
                save(targets[i],
                     request.kind == EffectKind::Bane ? rules::Ability::Charisma : rules::Ability::Wisdom,
                     request.difficulty);
            emit(candidate, EffectNotice::InitialSave, EffectEnd::None, targets[i], saved, request.difficulty,
                 now, events);
            accepted = !saved.succeeded;
        }
        if (accepted)
            candidate.targets[candidate.count++] = targets[i];
    }
    if (!candidate.count)
        return {};
    candidate.active = true;
    instances_[slot] = candidate;
    for (std::size_t i = 0; i < candidate.count; ++i)
        emit(candidate, EffectNotice::Applied, EffectEnd::None, candidate.targets[i], {}, 0, now, events);
    reconcile(now, events);
    return {EffectError::None, candidate.id};
}
void EffectController::reconcile(std::uint64_t now, CombatEvents& events) noexcept {
    // Serial order is independent of slot reuse.
    std::uint64_t previous = 0;
    for (;;) {
        EffectInstance* next = nullptr;
        for (auto& e : instances_)
            if (e.active && e.id.serial > previous && (!next || e.id.serial < next->id.serial))
                next = &e;
        if (!next)
            break;
        auto& e = *next;
        previous = e.id.serial;
        if (concentrates(e.kind) && !canAct(e.caster)) {
            end(e, EffectEnd::Incapacitated, now, events);
            continue;
        }
        for (std::size_t i = 0; i < e.count;) {
            const bool armor = e.kind == EffectKind::MageArmor && !unarmored(e.targets[i].character);
            if (living(e.targets[i]) && !armor) {
                ++i;
                continue;
            }
            emit(e, EffectNotice::RecipientRemoved, armor ? EffectEnd::Armor : EffectEnd::InvalidTarget,
                 e.targets[i], {}, 0, now, events);
            for (std::size_t j = i + 1; j < e.count; ++j)
                e.targets[j - 1] = e.targets[j];
            --e.count;
        }
        if (!e.count)
            end(e, EffectEnd::NoTargets, now, events);
    }
}
void EffectController::advance(std::uint64_t goal, std::uint64_t session, bool realTime,
                               CombatEvents& events) noexcept {
    assert(goal >= gameplayMs_);
    for (;;) {
        EffectInstance* next = nullptr;
        std::uint64_t deadline = std::numeric_limits<std::uint64_t>::max();
        bool expiration = false;
        for (auto& e : instances_) {
            if (!e.active)
                continue;
            const auto due =
                e.kind == EffectKind::HoldPerson ? std::min(e.expiresMs, e.retryMs) : e.expiresMs;
            const bool expires = due == e.expiresMs;
            if (due <= goal &&
                (!next || due < deadline ||
                 (due == deadline &&
                  (expires > expiration || (expires == expiration && e.id.serial < next->id.serial))))) {
                next = &e;
                deadline = due;
                expiration = expires;
            }
        }
        if (!next)
            break;
        gameplayMs_ = deadline;
        const auto when = realTime ? session - (goal - deadline) : session;
        auto& e = *next;
        if (expiration) {
            end(e, EffectEnd::Expired, when, events);
            continue;
        }
        // Hold Person has exactly one recipient in this slice.
        const auto saved = save(e.targets[0], rules::Ability::Wisdom, e.difficulty);
        emit(e, EffectNotice::RepeatSave, EffectEnd::None, e.targets[0], saved, e.difficulty, when, events);
        if (saved.succeeded)
            end(e, EffectEnd::Saved, when, events);
        else
            e.retryMs += retryIntervalMs;
    }
    gameplayMs_ = goal;
}
void EffectController::damaged(rules::CharacterId id, std::int64_t damage, std::uint64_t now,
                               CombatEvents& events) noexcept {
    reconcile(now, events);
    if (damage <= 0)
        return;
    const auto held = concentration(id);
    for (auto& e : instances_)
        if (e.active && held.serial && e.id.serial == held.serial) {
            const auto dc = std::max(std::int64_t{10}, damage / 2);
            const auto saved = save(id, rules::Ability::Constitution, dc);
            emit(e, EffectNotice::ConcentrationSave, EffectEnd::None, id, saved, dc, now, events);
            if (!saved.succeeded)
                end(e, EffectEnd::FailedSave, now, events);
            break;
        }
}
} // namespace blobber
