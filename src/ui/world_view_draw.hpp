#pragma once
#include "raylib.h"
#include "rlgl.h"
#include "ui/world_view_transform.hpp"

namespace blobber {
// A short drawing scope, never an owner of GPU resources. Nested depth clips
// explicitly restore the viewport scissor rather than disabling it.
class WorldOverlayScope {
  public:
    explicit WorldOverlayScope(WorldViewTransform transform) : transform_(transform) {
        constexpr float scale = float(gameWidth) / artWidth;
        const auto origin = transform.forward({0, 0});
        rlPushMatrix();
        rlTranslatef(origin.x * scale, origin.y * scale, 0);
        rlScalef(transform.zoom, transform.zoom, 1);
        viewportClip();
    }
    ~WorldOverlayScope() {
        EndScissorMode();
        rlPopMatrix();
    }
    void clip(UiRect r) const {
        const auto c = transform_.nativeClip(r);
        BeginScissorMode(c.x, c.y, c.width, c.height);
    }
    void viewportClip() const {
        constexpr int scale = gameWidth / artWidth;
        BeginScissorMode(viewX * scale, viewY * scale, viewWidth * scale, viewHeight * scale);
    }

  private:
    WorldViewTransform transform_;
};
} // namespace blobber
