#include "automation/visual.hpp"
#include "assets/preparation_profile.hpp"
#include "assets/prepared_assets.hpp"
#include "assets/prepared_loot.hpp"
#include "render/frame_pipeline.hpp"
namespace blobber::automation {
std::unique_ptr<SceneRenderer> loadVisualRenderer(const std::filesystem::path& assets,
                                                  const NpcDefinitions& defs) {
    const auto profile = loadPreparationProfile(assets / "preparation.json");
    validatePreparedPublication(assets / "prepared", profile, defs.definitionFingerprint);
    auto scene = loadPreparedScene(assets / "prepared/scene.bin", profile);
    auto loot = loadPreparedLoot(assets / "prepared/loot.bin");
    const auto bytes = sceneStorageBytes(scene) + loot.storageBytes() + projectilePreparedBytes();
    requirePreparationBytes(0, bytes, PreparationLimits{}.residentBytes);
    const auto remaining = PreparationLimits{}.residentBytes - bytes;
    auto npcs = loadPreparedNpcs(assets / "prepared/npcs.bin", defs.timings, defs.definitionFingerprint,
                                 profile, {remaining, remaining});
    return std::make_unique<SceneRenderer>(std::move(scene), std::move(npcs), defs.attachments,
                                           std::move(loot));
}
namespace {
class Visual final : public Visibility {
  public:
    Visual(VisualContext& c, const GameSession& g) : context_(c), animator_(g.actors(), c.timings) {}
    const TargetSnapshot& sample(const Map&, const GameSession& g) override {
        animator_.setTime(g.timeMs(), &g.effects());
        SpellArea area;
        area.ground = g.effects().ground();
        const bool changed =
            frames_.update(context_.renderer, g.geometry(), g.movement(), &g.actors(), &animator_.snapshot(),
                           &g.combat().attacks(), g.timeMs(), &area, &g.loot().snapshot());
        if (!changed)
            return targets_;
        loot_ = {};
        if (const auto* p = frames_.picking()) {
            targets_ = p->targets();
            for (int y = 0; y < viewHeight; ++y)
                for (int x = 0; x < viewWidth; ++x)
                    if (auto id = p->pickLoot(x, y))
                        loot_[id->slot] = *id;
        } else
            targets_ = {};
        return targets_;
    }
    bool lootVisible(LootId id) const noexcept override {
        return id.slot < loot_.size() && loot_[id.slot] == id;
    }

  private:
    VisualContext& context_;
    NpcAnimator animator_;
    FramePipeline frames_;
    TargetSnapshot targets_;
    std::array<LootId, lootCapacity> loot_{};
};
} // namespace
std::unique_ptr<Visibility> makeVisual(void* context, const Map&, const GameSession& g) {
    return std::make_unique<Visual>(*static_cast<VisualContext*>(context), g);
}
} // namespace blobber::automation
