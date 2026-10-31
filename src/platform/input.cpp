#include "platform/input.hpp"
#include "raylib.h"
namespace blobber {
TextInput pollTextInput() {
    TextInput t;
    std::size_t n = 0;
    for (int ch = GetCharPressed(); ch; ch = GetCharPressed())
        if (ch >= 32 && ch < 127 && n < 64)
            t.text[n++] = char(ch);
    t.backspace = IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE);
    t.erase = IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE);
    t.left = IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT);
    t.right = IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT);
    t.home = IsKeyPressed(KEY_HOME);
    t.end = IsKeyPressed(KEY_END);
    return t;
}
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
    uiDown[static_cast<std::size_t>(UiKey::Back)] = IsKeyDown(KEY_B);
    uiDown[static_cast<std::size_t>(UiKey::PreviousPage)] = IsKeyDown(KEY_PAGE_UP);
    uiDown[static_cast<std::size_t>(UiKey::NextPage)] = IsKeyDown(KEY_PAGE_DOWN);
    uiDown[static_cast<std::size_t>(UiKey::Inventory)] = IsKeyDown(KEY_I);
    uiDown[static_cast<std::size_t>(UiKey::Equip)] = IsKeyDown(KEY_E);
    uiDown[static_cast<std::size_t>(UiKey::TransferItem)] = IsKeyDown(KEY_T);
    uiDown[static_cast<std::size_t>(UiKey::UseItem)] = IsKeyDown(KEY_U);
    uiDown[static_cast<std::size_t>(UiKey::Interact)] = IsKeyDown(KEY_F);
    uiDown[static_cast<std::size_t>(UiKey::Travel)] = IsKeyDown(KEY_T);
    uiDown[static_cast<std::size_t>(UiKey::Rest)] = IsKeyDown(KEY_R);
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
        result.toggleMap = IsKeyPressed(KEY_M);
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
