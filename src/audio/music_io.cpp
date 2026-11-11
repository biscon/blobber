#include "audio/music.hpp"
#include "platform/file_io.hpp"
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
namespace blobber::audio {
namespace {
using Json = nlohmann::json;
Json read(const std::filesystem::path& path) {
    if (std::filesystem::file_size(path) > 1024 * 1024)
        throw std::runtime_error("Music metadata exceeds 1 MiB");
    std::ifstream in(path);
    return Json::parse(in);
}
} // namespace
std::vector<MusicTrack> loadMusicTracks(const std::filesystem::path& root, std::string& warning) {
    std::vector<MusicTrack> tracks;
    if (!std::filesystem::exists(root / "playlist.json"))
        return tracks;
    try {
        const auto j = read(root / "playlist.json").at("tracks");
        if (!j.is_array() || j.size() > musicCapacity)
            throw std::runtime_error("Invalid music track capacity");
        std::set<std::string> ids;
        for (const auto& entry : j) {
            try {
                MusicTrack t{entry.at("id"), entry.at("path"), entry.at("title"), entry.at("artist")};
                const float db = entry.value("gain_db", 0.f);
                const std::filesystem::path path(t.path);
                bool safe = !path.empty() && !path.is_absolute();
                for (const auto& part : path)
                    safe &= part != "..";
                if (!safe || t.id.empty() || t.id.size() > 128 || t.title.empty() || t.title.size() > 180 ||
                    t.artist.empty() || t.artist.size() > 96 || !std::isfinite(db) || db < -60 || db > 0 ||
                    !ids.insert(t.id).second || !std::filesystem::is_regular_file(root / path))
                    throw std::runtime_error("Invalid/missing music entry " + t.id);
                t.gain = std::pow(10.f, db / 20.f);
                tracks.push_back(std::move(t));
            } catch (const std::exception& e) {
                warning += std::string(e.what()) + '\n';
            }
        }
    } catch (const std::exception& e) {
        warning += std::string("Music playlist unavailable: ") + e.what() + '\n';
    }
    return tracks;
}
void loadPlaylist(const std::filesystem::path& path, const std::vector<MusicTrack>& tracks,
                  Playlist& playlist, std::string& warning) {
    if (path.empty() || !std::filesystem::exists(path))
        return;
    try {
        const auto j = read(path);
        const auto ids = j.at("order").get<std::vector<std::string>>();
        const auto cursor = j.at("cursor").get<unsigned>();
        const auto rng = j.at("random").get<std::uint32_t>();
        if (j.at("version") != 1 || ids.size() > musicCapacity || cursor > ids.size() || !rng)
            throw std::runtime_error("Invalid music state");
        auto restored = playlist;
        restored.count = restored.cursor = 0;
        restored.random = rng;
        restored.last = musicCapacity;
        std::set<std::string> unique;
        std::array<bool, musicCapacity> seen{};
        for (unsigned n = 0; n < ids.size(); ++n) {
            if (!unique.insert(ids[n]).second)
                throw std::runtime_error("Duplicate music state ID");
            for (unsigned i = 0; i < tracks.size(); ++i)
                if (tracks[i].id == ids[n]) {
                    restored.order[restored.count++] = i;
                    seen[i] = true;
                    if (n < cursor)
                        ++restored.cursor;
                }
        }
        const auto last = j.value("last", std::string{});
        for (unsigned i = 0; i < tracks.size(); ++i)
            if (tracks[i].id == last)
                restored.last = i;
        // New content joins the unplayed tail; do not replay already consumed IDs.
        const unsigned begin = restored.count;
        for (unsigned i = 0; i < tracks.size(); ++i)
            if (!seen[i])
                restored.order[restored.count++] = i;
        for (unsigned i = restored.count; i > begin + 1; --i)
            std::swap(restored.order[i - 1], restored.order[begin + restored.roll() % (i - begin)]);
        playlist = restored;
    } catch (const std::exception& e) {
        warning += std::string("Music state ignored: ") + e.what() + '\n';
    }
}
bool savePlaylist(const std::filesystem::path& path, const std::vector<MusicTrack>& tracks, const Playlist& p,
                  std::string& warning) {
    if (path.empty() || tracks.empty())
        return true;
    try {
        Json order = Json::array();
        for (unsigned i = 0; i < p.count; ++i)
            order.push_back(tracks.at(p.order[i]).id);
        Json j{{"version", 1},
               {"order", order},
               {"cursor", p.cursor},
               {"random", p.random},
               {"last", p.last < tracks.size() ? tracks[p.last].id : ""}};
        std::filesystem::create_directories(path.parent_path());
        disk::writeDocument(path, j.dump(2) + "\n");
        return true;
    } catch (const std::exception& e) {
        warning += std::string(e.what()) + '\n';
        return false;
    }
}
} // namespace blobber::audio
