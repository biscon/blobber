#pragma once
#include "rules/content.hpp"
#include <nlohmann/json_fwd.hpp>
namespace blobber {
// Internal preparation result; Catalog performs final cross-definition validation.
struct CharacterContent {
    std::vector<rules::SpellDefinition> spells;
    std::vector<rules::CreationClass> creation;
    std::vector<rules::RaceDefinition> races;
    std::vector<rules::ClassDefinition> classes;
    std::vector<rules::ItemDefinition> items;
    std::optional<rules::PresetParty> preset;
};
CharacterContent loadCharacterContent(const nlohmann::json& root);
} // namespace blobber
