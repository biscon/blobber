#pragma once
#include "platform/presenter.hpp"
#include "platform/world_renderer.hpp"
namespace blobber {
void captureDoors(const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&, WorldRenderer&,
                  Presenter&, const std::filesystem::path&);
}
