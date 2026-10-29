#include "game/combat.hpp"
#include "game/effects.hpp"
#include <algorithm>
#include <limits>

namespace blobber {
const char* effectName(EffectKind kind) noexcept {
    constexpr const char* names[]{"Bless",    "Bane",       "Shield of Faith", "Mage Armor", "Hold Person",
                                  "Guidance", "Resistance", "Sanctuary",       "Sleep",      "Blind",
                                  "Slow",     "Knockdown"};
    return unsigned(kind) < std::size(names) ? names[unsigned(kind)] : "Effect";
}
std::size_t EffectController::conditionIndex(EffectTarget t) const noexcept {
    if (t.npc)
        return actors_.actor(t.enemy) ? t.enemy.slot + rules::partySize : creatureCount;
    return party_.character(t.character) ? t.character.index : creatureCount;
}
bool EffectController::conscious(EffectTarget t) const noexcept {
    return living(t) && (t.npc || party_.character(t.character)->life == rules::Life::Conscious) &&
           !has(t, EffectKind::HoldPerson) && !has(t, EffectKind::Sleep);
}
bool EffectController::sees(EffectTarget t) const noexcept {
    return conscious(t) && !has(t, EffectKind::Blind);
}
bool EffectController::helpless(EffectTarget t) const noexcept {
    return has(t, EffectKind::HoldPerson) || has(t, EffectKind::Sleep);
}
bool EffectController::sleepImmune(EffectTarget t) const noexcept {
    if (!living(t))
        return true;
    return t.npc ? actors_.catalog().monster(actors_.actor(t.enemy)->monster)->sleepImmune
                 : party_.catalog().race(party_.character(t.character)->build.race)->sleepImmune;
}
bool EffectController::metalArmor(EffectTarget t) const noexcept {
    if (!living(t))
        return false;
    if (t.npc)
        return actors_.catalog().monster(actors_.actor(t.enemy)->monster)->metalArmor;
    const auto* armor = party_.equipped(t.character, EquipmentSlot::Body);
    return armor && armor->metalArmor;
}
rules::RollMode EffectController::attackMode(EffectTarget attacker, EffectTarget target, bool close,
                                             bool advantage, bool disadvantage) const noexcept {
    advantage |=
        helpless(target) || has(target, EffectKind::Blind) || (close && has(target, EffectKind::Knockdown));
    disadvantage |= has(attacker, EffectKind::Blind) || has(attacker, EffectKind::Knockdown) ||
                    (!close && has(target, EffectKind::Knockdown));
    return rules::rollMode(advantage, disadvantage);
}
int EffectController::oneShot(EffectTarget target, EffectKind kind) noexcept {
    EffectInstance* first = nullptr;
    for (auto& e : instances_) {
        if (!e.active || e.kind != kind)
            continue;
        for (std::size_t i = 0; i < e.count; ++i)
            if (e.targets[i] == target && (!first || e.id.serial < first->id.serial))
                first = &e;
    }
    if (!first)
        return 0;
    const int bonus = int(rng_.roll({1, 4, 0}).value.total);
    // Same-kind benefits never stack, and all copies expire on that eligible roll.
    for (auto& e : instances_) {
        if (!e.active || e.kind != kind)
            continue;
        for (std::size_t i = 0; i < e.count; ++i)
            if (e.targets[i] == target) {
                if (events_)
                    end(e, EffectEnd::Consumed, noticeMs_, *events_);
                else
                    e.active = false;
                break;
            }
    }
    return bonus;
}
void EffectController::breakWard(EffectTarget target, std::uint64_t now, CombatEvents& events) noexcept {
    for (auto& e : instances_)
        if (e.active && e.kind == EffectKind::Sanctuary && e.targets[0] == target)
            end(e, EffectEnd::Consumed, now, events);
}
bool EffectController::wardAllows(EffectTarget attacker, EffectTarget target, std::uint64_t now,
                                  CombatEvents& events) noexcept {
    const EffectInstance* ward = nullptr;
    for (const auto& e : instances_)
        if (e.active && e.kind == EffectKind::Sanctuary && e.targets[0] == target &&
            (!ward || e.difficulty > ward->difficulty ||
             (e.difficulty == ward->difficulty && e.id.serial < ward->id.serial)))
            ward = &e;
    if (!ward)
        return true;
    const auto saved = save(attacker, rules::Ability::Wisdom, ward->difficulty);
    emit(*ward, EffectNotice::InitialSave, EffectEnd::None, attacker, saved, ward->difficulty, now, events);
    return saved.succeeded;
}
void EffectController::conditionNotice(EffectTarget target, EffectKind kind, EffectNotice notice,
                                       EffectEnd reason, rules::CharacterId caster, std::uint64_t now,
                                       CombatEvents& events) const noexcept {
    EffectInstance e;
    e.kind = kind;
    e.caster = caster;
    emit(e, notice, reason, target, {}, 0, now, events);
}
void EffectController::condition(EffectTarget t, EffectKind kind, rules::CharacterId caster,
                                 std::uint64_t now, CombatEvents& events) noexcept {
    const auto i = conditionIndex(t);
    if (i >= conditions_.size() || !living(t) || !party_.character(caster) || kind < EffectKind::Sleep ||
        kind >= EffectKind::Count || (kind == EffectKind::Sleep && sleepImmune(t)))
        return;
    auto& c = conditions_[i][unsigned(kind) - unsigned(EffectKind::Sleep)];
    if (kind == EffectKind::Knockdown && c.active)
        return;
    const auto duration = kind == EffectKind::Sleep ? 60000u : 6000u;
    if (gameplayMs_ > UINT64_MAX - duration)
        return;
    c = {caster,   gameplayMs_ + duration,
         duration, casterTurns_[caster.index] + 1,
         true,     turns_ && (kind == EffectKind::Blind || kind == EffectKind::Slow)};
    conditionNotice(t, kind, EffectNotice::Applied, EffectEnd::None, caster, now, events);
    reconcile(now, events);
}
void EffectController::wake(EffectTarget t, std::uint64_t now, CombatEvents& events) noexcept {
    const auto i = conditionIndex(t);
    if (i >= conditions_.size())
        return;
    auto& c = conditions_[i][0];
    if (!c.active)
        return;
    c.active = false;
    conditionNotice(t, EffectKind::Sleep, EffectNotice::Ended, EffectEnd::Woken, c.caster, now, events);
}
bool EffectController::stand(EffectTarget t, std::uint64_t now, CombatEvents& events) noexcept {
    const auto i = conditionIndex(t);
    if (i >= conditions_.size() || !conscious(t))
        return false;
    auto& c = conditions_[i][3];
    if (!c.active)
        return false;
    c.active = false;
    conditionNotice(t, EffectKind::Knockdown, EffectNotice::Ended, EffectEnd::Recovered, c.caster, now,
                    events);
    return true;
}
void EffectController::clockMode(bool turns) noexcept {
    if (turns_ == turns)
        return;
    for (auto& row : conditions_)
        for (unsigned k : {1u, 2u, 3u}) {
            auto& c = row[k];
            if (!c.active)
                continue;
            if (turns) {
                c.remainingMs = c.untilMs > gameplayMs_ ? c.untilMs - gameplayMs_ : 0;
                c.casterTurn = casterTurns_[c.caster.index] + 1;
            } else
                c.untilMs = gameplayMs_ + c.remainingMs;
            c.turnBound = turns;
        }
    if (!turns && groundDueMs_ < gameplayMs_)
        groundDueMs_ = gameplayMs_ + 6000;
    turns_ = turns;
}
void EffectController::casterTurn(rules::CharacterId caster, std::uint64_t now,
                                  CombatEvents& events) noexcept {
    if (!party_.character(caster))
        return;
    ++casterTurns_[caster.index];
    for (std::size_t i = 0; i < conditions_.size(); ++i)
        for (unsigned k : {1u, 2u}) {
            auto& c = conditions_[i][k];
            if (!c.active || !c.turnBound || c.caster.index != caster.index ||
                c.casterTurn > casterTurns_[caster.index])
                continue;
            c.active = false;
            const auto t = i < rules::partySize ? EffectTarget(rules::CharacterId{std::uint16_t(i)})
                                                : EffectTarget(actors_.id(i - rules::partySize));
            conditionNotice(t, EffectKind(unsigned(EffectKind::Sleep) + k), EffectNotice::Ended,
                            EffectEnd::Expired, c.caster, now, events);
        }
}
bool EffectController::canCreateArea() const noexcept {
    return nextArea_ < UINT64_MAX && gameplayMs_ <= UINT64_MAX - 60000 &&
           std::any_of(
               ground_.areas.begin(), ground_.areas.end(), [](const GroundArea& a) { return !a.active; });
}
AreaId EffectController::createArea(Cell cell, rules::CharacterId caster, int dc, std::uint64_t now,
                                    CombatEvents& events) noexcept {
    if (!canCreateArea() || actors_.map().solid(cell) || !canAct(caster) || dc < 1 || dc > 1000)
        return {};
    const bool empty = std::none_of(ground_.areas.begin(), ground_.areas.end(),
                                    [](const GroundArea& a) { return a.active; });
    for (auto& a : ground_.areas)
        if (!a.active) {
            a = {{actors_.snapshot().world, nextArea_++}, cell, caster, dc, gameplayMs_ + 60000, true};
            ++ground_.revision;
            if (empty)
                groundDueMs_ = gameplayMs_ + 6000;
            (void)now;
            (void)events;
            return a.id;
        }
    return {};
}
const GroundArea* EffectController::area(Cell cell) const noexcept {
    const GroundArea* found = nullptr;
    for (const auto& a : ground_.areas)
        if (a.active && a.cell == cell &&
            (!found || a.difficulty > found->difficulty ||
             (a.difficulty == found->difficulty && a.id.serial < found->id.serial)))
            found = &a;
    return found;
}
unsigned EffectController::movementFactor(EffectTarget t, Cell from, Cell to) const noexcept {
    return has(t, EffectKind::Slow) || area(from) || area(to) ? 2 : 1;
}
void EffectController::enter(EffectTarget t, Cell cell, std::uint64_t now) noexcept {
    if (events_)
        groundCheck(t, cell, now, *events_);
}
void EffectController::groundCheck(EffectTarget target, Cell cell, std::uint64_t now,
                                   CombatEvents& events) noexcept {
    const auto* a = area(cell);
    if (!a || !living(target) ||
        (!target.npc && party_.character(target.character)->life != rules::Life::Conscious))
        return;
    const auto result = save(target, rules::Ability::Dexterity, a->difficulty);
    CombatOutcome out;
    out.event = CombatEvent::SpellSave;
    out.spell = rules::SpellKind::Grease;
    out.enemyAttack = !target.npc;
    out.enemy = target.enemy;
    out.recipient = target.character;
    out.character = a->caster;
    out.roll = result.roll.selected;
    out.saveTotal = int(result.total);
    out.saveDifficulty = a->difficulty;
    out.hit = result.succeeded;
    out.occurredMs = now;
    events.append(out);
    if (!result.succeeded)
        condition(target, EffectKind::Knockdown, a->caster, now, events);
}
void EffectController::groundOccupants(Cell partyCell, std::uint64_t now, CombatEvents& events) noexcept {
    for (std::uint16_t i = 0; i < rules::partySize; ++i)
        groundCheck(rules::CharacterId{i}, partyCell, now, events);
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto* a = actors_.actor(actors_.id(i));
        if (a && a->life == ActorLife::Alive)
            groundCheck(actors_.id(i), a->npc.cell, now, events);
    }
}
std::uint64_t EffectController::conditionDeadline(bool realTime) const noexcept {
    auto due = UINT64_MAX;
    for (const auto& row : conditions_)
        for (unsigned k = 0; k < row.size(); ++k) {
            const auto& c = row[k];
            if (c.active && !c.turnBound && (k != 3 || realTime))
                due = std::min(due, c.untilMs);
        }
    bool ground = false;
    for (const auto& a : ground_.areas)
        if (a.active) {
            ground = true;
            due = std::min(due, a.untilMs);
        }
    if (ground && realTime)
        due = std::min(due, groundDueMs_);
    return due;
}
void EffectController::advanceConditions(std::uint64_t goal, std::uint64_t now, bool realTime,
                                         CombatEvents& events) noexcept {
    for (auto& a : ground_.areas)
        if (a.active && a.untilMs <= goal) {
            a.active = false;
            ++ground_.revision;
        }
    for (std::size_t i = 0; i < conditions_.size(); ++i) {
        const auto t = i < rules::partySize ? EffectTarget(rules::CharacterId{std::uint16_t(i)})
                                            : EffectTarget(actors_.id(i - rules::partySize));
        for (unsigned k = 0; k < 4; ++k) {
            auto& c = conditions_[i][k];
            if (!c.active)
                continue;
            if (!living(t) || (!c.turnBound && c.untilMs <= goal && (k != 3 || realTime))) {
                c.active = false;
                conditionNotice(t, EffectKind(unsigned(EffectKind::Sleep) + k), EffectNotice::Ended,
                                k == 3 ? EffectEnd::Recovered : EffectEnd::Expired, c.caster, now, events);
            }
        }
    }
    if (realTime && groundDueMs_ <= goal) {
        groundDueMs_ = goal + 6000;
        groundOccupants(partyCell_, now, events);
    }
}
} // namespace blobber
