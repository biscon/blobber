#pragma once
#include "render/world_visibility.hpp"
namespace blobber::world_render {
struct EffectFrame;
struct LocalLight {
    Vec position{};
    std::array<float, 3> color{};
    float radius = 1.5f, strength = 0;
    unsigned priority = 0;
};
constexpr unsigned spellLightCapacity = 8, spellAftermathCapacity = 64, spellParticleCapacity = 256;
struct SpellEffectDiagnostics {
    std::uint64_t casts = 0, remnants = 0, particles = 0, packets = 0, lights = 0;
};
class SpellEffectsPlayer {
  public:
    explicit SpellEffectsPlayer(const world_assets::CpuAssets& assets) : clips_(assets.publication.effects) {}
    bool update(const FrameInput&, const WorldFrame&) noexcept;
    void append(const FrameInput&, const WorldFrame&, EffectFrame&) noexcept;
    const SpellEffectDiagnostics& diagnostics() const noexcept { return diagnostics_; }

  private:
    struct Cast {
        SpellCastVisual cue{};
        bool resolved = false;
    };
    struct Remnant {
        SpellCastVisualId id{};
        SpellTargetVisual target{};
        rules::SpellKind spell = rules::SpellKind::Count;
        std::uint64_t impactMs = 0;
    };
    world_assets::EffectClips clips_;
    std::array<Cast, SpellVisualSnapshot::capacity> casts_{};
    std::array<Remnant, spellAftermathCapacity> remnants_{};
    std::bitset<NpcWorld::maxInstances> slow_;
    std::uint64_t world_ = 0, epoch_ = 0, seen_ = 0, revision_ = UINT64_MAX, now_ = 0;
    bool active_ = false;
    SpellEffectDiagnostics diagnostics_{};
};
} // namespace blobber::world_render
