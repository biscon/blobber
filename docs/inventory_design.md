# Inventory and equipment — slice 11

Status: implemented, **accepted and closed** by the user on 2026-10-27, including UI corrections and the Character tab.
The [2026-11-08 paperdoll extension](#expanded-paperdoll-mockup--2026-11-08)
supersedes the historical four-slot/208-stack bounds below. Its mockup is accepted;
implementation is mechanically verified and awaits runtime visual review.
The user approved the planning conversation with “Implement the plan.” See
[verification and handoff](verification/archive/slice11_verification.md). Slice 12 is next eligible for
planning and remains unstarted.

## Approved behavior

Party purse revision (2026-11-07, implemented and mechanically verified; UI review pending): gold belongs to
Party as a weightless uint64 balance, never a live item stack. Startup grants and
loot collection credit the purse; all characters spend from it. The existing
inventory transaction revision also advances on currency acquisition, preventing
replayed requests and invalidating pending UI gestures. Currency contributes no
weight in authored character previews. The inventory header displays the shared
balance right-aligned to the bag grid. See [merchant design](merchant_design.md#transactions-time-and-saves)
for atomic exchanges, schema 14 and the one-time external edit of the user's save.

Four independent 8×6 bags plus body, main-hand, shield/off-hand and alternate
ranged equipment positions. Equipment leaves the bag; all weight counts once
against Strength × 15 lb. Preserve proficiency, Life heavy armor, Defense,
versatile grip and current shield/ranged incompatibility. Armor ends Mage Armor
immediately; removing it does not resurrect the effect.

Identical definitions stack: arrows 100, healing/mana potions 20, other items one.
Empty-cell drops move, unlike bag drops swap, matching drops merge what fits.
Explicit quantities are atomic. Split leaves at least one at source. Transfers
fill matching stacks then empty cells in row order, wholly or not at all.
Unequip needs space; replacement returns the old item to the vacated source cell.
No automatic removal of conflicting gear. All rejections conserve state and RNG.

Organization inside a bag is allowed during paused danger. Equipment/transfers
require stationary safe Exploration, no aware/adjacent living hostile or pending
attacks. Downed/dead characters retain their gear and can be managed when safe.

Healing potion: 2d4+2 HP, 0.5 lb. Mana potion: project adaptation, +2 mana,
0.5 lb. Everyone starts with two healing potions; cleric/wizard also have two mana
potions. Rogue retains 20 arrows. Conscious, able actors use their own potions on
any living ally; reject full resources/no mana capacity. Healing revives but never
resurrects; no Life bonus. Use costs one action or six-second character cooldown,
resolves immediately and uses existing feedback holds. Bow acceptance spends one
compatible arrow before rolls, without miss/interruption refunds. NPC ammo stays
abstract. Loot, arrow recovery, rest, progression, saves and dropped items deferred.

## Responsibilities and storage

Party is noncopyable/nonmovable so its handle identity and borrowed references
have one stable owner. Party owns a contiguous 208-stack inventory with four 48-cell bags and four
4-position equipment arrays. Typed handles carry owner identity, slot and
generation; movement preserves identity and consumption/complete merge invalidates
it. Exhausted generations retire their slot. Fixed scratch copies stage atomic
transactions and derived stats; no strings or growing containers enter this path.
GameSession gates safety and reconciles effects; Combat owns potion/action/ammo
acceptance and clocks. UI only submits requests and reads bounded snapshots.

Startup loadouts remain a separate immutable content representation; live equipment
queries must not read initial item indices. Pure derivation accepts current
loadouts. Catalog schema 5 adds typed ammo/consumables/stack limits; schemas 1–4
retain strict parsing and convert legacy quantities into legal runtime stacks,
rejecting capacity overflow. Definitions remain immutable, max 64 items.

Requests include expected inventory revision and checked handles. Results report
specific capacity, weight, ownership, compatibility, stale and safety failures.
All session calls invalidate the borrowed event batch; consume the whole batch
before the next simulation operation. Potion effects and costs commit together;
invalid requests spend no RNG. Existing impact outcomes and clocks remain intact.

## Presentation

Full-screen paused overlay: four character tabs, equipment silhouette left, 8×6
bag centrally and details/actions right. Open with I or
backpack/5. Drag and applicable button actions share transactions. Drop onto another character's
tab transfers. Shift-drag to a valid target opens a split dialog on release,
default 1, with no mutation before confirmation. Escape/focus loss/invalidated
source cancels drag. Mouse down/release and held latches prevent click-through.
Keyboard navigation, contextual actions and quantity controls remain available;
move/merge/split use drag gestures (see the review correction below). Use exits browsing
into live ally targeting; combat actor is independent of the inspected character.

Generate fifteen painted item icons plus a neutral silhouette using built-in
imagegen. Preserve masters/prompts, existing portraits and the native UI render
path; prepare bounded padded atlas regions at startup. Dungeon banks unchanged.
Add --inventory-demo and --capture-inventory, with in-process scripted input only.

## Verification strategy

Test-owned catalogs/generated art, fixed RNG, isolated directories. Cover
conservation, ownership, stale/foreign/reused handles, full capacity, stack/weight
boundaries, atomic transfer/swap failures, equipment/stat/effect reconciliation,
potion revival/caps/restrictions, last-arrow spending and no refunds. UI checks
include drag/click parity, split default one, focus/held/release isolation, stale
dialogs, keyboard and aspect-fit mapping. Extend first/repeated allocation guards.
Run Debug/Release suites, focused ASan/UBSan, separate capacity/reuse stress, hidden
GPU lifecycle and native/reduced captures. Record unavailable checks honestly.
Finish Ready for review with a durable handoff, never self-accept.

## Implementation details

`Inventory` contains only fixed value storage (about 8.2 KiB on this build).
`Party::inventoryChange` stages a copy, applies structural operations, derives four
current loadouts, then commits the copy and stats. The bounded work is a few scans
of at most 208 locations plus four 52-entry derivations. Rejected staged partial
merges are discarded. `Inventory::apply` is a private staging primitive; only Party can mutate it.
`Party::previewInventoryChange` uses the same staging/derivation for valid split
destinations without committing. Session callers use `inventoryChange`.
Consumption only accepts bag locations. Stack generation and inventory revision
counters never wrap during mutations; exhausted slots are retired and revision
exhaustion rejects. No new per-frame containers or polymorphic dispatch were added.

`CharacterIdentity` separates immutable identity/build strings from startup gear.
`CharacterLoadout` supplies pure derivation with sparse current inventory. Runtime
weapon previews use positions 49 and 51, and effects/combat/inspectors query actual
equipped definitions. UI cells borrow immutable definitions and copy small derived
weapon previews. Armor changes reconcile effects before returning the borrowed
session event batch. UI consumes that batch before the next session operation.

`combat_items.cpp` validates the turn/world token, owner, stack generation,
inventory revision, acting state and recipient before committing costs and rolls.
Healing emits one item-labelled restoration event; mana emits one recovery event.
Both share existing readiness and the 1000 ms feedback hold. Ranged acceptance
consumes ammunition before attack RNG; delayed impact cancellation cannot refund it.

`inventory_commands.cpp` owns selection, drag, split dialog and pending item use;
`inventory_interaction.cpp` maps pointer/key edges to those operations. Focus loss
cancels drags; inventory revision changes cancel pending destinations. Direct stale
confirmation is also rejected authoritatively. Button/drag transfers merge in row
order; equipment replacement swaps back into the emptied source. Full inventory
browsing is paused; choosing Use closes it and resumes live recipient targeting.
The viewed character remains separate from the combat actor and formation slot.

The native HUD renderer draws the inventory after other overlays. The UI atlas is
2048×2048 RGBA (16 MiB), preserving old regions and adding fifteen 168-pixel item
regions and a 384×600 silhouette region, with two-pixel edge extrusion. It is
prepared/uploaded once. Master PNGs and generation prompts are retained under
`assets/ui/items/`; the dungeon prepared banks are unchanged. Measured description
lines use fixed scratch storage and four-line pages; wheel/Page Up/Page Down reach
all accepted 512 bytes. Weapon details display current attack/damage bonuses.

The capture runner uses real sessions and the production input router, with empty
placements for safe management, ordinary placements for danger, and a catalog clone
with 100 arrows and long text for the full-bag/page stress image. These diagnostic
preparation changes are never applied to normal gameplay. All windows are hidden;
no external window or OS input is controlled.

## Review correction: simpler inventory controls (2026-10-27)

Implemented from the user's screenshot review; accepted with slice 11 on 2026-10-27. This supersedes
slice 11's original Move/Split button and keyboard destination modes. Drag/drop
organizes and merges stacks; Shift-drag opens the quantity dialog. Keep only
contextual Equip/Unequip, Transfer and Use Potion actions. Gear-only items such as
holy symbols, spellbooks and arcane focuses expose Transfer, without suggesting
unimplemented abilities. Applicable actions temporarily blocked by safety, capacity,
training, turn, cooldown or recipient state are visibly disabled with a reason.
The controller builds a fixed action snapshot using the same transaction previews
and combat validation as submission; both clicks and shortcuts consult it.
This adds bounded preview work for at most three actions/four recipients per refresh,
with no retained transaction copies or frame-time allocations. Labels/reasons point
to static strings; character/item ownership and mutation stay with Party/GameSession.

Move the body slot onto the torso and ranged slot below the main hand, alongside
the silhouette. Leave head and feet unobstructed for future slots without adding
those slots now. Center inventory button captions using font measurements; use a
larger prepared font for split +/- controls. Align stack badges to the right with
consistent padding. Add an explicit always-visible drag/Shift-drag hint.
The 48-pixel bold control font is loaded once alongside the existing size-specific
atlases and destroyed with UiFonts. Shared layout functions compact visible action
rows identically for painting and hit testing.

Follow-up alignment correction: center Body at the silhouette's x=91 axis.
Use equal 150-unit character boxes with the existing 4-unit gaps, spanning x=14
to x=626. This aligns the row with the title, leaves equal 10-unit panel insets,
and aligns its right edge with the close button (shifted left by one unit).
Drawing and pointer targets continue to use the shared layout rectangles. Verify
with the existing inventory test and hidden native/reduced captures.

The subsequently approved [Character tab](character_sheet_design.md) replaces the
small portrait inspector. It shares this overlay and selector without moving the
inventory contents; only the title band becomes Inventory / Character navigation.

I remains the inventory shortcut (already mapped, now regression-tested); M replaces
F1 for map toggling. Remove the obsolete M/S inventory command bindings. Update the
on-screen map hints and README. Replace the dock potion artwork with a separately
saved painted backpack and retain its generation prompt/master.
The dock tooltip names Inventory and shows its I/5 shortcuts.

Verify contextual actions and unavailable shortcuts, drag/split/merge conservation,
I opening, quantity-confirmation isolation, allocation guards, hidden GPU lifecycle,
and native/reduced review captures. No new D&D abilities or later-slice features.

## Slice 12 ground-drop extension

The user's 2026-10-27 follow-up adds Drop to the contextual sidebar after the other
applicable actions. Drop removes the entire selected stack (including equipped
items) into the current cell's loot bag, leaving Inventory open. Split first for a
partial drop. Safe stationary exploration, source revision and floor capacity are
checked before either owner changes; equipment-derived stats/effects update on
commit. The shared [loot/recovery design](loot_recovery_design.md#inventory-drop-and-top-bar-rest-follow-up)
owns the reusable floor slots and their generation checks. This supersedes slice
11's deferral of dropped items; other accepted Inventory controls remain unchanged.

## Expanded paperdoll mockup — 2026-11-08

Status: **mockup accepted; runtime implemented and mechanically verified 2026-11-08;
runtime visual review pending**. The user
approved the image with “yes thats exactly what I had in mind, continue”. The
historical four-slot implementation above is superseded by the extension below.

Prepare one 1920×1080 inventory concept using the current inventory screenshot and
existing artwork. Keep the party tabs, 8×6 bag, item-details panel, purse, palette
and typography. Replace the four 120-pixel equipment boxes with thirteen
72-pixel boxes. A central silhouette has five labelled worn slots on each side:
left Head, Body, Hands, Ring 1, Feet; right Cloak, Amulet, Wrists, Belt, Ring 2.
Thin subdued leaders associate worn slots with their body areas without obscuring
the silhouette or overlapping slot targets. Main Hand, Shield and Ranged form a
separate row below. Body, Main Hand and Shield reuse existing equipped-item art;
the new slots and Ranged remain empty. Hands means a pair of gloves/gauntlets;
the separately approved Wrists position permits future bracers alongside gloves.

This is an offline image, not a new UI or runtime subsystem. No public interface,
item definitions, allocation bounds, inventory ownership, gameplay or save schema
changes in this delivery. A later implementation must address the fixed four-slot
storage and its callers explicitly rather than treating this as only a visual edit.

SRD reference: [5.1, printed pp. 205–206](https://media.wizards.com/2023/downloads/dnd/SRD_CC_v5.1.pdf#page=205).
The fixed grid, two rings and one amulet are project rules. Equipment position is
separate from magic-item category; attunement is a separate future concern, with
the SRD's three-attuned-item limit applying only to items requiring attunement.
No new item types or magical effects are introduced for this mockup.

Verification is visual: inspect all thirteen positions, exact labels, blank new
slots, preserved surrounding inventory and readability at native and 1280×720
sizes. Present the full inventory and equipment close-up, retain selected evidence
under `docs/screenshots/paperdoll/`, and stop for user review. No build, gameplay
test matrix or desktop automation is necessary for an offline concept.

Mockup delivery: [full inventory and equipment close-up](screenshots/paperdoll/README.md)
were accepted by the user on 2026-11-08. Built-in imagegen used
the current inventory screenshot, followed by one refinement of slot sizes and
connector placement. ImageMagick exported the selected 1672×941 result at
1920×1080 and made a close-up; a disposable 1280×720 export was visually inspected.
All thirteen labels, nine empty new positions, separate hands/wrists/rings and
the 8×6 bag are visible. This is an illustrative concept: exact 72-pixel targets,
native typography and original-art fidelity remain production requirements.
The generated image omitted the current AC/safety readouts; keep those in the
eventual runtime design. No application build or behavior tests were run because
only documentation and offline mockup evidence changed. No desktop was controlled.

### Runtime extension — implemented design

The rules layer owns the shared equipment-position enum and counts. Preserve
Body/Main Hand/Off Hand/Ranged cell numbers 48–51 and append Head, Hands, Feet,
Belt, Cloak, Ring 1, Ring 2, Amulet and Wrists at 52–60. Each character has 48 bag
cells plus 13 equipment cells; the party owns a fixed 244-stack pool. Checkpoint,
loadout, derived weapon previews, HUD snapshots and automation use these shared
61-cell bounds. Party remains the sole transaction owner, staging fixed value
storage before validation/commit; all rejected swaps conserve ownership and stats.
The 36 additional pool entries cover fully occupied equipment without reducing
bag capacity. Exhaustion, generations and revision semantics remain unchanged.

Item category remains separate from wearing location. An optional schema-15
`wear_slot` on `gear` definitions names head/hands/feet/belt/cloak/ring/amulet/wrists.
Legacy gear defaults to no wearing location, so spellbooks/foci/symbols do not
become equippable accidentally. Validation rejects wearing metadata on other item
kinds and unknown positions. Wearable gear stacks to one and currently contributes
weight only: no implicit AC, proficiency requirement, magic or attunement. Ring
definitions fit either ring position; Equip selects the first empty ring position,
then replaces Ring 1 when both are occupied. Dragging explicitly chooses either.
No wearable definitions, grants, loot or icons are added to shipped content.
Existing armor, shield, ranged and two-handed compatibility remains authoritative.
Starting-build/library formats retain their four authored positions for now;
runtime wearables are not starting equipment in this slice.

The initial production renderer reused the prepared silhouette and item atlas. Equipment
boxes are exactly 24 art units / 72 native pixels, with ten worn positions in two
columns and three weapon boxes beneath; bag/details/tabs remain fixed. Leader
geometry and hit rectangles share one constexpr layout. Fixed arrays and drawing
primitives for structural lines add no frame-owned allocations or new textures.
Labels use the existing exact-size Small font atlas and integer native glyph
origins. Restore AC and safe-management readouts below the weapon row. Empty new
positions are selectable; keyboard navigation follows their visual arrangement.

Campaign schema/simulation advance from 17/21 to **18/22** because inventory arrays
and legal equipment changed. Follow the existing strict rejection policy: older
campaign files remain untouched, without conversion. Character-library and arena
formats are unchanged. Catalog schema 15 is opt-in for future wearable authoring;
the shipped catalog needs no byte change or new fingerprint.

Focused verification: generated test-owned wearable definitions exercise every
position, both rings, replacement, invalid swaps, full bags, ownership/weight,
safe-only management, restore validation and first/repeated allocation guards.
UI tests cover all thirteen pointer targets and spatial keyboard traversal.
Loader fixtures cover the optional metadata and malformed/legacy rejection.
Run affected inventory/rules/party/creation/library/trade/automation checks in one
Debug build, plus campaign persistence integration for the changed saved arrays.
A small hidden production-UI runner captures native/reduced inventory, checks
frame allocations and resource cleanup, and leaves interactive review to the user.

### Runtime verification and handoff — 2026-11-08

- Debug game, headless tool, nine selected test executables and the dedicated
  `blobber_review_paperdoll` target built. Shipped catalog/artwork and user saves
  were not edited. Schema-15 wearable loading is verified with owned fixtures;
  the shipped catalog stays on 14 with all new positions empty.
- `env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2 nice -n 15 ionice -c 3
  ctest --test-dir cmake-build-debug -R
  '^(inventory|party_content|campaign_save|trade|creation|library|party|automation|progression_allocations)$'
  -j 1 --output-on-failure`: **9/9 passed**, 115.25 seconds. The deliberately
  extended persistence suite accounted for 109.06 seconds; no second configuration,
  benchmark, stress run or redundant save-smoke pass was run.
- Inventory coverage includes all nine added positions on all four characters,
  all four full bags exceeding the old 208-stack limit, incompatible occupied-slot
  swaps, weight conservation, last-slot restore and fresh handle identity. Both
  rings, deterministic Equip/replacement and matching merchant comparison behave
  correctly without invented AC or training. First/repeated wearable equipment
  transactions and HUD refreshes perform zero observed C++ allocations.
- The allocation test had an existing ambiguous `ignite({6, 4})` call after the
  spell-area API extension. Naming its `Cell` type resolves compilation without
  changing the fixture. A test-only narrowing warning was also corrected.
- `blobber_review_paperdoll cmake-build-debug/assets RUN/captures` ran with bounded
  threads, low priorities and a hidden 1920×1080 window. Production pointer routing
  unequips/re-equips the compact shield target and verifies AC changes/restoration.
  First and eight repeated UI frames allocate no observed C++ storage, change no
  prepared GPU residency and perform no texture uploads; font accounting releases
  and OpenGL reports no error. These checks exclude driver/C allocator internals.
- Visually inspected the [four selected production captures](screenshots/paperdoll/README.md),
  including native/reduced presentation, ranged gear and empty Wrists selection.
  An initial capture exposed aspect-fit padding at the neck/shoulder/feet; shared
  anchor coordinates were corrected. Only affected inventory/UI checks were rerun
  afterward, passing. No desktop input or window selection was used; interactive
  user review remains unverified.
- To avoid another unchanged world-asset bake after this geometry-only correction,
  rebuilt the inventory/review/headless targets, then relinked the Debug game with
  the exact link argument list emitted by `ninja -C cmake-build-debug -t commands
  blobber`. All affected libraries were rebuilt first; the prepared publication
  from the successful full game build was reused. The initial full build performed
  the one necessary production asset refresh.
- Retained six feature images: two accepted concept views and four runtime views.
  Superseded captures and build/test logs are disposable; the task-owned agent run
  is cleaned after promotion. Build caches and unrelated runs remain untouched.

### Paperdoll readability correction — 2026-11-08

Status: **user rejected the initial runtime layout; correction implemented and
mechanically verified, revised appearance awaiting review**.
The user requires zero crossed connector paths, horizontally aligned rings (or
one vertical ring column), Head centered above the figure and Feet centered below.
Floating labels in the initial layout were easily read as belonging to the slot
above them. This correction supersedes the initial two-column worn-slot layout.

Keep all thirteen equipment positions and their gameplay/save semantics. Head and
Feet move to the central axis. Four grouped slots on the left read Body, Belt,
Hands and Ring 1; the right reads Amulet, Cloak, Wrists and Ring 2. The two rings
share the same horizontal baseline, with Feet centered between them below the
figure. Main Hand, Shield and Ranged remain in their own row beneath these slots.
A slightly narrower prepared silhouette leaves more horizontal clearance for
connectors. On each side, card and anatomical endpoint order agree vertically;
Head/Feet use short vertical connections, with a shared Feet fork to both shoes.
No independent connector segments may cross or touch, and no connector may enter
another slot's card or label area.

Each slot owns one 40×34-art-unit card with an inset 24×24 item square and an
attached nine-unit label band inside its border. Use the existing exact-size
21-pixel Small font atlas and integer native glyph origins. The entire card,
including the label, is a pointer selection/drop target. Shared constexpr card,
label, item and connector rectangles drive painting, hits and spatial keyboard
navigation without allocations or additional textures. AC/safety text stays
below the weapon row, with room before the existing footer.

Add focused geometric regression coverage for all connector segment pairs and
card/label exclusion, plus centered Head/Feet, paired-ring alignment, caption
hit targets and revised keyboard traversal. Run only the inventory suite and the
existing hidden production presenter review; persistence and other gameplay
checks remain valid because the correction changes layout/input geometry only.
Rebuild affected libraries and relink Debug game/headless while reusing unchanged
prepared assets. Replace the four superseded production images, preserving the
original concept's historical acceptance and marking the revised runtime pending.

Verification: rebuilt `blobber_inventory_tests`, `blobber_review_paperdoll` and
`blobber_headless` in Debug with the two-worker preset and low CPU/I/O priority.
Relinked the game with the CMake-generated argument list after the affected
libraries rebuilt, reusing the unchanged prepared publication. No asset bake,
save migration or version increment was needed for this layout-only correction.

`ctest --test-dir cmake-build-debug -R '^inventory$' -j 1 --output-on-failure`
passed in 0.02 seconds. The suite now checks every segment of independent leaders
for crossings/touches, all foreign card/label borders for intersections, visible
card separation, Head/Feet centering, ring baseline alignment, caption pointer
targets and updated keyboard traversal. The only shared junction is the intended
two-shoe Feet fork. Existing transaction, restoration and allocation assertions
remain covered by this same suite; the broader nine-suite/save result above was
not rerun after this presentation-only change.

The hidden `blobber_review_paperdoll` runner passed again with bounded thread
counts: all thirteen native label widths fit their own caption bands, compact
shield drag/drop and AC updates work, first/repeated frames show zero observed
C++ allocations, prepared GPU bytes/uploads remain fixed, font accounting releases
and OpenGL reports no error. Inspected all four refreshed production captures,
including native/reduced text, ranged equipment and whole-card empty-slot focus.
No desktop input or window control was used. User review remains pending.

Replaced the four rejected-layout production captures in the existing gallery;
kept the two historically accepted concept views for provenance (six total).
The task-owned run and its temporary build/test/capture output are cleaned after
promotion, without removing user references or build caches.

### Equipment-card finish correction — 2026-11-08

The user clarified that extra padding belongs **only beneath the item square**,
not elsewhere inside the card or between Head and the doll. Increase card height
from 34 to 36 art units while retaining the 24×24 square at its existing offset:
bottom padding becomes three art units / nine native pixels, up from one / three.
All thirteen card origins, caption bands, image positions and other padding stay
fixed. Move only the Head connector's starting point to its new bottom border;
the geometry tests must continue to exclude crossings and foreign cards.

Fill the entire outer card with its existing caption-band color (25,33,30), draw
one outer border, and omit the generic box painter's inset outline for these cards.
The distinct item square keeps its normal rendering. This is a local card-style
change with no new assets, atlases, frame allocations or persistence changes.
Verify with the existing inventory geometry/input suite and native/reduced hidden
captures; retain four refreshed runtime views and clean disposable output.

Implemented and mechanically verified; appearance awaits user review. The focused
Debug inventory suite passes (0.02 seconds), retaining card separation, centered
Head/Feet, paired rings, zero crossed/foreign-card connectors and caption hit tests.
The hidden reviewer passes compact drag/drop, AC, native caption widths,
first/repeated C++ allocations, stable GPU residency and OpenGL checks. Inspected
native and 1280×720 exports. Pixel spot checks on all thirteen cards confirm that
title, bottom padding and former inset-line locations share the requested fill.

Rebuilt the affected Debug inventory/review/headless targets with two workers,
low priorities, then relinked the game using CMake's generated link arguments and
the unchanged prepared assets. No broader gameplay or persistence rerun was needed.
Replaced the four production review images; six feature images remain including
the historical accepted concept pair. The owned disposable run is cleaned after
promotion; user references, saves and build caches are untouched.

### Equipment outline/alignment follow-up — 2026-11-08

The user requested removing the remaining outer gold card outlines and aligning
Main Hand with Ring 1, and Ranged with Ring 2. Keep the uniform outer fill and
attached caption, but draw no card perimeter, including for keyboard focus;
the existing item-square border retains its selected/cursor feedback. Move the
Main Hand card from x=20 to x=16 and Ranged from x=122 to x=126 art units, sharing
the side columns' exact x positions. Shield stays centered. Shared layout drives
the moved item squares, labels, hits and navigation. No assets, storage or save
changes are involved. Extend the existing geometry assertions for column
alignment, run the inventory suite and refresh the hidden production captures.

Implemented and mechanically verified; appearance awaits user review. The Debug
inventory suite passes (0.02 seconds), including the explicit column-alignment
invariant and existing connector, card separation, pointer and keyboard checks.
The hidden reviewer again passes native caption fit, compact drag/drop and AC,
C++ frame allocation guards, stable GPU residency and OpenGL/lifetime checks.
Inspected native and reduced exports; pixel samples on all thirteen card corners
in both normal and focused-slot captures confirm no outer perimeter is drawn.

Rebuilt affected Debug inventory/review/headless targets and relinked the game
with CMake's generated link arguments, reusing unchanged prepared assets. Replaced
four runtime gallery images; retained six images total with the historical
accepted concept pair. Disposable run output is cleaned after promotion. No
broader gameplay/save checks, asset bake, desktop input or save changes were needed.

### Inventory-key toggle — 2026-11-08

The user requests I to toggle the fullscreen inventory overlay, like M toggles the
map. Its focused press edge opens the Inventory page when closed and closes the
shared Inventory/Character overlay when open, including a pending split, transfer
or drag. Closing uses the existing overlay teardown, never commits an inventory
operation, and consumes the key so the same edge cannot reopen it. Physical key
edge/held/refocus latches remain unchanged. Clicking the Inventory page tab still
selects that page; 5/backpack entry and active potion targeting retain their rules.
No layout, artwork, saved state or frame-storage changes are involved. Verify the
open/close/held-key cycle, Character-page closure, tab-click navigation and safe
cancellation of pending split/drag through the existing inventory input suite.

Implemented and mechanically verified. Built the Debug inventory test and
headless targets with the bounded low-priority preset; relinked the game from
CMake's generated link arguments, reusing unchanged prepared assets.
`ctest --test-dir cmake-build-debug -R '^inventory$' -j 1 --output-on-failure`
passes (0.02 seconds). The regression routes real input edges through the HUD:
open, held-open, close, held-closed, reopen, Character-page close, a page-tab click
that remains open, pending split cancellation and drag closure followed by a
harmless release. Inventory revision and destination cells prove no split/drag
committed. No rendering, save or broad gameplay rerun was needed for this
input-only change. Disposable run logs are cleaned; no new images were generated.
