#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace blobber::audio {
constexpr unsigned musicCapacity = 64;
enum class MusicPolicy { Playlist, Once };
struct MusicTrack {
    std::string id, path, title, artist;
    float gain = 1;
};
struct Playlist {
    std::array<unsigned, musicCapacity> order{};
    unsigned count = 0, cursor = 0, last = musicCapacity;
    std::uint32_t random = 1;
    std::uint32_t roll() noexcept;
    void reset(unsigned size, std::uint32_t seed) noexcept;
    void shuffle() noexcept;
    unsigned next() noexcept;
};
// Decoder observations enter here; this controller never calls a platform API.
struct MusicInput {
    std::uint64_t encounter = 0;
    bool active = false, audible = true, paused = false, ended = false, bannerVisible = true;
    float remaining = 0, dt = 0;
};
class MusicTransport {
  public:
    Playlist playlist;
    unsigned track = musicCapacity;
    std::uint64_t revision = 0;
    float level = 0, bannerAge = 6;
    bool suspended = false;
    MusicPolicy policy = MusicPolicy::Playlist;
    void update(const MusicInput&) noexcept;
    void stop() noexcept;
    bool fading() const noexcept { return fading_; }

  private:
    std::uint64_t encounter_ = 0;
    float fadeLength_ = 0, fadeAge_ = 0;
    bool fading_ = false;
    bool completed_ = false;
    void start() noexcept;
};
std::vector<MusicTrack> loadMusicTracks(const std::filesystem::path& root, std::string& warning);
void loadPlaylist(const std::filesystem::path&, const std::vector<MusicTrack>&, Playlist&, std::string&);
bool savePlaylist(const std::filesystem::path&, const std::vector<MusicTrack>&, const Playlist&,
                  std::string&);
} // namespace blobber::audio
