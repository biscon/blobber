# Subcell spell areas and Sleep balance

Status: implemented and mechanically verified, 2026-11-08. User balance/appearance acceptance is pending.

The shared geometry unit is one five-foot subcell. Typed `SpellAim` identifies an
integer subcell-grid intersection; creature subcells identify occupied squares.
`SubcellFootprint` owns an origin and an 8x8 uint64 mask, with centre-point shape
inclusion and conservative whole-cell wall/corner clipping. Party formation maps
to four heading-relative subcells. No normal-frame allocation is introduced.

Ground targeting snaps floor hits to intersections; mouse hover previews, click
selects, Cast/Enter commits. Keyboard cycles a fixed bounded list of valid points.
Shatter is a ten-foot-radius circle (12 squares); Grease 2x2; Web 4x4; Sleep 6x6.
Ground range is 60 feet from the actual caster. Fixed forward spells instead use
the party's front edge, independent of formation: Burning Hands/Color Spray use
two near squares then four far squares; Thunderwave uses a symmetric 4x2 rectangle directly
in front. No covered square belongs to the party cell. Direction remains party-facing. Existing friendly
fire, wizard protection, damage, mana, readiness and target eligibility remain.
Every recipient query, preview, ground overlay and shader clips to the same mask.
Existing artwork, native text and 1920x1080 prepared rendering are preserved.

Persistent terrain owns 16 logical casts, each with one footprint, typed area ID,
caster, expiry/effect identity and per-covered-subcell burn deadlines. Reserve
capacity before RNG/payment; reject exhaustion without eviction. Precise movement,
rotation, forced movement, placement and end-turn checks update membership;
cardinal whole-cell steps also check their intermediate square. Party terrain
cost checks conscious formation members at the start, midpoint and destination; moving
within one continuous area does not repeat entry saves. Overlapping terrain costs
do not multiply. Partial burning removes only affected Web squares. NPC movement
uses actual subcells; party checks use formation and heading. Packet capacity 1536
covers expanded previews/terrain. Render packets carry only the canonical mask;
coarse cell caches remain solely on cast/area records for existing lighting cues.
The existing 512 KiB effect-workspace assertion still passes. Engine-owned frame processing remains allocation
free and existing bounded navigation work is reused.

Sleep lasts twelve gameplay seconds (two round advances), not presentation time.
Keep shared 5d8 current-HP budget, lowest first, immunities, no save, damage/Aid wake.
Intelligence 6+ conscious enemies with an action may rescue a sleeping encounter
ally reachable within their remaining ordinary movement and five-foot wake reach.
Consider eight closest eligible allies; cheapest route wins, then unspent turn,
then stable identity. Skip allies that naturally wake before another opportunity.
Once a reachable candidate exists, roll once per turn with chance
`min(50, 10 + 5 * ((Intelligence - 6) / 2))` percent: 20% at 10, 40% at 18.
Pending searches never reroll or spend RNG/actions. A failed roll uses normal AI;
a chosen rescue spends movement and then the action, never Dash/attack/cast too.
Invalidated targets resume ordinary behaviour without another roll. Replace the
old unconditional fallback. Publish explicit rescuer/recipient wake log events.

Ground spell requests gain `aim_subcell: {x,y}`; legacy `cell` maps to its centre,
and both together reject. Other cell-based abilities retain their targeting.
Persist terrain footprints/deadlines and validate masks/ownership/capacities.
Campaign schema/simulation advance from 16/20 to 17/21; no automatic migrations or
user-save edits. Arena scenarios and character libraries retain their formats.

Verification completed in Release with focused CPU checks, followed by affected
callers and the extended campaign save suite once. No stress or benchmark matrix
was run. The normal Debug game was rebuilt successfully with `nice -n 15 ionice -c 3
cmake --build --preset debug --target blobber -j2`; its prepared assets were
refreshed once. No interactive desktop playtest was performed.

The affected CTest suites are `spell_geometry`, `combat_movement`, `spells`,
`expanded_spells`, `level_spells`, `effects`, `impact`, `hybrid`, `ai`, `commands`,
`hud`, `targeting`, `combat_positioning`, `automation`, `world_visibility`,
`world_integration`, `world_environment`, `campaign`, and `campaign_save`: all pass.
Commands use `ctest --test-dir cmake-build-release -R '<selected suites>'
--output-on-failure -j1`; builds use `nice -n 15 ionice -c 3 cmake --build
--preset release --target <affected targets> -j2`. Extended `campaign_save` passed
in about seven seconds. Existing whole-cell test fixtures were updated to own
explicit masks rather than silently relying on representative-cell fields.

New regression coverage includes exact shapes/all headings and caster slots,
corner-straddling inclusion, walls/range/bounds, continuous terrain membership,
partial Web burning, precise twelve-second expiry, zero-allocation rescue routing,
route-cost preference and no Dash, Intelligence thresholds, occasional actual
move-and-wake turns with no attack, natural-expiry avoidance, protocol admission
and wake identities, raised-floor aim picking, and mask/burn-deadline save roundtrips.
Existing guards cover allocation-free movement and prepared rendering.

