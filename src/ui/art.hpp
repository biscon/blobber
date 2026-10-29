#pragma once
#include "platform/gpu_resources.hpp"
#include "rules/character.hpp"
#include "rules/spells.hpp"
#include "ui/layout.hpp"
#include <array>
#include <filesystem>
namespace blobber {
// One startup-prepared atlas, owned by Presenter's resource registry.
enum class EffectKind : std::uint8_t;
class UiArt {
  public:
    static constexpr unsigned classFeaturesIcon = 28, silhouetteIcon = 44, restIcon = 48;
    static unsigned effectIcon(EffectKind) noexcept;
    static constexpr unsigned holdPersonIcon = 59;
    static unsigned itemIcon(const rules::ItemDefinition&) noexcept;
    UiArt(GpuResources& resources, const std::filesystem::path& directory);
    static unsigned spellIcon(rules::SpellKind kind, rules::Feature feature = rules::Feature::None) noexcept;
    void draw(unsigned index, UiRect destination, Color tint = WHITE) const;
    void camp(unsigned frame, UiRect destination, Color tint = WHITE) const;

  private:
    GpuResources& resources_;
    TextureHandle atlas_, camp_, loot_;
    std::array<Rectangle, 57> regions_{};
};
} // namespace blobber
