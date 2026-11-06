# Shared Settings — borderless fullscreen

Implemented scope, 2026-11-06: add a checkbox labelled "Borderless fullscreen"
to the existing Settings screen shared by main and pause menus. Keep the five
audio controls, music controls and reset/back actions. Retitle the panel Settings.

The existing settings record/file gains an optional `borderless_fullscreen`
boolean, default false for old files. Retain its existing owner and persistence
path; no separate configuration service. Audio consumes only its existing fields.
Normal menu startup applies the loaded display preference after creating the
window; silent audio mode still loads settings. Hidden capture tools do not apply
the startup preference. The checkbox reflects raylib's actual window state.

A small platform function sets the desired mode using the pinned raylib
`ToggleBorderlessWindowed()` API only when the flag differs. Raylib owns monitor
selection and restoring the former window geometry. Toggle on pointer release or
Enter; Left disables and Right enables. Reset defaults restores windowed mode as
well as audio defaults. Persist through the existing Settings exit path. Gameplay
stays paused inside the existing settings loop; cancel pending pointer gestures
after a window mode change. Fixed 1920×1080 composition and letterboxing remain.

Verify old-file defaults and enabled/disabled round trips with the existing
headless settings tests, and build the normal application against pinned raylib.
Do not run the toggle or desktop input automation during verification: window mode,
focus, monitor/HiDPI behavior and interactive menu layout remain for user review.

Verification: normal Debug application built against the pinned raylib header and
GLFW implementation, including `ToggleBorderlessWindowed` and
`FLAG_BORDERLESS_WINDOWED_MODE`. The focused `audio` suite passes (0.01s), covering
enabled/disabled persistence and old-file defaults without losing audio settings.
The combined `targeting|hud|audio` run passes 3/3 in 0.11s. Main-menu and pause-menu
callers both use the updated shared screen. No live window toggle, screenshot or
OS input was performed; desktop behavior remains unverified. Temporary build/test
output was cleaned with no retained exceptions.
