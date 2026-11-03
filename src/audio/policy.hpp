#pragma once
#include "world/fixtures.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace blobber::audio {
struct Variation {
    std::array<unsigned, 16> bag{};
    unsigned size = 0, next = 0, last = 16;
    std::uint32_t random = 0x6d2b79f5;
    std::uint32_t roll() noexcept {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        return random;
    }
    float unit() noexcept { return float(roll() >> 8) / 16777216.f; }
    unsigned select(unsigned n) noexcept {
        if (!n || n > 16)
            return 16;
        if (size != n) {
            size = n;
            next = n;
            last = 16;
        }
        if (next == n) {
            for (unsigned i = 0; i < n; ++i)
                bag[i] = i;
            for (unsigned i = n - 1; i > 0; --i)
                std::swap(bag[i], bag[roll() % (i + 1)]);
            if (n > 1 && bag[0] == last)
                std::swap(bag[0], bag[1]);
            next = 0;
        }
        return last = bag[next++];
    }
};
struct Mix {
    float gain = 0, pan = 0;
};
inline Mix spatial(float dx, float dz, float rx, float rz, float range) noexcept {
    const float distance = std::sqrt(dx * dx + dz * dz);
    if (!std::isfinite(distance) || range <= 1 || distance >= range)
        return {};
    const float t = std::clamp((distance / range - .8f) / .2f, 0.f, 1.f);
    return {(1 - t * t * (3 - 2 * t)) / std::max(1.f, distance),
            distance > 1e-6f ? std::clamp((dx * rx + dz * rz) / distance, -1.f, 1.f) : 0};
}
bool unobstructed(GeometryView, float, float, float, float) noexcept;
struct Handle {
    unsigned slot = 64;
    std::uint64_t generation = 0, owner = 0;
};
struct Voice {
    bool active = false, paused = false, interface = false;
    unsigned priority = 0, sample = 0, alias = 0, cue = 0;
    std::uint64_t generation = 1, sequence = 0;
    float gain = 0, pitch = 1, x = 0, z = 0, range = 6;
    bool positional = false;
};
inline unsigned chooseVoice(const std::array<Voice, 64>& voices, unsigned priority) noexcept {
    const unsigned limit = priority >= 3 ? 64 : 56;
    for (unsigned i = 0; i < limit; ++i)
        if (!voices[i].active && voices[i].generation)
            return i;
    unsigned victim = 64;
    for (unsigned i = 0; i < limit; ++i) {
        const auto& v = voices[i];
        if (v.generation == UINT64_MAX || v.priority > priority || v.priority >= 4)
            continue;
        if (victim == 64 || v.priority < voices[victim].priority ||
            (v.priority == voices[victim].priority &&
             (v.gain < voices[victim].gain ||
              (v.gain == voices[victim].gain && v.sequence < voices[victim].sequence))))
            victim = i;
    }
    return victim;
}
struct Settings {
    float musicVolume = .5f;
    bool musicEnabled = true;
    std::array<float, 5> values{{.7f, .8f, .35f, .5f, .45f}};
};
} // namespace blobber::audio
