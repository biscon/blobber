#pragma once
#include "app/options.hpp"
#include "platform/presenter.hpp"
#include "platform/world_renderer.hpp"
namespace blobber {
void benchmarkWorldInteractions(const Options&, GpuResources&, WorldRenderer&, const std::filesystem::path&);
int runWorldInteractionPreview(const Options&);
void captureWorldInteractions(const Options&, GpuResources&, WorldRenderer&, Presenter&,
                              const std::filesystem::path&);
} // namespace blobber
