# System design

[Campaign travel residency correction](sc011_design.md#campaign-travel-residency-correction--2026-11-07):
implemented and mechanically verified. The user-requested 1 GiB world GPU limit
accommodates shipped city/dungeon overlap. Focused CPU/GPU residency checks and
hidden travel with the reported save pass; Debug rebuilt, save unchanged.
[Two review captures](screenshots/campaign-travel-budget/README.md); interactive review pending.

[Automatic introduction read status](dialogue_design.md#automatic-introduction-read-status): implemented and verified; introduction topics use existing per-resident history to appear read immediately, including existing saves.

[Dialogue panel cleanup](dialogue_design.md#dialogue-panel-cleanup): implemented and mechanically verified; larger speech, faster reveal and explicit Next controls. Focused tests and hidden captures pass; [visual review pending](screenshots/dialogue-panel/README.md).

[OnlyScrolls overlay timing correction](dialogue_design.md#overlay-timing-correction): implemented and mechanically verified; YAML controls delayed show after the document pull and first-Next dismissal. Focused tests and refreshed hidden captures pass; interactive review pending.

[OnlyScrolls portrait briefing](dialogue_design.md#onlyscrolls-portrait-briefing) **implemented and mechanically verified; artwork/timing review pending (2026-11-06)**: persistent prepared dialogue overlays, centered Gwendolyn parchment, Boric document pull and Mayor rage. Focused CPU, GPU lifecycle and hidden production checks pass; new campaign required by the changed NPC-content fingerprint. [Six review captures](screenshots/onlyscrolls/README.md).

[Multi-NPC dialogue](dialogue_design.md#multi-npc-event-beats) **implemented and mechanically verified; timing/listening review pending (2026-11-06)**: per-host YAML timelines, independently authored speaker/camera/reaction/audio beats, 200 ms minimum/default pans; Talk/Laugh end with text reveal. Reaction files live under `assets/audio/dialogue`. The reveal/pan/audio-location correction passes focused CPU and hidden production checks; interactive review remains pending. Existing campaign history and resident placement remain compatible. [Six review captures](screenshots/multi-npc-dialogue/README.md).

[Sanctuary mayor and guards](sanctuary_residents_design.md): implemented and
mechanically verified; runtime appearance approved. The dialogue idle correction
passes focused CPU and hidden render checks. Stationary residents, mayor
dialogue, prepared Idle/Walk/Talk and guard Laugh; requires a new campaign
(schema/simulation 12/17). [Review captures](screenshots/sanctuary-residents/README.md).
The user approved the residents' appearance. A dialogue presentation correction
keeps nearby residents idling while simulation is paused; see the resident design.
Dialogue ambience correction, implemented and verified: separate the ambient-stream pause from gameplay
audio so conversations preserve continuous background sound. Other pause causes
retain their current behavior; details in [audio design](audio_design.md#dialogue-ambience).

[Mayor](mockups/mayor-v1/README.md), [Boric](mockups/boric-v1/README.md) and
[Balin](mockups/balin-v1/README.md) **v2 concepts approved (2026-11-06)**.
[Gwendolyn](mockups/gwendolyn-v1/README.md) **v3 concept approved (2026-11-06)**.
Each character folder retains only its approved sheet and generation prompt;
superseded versions were deleted at the user's request. Use those approved sheets
for future work. These concept folders follow the innkeeper mockup format;
production mayor/guard assets live separately under `assets/npcs/`.

[Friendly NPC dialogue](dialogue_design.md) **implemented and mechanically verified;
awaiting runtime visual/interactive review (2026-11-06)**: the original JSON delivery is now superseded by YAML event beats above;
per-character introduction/fact history, gradual text/Talk, fixed-viewport overlay
and shared 700 ms conversation camera, based on the approved mockup.
Eleven focused Release CPU suites, the extended campaign save check and hidden
CPU/GPU parity pass; [six runtime captures](screenshots/dialogue/README.md).
Dialogue introduced schema/simulation **12/16**; Sanctuary residents now advance
compatibility to **12/17**, requiring a new campaign as authorized.
Trading, quests and procedural rumours remain later work.
The user accepted the interface and fluid transition; an [eye-level correction](dialogue_design.md#eye-level-review-correction--2026-11-06)
removes the rejected counter-height framing in favor of generic NPC-height framing.
The new framing awaits review; this correction preserves current saves.

Innkeeper **implemented and mechanically verified; awaiting runtime visual review (2026-11-06)**:
[Resident design](innkeeper_design.md) adds one stationary, peaceful innkeeper
behind the bar with approved artwork, four-view idle and front-only talk.
Twelve focused Release CPU suites and hidden OpenGL checks pass;
[six review captures](screenshots/innkeeper/README.md). Campaign schema/simulation
were 11/15 at the resident slice; dialogue advanced them to 12/16, and the mayor
and guards now require 12/17.
Trading remains deferred.
The user rejected the initial runtime scale impression and side orientations.
The [review correction](innkeeper_design.md#runtime-review-correction--2026-11-06)
reduces only the counter/rear shelves by 15%, corrects the NPC side-art assignment
and verifies the closest counter-facing subcell. Focused inn and hidden production
checks pass; the corrected presentation awaits re-review. That cosmetic correction
preserved compatibility; the subsequent dialogue slice requires a new campaign.
Character-reference approval is unchanged.

[Friendly NPC dialogue mockup](mockups/dialogue-v1/README.md) **presentation direction
approved (2026-11-06)**. The implemented UI uses the agreed fixed-viewport overlay
and smooth camera approach; runtime acceptance is separate from mockup approval.

Camera framing B **accepted and implemented (2026-11-06)**:
[projection comparison](renderer_world_design.md#camera-framing-trial--2026-11-06)
sets the horizon to 40% (about 5.8° down at screen centre), retaining .4-cell
pullback. The user selected B after comparing 34/40/45%. Three focused CPU suites,
projection checks and hidden CPU/GPU visibility checks pass;
[accepted B and original reference](screenshots/camera-framing/index.html).

Underkeep prop safety **implemented and mechanically verified; awaiting visual review (2026-11-06)**:
[placement and rack correction](underkeep_dressing_design.md#room-clearance-and-rack-correction--2026-11-06)
requires room-local clearance/connectivity and collision-aware progression checks.
Eight focused Release suites and hidden rack-only OpenGL checks pass;
[two review captures](screenshots/prop-placement/README.md) retain pending visual acceptance.
The rack uses one collision cell and half-size prepared geometry. Generator 10/7
requires a new campaign; established terrain/population seed streams stay 8/6.

[Shared Settings](settings_design.md) adds a persistent borderless fullscreen
checkbox to the main/pause Settings screen, using raylib window-mode handling
while preserving fixed logical presentation. Interactive desktop review is pending.

Spell effects SE04 **accepted and implemented; plan closed (2026-11-06)**:
[Integration audit](se04_design.md) preserves the accepted visuals, audits all spell
families and normal campaign lifecycle, and removes only redundant legacy paths.
Removed the redundant generic spell renderer while retaining arrow glints. Three
focused suites pass, including pause/Continue/travel checks. Accepted visuals and
resources are unchanged; prior GPU evidence is reused. SE04/E-C are accepted; the spell-effects roadmap is closed.

Spell effects SE03 **accepted and implemented (2026-11-06)**:
[SE03 design](se03_design.md) adds support, condition, ground and summoned-effect
presentation with selective quiet portrait cues. A bounded session journal carries resolved
cosmetic events; fixed condition snapshots reconstruct sustained cues after restore.
Gameplay/save contracts remain unchanged. Thirteen focused CPU suites and hidden GPU
checks at both quality settings pass; [six-file review gallery](screenshots/spell-effects/se03/README.md).
The user accepted SE03 and its review corrections, closing E-B; SE04 is also accepted and the plan is closed. The SE03 review correction lowers
NPC Bless to the feet anchor and adds bounded cosmetic Spiritual Weapon bobbing and
attack lunge/return. Next-round Abilities controls pass focused UI tests and the spell
description now points to them (see SE03 design). A
[party-step correction](se03_design.md#spiritual-weapon-follows-party-steps--2026-11-06)
keeps active weapons travelling with the party, with shared collision-aware
destination prediction and smooth presentation; command cooldowns are unchanged.

Spell effects SE02 **accepted and implemented (2026-11-05)**:
[SE02 design](se02_design.md) extends offensive spell identities and replaces party
spell/healing/buff badges with prepared portrait animation and small readouts.
Focused CPU and hidden GPU checks pass; [SE02 review gallery](screenshots/spell-effects/se02/README.md).
Poison density/green and healing green were strengthened following user review.
The user accepted SE02 including these corrections.
World and ordinary weapon feedback option A is **accepted and implemented**:
[painted feedback design](combat_feedback_design.md). Prepared blood/ink splashes
appear instantly, with a brief damage-number tremor and fade-out; focused CPU and
hidden OpenGL checks pass. See the [animation previews](screenshots/combat-feedback/README.md).

Spell effects SE01 **accepted and implemented (2026-11-05)**:
[SE01 design](se01_design.md) implements Fire Bolt, Ray of Frost and Burning Hands
with prepared animation, selective glow, bounded local light and creature aftermath.
Combat timing/rules and prepared-image presentation remain unchanged. The user
selected death-pose remnants and animated-door light occlusion. See the
[four-slice roadmap](spell_effects_plan.md) and [motion/capture gallery](screenshots/spell-effects/README.md); the user accepted the slice, particularly its local lighting. SE01 acceptance remains unchanged by the SE02 extension.

Spell targeting **corrected; Ready for review (2026-11-06)**:
[spell design](spells_design.md#acid-splash-proximity-correction) replaces the same-cell
restriction with subcell-aware distance, allowing equally close pairs across cell
boundaries. Shared command validation and UI feedback use the same rule. Spiritual Weapon
commands also gain immediate single-enemy clicking while retaining floor-only moves.
Action Surge activates directly when chosen in Abilities.
[Targeting hover correction](target_selection_design.md#spell-targeting-inspection--2026-11-06)
keeps enemy health bars and HP details available while choosing spell recipients.
[Shocking Grasp correction](expanded_spells_design.md#rear-row-shocking-grasp) restores
casting from either party row while retaining exposed enemy reach in the front cells.

Enemy melee lunge **accepted and implemented (2026-11-06)**:
[combat feedback design](combat_feedback_design.md#enemy-melee-lunge) adds a short,
impact-timed forward offset to the existing prepared attack animation. The user
accepted the doubled distance (0.28 cell), preserving timing and the attack-start
facing anchor.
Attack-direction correction: select directional artwork from the attack-start
position throughout a triggered melee clip, so diagonal enemies keep their facing
while the rendered body lunges across a viewing-sector boundary.
Authoritative positions, attack timing and ranged/casting presentation remain unchanged. Seven
focused CPU suites and six hidden production capture cases pass; see the
[melee preview](screenshots/combat-feedback/README.md#enemy-melee-lunge) and
[targeting evidence](screenshots/spell-targeting/README.md).

Stone dungeon distance lighting **C / Early accepted and implemented (2026-11-05)**:
[design](dungeon_lighting_design.md) defines three `underkeep`-only attenuation
presets and [matched comparison captures](screenshots/dungeon-lighting/README.md).
Focused CPU and hidden GPU checks pass; production city/inn images are identical
across presets. The selected C preset is the stone-dungeon default;
fog, AO and other biomes are unchanged.

Configurable world-window screenshake **implemented and mechanically verified; awaiting user review (2026-11-05)**:
[design](screenshake_design.md) adds bounded cosmetic impact impulses and a clipped
prepared-image transform, with stationary HUD and inverse-mapped world picking.
World rendering, movement and save formats remain unchanged. Eight focused CPU suites
and hidden OpenGL checks at both quality settings pass; [six preset previews](screenshots/screenshake/README.md)
retain pending timing/feel acceptance.

Procedural party melee swipe **implemented and mechanically verified; awaiting user review (2026-11-05)**:
[design and verification](melee_swipe_design.md) adds a bounded screen-space ribbon
around the unchanged authoritative impact deadline, using prepared body-only bounds.
Focused CPU/allocation and hidden OpenGL checks pass; [six review captures](screenshots/melee-swipe/README.md)
show the revised thicker, broader crescent with translucent edges after the user rejected
the first thin treatment. The user accepted the shape; the display
time is now tuned to 185 ms for trial (application rebuild only; tests not rerun). [Creature tooltips](target_selection_design.md#delayed-creature-hover--2026-11-05-implemented)
now wait 250 ms and hide during party/hovered-creature attack presentation. Focused
HUD/targeting/swipe and hidden GPU checks pass. Save compatibility and existing impact feedback remain unchanged.

## Agent artifacts and evidence

Artifact housekeeping is implemented (2026-11-05). Agents own disposable
output under ignored `output/agent-runs/<run-id>/`, created by
`python3 tools/agent_artifacts.py create`. The helper's `clean RUN_DIRECTORY`
previews removal; `--apply` removes only that marked run. It rejects symlinks,
unmanaged paths and tracked content. No age-based sweep or background service
owns other runs, build caches, artwork or saves.

At task completion, promote selected evidence into `docs/screenshots/<feature>/`,
update the existing design/handoff, and clean the run. Galleries retain at most six
images, or twelve pending user review, unless their README explains distinct cases
requiring an exception. Raw logs, traces and intermediate captures are disposable;
unresolved failures may retain selected diagnostics with reproduction instructions
and an explicit cleanup condition. Durable test conclusions remain in authored
verification documents; completed historical reports live in
`docs/verification/archive/`. Source artwork, prompts, references, test fixtures
and calibration manifests consumed by tools are separate from run output.
Browse [selected evidence](screenshots/README.md) and the
[historical verification archive](verification/archive/README.md).

The helper uses only the Python standard library and runs outside engine frame
processing. Verification uses isolated temporary directories for dry-run, deletion,
path/symlink rejection and tracked/neighbor protection; documentation checks replace
gameplay builds for housekeeping changes. See the
[verification workflow](verification_workflow.md#artifact-housekeeping) for commands.

Housekeeping verification (2026-11-05): six isolated helper cases pass directly
and through the `agent_artifacts_tools` CTest registration; seven route-tool checks
pass with the retained calibration input. A synthetic Pillow fixture exercises
the gallery helper's separate input/output paths, generated links and overwrite
refusal. Local Markdown file links and gallery limits were checked. No gameplay
build, rendering capture, desktop automation or benchmark was run for this change.

## Current systems and review status

Viewport interaction cleanup **implemented and mechanically verified; awaiting user review (2026-11-05)**: the
[HUD revision](native_hud_design.md#viewport-interaction-prompts--2026-11-05)
replaces bottom banners with actionable, shadowed white text and hides them outside
safe Exploration. Player-facing messages use gameplay language; technical failures
remain diagnostic output; creature names and descriptions hide development identifiers
and notes without changing catalog fingerprints or saves. Focused CPU/allocation and
hidden OpenGL checks pass; [four review captures](screenshots/interaction-prompts/README.md)
cover city, dungeon, loot and active combat. The
[audio follow-up](audio_design.md#travel-cue-removal--2026-11-05)
removes the shared travel completion sound without changing transitions or music.

Sanctuary inn **implemented and mechanically verified; awaiting user review (2026-11-05)**:
[design](inn_interior_design.md) adds an authored fifth campaign location,
facing-sensitive doorway travel, a furnished common room and decorative hearth,
and moves free long rest from the city sanctuary cell to the entire inn.
Campaign schema/simulation are 10/14; older saves reject without migration.
[Verification](inn_interior_verification.md) records focused CPU/extended-save checks,
hidden OpenGL captures and 348.09 MiB city–inn texture overlap within existing limits.
[Capture gallery](screenshots/inn/README.md); interactive acceptance remains user-owned.
The [human-scale correction](inn_interior_design.md#human-scale-correction-2026-11-05)
reduces oversized furnishings against the fixed eye height through offline meshes;
map data and saves remain compatible. The subsequent
[ceiling/wood/UI correction](inn_interior_design.md#ceiling-wood-joints-and-rest-affordance-2026-11-05)
lowers the ceiling with bounded compatibility for existing inn saves, cleans up
duplicate wood joints and removes the free-rest fixture prompt.
[Single-cell seating groups](inn_interior_design.md#single-cell-seating-groups-2026-11-05)
center each table/chair assembly on one cell and free the surrounding cells,
with compatibility for the previous inn layouts.

Environment AO **Accepted and closed (2026-11-05)**:
[design and verification](environment_ao_design.md) adds default-on environment-only
contact shading with `--ambient-occlusion on|off`, excluding creatures and preserving
fog, composition and prepared-image presentation. Three reusable targets add 12.91 MiB;
focused CPU/hidden GPU checks pass. [On/off captures](screenshots/environment-ao/README.md)
are retained. The user accepted the [stronger tuning](screenshots/environment-ao-stronger/README.md)
as the default: .75 strength / .375 darkening cap, .30-cell radius and .01-cell bias.
Focused AO checks pass; visual review is complete.

Sanctuary SC04 is **Accepted and closed (2026-11-05)**: [design](sc04_design.md),
[verification](verification/archive/sc04_verification.md) and [campaign captures](screenshots/sc04/README.md).
Normal campaigns now use the accepted city. New Game starts in the square, recall
arrives at sanctuary, and dungeon ascent returns facing into town. Named content
anchors drive travel, protected landings, recovery and HUD prompts. City discovery
and pose survive Continue; restore checks the canonical immutable city definition.
Schema/simulation are 9/13, without migration. Existing prepared view staging,
resource ceilings and accepted artwork remain. Acceptance includes the entrance
trim correction; milestone S-C and the [Sanctuary city roadmap](sanctuary_city_plan.md)
are complete and closed. The [entrance trim correction](sc04_design.md#entrance-trim-review-correction-2026-11-05)
recesses building-backed staircase terminal panels ahead of raised facade trim.

Sanctuary SC03 is **Accepted (2026-11-05)**: [design](sc03_design.md),
[verification](verification/archive/sc03_verification.md) and [captures](screenshots/sc03/README.md).
Bounded scenery placements and prepared cutout meshes support two trees, bushes,
three planting beds and sparse ivy; two stalls, a fountain and three painted service
signs complete the dressing. The shorter representative tree is accepted. Opaque
water uses eight prepared frames at 8 FPS, invalidating visible color only while
preserving picking/geometry and the existing pause clock. Publication 10 carries
cutout/water part flags, daylight fog and view distance; offline
variant bindings share published mesh IDs.
Review corrections cover stall scale/location, entrance spacing, grounded planting,
roof-contained ivy, soil artwork, fixed volumetric water streams and a
gentle city-only haze capped at 22% (dungeon defaults unchanged); see
the SC03 design addendum. The city pack now uses a shared CPU/GPU 40-cell view distance and modestly
brightened carved fountain stone; other packs retain 16 cells. See the
SC03 far-enclosure addendum for bounded culling storage and verification.
The inn sign review correction moves its bracket onto the solid facade strip
beside the upper window; see the SC03 mounting addendum. Milestone S-B is accepted.
SC04 delivers the campaign cutover below; SC03 artwork remains accepted.

Sanctuary SC02 is implemented and accepted (2026-11-05): [full town design](sc02_design.md)
and [verification/captures](verification/archive/sc02_verification.md). The 24×24 square-centered town
has ten principal buildings, separate dungeon/gate landmarks, two street loops,
and fixed named approaches/dressing reservations. Ten models and 91 placements
reuse SC01 ownership/rendering; campaign cutover remains SC04. User review reports
about 3,500 FPS uncapped; retain existing chunk/frustum culling without advanced
occlusion or additional spatial subdivision. SC03 adds distinct painted signs for the inn, smithy and provisioner, using
prepared artwork and static meshes.

Sanctuary city (SC01.1 implemented and accepted, 2026-11-05):
[four-slice roadmap](sanctuary_city_plan.md) stages an in-engine architectural
street corner, full town layout, vegetation/market/fountain, and campaign hub
replacement. Retained [visual references](mockups/sanctuary-city-v1/README.md)
guide an older, lightly overgrown town with occasional two-storey exterior shells.
Use simple prepared low-poly geometry and basic frustum culling; optimize observed
issues at slice handoffs. Interiors/services are deferred and existing development
saves require no migration. SC01 is split: [SC01.1 location-owned assets](sc011_design.md) is implemented with
[verification and handoff](verification/archive/sc011_verification.md); SC01.2 architecture is implemented
and accepted (2026-11-05); see the [architecture design](sc01_design.md) and
[verification/captures](verification/archive/sc01_verification.md). Supersampling and texture fidelity remain unchanged;
512 MiB–1 GiB is a memory target, not a mandate to reduce quality.
SC01.2 review follow-up corrects panel-divider contamination in the offline material
exporter, with aligned 16-pixel atlas gutters and focused verification; see
[before/after evidence](screenshots/sc01/material-seams/README.md).

Movement-frame performance (implemented and mechanically verified, 2026-11-04):
[skip unavailable picking and reuse visibility work](movement_frame_work.md) removes
moving software rasterization and empty HUD outline scans, caches owner/leaf relations,
and isolates optimized Debug raster kernels. The bounded 1920×1080 profile exceeds
400 FPS in Debug and Release, including movement stops and hover. `--uncapped`
disables VSync/the frame cap; F1 displays FPS. Campaign desktop review remains user-owned.

Ground-anchored door-leaf occlusion (implemented and mechanically verified, 2026-11-04):
[shared leaf relationships and creature compositing](creature_occlusion.md) replaces
leaf-volume strip deletion while preserving gameplay occupancy and artwork size.
The presentation interface separates ground anchors/artwork from future physical footprints.
Extended-plane classification avoids free-tip popping; only finite leaf silhouettes
occlude. Focused CPU/headless and hidden GPU checks pass; [review gallery](screenshots/ground-anchored-doors/README.md)
includes both sides and dense motion samples. Interactive acceptance remains user review.

Floor map camera correction (implemented and mechanically verified, 2026-11-04):
[per-axis centering and bounded tracking](map_revision.md#camera-bounds-correction)
uses stable floor/boundary-wall bounds, excluding unused exterior solid padding.
Fitting axes stay centered; larger axes track the party and stop at map edges.
All 41×28 whole cells are used; [corner captures](screenshots/map-camera/README.md)
and focused Debug/hidden GPU checks verify centering, clamping and allocation behavior.

Floor map revision (implemented and mechanically verified, 2026-11-04):
[viewport-sized floor map and room discovery](map_revision.md) adds fixed-size
27-pixel cells, complete explored-room walls and shared imagegen markers/legend.
Knowledge is derived from existing LOS discovery; save and simulation data remain
unchanged. Focused CPU checks and hidden GPU/application checks pass;
[review captures](screenshots/map-icons-desktop/README.md) include a read-only
copy of the user's save. Interactive acceptance remains user review.

Arrival-facing correction (implemented and verified, 2026-11-04):
[turn settled front-line enemies toward the party at movement completion](combat_positioning.md#arrival-facing-follow-up--2026-11-04)
uses the existing combat presentation handoff, preserving travel-facing and action costs.
Focused checks and hidden arrival captures pass; interactive acceptance remains user review.

Combat positioning correction (implemented and mechanically verified, 2026-11-03):
[waiting enemies receive turns and route into usable front slots](combat_positioning.md)
separates attack geometry from initiative, preserves usable attackers' positions,
and uses shared slot selection with bounded navigation for flanking/blocked actors.
Twenty-one affected suites and a hidden four-goblin encounter pass;
[review captures](screenshots/combat-positioning/README.md) show both flankers
joining the populated front. Interactive acceptance remains user review.

Creature occlusion correction (implemented and mechanically verified, 2026-11-03):
[stable creature dimensions and shared scenery masks](creature_occlusion.md)
replaces wall/door/viewport shrinking and lateral anchor fitting with per-fragment
intersection clipping plus existing depth. CPU picking and GPU drawing share
bounded masks; simulation and save data are unchanged. Eight focused CPU/headless
suites and hidden GPU checks pass; production captures are inspected. Interactive
acceptance remains user review.

Target selection follow-up (implemented and mechanically verified, 2026-11-03):
[geometry-exposed enemy candidates, compact picker and Tab cycling](target_selection_design.md).
Twelve affected CPU/headless suites and hidden production-art captures pass;
[review captures](screenshots/targeting/README.md). Interactive acceptance remains user review.

Music level correction implemented and verified: [restore mastered levels and compensate raylib center attenuation](combat_music.md#music-level-correction-2026-11-03-implemented).

Combat music (implemented and mechanically verified): [persistent playlist and animated track announcements](combat_music.md)
plays the 21 supplied Nick Panek tracks, with independent settings, bounded prepared
streams, five-second/remaining-time fades and local shuffle persistence. Headless
and runtime checks, muted real MP3 integration and hidden UI captures pass; listening
balance and interactive acceptance remain user review.

Audio loudness correction (implemented and verified): [mix gain and peak protection](audio_mix.md)
raises runtime bus levels without rewriting samples or user settings; a final
stereo-linked limiter bounds overlapping peaks before device output.

Organic audio experiment (closed): [listening selections and retirement](audio_model_pass.md)
retains five selected exports, keeps all other synth cues, and replaces encounter
with a short bell. The model, task Docker images, credentials and unused audio
were removed. The active bank has 75 cues / 210 samples; individual replacements
and Csound authoring remain supported.

Audio authoring revision (implemented, revised synth palette accepted):
[local model and distinct synth cues](audio_revision.md) records the listening
feedback, the historical local-model trial, 20 redesigned synth cues and
the level-up bell. The model installation was subsequently retired as above.

Audio delivery (implemented and mechanically verified, 2026-11-02):
[runtime, editable bank and verification](audio_design.md). Includes 75 cues / 218
generated variants, shared main/pause Settings, bounded playback and authoritative
event timing. Debug graphical/headless builds, 13 focused suites and real-device
checks pass. Subjective listening and interactive acceptance remain user review.

Combat-entry/front-slot follow-up (implemented and verified, 2026-11-02):
[settled contact and current-facing approaches](combat_entry_followup.md) addresses
premature destination-based contact, hidden front protectors holding the camera,
stale navigation after turns, and rear-slot arrivals during exploration/turns.
Nineteen affected CPU suites and two hidden production-art captures pass;
[capture notes](screenshots/combat-entry/README.md). Interactive acceptance remains user review.

Door/chest review follow-up (implemented and verified, 2026-11-02):
[door surfaces, sprite clearance and walkable chests](door_chest_followup.md)
records shared door geometry/UV corrections, bounded sprite fitting against door
boxes, and wall-side chest instances with player access to their cells. Existing
campaign definitions and save bytes remain compatible. Nineteen affected suites
pass, with focused viewport-edge follow-up and hidden CPU/GPU capture verification.
[Review gallery](screenshots/door-chest/README.md); user visual acceptance is pending.

Campaign Continue correction (implemented and verified, 2026-11-02): graphical startup must
borrow the already restored CampaignRun session without rebuilding initial NPC
placements against the saved party pose. A party may legitimately occupy a dead
or moved NPC's original spawn. `gameplayPlacements` becomes a preparation-only
optional result: absent when a campaign supplies the live session, otherwise the
existing validated idle placements for a standalone/arena session. Keep saved
definitions, current-actor collision validation and disk bytes unchanged. The
existing [save design](procedural_campaign_save_design.md) applies; no schema or
simulation change. Owned startup regressions cover save/restore at an old spawn,
preserved live state and strict standalone spawn rejection. Verify a read-only
copy of the reported save through application startup/view preparation. Main-menu
errors use a solid inset panel and larger high-contrast text above the bottom
border; other menu layout/input behavior remains. Focused CPU checks and a hidden
application-owned error capture verify the correction, without desktop input.
The owned startup regression and campaign-save smoke check pass; a read-only copy
of the reported save loads/renders successfully, with original bytes unchanged.
[Details and captures](procedural_campaign_save_design.md#graphical-continue-at-an-old-npc-spawn-2026-11-02).

Underkeep prop art **approved (2026-11-01)**: [geometry and metric UVs](underkeep_props_revision.md),
corrected to the dungeon’s 256 texels/cell after user review of the coarse version.
Future authoring: [painted dungeon prop guide](prop_modelling_guide.md).

Current work: [Underkeep dressing](underkeep_dressing_design.md) records the approved
biome art, procedural relief, furniture occupancy and persistent treasure-chest
delivery. Implementation and verification are recorded there; user visual acceptance is pending.

Underkeep review correction (implemented and verified, 2026-11-01):
[scale, floor-anchored moss and offscreen contact](underkeep_review_corrections.md)
records half-size chest instances, narrower/lower door leaves with filled stone
frames, and a shared horizontal camera contract for subcell-aware combat facing.
Current Underkeep saves remain compatible. Fifteen affected CPU suites and hidden
production-art captures pass; interactive acceptance remains user review.

Renderer roadmap **Accepted and closed** (2026-10-31): R01–R06 and milestones
R-A/R-B/R-C are complete. The user accepted R06 with “slice accepted, close up the
new renderer plan”. [Sole renderer cutover and retirement](renderer_cutover_design.md)
documents the delivered ownership, interfaces and deletion inventory;
[verification](verification/archive/r06_verification.md) retains executed checks and limitations unchanged.
[Historical design and acceptance records](system_history.md) preserve prior evidence
without describing current defaults. The bestiary expansion is closed with B01–B05
accepted; useful B06 scope transfers to campaign PC07. Campaign planning resumes
after P-B. [PC07.1 preparation and calibration](pc071_arena_calibration_design.md)
is Accepted and closed (2026-11-01); PC07.2 [population and route checks](pc072_population_design.md) is **Accepted and closed** (2026-11-01), including review corrections. Parent PC07
is closed; PC08 is next for detailed planning only and P-C remains incomplete.

Current revision: [dungeon art, stairwells, reusable doors and responsive loading](dungeon_art_loading_design.md) implements the separately approved follow-up. [Verification and captures](verification/archive/dungeon_art_loading_verification.md) record its delivered checks and limitations. It does not reopen the closed renderer/campaign roadmaps.

Current follow-up: [loot, guardian doors and uninterrupted encounter flow](dungeon_flow_polish.md)
records the implemented visual/interaction polish, unsaved-run terminal fix, and
focused CPU/GPU verification.

Accepted work: [early bestiary roadmap](bestiary_plan.md) — closed after five
accepted monster slices: skeleton, bugbear, standard giant rat, zombie and goblin shaman.
Each monster includes imagegen art in the goblins' style and their applicable
animation set. Implement and review in the arena before campaign integration;
all actors retain one-subcell footprints. [B01 design](b01_skeleton_design.md) is
**Accepted and closed** (2026-10-31): immutable damage profiles, typed impact resolution and full
skeleton art are implemented in the arena. [Verification](verification/archive/b01_verification.md)
records checks, captures and acceptance. B02 bugbear and full-resolution sprite
cropping are **Accepted and closed** (2026-10-31); [B02 verification](verification/archive/b02_verification.md) records
art, memory totals, CPU/GPU checks and arena captures. [B03 giant rat and Pack Tactics](b03_giant_rat_design.md) is **Accepted and closed** (2026-10-31).
[B04 zombie and atomic Undead Fortitude](b04_zombie_design.md) is **Accepted and closed**
(2026-11-01): mechanics, full art, native portrait and normal arena integration are
delivered. Reference v1 and the completed slice are accepted.
[B04 verification/gallery](verification/archive/b04_verification.md) records the checks and limitations.
[B05 goblin shaman and enemy spellcasting](b05_goblin_shaman_design.md) is
**Accepted and closed** (2026-11-01). Tagged NPC Bless
sources, fixed NPC resources, casting timelines, full 56-pose art and portrait are
implemented. Reference v1 and the completed slice are accepted.
[B05 verification/gallery](verification/archive/b05_verification.md) records checks and limitations.
B06 is superseded without implementation; reusable level preparation and targeted
calibration now belong to [campaign PC07](procedural_campaign_plan.md#pc07-depth-bands-and-finite-recovery-encounters).
B01 review adjustment: anchor the variable-width initiative strip to the viewport’s
right inset. The [compact initiative follow-up](native_hud_design.md#compact-initiative-strip--2026-11-06)
halves its footprint and aligns its top to the viewport, retaining native text.
The earlier adjustment uniformly reduced skeleton appearance dimensions by 12.5% while
retaining source art, pixel canvases and ground anchors. This is presentation
metadata/layout only, using the existing prepared-image pipeline. Health bars use the
projected body-axis anchor horizontally rather than the visible silhouette’s
bounds, so weapons and asymmetric poses do not pull them away from the head.
The three-floor campaign is the opening low-level vertical slice of an eventual
roughly 20+ floor game; planning resumes after accepted P-B.

Current campaign work: [PC07.1 arena preparation and calibration](pc071_arena_calibration_design.md)
implements the accepted first half of PC07. Its measurements precede population
selection in PC07.2. The target is level 2 before the final guardian under normal
shared XP. The bounded 36-trial study completed: all victories, three downings, zero permanent
deaths. [Costs and recommendations](pc071_calibration/README.md) and
[verification](verification/archive/pc071_verification.md) precede campaign population and route checks. General expedition agents remain PC08.

PC07.2 [variety and controls](pc072_variety_controls.md) are **Accepted and closed** (2026-11-01)
under the approved review plan: generated room graphs, one starting kit,
key-use logs, release-activated buttons and automatic rogue action completion.

PC07.2 [review corrections](pc072_review_corrections.md) are **Accepted and closed** (2026-11-01)
under the user-approved plan: sector locks, reduced guaranteed potions, automatic
End Turn Cunning Defend (superseded by automatic action completion above), responsive
saving feedback and alpha-aware presentation.
The original route study remains historical evidence for the previous budgets.

PC07.2 [population and generated-route checks](pc072_population_design.md) are
**Accepted and closed** (2026-11-01). Fixed five-recipe depth bands retain bounded
encounter/member records, an intended key/guardian route and finite caches.
Closed populated-room doors use existing shared geometry/AI. One population
resolver supplies launch and checked save definitions; diagnostic generation
reports include route/recipe metadata while player reports retain identity only.
Direct-route XP totals 1,200 before the final guardian; ordinary fights remain
bypassable. Existing Party, reward, travel and allocation ownership are unchanged.
The original generator-5 [eight persistent routes](pc072_routes/README.md) all won: 32 observed downings,
zero permanent deaths/recorded merges, level two before the final guardian and
three field rests each. Both selected replays matched with hidden captures.
[Verification and limitations](verification/archive/pc072_verification.md) retain frequent downings,
costly reserve detours, diagnostic policy limits and historical balance limitations.

Pause diagnostics follow-up (2026-11-01): Presenter owns a default-off F1 toggle
for raylib's live FPS counter, drawn over the final window composite in gameplay,
menus and cached/loading frames. It does not invalidate world images or enter saved
state/captures. Active-run pause opening and idle pause frames perform no campaign file I/O:
CampaignSaveFlow inspects the slot only on an explicit Save/Save and Main Menu
intent, retaining that revision across overwrite confirmation. Actual save/load
validation remains unchanged. The existing save design applies; focused owned
file-count regression checks cover idle pause frames and stale overwrite rejection.
Release application and the user's active Debug application both built with one
low-priority worker. `ctest --preset release -R '^campaign_save_smoke$'` passed
1/1 in 0.93 s, including 120 paused frames with zero reads/writes and concurrent
overwrite rejection. No GPU workload or desktop input automation ran while the
user was playing; interactive FPS/latency confirmation remains user review.

[Character-library compatibility follow-up](library_compatibility_fix.md) (2026-11-01, implemented): disk loading
admits each valid current character independently and skips incompatible records.
An unreadable library envelope becomes an empty usable library after a bounded
successful file read; real I/O failures remain reported. Strict decoding stays
available for validation. Loading never rewrites ignored data; the next explicit
character save writes the accepted records plus the new draft, backing up the old
bytes. CharacterLibrary retains the exact bytes observed at load and compares them
under the existing writer lock before committing, so filtering does not weaken
concurrent-writer protection. Storage and parsing remain preparation/I/O work,
outside steady frames. Lock files alone are not recovery data. Campaign inspection
already disables Continue on incompatible/corrupt saves while leaving New Game
available; preserve strict save validation and explicit replacement confirmation.
Owned regressions cover mixed/all-invalid characters, malformed/versioned envelopes,
creation/reload after filtering, stale writes and incompatible campaign replacement.

Inventory tab feedback follow-up (2026-11-01): page labels and party tabs paint
their own held state behind their text/portraits, using a subtle fill and teal
underline. Preserve the selected page/character treatment. The final generic button
feedback pass skips these rectangles, avoiding the inset button border and text
dimming on tabs. Capture/release/cancel semantics and storage remain unchanged;
verify through hidden application-owned captures, without OS input automation.

Implemented follow-up: [variable stone layouts](procedural_layout_design.md) replaces the lattice with explicit chamber membership, boundary sockets and passage paths. The preparation-only stone generator selects compact wings or chamber networks through biome/floor specifications, using variable rooms, bounded routing, checked loops and two frozen fallbacks. Fixtures, encounters and persistence consume actual geometry. Campaign population/resources and presentation remain unchanged. That geometry delivery used generator/catalog 8/6 and campaign schema/simulation 6/10; Underkeep now extends it at 9/7 and 7/11 while preserving its terrain/population streams. [Review maps and captures](screenshots/variable-layouts/README.md) accompany 12 passing integration checks and hidden production GPU verification; user review is pending.

## Boundaries and data flow

- **World:** each immutable WorldDefinition selects a prepared asset pack by stable ID
  (default `dungeon`); compact row-major solid-cell storage and immutable NPC
  loading/placement data; grid directions and coordinates are value types. Furniture
  movement occupancy is cached separately from terrain/sight solidity. Loading and connectivity validation happen before
  play. A map is limited to 1024×1024 cells and rejects malformed input explicitly.
- **Session/combat:** an immovable owner borrows map/catalog and owns actors,
  movement, AI, combat RNG and elapsed time. Standalone sessions own Party;
  campaign sessions borrow the single Party owned by CampaignRun. Actors and encounter membership
  are bounded at 256; initiative groups use catalog monster identities. Contact
  clears queued movement; commands validate turn tokens, life, equipment, range,
  formation and attacker availability before RNG. Acceptance returns synchronously;
  a fixed pending record retains the saved result until strike/arrival. Each update
  returns a borrowed timestamped batch (8,192 entries plus one automatic step),
  draining effect deadlines and existing attacks before movement/AI and then taking
  at most one new automatic step. Consume the batch before any subsequent session
  simulation operation, not only the next update.
  Acting turns, facing and results wait for pending impacts and presentation tails.
  Focus/map/details/history/menu pauses freeze session deadlines and gameplay time.
  Gameplay cooldowns retain the hybrid real-time/round-boundary policy.
- **Effects:** session-owned 64-instance fixed storage, up to three typed recipients
  per instance; world/serial IDs reject stale slot reuse. Tagged party/NPC sources own at most
  one concentration each; NPC sources currently support Bless only. Pending Bless
  reserves an existing effect slot before costs/RNG and commits at clip release. Checked apply/remove operations and save/lifecycle events
  serve checked spell commands and resolved NPC cast timelines. Full storage rejects
  before costs/RNG/replacement. Expiry precedes repeat saves and equal-time impacts;
  existing accepted attacks keep their saved rolls. Incapacitation masks suppress
  NPC facing as well as pathfinding; living held actors retain occupancy.
- **Party/rules:** immutable catalog values feed pure, allocation-free character
  derivation. A four-character party owns starting builds/gear, derived caches,
  resources, health/death-save state, and a formation permutation. Pure health
  transitions feed checked mutations; health/build snapshots remain const. Typed character/item indices never
  recycle; formation swaps preserve identities and resources. JSON/loading stays
  outside game code. Schema 2 adds party content; strict schema 1 remains readable
  as rules-only content, but startup requires a preset.
- **Actors:** a fixed-capacity runtime owner initializes monster HP and transforms
  from placements and the catalog. Checked movement, facing, attack cues, damage and removal
  atomically update records, occupancy and revisions. Typed IDs include a unique
  world identity; slots never recycle. Dead records remain readable, removed IDs
  fail lookup, and only living actors block movement.
- **AI/navigation:** fixed state for at most 256 actors; immutable placements supply
  guard/patrol/wander settings. Separate seeded randomness preserves combat rolls.
  One exploration tick per 100 ms uses a shared map-sized BFS workspace (six bytes
  per cell), at most 4096 search/cleanup visits and eight completed requests.
  Requests wait on budget exhaustion; checked ActorWorld steps revalidate occupancy.
  AI time freezes outside exploration; visual walk cues use paused session time.
- **Movement:** owns player pose, one pending command, transition state, and a
  revision counter. Exposes a const state snapshot; consumers cannot mutate its
  invariants. Key repetition is a separate graphics-independent input component.
- **Assets and preparation:** the offline world baker extracts registered source
  frames, prepares reusable material/sprite mip chains and CPU alpha masks, and
  publishes bounded versioned world metadata/textures, including up to eight pack
  manifests with local material palettes, door/edge/decal/sky bindings, bounded
  prepared prop meshes, an optional reusable guardian-door emblem, and world UV scale.
  Texture purpose is independent of sprite alpha/sampling; panoramas do not occupy
  terrain palette slots. Publication identity hashes canonical metadata including the
  checked payload checksum, so binding-only edits invalidate replay too. Runtime validates bounded catalog metadata and definitions, then verifies
  selected payload checksums when acquired. CPU masks/models and GPU textures are
  leased by active/staged location requirements; unused packs/types stay on disk.
  The offline baker verifies every payload. Native portrait/creator atlases have independent retained bakers.
- **World rendering:** bounded static chunk/material meshes plus per-view CPU
  visibility and effects consume the same camera, actor animation, loot, doors and
  surface snapshots as GPU drawing. Prepared one-cell stairwells contain eight steps,
  lined shafts, exterior shells and dark terminations; their full bounds and fixture
  IDs use the same triangles on CPU/GPU. Stair cells block movement; F uses the
  designated adjacent landing. Frames use wall-density world UVs, leaves fitted
  rectangular artwork and separate edge sampling. Guardian approach/gate leaves carry
  a transparent sigil on both faces through their current hinge transform. Loot uses
  smaller billboards and proportional floor shadows; shadows do not affect picking.
  Generated caches use persistent painted chest meshes, with prepared closed/open
  variants and checked container identities; creature/party drops remain bags.
  Picking has full per-pixel depth/IDs, with
  transparent holes, corpse occlusion and native status/health clipping. World
  effects never write picking IDs. A changed view invalidates rendering; idle views
  reuse their target without frame uploads. See the accepted R02–R05 designs.
- **Platform:** raylib window/input adaptation and GPU ownership. Typed texture
  and render-target handles refer to fixed-capacity, generation-checked slots.
  Borrowed raw raylib values stay inside short platform calls. RAII unloads GPU
  resources before the window closes, including exception paths.
- **Commands/UI:** a graphics-independent controller validates current body
  visibility and routes production requests through GameSession; it separately owns targeting, read-only hover, inspection, exclusive overlays, bounded enemy/ally
  targets, a 32×192-byte log, eight timed damage/MISS splashes and a fixed HUD snapshot. Confirmation must refer to
  the turn/outcome already presented. The bottom viewport strip appears only for
  contextual interaction/rest prompts or terminal guidance; floor/depth information
  belongs in the top bar. Idle exploration has no bottom location label or fill.
  Focus/edge handling, Aid selection and
  aspect-fit hit testing are CPU-testable.
- **Presentation:** Presenter uses the shared application registry for its viewport
  two native 1920×1080
  targets: a point-filtered dungeon base and a transparent premultiplied UI layer
  fitted with bilinear filtering. The 500×282 GPU dungeon retains the accepted framing and sampling policy.
  Layout and input retain logical 640×360 coordinates; HudPainter converts geometry
  to native pixels, and fonts draw at their native atlas sizes. Artwork, geometry
  and text share one ordered UI pass; modal fills cover all underlying UI content.
  Six startup-loaded RAII font atlases provide Plex Sans regular 21/24 px, bold
  27 px, Inconsolata 24 px and DejaVu Serif Bold 27 px and Plex Sans Bold 48 px controls. Font ownership is separate
  from the generic texture registry and ends before window shutdown.
  Startup-loaded native art atlases retain prepared portrait/icon derivatives at
  display sizes with padded edges and bilinear sampling. Portraits center-crop and
  icons aspect-fit. Captures use one temporary diagnostic target. See [native HUD design](native_hud_design.md).
- **Application/tools:** normal launches enter menus, campaign or arena through one
  renderer. GameView borrows WorldRenderer by reference and leases active/staged
  views. Preparation succeeds before session travel commits. CLI/client/local MCP
  visual observations use the same CPU depth/ID pipeline without a graphics context.
  Replay capture owns a hidden context and exports application-owned targets.
  Standalone demos, preview/showcase scenarios and focused capture runners are gone.
  One application-owned CPU worker runs loading I/O, parsing, campaign preparation,
  alpha preparation and scene construction while the window thread presents cached
  targets and pumps events. Explicit preparation scopes publish phase text; GPU/font
  work and one-at-a-time raylib image decoding stay on the window thread with a 4ms
  soft checkpoint budget. Individual driver/decoder operations remain indivisible.
  Stairs/recall/waypoints fade the entire presentation out over 250ms, publish black,
  prepare and commit, then fade in over 250ms. Failure returns to the source view;
  simulation clocks freeze, costs commit once and held input must release. A sole
  hub destination emits travel directly; multiple entries and paid recall retain
  their selectors. Ordinary local victories resume exploration automatically after
  recording settled combat events; terminal campaign/arena controls remain.

Dependencies flow from application/presentation toward platform, CPU rendering,
movement, and world values. World/movement/input timing have no raylib dependency.
There is no ECS, generic event bus, virtual renderer hierarchy, or service locator.

## Ownership, storage, and limits

FloorGeometry retains fixed current/remembered open
bits separately from first-opening completion. Ordinary doors toggle from adjacent
cells, reject occupied or moving-through thresholds, preserve acquired keys and share
a 300ms motion barrier. Guardian gates remain automatic. Discovery, navigation and
picking invalidate on both opening and closing; settled state survives save/travel.

Maps own contiguous cell bytes. CampaignRun retains five stable locations and the
travelling Party; the application owns shared prepared rendering. One active
session borrows its location/Party, with value snapshots for inactive locations.
Travel stages replacement simulation/view owners at an explicit preparation
boundary and commits before the next simulation tick. Only active/staged owners
have navigation/frame scratch; there is no live streaming or inactive simulation.
Standalone authored/arena sessions retain their existing map/catalog lifetime
contract. See the PC04 design for snapshot validation and clock ownership.

Runtime actors retain up to 256 stable records and one byte of live occupancy
per map cell. The immutable placement mask is loading data only. The map and
catalog outlive the noncopyable/nonmovable actor owner. Runtime operations return
fixed result values without allocation. Lethal impact unblocks immediately; animation
plays a 1200 ms goblin death/corpse clip, overlaying white for its first 100 ms,
then hides body/shadow. Legacy missing death art retains the frozen 100 ms flash.

WorldRenderer retains immutable catalog metadata and at most active and staged
views. View-owned leases share current texture/alpha/model payloads and release
source-only assets on commit, destination-only assets on cancel. Each view owns bounded static scene geometry, visibility scratch,
effects and GPU mesh/target leases. Resident world safety ceilings are 512 MiB mip textures
and 64 MiB alpha masks; scene limits are 64 MiB CPU/40 MiB GPU, view scratch 8 MiB,
and tracked world ceilings are 352 MiB CPU / 608 MiB GPU. Preparation rejects capacity or
allocation failures without replacing the playable view. These are preparation
boundaries, not frame-time growth policies.

Static architecture uses up to 128 world-owned placements and a separate pack-local
catalog of up to 32 named multipart models. Validated level solid footprints retain
movement/sight blocking while a derived ownership mask suppresses generated backing
walls. Prepared painted geometry lives in the anchor chunk; its bounds include roofs
and overhangs. Building scenes search 32 cells/81 candidate chunks for diagonal
far-frustum coverage; ordinary scenes retain 24/49. Fixed draw storage covers both.
Frames borrow prepared geometry and resolve no model names. Furniture's four slots
and tighter mesh profile remain intact. The standalone full-town city factory owns bounded named doorway/anchor and
future dressing metadata, validating connected routes with future bases blocked.
SC04 makes this factory the production campaign Sanctuary; review tooling shares it. SC03 adds up to 32 separate
scenery placements: open-terrain movement/NPC bases without solid-cell LOS or
generated-wall replacement. Named mesh cutouts share prepared alpha/mips and
CPU/GPU alpha rejection; view-owned geometry includes canopy bounds. No new
visibility subdivision or frame-time resource work. SC02 uses two shared
4096² atlases with existing multipart publication; no renderer or schema change. See [SC01.2](sc01_design.md) for
ownership, capacities, exports and focused verification.

Party stores four character identities/resources and owns one fixed 208-stack
inventory: four 48-cell bags plus four equipment positions per character. Startup
content retains up to 16 owned entries; live queries use inventory handles and
52-position sparse loadouts, never startup indices. Item handles carry party
inventory identity, pool index and generation. Moves preserve identity; complete
merges/consumption invalidate it, and exhausted generations retire slots.
Transactions stage fixed value storage and all four derived-stat records, committing
only after capacity, training, hand compatibility and weight validation. Weight is
integer hundredths of pounds with int64 totals. Catalog bounds remain 16 races,
16 classes and 64 items; all preparation bounds reject explicitly.

Party construction may copy strings; frame callers borrow const snapshots. The
catalog outlives sessions/parties. Campaign travel retains actual Party/inventory
identity and rebases portable deadlines without recreating starting builds. Arena
restart reconstructs borrowers while reusing prepared assets. These preparation
boundaries may allocate; ordinary engine-owned frames may not. Character IDs are
party-local; startup owned-item indices are character-local; catalog IDs remain
catalog-local. See [inventory design](inventory_design.md) for schema 5, live item
commands, safety, UI storage and tests.

The application-owned GPU registry holds 512 standalone textures, twelve
render targets (expanded for environment AO), eight static meshes and eight shaders. Presenter borrows the registry;
world scene/resource leases release resources at explicit preparation boundaries.
The new path additionally enforces texture/mesh/scene byte budgets in the
[R02 design](renderer_world_design.md). Normal gameplay always uses the world renderer. Supersampling defaults on;
`--supersampling off` selects native filtered quality. Graphical initialization
failure is an error, never a renderer fallback.
Handles include a typed slot index and generation. Invalid/stale lookup returns
no resource; checked platform operations reject it. Exhaustion is an explicit
startup error. Releasing a slot invalidates its generation; exhausted generation
counters retire that slot rather than allowing old handles to become valid again.
Registry identity prevents handles from accidentally crossing registries. Slots
contain resource values, not owning raw pointers. No extra handles are introduced
for ordinary grid coordinates or transient references.

The allocation guarantee covers engine-owned CPU gameplay processing. The test
intercepts ordinary and aligned C++ `new`/`new[]`; it does not intercept C
`malloc` calls. Engine-owned frame code uses no C allocation APIs. Loading uses one queued CPU task and one consumed result, not an unbounded queue;
UI/font payloads reject above 64 MiB, prepared textures retain their existing 128 MiB
per-read ceiling and validated dimensions. Loading,
cache preparation, exceptional error reporting, captures, and driver/raylib
internals are outside it. A targeted allocation guard is more useful here than
a Vulkan-style benchmark or stress-testing framework.

## Typed damage and bestiary content

B05 implements the approved [goblin shaman design](b05_goblin_shaman_design.md).
Catalog schema 13 adds a concrete Acolyte support profile. Combat owns fixed
per-actor mana/target cursors and reuses existing pending-action slots for Bless
and Sacred Flame; ranged-capable admission and AI holds preserve support positions.
Effect requests/instances/events carry tagged party or world-checked actor sources.
NPC Bless reservations share the 64 slots; damage and burning Web notify source
concentration exactly once. NPC sources/resources remain floor-owned across travel,
canonical restore and disk persistence (introduced in schema/simulation 4/7;
current identities are 6/10). Additive protocol
source tags, hover/log feedback, full production art and portrait reuse existing
owners. Seven appearances /375 textures total 314.56 MiB mip and 56.67 MiB alpha;
all budgets/count capacities remain fixed. [Verification](verification/archive/b05_verification.md)
records evidence, limitations and completed-slice user acceptance.

B04 implements the approved [zombie design](b04_zombie_design.md). Catalog schema 12
adds Undead Fortitude to the typed byte trait mask. One allocation-free NPC damage
helper serves combat impacts and burning Web, applying mitigation and an effect-
modified lethal Constitution save before the ActorWorld HP/death commit. Success
retains one HP, identity and occupancy with ordinary hurt, never a death marker or
premature reward. Radiant/critical bypass; full adjusted damage sets the 64-bit DC.
Existing impact order, accepted rolls and spell-hit granularity remain. Resolved
event metadata, bounded log/one-HP feedback, production 48-pose art and native
portrait use existing owners. Six-appearance publication is 260.95 MiB mip /
46.60 MiB alpha within unchanged limits; [verification](verification/archive/b04_verification.md) records
focused CPU/GPU/replay evidence, limitations and completed-slice user acceptance.

B03 mechanics follow the approved [giant rat design](b03_giant_rat_design.md).
Schema 11 extends the typed trait mask with Pack Tactics. At accepted NPC weapon
attacks combat scans at most 256 world-checked actors for another conscious ally
in legal melee contact with the shared party cell. Prone/immobile is not inherently
incapacitated. Existing advantage cancellation and saved delayed rolls remain
unchanged. Resolved attacks carry compact pack/mode context for bounded HUD logs
and additive automation fields; no frame allocation, support cache or capacity
increase. Rat production art (48 poses), native portrait and arena integration are delivered,
with accepted reference v3. [Verification](verification/archive/b03_verification.md) records CPU/GPU
checks, publication totals, strict arena captures, limitations and completed-slice acceptance.

B02 implementation follows the approved [bugbear and sprite memory design](b02_bugbear_design.md).
It implements full-resolution NPC crop rectangles and 512/608 MiB world
texture/GPU ceilings while preserving CPU/alpha and count limits. Bugbear uses
ordinary melee and one-subcell occupancy; Brute is included in immutable damage
dice. Crop rectangles reduce stored texels without changing logical anchors,
attachments, shadows or picking placement. See the design for verification and
user review status.

Rules catalog schema 10 adds immutable uint16 immunity/vulnerability masks to each
monster; schemas 1–9 default to normal. Loading rejects unknown types, duplicates
and overlap. A pure allocation-free resolver adjusts the final rolled total once
at impact, after critical aggregation/save reduction. Outcome records carry raw
and resolved damage, concrete type and adjustment. Weapons, spells, Spiritual
Weapon, Shatter and burning Web share this rule; parties retain normal profiles.
Zero damage causes no hurt, interruption, wake or concentration check. Pending
rolls remain private until resolution. The [B01 design](b01_skeleton_design.md)
documents save/RNG, hit-rider and persistence details.

Skeleton is a melee-only, single-subcell arena entry with poison immunity and
bludgeoning vulnerability. Prepared image masters, registration and mip/alpha
publication follow the existing renderer pipeline. Body width stays <=.5 cells;
all clip canvases may use the existing <=1-cell world publication bound without
changing occupancy. A fixed HUD species discriminator selects its generated
portrait in the existing UI atlas. Profiles and portraits add no frame allocations
or resource owner. Historical B01 totals remain in its [handoff](verification/archive/b01_verification.md);
current cropped publication totals are in [B02 verification](verification/archive/b02_verification.md).

## Interfaces and preservation

GameView/WorldRenderer stage the destination geometry, visibility, effects and first
image before committing travel/Continue. The old view dies before its session/map;
failed preparation leaves the current session, image/IDs, Party and save intact.
Presenter accepts only world target/revision publications. Engine-owned frame work
uses prepared fixed storage; export and diagnostic readbacks are explicit boundaries.

Campaign schema/simulation are 10/14 and generator/templates 9/7. Older saves reject
without being modified; no migration. World publication is format 13, appearance 5,
visibility world-visibility-v18, world fingerprint revision 4 and terrain-rules-v2. Door scheduling identity is
shared-door-motion-300ms-v2. Strict trace replay
checks identities rather than translating retired renderer recordings. Protocol v1
remains; renderer/visibility selectors and renderer_fixture scenario are removed.

Prefer an installed compatible raylib 6.2 package, otherwise use the current
hash-verified FetchContent fallback. nlohmann/json 3.12.0 likewise prefers the
installed package with a hash-pinned download fallback; only asset I/O uses JSON.
Debug/Release presets stay CLion-compatible.

## Validation strategy

Follow [verification workflow and resource limits](verification_workflow.md).
Use generated, owned small fixtures and focused one-configuration checks during
iteration. CPU simulation, world geometry/visibility/effects, command/HUD, allocation,
transport/replay and save smoke checks run without a window. GPU integration covers
actual resource ownership, full-pixel CPU/GPU parity, hidden world/native UI drawing,
staged rollback and cleanup separately. No desktop input automation. Full persistence
and capacity/performance investigations are explicit; no repeated routine matrices.
See [R06 verification](verification/archive/r06_verification.md) for executed checks and limitations.

## Source map and design index

| Location | Responsibility and design |
| --- | --- |
| `src/world/`, `src/generation/` | Immutable quantized maps/surfaces, fixtures, bounded procedural generation; [environment](renderer_environment_design.md), [generation](procedural_layout_design.md), [historical PC01](procedural_generation_design.md) |
| `src/game/`, `src/rules/` | Session, Party, actors, combat/effects, movement, navigation; [actors](actors_design.md), [combat](melee_combat_design.md), [effects](effects_design.md), [inventory](inventory_design.md); accepted [bestiary roadmap](bestiary_plan.md) |
| `src/render/world_*`, `src/assets/world_assets.*` | Geometry, camera, effects, depth/IDs and reusable publication; [world](renderer_world_design.md), [visibility](renderer_visibility_design.md), [integration](renderer_integration_design.md) |
| `src/platform/`, `src/ui/` | GPU resources, sole renderer, view leases, input and native presentation; [native HUD](native_hud_design.md), [cutover](renderer_cutover_design.md) |
| `src/app/` | Startup/menu/creator/arena/game orchestration and session transitions; [menu](menu_launch_design.md), [arena](arena_design.md) |
| `src/persistence/` | Checked content/character/scenario/campaign disk adapters; [campaign save](procedural_campaign_save_design.md) |
| `src/automation/`, `tools/` | Typed sessions, strict replay, CLI/client/local MCP, owned captures and opt-in experiments; [protocol](headless_protocol.md), [MCP](mcp_design.md) |
| `tests/` | Self-owned CPU/GPU regression fixtures and resource/allocation guards; no retired renderer harnesses |

The bestiary expansion is closed with B01–B05 accepted and B06 superseded into
[campaign PC07](procedural_campaign_plan.md#pc07-depth-bands-and-finite-recovery-encounters).
PC07.1 preparation and calibration are Accepted and closed;
[verification/handoff](verification/archive/pc071_verification.md) records the bounded study and limits.
PC07.2 population, route checks and review corrections are Accepted and closed
(2026-11-01), completing parent PC07. PC08 is next for separate detailed planning;
P-C remains incomplete. See the [current handoff](procedural_campaign_plan.md#pc072-accepted-closeout-and-next-agent-handoff-2026-11-01-current).
