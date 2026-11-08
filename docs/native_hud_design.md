# Native-resolution HUD and impact alerts

## Action dock hover and layout cleanup — 2026-11-07

Action, travel and rest button tooltips wait 350 ms on the same control, using
the session presentation clock while gameplay is unpaused. Moving within a button
preserves dwell; changing controls, leaving, opening a modal or suspending input
restarts it. CommandController owns one bounded kind/index/timestamp record; enemy
hover retains its separate 250 ms/attack suppression policy, and portrait popups
remain disabled. No frame allocations or new resources are needed.

Targets uses the action-button width (34 logical pixels) and centered text in the
existing shared draw/hit rectangle. Remove the adjacent dock readout, including
selected-target and combat messages, and stop drawing underlying hints where this
button is available. Target identity, eligibility and rejection feedback remain in
the opened picker; spell-recipient controls and other dock modes are unchanged.
Focused HUD tests cover dwell boundaries/reset and targeting tests cover picker
input. Hidden GPU review checks drawing when available; desktop input stays manual.

Verification: Debug game, HUD/targeting tests and world integration GPU runner
rebuilt with two workers at low CPU/I/O priority. The focused
`ctest --test-dir cmake-build-debug -j 1 --output-on-failure -R '^(hud|targeting)$'`
passes (0.11 s). The hidden production `--interaction-prompts-review` check passes
after granting display access (the sandbox attempt could not open X11). It checks
OpenGL errors and resource cleanup; exploration/combat captures were inspected
for width, centered text and the absent status line. [Two selected images](screenshots/action-dock/README.md)
remain pending user review; other captures and temporary build/test logs were cleaned.

## Party portrait hover cleanup — 2026-11-07

Party-card and details-button hover no longer display portrait tooltips.
CommandController suppresses their tooltip visibility while retaining hover identity
and targeting eligibility. Selection, inspection, Aid and spell-recipient clicks
keep their existing input routes; status and concentration tooltips remain available.
This uses existing snapshot storage and adds no allocations or rendering resources.
Verification uses the focused HUD and targeting checks; desktop review stays manual.

Verification: Debug application and `blobber_hud_tests`/`blobber_targeting_tests`
rebuilt with the two-worker preset at low CPU/I/O priority.
`ctest --test-dir cmake-build-debug -j 1 --output-on-failure -R '^(hud|targeting)$'`
passes both suites. No GPU capture or interactive review was performed for this
visibility-only change. Disposable build/test logs were cleaned with no exceptions.

Status: implemented, **accepted and closed**, 2026-10-27. The user explicitly
accepted slice 10 including this correction. Slice 11 remains Planned/unstarted.

## Rendering and ownership

Presenter owns a point-filtered dungeon texture/base target and a transparent
1920×1080 native UI target, fitted with bilinear sampling. Existing logical UI
coordinates and input mapping remain unchanged. HudPainter converts geometry to
native pixels; fonts already use native coordinates. One ordered UI pass draws
artwork, geometry and text together, so modal backgrounds cover all three.
World selection contours retain picking-mask alignment with native-width strokes.
The dungeon's prepared pixels, projection and discrete movement do not change.

UiArt owns one startup-prepared 2048×1024 RGBA atlas through GpuResources. Twenty-nine
256-pixel cells hold portraits up to 216×252 and icons up to 108×108, with two-pixel
edge extrusion. Sources remain intact. Portraits crop centrally to destinations;
icons aspect-fit. Source decoding/resampling occurs only at startup. The atlas is
8 MiB, with no retained decoded source images or per-frame allocations. The old
640×360 HUD target is removed. Code-native shapes supply thin borders, checkmarks,
darts and a clean eight-tooth gold cog; spell icons get a separate dark frame.
The Class Features correction adds one shared laurel emblem in the existing atlas;
its size and resource ownership are unchanged.

## Combat timing

Delayed outgoing attacks alert only at authoritative impact, before damage, for a
valid living target. Misses and zero damage alert; canceled/gone targets do not.
The existing radius-three local flood uses the current target cell and current
party position. Lethal hits therefore alert surviving neighbors. Different missile
recipients alert at their own deadlines, subject to normal local propagation.
Instant hostile effects still alert immediately. Normal sight detection remains
independent. Costs, saved rolls, flight, effect ordering and turn clocks are retained.

