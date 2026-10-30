#pragma once
#include "rules/creation.hpp"
namespace blobber {
struct StartingSnapshot {
    std::array<char, 129> id{};
    rules::CharacterDraft draft;
};
using StartingParty = std::array<StartingSnapshot, 4>;
rules::PresetParty startingParty(const rules::Catalog&, const StartingParty&);
} // namespace blobber
