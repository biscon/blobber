#pragma once
#include "game/actors.hpp"
#include "game/dialogue.hpp"
#include "render/world_scene.hpp"
namespace blobber::world_render {
Camera dialogueCamera(const Scene&, const world_assets::CpuAssets&, GeometryView, const Camera& source,
                      const ActorState&, const DialogueCharacter&);
Camera interpolateCamera(const Camera&, const Camera&, float weight) noexcept;
} // namespace blobber::world_render
