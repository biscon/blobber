#pragma once
#include "game/commands.hpp"
#include "ui/layout.hpp"
#include <cmath>

namespace blobber {
enum class FeedbackBackdrop : unsigned { Damage, Outcome, Immune };
struct FeedbackPresentation {
    FeedbackBackdrop backdrop = FeedbackBackdrop::Outcome;
    float alpha = 0, offsetX = 0, offsetY = 0;
};
// Presentation-only lifetime. The shared combat hold/retention must stay unchanged.
inline FeedbackPresentation feedbackPresentation(const CombatFeedback& feedback, std::uint64_t now) noexcept {
    FeedbackPresentation result;
    if (!feedback.active || now < feedback.startedMs || now - feedback.startedMs >= 640)
        return result;
    const auto age = unsigned(now - feedback.startedMs);
    const bool damage = feedback.hit && feedback.damage > 0 && !feedback.healing && !feedback.immune;
    result.backdrop = feedback.immune ? FeedbackBackdrop::Immune
                      : damage        ? FeedbackBackdrop::Damage
                                      : FeedbackBackdrop::Outcome;
    result.alpha = std::min(1.f, float(640 - age) / 240);
    if (damage && !feedback.text && age < 100) {
        const float amplitude = .65f * (1 - float(age) / 100);
        result.offsetX = std::sin(float(age) * .13f) * amplitude;
        result.offsetY = std::sin(float(age) * .19f) * amplitude * .5f;
    }
    return result;
}
// Fixed art-grid footprint. Clamp the entire splash inside its viewport/card.
inline std::optional<UiRect> feedbackRect(const CombatFeedback& feedback,
                                          const std::array<HudCard, rules::partySize>& cards) noexcept {
    if (!feedback.active)
        return {};
    const bool portraitOnly = feedback.party && feedback.portrait != PortraitFeedbackKind::None;
    const int width = portraitOnly ? 36 : 44, height = portraitOnly ? 24 : 26;
    if (feedback.party) {
        for (std::size_t i = 0; i < cards.size(); ++i)
            if (cards[i].id.index == feedback.character.index) {
                const auto card = hudLayout::card(static_cast<int>(i));
                return UiRect{card.x + (56 - width) / 2, card.y + 25, width, height};
            }
        return {};
    }
    if (!feedback.anchor.valid)
        return {};
    return UiRect{viewX + std::clamp(feedback.anchor.x - width / 2, 0, viewWidth - width),
                  viewY + std::clamp(feedback.anchor.y - height / 2, 0, viewHeight - height), width, height};
}
} // namespace blobber
