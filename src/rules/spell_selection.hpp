#pragma once
#include "rules/creation.hpp"
namespace blobber::rules {
enum class SpellChoice { Cantrips, Racial, Learned, Prepared };
// Masks refer to one immutable catalog. Locked grants never consume editable allowance.
struct SpellSelectionContext {
    CharacterIdentityView identity;
    PortraitId portrait;
    std::uint32_t baseline = 0, available = 0, locked = 0;
    unsigned minimum = 0, maximum = 0;
    std::uint32_t granted = 0;
    const char* lockedLabel = "Granted";
    bool editableFirst = false;
    bool showLocked = true;
};
struct SpellSelection {
    SpellSelectionContext context;
    std::uint32_t selected = 0;
    SpellId inspected;
    std::array<SpellId, 32> visible{};
    std::size_t count = 0, offset = 0;
    unsigned detailOffset = 0;
    // Measured by presentation; input uses the last displayed scroll range.
    mutable unsigned detailLines = 0, detailPageLines = 3;
    unsigned dragging = 0; // 0 none, 1 list, 2 description.
    float dragGrab = 0;
    std::array<char, 65> search{};
    void filter(const Catalog&) noexcept;
    bool toggle(SpellId) noexcept;
    void inspectSelected() noexcept;
    bool valid() const noexcept;
};
CharacterIdentityView draftIdentity(const CharacterDraft&) noexcept;
std::uint32_t racialCantrips(const Catalog&) noexcept;
std::uint32_t preparationOptions(const Catalog&, const CharacterIdentityView&, const SpellGrants&) noexcept;
bool validPreparation(const Catalog&, const CharacterIdentityView&, const SpellGrants&,
                      std::uint32_t) noexcept;
SpellSelectionContext choiceContext(const Catalog&, const CharacterIdentityView&, PortraitId,
                                    const SpellGrants&, SpellChoice) noexcept;
std::uint32_t recommendedChoices(const Catalog&, const SpellSelectionContext&, SpellChoice) noexcept;
} // namespace blobber::rules
