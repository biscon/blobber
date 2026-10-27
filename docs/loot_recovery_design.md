# Loot and recovery — slice 12

Status: **Accepted and closed**, 2026-10-27.
The user explicitly accepted slice 12 after the Drop, Take All auto-close and
top-bar Rest follow-up: “thats better, record the slice as accepted in the doc”.
The user accepted the camp mockup before the original implementation, then accepted
the loot-dialog mockup with the corrections recorded below. This document describes
the corrected system; the initial mockup checkpoint is retained as history.

## Scope and decisions

Implement corpse-independent floor loot, checked collection, gold and equipment
drops, authored camp supplies, short/long rests, and camp presentation. Preserve
combat rules, accepted Inventory / Character controls, discrete movement, fixed
1920×1080 presentation, prepared dungeon rendering and allocation-free frame work.
No shops, hub, dungeon progression, XP/level advancement, persistence, or
recovery of fired arrows. The user is considering a future hub with
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
authored ground stacks: 2,112 lifetime reservations that never recycle. Another 256
player-drop slots recycle with checked generations, for 2,368 total entries. Typed
world/slot/generation handles reject foreign, collected and stale references. Validate content bounds
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
Enter via the small campfire icon beside Settings in the top bar, a labelled pause-menu entry
or R; cancel pending targeting
and use an exclusive paused screen. Surrounding portraits/log remain visible.
Start commits once, plays two four-frame loops over about 1.6 seconds, then briefly
fades back to the dungeon. Cancel before Start costs nothing; Escape after commit
only skips cosmetics. Focus loss pauses presentation; held input cannot retrigger
or click through. Feature descriptions/Character sheet explain the recovery rules.

Floor loot uses one floor-anchored bag per nonempty cell. Clicking it opens a
paused ten-slot grid with wheel/page controls, hover stats, per-stack pickup, Take
All and Close/Escape. Portrait clicks change the recipient without closing. The
info panel stays inside its parent, top-aligned with the grid; Inventory and Loot
share one slot painter for borders, padding and quantity labels. Leftovers retain
the bag; final collection removes it. Armed spell/item targeting takes precedence.

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

Implementation finished at Ready for review with a durable verification/handoff
report, actual commands/results and review recipe. The user accepted the completed
slice and corrections on 2026-10-27. Slice 13 is next eligible for planning, unstarted.

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

`PreparedLoot` stores one transparent bag at every integer square size from 4 to
64: 89,426 RGBA pixels, 357,704 bytes, with no catalog-to-icon mapping. The separate
CMake bake reads `assets/ui/items/loot-bag.png`, writes checked `BLBLOOT2` content to
`loot.bin`, and never resizes images in normal frame work. The new signature rejects
old individual-item banks. Loading reserves its bytes from the shared prepared-memory
budget before NPC loading. UI item icons still come from the existing inventory art.

Composition gathers active slots into fixed scratch arrays, sorts by cell/slot,
selects one live representative per cell, then sorts visible bags back to front.
Work is bounded by 2,368 slots (O(n log n)) plus visible sprite pixels. Every size
uses the same projected quarter-cell width, capped at 64; the current-cell floor
anchor is clamped to keep the bag onscreen, without shrinking it. Front diagonals
fit the existing FOV. Per-column walls and per-pixel NPC/corpse IDs with projected
body depth prevent drawing/picking through occluders. The same uint16 picking pixels
encode actors 0–255 and representative loot slots offset by 256. Only visible opaque
bag pixels accept clicks. A stale representative is checked before opening; removing
it republishes the next live slot. No extra framebuffer depth allocation is added.
Immutable scene ownership retains scratch picking for transitions without a public
picking buffer; composition remains single-threaded.

Frame cache keys include loot world/revision. Dialog paging no longer invalidates
the dungeon frame. Turns/steps retain bags in prepared transition images and expose
no picks until stationary. Native
camp and missing item icons use two startup textures in the existing GPU registry;
painting uses one fixed background frame and a varying central fire region from
four retained frames, keeping the rest of the scene stable. All text stays live.
Masters and generation prompts are retained under `assets/ui/camp` and `assets/ui/items`.

