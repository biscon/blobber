#include "audio/music.hpp"
#include <algorithm>
#include <cmath>
namespace blobber::audio {
std::uint32_t Playlist::roll() noexcept {
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    return random;
}
void Playlist::reset(unsigned size, std::uint32_t seed) noexcept {
    count = std::min(size, musicCapacity);
    random = seed ? seed : 1;
    last = musicCapacity;
    shuffle();
}
void Playlist::shuffle() noexcept {
    for (unsigned i = 0; i < count; ++i)
        order[i] = i;
    for (unsigned i = count; i > 1; --i)
        std::swap(order[i - 1], order[roll() % i]);
    if (count > 1 && order[0] == last)
        std::swap(order[0], order[1]);
    cursor = 0;
}
unsigned Playlist::next() noexcept {
    if (!count)
        return musicCapacity;
    if (cursor == count)
        shuffle();
    return last = order[cursor++];
}
void MusicTransport::stop() noexcept {
    track = musicCapacity;
    level = 0;
    bannerAge = 6;
    fading_ = false;
    suspended = false;
    completed_ = false;
}
void MusicTransport::start(bool bannerVisible) noexcept {
    track = playlist.next();
    if (track == musicCapacity)
        return;
    ++revision;
    level = 1;
    bannerAge = bannerVisible ? 0.f : 6.f;
    fading_ = false;
}
void MusicTransport::update(const MusicInput& in) noexcept {
    const float dt = std::isfinite(in.dt) ? std::max(0.f, in.dt) : 0;
    if (in.encounter != encounter_) {
        stop();
        encounter_ = in.encounter;
    }
    // Hidden screens discard announcements, even if playback is suspended.
    // Returning to gameplay must never announce a song that already started.
    if (!in.bannerVisible)
        bannerAge = 6;
    // End-of-combat always wins over end-of-track, including while muted.
    if (!in.active && track != musicCapacity) {
        if (!in.audible || in.ended) {
            stop();
            return;
        }
        if (!fading_) {
            fading_ = true;
            fadeLength_ = std::isfinite(in.remaining) ? std::clamp(in.remaining, 0.f, 5.f) : 0;
            fadeAge_ = 0;
        }
    }
    suspended = in.paused || !in.audible;
    if (suspended)
        return;
    if (policy == MusicPolicy::Once && in.active && in.ended && track != musicCapacity) {
        stop();
        completed_ = true;
        return;
    }
    if (fading_) {
        fadeAge_ += dt;
        if (fadeLength_ <= 0 || fadeAge_ >= fadeLength_ || in.ended) {
            stop();
            return;
        }
        level = 1 - fadeAge_ / fadeLength_;
    } else if (in.active && !completed_ && (track == musicCapacity || in.ended)) {
        start(in.bannerVisible);
        return;
    }
    if (track != musicCapacity && in.bannerVisible)
        bannerAge = std::min(6.f, bannerAge + dt);
}
} // namespace blobber::audio
