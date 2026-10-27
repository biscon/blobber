# Loot and recovery — slice 12

Status: **implemented; Ready for review**, 2026-10-27.
The user approved the implementation plan with “Implement the plan.” The first
checkpoint is an image-generated camp-screen mockup, followed by explicit visual
approval **before coding or producing animation frames**. This is the user's
requested gate, not another request to approve the implementation plan.

## Scope and decisions

Implement corpse-independent floor loot, checked collection, gold and equipment
drops, authored camp supplies, short/long rests, and camp presentation. Preserve
combat rules, accepted Inventory / Character controls, discrete movement, fixed
1920×1080 presentation, prepared dungeon rendering and allocation-free frame work.
No shops, hub, dungeon progression, XP/level advancement, persistence, player item
dropping or recovery of fired arrows. The user is considering a future hub with
shops and progressively harder dungeon levels; that direction is context, not
authorization to implement those systems in this slice.

### Loot and acquisition

- Authoritative death creates one set of drops in the death cell, independently
  of corpse animation, visibility or removal. Removing a living actor drops nothing.
- Direct floor clicks collect the whole displayed stack into the selected gameplay
  character's bag, never the last character viewed in the fullscreen sheet or an
  automatically substituted recipient.
- Require a conscious able recipient, stationary safe Exploration, no pending
  attack, and visible loot in the party cell or three reachable front cells.
  Reuse threat and corner-blocking checks. Pickup is free; Approach, initiative,
  movement and result screens reject it.
- Stage acquisition with Party's authoritative inventory machinery: fill matching
  stacks then empty cells in row order, validate weight/ownership/capacity, commit
  before removing floor ownership. No partial pickup; failure preserves both owners.
- Melee goblins drop a scimitar, leather armor and shield. Archers drop shortbow,
  leather armor, shield and ten arrows. Each drops 1d6 gold and at most one potion:
  15% healing, 5% mana, 80% none. Preserve NPC stats/AI; document the archer shield
  and historical dagger-art/scimitar-rules adaptations. Carrying gear does not
  require proficiency; equipping retains existing checks.
- Gold is weighted, transferable inventory content: 0.02 lb per coin, maximum
  1,000 per stack. Gold profiles are authored for current humanoids; other future
  monsters do not automatically acquire coins. The 1d6 amount is this demo's
  CR-1/4 balance choice, not an SRD formula.
- Camp kits never occur in random drops. Each weighs 8 lb, stacks to five, and
  funds one entire-party field long rest. Start with one in Alden's bag and place
  one each at normal-dungeon cells (7,15) and (19,19); none respawn.

### Recovery

Normal dungeon play has no sanctuary. Explicitly authored sanctuary cells allow
free long rests in the recovery demo/tests and can later support a hub. Neither
spawn points nor all safe cells are implicitly sanctuaries.

Both rests require stationary safe Exploration, settled action presentation, at
least one conscious able member, and no dying ally. Stable allies may recover;
dead allies receive no benefits and are never resurrected. Reject a rest with
no recovery benefit without spending supplies/RNG or advancing time.

Short rest advances one gameplay hour, spends individually selected Hit Dice for
class-die + Constitution healing (nonnegative, capped at maximum HP), and refreshes
Second Wind. Hit Dice expenditure defaults to zero. No automatic mana recovery or
Arcane Recovery refresh. Long rest advances eight gameplay hours, restores living
members' HP/mana, restores max(1, floor(level/2)) spent Hit Dice up to their maximum,
and refreshes Second Wind and Arcane Recovery. Arcane Recovery retains its manual
safe-exploration activation. No daily long-rest limit.

Preview the exact supply owner/cost; field rests consume from bags in character-ID
then cell order, including bags of downed/dead party members under existing safe
inventory ownership rules. Sanctuary rests spend none. Rest time advances effect
deadlines and cooldowns; monsters remain in place with no patrol/AI catch-up.

## Ownership, interfaces and bounds

GameSession owns LootWorld. Reserve eight possible drop slots per actor plus 64
authored ground stacks: 2,112 lifetime slots. Slots never recycle; typed world/slot
handles reject foreign, collected and stale references. Validate content bounds
at loading rather than silently discarding runtime rewards.

