#pragma once
#include "game/movement.hpp"
#include "render/frame.hpp"

namespace blobber {
// Output must not alias either input. Normal calls allocate nothing.
void composeTransition(const Frame& from, const Frame& to, Action action, float t, Frame& output);
} // namespace blobber
