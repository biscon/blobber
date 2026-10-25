#pragma once
#include "ui/layout.hpp"
#include <array>
namespace blobber {
enum class UiKey : std::size_t {
    One,
    Two,
    Three,
    Four,
    Up,
    Down,
    Left,
    Right,
    Confirm,
    Cancel,
    Click,
    RightClick,
    DemoNext,
    DemoReset,
    Count
};
constexpr auto uiKeyCount = static_cast<std::size_t>(UiKey::Count);
struct UiInput {
    std::array<bool, uiKeyCount> pressed{};
    std::optional<ArtPoint> pointer;
    int scroll = 0;
    bool focused = true;
    bool has(UiKey key) const noexcept { return pressed[static_cast<std::size_t>(key)]; }
};
class UiInputEdges {
  public:
    UiInput sample(const std::array<bool, uiKeyCount>& down, bool focused,
                   std::optional<ArtPoint> pointer = {}, int scroll = 0) noexcept {
        UiInput result;
        result.focused = focused;
        result.pointer = focused ? pointer : std::nullopt;
        result.scroll = focused && previousFocus_ ? scroll : 0;
        for (std::size_t i = 0; i < down.size(); ++i) {
            // Some platforms synthesize key-up on blur and key-down after refocus.
            // Preserve held latches across that gap; only a focused release rearms them.
            if (!focused)
                blocked_[i] = blocked_[i] || previous_[i] || down[i];
            else if (!previousFocus_)
                blocked_[i] = blocked_[i] || down[i];
            else if (previous_[i] && !down[i])
                blocked_[i] = false;
            result.pressed[i] = focused && previousFocus_ && down[i] && !previous_[i] && !blocked_[i];
        }
        previous_ = down;
        previousFocus_ = focused;
        return result;
    }

  private:
    std::array<bool, uiKeyCount> previous_{}, blocked_{};
    bool previousFocus_ = true;
};
} // namespace blobber