Actor state retains an authoritative death marker after removal. Reconcile deaths
before publishing session results or accepting pickup; each actor activates its
reserved drops once. Prepare seeded outcomes per actor at session construction,
independent of combat/AI RNG and kill order. Repeated clicks or rendering never
reroll rewards. Snapshots are read-only; UI carries checked pickup requests.

Party gains staged acquisition and checked supply consumption, reusing full
inventory/stat validation. No character strings are copied in frame processing.
GameSession exposes RestRequest, RestPreview and RestResult with session identity
and single-use rest token. Shared validation drives previews and commits; all
recovery, resource costs and elapsed time commit exactly once on success.

Use a dedicated gameplay-time advance, not repeated ordinary session updates
(which clamp elapsed time and run AI). Preserve Effects deadline ordering and
Mage Armor expiry, bounded event delivery and existing borrowed-event lifetimes.
Keep presentation time and cosmetic camp animation separate from rest duration.

Schema 6 adds bounded monster loot, currency/supply content and scimitar support.
Strict schemas 1–5 remain readable with no legacy loot by default. Separate bounded
level recovery metadata supports 16 sanctuary cells and 64 placed stacks.
Keep loot/recovery implementation in cohesive subsystem modules; application code
only loads, connects and schedules owners. Restart rebuilds session borrowers.

## Presentation and visual approval

First use the built-in imagegen skill to generate a mockup using the accepted HUD
as reference. Replace only the dungeon viewport with a cozy bonfire scene and
four gathered silhouettes, keeping the portrait rail/log around it. Include
Short Rest / Long Rest selection, short-rest Hit Dice choices, recovery summaries,
supply cost/source, Start Rest and Cancel. Show the mockup and wait for the user's
explicit visual approval before writing gameplay/UI code or making animation frames.

Following approval, produce four registered frames with stable background/camera/
silhouettes and varying flames/light. Preserve masters and prompts. Preload native
UI artwork with RAII; do not introduce frame-time image generation or loading.
Enter via a labelled Rest entry in the pause menu or R; cancel pending targeting
and use an exclusive paused screen. Surrounding portraits/log remain visible.
Start commits once, plays two four-frame loops over about 1.6 seconds, then briefly
fades back to the dungeon. Cancel before Start costs nothing; Escape after commit
only skips cosmetics. Focus loss pauses presentation; held input cannot retrigger
or click through. Feature descriptions/Character sheet explain the recovery rules.

Floor loot uses six individually clickable, floor-anchored sprites per cell in
deterministic order. Wheel paging over the cell exposes more, with a page/count
indicator. Use a compact foreground arrangement in the party cell, preserving
visibility against adjacent walls. Hover names item, quantity, recipient and any
rejection reason. Armed spell/item targeting takes precedence.

Reuse item art and generate missing gold, kit and scimitar icons after the camp
checkpoint. Bake bounded floor-size derivatives offline; retain masters/provenance.
Integrate with wall/body/corpse occlusion, strafe/turn transitions and revision-based
frame invalidation. Typed floor picking must preserve enemy targeting and remain
disabled during transitions and exclusive overlays. Do not change camera/FOV.

## Verification and delivery

Use deterministic, test-owned catalogs, maps and generated art. Add headless
loot/recovery suites for death/removal ordering, simultaneous deaths, independent
reward RNG, duplicate/stale/foreign pickup, recipient identity, full bags, merge
rollback, weight boundaries and exactly-once supply use. Cover safety/movement,
stable/dying/dead allies, resource depletion, cancellation, repeated requests,
feature recovery, HP/mana caps, cooldown/effect expiry and frozen NPC positions.

Rendering/input tests cover every heading, adjacent-wall foreground, diagonal cells,
wall/body/alpha occlusion, paging, transitions, modal/focus/held input isolation and
native/reduced/letterboxed coordinates. Extend allocation guards over first and
repeated loot activation, acquisition, rest previews/commits, paging/rendering and
camp playback. Keep maximum-capacity stress separate from routine tests.

Run Debug/Release correctness suites, focused sanitizers, prepared-asset
reproducibility and hidden GPU lifecycle/capture checks. Add --recovery-demo with
separated encounters/authored supplies and --capture-recovery using in-process
production input paths. Inspect floor/camp captures and record unavailable checks.
Never automate OS input, focus or desktops.

