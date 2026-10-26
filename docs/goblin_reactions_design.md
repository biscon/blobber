# Goblin reactions — slice 06

Status: implemented, accepted and closed (2026-10-26), including impact timing
and diagonal front-slot review corrections. The user said: “I accept the slice,
finish it in the document”. No slice 07 work is included.

## Ownership and playback

ActorWorld continues to own damage timestamps, HP, life and occupancy. Death
releases occupancy immediately; dead records remain readable. NpcAnimator borrows
that owner and copies bounded clip timings at preparation. Animation never resolves
simulation, removes records, delays victory, or owns loot.

Append Hurt and Death to NpcAnimation, preserving existing enum values. Placement
loops remain Idle/Walk/Attack only. Reaction clips are optional for legacy assets,
but each present clip requires four directions and 1..4 frames with shared timing.
Hurt uses two 150 ms poses. Death uses 200/200/200/600 ms: three collapse poses,
then a final corpse hold; hide body and shadow at 1200 ms. Death always wins.
Among living cues, newest attack/damage wins, damage wins ties. Repeated damage
restarts hurt; an interrupted attack never resumes. Newer attacks may interrupt
hurt. Expiry returns to the underlying placement loop (idle in production).

Positive damage overlays white for the first 100 ms while reactions advance.
Zero damage and misses cause no hurt. Without hurt art preserve the old flash
behavior; without death art freeze the hit pose for 100 ms then hide. Directional
idle fallback is allowed only for Attack, never Hurt/Death. Existing front attack
art/timing are unchanged. The review correction below aligns enemy damage/MISS markers.

Sampling uses absolute paused session time, bounded O(instances * maxFrames)
work and fixed storage, independent of visibility or elapsed gaps. Removal hides
immediately. Victory/Continue retain the existing clock and do not wait for art.
Restart rebuilds session borrowers; diagnostic seeks remain possible. Snapshots
retain world/revision validation and both transition endpoints share one sample.

## Assets and preparation

Built-in imagegen authors eight reference-based transparent sheets (four hurt,
four death). Preserve exact prompts, reference paths, selected sheets and explicit
registration metadata in the goblin animation directory. Keep outfit, identity,
right-hand dagger, coherent four-view collapse and source alpha. No gore or new
attack artwork. Inspect source and runtime scale, handedness and grounding.

Extend the existing offline tool with optional canvas dimensions, output anchor
and explicit per-pose ground_y, plus measured split_y for unequal rows; old entries
keep existing defaults and lowest-ink
registration. New entries record explicit pivots and ground plane. One uniform
scale per sheet, calibrated against existing art; never normalize each pose by
its bounding box. Reject invalid geometry, empty frames, below-ground ink and
visible output clipping. Runtime frames are versioned; no generator is needed at
build/run time.

Hurt canvases remain 384x536, anchor (192,512). Death uses 768x536, anchor (384,512).
Optional clip-level width inherits appearance width when absent. Death width is
0.9 cells (bounded to 1); other clips remain bounded to 0.5. All retain appearance
height 0.65. This doubles horizontal padding without changing pixel/world scale.
The shared standing contact shadow continues using appearance width, not padding.

PreparedNpcs retains contiguous immutable pixels and bounded metadata, prepares
all directions/depths at startup, and uses effective clip width. Source plus
prepared colors plus shadow alpha retain the 160 MiB cap with explicit failure;
measured source storage is 49,397,760 bytes and prepared colors/shadow alpha
69,065,872 bytes: combined 118,463,632 bytes (112.98 MiB). No frame allocation.
Dead bodies remain alpha/wall occluders but not targets. Equal-depth dead bodies
draw before living replacements; other ordering remains far-to-near and stable.

## Verification and delivery

Generated fixture tests cover loader/placement rules, timing boundaries, missing
art compatibility, retriggers/interruption, hidden actors, pauses, removal,
large jumps, all directions, grounding, walls, same-subslot replacement, picking,
transitions, registration failures and budgets. Extend the C++ allocation guard
through all reaction stages. Build/run full Debug and Release CTest headlessly.

