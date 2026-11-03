#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
namespace blobber {
struct MusicBanner {
    const char* title = "";
    const char* secondLine = "";
    const char* artist = "";
    std::uint64_t revision = 0;
    float age = 6;
    bool visible = false;
};
struct MusicBannerMotion {
    float offset = 0, alpha = 0;
};
inline MusicBannerMotion musicBannerMotion(float age) noexcept {
    if (!std::isfinite(age) || age < 0 || age >= 6)
        return {};
    float offset = 0;
    if (age < .35f) {
        const float t = age / .35f - 1;
        const float ease = 1 + 2.70158f * t * t * t + 1.70158f * t * t;
        offset = -380 * (1 - ease);
    } else if (age > 5.5f) {
        const float t = (age - 5.5f) / .5f;
        offset = -380 * t * t;
    }
    return {offset, std::min(1.f, age / .15f)};
}
inline void musicBannerLetters(char (&out)[96], const char* text, float age, std::uint64_t salt) noexcept {
    std::snprintf(out, sizeof(out), "%s", text);
    const unsigned length = unsigned(std::strlen(out));
    const float progress = std::clamp((age - .25f) / .95f, 0.f, 1.f);
    const unsigned settled = unsigned(length * progress);
    constexpr char glyphs[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (unsigned i = settled; i < length; ++i) {
        if (out[i] == ' ' || out[i] == '-' || out[i] == ':')
            continue;
        std::uint32_t hash =
            std::uint32_t(salt) ^ (i * 0x9e3779b9u) ^ (unsigned(std::max(0.f, age) * 12) * 7919u);
        hash ^= hash >> 16;
        hash *= 0x7feb352du;
        hash ^= hash >> 15;
        out[i] = glyphs[hash % (sizeof(glyphs) - 1)];
    }
}
} // namespace blobber
