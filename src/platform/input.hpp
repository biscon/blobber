#pragma once
#include "input/key_repeat.hpp"
#include "input/ui_input.hpp"
#include "input/text_edit.hpp"
namespace blobber {
struct InputSample {
    std::optional<Action> action;
    UiInput ui;
    bool fresh = false, clearPending = false, toggleAnimation = false, toggleMap = false;
};
TextInput pollTextInput();
class Input {
  public:
    InputSample poll(float seconds);

  private:
    KeyRepeat repeat_;
    UiInputEdges edges_;
    bool previousFocus_ = true;
};
} // namespace blobber
