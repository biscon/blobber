#pragma once
#include "input/text_edit.hpp"
#include "ui/layout.hpp"
#include <array>
namespace blobber {
enum class UiKey : std::size_t {
    One,
    Two,
    Three,
    Four,
    Five,
    Six,
    Up,
    Down,
    Left,
    Right,
    Confirm,
    Cancel,
    Click,
    RightClick,
    Back,
    DemoNext,
    DemoReset,
    PreviousPage,
    NextPage,
    Inventory,
    Equip,
    TransferItem,
    UseItem,
    Tab,
    Rest,
    Interact,
    Count
};
constexpr auto uiKeyCount = static_cast<std::size_t>(UiKey::Count);
struct UiInput {
    std::array<bool, uiKeyCount> pressed{};
    std::optional<ArtPoint> pointer;
    int scroll = 0;
    TextInput text;
    bool focused = true, shift = false;
    bool pointerHeld = false, pointerReleased = false;
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
        const auto click = static_cast<std::size_t>(UiKey::Click);
        result.pointerHeld = focused && previousFocus_ && down[click] && !blocked_[click];
        result.pointerReleased =
            focused && previousFocus_ && previous_[click] && !down[click] && !blocked_[click];
        previous_ = down;
        previousFocus_ = focused;
        return result;
    }

  private:
    std::array<bool, uiKeyCount> previous_{}, blocked_{};
    bool previousFocus_ = true;
};
} // namespace blobber
