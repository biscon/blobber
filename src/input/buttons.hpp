#pragma once
#include "input/ui_input.hpp"
#include <algorithm>
#include <cstdint>
namespace blobber {
using ButtonContext = std::array<std::uint64_t, 8>;
struct ButtonTarget {
    UiRect rect{};
    std::uint64_t id = 0;
    bool enabled = true, raw = false;
};
struct ButtonTargets {
    static constexpr unsigned capacity = 160;
    std::array<ButtonTarget, capacity> entries{};
    unsigned count = 0;
    bool overflow = false;
    ButtonContext context{};
    void add(UiRect r, bool enabled = true, bool raw = false, std::uint64_t id = 0) noexcept {
        if (count == capacity) {
            overflow = true;
            return;
        }
        if (!id)
            id = (std::uint64_t(std::uint16_t(r.x)) << 48) | (std::uint64_t(std::uint16_t(r.y)) << 32) |
                 (std::uint64_t(std::uint16_t(r.width)) << 16) | std::uint16_t(r.height);
        entries[count++] = {r, id, enabled, raw};
    }
};
// One captured control, no allocation. Leaving cancels permanently until a new down edge.
class ButtonPress {
  public:
    std::optional<UiRect> feedback;
    void cancel() noexcept {
        armed_ = false;
        feedback.reset();
    }
    UiInput route(const UiInput& input, const ButtonTargets& targets, bool worldPress = false) noexcept {
        if (!input.pointerTracked)
            return input; // Already-logical activation from an input client.
        UiInput out = input;
        out.pointerTracked = false;
        out.pressed[std::size_t(UiKey::Click)] = false;
        if (!input.focused || targets.overflow || input.has(UiKey::Cancel) || input.has(UiKey::RightClick) ||
            input.has(UiKey::Confirm)) {
            cancel();
            return out;
        }
        if (context_ != targets.context) {
            cancel();
            context_ = targets.context;
        }
        const ButtonTarget* hit = nullptr;
        const ButtonTarget* captured = nullptr;
        for (unsigned i = 0; i < targets.count; ++i) {
            const auto& b = targets.entries[i];
            if (!hit && input.pointer && b.rect.contains(*input.pointer))
                hit = &b;
            if (armed_ && b.id == id_)
                captured = &b;
        }
        if (armed_ &&
            (!captured || !hit || hit->id != id_ || !captured->enabled || captured->raw || !input.pointer ||
             !captured->rect.contains(*input.pointer) || !same(captured->rect, rect_)))
            cancel();
        if (input.has(UiKey::Click)) {
            cancel();
            if (hit && hit->enabled && !hit->raw) {
                armed_ = true;
                id_ = hit->id;
                rect_ = hit->rect;
                feedback = rect_;
            } else if ((hit && hit->enabled && hit->raw) || (!hit && worldPress))
                out.pressed[std::size_t(UiKey::Click)] = true;
        }
        if (input.pointerReleased) {
            if (armed_)
                out.pressed[std::size_t(UiKey::Click)] = true;
            cancel();
        } else if (!input.pointerHeld && !input.has(UiKey::Click))
            cancel();
        return out;
    }

  private:
    static bool same(UiRect a, UiRect b) noexcept {
        return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
    }
    ButtonContext context_{};
    std::uint64_t id_ = 0;
    UiRect rect_{};
    bool armed_ = false;
};
} // namespace blobber
