#include "automation/world_visual.hpp"
#include "render/world_visibility.hpp"
namespace blobber::automation {
namespace {
class WorldVisual final : public Visibility {
  public:
    WorldVisual(WorldVisualContext& c, const Map& map, const GameSession& game)
        : scene_(world_render::prepare(map)), pipeline_(scene_, c.assets),
          animator_(game.actors(), c.timings) {
        sample(map, game);
    }
    const TargetSnapshot& sample(const Map&, const GameSession& game) override {
        animator_.setTime(game.timeMs(), &game.effects());
        pipeline_.update({game.geometry(), game.movement(), &game.actors(), &animator_.snapshot(),
                          &game.loot().snapshot()});
        const auto* picking = pipeline_.picking();
        return picking ? picking->targets() : empty_;
    }
    bool lootVisible(LootId id) const noexcept override { return pipeline_.lootVisible(id); }

  private:
    world_render::Scene scene_;
    world_render::VisibilityPipeline pipeline_;
    NpcAnimator animator_;
    TargetSnapshot empty_{};
};
} // namespace
std::unique_ptr<Visibility> makeWorldVisual(void* context, const Map& map, const GameSession& game) {
    return std::make_unique<WorldVisual>(*static_cast<WorldVisualContext*>(context), map, game);
}
} // namespace blobber::automation
