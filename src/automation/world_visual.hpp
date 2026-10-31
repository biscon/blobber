#pragma once
#include "assets/world_assets.hpp"
#include "automation/session.hpp"
namespace blobber::automation {
struct WorldVisualContext {
    const world_assets::CpuAssets& assets;
    const std::vector<NpcTimings>& timings;
};
std::unique_ptr<Visibility> makeWorldVisual(void*, const Map&, const GameSession&);
} // namespace blobber::automation
