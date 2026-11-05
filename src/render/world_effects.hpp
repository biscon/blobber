#pragma once
#include "render/attack_visuals.hpp"
#include "render/melee_swipe.hpp"
#include "render/spell_effects.hpp"
#include "render/world_visibility.hpp"
namespace blobber::world_render {
struct EffectPacket {
    std::array<Vec, 4> points{};
    std::array<float, 4> tint{1, 1, 1, 1};
    float depth = 0;
    std::uint16_t texture = 0;
    bool ground = false;
    bool additive = false;
    std::array<float, 4> uv{0, 0, 1, 1};
    std::array<Cell, 3> cells{};
    Cell origin{};
    unsigned footprint = 0; // bit 3 enables cone clipping, low bits select eligible cells
};
// NPC attacks/pulses own one glyph; four party casters can each contribute two
// additional Magic Missile glyphs beyond the retained record count.
constexpr unsigned effectCapacity = 1024;
struct EffectFrame {
    SwipeFrame swipes;
    std::array<LocalLight, spellLightCapacity> lights{};
    unsigned lightCount = 0;
    std::array<float, NpcWorld::maxInstances> frost{};
    std::uint16_t frostTexture = UINT16_MAX;
    std::array<float, 4> frostUv{};

    std::array<EffectPacket, effectCapacity> packets{};
    unsigned count = 0;
    std::uint64_t revision = 0;
};
class EffectsPipeline {
  public:
    explicit EffectsPipeline(const world_assets::CpuAssets&);
    bool update(const FrameInput&, const WorldFrame&);
    const EffectFrame& frame() const noexcept { return frame_; }
    const SpellEffectDiagnostics& diagnostics() const noexcept { return spells_.diagnostics(); }

  private:
    ProjectilePlayer shots_;
    MeleeSwipePlayer swipes_;
    SpellEffectsPlayer spells_;
    EffectFrame frame_;
    std::array<unsigned, 10> textures_{};
    SpellArea area_{};
    std::uint64_t visibility_ = UINT64_MAX, fireTick_ = UINT64_MAX;
    unsigned flame_ = UINT16_MAX;
};
} // namespace blobber::world_render
