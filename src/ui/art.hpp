#pragma once
#include "platform/gpu_resources.hpp"
#include "rules/spells.hpp"
#include "ui/layout.hpp"
#include <array>
#include <filesystem>
namespace blobber {
// One startup-prepared atlas, owned by Presenter's resource registry.
class UiArt {
  public:
    static constexpr unsigned classFeaturesIcon = 28;
    UiArt(GpuResources& resources, const std::filesystem::path& directory);
    static unsigned spellIcon(rules::SpellKind kind, rules::Feature feature = rules::Feature::None) noexcept;
    void draw(unsigned index, UiRect destination, Color tint = WHITE) const;

  private:
    GpuResources& resources_;
    TextureHandle atlas_;
    std::array<Rectangle, 29> regions_{};
};
} // namespace blobber
