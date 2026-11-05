#include "render/melee_swipe.hpp"
#include <cmath>
namespace blobber::world_render {
SwipeRibbon sampleSwipe(SwipePoint center, float width, float height, bool mirrored, float variation,
                        unsigned age, const MeleeSwipeTuning& tune) noexcept {
    SwipeRibbon out;
    if (!age || age >= tune.durationMs || width <= 0 || height <= 0)
        return out;
    const float head = std::min(1.f, float(age) / tune.revealMs);
    const float tail =
        age <= tune.retreatMs ? 0.f : float(age - tune.retreatMs) / (tune.durationMs - tune.retreatMs);
    const float strength = std::min(1.f, float(age) / tune.revealMs) * (1 - tail);
    out.alpha = 1 - tail;
    const float span = width * tune.span, bend = span * tune.curvature;
    const float angle = (tune.angleDegrees + variation) * pi / 180;
    const float c = std::cos(angle), s = std::sin(angle), mirror = mirrored ? -1.f : 1.f;
    for (unsigned i = 0; i <= swipeSegments; ++i) {
        const float v = float(i) / swipeSegments, u = tail + (head - tail) * v;
        // Subtract the midpoint bulge so B(.5) is exactly the contact anchor.
        const float x = span * (u - .5f), y = 2 * bend * u * (1 - u) - bend * .5f;
        const float dx = span, dy = 2 * bend * (1 - 2 * u);
        const float inv = 1 / std::sqrt(dx * dx + dy * dy);
        // sin(pi * v^1.6) peaks at v=.648, behind the advancing endpoint.
        const float profile = (i == 0 || i == swipeSegments) ? 0.f : std::sin(pi * std::pow(v, 1.6f));
        const float half = height * tune.thickness * .5f * strength * profile;
        for (unsigned edge = 0; edge < swipeColumns; ++edge) {
            const float side = swipeCrossSection[edge];
            const float px = x - dy * inv * half * side, py = y + dx * inv * half * side;
            out.vertices[i * swipeColumns + edge] = {center.x + mirror * (px * c - py * s),
                                                     center.y + px * s + py * c};
        }
    }
    return out;
}
bool MeleeSwipePlayer::update(const FrameInput& in, const WorldFrame& world) noexcept {
    const auto* snapshot = in.attacks;
    const auto identity = snapshot ? snapshot->world : 0;
    const auto revision = snapshot ? snapshot->revision : 0;
    if (world_ == identity && revision_ == revision && now_ == in.now)
        return false;
    const bool had = frame_.count != 0;
    frame_.count = 0;
    if (world_ != identity) {
        records_ = {};
        sequence_ = 0;
    }
    world_ = identity;
    revision_ = revision;
    now_ = in.now;
    if (!snapshot)
        return had;
    // Order new identities by serial so simultaneous publication has stable alternation.
    std::array<const AttackVisual*, rules::partySize> attacks{};
    unsigned count = 0;
    for (unsigned i = 0; i < rules::partySize; ++i) {
        const auto& a = snapshot->attacks[NpcWorld::maxInstances + i];
        if (a.stage == AttackStage::Empty || a.incoming || a.ranged || a.spell != rules::SpellKind::Count ||
            a.character.index >= rules::partySize)
            continue;
        unsigned j = count++;
        while (j && attacks[j - 1]->id.serial > a.id.serial) {
            attacks[j] = attacks[j - 1];
            --j;
        }
        attacks[j] = &a;
    }
    for (unsigned i = 0; i < count; ++i) {
        const auto& a = *attacks[i];
        auto& record = records_[a.character.index];
        if (record.id.world != a.id.world || record.id.serial != a.id.serial) {
            record = {};
            record.id = a.id;
            record.mirrored = (++sequence_ % 2) == 0;
            const auto hash = a.id.serial * UINT64_C(0x9e3779b97f4a7c15);
            record.variation = (float((hash >> 32) % 2001) / 1000.f - 1) * meleeSwipeTuning.variationDegrees;
            // Late first observation cannot recover acceptance-time geometry safely.
            if (in.now < a.impactMs && in.actors && in.actors->actor(a.enemy)) {
                for (unsigned j = 0; j < world.count; ++j) {
                    const auto& b = world.billboards[j];
                    if (b.id != a.enemy.slot || !b.alive)
                        continue;
                    const auto torso = attachment(world.camera, b, b.torso);
                    const auto p = project(world.camera, torso.position);
                    if (!torso.valid || p.depth <= nearPlane || !validBodyBounds(b.bodyBounds))
                        break;
                    const float scale = focal() / p.depth;
                    record.width = (b.bodyCanvasWidth > 0 ? b.bodyCanvasWidth : b.width) *
                                   (b.bodyBounds.right - b.bodyBounds.left) * scale;
                    record.height = b.height * (b.bodyBounds.bottom - b.bodyBounds.top) * scale;
                    record.center = {p.x, p.y};
                    if (!a.hit)
                        record.center.x +=
                            (record.mirrored ? -1 : 1) * record.width * meleeSwipeTuning.missOffset;
                    record.valid =
                        std::isfinite(p.x) && std::isfinite(p.y) && record.width > 0 && record.height > 0;
                    break;
                }
            }
        }
        if (a.stage == AttackStage::Canceled || a.targetGone)
            record.valid = false;
        const auto start =
            a.impactMs >= meleeSwipeTuning.contactMs ? a.impactMs - meleeSwipeTuning.contactMs : 0;
        if (!record.valid || in.now <= start || in.now - start >= meleeSwipeTuning.durationMs)
            continue;
        frame_.ribbons[frame_.count++] =
            sampleSwipe(record.center, record.width, record.height, record.mirrored, record.variation,
                        unsigned(in.now - start));
    }
    return had || frame_.count != 0;
}
} // namespace blobber::world_render
