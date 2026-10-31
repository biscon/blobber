#include "game/session.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace blobber {
void GameSession::validateCheckpoint(const FloorState& s) const {
    const auto reject = [](const char* part) {
        throw std::invalid_argument(std::string("Invalid saved floor ") + part);
    };
    const auto cell = [&](Cell p) {
        return p.x >= 0 && p.y >= 0 && p.x < map_.width() && p.y < map_.height();
    };
    const auto actor = [&](ActorId id) { return id.slot < s.actors.count; };
    const auto& c = s.combat.state;
    if (s.timeMs > maxCheckpointClock || s.effects.gameplayMs > maxCheckpointClock ||
        s.ai.state.timeMs > maxCheckpointClock)
        reject("clock capacity");
    const auto clocks = [&](const auto& values) {
        for (auto value : values)
            if (value > maxCheckpointClock)
                reject("deadline capacity");
    };
    if (s.ai.tickDue > maxCheckpointClock || s.effects.groundDueMs > maxCheckpointClock)
        reject("deadline capacity");
    clocks(s.effects.guidingUntil);
    clocks(s.combat.partyReady);
    clocks(s.combat.saveDue);
    clocks(s.combat.defendDue);
    clocks(s.combat.sneakReady);
    clocks(s.combat.cunningReady);
    clocks(s.combat.extraUntil);
    clocks(s.combat.knownUntil);
    clocks(s.combat.enemyReady);
    clocks(s.combat.enemyBusy);
    for (const auto& row : s.effects.conditions)
        for (const auto& condition : row)
            if (condition.active &&
                (condition.untilMs > maxCheckpointClock || condition.remainingMs > maxCheckpointClock ||
                 condition.casterTurn > maxCheckpointClock))
                reject("condition clock capacity");
    if (s.actors.count > NpcWorld::maxInstances || s.ai.state.count != s.actors.count ||
        s.objective.count > 32 || c.enemyCount > c.enemies.size() || c.upcomingCount > c.upcoming.size() ||
        c.pendingCount || c.phase != CombatPhase::Exploration || c.awaitingAction || c.resolvingAction ||
        c.canEndTurn || s.ai.priority >= NpcWorld::maxInstances ||
        s.ai.state.searchWork > Navigation::nodeBudget ||
        s.ai.state.completedSearches > Navigation::requestBudget ||
        s.combat.autoCursor >= NpcWorld::maxInstances || !s.combat.nextAttack || !s.restToken ||
        !s.loot.state.revision || (c.facing && unsigned(*c.facing) > 3) || (c.knownDirections & ~15u))
        reject("combat state");
    if (c.activeCharacter && !party_.character(*c.activeCharacter))
        reject("combat state");
    if (c.activeEnemy && !actor(*c.activeEnemy))
        reject("combat state");
    for (unsigned i = 0; i < c.enemyCount; ++i)
        if (!actor(c.enemies[i]))
            reject("combat state");
    for (unsigned i = 0; i < c.upcomingCount; ++i) {
        const auto& p = c.upcoming[i];
        if (bool(p.character) == bool(p.enemy) || (p.character && !party_.character(*p.character)) ||
            (p.enemy && !actor(*p.enemy)))
            reject("combat state");
    }
    for (std::size_t i = 0; i < s.actors.count; ++i) {
        const auto& a = s.actors.actors[i];
        const auto& ai = s.ai.state.actors[i];
        if (!std::isfinite(a.walkFromElevation) || a.walkFromElevation < -8 || a.walkFromElevation > 8 ||
            !std::isfinite(a.walkFrom.x) || !std::isfinite(a.walkFrom.y) || a.walkFrom.x < 0 ||
            a.walkFrom.y < 0 || a.walkFrom.x >= map_.width() || a.walkFrom.y >= map_.height() ||
            !cell(ai.lastSeen) || (a.life == ActorLife::Dead && (!a.hasDeath || a.hitPoints)) ||
            (a.hasDeath && a.life == ActorLife::Alive) ||
            (ai.config.routine != NpcRoutine::Patrol && ai.config.waypointCount))
            reject("actor state");
    }
    for (std::size_t i = s.actors.count; i < NpcWorld::maxInstances; ++i)
        if (s.creditedDeaths[i] || s.loot.deaths[i] || s.combat.known[i] || s.combat.episodeActors[i])
            reject("actor state");
    for (std::size_t i = std::size_t(map_.width() * map_.height()); i < discoveryCapacity; ++i)
        if (s.fixtures.discovered[i])
            reject("discovery");
    for (unsigned i = 0; i < floor_.definition_.count; ++i) {
        const auto& f = floor_.definition_.entries[i];
        if (f.kind == FixtureKind::Key && s.fixtures.completed[i] && !(party_.questKeys() & (1u << f.key)))
            reject("fixture/objective ledger");
        if (f.kind == FixtureKind::Door && f.key && s.fixtures.completed[i] &&
            !(party_.questKeys() & (1u << f.key)))
            reject("fixture/objective ledger");
        if (f.kind != FixtureKind::Door && f.kind != FixtureKind::Key &&
            f.kind != FixtureKind::GuardianGate && s.fixtures.completed[i])
            reject("fixture/objective ledger");
    }
    if (s.objective.count && s.fixtures.completed[objectiveDefinition_.gate] != s.objective.completed)
        reject("fixture/objective ledger");
    std::array<bool, 4> concentrating{};
    for (unsigned i = 0; i < s.effects.instances.size(); ++i) {
        const auto& e = s.effects.instances[i];
        if (!e.active)
            continue;
        if (e.expiresMs > maxCheckpointClock || e.retryMs > maxCheckpointClock ||
            e.readyMs > maxCheckpointClock)
            reject("effect clock capacity");
        if (e.count == 0 || e.count > 3 || !party_.character(e.caster) || e.difficulty < 1 ||
            e.difficulty > 1000 || e.duplicates > 7 || e.reserved)
            reject("effect state");
        for (unsigned j = 0; j < i; ++j)
            if (s.effects.instances[j].active && s.effects.instances[j].id.serial == e.id.serial)
                reject("effect state");
        const bool concentration = e.kind == EffectKind::Bless || e.kind == EffectKind::Bane ||
                                   e.kind == EffectKind::ShieldOfFaith || e.kind == EffectKind::HoldPerson ||
                                   e.kind == EffectKind::Guidance || e.kind == EffectKind::Resistance ||
                                   e.kind == EffectKind::Web;
        if (concentration) {
            if (concentrating[e.caster.index])
                reject("effect state");
            concentrating[e.caster.index] = true;
        }
        for (unsigned j = 0; j < e.count; ++j) {
            const auto& t = e.targets[j];
            if (t.npc ? !actor(t.enemy) : !party_.character(t.character))
                reject("effect state");
            for (unsigned k = 0; k < j; ++k)
                if (t == e.targets[k])
                    reject("effect state");
        }
        if (e.kind == EffectKind::SpiritualWeapon && (!cell(e.cell) || map_.base().solid(e.cell)))
            reject("effect state");
    }
    for (unsigned i = 0; i < s.effects.ground.areas.size(); ++i) {
        const auto& a = s.effects.ground.areas[i];
        if (!a.active)
            continue;
        if (a.untilMs > maxCheckpointClock || a.burnMs > maxCheckpointClock)
            reject("area clock capacity");
        if (a.difficulty < 0 || a.difficulty > 1000000 || !cell(a.cell) || map_.base().solid(a.cell))
            reject("ground effects");
        for (unsigned j = 0; j < i; ++j)
            if (s.effects.ground.areas[j].active && (s.effects.ground.areas[j].id.serial == a.id.serial ||
                                                     s.effects.ground.areas[j].cell == a.cell))
                reject("ground effects");
    }
    for (std::size_t i = 0; i < s.effects.restrained.size(); ++i) {
        const auto& r = s.effects.restrained[i];
        if (!r.area.serial)
            continue;
        if (i >= 4 + s.actors.count || !party_.character(r.caster) || !cell(r.cell))
            reject("ground effects");
        bool found = false;
        for (const auto& a : s.effects.ground.areas)
            found |= a.active && a.web && a.id.serial == r.area.serial && a.cell == r.cell;
        if (!found)
            reject("ground effects");
    }
    for (unsigned i = 0; i < 4; ++i)
        if (s.effects.ground.weapons[i].active) {
            bool found = false;
            for (const auto& e : s.effects.instances)
                found |= e.active && e.kind == EffectKind::SpiritualWeapon && e.caster.index == i &&
                         e.cell == s.effects.ground.weapons[i].cell;
            if (!found)
                reject("ground effects");
        }
    for (std::size_t i = 0; i < lootCapacity; ++i) {
        const auto& stack = s.loot.state.stacks[i];
        if (stack.active && (!cell(stack.cell) || map_.base().solid(stack.cell)))
            reject("loot ledger");
        if (i < lootReservedCapacity && stack.quantity) {
            const auto& original = loot_.state_.stacks[i];
            if (stack.item.index != original.item.index || stack.quantity != original.quantity ||
                stack.generation != 1)
                reject("loot ledger");
            if (i < NpcWorld::maxInstances * lootPerActor && stack.active && !s.loot.deaths[i / lootPerActor])
                reject("loot ledger");
            const auto rewardStart = NpcWorld::maxInstances * lootPerActor + recovery_.groundCount;
            if (i >= rewardStart && i < rewardStart + objectiveDefinition_.rewardCount && stack.active &&
                !s.objective.rewardIssued)
                reject("loot ledger");
        }
    }
}
} // namespace blobber
