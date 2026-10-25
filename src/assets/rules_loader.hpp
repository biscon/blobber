#pragma once
#include "rules/content.hpp"
#include <filesystem>

namespace blobber {
// Explicit loading phase: throws a file/field diagnostic; never publishes partial content.
rules::Catalog loadRules(const std::filesystem::path& path);
} // namespace blobber
