#pragma once
#include "game/advancement.hpp"
#include "game/creator.hpp"
namespace blobber {
enum class AdvancementStep { Overview, Features, Abilities, Cantrips, Learned, Prepared, Review };
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
    bool choosing() const noexcept;
    rules::SpellChoice choiceKind() const noexcept;
    void storeSelection() noexcept;
    void ability(unsigned index, int delta) noexcept;
    bool canIncrease(unsigned index) const noexcept;
    rules::CharacterIdentityView candidate() const noexcept;
};
void layoutAdvancement(FlowControls&, const AdvancementHud&) noexcept;
const char* advancementFeatures(rules::ProgressionClass, int level) noexcept;
} // namespace blobber
