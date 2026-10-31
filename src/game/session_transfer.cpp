#include "game/campaign.hpp"
#include "game/session.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace blobber {
const char* GameSession::saveSafety() const noexcept {
    if (!runActive())
        return "Campaign has ended.";
    if (timeMs_ > maxCheckpointClock || combat_.snapshot().gameplayMs > maxCheckpointClock ||
        (campaign_ && campaign_->gameplayMs() > maxCheckpointClock))
        return "Save clock capacity exhausted.";
    if (mutation_ == UINT64_MAX)
        return "Session revision exhausted.";
    if (movement_.pending_ || doorPending_ || actionBusy())
        return opening() ? "Wait until the door opens." : "Wait until movement finishes.";
    if (!combat_.settledRecovery(movement().player, timeMs_) || effects_.area(movement().player.cell))
        return "Saving requires settled safe exploration.";
    return nullptr;
}
const char* GameSession::travelSafety() const noexcept {
    if (mutation_ == UINT64_MAX)
        return "Session revision exhausted.";
    if (movement_.pending_ || doorPending_ || actionBusy())
        return opening() ? "Wait until the door opens." : "Wait until movement finishes.";
    if (!combat_.settledRecovery(movement().player, timeMs_) || effects_.area(movement().player.cell))
        return "Travel requires settled safe exploration.";
    bool able = false;
    for (std::uint16_t i = 0; i < rules::partySize; ++i) {
        able |= effects_.canAct(rules::CharacterId{i});
        if (effects_.restrained_[i].area.serial)
            return "Resolve the party's local restraint before travel.";
    }
    if (!able)
        return "An able conscious companion is required.";
    for (const auto& e : effects_.instances_) {
        if (!e.active)
            continue;
        if (e.kind == EffectKind::SpiritualWeapon)
            return "Dismiss Spiritual Weapon before travel.";
        if (e.kind == EffectKind::Web)
            return "Dismiss local Web concentration before travel.";
        const bool concentration = e.kind == EffectKind::Bless || e.kind == EffectKind::Bane ||
                                   e.kind == EffectKind::ShieldOfFaith || e.kind == EffectKind::HoldPerson ||
                                   e.kind == EffectKind::Guidance || e.kind == EffectKind::Resistance;
        if (concentration)
            for (std::size_t i = 0; i < e.count; ++i)
                if (e.targets[i].npc)
                    return "Dismiss concentration attached to local enemies before travel.";
    }
    return nullptr;
}
void GameSession::exportFloor(FloorState& s) const noexcept {
    s.fixtures.completed = floor_.completed_;
    s.fixtures.known = floor_.known_;
    s.fixtures.remembered = floor_.remembered_;
    s.fixtures.discovered = floor_.discovered_;
    s.fixtures.revision = floor_.revision_;
    s.fixtures.geometryRevision = floor_.geometryRevision_;
    s.fixtures.discoveryRevision = floor_.discoveryRevision_;
    s.loot.state = loot_.state_;
    s.loot.deaths = loot_.deaths_;
    s.loot.actorRevision = loot_.actorRevision_;
    s.ai.state = ai_.state_;
    s.ai.prepared = ai_.prepared_;
    s.ai.rng = ai_.rng_;
    s.ai.tickDue = ai_.tickDue_;
    s.ai.priority = ai_.priority_;
    s.effects.instances = effects_.instances_;
    s.effects.guidingUntil = effects_.guidingUntil_;
    s.effects.nextSerial = effects_.nextSerial_;
    s.effects.gameplayMs = effects_.gameplayMs_;
    s.effects.conditions = effects_.conditions_;
    s.effects.casterTurns = effects_.casterTurns_;
    s.effects.ground = effects_.ground_;
    s.effects.restrained = effects_.restrained_;
    s.effects.nextArea = effects_.nextArea_;
    s.effects.groundDueMs = effects_.groundDueMs_;
    s.effects.turns = effects_.turns_;
    s.combat.state = combat_.state_;
    s.combat.sneakReady = combat_.sneakReady_;
    s.combat.cunningReady = combat_.cunningReady_;
    s.combat.extraUntil = combat_.extraUntil_;
    s.combat.partyReady = combat_.partyReady_;
    s.combat.saveDue = combat_.saveDue_;
    s.combat.defendDue = combat_.defendDue_;
    s.combat.processedCasterRound = combat_.processedCasterRound_;
    s.combat.extraAction = combat_.extraAction_;
    s.combat.known = combat_.known_;
    s.combat.episodeActors = combat_.episodeActors_;
    s.combat.knownUntil = combat_.knownUntil_;
    s.combat.enemyReady = combat_.enemyReady_;
    s.combat.enemyBusy = combat_.enemyBusy_;
    s.combat.nextAttack = combat_.nextAttack_;
    s.combat.autoCursor = combat_.autoCursor_;
    s.objective = objective_;
    s.creditedDeaths = creditedDeaths_;
    s.rewardRevision = rewardRevision_;
    s.timeMs = timeMs_;
    s.restToken = restToken_;
    s.remainderMs = remainderMs_;
    s.rng = rng_;
    s.actors = actors_.state_;
    // Canonical logical references: runtime epochs never survive a location switch.
    s.actors.world = s.loot.state.world = s.combat.state.world = 0;
    for (auto& id : s.objective.members)
        id.world = 0;
    for (auto& e : s.effects.instances) {
        e.id.world = 0;
        for (auto& t : e.targets)
            t.enemy.world = 0;
    }
    for (auto& a : s.effects.ground.areas) {
        a.id.world = 0;
        a.effect.world = 0;
    }
    for (auto& r : s.effects.restrained)
        r.area.world = 0;
}
void GameSession::importFloor(const FloorState& s) {
    const auto reject = [] { throw std::invalid_argument("Invalid floor snapshot"); };
    if (s.actors.count != actors_.state_.count || s.actors.typeCount != actors_.state_.typeCount ||
        s.ai.state.count != s.actors.count || s.combat.state.phase != CombatPhase::Exploration ||
        s.effects.turns || s.effects.gameplayMs != s.combat.state.gameplayMs ||
        s.objective.count != objective_.count || !s.effects.nextSerial || !s.effects.nextArea ||
        s.remainderMs < 0 || s.remainderMs >= 1 || !std::isfinite(s.remainderMs))
        reject();
    const auto validCell = [&](Cell c) {
        return c.x >= 0 && c.y >= 0 && c.x < map_.width() && c.y < map_.height();
    };
    // Import is preparation-only: rebuild the staged world occupancy after validating bounds.
    for (unsigned i = floor_.definition_.count; i < fixtureCapacity; ++i)
        if (s.fixtures.completed[i] || s.fixtures.known[i] || s.fixtures.remembered[i])
            reject();
    std::fill(actors_.occupancy_.begin(), actors_.occupancy_.end(), 0);
    for (std::size_t i = 0; i < s.actors.count; ++i) {
        const auto& a = s.actors.actors[i];
        if (a.monster.index != actors_.state_.actors[i].monster.index ||
            a.maxHitPoints != actors_.state_.actors[i].maxHitPoints ||
            unsigned(a.life) > unsigned(ActorLife::Removed) || !validCell(a.npc.cell) ||
            unsigned(a.npc.facing) > 3 || unsigned(a.npc.animation) >= npcAnimationCount ||
            a.npc.type.index >= s.actors.typeCount || a.npc.subcell.x < 0 || a.npc.subcell.x > 1 ||
            a.npc.subcell.y < 0 || a.npc.subcell.y > 1 || a.hitPoints < 0 || a.hitPoints > a.maxHitPoints ||
            (a.life == ActorLife::Alive && (!a.hitPoints || a.hasDeath)))
            reject();
        if (a.life == ActorLife::Alive) {
            if (map_.base().solid(a.npc.cell) ||
                floor_.definition_.npcExcluded[std::size_t(a.npc.cell.y * map_.width() + a.npc.cell.x)])
                reject();
            for (unsigned f = 0; f < floor_.definition_.count; ++f)
                if (floor_.definition_.entries[f].cell == a.npc.cell &&
                    fixtureBlocks(floor_.definition_.entries[f], s.fixtures.completed[f]))
                    reject();
            auto& mask = actors_.mask(a.npc.cell);
            const auto bit = std::uint8_t(1u << (a.npc.subcell.x + 2 * a.npc.subcell.y));
            if (mask & bit)
                reject();
            mask |= bit;
        }
        const auto& ai = s.ai.state.actors[i];
        if (unsigned(ai.awareness) > unsigned(Awareness::Returning) ||
            unsigned(ai.config.routine) > unsigned(NpcRoutine::Wander) || ai.config.waypointCount > 8 ||
            unsigned(ai.homeFacing) > 3 || !validCell(ai.home) ||
            (ai.config.routine == NpcRoutine::Patrol &&
             (!ai.config.waypointCount || ai.waypoint >= ai.config.waypointCount)))
            reject();
        for (unsigned w = 0; w < ai.config.waypointCount; ++w)
            if (!validCell(ai.config.waypoints[w]))
                reject();
        if (s.ai.prepared[i] && !validCell(*s.ai.prepared[i]))
            reject();
    }
    for (const auto& stack : s.loot.state.stacks) {
        if (!stack.generation)
            reject();
        if (stack.quantity) {
            const auto* item = party_.catalog().item(stack.item);
            if (!item || stack.quantity > rules::stackLimit(*item) ||
                (stack.active && !validCell(stack.cell)))
                reject();
        } else if (stack.active)
            reject();
    }
    for (const auto& e : s.effects.instances) {
        if (!e.active)
            continue;
        if (unsigned(e.kind) >= unsigned(EffectKind::Count) || !e.id.serial ||
            e.id.serial >= s.effects.nextSerial || e.count == 0 || e.count > e.targets.size() ||
            !party_.character(e.caster) || e.reserved || e.kind == EffectKind::Restrained ||
            (e.kind >= EffectKind::Sleep && e.kind <= EffectKind::Knockdown))
            reject();
        for (std::size_t i = 0; i < e.count; ++i)
            if (e.targets[i].npc ? e.targets[i].enemy.slot >= s.actors.count
                                 : !party_.character(e.targets[i].character))
                reject();
    }
    for (std::size_t row = 0; row < s.effects.conditions.size(); ++row)
        for (const auto& condition : s.effects.conditions[row])
            if (condition.active &&
                (!party_.character(condition.caster) || (row >= rules::partySize + s.actors.count)))
                reject();
    for (const auto& area : s.effects.ground.areas) {
        if (!area.active)
            continue;
        if (!validCell(area.cell) || !area.id.serial || area.id.serial >= s.effects.nextArea ||
            !party_.character(area.caster))
            reject();
        if (area.web) {
            bool found = false;
            for (const auto& effect : s.effects.instances)
                found |=
                    effect.active && effect.kind == EffectKind::Web && effect.id.serial == area.effect.serial;
            if (!found)
                reject();
        }
    }
    for (std::size_t i = 0; i < s.actors.count; ++i)
        if ((s.creditedDeaths[i] || s.loot.deaths[i]) && !s.actors.actors[i].hasDeath)
            reject();
    for (unsigned i = 0; i < s.objective.count; ++i)
        if (s.objective.members[i].slot != objective_.members[i].slot)
            reject();
    if ((!s.objective.count && s.objective.completed) || s.objective.completed != s.objective.rewardIssued)
        reject();
    if (s.objective.completed) {
        if (!s.fixtures.completed[objectiveDefinition_.gate])
            reject();
        for (unsigned i = 0; i < s.objective.count; ++i)
            if (!s.actors.actors[s.objective.members[i].slot].hasDeath)
                reject();
    }
    floor_.completed_ = s.fixtures.completed;
    floor_.known_ = s.fixtures.known;
    floor_.remembered_ = s.fixtures.remembered;
    floor_.discovered_ = s.fixtures.discovered;
    floor_.revision_ = s.fixtures.revision;
    floor_.geometryRevision_ = s.fixtures.geometryRevision;
    floor_.discoveryRevision_ = s.fixtures.discoveryRevision;
    loot_.state_ = s.loot.state;
    loot_.deaths_ = s.loot.deaths;
    loot_.actorRevision_ = s.loot.actorRevision;
    ai_.state_ = s.ai.state;
    ai_.prepared_ = s.ai.prepared;
    ai_.rng_ = s.ai.rng;
    ai_.tickDue_ = s.ai.tickDue;
    ai_.priority_ = s.ai.priority;
    effects_.instances_ = s.effects.instances;
    effects_.guidingUntil_ = s.effects.guidingUntil;
    effects_.nextSerial_ = s.effects.nextSerial;
    effects_.gameplayMs_ = s.effects.gameplayMs;
    effects_.conditions_ = s.effects.conditions;
    effects_.casterTurns_ = s.effects.casterTurns;
    effects_.ground_ = s.effects.ground;
    effects_.restrained_ = s.effects.restrained;
    effects_.nextArea_ = s.effects.nextArea;
    effects_.groundDueMs_ = s.effects.groundDueMs;
    effects_.turns_ = s.effects.turns;
    combat_.state_ = s.combat.state;
    combat_.sneakReady_ = s.combat.sneakReady;
    combat_.cunningReady_ = s.combat.cunningReady;
    combat_.extraUntil_ = s.combat.extraUntil;
    combat_.partyReady_ = s.combat.partyReady;
    combat_.saveDue_ = s.combat.saveDue;
    combat_.defendDue_ = s.combat.defendDue;
    combat_.processedCasterRound_ = s.combat.processedCasterRound;
    combat_.extraAction_ = s.combat.extraAction;
    combat_.known_ = s.combat.known;
    combat_.episodeActors_ = s.combat.episodeActors;
    combat_.knownUntil_ = s.combat.knownUntil;
    combat_.enemyReady_ = s.combat.enemyReady;
    combat_.enemyBusy_ = s.combat.enemyBusy;
    combat_.nextAttack_ = s.combat.nextAttack;
    combat_.autoCursor_ = s.combat.autoCursor;
    objective_ = s.objective;
    creditedDeaths_ = s.creditedDeaths;
    rewardRevision_ = s.rewardRevision;
    timeMs_ = s.timeMs;
    doors_.now = timeMs_;
    doors_.until.fill(0);
    doorPending_.reset();
    ++doors_.revision;
    restToken_ = s.restToken;
    remainderMs_ = s.remainderMs;
    rng_ = s.rng;
    const auto world = actors_.state_.world;
    actors_.state_ = s.actors;
    actors_.state_.world = world;
    loot_.state_.world = combat_.state_.world = world;
    ai_.geometryRevision_ = floor_.geometryRevision_;
    for (auto& id : objective_.members)
        id.world = world;
    for (auto& e : effects_.instances_) {
        e.id.world = world;
        for (auto& t : e.targets)
            t.enemy.world = world;
    }
    for (auto& a : effects_.ground_.areas) {
        a.id.world = world;
        a.effect.world = world;
    }
    for (auto& r : effects_.restrained_)
        r.area.world = world;
    effects_.partyCell(movement().player.cell);
    effects_.bindEvents(events_, timeMs_);
    combat_.noticeTime_ = timeMs_;
    discoveredReady_ = false;
}
} // namespace blobber
