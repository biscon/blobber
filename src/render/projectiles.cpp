#include "render/projectiles.hpp"
#include "render/lighting.hpp"
#include <algorithm>
#include <cmath>

namespace blobber {
bool ProjectilePlayer::update(const AttackVisualSnapshot* snapshot, std::uint64_t now) noexcept {
    const auto oldCount = count_;
    const bool changed = world_ != (snapshot ? snapshot->world : 0) ||
                         revision_ != (snapshot ? snapshot->revision : 0) || (count_ && now != now_);
    count_ = 0;
    now_ = now;
    world_ = snapshot ? snapshot->world : 0;
    revision_ = snapshot ? snapshot->revision : 0;
    if (snapshot && capacity_) {
        const auto add = [&](const AttackVisual& v) {
            if (v.stage == AttackStage::Empty || v.stage == AttackStage::Canceled ||
                now < (v.ranged ? v.releaseMs : v.impactMs) || now >= v.impactMs + 120)
                return;
            if (!v.ranged && v.stage != AttackStage::Resolved)
                return;
            if (count_ < capacity_)
                shots_[count_++] = v;
            else {
                auto oldest = shots_.begin();
                for (auto i = shots_.begin() + 1; i != shots_.begin() + count_; ++i)
                    if (i->id.serial < oldest->id.serial)
                        oldest = i;
                if (v.id.serial > oldest->id.serial)
                    *oldest = v;
            }
        };
        for (const auto& v : snapshot->attacks)
            add(v);
        for (const auto& v : snapshot->spellExtras)
            add(v);
        for (const auto& v : snapshot->spellPulses)
            add(v);
    }
    return changed || oldCount != count_;
}
namespace {
constexpr std::array<int, 8> sizes{3, 5, 7, 10, 14, 20, 28, 40};
constexpr std::array<float, 16> cosine{1,  .92388f,  .707107f,  .382683f,  0, -.382683f, -.707107f, -.92388f,
                                       -1, -.92388f, -.707107f, -.382683f, 0, .382683f,  .707107f,  .92388f};
constexpr std::array<float, 16> sine{0, .382683f,  .707107f,  .92388f,  1,  .92388f,  .707107f,  .382683f,
                                     0, -.382683f, -.707107f, -.92388f, -1, -.92388f, -.707107f, -.382683f};
constexpr std::size_t shapePixels = 3163 * 16;
constexpr float absValue(float f) {
    return f < 0 ? -f : f;
}
// Tiny code-authored arrow/strike artwork. All orientations and sizes are built
// by the compiler, not by runtime image preparation or resampling.
constexpr std::array<Color, shapePixels> makeShape(int shape) {
    std::array<Color, shapePixels> pixels{};
    std::size_t at = 0;
    for (int angle = 0; angle < 16; ++angle)
        for (const int size : sizes)
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x) {
                    const float dx = (x + .5f - size * .5f) / size, dy = (y + .5f - size * .5f) / size;
                    const float u = dx * cosine[angle] + dy * sine[angle],
                                v = -dx * sine[angle] + dy * cosine[angle];
                    Color c{0, 0, 0, 0};
                    if (shape == 0) {
                        if (absValue(v) < .035f && u > -.40f && u < .32f)
                            c = {155, 107, 54, 255};
                        if (u > .22f && u < .48f && absValue(v) < (.48f - u) * .50f)
                            c = {222, 228, 221, 255};
                        if (u < -.22f && u > -.46f && absValue(v) < (.47f + u) * .55f + .03f)
                            c = {205, 196, 163, 255};
                    } else if (shape == 1) {
                        if (u * u + v * v < .020f)
                            c = {218, 225, 224, 255};
                        if (absValue(u) < .035f && absValue(v) < .20f)
                            c = {144, 155, 158, 255};
                    } else if (shape == 2) {
                        if ((absValue(v) < .045f && absValue(u) < .25f) ||
                            (absValue(u) < .045f && absValue(v) < .25f))
                            c = {225, 216, 183, 255};
                        if (u * u + v * v < .008f)
                            c = {119, 77, 39, 255};
                    } else if (shape == 3) {
                        if (u > -.46f && u < .46f && absValue(v - .12f * u * u) < .035f)
                            c = {242, 231, 194, 230};
                    } else {
                        if ((absValue(v) < .035f && absValue(u) < .42f) ||
                            (absValue(u) < .035f && absValue(v) < .42f) || u * u + v * v < .012f)
                            c = {255, 231, 155, 240};
                    }
                    pixels[at++] = c;
                }
    return pixels;
}
constexpr auto shaft = makeShape(0), point = makeShape(1), feathers = makeShape(2), slash = makeShape(3),
               glint = makeShape(4);
static_assert(sizeof(shaft) * 5 < 1024 * 1024, "Prepared projectile bank exceeds budget");
const auto& raster(int shape) {
    return shape == 0 ? shaft : shape == 1 ? point : shape == 2 ? feathers : shape == 3 ? slash : glint;
}
std::size_t offset(int angle, int tier) {
    std::size_t index = angle * 3163;
    for (int i = 0; i < tier; ++i)
        index += sizes[i] * sizes[i];
    return index;
}
ProjectedAttachment partyAnchor(const AttackVisual& v, Player view, int phase, bool current) {
    const auto origin = current ? view : v.partyOrigin;
    const auto f = forward(origin.heading), r = right(origin.heading);
    const float lane = (v.formationSlot % 2 ? 1.f : -1.f) * .15f;
    const float wx = origin.cell.x + .5f + f.x * .10f + r.x * lane;
    const float wy = origin.cell.y + .5f + f.y * .10f + r.y * lane;
    const auto vf = forward(view.heading), vr = right(view.heading);
    const float dx = wx - (view.cell.x + .5f), dy = wy - (view.cell.y + .5f);
    const float depth = dx * vf.x + dy * vf.y + ViewProjection::cameraSetback;
    const float lateral = dx * vr.x + dy * vr.y - float(phase) / ViewProjection::strafeSteps;
    if (depth < .1f)
        return {};
    const float height = v.formationSlot < 2 ? .20f : .17f;
    return {viewWidth * .5f + ViewProjection::focalLength() * lateral / depth,
            ViewProjection::horizon +
                ViewProjection::focalLength() * (ViewProjection::eyeHeight - height) / depth,
            depth, true};
}
} // namespace
std::size_t projectilePreparedBytes() noexcept {
    return sizeof(shaft) * 5;
}
void composeProjectiles(const ProjectilePlayer& effects, const PreparedNpcs& prepared,
                        const ActorWorld& actors, Player view, int phase,
                        const std::array<float, viewWidth>& walls, const PickingBuffer* bodies,
                        Frame& frame) {
    if (!effects.size())
        return;
    std::array<float, NpcWorld::maxInstances> depths{};
    for (std::size_t i = 0; i < actors.snapshot().count; ++i) {
        const auto& a = actors.snapshot().actors[i];
        depths[i] = prepared.attachment(a.npc, actorPosition(a, effects.timeMs()), false, view, phase).depth;
    }
    for (const auto& shot : effects) {
        const auto* actor = actors.actor(shot.enemy);
        auto npc = shot.npcOrigin;
        auto position = shot.incoming ? shot.sourcePosition : shot.targetPosition;
        if (!shot.incoming && actor) {
            npc = actor->npc;
            position = actorPosition(*actor, effects.timeMs());
        }
        const auto enemy = prepared.attachment(npc, position, shot.incoming, view, phase);
        const auto party = partyAnchor(shot, view, phase, shot.incoming);
        const auto from = shot.incoming ? enemy : party, to = shot.incoming ? party : enemy;
        if (!from.valid || !to.valid)
            continue;
        const bool ending = effects.timeMs() >= shot.impactMs;
        if (ending && shot.targetGone)
            continue;
        const float span = std::max<std::uint64_t>(1, shot.impactMs - shot.releaseMs);
        float t = std::clamp(float(effects.timeMs() - shot.releaseMs) / span, 0.f, 1.f);
        float tx = to.x, ty = to.y;
        if (!shot.hit) {
            tx += (shot.id.serial % 2 ? 1.f : -1.f) * (shot.incoming ? 65.f : 25.f / to.depth);
            ty += shot.incoming ? 45.f : 5.f;
        }
        const float depth = std::max(.1f, from.depth * (1 - t) + to.depth * t);
        float x = ((1 - t) * from.x * from.depth + t * tx * to.depth) / depth;
        float y = ((1 - t) * from.y * from.depth + t * ty * to.depth) / depth -
                  4 * t * (1 - t) * ViewProjection::focalLength() * .08f / depth;
        int shape = 0;
        float size = ViewProjection::focalLength() * .13f / depth;
        if (ending) {
            if (!shot.hit)
                continue;
            x = to.x;
            y = to.y;
            shape = shot.ranged ? 4 : 3;
            size = shot.ranged ? 12.f : 28.f;
        } else if (std::abs(to.x - from.x) < 25 && std::abs(to.y - from.y) < 35)
            shape = shot.incoming ? 1 : 2;
        const bool magical = shot.spell != rules::SpellKind::Count;
        if (magical) {
            shape = 4;
            size = ending ? 28.f : std::max(7.f, size * 1.4f);
        }
        const float direction = std::atan2(ty - from.y, tx - from.x);
        const int angle = (int(std::round(direction * (8.f / 3.14159265358979323846f))) + 16) % 16;
        int tier = 0;
        for (int i = 1; i < 8; ++i)
            if (std::abs(sizes[i] - size) < std::abs(sizes[tier] - size))
                tier = i;
        const auto& pixels = raster(shape);
        const auto first = offset(angle, tier);
        const int width = sizes[tier], left = int(std::round(x)) - width / 2,
                  top = int(std::round(y)) - width / 2;
        const float fade = ending ? 1.f - float(effects.timeMs() - shot.impactMs) / 120.f : 1.f;
        for (int dart = 0; dart < std::max<int>(1, shot.darts); ++dart)
            for (int sy = 0; sy < width; ++sy)
                for (int sx = 0; sx < width; ++sx) {
                    const int px = left + sx + (shot.darts > 1 ? (dart * 2 - shot.darts + 1) * 5 : 0),
                              py = top + sy;
                    if (px < 0 || px >= viewWidth || py < 0 || py >= viewHeight || depth >= walls[px])
                        continue;
                    const auto body = bodies ? bodies->slot(px, py) : PickingBuffer::empty;
                    if (body < actors.snapshot().count && depths[body] + .025f < depth &&
                        !(ending && body == shot.enemy.slot && !shot.incoming))
                        continue;
                    auto color = pixels[first + sy * width + sx];
                    if (!color.a)
                        continue;
                    if (magical) {
                        switch (shot.spell) {
                        case rules::SpellKind::FireBolt:
                        case rules::SpellKind::BurningHands:
                        case rules::SpellKind::ScorchingRay:
                            color.r = 255;
                            color.g = 114;
                            color.b = 38;
                            break;
                        case rules::SpellKind::PoisonSpray:
                        case rules::SpellKind::AcidSplash:
                            color.r = 118;
                            color.g = 242;
                            color.b = 60;
                            break;
                        case rules::SpellKind::Sleep:
                        case rules::SpellKind::ColorSpray:
                        case rules::SpellKind::InflictWounds:
                        case rules::SpellKind::Bane:
                            color.r = 179;
                            color.g = 75;
                            color.b = 215;
                            break;
                        case rules::SpellKind::RayOfFrost:
                        case rules::SpellKind::ShockingGrasp:
                        case rules::SpellKind::Thunderwave:
                        case rules::SpellKind::MagicMissile:
                            color.r = 110;
                            color.g = 174;
                            color.b = 255;
                            break;
                        default:
                            color.r = 255;
                            color.g = 232;
                            color.b = 150;
                            break;
                        }
                    }
                    color = shadeScene(color, depth);
                    const unsigned alpha = unsigned(color.a * fade), inverse = 255 - alpha;
                    auto& dst = frame[py * viewWidth + px];
                    dst = {static_cast<unsigned char>((color.r * alpha + dst.r * inverse + 127) / 255),
                           static_cast<unsigned char>((color.g * alpha + dst.g * inverse + 127) / 255),
                           static_cast<unsigned char>((color.b * alpha + dst.b * inverse + 127) / 255), 255};
                }
    }
}
} // namespace blobber
