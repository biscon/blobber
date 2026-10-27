#include "platform/input.hpp"
#include "raylib.h"
namespace blobber {
InputSample Input::poll(float seconds) {
    InputSample result;
    const bool focused = IsWindowFocused();
    result.clearPending = !focused || !previousFocus_;
    if (result.clearPending)
        repeat_.clear();
    constexpr std::array<int, 12> uiKeys{KEY_ONE, KEY_TWO,  KEY_THREE, KEY_FOUR,  KEY_FIVE,  KEY_SIX,
                                         KEY_UP,  KEY_DOWN, KEY_LEFT,  KEY_RIGHT, KEY_ENTER, KEY_ESCAPE};
    std::array<bool, uiKeyCount> uiDown{};
    for (std::size_t i = 0; i < uiKeys.size(); ++i)
        uiDown[i] = IsKeyDown(uiKeys[i]);
    uiDown[static_cast<std::size_t>(UiKey::Cancel)] |= IsKeyDown(KEY_BACKSPACE);
    uiDown[static_cast<std::size_t>(UiKey::Click)] = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    uiDown[static_cast<std::size_t>(UiKey::RightClick)] = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    uiDown[static_cast<std::size_t>(UiKey::DemoNext)] = IsKeyDown(KEY_F2);
    uiDown[static_cast<std::size_t>(UiKey::DemoReset)] = IsKeyDown(KEY_F3);
    uiDown[static_cast<std::size_t>(UiKey::Back)] = IsKeyDown(KEY_B);
    uiDown[static_cast<std::size_t>(UiKey::PreviousPage)] = IsKeyDown(KEY_PAGE_UP);
    uiDown[static_cast<std::size_t>(UiKey::NextPage)] = IsKeyDown(KEY_PAGE_DOWN);
    uiDown[static_cast<std::size_t>(UiKey::Inventory)] = IsKeyDown(KEY_I);
    uiDown[static_cast<std::size_t>(UiKey::Equip)] = IsKeyDown(KEY_E);
    uiDown[static_cast<std::size_t>(UiKey::MoveItem)] = IsKeyDown(KEY_M);
    uiDown[static_cast<std::size_t>(UiKey::SplitItem)] = IsKeyDown(KEY_S);
    uiDown[static_cast<std::size_t>(UiKey::TransferItem)] = IsKeyDown(KEY_T);
    uiDown[static_cast<std::size_t>(UiKey::UseItem)] = IsKeyDown(KEY_U);
    uiDown[static_cast<std::size_t>(UiKey::Tab)] = IsKeyDown(KEY_TAB);
    const auto pointer = GetMousePosition();
    const float wheel = GetMouseWheelMove();
    result.ui = edges_.sample(uiDown, focused,
                              artPoint(pointer.x, pointer.y, GetScreenWidth(), GetScreenHeight(),
                                       GetRenderWidth(), GetRenderHeight()),
                              wheel > 0   ? 1
                              : wheel < 0 ? -1
                                          : 0);
    result.ui.shift = focused && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
    if (focused && previousFocus_) {
        result.toggleAnimation = IsKeyPressed(KEY_TAB);
        result.toggleMap = IsKeyPressed(KEY_F1);
        constexpr std::array<int, 6> keys{KEY_W, KEY_S, KEY_A, KEY_D, KEY_Q, KEY_E};
        std::array<bool, 6> down{}, pressed{};
        for (int i = 0; i < 6; ++i) {
            down[i] = IsKeyDown(keys[i]);
            pressed[i] = IsKeyPressed(keys[i]);
        }
        result.action = repeat_.sample(down, pressed, seconds);
        if (result.action)
            result.fresh = pressed[static_cast<int>(*result.action)];
    }
    previousFocus_ = focused;
    return result;
}
} // namespace blobber
