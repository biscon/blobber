# Sanctuary mayor and guards

Dialogue follow-up (2026-11-06): [multi-NPC event beats](dialogue_design.md#multi-npc-event-beats) now implement scripted guard interruptions and Talk/Laugh reactions using these existing assets. The historical deferrals below describe the original resident delivery. Placement/save compatibility and approved artwork are unchanged.

Status: implemented and mechanically verified; the user approved the runtime
appearance (2026-11-06), requesting that guards keep idling during dialogue.

## Ownership and behavior

Add three canonical peaceful residents to Sanctuary, using existing floor-owned
actors, stable handles, occupancy and resident combat exclusions. Preparation owns
the small placement vector; no additional per-frame containers or services.
Stable identities are sanctuary.mayor, sanctuary.boric and sanctuary.balin.
They face east outside the west-home: mayor cell (9,8)/subcell (0,1), Boric
(8,8)/(1,0), Balin (8,9)/(1,0). Preserve the doorway (7,8), arrival and routes.
Validate canonical placement and runtime peaceful invariants during restore.
Arrival safety treats only hostile living actors as threats; resident occupancy
still blocks landing directly on an NPC. The city formation makes this distinction
necessary for returning from the dungeon and recalling to Sanctuary.

Only the mayor has a dialogue catalog entry. Manual F/body interaction starts the
existing introduction/history/camera flow; guards have no prompt or conversation.
Reject absent dialogue entries before starting session state. Mayor explains that
monsters kidnapped his adult daughter Gwendolyn last night and took her into the
dungeon. Topics provide editable context, not quest state or rewards. Guards stay
idle during his speech. Automatic introductions, actual resident movement, guard
interruptions, arbitrary dialogue animation cues and rescue gameplay are deferred.

Dialogue idle correction: `NpcAnimator` retains a small presentation-only elapsed
dialogue offset for resident Idle sampling. It accumulates the existing focus-aware
conversation clock once per update and preserves that offset after conversation,
so background residents neither freeze nor jump back to their pre-dialogue phase.
The speaker retains existing Talk timing; hostile actors and authoritative game
time stay frozen. The offset belongs to the view animator, needs no allocations
or save data, and resets when that view is rebuilt. Verify multiple residents,
focus loss, repeated samples/conversations, exit continuity and unchanged gameplay.

## Artwork and animation interface

Approved v2 mockups are the sole identity references. Generate separate masters
with built-in imagegen, preserving prompts. Each character owns production assets
under assets/npcs/NAME. Reuse sheet registration, crop/mip publication
and shared human scale. No procedural deformation substitutes for authored poses.
The production baker retains its existing nearest sampling; a separate area-filtered
extraction also checks all registered canvases for clipping.
Idle: two frames, four directions, 1800/140 ms. Walk: four frames, four directions,
150 ms each. Talk: four front frames, 180/160/180/220 ms. Guards additionally have
four front Laugh frames, 160/120/160/240 ms. Small spit flecks belong only to the
mayor's authored Talk frames; no particles or effects. Feet stay registered.

Append Laugh to NpcAnimation, preserving numeric values 0–7; count becomes nine.
Talk/Laugh use directional Idle fallback. Residents may author/select Idle, Walk,
Talk and Laugh; selected absent Laugh/Walk reject. Missing optional Talk retains
the existing Idle fallback. Diagnostic placements exercise Walk/Laugh without
allowing movement; production canonical placements remain Idle. Hostile Laugh
placements reject. Dialogue continues to select only Talk/Idle.

Appearance publication revision advances 6→7. Texture descriptor capacity grows
480→640 for the additional 92 poses (28 mayor, 32 each guard). Audit dependent
fixed arrays and indices. The application texture-handle pool grows 512→704
(640 world entries plus 64 UI/preparation slots); existing texture-byte/GPU
ceilings stay unchanged.
Exhaustion still fails during preparation. Existing bounded animation arrays grow
by one clip per appearance, without heap activity in steady-state processing.

Save shape stays schema 12, simulation compatibility advances 16→17. Canonical
city actor content and fingerprints change; old campaigns reject explicitly, with
no migration or removal of user saves. Regenerate publication and packaged assets.

## Verification strategy

Use owned small CPU fixtures for clip validation/timing/fallback, absent Laugh,
canonical residents, route connectivity with occupied cells, combat exclusion,
inert guard interaction, introduction/greeting and save/travel persistence. Check
warmed engine-owned updates for allocations, not third-party/driver internals.
One focused Release integration checkpoint, two build workers/one test process,
low CPU/I/O priority. No benchmarks or repeated configuration matrix.
Hidden application-owned captures cover formation, facing, scale, Walk, Talk,
Laugh, dialogue camera and resource release. No desktop input/focus automation.
Retain up to six selected review images; runtime visual approval remains separate
from approved concepts. Source masters/prompts/registration are durable inputs;
dispose of task-owned run logs/captures after evidence selection.

## Verification results

Release game, headless executable and resident review tool built with two workers
at low CPU/I/O priority. All 29 transparent masters, exact built-in imagegen prompts,
measurement audits and definitions are saved under `assets/npcs/{mayor,boric,balin}`.
`tools/register_resident_art.py` measures alpha only; the existing C++ extractor
produces all 92 frames without clipping. Packaged world publication contains 517
textures across all asset packs; only required scene resources are resident.

Focused checks passed:

```sh
nice -n 15 ionice -c 3 ctest --preset release -R '^(residents|dialogue|inn|npc_metadata|animation|sanctuary_city|world_residency|campaign_save_smoke)$'
nice -n 15 ionice -c 3 ctest --preset release -R '^(world_visibility|commands|combat|ai|campaign|targeting)$'
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-release -R '^(campaign|residents|inn|campaign_save)$' -j 1 --output-on-failure
```

The initial eight checks passed in 2.05 seconds. The broader checkpoint found that
arrival safety incorrectly counted peaceful actors as threats; the hostile-only
correction and owned regression now pass. The other five integration suites passed;
the four affected arrival/persistence checks passed in 7.68 seconds, including the
full campaign save suite (6.37 seconds). Resident tests cover stationary Walk/Laugh,
timing, missing clips, directional fallback, picking/combat exclusions and warmed
allocation checks. Existing campaign/inn tests exercise travel and restored content.

Hidden OpenGL commands (bounded internal threads, no OS input):

```sh
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 cmake-build-release/blobber_review_sanctuary_city cmake-build-release/assets RUN/residents residents
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 cmake-build-release/blobber_world_visibility_gpu_tests
```

The sandbox could not open display `:0`; the authorized hidden checks succeeded
with display access. Production review exercised all four Walk directions, all
Talk/Laugh frame boundaries, mayor admission/repeat greeting, silent guard admission
rejection, and actual-content encode/decode retaining introduction history. City
actors never entered offensive picking. GPU parity/lifecycle passed with zero
boundary pixel differences. The scene used 134 textures, 379,817,416 texture bytes,
27,978,684 alpha bytes and 407,367,088 world GPU bytes, within unchanged byte budgets.
All scene texture/alpha leases released to zero.

Inspected deterministic captures cover formation, dialogue, guard laughs and
front/side/rear presentation. [Six selected captures](screenshots/sanctuary-residents/README.md)
were retained for the original review; task-owned raw captures, extraction copies
and logs were cleaned after selection. The user subsequently approved the runtime
appearance. Diagnostic Walk/Laugh previews use stationary actors; no
production roaming or dialogue-triggered guard laughter has been introduced.

### Dialogue idle correction verification — 2026-11-06

Release game and review runner rebuilt with two workers at low priority.
`ctest --preset release -R '^(residents|dialogue|animation)$' passed all three
focused suites in 0.07 seconds. New owned regression checks two idle bystanders,
frozen hostile/simulation clocks, repeated/focus-frozen samples, close/reopen
continuity and zero engine-owned frame allocations. No save compatibility changes.

The hidden production runner's `residents-idle` mode rendered both authored idle
frames for each guard over two seconds of open mayor dialogue, asserted that game
time stayed frozen, and released scene texture/alpha leases. Both inspected guard
captures are retained alongside the six accepted appearance cases while correction
review is pending. No desktop input automation was used. Disposable run output
was cleaned; interactive review of this correction remains with the user.
