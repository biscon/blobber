#pragma once
#include "render/attack_visuals.hpp"
#include "render/npc_sprites.hpp"
namespace blobber {
std::size_t projectilePreparedBytes() noexcept;
void composeProjectiles(const ProjectilePlayer&, const PreparedNpcs&, const ActorWorld&, Player, int phase,
                        const std::array<float, viewWidth>& walls, const PickingBuffer*, Frame&);
} // namespace blobber
