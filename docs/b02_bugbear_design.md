# B02 — Bugbear and sprite memory preparation

2026-10-31. **Accepted and closed (2026-10-31).** Detailed plan approved by the
user with “Implement the plan.” The user accepted turnaround-v1.png with “reference accepted, looks great”;
full production animation is delivered. The user accepted the completed slice with
“slice accepted, record it in the plan doc”.

Current preparation settings and budgets are described in
[world sprite resolution](sprite_resolution_design.md) and the
[travel residency correction](sc011_design.md#campaign-travel-residency-correction--2026-11-07).
The full-resolution extraction and capacity values below record the original B02
delivery; selected actors now use uniform offline reductions before cropping.

## Delivery and ownership

Deliver a bulky, single-subcell melee bugbear through normal Arena Setup. Append
immutable catalog/appearance definitions; ActorWorld retains identity, HP and
occupancy, combat retains decisions/rolls/impact scheduling, and the existing
prepared-world owner retains textures and masks. No new runtime resource owner,
combat trait framework, footprint, reach, campaign population or streaming system.

Adopt SRD 5.1 p. 266 abilities 15/14/13/8/11/9, AC 16, HP 27, 200 XP and one
morningstar +4, 2d8+2 piercing. Brute is included in those two dice, never applied
again; ordinary criticals are 4d8+2. Humanoid, sleep-susceptible, nonmetal hide
armor, normal damage profile, no Nimble Escape. Omit Surprise Attack and javelin.
Guaranteed loot: one existing shield; morningstar/hide equipment is not added.
Preserve normal approach, formation protection, target selection and control.

## Prepared texture layout and budgets

Raise world mip payload capacity from 256 to 512 MiB and tracked GPU capacity
from 352 to 608 MiB. Retain 64 MiB alpha, 352 MiB CPU, existing scene/scratch,
preparation and count limits. These are checked engine payload ceilings, not whole
process or driver residency guarantees. Preparation may allocate; gameplay cannot.

NPC-only cropping runs offline after registered extraction at existing full
prepared resolution. Scan alpha above the existing cutoff, reject empty poses,
expand bounds by two texels within the original canvas and copy without resampling.
Premultiplied mips and alpha masks use the stored rectangle. Other textures retain
identity crops. Keep source masters, authored scales, anchors and clip dimensions.

Prepared Texture adds original canvas width/height and crop x/y; existing texture
width/height describe stored pixels. Validate containment and exact payload sizes.
Format becomes 3; appearance revision remains 4. Metadata participates in canonical
publication identity. Old publications reject with rebuild guidance.

Billboard feet, logical dimensions, anchors, release/torso attachments, edge fitting
and shadows remain in original canvas space. A normalized crop rectangle selects
the drawable region. Shared corners drive GPU transforms and CPU alpha picking,
with full UVs on the stored image. Convert alpha bounds back to original canvas
coordinates for prone/status anchors. No per-frame image work, atlas or packer.
The offline baker offers --uncropped-npcs only for deliberate before/after
verification publications; ordinary builds always crop NPCs.
Visibility identity becomes world-visibility-v6. Retain standalone texture leases.

Cropping retains all base texels above cutoff. Regenerated mips may differ in
sampling alignment; inspect filtered edges and distant poses before considering
the optimization verified. Do not reduce prepared resolution to satisfy budgets.

## Art and arena contract

Use imagegen skill/built-in generation with actual goblin/archer image references.
Review a front/left/back/right turnaround before generating the full set. Shaggy,
bulky goblinoid, muted hide armor, right-hand morningstar, left-hand shield. Final registration uses
.95-cell logical canvas height (raised-weapon headroom), .5 body width; clip widths follow registered canvases
within one cell. Transparent masters/prompts/corrections/registration remain in
assets/npcs/bugbear; extracted derivatives live in the build tree.

48 poses / 21 sheets: four-direction idle (600/600 ms), walk (300/300), hurt
(150/150), death (200/200/200/600), prone (one static); front melee
(700/250/250/350, impact 950 ms). Preserve equipment and ground; do not scale
collapsed frames to standing height. Add generated initiative art to existing UI
atlas and species discriminator. Arena selection/save/edit/rematch reuse current
interfaces. Seed-5 review: one bugbear; one bugbear plus two melee goblins. These
are dangerous level-one encounters, without hidden protection or weakened stats.

Rules schema remains 10, campaign schema/simulation 3/4, arena format unchanged.
Content fingerprints reject incompatible prior saves without rewriting files;
strict replay checks new binary/content/publication/visibility identities.

## Verification and handoff

Owned small fixtures cover asymmetric crop/anchors, holes/thin detail, odd sizes,
edge contact, empty/malformed data, identity crops, base texel retention, CPU/GPU
picking, attachments and badges. Focused combat checks cover ordinary/critical
damage without doubled Brute, approach/front protection, control, interruption,
death/rewards and allocation guards. Check arena persistence/rematch, species
feedback, replay and campaign save smoke/content mismatch. No production art test
fixtures. Inspect existing-monster before/after and bugbear captures at near/far,
diagonal/overlap and reaction/death states. Use application-owned hidden windows;
never OS input or desktop/focus control.

One Release configuration, one intensive job, <=2 build workers, one test process,
low CPU/I/O priority; no routine benchmarks or stress matrix. Report actual bake
totals and limitations. Keep docs aligned; retain evidence in b02_verification.md,
finish Ready for review, await user acceptance; do not begin B03/campaign work.

## Delivery checkpoint

The approved design is implemented. All 48 poses, arena binding, initiative art,
crop metadata/shared geometry and revised memory ceilings are delivered. Focused
CPU/publication and hidden GPU checks pass; production traces/captures are retained
in [verification](verification/archive/b02_verification.md). The reference and completed slice are accepted. B03 is next for separate detailed
planning; no B03 implementation or campaign integration is started.
