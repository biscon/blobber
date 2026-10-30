#pragma once
#include "probe/assets.hpp"
#include "render/frame_pipeline.hpp"
#include <memory>
namespace blobber::probe {
class Legacy {
  public:
    Legacy(const Scene&, const Art&);
    ~Legacy();
    Legacy(const Legacy&) = delete;
    const Texture2D& render(const Scene&, const MovementState&, unsigned ms);
    std::size_t bytes = 0;
    bool cached = false;

  private:
    rules::Catalog catalog_;
    NpcWorld placements_;
    ActorWorld actors_;
    NpcAnimator animator_;
    std::unique_ptr<SceneRenderer> renderer_;
    FramePipeline pipeline_;
    Texture2D texture_{};
    MovementState previous_{};
    std::uint64_t revision_ = 0;
};
} // namespace blobber::probe
