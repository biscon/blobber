#pragma once
#include <algorithm>
namespace blobber {
// Independent of simulation clocks and graphics. Loading may begin only after a black presentation.
class TravelFade {
  public:
    enum class Phase { Out, Black, Loading, In, Done };
    static constexpr float duration = .25f;
    Phase phase() const noexcept { return phase_; }
    float alpha() const noexcept {
        return phase_ == Phase::Out    ? elapsed_ / duration
               : phase_ == Phase::In   ? 1 - elapsed_ / duration
               : phase_ == Phase::Done ? 0
                                       : 1;
    }
    void advance(float seconds) noexcept {
        if (phase_ != Phase::Out && phase_ != Phase::In)
            return;
        elapsed_ = std::min(duration, elapsed_ + std::clamp(seconds, 0.f, .1f));
        if (elapsed_ == duration) {
            phase_ = phase_ == Phase::Out ? Phase::Black : Phase::Done;
            elapsed_ = 0;
        }
    }
    void blackPresented() noexcept {
        if (phase_ == Phase::Black)
            phase_ = Phase::Loading;
    }
    void prepared() noexcept {
        if (phase_ == Phase::Loading) {
            phase_ = Phase::In;
            elapsed_ = 0;
        }
    }

  private:
    Phase phase_ = Phase::Out;
    float elapsed_ = 0;
};
} // namespace blobber
