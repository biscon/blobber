#pragma once
#include <algorithm>
#include <array>
#include <cstring>
namespace blobber {
// The platform adapter supplies one bounded sample, never raylib calls in the model.
struct TextInput {
    std::array<char, 65> text{};
    bool backspace = false, erase = false, left = false, right = false, home = false, end = false;
};
struct TextEdit {
    std::size_t cursor = 0;
    float blink = 0;
    bool focused = false;
    void focus(bool on, const std::array<char, 65>& text) noexcept {
        focused = on;
        cursor = std::strlen(text.data());
        blink = 0;
    }
    bool edit(std::array<char, 65>& text, const TextInput& input) noexcept {
        if (!focused)
            return false;
        auto n = std::strlen(text.data());
        cursor = std::min(cursor, n);
        const auto before = text;
        if (input.home)
            cursor = 0;
        if (input.end)
            cursor = n;
        if (input.left && cursor)
            --cursor;
        if (input.right && cursor < n)
            ++cursor;
        if (input.backspace && cursor) {
            std::memmove(text.data() + cursor - 1, text.data() + cursor, n - cursor + 1);
            --cursor;
            --n;
        }
        if (input.erase && cursor < n) {
            std::memmove(text.data() + cursor, text.data() + cursor + 1, n - cursor);
            --n;
        }
        for (const auto ch : input.text) {
            if (!ch)
                break;
            if (ch < 32 || ch > 126 || n == 64)
                continue;
            std::memmove(text.data() + cursor + 1, text.data() + cursor, n - cursor + 1);
            text[cursor++] = ch;
            ++n;
        }
        if (input.home || input.end || input.left || input.right || input.backspace || input.erase ||
            input.text[0])
            blink = 0;
        return text != before;
    }
    void tick(float seconds, bool active) noexcept {
        if (!active)
            focused = false;
        if (seconds >= 0 && seconds < 1) {
            blink += seconds;
            if (blink >= 1)
                blink -= 1;
        }
    }
};
} // namespace blobber
