#pragma once
#include "game/dialogue.hpp"
#include <filesystem>
namespace blobber {
// Audio/overlay references use shared audio/UI roots, independent of script overrides.
// Omitted roots default to sibling audio/ui directories of the script directory.
DialogueCatalog loadDialogue(const std::filesystem::path& directory,
                             const std::filesystem::path& audioDirectory = {},
                             const std::filesystem::path& uiDirectory = {});
} // namespace blobber