Validate shipped preparation and actual pixel memory separately. Extend hidden,
application-owned deterministic captures for every new pose/direction plus combat
victory, interruption, overlap and transitions. Inspect source/prepared art and
native/reduced captures; run the hidden GPU lifecycle check if available. Never
inject OS input or control windows/desktops. Report unavailable checks. Finish
with durable verification/review/handoff, mark Ready for review, and stop.

## Implemented interfaces and reproduction

`NpcClipImages::width` is zero to inherit the appearance canvas; positive JSON
`width` overrides it. `NpcAnimation` appends Hurt/Death without renumbering old
values. Existing ActorWorld APIs and NpcFrameSelection are reused. No actor cue
queue or additional steady-state allocation was introduced. The offline
`assets/npc_sheet` function is shared by the CLI and generated-fixture tests;
application entry points retain orchestration only.

The preparer uses explicit source splits, pivots and ground planes from
`assets/npcs/goblin/animation/sheets.json`. The initial left hurt candidate put the
dagger in the near hand; a recorded imagegen correction restored the far right
hand. Death sheets needed non-midpoint row splits to preserve the upper poses.
All original twenty frames reproduce unchanged. Selected sheets, 24 new frames
and prompt/reference provenance are versioned in the asset directory.

`--capture-reactions DIRECTORY` exports isolated four-direction hurt/death cases,
all legal movement transitions, corpse replacement and production combat outcomes.
It logs source/prepared byte usage and uses the existing hidden capture window.
Full `--capture` includes these cases too. Capture/export allocations remain
outside the frame guarantee. See [verification and user review](slice06_verification.md).

## Attack impact feedback

Review correction implemented 2026-10-25, accepted with slice 06 on 2026-10-26. Enemy attack
outcomes previously showed damage/MISS and the portrait highlight at calculation,
while the authored clip was still guarding. CommandController copies one impact
delay per NPC type from validated prepared timings at startup. The strike is the
middle frame (index count/2): sum preceding durations, giving 950 ms for the
existing four-pose goblin attack. Missing/single-frame clips have no delay. No
asset schema or simulation timing changes are needed. Player attacks have no
outgoing weapon clip; their markers remain immediate with the monster hurt/flash.
HP, life transitions and logs remain authoritative and immediate.

Keep eight pending entries separately from eight visible markers. Each entry owns
its recipient/outcome and intended impact time on paused session time; no actor
pointers or callbacks survive record(). Do not coalesce pending attacks on the
same recipient: automatic turns can resolve before an earlier strike. Exhaustion
drops the oldest pending impact. Refresh activates due entries in timestamp order,
replacing a visible marker on the same recipient only at impact. Display lifetime
is 1000 ms from impact; positive hits highlight the portrait for 150 ms from that
same time. A skipped interval never replays already-expired feedback. The resolved
outcome still appears if a later reaction interrupts the attack. Movement or return
to exploration cancels pending/visible feedback and highlights; restart rebuilds
the controller. Modal/focus/map pauses freeze the session clock. Fixed arrays keep
frame work bounded and allocation-free. Legacy/headless controllers without
prepared timings retain immediate feedback.

Verify strike boundaries against NpcAnimator using generated timings, hit/MISS,
multiple pending attacks, pause/resume, large skips, cancellation, expiration and
legacy fallback. Exercise the delayed path under the frame allocation guard.
Capture authored attack before/at strike in the hidden application-owned window
and inspect alongside the recipient splash. Run Debug/Release correctness checks.

## Slice 07 turn-handoff correction

Implemented and accepted with slice 07 on 2026-10-26: [attack presentation handoff](goblin_ai_design.md#review-correction-finish-attack-presentation-before-advancing).
Combat now copies clip durations/strike offsets at startup and holds each resolved
attack through the later of clip expiry and marker expiry. The shared marker
constant and impact helper keep simulation pacing and UI aligned. Existing
pending-feedback capacity remains useful for diagnostic/external outcomes; normal
turns no longer overlap these presentations. HP and damage resolution remain
immediate and independent of drawing. Production session and HUD borrowers receive
the same prepared timing metadata. Missing metadata uses an immediate strike plus
the one-second marker hold. The original slice 06 acceptance remains unchanged.
