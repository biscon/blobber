#pragma once
#include "rules/content.hpp"

namespace blobber {
enum class FormationSlot : std::uint8_t { FrontLeft, FrontRight, BackLeft, BackRight };
struct PartyCharacter {
    rules::CharacterBuild build;
    rules::CharacterStats stats;
    int hitPoints = 0, availableHitDice = 0;
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
    const rules::Catalog& catalog() const noexcept { return *catalog_; }

  private:
    const rules::Catalog* catalog_;
    PartySnapshot state_;
};
} // namespace blobber
