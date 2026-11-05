#pragma once
#include "game/commands.hpp"
#include <algorithm>
namespace blobber {
struct PortraitFeedbackSample {
    bool active = false, support = false;
    unsigned row = 0, frame = 0;
    std::array<unsigned char, 4> tint{255, 255, 255, 0};
};
inline PortraitFeedbackSample portraitFeedbackSample(const PortraitFeedback& f, std::uint64_t now) noexcept {
    PortraitFeedbackSample result;
    if (f.kind == PortraitFeedbackKind::None || now < f.startedMs || now - f.startedMs >= f.durationMs)
        return result;
    const auto age = unsigned(now - f.startedMs);
    result.active = true;
    if (f.support != rules::SpellKind::Count) {
        using K = rules::SpellKind;
        result.support = true;
        result.row =
            f.support == K::SpareTheDying       ? 0
            : f.support == K::LesserRestoration ? 1
            : f.support == K::Bane || f.support == K::HoldPerson || f.support == K::Web ||
                    f.support == K::ColorSpray
                ? 3
            : f.support == K::ShieldOfFaith || f.support == K::Resistance || f.support == K::FalseLife ? 4
            : f.support == K::MageArmor || f.support == K::Aid                                         ? 5
            : f.support == K::Sanctuary || f.support == K::MirrorImage                                 ? 6
            : f.support == K::Sleep                                                                    ? 7
                                                                                                       : 2;
        result.frame = std::min(7u, age * 8 / f.durationMs);
        if (f.phase == SupportPhase::End || f.phase == SupportPhase::ImageLost)
            result.frame = 7 - result.frame;
        if (f.support == K::Guidance || f.support == K::FalseLife)
            result.frame = 7 - result.frame;
        result.tint = result.row < 2                       ? std::array<unsigned char, 4>{100, 255, 145, 0}
                      : result.row == 3 || result.row == 7 ? std::array<unsigned char, 4>{195, 150, 255, 0}
                                                           : std::array<unsigned char, 4>{145, 205, 255, 0};
        const float strength = f.phase == SupportPhase::Apply ? 175.f : 85.f;
        result.tint[3] =
            static_cast<unsigned char>(strength * std::min(1.f, float(f.durationMs - age) / 160.f));
        return result;
    }
    result.row = f.kind == PortraitFeedbackKind::Healing      ? 0
                 : f.kind == PortraitFeedbackKind::Protection ? 1
                 : f.kind == PortraitFeedbackKind::Harmful    ? 3
                                                              : 2;
    result.frame = std::min(7u, age * 8 / f.durationMs);
    if (f.kind == PortraitFeedbackKind::Healing)
        result.tint = {60, 255, 80, 0};
    using D = rules::DamageType;
    if (result.row == 2) {
        switch (f.element) {
        case D::Radiant:
            result.tint = {255, 221, 125, 0};
            break;
        case D::Fire:
            result.tint = {255, 122, 48, 0};
            break;
        case D::Cold:
            result.tint = {105, 220, 255, 0};
            break;
        case D::Lightning:
            result.tint = {155, 190, 255, 0};
            break;
        case D::Necrotic:
            result.tint = {186, 95, 226, 0};
            break;
        case D::Acid:
            result.tint = {208, 235, 70, 0};
            break;
        case D::Poison:
            result.tint = {135, 185, 95, 0};
            break;
        case D::Force:
            result.tint = {202, 165, 255, 0};
            break;
        default:
            result.tint = {205, 218, 235, 0};
            break;
        }
    }
    const float fade = std::min(1.f, float(f.durationMs - age) / 160.f);
    result.tint[3] = static_cast<unsigned char>((f.kind == PortraitFeedbackKind::Negated ? 75 : 195) * fade);
    return result;
}
} // namespace blobber
