# Spell effects: design and delivery roadmap

Created: 2026-11-05. Last updated: 2026-11-05.
Status: **SE01–SE02 Accepted; SE03 Ready for review; SE04 Planned.**

Give spells distinct animated silhouettes, restrained glow, moving local light,
and readable elemental impacts while preserving the game's textured pixel art.
The user endorsed this direction and requested the same staged workflow as the
[campaign roadmap](procedural_campaign_plan.md) and
[city roadmap](sanctuary_city_plan.md). The proposed starting set—Fire Bolt,
Ray of Frost and Burning Hands—is the first slice, including its renderer and
asset foundation. This is a roadmap, not approval to implement all four slices.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Agreed direction | Authored spell animation, procedural motion, selective additive glow, local illumination, and target aftermath within the existing aesthetic |
| Proposed sequence | Four slices, SE01–SE04 |
| Active slice | SE03 — Ready for review |
| Awaiting review | SE03 support, conditions and persistent effects; E-B |
| Next eligible slice | SE04 — requires SE03 acceptance |
| Next action | User reviews the [SE03 gallery](screenshots/spell-effects/se03/README.md) and [implementation handoff](se03_design.md) |
| Gameplay policy | Preserve damage, saves, targeting, costs, authoritative timing and existing conditions |
| Rendering policy | Keep prepared assets, fixed 1920×1080 presentation, the current world renderer and discrete movement |
| Blockers | None |

Update this checkpoint, tracker, decision log and relevant slice handoff together.
Direction agreement, detailed-plan approval, implementation and delivered-slice
acceptance are separate states. Only the user accepts a delivered slice. This
roadmap does not advance campaign PC08 or reopen accepted city/renderer slices.

## Delivery workflow

1. Read [AGENTS.md](../AGENTS.md), [system design](system_design.md), this checkpoint,
   the preceding slice handoff and relevant code. Preserve working-tree changes.
2. Work on one slice at a time. Mark **In progress — planning** and write its
   practical design under `docs/`, linked from the system overview before
   implementation. Specify ownership, interfaces, data layout, capacities,
   exhaustion behavior, artwork preparation and focused verification.
3. Obtain review of that detailed plan before implementation. Existing explicit
   approval in the conversation suffices; do not ask again about settled choices
   or routine reversible implementation work within the approved scope.
4. Implement the approved slice as a usable improvement in normal gameplay.
   Include affected asset publication, diagnostics and correctness checks in that
   slice. SE04 is not permission to defer integration or tests.
5. Update design notes to match the result. Record exact checks, resource totals,
   selected visual evidence, limitations and reproduction steps in the slice's
   durable design/handoff. Avoid a separate permanent report for every run.
6. Mark **Ready for review** and stop. Review corrections stay in that slice.
   Following explicit user acceptance, record **Accepted** and identify the next
   eligible slice; do not automatically implement it.

Use **Planned**, **In progress — planning/implementation**, **Ready for review**,
**Accepted**, **Blocked**, and **Superseded/Retired** consistently. Blocked names a
real prerequisite. A necessary split may use SE01.1-style suffixes with explicit
scope and dependencies; do not silently remove failed acceptance criteria.
No commits, toolchain changes or automatic benchmarks are implied.

## Appearance and scope contract

- Match the existing painted pixel art: visible pixel clusters, rough silhouettes,
  controlled palettes, dark coloured edges and compact bright cores. Avoid smooth
  vector stars, featureless glowing balls and full-screen bloom washes.
- Combine authored animation frames with procedural placement, trajectories and
  particle variation. Use ImageGen for original raster artwork and preserve source
  masters/prompts; prepare and inspect coherent animation frames offline. Generated
  sheets still need temporal review and cleanup. Do not assume generation alone
  produces usable animation or substitute a recoloured cross for finished art.
- Give each element characteristic motion: curling fire, brittle frost, flowing
  force, snapping electricity, heavy acid droplets and drifting poison. Palette
  differences alone are insufficient.
- Use ordinary transparency for the coloured body, smoke and frost; additive
  blending for selected cores, sparks and small halos. Preserve silhouette/detail
  during overlap. Composite at the existing world art resolution and verify both
  presentation quality modes; retain the current HUD and scaling contracts.
- Local lights reveal nearby walls, floors and creatures with short-range coloured
  illumination. A halo around a sprite is not a substitute for surface lighting.
  Retain accepted dungeon attenuation/AO/fog and city/inn appearance when no spell
  light is active. Prevent obvious light transmission through walls and closed doors.
