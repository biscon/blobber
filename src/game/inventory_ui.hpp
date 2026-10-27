#pragma once
#include "game/command_types.hpp"
namespace blobber {
enum class CharacterPage : std::uint8_t { Inventory, Character };
struct CharacterFeatureRow {
    const char* name = "";
    const char* detail = "";
    std::array<char, 32> value{};
};
struct CharacterSheet {
    rules::Abilities modifiers{}, saves{};
    std::array<bool, 6> proficientSaves{};
    int initiative = 0, proficiency = 0, hitDice = 0, hitDie = 0;
    int spellAttack = 0, spellDifficulty = 0;
    std::uint32_t experience = 0, nextLevelExperience = 0;
    unsigned arrows = 0;
    std::array<char, 96> armor{};
    std::array<char, 384> weapons{};
    std::array<char, 192> effects{};
    std::array<CharacterFeatureRow, 6> features{};
    unsigned featureCount = 0;
};
enum class InventoryMode : std::uint8_t { Browse, Transfer, Quantity };
enum class InventoryAction : std::uint8_t { Equip, Transfer, Use, Count };
struct InventoryActionState {
    bool visible = false, enabled = false;
    const char* label = "";
    const char* reason = "";
};
struct InventoryCell {
    ItemHandle handle;
    const rules::ItemDefinition* definition = nullptr;
    unsigned quantity = 0;
    rules::WeaponPreview weapon;
};
struct InventoryHud {
    CharacterPage page = CharacterPage::Inventory;
    CharacterSheet sheet;
    bool open = false, pointerDown = false, dragging = false, splitDrag = false, itemTargeting = false;
    rules::CharacterId character{0};
    std::uint16_t cursor = 0;
    std::array<InventoryCell, 52> cells{};
    ItemHandle selected{}, source{};
    InventoryMode mode = InventoryMode::Browse;
    std::array<InventoryActionState, static_cast<unsigned>(InventoryAction::Count)> actions{};
    InventoryRequest pending;
    unsigned quantity = 1, maximum = 1;
    std::uint64_t revision = 0;
    float startX = 0, startY = 0, pointerX = 0, pointerY = 0;
    std::int64_t weight = 0, capacity = 0;
    int armorClass = 0, descriptionPage = 0;
    InventoryError management = InventoryError::None;
    std::array<char, 192> feedback{};
    PartyCommand itemCommand;
    std::array<CommandReason, 4> recipients{};
    const rules::ItemDefinition* usingItem = nullptr;
};
} // namespace blobber
