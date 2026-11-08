# Game time and outdoor day/night

Status: implemented and mechanically verified, 2026-11-07.
[Six captures](screenshots/game-time/README.md) await user visual/interactive acceptance.

## Ownership and clock domains

`GameTime` is a small value containing elapsed game milliseconds, not a second
mutable timer. `CampaignRun::gameTime()` wraps the existing cumulative campaign
clock; `GameSession::gameTime()` delegates to its campaign or uses standalone
combat gameplay milliseconds. Existing campaign checkpoint fields and schema /
simulation 12/17 remain unchanged. Old saves derive exactly the same calendar as
new ones: elapsed zero is Day 1 08:00; days are 24 hours and roll over at midnight.
Calendar conversion divides before adding the eight-hour origin to avoid overflow.
Bounded formatting, duration constants and deadline helpers require no allocation.

Exploration remains 1:1; full combat rounds advance six seconds, successful short
and long rests one/eight hours. Existing pauses, loading and instantaneous travel
do not advance time. Animation/session milliseconds remain a different domain.
Effects retain location-local deadlines and existing travel rebasing; the global
clock is for calendar consumers, not a replacement deadline for local effects.
No offline progression, waiting UI, new rest restrictions or vendor inventory
implementation. Future vendors may store global deadlines and reconcile on access;
there is no speculative scheduler or callback storage.

## Presentation and interfaces

The HUD snapshots `GameTime`; map rendering formats `Day N HH:MM` into fixed
storage in the sidebar, below keys and above diagnostic controls. It does not
shrink the map or add a permanent HUD strip. Large supported dates must fit.

A pure `OutdoorLighting` profile uses whole game minutes and smoothstep ramps
05:00–07:00 and 17:00–19:00. Full day is identity; full night has ambient RGB
(0.30, 0.36, 0.50). The same multiplier shades ambient terrain, billboard light,
fog horizon and the existing sky panorama; spell illumination and emissive effects
remain additive and independent. The existing outdoor/indoor classification and
distance/fog formulas are preserved. Interior ambient is identity at all times.

`FrameInput` carries the profile (identity by default for low-level callers).
`GameView` derives it from session game time. Staged travel supplies an explicit
source-clock override before the destination belongs to the campaign. Profiles
are render inputs, not visibility inputs: color invalidates when the effective
profile changes; geometry, AO, picking and asset resources do not. Indoor profiles
resolve to identity. No allocations/uploads/new GPU targets in frame processing.

## Verification strategy

Focused CPU checks cover calendar rollover/limits/formatting, deadlines, fractional
updates and pauses, rest success/failure and expiry, round time, save/restore and
travel continuity with unchanged effect rebasing. Existing pre-change codec
output is tested without changing its schema. Owned small fixtures cover lighting
boundaries, continuity and allocation-free resolution. Hidden GPU checks cover
surface/billboard/sky behavior, interiors, local spell lights, color cache versus
AO/picking and staged lifecycle in both quality modes. Production evidence is at
most six images (city day/dusk/night, inn, dungeon and map); interactive acceptance
is left to the user. No OS input automation.

One low-priority intensive job, up to two build workers and one test process.
Disposable data belongs to the task's ignored agent-artifact run. Promote only
selected evidence, record commands/outcomes here, preview and apply run cleanup.

## Implementation and verification — 2026-11-07

Implemented the value API, HUD snapshot/map line, source-clock staging override
and time-only renderer profile/cache input described above. The three new shader
uniform locations are cached at resource preparation; existing geometry, picking,
AO, spell illumination and emissive presentation remain separate. Rest and round
durations use shared constants without changing their values. No save/content
fields, versions or asset artwork changed. The normal Debug game was rebuilt.

Passed focused checks (one Debug configuration, two low-priority build workers,
one test process, no benchmarks/stress or desktop input):

- `ctest --test-dir cmake-build-debug -R '^(game_time|campaign|campaign_save_smoke|recovery|map)$' --output-on-failure -j 1`:
  all five passed in 28.13 s. Includes calendar limits/midnight, allocation guards,
  fractional updates, paused/invalid updates, thinking time versus a full combat
  round, rejected/duplicate/successful rests, campaign and standalone clocks,
  travel cancellation/continuity, save/restore, portable effect remaining time
  and exact rest-expiry regressions in the existing recovery/campaign suites.
- Before rebuilding the core, a disposable harness linked the pre-change core and
  codec and exported an owned deterministic campaign after 37.25 seconds.
  `blobber_game_time_tests PRE_CHANGE_SAVE` then loaded it through the new build,
  checked the clock and re-encoded exactly the same bytes. Schema/simulation stay
  12/17; no player saves were used or rewritten. The test's optional input supports
  repeating this cross-build compatibility check; routine tests generate fixtures.
- `blobber_game_time_gpu_tests`: passed in a hidden OpenGL window, both quality
  modes. Owned tiny materials, sprites and sky cover expected RGB night ratios,
  sky/terrain/creature consistency, unchanged IDs, visibility and AO reuse, no C++
  frame allocations or GPU uploads for clock-only changes, active/staged
  isolation/cancel, spell illumination independence, unchanged inn/Underkeep
  interiors, complete resource release and final GL state.
- `blobber_dungeon_lighting_gpu_tests`: passed existing accepted-distance,
  surface/creature, allocation, lifecycle and unrelated-pack regressions.
- `blobber_review_game_time cmake-build-debug/assets NEW_OUTPUT`: six production
  captures, all visually inspected. Production inn and Underkeep world output
  compares pixel-identically between day and night. City differences stay inside
  the world viewport. The longest supported date uses 67 / 102 sidebar art units.
  Fixed review samples do not advance or alter a real campaign.
- `git diff --check`: passed. Only selected six images are retained in the linked
  gallery. Interactive review remains unverified and acceptance is pending.

GPU launches initially failed inside the sandbox because X11 was unavailable;
authorized retries used hidden application windows, without OS input/focus control,
with `OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2` and low priorities.
The initial GPU test build needed a test-only handle comparison correction before
the successful build; no runtime failure remained. Disposable logs, legacy fixture,
review exports and harness are cleaned after evidence promotion; no diagnostics
require retention.
