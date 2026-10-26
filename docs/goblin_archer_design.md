# Slice 8.2: distinct goblin archer artwork and loadouts

Status: **Implemented, accepted and closed**, 2026-10-26. The user approved the
implementation plan with “Implement the plan.” The design below is delivered;
see [verification and handoff](slice82_verification.md). Slice 8.1 is accepted;
8.3 remains Planned. The user accepted the completed slice with “slice accepted,
close it up”. Attack-consequence timing remains unchanged.

## Visual direction and review gate

The user selected a distinct outfit: a muted forest-green hood and short shoulder
covering over brown leather, with face and ears exposed, wooden shortbow and back
quiver. Retain the existing goblin face, proportions, boots and detailed shaded
retro dungeon style. Bow stays in the anatomical left hand; the right draws.
No dagger is held. The melee goblin artwork remains intact.

Use the built-in imagegen skill. First generate a four-view reference and present
it beside the existing melee goblin. **Obtain user review of that reference before
generating the full animation set**, as required by the roadmap. Plan approval
does not constitute approval of an unseen reference. Preserve selected masters,
reference images, exact prompts/corrections and registration metadata in the new
archer asset directory. Existing bow sheets are pose references, not final assets
for the new outfit. Do not use OS input or focus automation.

After reference approval, author twenty sheets (forty-eight poses):

| Clip | Directions | Frames per direction | Durations (ms) |
| --- | --- | ---: | --- |
| Idle | Front/back/left/right | 2 | 600/600 |
| Walk | Front/back/left/right | 2 | 300/300 |
| Ranged attack | Front/back/left/right | 2 | 600/400 |
| Hurt | Front/back/left/right | 2 | 150/150 |
| Death/corpse | Front/back/left/right | 4 | 200/200/200/600 |

Draw/release poses switch at 600 ms; do not author a flying arrow. Preserve
equipment and identity across clips, with a consistent ground plane and body
scale. Measure sheet splits, pivots and ground planes; use one scale per sheet,
never normalize individual collapsing poses. Ordinary canvases are 384×536,
bow canvases 512×536, death canvases 768×536; corresponding world widths are
0.45/0.6/0.9 and height stays 0.65. Normalized anchors remain centered at
512/536 vertically. Reference and sheets must fit these existing bounds.

Reference accepted on 2026-10-26: user said “yes it looks great, approved :)”.
Selected reference: [turnaround-v2.png](../assets/npcs/goblin-archer/reference/turnaround-v2.png).
This approves the visual reference, not the completed slice.

## Content, interfaces and ownership

Add a level-lifetime appearance named `goblin.archer` after `goblin` in the level
definition order. Its matching catalog monster already exists and is bow-only.
ActorWorld's existing name binding initializes the correct rules without a new
schema or a runtime equipment-rendering system. Bind hybrid-demo actors 4 and 5
to this new type, retaining positions/routines and removing redundant monster
overrides. Capture selection must recognize their default binding. Generic
placement overrides remain supported and tested.

Remove the shortbow from the production `goblin` monster and `ranged_attack`
from its appearance. Retain all stats, traits and attack values. Give its
`source_sheets` a melee-only registration; preserve the original full registration,
bow masters, extracted frames and provenance for historical reproduction.
The normal level's placements remain melee goblins. Document these role loadouts
as project adaptations in the source ledger. Keep mixed-weapon fixture catalogs
independent of production data; do not weaken or change the engine's mixed policy.

The renderer owns both immutable prepared appearance banks. Runtime actors retain
typed appearance/catalog IDs and the animator copies bounded per-type timings.
No new gameplay API, resource lifetime, JSON schema, binary format, frame allocator,
AI policy, camera or movement behavior is required. Damage and occupancy release
remain immediate until slice 8.3. Capture helpers belong in cohesive app diagnostic
sources, with startup limited to orchestration.

## Preparation and performance

Follow the accepted [8.1 integration contract](image_preparation_design.md#implemented-interfaces-and-appearance-integration-contract).
Register archer masters through `source_sheets`; all runtime derivatives are
generated in the build tree. Preserve global area/half-resolution NPC profile C
and point-sampled surfaces. The 512 MiB resident and tracked loading limits remain
separate and exhaustion remains an explicit error. No per-appearance exceptions,
runtime decoding, atlas or paper-doll path.

Measure the actual two-appearance banks rather than extrapolating from duplicated
8.1 art: source/derivative bytes, stationary/travel/shadow arrays, metadata,
resident bytes, tracked loading high water and bake/load time. Process RSS is a
separate observation, not an equivalent accounting measure.

## Validation and completion

Generated fixtures must cover distinct appearance/rules bindings, missing bindings,
different clip sets, loaded-bundle round trips and animation selection. Retain
single-weapon and mixed-weapon behavior tests, including closing, shooting,
distance creation and trapped contact. Extend the loaded-bank allocation guard
to both appearances through movement, attacks, reactions, death and picking.

Extend preparation/hybrid diagnostics with every archer pose/direction, near/far
views, travel, strafe, wall occlusion, diagonal edge slots, matching shadows and
visible-body picking. Exercise both roles together. Inspect source sheets and
native/reduced deterministic captures; authored consistency requires visual review.

Run Debug/Release builds and CTest, shipped catalog validation, `--check-prepared`,
focused captures and the hidden GPU lifecycle check. Check reproducible clean/repeated
bakes and loading without NPC/material sources. Preserve existing broad-capture
victory failure as a documented limitation, never weaken its assertion. No OS
input/focus automation; interactive review belongs to the user.

Record executed commands, limitations, regeneration instructions and selected
captures in `slice82_verification.md`. Update this document and the system overview
to reflect the delivered result, then mark only 8.2 **Ready for review** and stop.
The 8.3 handoff must retain appearance IDs, clip durations and the 600 ms bow-release
boundary, explicitly distinguishing current immediate gameplay consequences from
future impact timing. Only user acceptance closes this slice.

## Delivered integration

The twenty masters and their five preserved corrections live under
`assets/npcs/goblin-archer/animation/`; prompts and measured registration are
tracked beside them. The level registers `goblin` then `goblin.archer`, but binding
uses names rather than catalog order. `sheets-melee.json` omits the original bow
sheets from production baking while the full historical `sheets.json` remains.

`src/app/archer_captures.cpp` owns production validation and the role-specific
capture suite. Existing preparation/hybrid commands invoke it; application startup
only orchestrates. Captures exercise actual actor attack, travel and damage cues,
assert pose/frame selection, and check living-target visibility and corpse expiry.
Generated fixture coverage lives in actors, prepared-assets and frame-allocation
tests. No engine APIs, binary formats, AI rules or rendering algorithms changed.

The verified bank sizes, visual review evidence and reproduction commands are in
[slice82_verification.md](slice82_verification.md). Slice 8.2 is closed; 8.3 is
eligible for planning in a clean context and requires its own approved plan.
