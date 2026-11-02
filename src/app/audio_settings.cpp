#include "app/audio_settings.hpp"
#include "platform/audio.hpp"
#include "platform/input.hpp"
#include "ui/hud_painter.hpp"
namespace blobber {
namespace {
struct SettingsView {
    AudioRuntime& audio;
    unsigned focus = 0;
    ButtonPress press;
};
UiRect control(unsigned row, bool plus) {
    return {plus ? 440 : 365, 65 + int(row) * 39, 48, 28};
}
void drawSettings(void* context, const UiFonts& fonts, const UiArt& art) {
    const auto& v = *static_cast<SettingsView*>(context);
    HudPainter p(&fonts, &art);
    p.overlayBox({125, 20, 390, 315});
    p.centered("SETTINGS / AUDIO", {140, 28, 360, 25}, hudInk::gold, UiTextStyle::Control);
    const char* labels[]{"Master", "Effects", "Ambience", "UI", "Party footsteps"};
    for (unsigned i = 0; i < 5; ++i) {
        char value[24];
        std::snprintf(value, sizeof(value), "%d%%", int(v.audio.settings.values[i] * 100 + .5f));
        p.text(labels[i], 145, 73 + i * 39, hudInk::text);
        p.text(value, 307, 73 + i * 39, hudInk::gold);
        for (bool plus : {false, true}) {
            auto r = control(i, plus);
            p.box(r, v.focus == i ? hudInk::teal : hudInk::edge);
            p.centered(plus ? "+" : "-", r, hudInk::text);
        }
    }
    p.box({145, 270, 160, 28}, v.focus == 5 ? hudInk::teal : hudInk::edge);
    p.centered("Reset defaults", {145, 270, 160, 28});
    p.box({335, 270, 160, 28}, v.focus == 6 ? hudInk::teal : hudInk::edge);
    p.centered("Back", {335, 270, 160, 28});
    p.text("Arrows adjust / Tab selects / Escape saves", 145, 311, hudInk::muted, UiTextStyle::Small);
    p.buttonFeedback(v.press.feedback);
}
} // namespace
void captureAudioSettings(Presenter& presenter, const std::filesystem::path& path) {
    SettingsView view{*presenter.audio, 0, {}};
    presenter.prepareMenu();
    presenter.presentMenu(drawSettings, &view);
    presenter.capture(path);
}
void runAudioSettings(Presenter& presenter, GameSession* game) {
    if (!presenter.audio)
        return;
    SettingsView v{*presenter.audio, 0, {}};
    Input input;
    bool done = false, armed = false;
    presenter.prepareMenu();
    while (!done && !WindowShouldClose()) {
        auto in = input.poll(GetFrameTime()).ui;
        if (!armed) {
            armed = !IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsKeyDown(KEY_ENTER) && !IsKeyDown(KEY_ESCAPE);
            in.pressed.fill(false);
            in.pointerHeld = in.pointerReleased = false;
            v.press.cancel();
        }
        ButtonTargets targets;
        for (unsigned i = 0; i < 5; ++i) {
            targets.add(control(i, false));
            targets.add(control(i, true));
        }
        targets.add({145, 270, 160, 28});
        targets.add({335, 270, 160, 28});
        in = v.press.route(in, targets);
        if (in.has(UiKey::Cancel))
            done = true;
        if (in.has(UiKey::Down) || in.has(UiKey::Tab))
            v.focus = (v.focus + 1) % 7;
        if (in.has(UiKey::Up))
            v.focus = (v.focus + 6) % 7;
        int delta = in.has(UiKey::Right) ? 1 : in.has(UiKey::Left) ? -1 : 0;
        bool activate = in.has(UiKey::Confirm);
        if (in.has(UiKey::Click) && in.pointer) {
            for (unsigned i = 0; i < 5; ++i)
                for (bool plus : {false, true})
                    if (control(i, plus).contains(*in.pointer)) {
                        v.focus = i;
                        delta = plus ? 1 : -1;
                    }
            if (UiRect{145, 270, 160, 28}.contains(*in.pointer)) {
                v.focus = 5;
                activate = true;
            }
            if (UiRect{335, 270, 160, 28}.contains(*in.pointer)) {
                v.focus = 6;
                activate = true;
            }
        }
        if (v.focus < 5 && delta)
            v.audio.settings.values[v.focus] =
                std::clamp(v.audio.settings.values[v.focus] + delta * .05f, 0.f, 1.f);
        if (activate && v.focus == 5)
            v.audio.settings = {};
        if (activate && v.focus == 6)
            done = true;
        v.audio.update(game, true, in.focused, GetFrameTime());
        if (done)
            v.audio.play("ui.cancel", 0, 0, false, true);
        else if (delta || activate)
            v.audio.play("ui.confirm", 0, 0, false, true);
        presenter.presentMenu(drawSettings, &v);
    }
    v.audio.persist();
}
} // namespace blobber
