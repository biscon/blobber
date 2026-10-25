#include "platform/input.hpp"
#include "raylib.h"
namespace blobber {
InputSample Input::poll(float seconds) {
    InputSample result;
    const bool focused = IsWindowFocused();
    result.clearPending = !focused || !previousFocus_;
    if (result.clearPending)
        repeat_.clear();
    constexpr std::array<int, 12> uiKeys{KEY_ONE,  KEY_TWO,   KEY_THREE, KEY_FOUR,      KEY_UP, KEY_DOWN,
                                         KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_BACKSPACE, KEY_F2, KEY_F3};
    std::array<bool, uiKeyCount> uiDown{};
    for (std::size_t i = 0; i < 10; ++i)
        uiDown[i] = IsKeyDown(uiKeys[i]);
    uiDown[static_cast<std::size_t>(UiKey::Click)] = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    uiDown[static_cast<std::size_t>(UiKey::RightClick)] = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    uiDown[static_cast<std::size_t>(UiKey::DemoNext)] = IsKeyDown(uiKeys[10]);
    uiDown[static_cast<std::size_t>(UiKey::DemoReset)] = IsKeyDown(uiKeys[11]);
    const auto pointer = GetMousePosition();
    const float wheel = GetMouseWheelMove();
    result.ui = edges_.sample(uiDown, focused,
                              artPoint(pointer.x, pointer.y, GetScreenWidth(), GetScreenHeight(),
                                       GetRenderWidth(), GetRenderHeight()),
                              wheel > 0   ? 1
                              : wheel < 0 ? -1
                                          : 0);
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
