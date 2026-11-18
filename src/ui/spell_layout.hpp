#pragma once
#include "game/commands.hpp"
#include "render/picking.hpp"
#include "ui/layout.hpp"
namespace blobber {
inline bool badgesOverlap(UiRect a, UiRect b) noexcept {
    return a.x < b.x + b.width + 2 && a.x + a.width + 2 > b.x && a.y < b.y + b.height + 2 &&
           a.y + a.height + 2 > b.y;
}
inline std::optional<UiRect> hoveredEnemyHealthBar(const HudSnapshot& h) noexcept {
    const auto& hover = h.hover;
    if (!hover.tooltipVisible || hover.kind != HoverKind::Enemy || !hover.enemy ||
        hover.enemy->world != h.targets.world || hover.enemy->slot >= h.targets.anchors.size())
        return {};
    const auto& anchor = h.targets.anchors[hover.enemy->slot];
    if (!anchor.valid)
        return {};
    return UiRect{viewX + std::clamp(anchor.badgeX - 23, 0, viewWidth - 46),
                  viewY + std::clamp(anchor.top - 8, 0, viewHeight - 6), 46, 6};
}
// Keep indicators near their owner; reject whole rectangles obscured by geometry
// or another body rather than leaving small misleading fragments.
inline bool completeMarker(const PickingBuffer* picking, unsigned owner, UiRect r) noexcept {
    if (!picking)
        return true;
    for (int y = 0; y < r.height; ++y)
        for (int x = 0; x < r.width; ++x)
            if (!picking->badgeVisible(std::uint16_t(owner), r.x - viewX + x, r.y - viewY + y))
                return false;
    return true;
}
template <class Occupied>
std::optional<UiRect> placeMarker(const HudSnapshot& h, const PickingBuffer* picking, unsigned owner,
                                  UiRect preferred, Occupied occupied) noexcept {
    constexpr int offsetsY[]{0, 14, 28, 42, -14, -28};
    constexpr int offsetsX[]{0, 14, -14, 28, -28};
    const int width = 54 + int(h.combat.upcomingCount) * 30;
    const UiRect initiative{viewX + viewWidth - width - 6, 24, width, 34};
    const auto health = hoveredEnemyHealthBar(h);
    for (int dy : offsetsY)
        for (int dx : offsetsX) {
            UiRect r{std::clamp(preferred.x + dx, viewX + 1, viewX + viewWidth - preferred.width - 1),
                     std::clamp(preferred.y + dy, viewY + 1, viewY + viewHeight - 13), preferred.width, 12};
            if ((!h.suspended && h.combat.phase == CombatPhase::Turns && h.combat.upcomingCount &&
                 badgesOverlap(r, initiative)) ||
                (health && badgesOverlap(r, *health)) ||
                occupied(r) || !completeMarker(picking, owner, r))
                continue;
            return r;
        }
    return {};
}
inline std::array<std::optional<UiRect>, NpcWorld::maxInstances>
statusMarkerLayout(const HudSnapshot& h, const PickingBuffer* picking = nullptr) noexcept {
    std::array<std::optional<UiRect>, NpcWorld::maxInstances> result{};
    for (std::size_t i = 0; i < h.targets.count; ++i) {
        const auto id = h.targets.ids[i];
        if (id.world != h.targets.world || id.slot >= result.size())
            continue;
        const auto mask = h.conditions[id.slot];
        const auto a = h.targets.anchors[id.slot];
        if (!mask || !a.valid)
            continue;
        unsigned count = 0;
        for (unsigned k = 0; k < unsigned(EffectKind::Count); ++k)
            count += (mask >> k) & 1;
        const int width = int(std::min(count, 6u)) * 12;
        UiRect preferred{viewX + a.badgeX - width / 2, viewY + a.badgeTop - 14, width, 12};
        if (const auto health = hoveredEnemyHealthBar(h); health && badgesOverlap(preferred, *health))
            preferred.y = health->y - preferred.height - 2;
        result[id.slot] =
            placeMarker(h, picking, id.slot, preferred, [&](UiRect r) {
                            for (const auto& other : result)
                                if (other && badgesOverlap(r, *other))
                                    return true;
                            return false;
                        });
    }
    return result;
}
// Presentation-only placement. The badge never changes the body hit target.
inline std::array<std::optional<UiRect>, 36>
spellMarkerLayout(const HudSnapshot& h, const PickingBuffer* picking = nullptr) noexcept {
    std::array<std::optional<UiRect>, 36> result{};
    const auto statuses = statusMarkerLayout(h, picking);
    const int width = h.magic.missiles ? 23 : 12;
    for (std::size_t i = 0; i < h.magic.affectedCount; ++i) {
        const auto id = h.magic.affected[i];
        if (id.world != h.targets.world || id.slot >= h.targets.anchors.size())
            continue;
        const auto a = h.targets.anchors[id.slot];
        if (!a.valid)
            continue;
        result[i] = placeMarker(h, picking, id.slot,
                                {viewX + a.badgeX + 10, viewY + a.badgeTop - 8, width, 12}, [&](UiRect r) {
                                    for (const auto& status : statuses)
                                        if (status && badgesOverlap(r, *status))
                                            return true;
                                    for (const auto& other : result)
                                        if (other && badgesOverlap(r, *other))
                                            return true;
                                    return false;
                                });
    }
    return result;
}
} // namespace blobber
