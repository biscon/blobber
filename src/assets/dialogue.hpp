#pragma once
#include "game/dialogue.hpp"
#include <filesystem>
namespace blobber {
DialogueCatalog loadDialogue(const std::filesystem::path& directory);
} // namespace blobber
