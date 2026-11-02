#pragma once
#include "audio/policy.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace blobber::audio {
struct Variant {
    std::string id, path;
    float gain = 1;
};
struct Cue {
    std::string id;
    std::vector<Variant> variants;
    float gain = .4f, pitchMin = .96f, pitchMax = 1.04f, volumeMin = .9f, range = 6, cooldown = .06f;
    unsigned category = 1, priority = 1, concurrency = 4, aliases = 4;
    bool loop = false;
};
struct Bank {
    std::vector<Cue> cues;
    std::string warning;
};
Bank loadBank(const std::filesystem::path&);
std::filesystem::path settingsPath(const std::filesystem::path& overridePath = {});
Settings loadSettings(const std::filesystem::path&, std::string& warning);
bool saveSettings(const std::filesystem::path&, const Settings&, std::string& warning);
} // namespace blobber::audio
