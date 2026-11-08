#pragma once
#include "game/subcell_area.hpp"
#include "game/attack_timeline.hpp"
#include <bitset>
namespace blobber {
constexpr bool elementalSpell(rules::SpellKind k) noexcept {
    return k == rules::SpellKind::FireBolt || k == rules::SpellKind::RayOfFrost ||
           k == rules::SpellKind::BurningHands || k == rules::SpellKind::MagicMissile ||
           k == rules::SpellKind::ScorchingRay || k == rules::SpellKind::GuidingBolt ||
           k == rules::SpellKind::SacredFlame || k == rules::SpellKind::ShockingGrasp ||
           k == rules::SpellKind::InflictWounds || k == rules::SpellKind::AcidSplash ||
           k == rules::SpellKind::PoisonSpray || k == rules::SpellKind::Thunderwave ||
           k == rules::SpellKind::Shatter;
}
struct SpellCastVisualId {
    std::uint64_t world = 0, serial = 0;
};
struct SpellTargetVisual {
    ActorId actor{};
    ActorPosition position{};
    float floor = 0;
    std::int64_t damage = 0;
    bool hit = false, gone = false, died = false, saved = false;
};
struct SpellDeliveryVisual {
    CombatAttackId attack{};
    AttackStage stage = AttackStage::Empty;
    std::uint64_t releaseMs = 0, impactMs = 0;
    unsigned darts = 1;
};
struct SpellCastVisual {
    SpellCastVisualId id{};
    CombatAttackId attack{};
    rules::SpellKind spell = rules::SpellKind::Count;
    rules::CharacterId caster{};
    Player origin{};
    std::uint8_t formation = 0;
    AttackStage stage = AttackStage::Empty;
    std::uint64_t releaseMs = 0, impactMs = 0;
    SubcellFootprint footprint;
    std::array<Cell, 16> cells{};
    std::array<bool, 16> eligible{};
    bool includeOrigin = true;
    Cell center{};
    std::array<SpellDeliveryVisual, 3> deliveries{};
    unsigned deliveryCount = 0;
    std::array<SpellTargetVisual, 36> targets{};
    unsigned count = 0;
};
struct SpellVisualSnapshot {
    static constexpr unsigned capacity = 16;
    std::array<SpellCastVisual, capacity> casts{};
    std::uint64_t world = 0, revision = 0, dropped = 0, epoch = 0;
};
struct ActorConditionVisualSnapshot {
    std::uint64_t world = 0;
    std::bitset<NpcWorld::maxInstances> slow, guiding;
    std::array<std::uint32_t, NpcWorld::maxInstances> actors{};
    std::array<std::uint32_t, rules::partySize> party{};
    std::array<std::uint8_t, rules::partySize> duplicates{};
};
// Session-owned, ephemeral journal. Cosmetic capacity never gates combat.
class SpellVisualJournal {
  public:
    void clear(std::uint64_t world) noexcept {
        if (snapshot_.world != world)
            next_ = 1;
        const auto epoch = snapshot_.epoch + 1;
        snapshot_ = {};
        snapshot_.world = world;
        snapshot_.epoch = epoch;
    }
    SpellCastVisual* begin(rules::SpellKind, rules::CharacterId, Player, std::uint64_t) noexcept;
    void attack(const AttackVisual&, SpellTargetVisual, CombatAttackId parent = {}) noexcept;
    void resolve(CombatAttackId, SpellTargetVisual, std::uint64_t, bool canceled = false) noexcept;
    const SpellVisualSnapshot& snapshot() const noexcept { return snapshot_; }

  private:
    SpellVisualSnapshot snapshot_{};
    std::uint64_t next_ = 1;
};
} // namespace blobber
