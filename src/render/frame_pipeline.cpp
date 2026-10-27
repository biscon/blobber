#include "render/frame_pipeline.hpp"
#include "render/transitions.hpp"
#include <stdexcept>

namespace blobber {
bool FramePipeline::update(const SceneRenderer& renderer, const Map& map, const MovementState& state,
                           const ActorWorld* npcs, const NpcAnimationSnapshot* animation,
                           const AttackVisualSnapshot* attacks, std::uint64_t now) {
    if (attacks && (!npcs || attacks->world != npcs->snapshot().world))
        throw std::invalid_argument("Attack snapshot belongs to another actor world");
    const bool effectsChanged = projectiles_.update(attacks, now);
    // Validate even on the cached-frame path; equal animation revisions do not
    // make snapshots from different actor owners interchangeable.
    if (animation &&
        (!npcs || animation->world != npcs->snapshot().world ||
         animation->actorRevision != npcs->snapshot().revision || animation->count != npcs->snapshot().count))
        throw std::invalid_argument("NPC animation snapshot belongs to another world or revision");
    const auto animationRevision = animation ? animation->revision : 0;
    const auto actorRevision = npcs ? npcs->snapshot().revision : 0;
    const auto actorWorld = npcs ? npcs->snapshot().world : 0;
    const bool newContent = effectsChanged || !ready_ || revision_ != state.revision ||
                            actorRevision_ != actorRevision || actorWorld_ != actorWorld ||
                            hadAnimation_ != (animation != nullptr) ||
                            animationRevision_ != animationRevision;
    if (newContent) {
        renderer.compose(map, state.player, destination_, npcs, animation, &picking_, &projectiles_);
        if (state.transition.active() && !isStrafe(state.transition.action))
            renderer.compose(map, state.transition.from, source_, npcs, animation,
                             projectiles_.size() ? &effectPicking_ : nullptr, &projectiles_);
        revision_ = state.revision;
        actorRevision_ = actorRevision;
        actorWorld_ = actorWorld;
        animationRevision_ = animationRevision;
        hadAnimation_ = animation != nullptr;
    }
    const bool moving = state.transition.active();
    const bool changed = newContent || moving || wasAnimating_;
    if (moving) {
        const auto& t = state.transition;
        if (isStrafe(t.action))
            renderer.composeStrafe(map, t.from, t.to, t.progress(), current_, npcs, animation,
                                   projectiles_.size() ? &effectPicking_ : nullptr, &projectiles_);
        else
            composeTransition(source_, destination_, t.action, t.progress(), current_);
    } else if (changed)
        current_.copyFrom(destination_);
    ready_ = true;
    wasAnimating_ = moving;
    return changed;
}
} // namespace blobber
