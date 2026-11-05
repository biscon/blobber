#pragma once
#include "render/world_visibility.hpp"
namespace blobber::world_render {
struct MeleeSwipeTuning {
    unsigned durationMs = 120, revealMs = 35, retreatMs = 65, contactMs = 50;
    float span = 1.35f, thickness = .085f, curvature = .30f;
    float angleDegrees = 35, variationDegrees = 10, missOffset = 1.05f;
    std::array<unsigned char, 3> color{245, 240, 220};
};
inline constexpr MeleeSwipeTuning meleeSwipeTuning{};
constexpr unsigned swipeSegments = 20;
// Cross-section positions and opacity: opaque core, feathered outer edges.
inline constexpr std::array<float, 6> swipeCrossSection{-1, -.85f, -.65f, .65f, .85f, 1};
inline constexpr std::array<float, 6> swipeEdgeAlpha{0, .45f, 1, 1, .45f, 0};
constexpr unsigned swipeColumns = swipeCrossSection.size();
struct SwipePoint {
    float x = 0, y = 0;
};
struct SwipeRibbon {
    // Six vertices per sample; adjacent columns form non-overlapping alpha bands.
    std::array<SwipePoint, (swipeSegments + 1) * swipeColumns> vertices{};
    float alpha = 0;
};
struct SwipeFrame {
    std::array<SwipeRibbon, rules::partySize> ribbons{};
    unsigned count = 0;
};
// Pure sampling; age is relative to the swipe start, not attack acceptance.
SwipeRibbon sampleSwipe(SwipePoint center, float width, float height, bool mirrored, float variationDegrees,
                        unsigned age, const MeleeSwipeTuning& = meleeSwipeTuning) noexcept;
class MeleeSwipePlayer {
  public:
    bool update(const FrameInput&, const WorldFrame&) noexcept;
    const SwipeFrame& frame() const noexcept { return frame_; }

  private:
    struct Record {
        CombatAttackId id{};
        SwipePoint center{};
        float width = 0, height = 0, variation = 0;
        bool mirrored = false, valid = false;
    };
    std::array<Record, rules::partySize> records_{};
    SwipeFrame frame_{};
    std::uint64_t world_ = 0, revision_ = UINT64_MAX, now_ = UINT64_MAX;
    unsigned sequence_ = 0;
};
} // namespace blobber::world_render
