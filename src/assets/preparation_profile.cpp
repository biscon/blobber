#include "assets/preparation_profile.hpp"
#include "assets/fingerprint.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace blobber {
std::uint64_t loadPreparationProfile(const std::filesystem::path& file) {
    if (std::filesystem::file_size(file) > 4096)
        throw std::runtime_error("Preparation profile exceeds 4096 bytes");
    std::ifstream input(file);
    const auto profile = nlohmann::json::parse(input);
    const nlohmann::json supported{
        {"version", 1}, {"npc_filter", "area"}, {"npc_resolution", 0.5}, {"surface_filter", "nearest"}};
    if (profile != supported)
        throw std::runtime_error(
            "Unsupported preparation profile; expected NPC profile C and point surfaces");
    const auto canonical = profile.dump();
    auto hash = fingerprintBasis;
    fingerprintBytes(hash, canonical.data(), canonical.size());
    return hash;
}
} // namespace blobber
