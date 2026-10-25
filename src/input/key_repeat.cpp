#include "input/key_repeat.hpp"
#include <algorithm>

namespace blobber {
void KeyRepeat::clear() {
    held_ = -1;
    elapsed_ = 0;
    next_ = 0.300f;
}
std::optional<Action> KeyRepeat::sample(const std::array<bool, 6>& down, const std::array<bool, 6>& pressed,
                                        float dt) {
    for (int i = 0; i < 6; ++i) {
        if (pressed[i]) {
            held_ = i;
            elapsed_ = 0;
            next_ = 0.300f;
            return static_cast<Action>(i);
        }
    }
    if (held_ < 0)
        return {};
    if (!down[held_]) {
        clear();
        return {};
    }
    elapsed_ += std::max(0.0f, dt);
    if (elapsed_ >= next_) {
        next_ = elapsed_ + 0.180f;
        return static_cast<Action>(held_);
    }
    return {};
}
} // namespace blobber