- Establish release, travel, impact and aftermath as distinct visual phases.
  Start tuning authored animation around 10–15 FPS and brief hit remnants around
  0.5–1 second; these are review starting points, not new gameplay deadlines.
- Burning Hands reads as a coherent spreading surge of flame tongues, clipped to
  the existing affected front cells and cover, rather than scattered projectiles
  or a cone implying a larger damage area. Area releases remain visible even when
  few or no creatures occupy the eligible cells.
- Target aftermath follows the creature, respects depth and transparent silhouette,
  and leaves its identity, target marker, health and status feedback readable.
  Flames may extend beyond the silhouette; frost coating belongs inside it.
- Short fire remnants are cosmetic: do not add a burning damage condition.
  Distinguish the initial frost burst from a quieter indication of the existing
  Slow effect. Sustained effects follow authoritative condition lifetimes.
- Reuse existing ground artwork, creature poses, icons, sound cues and screenshake
  where appropriate. Synchronize with existing feedback without adding mandatory
  camera shake to every spell. No new audio bank or HUD redesign is included.
- No new spells, combat balance, physics-based collision, fluid simulation,
  environmental destruction, global dynamic shadows, general VFX editor, entity
  framework, party world models or save migration project.

## Current foundation and proposed boundaries

These are roadmap-level directions, not implemented interfaces. Read current code
again before each detailed design; historical design documents describe earlier
renderers as well as the current system.

### Historical baseline before SE01

This paragraph records the original starting point. The implemented SE01 and SE02
interfaces and checks are described in their linked slice designs.

`tools/bake_world_assets.cpp` generates a 64×64 cross/glint source texture.
`src/render/world_effects.cpp` uses that glyph, tinted by spell, for travelling
spells and their impacts. `ProjectilePlayer` in `src/render/attack_visuals.cpp`
retains visible attacks only through `impactMs + 120`. `EffectFrame` holds a fixed
array of textured quads; `WorldRenderer::drawEffects` currently uses premultiplied
alpha blending with depth tests and no effect depth writes.

Prepared Grease, Web and Spiritual Weapon artwork already exists in
`assets/effects/`; the inn has flame artwork and bounded analytic hearth particles.
The environment shader receives world positions and surface normals, while
creatures use billboard shading. Local spell lights and sustained elemental body
overlays are new work, not existing capabilities to toggle on.

`AttackVisualSnapshot` supplies attack identities, origins, targets and deadlines;
the session also owns effect/condition and ground snapshots. Area casts can publish
per-target pulses, so SE01 must establish a single cast-level cone cue independent
of victim count. Inspect the producer/consumer path before choosing its record.
Do not emit one complete Burning Hands cone per victim.

### Simulation and cosmetic playback

Keep the session authoritative. Rendering consumes read-only attack/cast outcomes
and condition summaries; it never rolls dice, resolves hits or applies damage.
Any new cast cue carries stable world/cast identity, origin/facing, actual affected
cells and authoritative occurrence time sufficient to reconstruct its appearance.
Extend only concrete missing presentation data, without creating a second spell
executor or using particles to decide targeting.

A cohesive cosmetic playback owner retains short-lived aftermath independently
of the 120 ms projectile copy. It must not lengthen simulation busy/turn-handoff
deadlines. Deduplicate repeated snapshots/events, use cosmetic seeds separate from
combat RNG, and compute age from the paused session clock. Condition duration
comes from existing hybrid gameplay clocks; renderer elapsed time cannot expire
Slow, concentration or ground hazards.

Use typed world/actor/cast identities rather than long-lived actor pointers.
Define removal, death/corpse, cancellation, misses, scene change, Continue and
large-time-step behavior. Old bursts must not replay when a target becomes visible
again. Ephemeral particles are not save data; rebuild supported sustained visuals
from authoritative state after restore without replaying application bursts.

### Prepared artwork and bounded frame data

Prepare animation atlases, alpha/mip data and typed effect definitions before play.
Resolve names during preparation, retain resource leases through active/staged
views, and use existing generation-checked GPU handles and RAII. Validate frame
rectangles, timings, texture purpose, alpha convention and missing assets. Version
publication/cache identity if necessary; do not change save schemas merely for
new artwork. Failed preparation must preserve the current playable view.

