#pragma once
#include "rules/progression.hpp"
namespace blobber {
enum class AdvancementError { None, Stale, Unavailable, Experience, Unsafe, Moving, Choices, Capacity };
const char* advancementMessage(AdvancementError) noexcept;
struct AdvancementRequest {
    std::uint64_t world = 0, revision = 0, inventoryRevision = 0, preparationRevision = 0;
    rules::CharacterId character;
    int level = 0;
    rules::AdvancementChoices choices;
};
struct AdvancementResult {
    AdvancementError error = AdvancementError::None;
    rules::AdvancementPreview preview;
    explicit operator bool() const noexcept { return error == AdvancementError::None; }
};
} // namespace blobber
