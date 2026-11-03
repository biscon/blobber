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
    bool dragging = false;
};
UiRect control(unsigned row, bool plus) {
    return {plus ? 440 : 365, 58 + int(row) * 31, 48, 25};
}
constexpr UiRect musicSlider{260, 218, 168, 22}, musicToggle{440, 218, 48, 25};
constexpr UiRect defaults{145, 278, 160, 28}, back{335, 278, 160, 28};
void drawSettings(void* context, const UiFonts& fonts, const UiArt& art) {
    const auto& v = *static_cast<SettingsView*>(context);
    HudPainter p(&fonts, &art);
    p.overlayBox({125, 20, 390, 320});
    p.centered("SETTINGS / AUDIO", {140, 28, 360, 25}, hudInk::gold, UiTextStyle::Control);
    const char* labels[]{"Master", "Effects", "Ambience", "UI", "Party footsteps"};
    for (unsigned i = 0; i < 5; ++i) {
        char value[24];
        std::snprintf(value, sizeof(value), "%d%%", int(v.audio.settings.values[i] * 100 + .5f));
        p.text(labels[i], 145, 65 + i * 31, hudInk::text);
        p.text(value, 307, 65 + i * 31, hudInk::gold);
        for (bool plus : {false, true}) {
            auto r = control(i, plus);
            p.box(r, v.focus == i ? hudInk::teal : hudInk::edge);
            p.centered(plus ? "+" : "-", r, hudInk::text);
        }
    }
    p.text("Combat music", 145, 225);
    p.box(musicSlider, v.focus == 5 ? hudInk::teal : hudInk::edge);
    p.fill({musicSlider.x + 4, musicSlider.y + 9, musicSlider.width - 8, 4}, hudInk::muted);
    const int knob = musicSlider.x + 4 + int((musicSlider.width - 8) * v.audio.settings.musicVolume);
    p.fill({knob - 2, musicSlider.y + 4, 4, 14}, hudInk::teal);
    char value[32];
    std::snprintf(value, sizeof(value), "%d%%", int(v.audio.settings.musicVolume * 100 + .5f));
    p.text(value, 260, 246, hudInk::gold, UiTextStyle::Small);
    p.box(musicToggle, v.focus == 6 ? hudInk::teal : hudInk::edge);
    p.centered(v.audio.settings.musicEnabled ? "On" : "Off", musicToggle);
    p.box(defaults, v.focus == 7 ? hudInk::teal : hudInk::edge);
    p.centered("Reset defaults", defaults);
    p.box(back, v.focus == 8 ? hudInk::teal : hudInk::edge);
    p.centered("Back", back);
    p.text("Arrows adjust / Tab selects / Escape saves", 145, 320, hudInk::muted, UiTextStyle::Small);
    p.buttonFeedback(v.press.feedback);
}
} // namespace
void captureAudioSettings(Presenter& presenter, const std::filesystem::path& path) {
    SettingsView view{*presenter.audio, 5, {}};
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
        // Slider owns its pointer gesture; button activation remains release-based.
        if (in.has(UiKey::Click) && in.pointer && musicSlider.contains(*in.pointer))
            v.dragging = true;
        if (!in.focused || !armed)
            v.dragging = false;
        if (v.dragging) {
            if (in.pointer) {
                v.focus = 5;
                v.audio.settings.musicVolume =
                    std::clamp((in.pointer->x - musicSlider.x - 4) / (musicSlider.width - 8), 0.f, 1.f);
            }
            in.pressed[std::size_t(UiKey::Click)] = false;
            if (in.pointerReleased || !in.pointerHeld)
                v.dragging = false;
        }
        ButtonTargets targets;
        for (unsigned i = 0; i < 5; ++i) {
            targets.add(control(i, false));
            targets.add(control(i, true));
        }
        targets.add(musicToggle);
        targets.add(defaults);
        targets.add(back);
        in = v.press.route(in, targets);
        if (in.has(UiKey::Cancel))
            done = true;
        if (in.has(UiKey::Down) || in.has(UiKey::Tab))
            v.focus = (v.focus + 1) % 9;
        if (in.has(UiKey::Up))
            v.focus = (v.focus + 8) % 9;
        int delta = in.has(UiKey::Right) ? 1 : in.has(UiKey::Left) ? -1 : 0;
        bool activate = in.has(UiKey::Confirm);
        if (in.has(UiKey::Click) && in.pointer) {
            for (unsigned i = 0; i < 5; ++i)
                for (bool plus : {false, true})
                    if (control(i, plus).contains(*in.pointer)) {
                        v.focus = i;
                        delta = plus ? 1 : -1;
                    }
            if (musicToggle.contains(*in.pointer)) {
                v.focus = 6;
                activate = true;
            }
            if (defaults.contains(*in.pointer)) {
                v.focus = 7;
                activate = true;
            }
            if (back.contains(*in.pointer)) {
                v.focus = 8;
                activate = true;
            }
        }
        if (v.focus < 5 && delta)
            v.audio.settings.values[v.focus] =
                std::clamp(v.audio.settings.values[v.focus] + delta * .05f, 0.f, 1.f);
        if (v.focus == 5 && delta)
            v.audio.settings.musicVolume = std::clamp(v.audio.settings.musicVolume + delta * .05f, 0.f, 1.f);
        if (v.focus == 6 && (activate || delta))
            v.audio.settings.musicEnabled = delta ? delta > 0 : !v.audio.settings.musicEnabled;
        if (activate && v.focus == 7)
            v.audio.settings = {};
        if (activate && v.focus == 8)
            done = true;
        v.audio.update(game, true, in.focused, GetFrameTime(), true, false);
        if (done)
            v.audio.play("ui.cancel", 0, 0, false, true);
        else if (delta || activate)
            v.audio.play("ui.confirm", 0, 0, false, true);
        presenter.presentMenu(drawSettings, &v);
    }
    v.audio.persist();
}
} // namespace blobber
