#pragma once
#include "game/actors.hpp"
#include "game/movement.hpp"
#include "game/npc_animation.hpp"
#include "render/frame.hpp"
#include "render/projection.hpp"
#include <array>
#include <vector>

namespace blobber {
enum class NpcSide { Front, Back, Left, Right };
struct NpcImage {
    int width = 0, height = 0;
    std::vector<Color> pixels;
    float anchorX = 0.5f, anchorY = 1.0f;
};
struct NpcClipImages {
    NpcClipTiming timing;
    std::array<std::vector<NpcImage>, 4> views;
};
struct NpcAppearance {
    float width = 0.45f, height = 0.65f;
    std::array<NpcImage, 4> views; // Legacy static assets; mutually exclusive with animations.
    std::array<NpcClipImages, npcAnimationCount> animations;
    unsigned char alphaCutoff = 0;
};
constexpr std::size_t npcPixelBudget = 128u * 1024u * 1024u / sizeof(Color);
constexpr int npcMaxImageDimension = 2048;
// Loading-time validation, also used for generated test assets. Returns source pixel count.
std::size_t validateNpcAppearance(const NpcAppearance& appearance);
NpcTimings npcTimings(const NpcAppearance& appearance);
NpcSide npcSide(Heading facing, float toCameraX, float toCameraY);

// Loading-time scaled/shaded images; compose only clips and blends prepared pixels.
class PreparedNpcs {
  public:
    explicit PreparedNpcs(const std::vector<NpcAppearance>& appearances = {});
    void compose(const ActorWorld& npcs, Player player, int phase,
                 const std::array<float, viewWidth>& wallDepth, Frame& frame,
                 const NpcAnimationSnapshot* animation = nullptr) const;
    std::size_t pixelCount() const noexcept { return pixels_.size(); }

  private:
    static constexpr int depthCount = (ViewProjection::visibleRows + 1) * 2;
    struct Sprite {
        int width = 0, top = 0, rows = 0;
        float pivotX = 0;
        std::size_t first = 0;
    };
    struct ShadowRow {
        int y, width;
        float depth;
        std::size_t first;
    };
    struct Shadow {
        std::size_t firstRow = 0, rowCount = 0;
    };
    void prepareShadows(const std::vector<NpcAppearance>& appearances, std::size_t usedPixelBytes);
    struct Type {
        std::array<std::array<std::size_t, 4>, npcAnimationCount> first{};
        std::array<std::array<std::uint8_t, 4>, npcAnimationCount> count{};
    };
    std::vector<Sprite> sprites_;
    std::vector<Type> types_;
    std::vector<Color> pixels_;
    std::vector<std::array<Shadow, depthCount>> shadows_;
    std::vector<ShadowRow> shadowRows_;
    std::vector<unsigned char> shadowAlpha_;
};
} // namespace blobber