Hidden GPU integration command: `env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1
LP_NUM_THREADS=2 nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests
cmake-build-release/assets <owned-run>/spell-areas --spell-area-review`.
The sandbox could not access X11; the authorized unrestricted rerun passed.
The hidden window stays hidden; no OS input or focus is controlled. All six
[review captures](screenshots/spell-areas/README.md) were inspected. Preview counts,
shader GL errors, prepared-frame engine-owned C++ allocations and GPU resource
release are checked. A targeting-label correction was verified with the affected
UI test and refreshed captures. Interactive feel and balance remain unverified.

Campaign save schema/simulation are now **17/21**. Older campaign files reject
without alteration; arena scenario and character-library formats are unchanged.
Disposable logs and superseded captures belong to the owned agent run and are
removed after runners close; only the six selected review images remain.

## Party-front fixed templates — follow-up, 2026-11-08

Implemented correction requested and clarified by the user: fixed forward spells use one
stable party-front origin in every formation slot. No covered subcell may belong
to the party's dungeon cell. Burning Hands (and the shared Color Spray template)
has two rows: two subcells beside each other, then four centred on them (six total).
Thunderwave now has eight subcells: four wide and two deep, with its first row
immediately beyond the front edge. Both rows extend one subcell to either side
of the two front lanes, symmetrically about the party centre. There is no third
row and it never shifts with caster selection. This supersedes the earlier
asymmetric 3x3 template at the user’s request. The front-left formation square is only a
heading-relative coordinate reference, not a lookup of the selected caster.

Ground-aimed spells retain caster-based range/targeting. Shape masks remain
authoritative for recipients, previews and animation clipping. Forward animation
origins move to the same fixed front edge. No serialized terrain changes are
needed. Validation covers all four headings and caster slots, whole party-cell
exclusion, wall clipping and preview/result parity, plus selected hidden captures.

Follow-up verification: Release `spell_geometry`, `spells`, `expanded_spells`, and
`world_integration` pass. The exact geometry regression checks all four headings
and caster slots, the two/four fan rows, the eight-square Thunderwave rectangle,
party-cell exclusion and wall clipping. Spell integration checks six occupied fan subcells, eight occupied Thunderwave
subcells and stable preview/result recipient ordering. An initially
unclosed generated test map was corrected to an enclosed owned fixture.

`blobber_world_integration_gpu_tests <assets> <owned-run>/forward
--forward-spell-review` passes in an authorized hidden OpenGL context, with
`OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2`, low CPU/I/O priority
and no OS input or focus control. The sandbox cannot open X11. Six inspected
[forward-spell captures](screenshots/forward-spells/README.md) cover previews and
results for Burning Hands, Thunderwave and Color Spray, with GL error, prepared
C++ allocation and GPU lifetime checks. Ground-spell evidence above remains valid.
No persistence, benchmark or unrelated test matrix was repeated for this local
template/render-origin correction. The normal Debug game rebuilt successfully
with `nice -n 15 ionice -c 3 cmake --build --preset debug --target blobber -j2`,
including the current spell descriptions and prepared assets. Interactive play
and visual acceptance remain with the user.
Owned disposable output is cleaned after runners close; only the six selected
forward-spell review images are retained for this follow-up.

### Symmetric Thunderwave adjustment — 2026-11-08

Design recorded before this adjustment: replace the 3x3 mask with two rows of
four subcells, centred on the two party-front lanes. Keep the full party cell
excluded and formation-independent placement. The existing bounded mask and
clipping pipeline need no new data or allocation; centre the Thunderwave burst
on the same party-front edge. Update exact geometry/recipient tests and replace
only the two superseded Thunderwave review captures. Verification: Release `spell_geometry` and `spells` pass using the affected targets
and `ctest --test-dir cmake-build-release -R '^(spell_geometry|spells)$'
--output-on-failure -j1`. Exact membership checks cover all four headings and
caster slots, both symmetric rows, exclusion of the third row, wall clipping and
exclusion of the party cell. Actual spell outcomes match eight covered front-row
occupants, including the newly covered left side.

The hidden `--thunderwave-review` mode of `blobber_world_integration_gpu_tests`
passes with bounded threads and low CPU/I/O priority; it renders only the changed
spell. The sandbox X11 restriction required the authorized unrestricted run.
Both inspected Thunderwave captures replace superseded evidence in the existing
six-image gallery. GL errors, prepared-frame C++ allocations and resource release
pass. No desktop input/focus was controlled and no unrelated matrix was repeated.
The normal Debug game rebuilt successfully with the usual low-priority,
two-worker `cmake --build --preset debug --target blobber -j2` command.
Owned output is cleaned after runners close; the gallery retains six images and awaits user visual acceptance.
