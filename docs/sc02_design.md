# SC02 — Complete town architecture and layout

Status: **Implemented; Accepted** (2026-11-05).
The user explicitly accepted the delivered slice on 2026-11-05.
The user approved the conversation plan and requested implementation. SC01 is
accepted, including its material correction. Campaign cutover remains SC04.

## Content and boundaries

Delivered a compact square-centered 24×24 outdoor town with ten principal buildings:
six homes, inn, smithy, provisioner and sanctuary. Three are two-storey shells.
A separate dungeon entrance and modest permanently closed gatehouse are landmarks.
The square connects residential and service loops, a one-cell alley and sheltered
courtyard. Main paths are two or three cells wide. Arrival is at most six movement
steps from rest and dungeon landing; rest-to-dungeon is at most eight.
Fountain, market and planting reservations remain undressed for SC03. Check
connectivity with their future bases blocked. Interiors/services/travel stay absent.

The accepted limestone/plaster/slate/oak art remains the reference. Vary facade,
roof, shutter and doorway construction, retain real recessed entrances and closed
shells. Model the gate and boundary one cell inside the outer solid map ring so
existing placement constraints remain valid. Keep a three-cell gate approach clear.
Camera, 16-cell far plane, fog, fixed presentation and supersampling are unchanged.

## Ownership and interfaces

The world city factory owns the map plus bounded authored metadata: stable named
doorways with placement indices/approach poses, arrival/rest/dungeon/gate anchors,
and rectangular future dressing reservations (16 doorways and 16 reservations).
Preparation-only validation checks IDs, approach ownership and facing, disjoint
reservations, protected cells, route lengths and connected walkable space with and
without future bases. Named loop routes and the narrow alley are checked as well.
No metadata is serialized into WorldDefinition; existing building records remain
the collision/sight and generated-wall replacement authority. No schema changes,
portal/entity framework or campaign changes are needed. Frames borrow immutable
prepared geometry; city authoring and validation may allocate only during loading.

## Assets and budgets

Extend the reproducible Blender exporter using shared architectural components and
accepted original material masters. Preserve metric 256 texels/cell, inset panel
extraction, continuous material fields and aligned 16-pixel padding. Deterministic
multipart mesh exports and at most two 4096² atlases use existing runtime formats.
Keep packed Blender, unlit GLB, runtime meshes and measured export report together.
New painted artwork, if needed, uses ImageGen; no procedural icon substitutions.

SC02 limits: 16 named models, 96 placements, 16 doorway records, 16 reservations,
two 4096² atlases. Runtime limits remain 32 materials, eight parts/model, 6,000
vertices/part and 24,000/model, +/-4 horizontal and 0–4 vertical model bounds.
Existing 64 MiB CPU/40 MiB GPU scene, 8 MiB view scratch, 512 MiB mip textures,
64 MiB alpha masks and 352/608 MiB tracked CPU/GPU ceilings still apply, including
active/staged accounting. Exhaustion rejects preparation; no fidelity reduction,
frame allocation, new visibility system or asset creation during play.

## Review and verification

The existing development flag consumes the full city without save/library startup.
The capture tool uses named metadata for stills and scripted routes, exports a
labeled authoritative top-down SVG, and records asset/scene/draw/memory totals.
Keep selective capture and new-directory semantics; historical SC01 images remain.

Fast owned CPU checks cover topology, reservations, approaches, identity/capacity
failure and gate obstruction; generated geometry/publication fixtures cover joins,
multipart exports, bounds, visibility and preparation failure. Run architecture,
material, options, movement/visibility, residency and publication/save-smoke callers
in one Debug configuration. Hidden GPU checks exercise CPU/GPU agreement, rollback,
release, warmed allocations and moving/settled cache policy. Production captures
are separate art verification, not artwork-dependent correctness tests.

One intensive job at a time, low CPU/I/O priority, <=2 build workers and one test
process. No desktop input/focus control, routine benchmark or configuration matrix.
Record actual checks, totals, captures, limitations and regeneration/review commands
in sc02_verification.md. The delivery stopped at Ready for review and was
subsequently accepted by the user; SC03 requires its own requested planning.

## Implemented layout and handoff

The shipped factory uses 91 placements (12 shells/landmarks plus 79 wall modules),
10 catalog models, 12 named approaches and five future dressing reservations.
The two loops remain open after future bases are blocked. Arrival/rest/dungeon
routes are four/four/six steps. No renderer/camera/schema change was required.

The gate is a 5×2 solid footprint; generated boundary faces are replaced through
the existing ownership mask. Its front approach is protected. The future dungeon
fixture is the clear cell (14,6) in front of the shell, with
landing (14,7), facing north. Fixture admission is checked against the existing
FloorGeometry contract; future-route validation treats the stair cell as blocked.
SC04 can install the existing prepared stairwell without altering the solid shell.
The development scene contains no active travel or rest fixtures.

The exporter uses two material parts per model and two shared 4096² atlases. Both
closed-loop walking and selected turn/translation/strafe samples are captured by
the review tool. A second full city view stages/cancels without replacing active
state. Actual counts, memory, checks and visual limits are in the
[verification and next-agent handoff](sc02_verification.md).
