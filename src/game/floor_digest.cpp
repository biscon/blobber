#include "game/session.hpp"
#include <sstream>
#include <type_traits>
namespace blobber {
namespace {
// Diagnostic serialization enumerates fields: no padding, addresses or runtime epochs.
struct Canonical {
    std::ostringstream out;
    template <class T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, int> = 0>
    void add(T v) {
        if constexpr (std::is_floating_point_v<T>)
            out << std::hexfloat << v << ';';
        else
            out << std::uint64_t(v) << ';';
    }
    template <class T, std::size_t N> void add(const std::array<T, N>& a) {
        for (const auto& v : a)
            add(v);
    }
    template <class T> void add(const std::optional<T>& v) {
        add(bool(v));
        if (v)
            add(*v);
    }
    template <std::size_t N> void add(const std::bitset<N>& bits) {
        for (std::size_t i = 0; i < N; ++i)
            add(bits[i]);
    }
    void add(const std::mt19937& rng) { out << rng << ';'; }
    void add(const rules::DiceRng& rng) {
        rng.describe(out);
        out << ';';
    }
    void add(const Cell& v) {
        add(v.x);
        add(v.y);
    }
    void add(const ActorPosition& v) {
        add(v.x);
        add(v.y);
    }
    void add(const ActorId& v) { add(v.slot); }
    void add(const EffectId& v) { add(v.serial); }
    void add(const AreaId& v) { add(v.serial); }
    void add(const NpcAiConfig& v) {
        add(v.routine);
        add(v.waypoints);
        add(v.waypointCount);
    }
    void add(const NpcInstance& v) {
        add(v.type.index);
        add(v.cell);
        add(v.subcell);
        add(v.facing);
        add(v.animation);
        add(v.phaseMs);
        add(v.ai);
        add(v.monster);
    }
    void add(const ActorState& v) {
        add(v.npc);
        add(v.monster.index);
        add(v.hitPoints);
        add(v.maxHitPoints);
        add(v.life);
        add(v.hasDeath);
        add(v.hasDamage);
        add(v.damageTimeMs);
        add(v.hasAttack);
        add(v.rangedAttack);
        add(v.attackTimeMs);
        add(v.resumableAttack);
        add(v.attackPausedMs);
        add(v.attackPauseFromMs);
        add(v.attackPauseUntilMs);
        add(v.hasWalk);
        add(v.walkTimeMs);
        add(v.walkDurationMs);
        add(v.hasTravel);
        add(v.walkFrom);
    }
    void add(const ActorSnapshot& v) {
        add(v.actors);
        add(v.count);
        add(v.typeCount);
        add(v.revision);
    }
    void add(const FloorGeometry::Saved& v) {
        add(v.completed);
        add(v.known);
        add(v.remembered);
        add(v.discovered);
        add(v.revision);
        add(v.geometryRevision);
        add(v.discoveryRevision);
    }
    void add(const FloorLoot& v) {
        add(v.cell);
        add(v.item.index);
        add(v.quantity);
        add(v.active);
        add(v.generation);
    }
    void add(const LootSnapshot& v) {
        add(v.stacks);
        add(v.revision);
    }
    void add(const LootWorld::Saved& v) {
        add(v.state);
        add(v.deaths);
        add(v.actorRevision);
    }
    void add(const NpcAiState& v) {
        add(v.awareness);
        add(v.home);
        add(v.lastSeen);
        add(v.homeFacing);
        add(v.config);
        add(v.waypoint);
        add(v.searchTurns);
        add(v.lastSeenMs);
        add(v.searchStartMs);
        add(v.moveDueMs);
    }
    void add(const NpcAiSnapshot& v) {
        add(v.actors);
        add(v.count);
        add(v.searchWork);
        add(v.completedSearches);
        add(v.timeMs);
    }
    void add(const NpcAiController::Saved& v) {
        add(v.state);
        add(v.prepared);
        add(v.rng);
        add(v.tickDue);
        add(v.priority);
    }
    void add(const EffectTarget& v) {
        add(v.npc);
        add(v.character.index);
        add(v.enemy);
    }
    void add(const EffectInstance& v) {
        add(v.id);
        add(v.kind);
        add(v.caster.index);
        add(v.targets);
        add(v.count);
        add(v.difficulty);
        add(v.expiresMs);
        add(v.retryMs);
        add(v.active);
        add(v.duplicates);
        add(v.reserved);
        add(v.cell);
        add(v.readyMs);
    }
    void add(const ConditionState& v) {
        add(v.caster.index);
        add(v.untilMs);
        add(v.remainingMs);
        add(v.casterTurn);
        add(v.active);
        add(v.turnBound);
    }
    void add(const GroundArea& v) {
        add(v.id);
        add(v.cell);
        add(v.caster.index);
        add(v.difficulty);
        add(v.untilMs);
        add(v.active);
        add(v.web);
        add(v.effect);
        add(v.burnMs);
    }
    void add(const SpectralWeapon& v) {
        add(v.cell);
        add(v.active);
    }
    void add(const GroundSnapshot& v) {
        add(v.weapons);
        add(v.areas);
        add(v.revision);
    }
    void add(const Restraint& v) {
        add(v.area);
        add(v.cell);
        add(v.difficulty);
        add(v.caster.index);
    }
    void add(const EffectController::Saved& v) {
        add(v.instances);
        add(v.guidingUntil);
        add(v.nextSerial);
        add(v.gameplayMs);
        add(v.conditions);
        add(v.casterTurns);
        add(v.ground);
        add(v.restrained);
        add(v.nextArea);
        add(v.groundDueMs);
        add(v.turns);
    }
    void add(const CombatParticipant& v) {
        add(v.character);
        add(v.enemy);
        add(v.deathSave);
    }
    void add(const CombatSnapshot& v) {
        add(v.phase);
        add(v.turn);
        add(v.round);
        add(v.activeCharacter);
        add(v.activeEnemy);
        add(v.awaitingAction);
        add(v.resolvingAction);
        add(v.canEndTurn);
        add(v.defending);
        add(v.enemies);
        add(v.enemyCount);
        add(v.upcoming);
        add(v.upcomingCount);
        add(v.cooldownMs);
        add(v.spent);
        add(v.facing);
        add(v.knownDirections);
        add(v.pendingCount);
        add(v.gameplayMs);
    }
    void add(const CombatController::Saved& v) {
        add(v.state);
        add(v.sneakReady);
        add(v.cunningReady);
        add(v.extraUntil);
        add(v.partyReady);
        add(v.saveDue);
        add(v.defendDue);
        add(v.processedCasterRound);
        add(v.extraAction);
        add(v.known);
        add(v.episodeActors);
        add(v.knownUntil);
        add(v.enemyReady);
        add(v.enemyBusy);
        add(v.nextAttack);
        add(v.autoCursor);
    }
    void add(const FloorObjectiveSnapshot& v) {
        add(v.members);
        add(v.count);
        add(v.known);
        add(v.completed);
        add(v.rewardIssued);
        add(v.capacityFailure);
        add(v.revision);
    }
    void add(const FloorState& v) {
        add(v.fixtures);
        add(v.actors);
        add(v.loot);
        add(v.ai);
        add(v.effects);
        add(v.combat);
        add(v.objective);
        add(v.creditedDeaths);
        add(v.rewardRevision);
        add(v.timeMs);
        add(v.restToken);
        add(v.remainderMs);
        add(v.rng);
    }
    void add(const rules::CharacterId& v) { add(v.index); }
    void add(const rules::MonsterId& v) { add(v.index); }
};
} // namespace
std::uint64_t FloorState::digest() const {
    Canonical fields;
    fields.add(*this);
    std::uint64_t hash = 14695981039346656037ull;
    for (const unsigned char c : fields.out.str()) {
        hash ^= c;
        hash *= 1099511628211ull;
    }
    return hash;
}
} // namespace blobber
