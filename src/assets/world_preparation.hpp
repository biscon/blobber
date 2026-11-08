#pragma once
#include "assets/image.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace blobber {
// Offline-only settings; appearance order and simulation metadata stay unchanged.
std::vector<double> npcPreparationResolutions(const std::filesystem::path& file,
                                              const std::vector<std::string>& types);
// Preserve aspect and straight-alpha colour; no upscaling or source-file writes.
void limitWorldSprite(OwnedImage& image, unsigned maximum);
} // namespace blobber
