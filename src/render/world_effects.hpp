#pragma once
#include "render/attack_visuals.hpp"
#include "render/world_visibility.hpp"
namespace blobber::world_render {
struct EffectPacket {
    std::array<Vec, 4> points{};
    std::array<float, 4> tint{1, 1, 1, 1};
    float depth = 0;
    std::uint16_t texture = 0;
    bool ground = false;
};
// NPC attacks/pulses own one glyph; four party casters can each contribute two
// additional Magic Missile glyphs beyond the retained record count.
constexpr unsigned effectCapacity = attackCapacity + rules::partySize * 2 + 16 * 2 + 9 + 4;
struct EffectFrame {
    std::array<EffectPacket, effectCapacity> packets{};
    unsigned count = 0;
    std::uint64_t revision = 0;
};
class EffectsPipeline {
  public:
    explicit EffectsPipeline(const world_assets::CpuAssets&);
    bool update(const FrameInput&, const WorldFrame&);
    const EffectFrame& frame() const noexcept { return frame_; }

  private:
    ProjectilePlayer shots_;
    EffectFrame frame_;
    std::array<unsigned, 10> textures_{};
    SpellArea area_{};
    std::uint64_t visibility_ = UINT64_MAX;
};
} // namespace blobber::world_render
