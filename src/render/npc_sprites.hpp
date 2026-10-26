#pragma once
#include "game/actors.hpp"
#include "game/movement.hpp"
#include "game/npc_animation.hpp"
#include "render/frame.hpp"
#include "render/picking.hpp"
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
    float width = 0; // Zero inherits appearance canvas width; never a per-frame scale.
    std::array<std::vector<NpcImage>, 4> views;
};
struct NpcAppearance {
    float width = 0.45f, height = 0.65f;
    std::array<NpcImage, 4> views; // Legacy static assets; mutually exclusive with animations.
    std::array<NpcClipImages, npcAnimationCount> animations;
    unsigned char alphaCutoff = 0;
};
// Narrower 84-degree framing enlarges prepared sprites; startup remains bounded.
constexpr std::size_t npcPixelBudget = 160u * 1024u * 1024u / sizeof(Color);
constexpr int npcMaxImageDimension = 2048;
// Loading-time validation, also used for generated test assets. Returns source pixel count.
std::size_t validateNpcAppearance(const NpcAppearance& appearance);
NpcTimings npcTimings(const NpcAppearance& appearance);
NpcSide npcSide(Heading facing, float toCameraX, float toCameraY);

// Immutable prepared images; travel projects bounded full-pose rasters between grid banks.
class PreparedNpcs {
  public:
    explicit PreparedNpcs(const std::vector<NpcAppearance>& appearances = {});
    void compose(const ActorWorld& npcs, Player player, int phase,
                 const std::array<float, viewWidth>& wallDepth, Frame& frame,
                 const NpcAnimationSnapshot* animation = nullptr, PickingBuffer* picking = nullptr) const;
    std::size_t pixelCount() const noexcept { return pixels_.size(); }
    std::size_t preparedPixelBytes() const noexcept {
        return pixels_.size() * sizeof(Color) + shadowAlpha_.size() + travelShadow_.size();
    }

  private:
    static constexpr int depthCount = (ViewProjection::visibleRows + 1) * 2;
    struct Sprite {
        int width = 0, top = 0, rows = 0;
        float pivotX = 0;
        std::size_t first = 0;
    };
    struct TravelSprite {
        int width, height;
        float worldWidth, worldHeight, anchorX, anchorY;
        std::size_t first;
    };
    void composeTravel(const TravelSprite& sprite, float lateral, float depth, std::uint16_t instance,
                       bool flash, const std::array<float, viewWidth>& walls, Frame& frame,
                       PickingBuffer* picking) const;
    void composeTravelShadow(float radius, float lateral, float depth,
                             const std::array<float, viewWidth>& walls, Frame& frame) const;
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
        std::array<std::array<std::size_t, 4>, npcAnimationCount> travelFirst{};
        float shadowRadius = 0;
    };
    std::vector<Sprite> sprites_;
    std::vector<TravelSprite> travelSprites_;
    std::array<unsigned char, 64 * 64> travelShadow_{};
    std::vector<Type> types_;
    std::vector<Color> pixels_;
    std::vector<std::array<Shadow, depthCount>> shadows_;
    std::vector<ShadowRow> shadowRows_;
    std::vector<unsigned char> shadowAlpha_;
};
} // namespace blobber
