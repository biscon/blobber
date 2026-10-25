#include "platform/input.hpp"
#include "raylib.h"
namespace blobber {
InputSample Input::poll(float seconds) {
    InputSample result;
    const bool focused = IsWindowFocused();
    result.clearPending = !focused || !previousFocus_;
    if (result.clearPending)
        repeat_.clear();
    if (focused) {
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
