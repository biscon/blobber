#pragma once
#include "audio/music.hpp"
#include "audio/music_banner.hpp"
#include <raylib.h>
namespace blobber {
class MusicPlayer {
  public:
    void prepare(const std::filesystem::path& root, const std::filesystem::path& state, std::string& warning,
                 audio::MusicPolicy policy = audio::MusicPolicy::Playlist);
    void update(audio::MusicInput input, float volume);
    void detach() noexcept;
    void shutdown() noexcept;
    void persist(std::string& warning);
    MusicBanner banner() const noexcept;
    const audio::MusicTransport& transport() const noexcept { return transport_; }

  private:
    std::vector<audio::MusicTrack> tracks_;
    struct Title {
        std::array<char, 96> first{}, second{};
    };
    std::array<Title, audio::musicCapacity> titles_{};
    std::array<Music, audio::musicCapacity> streams_{};
    audio::MusicTransport transport_;
    std::filesystem::path statePath_;
    unsigned playing_ = audio::musicCapacity;
    std::uint64_t revision_ = 0;
    bool paused_ = false, showBanner_ = false;
};
} // namespace blobber
