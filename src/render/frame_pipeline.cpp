#include "render/frame_pipeline.hpp"
#include "render/transitions.hpp"

namespace blobber {
bool FramePipeline::update(const SceneRenderer& renderer, const Map& map, const MovementState& state,
                           const NpcWorld* npcs, const NpcAnimationSnapshot* animation) {
    const auto animationRevision = animation ? animation->revision : 0;
    const bool newContent = !ready_ || revision_ != state.revision ||
                            hadAnimation_ != (animation != nullptr) ||
                            animationRevision_ != animationRevision;
    if (newContent) {
        renderer.compose(map, state.player, destination_, npcs, animation);
        if (state.transition.active() && !isStrafe(state.transition.action))
            renderer.compose(map, state.transition.from, source_, npcs, animation);
        revision_ = state.revision;
        animationRevision_ = animationRevision;
        hadAnimation_ = animation != nullptr;
    }
    const bool moving = state.transition.active();
    const bool changed = newContent || moving || wasAnimating_;
    if (moving) {
        const auto& t = state.transition;
        if (isStrafe(t.action))
            renderer.composeStrafe(map, t.from, t.to, t.progress(), current_, npcs, animation);
        else
            composeTransition(source_, destination_, t.action, t.progress(), current_);
    } else if (changed)
        current_.copyFrom(destination_);
    ready_ = true;
    wasAnimating_ = moving;
    return changed;
}
} // namespace blobber
