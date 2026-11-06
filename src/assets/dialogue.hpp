#pragma once
#include "game/dialogue.hpp"
#include <filesystem>
namespace blobber {
// Audio references use the shared asset audio root, independent of script overrides.
// When omitted, use the sibling audio directory of the script directory.
DialogueCatalog loadDialogue(const std::filesystem::path& directory,
                             const std::filesystem::path& audioDirectory = {});
} // namespace blobber
