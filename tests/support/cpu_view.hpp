#pragma once
#include "render/world_effects.hpp"
namespace blobber::test {
// Test adapter calls the same world pipelines as graphical GameView, without a context.
struct CpuView {
    world_render::Scene scene;
    world_render::VisibilityPipeline visibility;
    world_render::EffectsPipeline effects;
    NpcAnimator animator;
    CpuView(const GameSession& game, const world_assets::CpuAssets& assets,
            const std::vector<NpcTimings>& timings)
        : scene(world_render::prepare(game.geometry().base())), visibility(scene, assets), effects(assets),
          animator(game.actors(), timings) {}
    void update(const GameSession& game, const SpellArea* preview = nullptr) {
        animator.setTime(game.timeMs(), &game.effects());
        SpellArea area = preview ? *preview : SpellArea{};
        area.ground = game.effects().ground();
        const world_render::FrameInput input{game.geometry(),
                                             game.movement(),
                                             &game.actors(),
                                             &animator.snapshot(),
                                             &game.loot().snapshot(),
                                             &game.doors(),
                                             &game.combat().attacks(),
                                             &area,
                                             game.timeMs()};
        visibility.update(input);
        effects.update(input, visibility.frame());
    }
};
} // namespace blobber::test
