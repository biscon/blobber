#pragma once
#include <array>
#include <cstdint>
namespace blobber::world_assets {
enum class EffectClipKind : std::uint8_t {
    FireFlight,
    FireImpact,
    FireRemnant,
    FrostFlight,
    FrostImpact,
    FrostCoating,
    Cone,
    Particles,
    ForceFlight,
    ForceImpact,
    RayFlight,
    RayImpact,
    RadiantFlight,
    RadiantImpact,
    SacredFlame,
    GuidingCoating,
    Electricity,
    Necrotic,
    Acid,
    Poison,
    Thunderwave,
    Shatter,
    ArcaneParticles,
    AcidFlecks,
    Count
};
constexpr unsigned effectClipCount = unsigned(EffectClipKind::Count);
struct EffectClip {
    std::uint16_t body = UINT16_MAX, emission = UINT16_MAX;
    unsigned count = 0, frameMs = 80;
    std::array<std::array<float, 4>, 8> frames{}; // normalized x/y/width/height
    std::array<float, 2> anchor{.5f, .5f};
};
using EffectClips = std::array<EffectClip, effectClipCount>;
inline void mapEffectSheet(EffectClips& clips, unsigned first, unsigned body, unsigned emission) {
    for (unsigned row = 0; row < 8; ++row) {
        auto& c = clips[first + row];
        c.body = std::uint16_t(body);
        c.emission = std::uint16_t(emission);
        c.count = 8;
        for (unsigned col = 0; col < 8; ++col)
            c.frames[col] = {float(col * 128 + 8) / 1024, float(row * 128 + 8) / 1024, 112.f / 1024,
                             112.f / 1024};
    }
}
inline EffectClips atlasEffectClips(unsigned body, unsigned emission, unsigned tile = 128,
                                    unsigned gutter = 8) {
    EffectClips result{};
    const float extent = float(tile * 8), size = float(tile - 2 * gutter);
    for (unsigned row = 0; row < effectClipCount; ++row) {
        auto& c = result[row];
        c.body = std::uint16_t(body);
        c.emission = std::uint16_t(emission);
        c.count = 8;
        for (unsigned col = 0; col < 8; ++col)
            c.frames[col] = {float(col * tile + gutter) / extent, float((row % 8) * tile + gutter) / extent,
                             size / extent, size / extent};
    }
    return result;
}
} // namespace blobber::world_assets
