# SC01.1 — Location-owned prepared assets

Status: **Implemented; Accepted by the user** (2026-11-05).
[Verification and handoff](sc011_verification.md); acceptance recorded 2026-11-05.
User approved the SC01 split and implementation. SC01.2 architecture remains
eligible for separate detailed planning. No campaign hub replacement here.

## Visual and memory contract

Preserve supersampling ON by default, the 1000×564 supersampled pass resolved to
500×282, native 1920×1080 presentation, camera, pixel size, source texture detail
and sampling. The user clarified that 512 MiB–1 GiB is a hardware design target,
not a reason to lower fidelity or reject otherwise valid content. 384 MiB steady
and 448 MiB transition totals are diagnostic reference lines only. Report excess;
never silently disable supersampling, downsample artwork or omit geometry.
Existing bounded resource counts, scene/CPU limits and allocation failure rollback
remain safety contracts. Any future change to those ceilings is explicit.

## Responsibilities, data and lifetime

Publication format 5 separates bounded catalog metadata from indexed checked
texture, alpha and mesh payloads. Catalog limits: 4096 textures, 64 packs, 512
meshes, 8 MiB metadata, 8 GiB installed payload. Per-texture dimensions, furniture
geometry checks and stable actor appearance indices retain their existing meaning.
Opening validates metadata/offsets/file size without reading the entire payload;
acquisition verifies each accessed payload. The offline writer verifies the full
publication. Mesh vertices are encoded as little-endian IEEE float32 triangles.
Canonical metadata and payload checksums determine strict publication identity.
Campaign schema and simulation do not change for residency alone.

CpuAssets is a catalog and preparation-only resident CPU store. An
AssetRequirements value contains bounded texture/model ID sets. Requirements
include the location pack, all clips/directions of types in its actor records
(including corpse records), and common interaction/effect sprites. Fixed catalog
indices are never compacted into actor indices. A move-only RAII lease acquires
CPU masks/meshes, shares them by ID, and releases unused payloads. Map/scene
preparation and windowless visibility borrow the same store and explicit leases.
No entity system, general streaming service or unused-biome cache is introduced.

WorldRenderer owns catalog-indexed weak GPU entries. Each active/staged view owns
strong texture leases and an immutable indexed handle table for frame reads.
Acquisition reuses shared textures; commit destroys source-only leases, cancel
releases destination-only leases. CPU requirements are acquired before geometry
or visibility preparation. Source remains intact on every failure. Normal frame
work does not read files, load payloads, grow containers or acquire resources.
Loading CPU work runs on the existing single worker; GPU operations stay on the
window thread while simulation is paused. Lifetimes require the asset store and
GPU registry to outlive their borrowing views/leases.

The baker reuses canonical textures for identical source paths and preparation
settings, including cross-pack bindings. Registered names for common sprites and
NPC animation remain stable. No GPU memory depends on the number of unused packs.
Native UI retains its existing owner; fonts outside the GPU registry contribute
to application memory reports. GPU reports distinguish installed payload totals,
resident world payloads, registry/UI/font bytes and preparation high-water marks.
Explicit off-mode construction omits high-resolution targets. Once high targets
are prepared, retain that bounded scratch for allocation-free quality switches;
the normal on-mode is unchanged.

## Failure and capacity behavior

Validate the deduplicated active/destination union before uploading: resident
world texture count remains 480, GPU registry 512, alpha masks 64 MiB, tracked
world CPU 352 MiB, existing world texture/GPU safety ceilings 512/608 MiB.
The new lower application reference lines are advisory, not rejection gates.
Staged resource acquisition owns all partial work and rolls back on invalid IDs,
corrupt data, safety-capacity exhaustion, allocation failure or failed first draw.
Prepared source view, Party and save bytes are unchanged. Inactive locations
retain simulation state only. Future runtime additions of asset types must use a
preparation boundary rather than lazy frame-time I/O.

## Verification and delivery

Small owned publications exercise disjoint packs/types, shared bindings,
selective reads/uploads, lease sharing/release, unused catalog growth, corrupted
destination data, capacity boundaries, repeated transitions, rollback and frame
allocation/upload invariants. Run affected publication/world/visibility and
headless integration checks plus campaign save/travel smoke in one configuration.
Inspect hidden production hub/dungeon/travel captures and record application GPU
steady/peak memory with supersampling ON. Off-mode is an optional comparison only,
not a required quality reduction. Hardware with 512 MiB/1 GiB remains unverified
unless actually available; accounting is not a driver-memory measurement.

One intensive job at a time, low CPU/I/O priority, two build workers maximum and
one test process. No routine benchmark/stress matrix or desktop input automation.
Record actual results and limitations in sc011_verification.md; update this design,
system overview and roadmap, stop at Ready for review. Do not begin SC01.2.

## Final implementation notes

Pack requirements are deliberately coarse: acquire the selected pack palette and
bindings, plus the actual actor types and common gameplay art. Assets are not
streamed by view frustum or individual animation frame. All current common art is
shared across locations; unused packs/types remain catalog-only. Actor records
marked removed remain part of the floor type set until leaving it; no mid-floor
asset eviction is required or performed.

Model payloads use the existing prepared triangle representation with checked
little-endian float32 encoding. Furniture constraints are unchanged; architecture
bindings/extents belong to SC01.2. Read-only catalog indices map to per-view typed
GPU handles; the GPU registry remains the lifetime authority. CPU mask access is
checked before rasterization and unprepared actor artwork rejects explicitly.

The measured supersampled hub/floor-one totals and advisory-bound overruns in
conservative location-pair projections are recorded in the verification handoff.
No quality setting is selected automatically from these reports.