Use contiguous fixed-capacity playback/particle/light records and reusable upload
storage. Batch compatible draws where practical without breaking transparent
ordering. SE01's detailed design must choose numeric budgets for concurrent casts,
aftermath, particles, lights, draw packets and active-plus-staged resource memory,
based on the actual bounded combat records and representative on-screen overlap.
The current one-quad capacity cannot be assumed to accommodate particle effects.

Define deterministic cosmetic exhaustion: preserve primary spell/impact readability
before secondary sparks, smoke or extra lights, and never change gameplay or grow
containers mid-frame. Invalid required assets reject preparation clearly. Keep
engine-owned frame processing allocation-free, including event consumption and
reusable GPU uploads; make no claims about driver-internal allocation.

### Lighting, depth and cache behavior

Start with a small fixed light list using short-range attenuation, surface facing
and conservative blocker handling. SE01 selects the concrete wall/door occlusion
method after inspecting current geometry; neither naive unoccluded radius lighting
nor a global shadow-map system is the agreed finished result. Keep creature
illumination modest and consistent with its painted form; detailed sprite normal
maps are not a prerequisite.

Respect depth, door-leaf masks, nearer creatures and world viewport clipping for
trails, halos and target attachments. Effects never write picking IDs. Active
lights/animation invalidate affected prepared colour correctly, while geometry,
picking and reusable environment work retain their existing cache boundaries.
After the last effect expires, idle colour caching resumes. Do not rerun expensive
static preparation or picking merely because a flame frame changed.

## Slice tracker and milestones

| Slice | Deliverable | Dependency | Status |
| --- | --- | --- | --- |
| SE01 | Fire Bolt, Ray of Frost and Burning Hands with animation, glow, local light and target aftermath | Approved detailed plan | Accepted |
| SE02 | Remaining offensive spells and healing/buff/damage portrait animation | SE01 Accepted | Accepted |
| SE03 | Support, conditions and persistent ground/summoned effects | SE02 Accepted | Ready for review |
| SE04 | Complete catalogue integration, visual consistency and delivery polish | SE03 Accepted | Planned |

| Milestone | Meaning | Status |
| --- | --- | --- |
| E-A | In-engine visual quality and technical foundation proven through SE01 | Accepted |
| E-B | All current spell families have intentional treatment through SE03 | Ready for review |
| E-C | Normal campaign delivery and final consistency review accepted through SE04 | Planned |

### SE01 — Fire, frost and cone foundation

**Purpose:** deliver the proposed starting set at finished visual quality and make
it the in-engine reference for subsequent spells.

Deliver Fire Bolt as an asymmetric animated flame with a short ember trail,
compact additive core, travelling warm light and an outward impact flare followed
by attached flames. Deliver Ray of Frost as a narrow crystalline lance with a
sparse cold wake, icy impact shards and a patchy target coating; transition to a
subdued condition-bound frost cue while its existing Slow remains active.

Deliver Burning Hands as one spreading fan of layered flame tongues from the
lower viewport through its actual eligible front cells, with a brief warm lighting
surge and hit remnants on affected creatures. Preserve its current instant
resolution: the flare begins at that deadline and its expansion is cosmetic.
Do not delay damage to accommodate a new windup or visible travel time.

Include prepared animation assets, blend modes, bounded playback/trails, local
light support, target attachment/masking and a deterministic review/capture path.
The effects must work through normal spell commands, not only a visual showcase.
Review the complete three-spell treatment before creating the remaining catalogue.

Acceptance:

- All three spells read distinctly without labels at normal viewport scale and
  near/far combat distances. Fire feels animated, frost feels brittle, and Burning
  Hands reads as a coherent area release even with sparse targets.
- A Fire Bolt illuminates nearby floor/wall surfaces as it passes and briefly
  lights the target at impact. Occluded surfaces do not visibly glow through
  walls/closed doors; no-light views retain accepted scene appearance.
- Alpha bodies and additive cores retain detail in a dark corridor and a crowded
  encounter. Fire/frost aftermath follows moving and hurt creatures without
  rectangular overlays, depth leaks or detached attachment points.
- Hits, misses, canceled casts, removed/dead targets, partial saves and empty area
  cells do not produce misleading target effects. Cosmetic remnants do not add
  damage, prolong turn holds or outlive scene ownership.
