# Slice 12.1 visual review corrections

Status: Implemented, **Ready for review**, 2026-10-29. User approved the plan and
selected an Abilities tab and a shared sleeping/knocked-down pose.

## Presentation ownership

CommandController keeps the complete 22-entry spell/ability catalog and a filtered
index view for Cantrips, populated Roman-numeral levels, and Abilities. Selection
and scrolling are remembered per character/tab in fixed arrays (ten spell levels plus Abilities). Pending casts retain stable spell
or feature identity. Seven rows remain; the panel extends upward for tabs.

EffectController supplies read-only bounded status summaries from its authoritative
clock and lifetimes, plus Party temporary HP. Each summary holds at most 14 entries;
HUD tooltip storage is fixed at 29 lines of 96 bytes. HUD snapshots own these values;
rendering never changes effects. Duplicate kinds share one icon and the longest
remaining lifetime. Six portrait slots show spell art; over six effects show five icons plus a +N
slot whose tooltip lists every remaining name and lifetime.
Harmful/beneficial borders distinguish effects. Tooltips use real remaining gameplay
time or explicit turn/consumption descriptions, including while the picker is open.
Enemy conditions use collision-aware spell badges coordinated with target markers.
Each visible actor gets at most six badges; a bounded 25-position search omits a
row if no nonoverlapping position fits. Badge placement never changes body picking.

## Ground artwork

Imagegen authors a transparent top-down oily pool. Offline scene preparation samples
it into immutable RGBA floor banks for each heading and eight strafe phases, anchored
to world-cell coordinates. Composition blends the prepared pool before NPCs/loot,
respecting existing wall depth, then separately blends targeting feedback. Preparation
and publication include source fingerprints, memory budgets, and format invalidation.
Prepared binary format and renderer revision are both 2; old bundles must be
rebaked. Four Grease frames raise each prepared view from 16 to 20 color frames.
No frame-time decoding/resampling or allocations are introduced.

## Prone appearances

Imagegen authors front/back/left/right static prone views for melee goblin and archer,
matching their existing equipment and scale. One-frame sheet extraction preserves
body pivots and ground registration. A Prone clip with exactly one frame in each direction is required by gameplay baking;
legacy static diagnostic loading remains supported. NpcAnimator reads authoritative
conditions: death wins, then living Sleep/Knockdown selects frame zero of Prone.
Waking/standing resumes normal poses without changing occupancy or life. Prepared
silhouettes remain the picking source. New art and exact prompts live in assets.

## Verification

Implemented regression coverage uses generated, test-owned fixtures: tab identity,
navigation and remembered scroll; effect grouping, overflow, expiry and clock
semantics; modal hover without click-through; four-direction prone, waking,
standing, death priority and picking; Grease alpha, anchoring, overlap, occlusion,
strafe and expiry; one-column source extraction; prepared bundle round trips.

Debug and Release pass all 31 suites, including zero tracked C++ frame allocations.
Six focused ASan/UBSan suites pass. Hidden GPU lifecycle checks and native/reduced/
letterboxed captures pass; representative captures were visually inspected.
The complete production bake uses 209,665,518 resident bytes and 150,419,024 tracked
loading bytes. Duplicating both complete appearance sets uses 278,669,284 resident
bytes, below the unchanged 512 MiB limits. These exclude driver/dependency memory.

See the [verification and handoff](slice121_verification.md) for commands, captures,
asset provenance and review instructions. Interactive desktop play remains
unverified; no desktop input was automated. Slice 12.1 awaits user acceptance;
12.2 remains unstarted.
