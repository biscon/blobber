#pragma once
#include "game/recovery.hpp"
#include "rules/creation.hpp"
#include "rules/health.hpp"

namespace blobber {
enum class FormationSlot : std::uint8_t { FrontLeft, FrontRight, BackLeft, BackRight };
struct PartyCharacter : rules::HealthState {
    rules::CharacterIdentity build;
    rules::CharacterStats stats;
    int availableHitDice = 0;
    int mana = 0, maxMana = 0;
    bool secondWind = false, arcaneRecovery = false;
    std::uint32_t experience = 0;
    std::uint64_t temporaryUntilMs = 0;
};
struct PartySnapshot {
    std::array<PartyCharacter, rules::partySize> characters;
    std::array<rules::CharacterId, rules::partySize> formation;
};
// The immutable catalog must outlive this party. Item handles are generation checked.
class Party {
  public:
    explicit Party(const rules::Catalog& catalog);
    Party(const rules::Catalog&, const rules::PresetParty& startingParty);
    Party(rules::Catalog&&, const rules::PresetParty&) = delete;
    Party(const rules::Catalog&&, const rules::PresetParty&) = delete;
    Party(const Party&) = delete;
    Party& operator=(const Party&) = delete;
    Party(Party&&) = delete;
    Party& operator=(Party&&) = delete;
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
    bool grantTemporary(rules::CharacterId, int amount, std::uint64_t until) noexcept;
    void expireTemporary(std::uint64_t now) noexcept;
    bool spendMana(rules::CharacterId, int amount) noexcept;
    bool useFeature(rules::CharacterId, rules::Feature) noexcept;
    bool hasFeature(rules::CharacterId, rules::ClassFeature) const noexcept;
    int castingModifier(rules::CharacterId) const noexcept;
    rules::SpellAccess spellAccess(rules::CharacterId, rules::SpellId) const noexcept;
    void lockFormation(bool locked) noexcept { formationLocked_ = locked; }
    const Inventory& inventory() const noexcept { return inventory_; }
    InventoryResult previewInventoryChange(const InventoryRequest&) const noexcept;
    InventoryResult inventoryChange(const InventoryRequest&) noexcept;
    InventoryResult acquire(const AcquisitionRequest&, bool preview = false) noexcept;
    PreparationRequest preparationRequest() const noexcept;
    bool alternativePreparation(RestRequest&) const noexcept;
    std::uint64_t preparationRevision() const noexcept { return preparationRevision_; }
    RestError recoveryBenefits(const RestRequest&) const noexcept;
    RestResult recover(const RestRequest&, ItemHandle supply, rules::DiceRng&) noexcept;
    InventoryResult releaseItem(ItemHandle, std::uint64_t revision, bool preview = false) noexcept;
    bool consumeItem(ItemHandle, unsigned quantity = 1) noexcept;
    bool restoreMana(rules::CharacterId, int amount) noexcept;
    const rules::ItemDefinition* equipped(rules::CharacterId, EquipmentSlot) const noexcept;
    const rules::Catalog& catalog() const noexcept { return *catalog_; }

  private:
    const rules::Catalog* catalog_;
    PartySnapshot state_;
    Inventory inventory_;
    bool formationLocked_ = false;
    std::uint64_t preparationRevision_ = 0;
    InventoryResult stageInventoryChange(const InventoryRequest&, Inventory&,
                                         std::array<rules::CharacterStats, 4>&) const noexcept;
    rules::HealthResult commit(rules::CharacterId id, rules::HealthResult result) noexcept;
};
} // namespace blobber
