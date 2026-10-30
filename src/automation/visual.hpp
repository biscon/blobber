#pragma once
#include "automation/session.hpp"
#include "render/scene_renderer.hpp"
namespace blobber::automation {
std::unique_ptr<SceneRenderer> loadVisualRenderer(const std::filesystem::path&, const NpcDefinitions&);
struct VisualContext {
    const SceneRenderer& renderer;
    const std::vector<NpcTimings>& timings;
};
std::unique_ptr<Visibility> makeVisual(void*, const Map&, const GameSession&);
} // namespace blobber::automation
