#pragma once
#include "rules/creation.hpp"
#include <nlohmann/json.hpp>
namespace blobber {
nlohmann::json encodeStartingDraft(const rules::Catalog&, const rules::CharacterDraft&,
                                   const std::string& id);
rules::CharacterDraft decodeStartingDraft(const rules::Catalog&, const nlohmann::json&);
} // namespace blobber
