#pragma once
#include <cstdint>
#include <filesystem>
namespace blobber {
// The shared production policy applies to every appearance, including new types.
// A new policy requires an explicit version/implementation change and rebake.
std::uint64_t loadPreparationProfile(const std::filesystem::path& file);
} // namespace blobber