- Focused owned CPU checks cover cast deduplication, exact phase boundaries,
  pause/large steps, condition expiry, bounded exhaustion and frame allocations.
  Hidden GPU checks cover blend/depth/door integration, light removal/cache reuse,
  quality modes and resource lifetimes. Production review includes motion, not
  only attractive still frames; interactive feel remains user-owned.

### SE02 — Offensive spell identities

**Purpose:** extend the accepted foundation to the remaining offensive catalogue
without reducing spells to palette swaps.

| Spells | Intended treatment |
| --- | --- |
| Magic Missile | Distinct curved luminous darts with tapered trails, correct assigned dart counts and convergence at existing deadlines |
| Scorching Ray | Narrow fast fire streaks with ragged edges and concentrated flashes; distinguish from Fire Bolt and retain each ray's result |
| Guiding Bolt, Sacred Flame | Gold-white radiant streak/impact for the bolt; descending or target-centred sacred flame for the save-based spell; modest radiant aftermath |
| Shocking Grasp, Inflict Wounds | Close-contact branching electricity versus a dark violet inward pulse; no misleading long-range projectile |
| Acid Splash, Poison Spray | Heavy splash droplets and brief corrosive flecks versus a short drifting cloud; preserve their distinct target/range rules |
| Thunderwave, Shatter | Outward broken pressure ring/dust versus a compact target-area fracture burst; align with actual affected cells and resolved push behavior |

Acceptance:

- Motion, silhouette and impact distinguish force, radiant, electricity, necrotic,
  acid, poison and thunder at actual scale. Shared components remain visually
  appropriate; every spell need not have unique infrastructure or a local light.
- Magic Missile assignments and Scorching Ray hit/miss outcomes remain legible
  for one or several targets. Cosmetic curvature cannot imply another victim.
- Area releases use cast identity and real affected geometry, including empty
  cells, rather than multiplying a whole effect by recipient count.
- Existing guiding status, displacement, immunity and no-damage outcomes remain
  authoritative. Do not invent residual damage, poison or blindness conditions.
- Focused checks cover multi-target identities, event ordering, contacts, cover,
  representative overlap and unchanged gameplay outcomes. Review motion and
  selected dark/bright scenes with the accepted SE01 references alongside them.

### SE03 — Support, conditions and persistent effects

**Purpose:** make the rest of the catalogue intentional and readable, including
effects that remain after the cast, without surrounding every actor with clutter.

| Spells | Intended treatment |
| --- | --- |
| Cure Wounds, Spare the Dying, Lesser Restoration, Aid, False Life | Distinct recovery, cleansing or protective application pulses; party-only recipients use restrained existing portrait feedback rather than invented world bodies |
| Bless, Bane, Guidance, Resistance | Small boon/curse application accents and quiet sustained cues where useful; preserve authoritative status badges and duration descriptions |
| Shield of Faith, Mage Armor, Sanctuary, Mirror Image | Brief ward/shimmer or duplicate-image feedback appropriate to the represented recipient; persistent indications and consumption/end reactions reflect actual state |
| Sleep, Color Spray, Hold Person | Drifting sleep motes, a short controlled prismatic fan, and readable restraint accents; preserve existing prone/held poses and successful-save distinctions |
| Grease, Web, Spiritual Weapon | Retain accepted prepared artwork and add appropriate placement, existing burning-Web, summon, movement and strike accents |

Frost/Slow and Guiding Bolt sustained cues from earlier slices join the same
lifetime/readability policy. Preserve NPC spellcasting, including goblin shaman
Bless. No new NPC spell access or class-feature redesign is included.

Acceptance:

- Every current catalogue entry has an explicit treatment, including deliberate
  reuse of existing poses/art/portrait cues. Party-only spells do not create fake
  dungeon targets. Effects remain distinguishable from damage and target selection.
- Persistent cues follow application, replacement, concentration loss, expiry,
  consumption, waking, restoration and removal. Continue reconstructs the current
  state without replaying old cast bursts; world travel clears stale attachments.
- Existing Web ignition looks like burning strands/flames on its actual cells,
  respecting its current duration and damage. Grease is not made flammable by art.
- Overlapping buffs, debuffs, ground patches and creature effects retain readable
  silhouettes and HUD badges. Keep quiet sustained cues subordinate to new casts.
- Focused condition/ground/restore and render checks cover the changed consumers,
  allocation/resource lifetimes, party/NPC recipients and representative overlap.
  Use save smoke checks if restore integration changes; broader persistence checks
  require a concrete affected invariant, not just the presence of visual state.

