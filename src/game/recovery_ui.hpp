#pragma once
#include "game/recovery.hpp"
#include "game/creator.hpp"
namespace blobber {
struct CampMember {
    const char* name = "";
    int hp = 0, maxHp = 0, mana = 0, maxMana = 0, available = 0, sides = 0, modifier = 0;
    bool living = true;
    int healingPool = 0, healingMaximum = 0;
    bool channelAvailable = false, hasChannel = false;
};
struct CampHud {
    bool open = false, playing = false;
    double elapsedMs = 0;
    RestRequest request;
    RestPreview preview;
    std::array<CampMember, 4> members{};
    const char* supplier = "";
    RestResult result;
    bool preparing = false, canPrepare = false;
    const rules::Catalog* catalog = nullptr;
    std::array<rules::SpellSelectionContext,4> contexts{};
    std::array<bool,4> casters{};
    std::array<rules::SpellMask,4> editing{};
    unsigned preparationMember = 0;
    PreparationRequest staged, editingRequest;
    rules::SpellSelection selection;
    FlowControls controls;
    TextEdit searchEdit;
    std::array<char,192> preparationMessage{};
};
void layoutPreparation(FlowControls&, const CampHud&) noexcept;
} // namespace blobber
