#pragma once
#include "render/world_effects.hpp"
namespace blobber::world_render {
constexpr unsigned lightCellsOffset = 24, lightDoorsOffset = 224;
struct SpellLightData {
    std::array<std::array<float, 4>, 1024> texels{};
    unsigned count = 0;
};
SpellLightData prepareSpellLights(const FrameInput&, const WorldFrame&, const EffectFrame&) noexcept;
bool spellLightVisible(const SpellLightData&, unsigned, Vec receiver, Vec normal = {}) noexcept;
} // namespace blobber::world_render