Finish at Ready for review with a durable verification/handoff report, actual
commands/results and review recipe. User acceptance is pending; do not start 13.

## Current checkpoint

2026-10-27: plan approved; documentation recorded; [camp mockup v1](mockups/slice12/camp-screen-v1.png)
generated with the built-in imagegen tool and visually inspected. [Exact prompt and
provenance](mockups/slice12/camp-screen-v1.prompt.json) are retained. This is a
concept image, not an application capture or runtime asset. Four party figures,
Short/Long Rest, per-character Hit Dice controls, Start/Cancel, portraits and log
are visible. The three-kit count is a diagnostic example, not three starting kits;
production values will come from authoritative state. Long Rest will replace the
Hit Dice controls with the recovery/supply summary. Production text remains live
native HUD text, not text baked into the painting.

**Mockup accepted 2026-10-27:** the user said “yes that looks very nice, continue”.
The visual gate is satisfied; implementation and animation production are authorized.
Implementation and runtime verification are complete; see the
[verification and review handoff](slice12_verification.md). User acceptance of the
implementation is pending.

## Implemented data and rendering details

`LootWorld` reserves eight slots for each of 256 possible actors and 64 authored
stacks. It uses the actor world's nonrecycling identity plus slot, and reconciles
only after actor revision changes. Seeded actor-local rewards never consume the
combat RNG. `ActorState::hasDeath` survives removal. The inventory transaction
copies fixed storage, validates merged/new stacks and derived weight, then commits
before the floor reservation is collected. Party ownership and gameplay selection
remain independent of the character currently viewed in the sheet.

`GameSession::previewRest/rest` share safety, benefit and cost validation. A
world-bound single-use token prevents replay. `Party::recover` performs all failure
checks before spending supplies or consuming RNG. `CombatController::restTime`
advances effect/cooldown deadlines without running AI or advancing presentation.
`CommandController` owns paused preview choices and a separate focused cosmetic
clock. The application consumes the frame on which playback ends to prevent
closing-frame click-through. Rest implementation is split across recovery,
session recovery and recovery command modules rather than application code.

`PreparedLoot` stores nine atlas regions at every integer square size from 4 to
48: 342,090 RGBA pixels, 1,368,360 bytes plus at most 64 item mappings. The production
catalog adds 18 mapping bytes. The separate CMake bake reads the retained transparent
master, writes a checked `loot.bin` through the existing binary format machinery,
and never resizes images in normal frame work. Loading reserves its bytes from the
shared prepared-memory budget before loading NPCs. The nine supported demo visuals
are scimitar, leather, shield, bow, arrows, coins, healing/mana potions and kit;
other future content needs its own visual mapping.

Composition gathers active slots into fixed scratch arrays, sorts by cell/slot,
selects six per cell, then sorts visible sprites back to front. Work is bounded by
2,112 slots (O(n log n)) plus visible sprite pixels. Current-cell sprites use a
compact foreground layout; front-diagonal groups are fitted within the existing
FOV. Per-column wall coverage and existing per-pixel NPC/corpse IDs with projected
body depth prevent drawing/picking through occluders. The same uint16 picking
pixels encode actor slots 0–255 and loot slots offset by 256. Fixed region records
allow wheel paging over the visible cluster; exact opaque sprite pixels are required
for clicks. The hover line reports item count and page. No extra framebuffer depth
allocation is introduced. Immutable scene ownership includes scratch picking for
transition composition when no public picking buffer is requested; composition
remains single-threaded like the existing pipeline.

Frame cache keys include loot world/revision and page revision. Turns/steps retain
loot in prepared transition images and expose no picks until stationary. Native
camp and missing item icons use two startup textures in the existing GPU registry;
painting uses one fixed background frame and a varying central fire region from
four retained frames, keeping the rest of the scene stable. All text stays live.
Masters and generation prompts are retained under `assets/ui/camp` and `assets/ui/items`.

`rules/recovery.json` is bounded to 64 KiB, depth eight, 16 unique walkable sanctuary
cells and 64 valid ground stacks. Schema 6 rejects unknown IDs, oversized drops and
bad percentile ranges. Catalog schemas 1–5 remain strict and readable without loot.
`--recovery-demo` places three separated encounters in the normal map with a demo-only
spawn sanctuary. Normal gameplay loads the authored no-sanctuary configuration.
