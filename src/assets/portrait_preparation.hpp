#pragma once
#include "assets/portrait_atlas.hpp"
#include <filesystem>
namespace blobber {
// Explicit offline phase; bounded inputs and workspace, never called by frame processing.
void preparePortraits(const std::filesystem::path& source, const std::filesystem::path& output);
} // namespace blobber
