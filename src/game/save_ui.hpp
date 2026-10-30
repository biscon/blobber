#pragma once
#include <array>
namespace blobber {
enum class SaveIntent { None, Save, SaveMenu, RetryTerminal };
enum class SaveConfirmation { None, Overwrite, Leave, Quit };
struct SaveHud {
    bool enabled = false, allowed = false, terminalWarning = false, replace = false, leaveAfter = false;
    unsigned cursor = 0;
    SaveIntent intent = SaveIntent::None;
    SaveConfirmation confirmation = SaveConfirmation::None;
    const char* reason = "";
    std::array<char, 256> message{};
};
} // namespace blobber
