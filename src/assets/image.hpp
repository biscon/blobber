#pragma once
#include <raylib.h>

namespace blobber {
// Owns CPU image memory across loader/export exception paths. Never retained by the frame loop.
class OwnedImage {
  public:
    explicit OwnedImage(Image image) noexcept : value(image) {}
    ~OwnedImage() {
        if (value.data)
            UnloadImage(value);
    }
    OwnedImage(const OwnedImage&) = delete;
    OwnedImage& operator=(const OwnedImage&) = delete;
    Image value;
};
} // namespace blobber