`rules/recovery.json` is bounded to 64 KiB, depth eight, 16 unique walkable sanctuary
cells and 64 valid ground stacks. Schema 6 rejects unknown IDs, oversized drops and
bad percentile ranges. Catalog schemas 1–5 remain strict and readable without loot.
`--recovery-demo` places three separated encounters in the normal map with a demo-only
spawn sanctuary. Normal gameplay loads the authored no-sanctuary configuration.

## Loot-bag review correction

2026-10-27: the user rejected individual ground-item browsing and requested one
loot-bag graphic per cell containing loot. Death locations and authoritative floor
ownership remain unchanged. Clicking a nearby bag after combat should open a paused
loot dialog with an item grid, click-to-take, hover stat tooltips, Take All and
Close/Escape. Uncollected contents keep the bag present. This request supersedes
the original individual floor icons and floor-wheel paging; it does not change
rest mechanics or authorize slice 13.

**Accepted checkpoint:** the user explicitly asked for an imagegen mockup, then a
pause for acceptance before implementation. [Loot dialog v1](mockups/slice12/loot-dialog-v1.png)
was produced with the built-in imagegen tool; [exact prompt/provenance](mockups/slice12/loot-dialog-v1.prompt.json)
are retained. It shows a five-column grid with quantities, selected recipient and
carry weight, a hover tooltip, Take All and Close/Escape while keeping the existing
HUD visible. Item amounts and carry weight are illustrative. Runtime text will be
native UI. Crowded cells will need scrollable grid access without taking unwanted
items first; the visible grid size is not an ownership capacity limit.

**Scaling defect:** inspection found a discontinuity in `PreparedLoot::compose`:
current-cell sprites force size 32, while cells ahead use projected size up to 48.
This can make an approaching item shrink on entry into its cell. The follow-up
must replace this special-case size jump with consistent bag scale and positioning,
including current-cell visibility and near/adjacent/diagonal regression checks.
This diagnosis is from code inspection; the reported movement was not replayed.

**Original next step (now implemented):** finalize bounded cell grouping/dialog snapshots,
reuse checked inventory transactions and stat descriptions, define Take All failure
feedback without losing leftovers, replace world icons/picking with one bag per
cell, and test partial/empty bag lifetime, recipient identity, duplicate requests,
full/overweight inventory, scrolling, tooltips, Escape and approach scaling. Keep
rest and accepted Inventory/Character controls intact. No gameplay, renderer or
runtime asset changes were made during this mockup checkpoint.

### Initial approved correction implementation — 2026-10-27

The user approved proceeding without another mockup and required the info panel
inside its parent, top-aligned with the item grid; identical Inventory slot borders
and quantity treatment; clickable party portraits to change the recipient while
looting; and a visible disabled-when-unavailable Rest button below the portraits.

Initial approved plan (Take All closure and the large Rest dock are superseded
by the follow-up below):
- Keep authoritative per-stack loot reservations and death cells. Render one bag
  per nonempty cell, identified by a checked representative live slot. The dialog
  stores world/cell identity, never a borrowed stack pointer. Retain safe pickup
  reach/threat rules. It pauses simulation but revalidates each transaction.
- Use fixed arrays of at most 2,368 stack IDs with a ten-slot visible grid and
  scrolling/page controls. Click takes one whole displayed stack. Take All tries
  whole stacks in stable slot order for the selected recipient; anything that
  cannot fit stays in the bag, with an explicit count/reason. Never redirect items
  to another character. Empty bags disappear; the empty dialog can be closed.
- Portrait clicks in the loot modal change gameplay selection and recipient while
  keeping the modal open. Tooltips follow hover and show base item stats, weight,
  quantity, description and collection availability. The info rectangle stays
  within the modal and shares the grid's top edge. Extract one shared slot painter
  so Inventory and Loot cannot diverge in borders, art padding or quantity labels.