### SE04 — Catalogue integration and final polish

**Purpose:** finish the delivered set as one coherent part of ordinary play.

Audit the current `SpellKind` catalogue against SE01–SE03, including cast, travel
or area release, hit/miss/save feedback, aftermath and persistent lifecycle as
applicable. The inspected baseline contains 32 spells; reconcile later additions
explicitly rather than assuming an old count is coverage. Remove obsolete spell
glint paths only after their replacements work; retain non-spell assets/callers.

Tune relative scale, brightness, animation cadence, trail density, aftermath
duration and existing sound/impact alignment across the set. Address crowding,
near-camera clipping, minification, moving/turning views and scene brightness.
Confirm normal campaign loading packages all required assets and that review
switches are unnecessary for ordinary gameplay. No new spell family is reserved
for this slice and no speculative optimization project is implied.

Acceptance:

- Catalogue audit has no accidental generic fallback; intentional shared treatment
  is documented. A user review route exercises representative spells through the
  normal UI in a corridor and a multi-enemy encounter, plus a bright environment.
- Existing movement, picking, targeting, enemy attacks, melee swipes, hearth,
  ground artwork, AO, dungeon lighting and stationary HUD remain coherent.
- Normal New Game/Continue, pause/resume and location travel retain correct visual
  state. Final transient cleanup restores idle caching and resource ownership.
- Record actual asset memory, active/staged GPU resources and configured effect
  budgets. Investigate observed regressions with a named bounded check; do not
  claim production-scale performance from tiny correctness fixtures.
- Run one meaningful affected integration checkpoint, reusing prior passing
  evidence where unchanged. Deliver final review instructions and known limits;
  user acceptance closes SE04 and milestone E-C.

## Verification, resources and evidence

Follow [the verification workflow](verification_workflow.md). Test engine behavior
with small deterministic generated fixtures independent of production art. Keep
CPU correctness/allocation tests windowless and GPU integration separate. Visual
review uses actual prepared production artwork; still captures establish shape,
lighting and occlusion, while short deterministic sequences establish timing and
motion. Unavailable GPU checks and interactive review remain explicitly unverified.

Never control desktop input, window focus or other applications. Use permitted
hidden application-owned captures and in-process scripted commands. Keep each
slice's capture set small and purposeful; no automatic all-spell/frame matrix.

Run one intensive job at a time with low CPU/I/O priority, at most two build workers,
one test process and bounded library threads. Check memory/scratch space before
baking or building. Benchmarks and capacity stress are opt-in investigations with
a concrete question, not routine acceptance steps. Verify bounded exhaustion with
small configurable fixtures where possible rather than full-world stress.

Create disposable output with `python3 tools/agent_artifacts.py create`. Keep logs,
captures and experiments in that run. Promote selected evidence to
`docs/screenshots/spell-effects/`, update its README, replace superseded images and
retain at most six images, or twelve pending review, unless distinct cases justify
an explicit exception. Keep source artwork and prompts as durable assets. Preview
cleanup with `python3 tools/agent_artifacts.py clean RUN_DIRECTORY`, then apply it
with `--apply`; document only selected retained failure diagnostics and their
cleanup condition. Preserve acceptance status when replacing evidence.

Each handoff records approved scope, final interfaces, asset regeneration steps,
numeric budgets/actual totals, exact checks/outcomes, skipped checks, selected
evidence, deviations, limitations, acceptance state and the next eligible action.

### Reusable prompts

> Read AGENTS.md, docs/system_design.md and docs/spell_effects_plan.md. Inspect the
> current checkpoint, previous slice handoff and relevant code. Plan the next
> eligible spell-effects slice only for review; preserve working-tree changes.
> Do not implement.

> Implement the approved spell-effects slice. Follow its detailed design, resource
> limits and desktop restrictions. Update the design, roadmap checkpoint and
> durable verification/handoff. Run focused checks and stop at Ready for review.

## Decision log

