#pragma once
#include "rules/content.hpp"

namespace blobber::rules {
struct PortraitDefinition {
    const char *id, *race, *file;
};
constexpr std::size_t portraitCount = 20;
const PortraitDefinition* portrait(PortraitId) noexcept;
PortraitId findPortrait(std::string_view) noexcept;
PortraitId defaultPortrait(std::string_view race) noexcept;

// Fixed mutable UI/library storage. All references belong to the immutable catalog.
struct CharacterDraft {
    std::array<char, 65> name{};
    RaceId race;
    ClassId characterClass;
    PortraitId portrait;
    Abilities abilities{8, 8, 8, 8, 8, 8};
    bool defense = false, lifeDomain = false;
    SpellGrants spells;
    std::array<std::uint8_t, 3> gear{};
};
enum class BuildError : std::uint8_t {
    None,
    Name,
    Race,
    Class,
    Portrait,
    Abilities,
    Budget,
    Features,
    Cantrips,
    Learned,
    Prepared,
    Domain,
    Racial,
    Gear,
    Derived,
    Content
};
struct BuildPreview {
    std::uint32_t errors = 0;
    CharacterStats stats{};
    CharacterLoadout loadout{};
    int points = 0, preparation = 0;
    CharacterError derivation = CharacterError::None;
    bool has(BuildError e) const noexcept { return (errors & (1u << unsigned(e))) != 0; }
    explicit operator bool() const noexcept { return errors == 0; }
};
const char* buildErrorMessage(BuildError) noexcept;
int pointCost(int score) noexcept;
int pointBuyCost(const Abilities&) noexcept;
unsigned spellCount(std::uint32_t) noexcept;
std::uint32_t spellBit(SpellId) noexcept;
std::uint32_t classSpells(const Catalog&, ClassId, int level) noexcept;
int preparationLimit(const Catalog&, const CharacterIdentityView&) noexcept;
bool recommendSpells(const Catalog&, const CharacterIdentityView&, SpellGrants&) noexcept;
BuildError validateGrants(const Catalog&, const CharacterIdentityView&, const SpellGrants&,
                          bool counts = true) noexcept;
enum class SpellSource : std::uint8_t { None, Class, Domain, Racial };
struct SpellAccess {
    SpellSource source = SpellSource::None;
    Casting casting = Casting::None;
    int modifier = 0, attack = 0, difficulty = 0, cost = 0;
    explicit operator bool() const noexcept { return source != SpellSource::None; }
};
SpellAccess spellAccess(const Catalog&, const CharacterIdentity&, const CharacterStats&, SpellId) noexcept;
const CreationClass* creationClass(const Catalog&, ClassId) noexcept;
BuildPreview previewBuild(const Catalog&, const CharacterDraft&) noexcept;
CharacterDraft recommendedDraft(const Catalog&, RaceId, ClassId) noexcept;
CharacterBuild finishBuild(const Catalog&, const CharacterDraft&, std::string id);
// Recover exact curated gear choices from authored/library builds; no arbitrary gear import.
bool draftFromBuild(const Catalog&, const CharacterBuild&, CharacterDraft&) noexcept;
} // namespace blobber::rules
