#pragma once
#include "game/advancement.hpp"
#include "game/creator.hpp"
namespace blobber {
enum class AdvancementStep { Overview, Features, Learned, Prepared, Review };
struct AdvancementHud {
    bool open = false;
    AdvancementStep step = AdvancementStep::Overview;
    const rules::Catalog* catalog = nullptr;
    rules::CharacterIdentityView identity;
    rules::PortraitId portrait;
    rules::SpellGrants baseline;
    AdvancementRequest request;
    AdvancementResult preview;
    rules::SpellSelection selection;
    TextEdit searchEdit;
    FlowControls controls;
    int oldHp = 0, oldMana = 0;
    std::array<char, 192> message{};
    bool applicable(AdvancementStep) const noexcept;
    void sync() noexcept;
};
void layoutAdvancement(FlowControls&, const AdvancementHud&) noexcept;
const char* advancementFeatures(rules::ProgressionClass, int level) noexcept;
} // namespace blobber
