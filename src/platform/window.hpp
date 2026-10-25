#pragma once
namespace blobber {
class Window {
  public:
    Window(int width, int height, bool hidden = false);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};
} // namespace blobber