## Verification

Generated GPU artwork checks native detail, fitting/cropping, padding and cleanup.
Hidden native/reduced captures cover the picker, dock, portraits/inspector,
initiative, targeting, cog and overlapping modal layers. No OS input automation.
CPU regressions cover awareness before/at impact, misses, lethal hits, cancellation,
gone targets, staggered missiles, normal sight, pauses and coarse updates. Run full
Debug/Release, focused sanitizers and allocation guards; retain review evidence.

Actual execution results and retained captures: [verification](verification/archive/native_hud_verification.md).
The atlas replaces the former 256×448 atlas (8,388,608 versus 458,752 bytes).
Removing the 640×360 render target saves its 921,600-byte color attachment plus
its depth storage. GPU driver allocations are outside engine allocation claims.
GameSession retains ordinary sight/contact detection while flights are pending;
approach-only death-save scheduling begins when the impact actually starts approach.

## B01 review adjustment — 2026-10-31

The initiative strip now anchors to the dungeon viewport’s right edge with a
six-logical-unit inset (18 native pixels), retaining its existing top edge, bounded
queue and portrait ordering. Its width follows the remaining queue, so the right
edge remains fixed as turns advance. This clears the central monster/health-bar
area. [B01 review evidence](verification/archive/b01_verification.md#user-review-adjustment--size-and-initiative-2026-10-31)
records the smaller skeleton metadata and fresh GPU captures. No per-frame storage,
input regions, render targets or resource ownership changed.

The B01 health-bar centering correction uses `TargetAnchor::badgeX`, the existing
projected body/ground axis, rather than the visible alpha-bound center (`x`), which
includes outstretched weapons and partial occlusion. Keep the current visible-top
height, viewport clamping and depth clipping. This shares the stable horizontal
anchor already used by status indicators; no new per-frame data or rendering pass.
Verify on a hidden production skeleton hover capture and the existing HUD and
visibility checks.

## Viewport interaction prompts — 2026-11-05

Status: implemented and mechanically verified; user visual acceptance pending.

CommandController owns bounded prompt strings and selects only accepted interactions
from the live session in safe, settled Exploration. Movement, door animation, pending
attacks, local hazards, combat, focus loss, overlays and spell/item targeting hide
prompts. Blocked attempts retain authoritative validation and plain-language log
feedback. A usable hovered loot bag takes precedence over the existing doorway,
fixture and chest selection. Terminal guidance remains in result/dock UI.
The existing two 192-byte buffers remain controller-owned. A transient checked
LootId preserves hover across the application's second refresh, revalidates
world/generation/access and clears on overlays or suspension. No new persistent
state, container growth or resource ownership is introduced.

HudPainter draws exactly one prompt in the native UI pass: white Display text
(27-pixel DejaVu Serif Bold, matching CHOOSE AN ACTION), horizontally centered in
the dungeon viewport, visible ink bottom 24 native pixels above its lower edge.
A near-black copy two native pixels down/right supplies contrast. There is no
background rectangle, new atlas or frame allocation. Labels are [F] Open/Close,
Inspect, Take, Enter inn/Leave inn, Enter dungeon, Ascend/Descend; loot uses
[Click] Loot, preserving mouse-only browsing. No interaction rules or save schemas
change.

Player UI text and application error boundaries are audited for engine terminology.
Routine messages describe player actions; low-level exceptions are logged to stderr
and replaced at UI boundaries with readable failures. Campaign seed reporting stays
in diagnostics rather than ordinary gameplay logs. Loading screens say Loading or
Travelling instead of exposing preparation resource names. Sanctuary loses its
internal "hub" suffix. Development feature notes are removed from character creation
and advancement.

`ui/player_text.hpp` supplies borrowed static creature labels for the eight current
catalog species (including Goblin Archer, Goblin Guardian, Goblin Shaman and Giant
Rat); unknown species display Creature. Logs, tooltips, target rows and arena names
share this bounded, allocation-free mapping, and arena search matches the displayed
names. A shared Lesser Restoration description omits an old unsupported-feature note
in spellbooks and creation/level-up pickers. The authored catalog, content fingerprints,
simulation identity and save schema remain unchanged.

Verification (2026-11-05): the Debug game and selected tests build with two workers,
at low CPU/I/O priority. The focused 14-suite integration pass below passed in
29.91 seconds, including the 20.96-second save smoke test. The first development
pass exposed two test issues (discovery after restored pose and an outdated expected
travel reason), both corrected before the passing integration pass.

```sh
ctest --test-dir cmake-build-debug -j 1 --output-on-failure -R '^(fixtures|hud|commands|dressing|campaign|inn|recovery|inventory|progression|progression_allocations|menu|creator|campaign_save_smoke|gameplay_startup)$'
# After the final player-name/description cleanup, only affected callers were rerun:
ctest --test-dir cmake-build-debug -j 1 --output-on-failure -R '^(targeting|hud|commands|damage|shaman|campaign|inn|creator|progression_allocations|arena)$'
# Pass a new directory inside the current agent-artifact run:
cmake-build-debug/blobber_world_integration_gpu_tests cmake-build-debug/assets RUN_DIRECTORY/captures --interaction-prompts-review
```

The ten follow-up suites pass in 7.56 seconds. Coverage includes exact action labels,
every non-Exploration combat phase, live phase versus stale HUD state, movement and
door animation, hazards, unfinished action presentation versus cooldown, overlays,
focus, locked/sealed actions, readable rejection feedback, loot priority and checked
identity across both frame refreshes. The existing allocation guard adds 32 repeated
door/loot refresh cycles and reports zero engine-owned C++ allocations. Targeting
regressions exercise placeholder-free guardian labels; arena search covers display
names and existing identifiers.

Hidden 1920×1080 OpenGL captures on NVIDIA RTX 3060 pass: readable white glyphs,
centering, bottom margin, sparse text without a background bar, GL error checks and
world-resource release. Compared with prompt-free baselines, changed bounds are
678,852–851,875 (city), 706,852–823,876 (door), and 686,852–843,875 (loot), including
the offset shadow. Active two-goblin turn combat beside an open door has no prompt.
Inspected all four cases; [selected captures](screenshots/interaction-prompts/README.md)
await user acceptance. The initial sandboxed display connection was unavailable;
the authorized hidden OpenGL runner succeeded with display access. No OS input,
window selection, focus automation, benchmark or additional build configuration was
used. Driver/internal C allocations are outside the C++ allocation guard's claim.

Four selected PNGs are retained; raw baselines, superseded captures and run logs are
disposable and cleaned after handoff preparation. No diagnostic retention exception
is needed. Source artwork, prepared build caches and user saves remain intact.


## Compact initiative strip — 2026-11-06

User-requested layout correction: halve the strip's logical width and height
(27 + 15 per queued participant, height 17), keep its existing six-unit right
inset, and align its top directly to `viewY`. Halve portrait size to 12×12 logical
units (36×36 native pixels). Retain native Small text and one-native-pixel borders
for legibility, fitting round/actor-number/death-save labels into the compact
layout. Queue ordering, active-turn emphasis and combat behavior are unchanged.
This reuses the existing painter, portrait atlas and fixed bounded queue; no new
resources, state or input regions. Build the application and run the focused HUD
suite; leave in-game visual review to the user, without new screenshots.

Round text uses the compact `RND` label and bounded width; enemy numbers and SAVE
labels retain native Small text. Verification: normal Debug application rebuilt;
`ctest --test-dir cmake-build-debug --output-on-failure -j1 -R '^hud$'` passed
(0.03s), as did formatting/diff checks. No GPU capture or interactive review was
performed. Temporary build/test output was cleaned with no retained exceptions.

Initiative follow-up (2026-11-06): remove internal enemy slot numbers from portraits.
Keep portrait order, current-turn border, round label and death-save indicator.
Normal Debug application rebuilt and the focused HUD test passed (0.03s).
No new captures; shared task output was cleaned.
