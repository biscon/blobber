#include "render/dialogue_camera.hpp"
#include "game/actors.hpp"
#include <algorithm>
#include <cmath>
namespace blobber::world_render {
namespace {
Vec sub(Vec a, Vec b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec cross(Vec a, Vec b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
float dot(Vec a, Vec b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
float hit(Vec origin, Vec travel, Vec a, Vec b, Vec c) {
    const auto e1 = sub(b, a), e2 = sub(c, a), p = cross(travel, e2);
    const float determinant = dot(e1, p);
    if (std::abs(determinant) < 1e-7f)
        return 1;
    const auto t = sub(origin, a);
    const float u = dot(t, p) / determinant;
    if (u < 0 || u > 1)
        return 1;
    const auto q = cross(t, e1);
    const float v = dot(travel, q) / determinant;
    if (v < 0 || u + v > 1)
        return 1;
    const float distance = dot(e2, q) / determinant;
    return distance > .0001f && distance < 1 ? distance : 1;
}
float pathLimit(const Scene& scene, Vec source, Vec destination) {
    const auto travel = sub(destination, source);
    const float length = std::sqrt(dot(travel, travel));
    float limit = 1;
    const std::array<Vec, 5> offsets{
        {{0, 0, 0}, {.035f, .035f, 0}, {-.035f, .035f, 0}, {0, -.035f, .035f}, {0, -.035f, -.035f}}};
    for (const auto offset : offsets) {
        const Vec origin{source.x + offset.x, source.y + offset.y, source.z + offset.z};
        for (std::size_t i = 0; i + 2 < scene.indices.size(); i += 3)
            limit = std::min(limit, hit(origin, travel, scene.vertices[scene.indices[i]].position,
                                        scene.vertices[scene.indices[i + 1]].position,
                                        scene.vertices[scene.indices[i + 2]].position));
    }
    return limit < 1 && length > 0 ? std::max(0.f, limit - .05f / length) : 1;
}
} // namespace
Camera interpolateCamera(const Camera& source, const Camera& target, float weight) noexcept {
    if (weight <= 0)
        return source;
    if (weight >= 1)
        return target;
    auto out = source;
    out.position = {source.position.x + (target.position.x - source.position.x) * weight,
                    source.position.y + (target.position.y - source.position.y) * weight,
                    source.position.z + (target.position.z - source.position.z) * weight};
    float delta = std::remainder(target.yaw - source.yaw, 2 * pi);
    out.yaw = source.yaw + delta * weight;
    out.forward = {std::sin(out.yaw), 0, -std::cos(out.yaw)};
    out.right = {std::cos(out.yaw), 0, std::sin(out.yaw)};
    out.horizon = source.horizon + (target.horizon - source.horizon) * weight;
    return out;
}
Camera dialogueCamera(const Scene& scene, const world_assets::CpuAssets& assets, GeometryView geometry,
                      const Camera& source, const ActorState& actor, const DialogueFraming& character) {
    const auto& npc = actor.npc;
    const float base = elevation(geometry.surface(npc.cell).floor);
    const auto direction = forward(npc.facing);
    const Vec feet{npc.cell.x + .25f + npc.subcell.x * .5f, base, npc.cell.y + .25f + npc.subcell.y * .5f};
    const auto& appearance = assets.publication.appearances.at(npc.type.index);
    const auto texture = appearance.clips[0].textures[0][0];
    const auto anchor = assets.publication.anchors.at(texture);
    const auto canvas = assets.publication.textures.at(texture).canvas;
    const auto bounds = assets.bounds.at(texture);
    const unsigned inkTop = bounds.bottom > bounds.top ? bounds.top : 0;
    const float croppedTop = canvas.height ? float(canvas.y + inkTop) / canvas.height : 0;
    const float bodyTop = std::max(croppedTop, appearance.attachments.bodyBounds[0].top);
    const float top = base + appearance.height * (anchor.y - bodyTop);
    // Prepared sprites have no rig: body-relative eyes are the generic default.
    const float eye = character.eyeHeight > 0 ? base + character.eyeHeight : base + .92f * (top - base);
    auto target = source;
    target.position = {feet.x + direction.x * character.distance, eye,
                       feet.z + direction.y * character.distance};
    const auto travel = sub(target.position, source.position);
    const float limit = pathLimit(scene, source.position, target.position);
    if (limit < 1) {
        target.position = {source.position.x + travel.x * limit, source.position.y + travel.y * limit,
                           source.position.z + travel.z * limit};
    }
    target.yaw = std::atan2(feet.x - target.position.x, -(feet.z - target.position.z));
    target.forward = {std::sin(target.yaw), 0, -std::cos(target.yaw)};
    target.right = {std::cos(target.yaw), 0, std::sin(target.yaw)};
    const float depth = std::max(.1f, dot(sub(feet, target.position), target.forward));
    target.horizon = std::clamp((14 + focal() * (top - target.position.y) / depth) / viewHeight, .05f, .6f);
    return target;
}
void DialogueCameraController::sample(std::uint64_t clock) noexcept {
    elapsedMs_ = clock >= startMs_ ? clock - startMs_ : 0;
    const float t = durationMs_ ? std::min(1.f, float(elapsedMs_) / durationMs_) : 1;
    current_ = interpolateCamera(from_, to_, t * t * (3 - 2 * t));
}
void DialogueCameraController::transition(const Scene& scene, const Camera& destination, unsigned duration,
                                          std::uint64_t clock) {
    from_ = current_;
    to_ = destination;
    startMs_ = clock;
    // Both endpoints come from conservative framing. An obstructed connecting
    // segment is a cut, never a traversal through the obstacle.
    durationMs_ =
        pathLimit(scene, from_.position, to_.position) < 1 ? 0 : std::max(dialogueMinimumPanMs, duration);
    sample(clock);
}
std::optional<Camera> DialogueCameraController::update(const Scene& scene,
                                                       const world_assets::CpuAssets& assets,
                                                       GeometryView geometry, const Camera& exploration,
                                                       const ActorState* primary, const ActorState* target,
                                                       const DialoguePlayer& player) {
    if (!player.active() || !primary || !player.character()) {
        active_ = false;
        return {};
    }
    const auto clock = player.clockMs();
    const auto& character = *player.character();
    if (!active_) {
        active_ = true;
        leaving_ = false;
        exploration_ = current_ = exploration;
        primary_ = dialogueCamera(scene, assets, geometry, exploration_, *primary, character);
        target_ = character.primary;
        serial_ = 0;
        transition(scene, primary_, player.transitionMs(), 0);
    }
    sample(clock);
    if (player.leaving()) {
        if (!leaving_) {
            leaving_ = true;
            transition(scene, exploration_, player.transitionMs(), clock);
        }
    } else if (player.cuesReady() && serial_ != player.beatSerial()) {
        serial_ = player.beatSerial();
        const auto& beat = *player.beat();
        if (target && (target_ != beat.target || beat.durationMs)) {
            const auto destination = beat.target == character.primary
                                         ? primary_
                                         : dialogueCamera(scene, assets, geometry, exploration_, *target,
                                                          character.participants[beat.target]);
            transition(scene, destination, beat.durationMs.value_or(dialogueMinimumPanMs), clock);
            target_ = beat.target;
        }
    } else if (!player.beat() && target_ != character.primary) {
        transition(scene, primary_, dialogueMinimumPanMs, clock);
        target_ = character.primary;
    }
    return current_;
}
} // namespace blobber::world_render
