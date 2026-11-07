# Action dock cleanup — 2026-11-07

Status: mechanically verified; user visual/interactive review pending.

- [Exploration](exploration.png): refreshed by the encounter-scoping follow-up;
  Targets is hidden and the dock status line remains absent.
- [Combat](combat.png): the same compact layout during a party turn.

Combat was captured with the hidden Debug `--interaction-prompts-review` check;
exploration now uses `blobber_capture_targeting`. No desktop input automation.
Tooltip delay is covered by deterministic
HUD tests at 349/350 ms, including control changes, leaving, focus and overlays;
these still captures do not establish interactive timing acceptance.