- Replace the floor derivative atlas with a single transparent bag image baked at
  bounded sizes. Project size monotonically with approach; only the floor anchor
  is clamped upward to keep a current-cell bag onscreen. Retain wall/body/corpse
  occlusion, transition picking suppression, fixed scratch and cache invalidation.
- Put a labelled campfire/Rest button in the bottom-right HUD below the unchanged
  portrait rail; narrow and rewrap the adjacent log to reserve this area. Enable
  it only when some Short/Long Rest can provide a legal benefit (considering Hit
  Dice, supplies, threats, movement and presentation). Keep R and menu access for
  inspecting camp failure reasons. The button is dim and inert when unavailable.
- Test bag grouping/leftovers/final removal, scroll without pickup, Take All partial
  capacity failures, recipient switching, parent/info alignment, shared slot
  visuals, Escape/focus isolation, Rest availability, monotonic approach size,
  allocation behavior and hidden native/reduced/letterboxed application captures.
  Update verification and stop for user review; no slice 13 work.

Implementation boundaries: `loot_commands.cpp` owns the modal and its bounded ID
snapshot; `loot_screen.cpp` paints it; `loot_layout.hpp` defines fixed geometry.
`GameSession::lootAccess` checks safe reach independently of inventory capacity, so
full or incapacitated recipients can still browse and choose another portrait.
Every collection revalidates recipient, world/slot, safety and inventory revision.
Take All performs stable-order whole-stack transactions across every page; failures
do not stop later stacks from merging. Fixed feedback reports success/leftover counts
and the first rejection, with the complete message also in history. Take All closes the dialog even if some items remain; individual pickup leaves
it open until Close/Escape. Focus loss blocks input; closing consumes the frame.
The fixed 2,368-ID list fits the authoritative floor capacity, so it never grows
or truncates. Catalog definition pointers in ten visible entries share the existing
immutable catalog lifetime. No ownership or stack pointers are retained.

Rest availability previews a beneficial Short Rest with at most one available die
per injured living member, and a Long Rest, using the same authoritative checks as
camp. This is bounded work over four members/192 bag cells without allocations.
The follow-up replaces the bottom-right dock with a generated 16×13-art-unit
campfire immediately left of Settings, using the existing atlas and disabled tint.
Normal/casting logs return to their original widths and wrapping.

## Inventory Drop and top-bar Rest follow-up

Approved 2026-10-27, plan recorded before code; implemented and verified:
- Take All closes after attempting every stack, including partial/failing results;
  leftover bags persist and the existing log retains the counts/rejection reason.
- Inventory adds Drop beside existing contextual actions. It removes the entire
  selected stack (bag or equipped item), keeps Inventory open, and deposits it in
  the current party cell; split first to drop part. Safe stationary exploration is
  required. GameSession validates revision, source, safety and floor capacity before
  Party stages removal/derived stats. Publication then cannot fail in this single-
  threaded transaction. Reconcile equipment effects after successful removal.
- Retain 2,112 actor/authored reservations; append 256 reusable player-drop slots.
  Floor exhaustion leaves inventory unchanged with a clear reason. Add a generation
  to floor handles/entries, carry it through picking and dialog IDs, and retire a
  dynamic slot if its generation exhausts. Collection permits reuse without stale
  handles naming new items; actor/authored slots remain non-recycled. Fixed total
  capacity 2,368; no per-frame allocations. Different dropped stacks in one cell
  share its existing single bag. Pick/drop cycles can reuse slots indefinitely
  within generation bounds without consuming future death reservations.
- Restore normal/casting log bounds and wrapping to the pre-Rest-dock sizes. Put a
  small generated campfire icon immediately left of Settings in the top bar. It
  reuses the UI atlas (no extra GPU texture), uses tint/alpha when disabled, and
  retains Rest [R]/availability hover text and input behavior.
- Test equipment/stat and whole-stack conservation, foreign/stale/replayed requests,
  repeated slot reuse and stale floor IDs, exhaustion rollback, unsafe/moving
  rejection, modal auto-close including leftovers, headless allocation invariants,
  and hidden native/reduced/letterboxed captures. No slice 13 work.
