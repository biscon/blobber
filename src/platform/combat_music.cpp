#include "platform/combat_music.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
namespace blobber {
void CombatMusic::prepare(const std::filesystem::path& root, const std::filesystem::path& state,
                          std::string& warning) {
    statePath_ = state;
    auto candidates = audio::loadMusicTracks(root, warning);
    tracks_.reserve(audio::musicCapacity);
    for (auto& track : candidates) {
        const auto path = (root / track.path).string();
        auto stream = LoadMusicStream(path.c_str());
        const float duration = IsMusicValid(stream) ? GetMusicTimeLength(stream) : 0;
        if (!std::isfinite(duration) || duration <= 0) {
            warning += "Cannot decode music " + track.path + '\n';
            if (IsMusicValid(stream))
                UnloadMusicStream(stream);
            continue;
        }
        stream.looping = false;
        // Warm raylib's lazy PCM scratch/decoder at the preparation boundary, silently.
        SetMusicVolume(stream, 0);
        PlayMusicStream(stream);
        UpdateMusicStream(stream);
        StopMusicStream(stream);
        auto& title = titles_[tracks_.size()];
        auto split = track.title.size() <= 42 ? track.title.size() : track.title.rfind(' ', 42);
        if (split == std::string::npos)
            split = 42;
        std::snprintf(title.first.data(), title.first.size(), "%.*s", int(split), track.title.c_str());
        if (split < track.title.size())
            std::snprintf(title.second.data(), title.second.size(), "%s",
                          track.title.c_str() + split + (track.title[split] == ' '));
        streams_[tracks_.size()] = stream;
        tracks_.push_back(std::move(track));
    }
    transport_.playlist.reset(unsigned(tracks_.size()),
                              std::uint32_t(std::chrono::steady_clock::now().time_since_epoch().count()));
    audio::loadPlaylist(statePath_, tracks_, transport_.playlist, warning);
}
void CombatMusic::update(audio::MusicInput input, float volume) {
    input.audible &= std::isfinite(volume) && volume > 0;
    if (playing_ < tracks_.size()) {
        const auto stream = streams_[playing_];
        input.remaining = std::max(0.f, GetMusicTimeLength(stream) - GetMusicTimePlayed(stream));
        // StopMusicStream rewinds on EOF: capture remaining time before servicing it.
        if (!paused_)
            UpdateMusicStream(stream);
        input.ended = !paused_ && !IsMusicStreamPlaying(stream);
    }
    transport_.update(input);
    if (playing_ != transport_.track || revision_ != transport_.revision) {
        if (playing_ < tracks_.size())
            StopMusicStream(streams_[playing_]);
        playing_ = transport_.track;
        revision_ = transport_.revision;
        paused_ = false;
        if (playing_ < tracks_.size()) {
            const auto stream = streams_[playing_];
            StopMusicStream(stream); // Explicit decoder rewind even when reusing a one-song playlist.
            SetMusicVolume(stream, volume * tracks_[playing_].gain * transport_.level);
            PlayMusicStream(stream);
            UpdateMusicStream(stream);
        }
    }
    if (playing_ < tracks_.size()) {
        const auto stream = streams_[playing_];
        if (paused_ != transport_.suspended) {
            if (transport_.suspended)
                PauseMusicStream(stream);
            else
                ResumeMusicStream(stream);
            paused_ = transport_.suspended;
        }
        SetMusicVolume(stream, volume * tracks_[playing_].gain * transport_.level);
    }
    showBanner_ = input.audible && !input.paused && input.bannerVisible;
}
MusicBanner CombatMusic::banner() const noexcept {
    if (playing_ >= tracks_.size())
        return {};
    return {titles_[playing_].first.data(),
            titles_[playing_].second.data(),
            tracks_[playing_].artist.c_str(),
            revision_,
            transport_.bannerAge,
            showBanner_ && transport_.bannerAge < 6};
}
void CombatMusic::detach() noexcept {
    if (playing_ < tracks_.size())
        StopMusicStream(streams_[playing_]);
    playing_ = audio::musicCapacity;
    paused_ = showBanner_ = false;
    transport_.stop();
}
void CombatMusic::persist(std::string& warning) {
    audio::savePlaylist(statePath_, tracks_, transport_.playlist, warning);
}
void CombatMusic::shutdown() noexcept {
    detach();
    for (unsigned i = 0; i < tracks_.size(); ++i)
        UnloadMusicStream(streams_[i]);
    tracks_.clear();
    streams_ = {};
    transport_ = {};
}
} // namespace blobber
