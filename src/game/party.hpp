#pragma once
#include "rules/content.hpp"
#include "rules/health.hpp"

namespace blobber {
enum class FormationSlot : std::uint8_t { FrontLeft, FrontRight, BackLeft, BackRight };
struct PartyCharacter : rules::HealthState {
    rules::CharacterBuild build;
    rules::CharacterStats stats;
    int availableHitDice = 0;
    std::uint32_t experience = 0;
};
struct PartySnapshot {
    std::array<PartyCharacter, rules::partySize> characters;
    std::array<rules::CharacterId, rules::partySize> formation;
};
// The immutable catalog must outlive this party. Character/item slots never recycle.
class Party {
  public:
    explicit Party(const rules::Catalog& catalog);
    Party(rules::Catalog&&) = delete;
    Party(const rules::Catalog&&) = delete;
    const PartySnapshot& snapshot() const noexcept { return state_; }
    const PartyCharacter* character(rules::CharacterId id) const noexcept;
    const PartyCharacter* occupant(FormationSlot slot) const noexcept;
    bool swapFormation(FormationSlot a, FormationSlot b) noexcept;
    rules::HealthResult damage(rules::CharacterId id, std::int64_t amount, bool critical = false) noexcept;
    rules::HealthResult heal(rules::CharacterId id, std::int64_t amount) noexcept;
    rules::HealthResult stabilize(rules::CharacterId id) noexcept;
    rules::HealthResult deathSave(rules::CharacterId id, int face, int modifier = 0) noexcept;
    void lockFormation(bool locked) noexcept { formationLocked_ = locked; }
    const rules::Catalog& catalog() const noexcept { return *catalog_; }

  private:
    const rules::Catalog* catalog_;
    PartySnapshot state_;
    bool formationLocked_ = false;
    rules::HealthResult commit(rules::CharacterId id, rules::HealthResult result) noexcept;
};
} // namespace blobber
