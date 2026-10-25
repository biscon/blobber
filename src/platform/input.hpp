#pragma once
#include "input/key_repeat.hpp"
namespace blobber {
struct InputSample {
    std::optional<Action> action;
    bool fresh = false, clearPending = false, toggleAnimation = false, toggleMap = false;
};
class Input {
  public:
    InputSample poll(float seconds);

  private:
    KeyRepeat repeat_;
    bool previousFocus_ = true;
};
} // namespace blobber
