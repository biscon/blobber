#pragma once
#include "game/commands.hpp"
#include "ui/layout.hpp"

namespace blobber {
// Fixed art-grid footprint. Clamp the entire splash inside its viewport/card.
inline std::optional<UiRect> feedbackRect(const CombatFeedback& feedback,
                                          const std::array<HudCard, rules::partySize>& cards) noexcept {
    if (!feedback.active)
        return {};
    constexpr int width = 36, height = 24;
    if (feedback.party) {
        for (std::size_t i = 0; i < cards.size(); ++i)
            if (cards[i].id.index == feedback.character.index) {
                const auto card = hudLayout::card(static_cast<int>(i));
                return UiRect{card.x + 2, card.y + 25, width, height};
            }
        return {};
    }
    if (!feedback.anchor.valid)
        return {};
    return UiRect{viewX + std::clamp(feedback.anchor.x - width / 2, 0, viewWidth - width),
                  viewY + std::clamp(feedback.anchor.y - height / 2, 0, viewHeight - height), width, height};
}
// Clockwise screen-space perimeter, with an uncluttered center for digits/MISS.
inline constexpr std::array<ArtPoint, 24> bloodSplat{
    {{0.02f, 0.40f}, {0.16f, 0.34f}, {0.09f, 0.11f}, {0.29f, 0.23f}, {0.35f, 0.01f}, {0.43f, 0.20f},
     {0.57f, 0.18f}, {0.70f, 0.00f}, {0.73f, 0.23f}, {0.95f, 0.13f}, {0.85f, 0.36f}, {1.00f, 0.43f},
     {0.87f, 0.55f}, {0.96f, 0.77f}, {0.77f, 0.72f}, {0.80f, 0.96f}, {0.62f, 0.80f}, {0.51f, 1.00f},
     {0.43f, 0.80f}, {0.25f, 0.96f}, {0.25f, 0.75f}, {0.03f, 0.84f}, {0.15f, 0.60f}, {0.00f, 0.56f}}};
} // namespace blobber
