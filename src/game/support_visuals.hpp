#pragma once
#include "game/effects.hpp"
namespace blobber {
struct CombatOutcome;
enum class SupportPhase : std::uint8_t { Apply, End, Negated, Placement, Move, Strike, ImageLost };
struct SupportVisual {
    SpellCastVisualId id{};
    rules::SpellKind spell = rules::SpellKind::Count;
    EffectTarget target{};
    EffectSource source{};
    EffectId effect{};
    AreaId area{};
    SupportPhase phase = SupportPhase::Apply;
    EffectEnd reason = EffectEnd::None;
    Cell cell{}, from{};
    std::uint64_t occurredMs = 0;
    unsigned duplicate = 0;
};
constexpr unsigned supportDuration(SupportPhase phase) noexcept {
    return phase == SupportPhase::End || phase == SupportPhase::Negated || phase == SupportPhase::ImageLost
               ? 240u
           : phase == SupportPhase::Move ? 320u
                                         : 640u;
}
struct SupportVisualSnapshot {
    static constexpr unsigned capacity = 128;
    std::array<SupportVisual, capacity> records{};
    std::uint64_t world = 0, epoch = 0, revision = 0, dropped = 0;
};
// Session lifetime; observers only publish resolved cosmetic facts, never simulation work.
class SupportVisualJournal {
  public:
    void clear(std::uint64_t world) noexcept;
    void bind(const EffectController& effects) noexcept { effects_ = &effects; }
    void combat(const CombatOutcome&) noexcept;
    void emit(SupportVisual) noexcept;
    const SupportVisualSnapshot& snapshot() const noexcept { return snapshot_; }

  private:
    SupportVisualSnapshot snapshot_{};
    const EffectController* effects_ = nullptr;
    std::uint64_t next_ = 1;
    unsigned cursor_ = 0;
};
} // namespace blobber
