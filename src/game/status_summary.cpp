#include "game/effects.hpp"
#include <algorithm>
#include <cstdio>
namespace blobber {
rules::SpellKind effectSpell(EffectKind kind) noexcept {
    using S = rules::SpellKind;
    constexpr S spells[]{S::Bless, S::Bane,       S::ShieldOfFaith, S::MageArmor,
                         S::Count, S::Guidance,   S::Resistance,    S::Sanctuary,
                         S::Sleep, S::ColorSpray, S::RayOfFrost,    S::Grease};
    return unsigned(kind) < std::size(spells) ? spells[unsigned(kind)] : S::Count;
}
bool harmfulEffect(EffectKind kind) noexcept {
    return kind == EffectKind::Bane || kind == EffectKind::HoldPerson || kind >= EffectKind::Sleep;
}
void statusDuration(const StatusEntry& s, char* output, std::size_t bytes) noexcept {
    if (s.clock == StatusClock::CasterTurn)
        std::snprintf(output, bytes, "Until caster's next turn");
    else if (s.clock == StatusClock::Standing)
        std::snprintf(output, bytes, "Until standing on next turn");
    else {
        const auto seconds = s.remainingMs / 1000 + (s.remainingMs % 1000 != 0);
        if (seconds >= 60)
            std::snprintf(output, bytes, "%llum %02llus remaining",
                          static_cast<unsigned long long>(seconds / 60),
                          static_cast<unsigned long long>(seconds % 60));
        else
            std::snprintf(output, bytes, "%llus remaining", static_cast<unsigned long long>(seconds));
    }
}
StatusSummary EffectController::statuses(EffectTarget target) const noexcept {
    StatusSummary result;
    if (!living(target))
        return result;
    const auto remaining = [&](std::uint64_t end) { return end > gameplayMs_ ? end - gameplayMs_ : 0; };
    for (unsigned k = 0; k < unsigned(EffectKind::Count); ++k) {
        const auto kind = EffectKind(k);
        if (!has(target, kind))
            continue;
        StatusEntry status;
        status.effect = kind;
        status.spell = effectSpell(kind);
        status.name = kind == EffectKind::Blind       ? "Color Spray - Blinded"
                      : kind == EffectKind::Slow      ? "Ray of Frost - Slowed"
                      : kind == EffectKind::Knockdown ? "Grease - Prone"
                                                      : effectName(kind);
        status.harmful = harmfulEffect(kind);
        if (kind >= EffectKind::Sleep) {
            const auto& c = conditions_[conditionIndex(target)][k - unsigned(EffectKind::Sleep)];
            status.remainingMs = remaining(c.untilMs);
            if (c.turnBound)
                status.clock = StatusClock::CasterTurn;
            else if (kind == EffectKind::Knockdown && turns_)
                status.clock = StatusClock::Standing;
        } else {
            for (const auto& e : instances_)
                if (e.active && e.kind == kind)
                    for (std::size_t t = 0; t < e.count; ++t)
                        if (e.targets[t] == target)
                            status.remainingMs = std::max(status.remainingMs, remaining(e.expiresMs));
        }
        status.ending = kind == EffectKind::Sleep        ? "Ends early on damage or Aid."
                        : kind == EffectKind::Guidance   ? "Consumed by the next ability check."
                        : kind == EffectKind::Resistance ? "Consumed by the next saving throw."
                        : kind == EffectKind::Sanctuary
                            ? "Ends when the recipient attacks or casts offensively."
                        : kind == EffectKind::HoldPerson ? "A successful repeat save ends it early."
                                                         : "";
        result.entries[result.count++] = status;
    }
    if (target.npc && guiding(target.enemy))
        result.entries[result.count++] = {EffectKind::Count,
                                          rules::SpellKind::GuidingBolt,
                                          "Guiding Bolt",
                                          "Consumed by the next attack.",
                                          remaining(guidingUntil_[target.enemy.slot]),
                                          StatusClock::Seconds,
                                          true};
    if (!target.npc) {
        const auto* c = party_.character(target.character);
        if (c && c->temporaryHitPoints > 0)
            result.entries[result.count++] = {EffectKind::Count,
                                              rules::SpellKind::FalseLife,
                                              "False Life",
                                              "Ends early when temporary HP is depleted.",
                                              remaining(c->temporaryUntilMs),
                                              StatusClock::Seconds,
                                              false};
    }
    return result;
}
} // namespace blobber
