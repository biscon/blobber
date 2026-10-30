#include "game/combat.hpp"
#include "game/perception.hpp"
#include <algorithm>
namespace blobber {
void EffectController::reconcileAid() noexcept {
    for (std::uint16_t i = 0; i < 4; ++i)
        party_.aidBonus({i}, has(rules::CharacterId{i}, EffectKind::Aid) ? 5 : 0);
}
bool EffectController::restoreCondition(rules::CharacterId id, EffectKind kind, std::uint64_t now,
                                        CombatEvents& events) noexcept {
    if (!living(id) || !has(id, kind))
        return false;
    if (kind == EffectKind::Blind) {
        auto& c = conditions_[id.index][1];
        c.active = false;
        conditionNotice(id, kind, EffectNotice::Ended, EffectEnd::Recovered, c.caster, now, events);
        return true;
    }
    if (kind == EffectKind::HoldPerson) {
        for (auto& e : instances_)
            if (e.active && e.kind == kind && e.targets[0] == EffectTarget(id))
                end(e, EffectEnd::Recovered, now, events);
        return true;
    }
    return false;
}
MirrorReservation EffectController::mirror(EffectTarget attacker, rules::CharacterId target, int roll,
                                           int bonus) noexcept {
    if (!sees(attacker))
        return {};
    for (auto& e : instances_) {
        if (!e.active || e.kind != EffectKind::MirrorImage || !(e.targets[0] == EffectTarget(target)))
            continue;
        const unsigned available = e.duplicates & ~e.reserved;
        const int count = int(bool(available & 1)) + int(bool(available & 2)) + int(bool(available & 4));
        if (!count || rng_.d20().value.selected < (count == 3 ? 6 : count == 2 ? 8 : 11))
            return {};
        unsigned bit = 1;
        while (!(available & bit))
            bit <<= 1;
        e.reserved |= bit;
        const bool hit =
            roll == 20 || (roll != 1 && roll + bonus >= 10 + party_.character(target)->stats.modifiers[1]);
        return {e.id, std::uint8_t(bit), hit};
    }
    return {};
}
void EffectController::resolveMirror(MirrorReservation r, bool canceled, std::uint64_t now,
                                     CombatEvents& events) noexcept {
    if (!find(r.effect))
        return;
    for (auto& e : instances_)
        if (e.active && e.id.serial == r.effect.serial) {
            e.reserved &= ~r.duplicate;
            if (!canceled && r.hit)
                e.duplicates &= ~r.duplicate;
            if (!e.duplicates)
                end(e, EffectEnd::Consumed, now, events);
            return;
        }
}
const EffectInstance* EffectController::spiritualWeapon(rules::CharacterId id) const noexcept {
    for (const auto& e : instances_)
        if (e.active && e.kind == EffectKind::SpiritualWeapon && e.caster.index == id.index)
            return &e;
    return nullptr;
}
bool EffectController::weaponCanMove(rules::CharacterId id, Cell to) const noexcept {
    const auto* e = spiritualWeapon(id);
    if (!e || e->readyMs > gameplayMs_ || actors_.map().solid(to))
        return false;
    // At most two cardinal floor steps, including turning around a corner.
    if (to == e->cell)
        return true;
    for (int d = 0; d < 4; ++d) {
        const auto first = e->cell + forward(Heading(d));
        if (actors_.map().solid(first))
            continue;
        if (first == to)
            return true;
        for (int q = 0; q < 4; ++q)
            if (first + forward(Heading(q)) == to)
                return true;
    }
    return false;
}
bool EffectController::moveWeapon(rules::CharacterId id, Cell to, bool initial) noexcept {
    if ((!initial && !weaponCanMove(id, to)) || actors_.map().solid(to))
        return false;
    for (auto& e : instances_)
        if (e.active && e.kind == EffectKind::SpiritualWeapon && e.caster.index == id.index) {
            e.cell = to;
            e.readyMs = gameplayMs_ + 6000;
            ground_.weapons[id.index] = {to, true};
            ++ground_.revision;
            return true;
        }
    return false;
}
bool EffectController::canCreateWeb(const Cell* cells, std::size_t count,
                                    rules::CharacterId caster) const noexcept {
    if (!count || count > 4 || nextArea_ > UINT64_MAX - count || gameplayMs_ > UINT64_MAX - 3600000)
        return false;
    EffectRequest r;
    r.world = actors_.snapshot().world;
    r.caster = caster;
    r.kind = EffectKind::Web;
    r.targets[0] = caster;
    r.count = 1;
    if (validate(r) != EffectError::None)
        return false;
    for (std::size_t i = 0; i < count; ++i) {
        if (actors_.map().solid(cells[i]))
            return false;
        for (std::size_t j = 0; j < i; ++j)
            if (cells[i] == cells[j])
                return false;
    }
    const auto old = concentration(caster);
    std::size_t free = 0;
    for (const auto& a : ground_.areas)
        free += !a.active || (a.web && a.effect.serial == old.serial);
    return free >= count;
}
bool EffectController::createWeb(const Cell* cells, std::size_t count, rules::CharacterId caster, int dc,
                                 std::uint64_t now, CombatEvents& events) noexcept {
    if (!canCreateWeb(cells, count, caster) || dc < 1 || dc > 1000)
        return false;
    EffectRequest r;
    r.world = actors_.snapshot().world;
    r.caster = caster;
    r.kind = EffectKind::Web;
    r.targets[0] = caster;
    r.count = 1;
    r.difficulty = dc;
    const auto result = apply(r, now, events);
    if (!result)
        return false;
    std::size_t cell = 0;
    for (auto& a : ground_.areas)
        if (!a.active && cell < count) {
            a = {{actors_.snapshot().world, nextArea_++},
                 cells[cell++],
                 caster,
                 dc,
                 gameplayMs_ + 3600000,
                 true,
                 true,
                 result.id,
                 0};
            ++ground_.revision;
        }
    if (groundDueMs_ <= gameplayMs_)
        groundDueMs_ = gameplayMs_ + 6000;
    groundOccupants(partyCell_, now, events);
    return true;
}
void EffectController::reconcileWeb(std::uint64_t now, CombatEvents& events) noexcept {
    for (std::size_t i = 0; i < restrained_.size(); ++i) {
        auto& r = restrained_[i];
        if (!r.area.serial)
            continue;
        const auto t =
            i < 4 ? EffectTarget(rules::CharacterId{std::uint16_t(i)}) : EffectTarget(actors_.id(i - 4));
        bool present = false;
        const auto* a = t.npc ? actors_.actor(t.enemy) : nullptr;
        const auto cell = t.npc && a ? a->npc.cell : partyCell_;
        for (const auto& g : ground_.areas)
            present |= g.active && g.web && g.id.serial == r.area.serial && g.cell == cell;
        if (!living(t) || !present) {
            conditionNotice(t, EffectKind::Restrained, EffectNotice::Ended, EffectEnd::Recovered, r.caster,
                            now, events);
            r = {};
        }
    }
}
bool EffectController::escapeWeb(EffectTarget t, std::uint64_t now, CombatEvents& events) noexcept {
    if (!canAct(t) || !has(t, EffectKind::Restrained))
        return false;
    auto& r = restrained_[conditionIndex(t)];
    const int modifier =
        t.npc ? rules::abilityModifier(actors_.catalog().monster(actors_.actor(t.enemy)->monster)->abilities,
                                       rules::Ability::Strength)
                    .value
              : party_.character(t.character)->stats.modifiers[0];
    const auto roll = rng_.d20().value.selected;
    const int total = roll + modifier + oneShot(t, EffectKind::Guidance);
    CombatOutcome out;
    out.event = CombatEvent::SpellSave;
    out.feature = rules::Feature::EscapeWeb;
    out.spell = rules::SpellKind::Web;
    out.character = r.caster;
    out.recipient = t.character;
    out.enemy = t.enemy;
    out.enemyAttack = !t.npc;
    out.roll = roll;
    out.saveTotal = total;
    out.saveDifficulty = r.difficulty;
    out.hit = total >= r.difficulty;
    out.occurredMs = now;
    events.append(out);
    if (out.hit) {
        conditionNotice(t, EffectKind::Restrained, EffectNotice::Ended, EffectEnd::Recovered, r.caster, now,
                        events);
        r = {};
    }
    return true;
}
void EffectController::ignite(Cell cell) noexcept {
    for (auto& a : ground_.areas)
        if (a.active && a.web && a.cell == cell && !a.burnMs) {
            a.burnMs = gameplayMs_ + 6000;
            ++ground_.revision;
        }
}
void EffectController::burnWeb(std::uint64_t now, CombatEvents& events) noexcept {
    for (auto& a : ground_.areas) {
        if (!a.active || !a.burnMs || a.burnMs > gameplayMs_)
            continue;
        const auto cell = a.cell;
        const auto caster = a.caster;
        // Co-located burning webs burn once, regardless of overlapping casts.
        for (auto& g : ground_.areas)
            if (g.active && g.web && g.cell == cell && g.burnMs && g.burnMs <= gameplayMs_) {
                g.active = false;
                ++ground_.revision;
            }
        const auto damage = rng_.roll({2, 4, 0}).value.total;
        for (std::size_t i = 0; i < creatureCount; ++i) {
            const auto t =
                i < 4 ? EffectTarget(rules::CharacterId{std::uint16_t(i)}) : EffectTarget(actors_.id(i - 4));
            if (!living(t))
                continue;
            if (!((t.npc ? actors_.actor(t.enemy)->npc.cell : partyCell_) == cell))
                continue;
            CombatOutcome out;
            out.event = CombatEvent::Attack;
            out.spell = rules::SpellKind::Web;
            out.character = caster;
            out.enemy = t.enemy;
            out.recipient = t.character;
            out.enemyAttack = !t.npc;
            out.hit = true;
            out.damage = damage;
            out.occurredMs = now;
            if (t.npc)
                out.died = actors_.damage(t.enemy, damage, now).died;
            else
                out.died = party_.damage(t.character, damage).died;
            events.append(out);
            wake(t, now, events);
            if (!t.npc)
                damaged(t.character, damage, now, events);
        }
    }
    reconcileWeb(now, events);
    for (auto& e : instances_)
        if (e.active && e.kind == EffectKind::Web) {
            bool remains = false;
            for (const auto& a : ground_.areas)
                remains |= a.active && a.effect.serial == e.id.serial;
            if (!remains)
                end(e, EffectEnd::Consumed, now, events);
        }
}
} // namespace blobber
