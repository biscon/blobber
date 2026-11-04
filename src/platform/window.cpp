#include "platform/window.hpp"
#include "raylib.h"
#include <stdexcept>
namespace blobber {
Window::Window(int width, int height, bool hidden, bool uncapped) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | (uncapped ? 0 : FLAG_VSYNC_HINT) | FLAG_WINDOW_HIGHDPI |
                   (hidden ? FLAG_WINDOW_HIDDEN : 0));
    InitWindow(width, height, "Stone & Shadow | Blobber");
    if (!IsWindowReady()) {
        // raylib returns before initializing rlgl on platform failure; its
        // CloseWindow path assumes initialized GL resources.
        throw std::runtime_error("Graphics unavailable: cannot open an OpenGL window");
    }
    if (uncapped)
        ClearWindowState(FLAG_VSYNC_HINT);
    SetExitKey(KEY_NULL);
    SetWindowMinSize(320, 240);
    SetTargetFPS(uncapped ? 0 : 60);
}
Window::~Window() {
    CloseWindow();
}
} // namespace blobber
