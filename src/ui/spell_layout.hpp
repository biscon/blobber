#pragma once
#include "game/commands.hpp"
#include "ui/layout.hpp"
namespace blobber {
inline bool badgesOverlap(UiRect a, UiRect b) noexcept {
    return a.x < b.x + b.width + 2 && a.x + a.width + 2 > b.x && a.y < b.y + b.height + 2 &&
           a.y + a.height + 2 > b.y;
}
inline std::array<std::optional<UiRect>, NpcWorld::maxInstances>
statusMarkerLayout(const HudSnapshot& h) noexcept {
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
        const int y = viewY + std::clamp(a.badgeTop - 14, 1, viewHeight - 13);
        UiRect r{viewX + std::clamp(a.badgeX - width / 2, 1, viewWidth - width - 1), y, width, 12};
        for (int attempt = 0; attempt < 25; ++attempt) {
            bool overlap = false;
            for (std::size_t j = 0; j < i; ++j)
                if (h.targets.ids[j].slot < result.size() && result[h.targets.ids[j].slot])
                    overlap |= badgesOverlap(r, *result[h.targets.ids[j].slot]);
            if (!overlap) {
                result[id.slot] = r;
                break;
            }
            const int step = (attempt / 2 + 1) * 14;
            r.y = std::clamp(y + (attempt % 2 ? step : -step), viewY + 1, viewY + viewHeight - 13);
        }
    }
    return result;
}
// Presentation-only placement. The badge never changes the body hit target.
inline std::array<std::optional<UiRect>, 36> spellMarkerLayout(const HudSnapshot& h) noexcept {
    std::array<std::optional<UiRect>, 36> result{};
    const auto statuses = statusMarkerLayout(h);
    const int width = h.magic.missiles ? 23 : 12;
    for (std::size_t i = 0; i < h.magic.affectedCount; ++i) {
        const auto id = h.magic.affected[i];
        if (id.world != h.targets.world || id.slot >= h.targets.anchors.size())
            continue;
        const auto a = h.targets.anchors[id.slot];
        if (!a.valid)
            continue;
        UiRect r{viewX + std::clamp(a.badgeX + 10, 1, viewWidth - width - 1),
                 viewY + std::clamp(a.badgeTop - 8, 1, viewHeight - 13), width, 12};
        const int preferred = r.y;
        for (int attempt = 0; attempt < 25; ++attempt) {
            bool overlap = false;
            for (const auto& status : statuses)
                if (status)
                    overlap |= badgesOverlap(r, *status);
            for (std::size_t j = 0; j < i; ++j)
                if (result[j]) {
                    const auto other = *result[j];
                    overlap |= r.x < other.x + other.width + 2 && r.x + r.width + 2 > other.x &&
                               r.y < other.y + other.height + 2 && r.y + r.height + 2 > other.y;
                }
            if (!overlap) {
                result[i] = r;
                break;
            }
            // Search above then below the anchor; twelve badges always fit the viewport height.
            const int step = (attempt / 2 + 1) * 14;
            r.y = std::clamp(preferred + (attempt % 2 ? step : -step), viewY + 1, viewY + viewHeight - 13);
        }
    }
    return result;
}
} // namespace blobber
