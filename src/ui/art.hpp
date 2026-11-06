#pragma once
#include "game/dialogue.hpp"
#include "platform/gpu_resources.hpp"
#include "rules/character.hpp"
#include "rules/creation.hpp"
#include "rules/spells.hpp"
#include "ui/layout.hpp"
#include "world/grid.hpp"
#include <array>
#include <filesystem>
namespace blobber {
// Startup-prepared interface textures, owned by Presenter's resource registry.
enum class EffectKind : std::uint8_t;
enum class MapMarker : std::uint8_t;
enum class FeedbackBackdrop : unsigned;
class UiArt {
  public:
    static constexpr unsigned classFeaturesIcon = 28, silhouetteIcon = 44, restIcon = 48;
    static unsigned effectIcon(EffectKind) noexcept;
    static constexpr unsigned holdPersonIcon = 59, quarterstaffIcon = 60, travelIcon = 61,
                              skeletonPortraitIcon = 62, bugbearPortraitIcon = 63,
                              giantRatPortraitIcon = 64 + rules::portraitCount,
                              zombiePortraitIcon = giantRatPortraitIcon + 1,
                              shamanPortraitIcon = zombiePortraitIcon + 1;
    static unsigned itemIcon(const rules::ItemDefinition&) noexcept;
    UiArt(GpuResources& resources, const std::filesystem::path& directory);
    static unsigned spellIcon(rules::SpellKind kind, rules::Feature feature = rules::Feature::None) noexcept;
    void prepareDialogue(const std::vector<std::string>& files);
    void clearDialogue() noexcept;
    void dialogueOverlay(DialogueOverlayId, UiRect, Color tint = WHITE) const;
    void draw(unsigned index, UiRect destination, Color tint = WHITE) const;
    void creator(unsigned index, UiRect destination, Color tint = WHITE) const;
    void portraitEffect(unsigned row, unsigned frame, UiRect destination, Color tint) const;
    void supportEffect(unsigned row, unsigned frame, UiRect destination, Color tint) const;
    void feedbackBackdrop(FeedbackBackdrop, UiRect destination, Color tint) const;
    void camp(unsigned frame, UiRect destination, Color tint = WHITE) const;
    void mapMarker(MapMarker, Rectangle destination, Heading facing = Heading::North) const;

  private:
    GpuResources& resources_;
    TextureHandle creator_, atlas_, camp_, loot_, portraitSmall_, portraitLarge_, map_, portraitEffects_,
        feedback_, supportEffects_;
    std::array<Rectangle, 64> regions_{};
    std::array<GpuLease<TextureHandle>, dialogueOverlayCapacity> dialogue_;
};
} // namespace blobber