| Date | Decision |
| --- | --- |
| 2026-11-05 | User endorsed the proposed visual direction and requested a 3–4 slice roadmap following the campaign/city workflow. |
| 2026-11-05 | Four slices proposed; Fire Bolt, Ray of Frost and Burning Hands form SE01, including glow, local light and creature aftermath. |
| 2026-11-05 | Roadmap creation only: no detailed slice approval, implementation or delivered acceptance is implied. |
| 2026-11-05 | User approved the SE01 detailed plan and requested implementation; death-pose remnants and animated-door light occlusion selected. |
| 2026-11-05 | SE01 implemented and mechanically verified; delivered motion/stills Ready for review. User acceptance remains pending; SE02 remains Planned. |
| 2026-11-05 | User explicitly accepted SE01: “those are some nice spell effects, the local lighting especially sells it :) slice accepted”. E-A is Accepted; SE02 is next eligible and remains Planned. |
| 2026-11-05 | User approved the SE02 detailed plan and requested implementation, including green healing, blue buffs and elemental party-damage portrait animation with small readouts instead of named badges. |
| 2026-11-05 | SE02 implemented; focused CPU and hidden GPU checks passed, production motion/stills delivered. Ready for review; acceptance pending. SE03 remains Planned. |
| 2026-11-05 | User requested denser, greener Poison Spray and a more distinct green healing shimmer; both presentation adjustments implemented and recaptured. Other feedback was positive; SE02 remains Ready for review. |
| 2026-11-05 | User selected A, painted blood/ink splashes, and requested implementation. Replaced world/weapon starbursts, added instant/hold/fade and local damage-number tremor. Four focused CPU suites and three hidden production capture cases pass; Ready for review. |
| 2026-11-05 | User explicitly accepted SE02: “nice work, record the slice as accepted in the plan”. Acceptance includes the poison/healing colour corrections and option A painted combat feedback. SE03 is next eligible and remains Planned. |
| 2026-11-05 | User approved the detailed SE03 plan and requested implementation, selecting quiet portrait cues for wards and disabling conditions. |
| 2026-11-05 | SE03 implemented: all nineteen remaining spells, actual condition/duplicate state, ground and weapon accents. Thirteen focused CPU suites and both-quality hidden GPU checks pass; six review files delivered. SE03 and E-B are Ready for review, not Accepted; SE04 remains Planned. |

## Current next-agent handoff

SE01 remains **Accepted** (2026-11-05), including its local-light treatment and
[accepted reference gallery](screenshots/spell-effects/README.md).

SE02 is **Accepted** (2026-11-05), including the colour corrections and painted
combat-feedback follow-up. The ten remaining offensive spells use prepared
animation with independent multi-target deliveries, actual area footprints and
quiet authoritative guiding marks. Party spell damage, healing and positive buffs
use portrait pulses/shimmers and small numbers/outcome text. Ordinary weapon and
world-enemy feedback now uses approved option A: prepared painted blood/ink
backdrops, instant appearance, brief numeral tremor and fade. See the
[follow-up design/checks](combat_feedback_design.md) and
[animation previews](screenshots/combat-feedback/README.md). Names remain in logs/tooltips. See the
[implemented design, exact checks and memory totals](se02_design.md),
[selected motion/stills](screenshots/spell-effects/se02/README.md), and
[source artwork/prompts](../assets/effects/se02/README.md).

All 13 selected CPU suites and hidden GPU checks at both quality settings passed.
Further focused tests covered the final playback and restore regressions. Six
SE02 evidence files are retained; disposable run artifacts were cleaned. Interactive
feel, cross-device appearance and production-scale performance remain unverified.
No change to combat rules, save schemas or configured capacity ceilings.

SE03 is **Ready for review**. The user approved the detailed plan and selective
quiet portrait cues. The session-owned bounded support journal and fixed condition
snapshot drive recovery/boon/ward/curse presentation, actual Mirror Image duplicates,
Sleep/Color Spray footprints, restraint, Grease/Web placement, actual burning Web
cells and Spiritual Weapon summon/move/strike/end accents. Prepared assets and
existing simulation/save contracts are preserved. See the [implemented design,
checks and memory totals](se03_design.md), [six-file review gallery](screenshots/spell-effects/se03/README.md)
and [built-in ImageGen masters/prompts](../assets/effects/se03/README.md).

All thirteen selected CPU suites and hidden GPU checks at both quality settings
pass. Sixteen production command-driven capture cases passed, including NPC Bless,
Web ignition and weapon movement. No OS input or focus control was used. Six SE03
evidence files are retained and disposable run output was cleaned. Interactive feel,
cross-device appearance and production-scale performance remain unverified.
SE03/E-B await explicit user acceptance. SE04 remains Planned and was not started.
