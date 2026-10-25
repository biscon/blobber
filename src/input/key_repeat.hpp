#pragma once
#include "game/movement.hpp"
#include <array>

namespace blobber {
class KeyRepeat {
  public:
    std::optional<Action> sample(const std::array<bool, 6>& down, const std::array<bool, 6>& pressed,
                                 float dt);
    void clear();

  private:
    int held_ = -1;
    float elapsed_ = 0, next_ = 0.300f;
};
} // namespace blobber
