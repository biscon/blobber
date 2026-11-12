#include "audio/bank.hpp"
#include "platform/file_io.hpp"
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
namespace blobber::audio {
using Json = nlohmann::json;
namespace {
Json read(const std::filesystem::path& p) {
    if (std::filesystem::file_size(p) > 1024 * 1024)
        throw std::runtime_error("Audio JSON exceeds 1 MiB");
    std::ifstream f(p);
    return Json::parse(f);
}
float number(const Json& j, const char* key, float def, float low, float high) {
    float v = j.value(key, def);
    if (!std::isfinite(v) || v < low || v > high)
        throw std::runtime_error(std::string("Invalid audio ") + key);
    return v;
}
bool safePath(const std::string& s) {
    const auto p = std::filesystem::u8path(s);
    if (s.empty() || p.has_root_path())
        return false;
    for (const auto& part : p)
        if (part == "..")
            return false;
    return true;
}
} // namespace
Bank loadBank(const std::filesystem::path& root) {
    Bank bank;
    try {
        const auto base = read(root / "bank.json");
        if (base.contains("mix_gain_db")) {
            const auto& mix = base.at("mix_gain_db");
            if (!mix.is_object())
                throw std::runtime_error("Invalid mix_gain_db");
            const char* categories[]{"", "effects", "ambience", "ui", "party_footsteps"};
            for (unsigned i = 1; i < bank.categoryGain.size(); ++i)
                bank.categoryGain[i] = std::pow(10.f, number(mix, categories[i], 0, -24, 18) / 20.f);
        }
        Json overrides = Json::object();
        if (std::filesystem::exists(root / "overrides.json")) {
            try {
                overrides = read(root / "overrides.json").at("cues");
            } catch (const std::exception& e) {
                bank.warning += std::string("Overrides ignored: ") + e.what() + "\n";
            }
        }
        const auto& defs = base.at("cues");
        if (!defs.is_object() || defs.size() > 256)
            throw std::runtime_error("Invalid cue capacity");
        for (auto it = defs.begin(); it != defs.end(); ++it)
            try {
                auto j = it.value();
                const auto original = j;
                if (overrides.contains(it.key())) {
                    const auto& o = overrides[it.key()];
                    for (auto f = o.begin(); f != o.end(); ++f)
                        if (f.key() != "variant_overrides")
                            j[f.key()] = f.value();
                    if (o.contains("variant_overrides"))
                        for (auto& v : j["variants"])
                            if (o["variant_overrides"].contains(v.at("id").get<std::string>()))
                                v.update(o["variant_overrides"][v.at("id").get<std::string>()]);
                }
                Cue c;
                c.id = it.key();
                c.gain = number(j, "gain", .4f, 0, 1);
                c.pitchMin = number(j, "pitch_min", .96f, .5f, 2);
                c.pitchMax = number(j, "pitch_max", 1.04f, c.pitchMin, 2);
                c.volumeMin = number(j, "volume_min", .9f, 0, 1);
                c.range = number(j, "range", 6, 1.01f, 32);
                c.cooldown = number(j, "cooldown", .06f, 0, 10);
                c.category = unsigned(number(j, "category", 1, 1, 4));
                c.priority = unsigned(number(j, "priority", 1, 0, 4));
                c.concurrency = unsigned(number(j, "concurrency", 4, 1, 8));
                c.aliases = unsigned(number(j, "aliases", 4, 1, 8));
                c.loop = j.value("loop", false);
                const auto& variants = j.at("variants");
                if (!variants.is_array() || variants.empty() || variants.size() > 16)
                    throw std::runtime_error("Invalid variants");
                std::set<std::string> ids;
                for (const auto& v : variants) {
                    Variant a{v.at("id"), v.at("path"), number(v, "gain", 1, 0, 2)};
                    if (!ids.insert(a.id).second || !safePath(a.path))
                        throw std::runtime_error("Invalid variant identity/path");
                    if (!std::filesystem::is_regular_file(root / std::filesystem::u8path(a.path))) {
                        for (const auto& b : original.at("variants"))
                            if (b.at("id") == a.id) {
                                bank.warning += "Missing " + a.path + "; using generated base\n";
                                a.path = b.at("path");
                                break;
                            }
                    }
                    if (!safePath(a.path))
                        throw std::runtime_error("Invalid base variant path");
                    c.variants.push_back(std::move(a));
                }
                bank.cues.push_back(std::move(c));
            } catch (const std::exception& e) {
                bank.warning += "Cue " + it.key() + " disabled: " + e.what() + "\n";
            }
    } catch (const std::exception& e) {
        bank.warning += std::string("Audio bank unavailable: ") + e.what();
    }
    return bank;
}
std::filesystem::path settingsPath(const std::filesystem::path& p) {
    if (!p.empty())
        return p;
    try {
        return disk::configDirectory() / "settings.json";
    } catch (const std::exception&) {
        return {};
    }
}
Settings loadSettings(const std::filesystem::path& p, std::string& warning) {
    Settings s;
    try {
        if (!p.empty() && std::filesystem::exists(p)) {
            const auto j = read(p);
            const auto v = j.at("audio").get<std::array<float, 5>>();
            for (float x : v)
                if (!std::isfinite(x) || x < 0 || x > 1)
                    throw std::runtime_error("Invalid audio volume");
            s.values = v;
            s.musicVolume = number(j, "music_volume", .5f, 0, 1);
            s.musicEnabled = j.value("music_enabled", true);
            s.borderlessFullscreen = j.value("borderless_fullscreen", false);
        }
    } catch (const std::exception& e) {
        warning = e.what();
    }
    return s;
}
bool saveSettings(const std::filesystem::path& p, const Settings& s, std::string& warning) {
    try {
        if (p.empty())
            throw std::runtime_error("No settings directory");
        std::filesystem::create_directories(p.parent_path());
        disk::writeDocument(p, Json{{"version", 2},
                                    {"audio", s.values},
                                    {"music_volume", s.musicVolume},
                                    {"music_enabled", s.musicEnabled},
                                    {"borderless_fullscreen", s.borderlessFullscreen}}
                                       .dump(2) +
                                   "\n");
        return true;
    } catch (const std::exception& e) {
        warning = e.what();
        return false;
    }
}
} // namespace blobber::audio
