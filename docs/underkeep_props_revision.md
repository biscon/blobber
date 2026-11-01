# Underkeep prop surface revision

Status: **Approved by the user, 2026-11-01**, including the terrain-density correction.
Supersedes the original box/UV art construction. Lessons for future work are in
the [prop modelling guide](prop_modelling_guide.md).

Blender emission renders expose massively stretched grain on chest walls and tiny
repeated boards on posts. Every face used 119 texels per edge, regardless of size.
The rack's eight rotated boxes have coincident overlapping end caps, rather than
forming a cylinder. Cage rails meet with overlapping coplanar faces; chest lid
straps touch the lid surface exactly. These are asset defects, independent of
runtime depth precision. Runtime inspection is recorded in the comparison gallery.

Rebuild rollers as closed octagonal prisms, inset rails between posts, and keep
metal straps visibly proud of wood. Use metric UV rectangles with a consistent
256 texels per runtime cell (128 per source unit for half-size chests, 256 otherwise).
This supersedes the original 80-texel target described in historical verification below.
The existing painted master remains the source: offline crop/repaint four padded
material fields with restrained grain, iron oxidation, dark undersides, and joint
shading. Face rectangles are sized to actual edges and long grain follows the
long edge. Face-boundary shading is baked into the atlas, never runtime lighting.
Use a 512-square chest atlas and 1024-square cage/rack atlases, with both chest
states sharing the chest atlas.

The exporter owns mesh, UV and atlas generation, packed Blender and GLB. Scene
preparation, half-size transform, floor anchors, conditional chest ranges, IDs,
footprints and saves stay unchanged. No frame storage or depth setting changes.
Verify emission renders from four sides and below; production hidden captures
from four headings, adjacent close view and five turn samples in both chest states.
Use focused mesh/render tests, GL error, CPU/GPU IDs and resource lifetime checks.

## Final result and independent renderer review

Blender emission views showed the face-size mismatch directly, including the
undersides and open lid. The production renderer separately showed noisy small
posts, broad smeared panels, and inconsistent detail during turns. Inspection of
its sampler confirmed nearest magnification with mip-linear minification; painted
mode bypasses face lighting. There is no evidence requiring global depth changes.
The rack end-cap overlap and chest base/wall overlap are geometric causes;
compressed grain on bars is a separate sampling/UV problem. Static images alone
cannot prove the absence of temporal flicker at every possible camera pose.

Final geometry retains both chest silhouettes and floor origins. Chest walls start
at the base top, avoiding exposed coincident side faces. Cage rails terminate at
post interiors. Rollers are single watertight octagonal prisms with one end cap
per end, and their undersides use dark wood. Raised bands use iron colour.
Triangles: chest 192 each, cage 672, rack 260 (previously 396).

Face rectangles use exact metric UV spans, including subtexel thin faces, rather
than rounding the UV scale. Integer atlas storage rounds up, with four replicated
gutter pixels per side. The shared master is sampled offline and desaturated;
face edges and undersides are darkened. Deterministic size-sorted packing reuses
same-size material rectangles. Exhaustion asserts instead of silently rescaling.
Only offline Python/Blender allocates; runtime ownership and budgets are unchanged.
The exporter checks every triangle's UV area against its physical area.
A separate check of the serialized JSON measured 79.996–80.015 texels/runtime-cell
across all four exports (rounding tolerance 0.1).

Focused Release `dressing` and `underkeep_render` passed in 0.10 s, covering the
preserved movement footprint, both half-size chest variants, floor anchoring and
picking contracts with owned fixtures. Production assets were rebaked. Hidden
application-owned review checks GL errors, CPU/GPU interior IDs, checked chest
opening and resource release; 40 captures per revision cover four headings,
adjacent views and five 25 ms turn samples for each prop/state. Blender supplied
20 emission views per revision, including undersides. No desktop control,
benchmark, full build matrix or runtime lighting change.

See the [in-game comparison gallery](screenshots/underkeep-props/README.md).
Regenerate game captures using `blobber_world_integration_gpu_tests ASSET_DIRECTORY
NEW_OUTPUT_DIRECTORY --underkeep-props-review`, then assemble the retained gallery
with `python3 tools/underkeep_props_gallery.py`. This is an explicit art diagnostic,
not an ordinary CPU test depending on game content.

Blender wrote all artifacts successfully but stalled at PulseAudio shutdown in
this sandbox. Only task-owned background processes were terminated; final runs
used timeouts. The connected interactive Blender scene was untouched. Graphical
capture needed sandbox escalation for display access and used hidden windows.
Logs are `/tmp/underkeep-{export-final,blender-final,props-bake-final,props-final}.log`.

## Terrain-density correction (2026-11-01)

User accepted overlap fixes and recoloring but rejected the 80-texel target as too
coarse beside the dungeon. The floor/wall sources are 256², with worldScale 1 and
one UV repeat per cell. Match **256 texels/runtime-cell**, or 128/source-unit for
half-size chests and 256 for cage/rack. This is 3.2 times the previous linear
resolution. Preserve geometry, recoloring and physical shading width. Sample the
retained 1254² master at higher resolution for genuine additional detail; do not
upscale the coarse atlases. Use 512² chest and 1024² cage/rack atlases with padded
face rectangles. Additional mip storage is about 11 MiB, within existing budgets;
no runtime or depth changes. Regenerate all authoring/export formats, check exact
UV density and unchanged positions/normals, rebake and inspect matching hidden
captures and Blender views. Keep the rejected 80-texel captures for comparison.

Serialized output checks measured 255.990–256.006 texels/runtime-cell on every
triangle. Vertex counts, positions and normals are exactly unchanged from the
accepted geometry. Blender emission inspection confirmed finer real grain while
retaining the color treatment; atlas size increased instead of stretching UVs.
The user chose in-game/model-viewer review, explicitly requested no new gallery,
and subsequently approved the result ("looks much better, approved"). The linked
gallery above remains historical evidence of the 80-texel revision. No new
in-game capture suite was run for this texture-only correction.

Release and Debug prepared asset publications both rebaked successfully: 401
textures, 359,771,008 mip bytes and unchanged 60,185,701 alpha bytes.
