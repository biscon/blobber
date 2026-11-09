# Authored dungeon rooms: proposed delivery roadmap

Created: 2026-11-09.
Status: **BR01 accepted and closed by the user on 2026-11-09.
BR02 and BR03 remain proposed.**

The user wants recognizable kitchen, armory, mead hall and barracks blueprints,
placed by rules within randomized dungeons. The proposed first playable version
contains exactly one of each on every generated dungeon floor. This document
records the recommended three-slice delivery; it is not approval to build all
slices. Current campaign, renderer and ceiling deliveries retain their own status.

## Intent and boundaries

Give rooms a readable purpose through authored spatial arrangements and sensible
relationships. Randomness chooses their positions, rotations, supported entrances
and surrounding connections. A room's essential furniture arrangement stays authored.

Preserve discrete movement, fixed 1920×1080 presentation, prepared assets, level
floors, current campaign length, and encounter/reward budgets. Retain the existing
ceiling hierarchy, with authored room profiles fitted to it. No new crafting,
cooking, sleeping, looting-every-prop, fire simulation, lighting system or floor
biomes in this delivery. Decorative weapon storage does not grant extra equipment.

Four authored rooms replace ordinary room selections within the existing floor
budget; they do not automatically add four rooms or enlarge every floor. Corridors,
loops, ordinary chambers and progression structure remain procedurally arranged.
Protected stair regions remain separate. Existing entrance/junction/approach/objective
roles describe progression; kitchen/barracks/etc. describe purpose and must not
replace those roles. Required rooms also appear in the checked fallback layouts.

| Room | Essential composition | Proposed relationship |
| --- | --- | --- |
| Kitchen | Wall-backed hearth, preparation surface, grouped provisions, working aisle | Direct connection to mead hall through a doorway or short passage |
| Mead hall | Long tables, clear central aisle, high ceiling and deliberate supports | Prominent connected chamber; entrances work with the table arrangement |
| Barracks | Repeated sleeping bays, footlockers, central circulation and encounter space | Prefer close graph distance to armory |
| Armory | Wall storage, equipment/work surface, clear access to any assigned reward | Prefer side-room placement near barracks rather than unrelated through traffic |

Kitchen/hall connection and legal circulation are hard requirements. Barracks/armory
proximity and armory side-room placement are preferences; their exact scoring belongs
in BR03's detailed plan. Requiring every relationship would make layouts repetitive
and increase rejection rates. One blueprint per purpose is enough initially; rotations
and authored doorway alternatives provide variation without arbitrary stretching.
Additional layouts, ruin/occupation variants and floor-specific room rosters are later
work. Unrelated random torture furniture should not intrude into these rooms; whether
it survives in ordinary rooms is an explicit BR03 dressing-policy decision.

## Delivery slices

| Slice | Deliverable | Dependency | Status |
| --- | --- | --- | --- |
| BR01 | Blueprint layouts, contracts and playable blockouts | Existing generation/rendering | Accepted and closed — 2026-11-09 |
| BR02 | Finished room kit and isolated in-engine room review | Reviewed BR01 layouts/contracts | Proposed |
| BR03 | Procedural placement, population, persistence and campaign integration | Reviewed BR02 rooms | Proposed |

### BR01 — Layouts and blueprint foundation

Detailed implementation: [BR01 design and handoff](br01_blueprint_rooms_design.md)
(implemented, mechanically verified and accepted by the user 2026-11-09).

Author the four top-down layouts before producing finished models. Each specifies
its exact grid footprint, allowed rotations and doorway sockets, ceiling profile,
authored furniture placements/collision footprints, reserved walking lanes, encounter
areas and candidate reward-access positions. Review a connected kitchen/hall pair
as well as the individual rooms.

Implement only the small blueprint data/loading/validation and isolated preview path
needed to walk these layouts using temporary geometry or existing assets. Keep normal
campaign generation unchanged. This slice includes foundation code: it is not only a
collection of concept images. Its handoff must establish the placement-coordinate,
pivot, scale and collision contracts that BR02 models and BR03 generation share.

Validate connected entrances, room-local circulation around furniture, door swings,
container access and usable combat space. Whole-cell traversal restrictions must be
visible in the blockout: visually narrow furniture must not promise a walkable route
that the grid forbids. Check transformed footprints and sockets in every permitted
orientation. Do not mirror directional artwork by default.

**Exit evidence:** four readable top-down plans, representative hidden in-engine
blockout views, focused owned-fixture validation results, and a concrete asset list
with dimensions/anchors. Exact dimensions, numeric capacities and the file schema
are settled in this slice's detailed design before coding; they are deliberately
not guessed by this roadmap. User review settles layout and circulation before art.

### BR02 — Artwork and assembled-room review

Create the required reusable kit and room-specific focal pieces against BR01's
contracts. Start with the kitchen/mead hall pair to establish proportions, material
style and reuse, then complete barracks and armory. A kitchen and barracks may share
storage pieces; each room still gets a distinctive authored arrangement.

