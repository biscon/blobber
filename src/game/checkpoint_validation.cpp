#include "game/session.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace blobber {
void GameSession::validateContainers(const FloorState& s) const {
    const auto reject = [](const char* part) { throw std::invalid_argument(part); };
    if (s.loot.state.containerCount != loot_.state_.containerCount)
        reject("container count");
    for (unsigned i = 0; i < containerCapacity; ++i) {
        const auto& saved = s.loot.state.containers[i];
        const auto& original = loot_.state_.containers[i];
        if (!(saved.cell == original.cell) || saved.facing != original.facing ||
            (i >= s.loot.state.containerCount && (saved.opened || saved.sealed)) ||
            (i < s.loot.state.containerCount && ((saved.opened && saved.sealed))))
            reject("container state");
    }
    for (const auto& d : map_.base().definition().decorations)
        if (d.kind == DecorationKind::Chest) {
            const auto& c = s.loot.state.containers[d.container];
            if (!(c.cell == d.cell) || c.facing != d.face ||
                c.sealed != (d.sealed && !s.objective.completed) || (c.sealed && c.opened))
                reject("Invalid container definition/state");
        }
    for (unsigned i = 0; i < lootCapacity; ++i) {
        const auto& stack = s.loot.state.stacks[i];
        if (stack.container != loot_.state_.stacks[i].container ||
            (stack.container != noContainer &&
             (stack.container >= s.loot.state.containerCount ||
              !(stack.cell == s.loot.state.containers[stack.container].cell))))
            reject("Invalid container loot owner");
    }
}
void GameSession::validateCheckpoint(const FloorState& s) const {
    const auto reject = [](const char* part) {
        throw std::invalid_argument(std::string("Invalid saved floor ") + part);
    };
    const auto cell = [&](Cell p) {
        return p.x >= 0 && p.y >= 0 && p.x < map_.width() && p.y < map_.height();
    };
    const auto actor = [&](ActorId id) {
        return id.slot < s.actors.count && s.actors.actors[id.slot].hostile();
    };
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
    for (std::size_t i = 0; i < NpcWorld::maxInstances; ++i) {
        const auto* definition =
            i < s.actors.count ? actors_.catalog().monster(s.actors.actors[i].monster) : nullptr;
        const bool caster = definition && definition->spellcasting == rules::EnemyCasting::AcolyteSupport;
        if (s.combat.enemyTarget[i] >= rules::partySize || s.combat.enemyMana[i] > (caster ? 6 : 0) ||
            (!caster && s.combat.enemyTarget[i]))
            reject("NPC casting resources");
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
        if ((f.kind != FixtureKind::Door && s.fixtures.open[i]) ||
            (s.fixtures.open[i] && !s.fixtures.completed[i]) ||
            (s.fixtures.rememberedOpen[i] && (!s.fixtures.known[i] || !s.fixtures.remembered[i])) ||
            (f.kind != FixtureKind::Door && f.kind != FixtureKind::GuardianGate &&
             s.fixtures.rememberedOpen[i]))
            reject("door state");
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
    std::array<bool, rules::partySize + NpcWorld::maxInstances> concentrating{};
    for (unsigned i = 0; i < s.effects.instances.size(); ++i) {
        const auto& e = s.effects.instances[i];
        if (e.pendingBless)
            reject("pending Bless in settled snapshot");
        if (!e.active)
            continue;
        if (e.expiresMs > maxCheckpointClock || e.retryMs > maxCheckpointClock ||
            e.readyMs > maxCheckpointClock)
            reject("effect clock capacity");
        if (e.count == 0 || e.count > 3 ||
            (e.caster.npc ? !actor(e.caster.enemy) || e.kind != EffectKind::Bless ||
                                s.actors.actors[e.caster.enemy.slot].life != ActorLife::Alive ||
                                !actors_.catalog().monster(s.actors.actors[e.caster.enemy.slot].monster) ||
                                actors_.catalog()
                                        .monster(s.actors.actors[e.caster.enemy.slot].monster)
                                        ->spellcasting != rules::EnemyCasting::AcolyteSupport
                          : !party_.character(e.caster.character)) ||
            e.difficulty < 1 || e.difficulty > 1000 || e.duplicates > 7 || e.reserved)
            reject("effect state");
        for (unsigned j = 0; j < i; ++j)
            if (s.effects.instances[j].active && s.effects.instances[j].id.serial == e.id.serial)
                reject("effect state");
        const bool concentration = e.kind == EffectKind::Bless || e.kind == EffectKind::Bane ||
                                   e.kind == EffectKind::ShieldOfFaith || e.kind == EffectKind::HoldPerson ||
                                   e.kind == EffectKind::Guidance || e.kind == EffectKind::Resistance ||
                                   e.kind == EffectKind::Web;
        if (concentration) {
            const auto sourceIndex =
                e.caster.npc ? rules::partySize + e.caster.enemy.slot : e.caster.character.index;
            if (concentrating[sourceIndex])
                reject("effect state");
            concentrating[sourceIndex] = true;
        }
        for (unsigned j = 0; j < e.count; ++j) {
            const auto& t = e.targets[j];
            if ((e.caster.npc && !t.npc) || (t.npc ? !actor(t.enemy) : !party_.character(t.character)))
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
        if (!a.footprint.mask || a.footprint.count() > (a.web ? 16u : 4u) || a.footprint.origin.x < -7 ||
            a.footprint.origin.y < -7 || a.footprint.origin.x > map_.width() * 2 ||
            a.footprint.origin.y > map_.height() * 2)
            reject("ground footprint");
        std::uint64_t earliestBurn = 0;
        for (unsigned bit = 0; bit < 64; ++bit) {
            if (a.burnDue[bit] && (!earliestBurn || a.burnDue[bit] < earliestBurn))
                earliestBurn = a.burnDue[bit];
            const bool covered = a.footprint.mask & (UINT64_C(1) << bit);
            if (a.burnDue[bit] > maxCheckpointClock || (a.burnDue[bit] && (!covered || !a.web)))
                reject("ground fire");
            if (covered && (!cell(dungeonCell(a.footprint.tile(bit))) ||
                            map_.base().solid(dungeonCell(a.footprint.tile(bit)))))
                reject("ground footprint");
        }
        if (a.burnMs != earliestBurn)
            reject("ground fire deadline");
        for (unsigned j = 0; j < i; ++j)
            if (s.effects.ground.areas[j].active && (s.effects.ground.areas[j].id.serial == a.id.serial))
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
            found |= a.active && a.web && a.id.serial == r.area.serial && [&] {
                for (unsigned bit = 0; bit < 64; ++bit)
                    if ((a.footprint.mask & (UINT64_C(1) << bit)) &&
                        dungeonCell(a.footprint.tile(bit)) == r.cell)
                        return true;
                return false;
            }();
        if (!found)
            reject("ground effects");
    }
    for (unsigned i = 0; i < 4; ++i)
        if (s.effects.ground.weapons[i].active) {
            bool found = false;
            for (const auto& e : s.effects.instances)
                found |= e.active && e.kind == EffectKind::SpiritualWeapon && e.caster.character.index == i &&
                         e.cell == s.effects.ground.weapons[i].cell;
            if (!found)
                reject("ground effects");
        }
    validateContainers(s);
    for (std::size_t i = 0; i < lootCapacity; ++i) {
        const auto& stack = s.loot.state.stacks[i];
        const auto& initial = loot_.state_.stacks[i];
        if (stack.container != initial.container ||
            (stack.container != noContainer &&
             (stack.container >= s.loot.state.containerCount ||
              !(stack.cell == s.loot.state.containers[stack.container].cell))))
            reject("container loot ownership");
        if (stack.active && (!cell(stack.cell) || map_.base().solid(stack.cell)))
            reject("loot ledger");
        if (initial.container != noContainer && !s.loot.state.containers[initial.container].opened &&
            (stack.quantity != initial.quantity || stack.item.index != initial.item.index ||
             stack.active != (initial.active || s.objective.rewardIssued)))
            reject("unopened container contents");
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
