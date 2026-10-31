#include "platform/window.hpp"
#include "raylib.h"
#include <stdexcept>
namespace blobber {
Window::Window(int width, int height, bool hidden) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI |
                   (hidden ? FLAG_WINDOW_HIDDEN : 0));
    InitWindow(width, height, "Stone & Shadow | Blobber");
    if (!IsWindowReady()) {
        // raylib returns before initializing rlgl on platform failure; its
        // CloseWindow path assumes initialized GL resources.
        throw std::runtime_error("Graphics unavailable: cannot open an OpenGL window");
    }
    SetExitKey(KEY_NULL);
    SetWindowMinSize(320, 240);
    SetTargetFPS(60);
}
Window::~Window() {
    CloseWindow();
}
} // namespace blobber
