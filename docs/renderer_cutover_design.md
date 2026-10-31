# R06 — Sole renderer cutover and retirement

Status: **Accepted and closed** (2026-10-31).
Parent: [renderer roadmap](renderer_plan.md). The user approved the reviewed plan
with “Implement the plan.” The user accepted R06 with “slice accepted, close up the
new renderer plan”. Milestone R-C and the complete renderer roadmap are accepted
and closed. Verification results and limitations remain unchanged.

## Ownership and interfaces

WorldRenderer is the only graphical world renderer. GameView borrows it by
reference and retains active/staged view ownership: prepare geometry, visibility,
effects and the first image before committing travel/Continue. Failure preserves
the current session, Party, image, IDs and save. GPU leases die before the window.
Presenter consumes a world target plus revision; CPU color frames and uploads go.
Native 1920×1080 UI, 500×282 world presentation, discrete movement and accepted
supersampling-on/native-filtered-off policy remain unchanged.

Windowless visual automation always uses world CPU depth/IDs, including graphics-
disabled builds. Capture alone creates a hidden GPU context. Remove renderer and
visibility selectors from app/CLI/client/MCP. Retain backend/version metadata as
identity, not selection. Strict replay rejects incompatible old recordings; saves
stay schema/simulation 2/2 and world assets format 1/appearance 4 unless a concrete
behavior/serialization change requires otherwise. No fallback implementation.

## Removal inventory

Delete old scene/material color composition, perspective caches, prepared NPC and
loot raster banks, CPU projectile/spectral painting, Frame/FramePipeline and image
transitions; remove their exclusive projection/lighting/loading interfaces.
Delete prepared-bank serialization/publication, profile-C derivatives, preparation
profile, runtime-bank and loot bakers and associated CMake/copy/package rules.
Keep source artwork, shared NPC metadata/sheet extraction and image sampling,
world texture/mip/alpha preparation, UI/creator/portrait atlases and their tools.
Separate shared preparation budgets from old-renderer ownership.

Delete standalone application demos, previews, showcases, focused capture runners
(including menu/creator/arena/save captures), options and dispatch branches. Remove
unused authored-demo map/recovery content and its recovery JSON loader. Remove
HudDemo/fabricated presentation and const-owner preview-only command-controller
mode; real GameSession preview remains. Delete all R01 probe code/tests/exporters
and executable legacy comparisons. Preserve only neutral allocation/failure test
support consumed by current tests. Keep regular game, arena and automation.
Remove public renderer_fixture scenario and move its height map/fixtures to test
support. Keep sanctuary production data. A future cheat/debug interface is outside
R06. Retain useful regular-engine automation scripts without backend comparisons.

Delete old-renderer-specific tests/helpers/goldens; first preserve useful shared
animation, picking, command, HUD, lifetime and allocation invariants using generated
world fixtures and real GameSession. Mixed NPC source-sheet/metadata and UI tests
must survive independently of raster banks. No obsolete executable reference copy.

Idempotent build cleanup is limited to managed binary asset roots, including
single/multi-config layouts: scene.bin, npcs.bin, loot.bin and their temporary
files, root legacy metadata, derivatives and copied preparation.json. Preserve
prepared/world and native atlases. Clean obsolete managed executables/libraries
once during cutover. Never sweep external asset directories or unrelated files.
Arena experiment provenance changes to world-publication fingerprints.

## Storage and performance

Keep existing world resource budgets, active+staged accounting, fixed capacities,
and failure behavior. Preparation/export may allocate; steady engine-owned frame
processing must not allocate C++ storage, decode sources or upload resources.
Picking uses the sole full-pixel world depth contract; remove legacy reduced-depth
fallbacks. No new renderer hierarchy, debug framework or streaming system.

## Verification and delivery

Use owned small fixtures. Port alpha/depth picking, corpses, loot/fixture identity,
ground picking, status clipping, animation/reactions, interaction suspension and
effect lifetime/overflow coverage before deleting old harnesses. Exercise real
session/commands/HUD with world visibility/effects in the CPU allocation guard.
Verify default windowless visual mode, replay rejection, publication failures,
CLI/client/MCP and arena experiment provenance; save/Continue/travel/recall,
arena rematch/restart, menu return and staged preparation failure/cleanup.

Run focused Release checks during development, ordinary graphics and headless
Release suites once at integration, serial hidden world/environment/UI GPU checks
and generated clean/incremental single/multi-config publication/cleanup fixtures.
Follow [verification policy](verification_workflow.md): two build workers total,
one intensive job, low CPU/I/O priority, check disk/memory first. No routine
benchmark/stress or repeated configuration matrix. No desktop input automation.

Retain a compact regular automation/replay gallery and motion sample for indoor/
outdoor, doors, creatures, effects and HUD; height captures may use test-owned GPU
fixtures. Record actual commands, results, versions, removal audit and limitations
in r06_verification.md. Update current build/runtime/system/protocol/MCP docs;
historical designs/captures remain clearly historical, without runnable dependencies.
The inventory and user acceptance close R06/R-C. Additional monsters/debug tools
are separate work; PC07 stays paused.

## Implemented result

GameView requires WorldRenderer by reference; CommandController requires a real
GameSession reference. Presenter no longer owns a CPU viewport/upload path; picking
always owns full depth. The shared blobber_application library keeps main small and
lets hidden integration checks call the production menu/arena UI, without restoring
public demo/capture modes. Optional test exports use real artwork only for review;
ordinary fixtures remain self-owned. The sole offline world baker retains mandatory
prone-art validation formerly supplied by the retired baker. CommandReason value
zero is retired while the other persisted numeric values remain unchanged.

All 124 deleted files and retained shared consumers are accounted for in the
[removal inventory](r06_removal_inventory.md). [Verification](r06_verification.md)
records actual runs, clean builds, affected final checks and sandbox limitations.
The user accepted R06 and milestone R-C; the renderer roadmap is complete.
Acceptance adds no new verification or coverage. No future cheat/debug interface
was added; additional monsters require their own planning request.
