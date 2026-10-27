#pragma once
#include "game/commands.hpp"
#include "ui/layout.hpp"
namespace blobber {
// Presentation-only placement. The badge never changes the body hit target.
inline std::array<std::optional<UiRect>, 12> spellMarkerLayout(const HudSnapshot& h) noexcept {
    std::array<std::optional<UiRect>, 12> result{};
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
