#pragma once
#include <algorithm>
#include <cmath>
namespace blobber::audio {
// Stereo-linked sample-peak protection. State belongs exclusively to the mixer thread.
class PeakLimiter {
  public:
    static constexpr float ceiling = .84139514f; // -1.5 dBFS
    void reset(unsigned sampleRate = 48000) noexcept {
        gain_ = 1;
        release_ = -std::expm1(-1.f / (.08f * std::max(1u, sampleRate)));
    }
    void process(float* stereo, unsigned frames) noexcept {
        for (unsigned i = 0; i < frames; ++i) {
            auto& left = stereo[2 * i];
            auto& right = stereo[2 * i + 1];
            if (!std::isfinite(left))
                left = 0;
            if (!std::isfinite(right))
                right = 0;
            const float peak = std::max(std::abs(left), std::abs(right));
            const float required = peak > ceiling ? ceiling / peak : 1.f;
            gain_ = std::min(required, gain_ + (1 - gain_) * release_);
            left *= gain_;
            right *= gain_;
        }
    }

  private:
    float gain_ = 1;
    float release_ = .00026038276f; // 80 ms at 48 kHz until reset
};
} // namespace blobber::audio