Use the existing concept/texture and Blender/prepared-mesh workflow, matching Underkeep
art. Retain source artwork, prompts, editable models, export inputs and texture masters.
Use modular or deliberately grouped furnishings with explicit collision footprints;
avoid baking a whole room into an opaque monolithic model. Keep room terrain, doors,
fixtures, furniture and collision ownership distinguishable.

Add only the asset registration/publication/render bindings required to preview the
finished rooms through the actual game renderer. Asset integration is part of this
slice; waiting until BR03 to discover scale, clipping or publication problems defeats
the split. Normal campaign placement still remains unchanged.

**Exit evidence:** all four rooms assembled and visually reviewed in-engine, with
correct native scale, floor/ceiling attachment, door clearance and asset/resource
budgets. Focused generated-asset tests check export/loading/lifetime behavior; production
captures check appearance. Concept art or Blender-only views do not establish readiness.

### BR03 — Generation and playable campaign integration

Select required blueprint rooms early, place their geometry and legal sockets, then
connect them and the ordinary rooms through bounded procedural layout generation.
Do not try to squeeze fully furnished blueprints into arbitrary completed rooms.
Preserve useful loops, protected stairs, keys, guardian progression and legal arrival
space. Test the required room roster in both layout profiles and authored fallbacks;
never silently omit a required room when placement fails.

Reserve authored furniture and circulation before encounter/reward placement. Let the
existing budgets choose suitable room-owned encounter and reward sites without adding
loot or encounters merely because a themed room exists. Generic dressing must respect
blueprint reservations and purpose; it must not overwrite the authored composition.
Replace the current occasional-furniture approach where these blueprints apply.

Integrate room identity into diagnostics, canonical fingerprints, saved definitions
and deterministic replay. Preserve the current strict campaign compatibility policy:
changed generation/content identities require new campaigns; do not rewrite user saves.
Decide exact version increments from the implemented formats at this checkpoint.

**Exit evidence:** deterministic fixed-seed floors containing exactly one of each room,
clear kitchen/hall connection, varied surrounding topology, working movement/combat/
progression and active/inactive save/restore. Include bad blueprints, placement exhaustion,
fallbacks and resource-lifetime failures in focused coverage. Review selected full-floor
maps and first-person transitions; final acceptance requires user visual/play review.

## System responsibilities and performance

A small immutable blueprint catalog owns authored local layouts and placement rules.
Generation chooses instances and transforms, then publishes ordinary world geometry,
fixtures and prepared decoration placements. Generated-floor metadata owns stable
blueprint/instance identity as required for diagnostics and reconstruction. Runtime
systems consume the published map; no live blueprint interpretation or per-frame
placement searches are introduced.

Keep generation geometry, furnishing reservations, population, asset preparation and
scene composition separate. Extend the existing representation only where a concrete
new furnishing requires it; no general prefab/entity/editor framework is planned.
Reuse contiguous cell membership/masks and bounded room/socket/path storage. Define
catalog/instance/placement capacities and bounded retry/fallback behavior before coding
BR01/BR03 respectively. Invalid content or exhausted preparation must fail clearly and
leave the current playable session intact. Review any necessary budget increases
against actual room assets; do not silently expand limits to hide failures.

The current furniture helper assumes isolated one-cell props and a supporting clear
patch. Authored multi-piece arrangements need validation of their combined occupied
footprint and remaining room-local circulation. Establish that distinction in BR01;
do not admit each piece independently and assume the final arrangement is safe.

## Working sequence and verification

For each slice, read current code and [system design](system_design.md), prepare its
practical detailed design here or in a linked document, then implement the scope the
user authorizes. Direction, implementation completion and user acceptance are separate
states. Honor explicit session authorization without repeatedly requesting it. Do not
advance to the next slice automatically; incorporate review corrections in the current
slice and record any changed contracts for its successor.

Follow [verification workflow](verification_workflow.md): one intensive job at a time,
at most two build workers and one test process, configured toolchains, low priority and
owned small fixtures. Tests must not depend on production room artwork. Production art
review is a separate hidden application-owned capture step; no desktop input/focus
control. No routine benchmark, balance study or broad seed sweep is implied.

Maintain a small feature gallery (at most six retained images, or twelve pending review)
with acceptance status. Record commands, outcomes, limitations and the next action in
the relevant slice design; do not create a permanent report for each attempt. Clean
owned disposable runs after selecting evidence and preserve source art and test inputs.

## Current checkpoint

- BR01 is **accepted and closed**, 2026-11-09. User approval: “slice approved, update the plan doc”.
  The [detailed handoff](br01_blueprint_rooms_design.md)
  records the four layouts, eight kit contracts, developer launcher, focused CPU/GPU
  checks, failure corrections and reproduction commands.
- [Five plans and five native blockout views](screenshots/blueprint-rooms-br01/README.md)
  record the accepted BR01 layouts and circulation contracts. Campaign generation and
  save versions are unchanged.
- Next action: separately plan BR02 artwork and assembled-room review against the accepted
  BR01 contracts. Approval of BR01 does not authorize implementation of BR02 or BR03.
  Disposable run output has been cleaned.

Related foundations: [variable layouts and ceilings](procedural_layout_design.md),
[Underkeep dressing](underkeep_dressing_design.md),
[world environment](renderer_environment_design.md).
