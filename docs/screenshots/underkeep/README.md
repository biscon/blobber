# Underkeep dressing review

[Contact sheet](contact.png) · [Footprint maps](maps.png) · [Capture manifest](captures.json)

Seed **0**, depth **1** (chamber network) and depth **3** (compact wings).
Full-resolution application-owned images are in [first-person](first-person/).
These are diagnostic preparation poses in ordinary generated geometry, not a
campaign combat/balance study. Room/detail shots retain their real populations.
Chest closed/open/empty and mid-turn shots omit NPCs and gameplay fixtures in an
isolated inspection session, preserving the generated supply contents and using
checked chest/pickup commands. They do not imply those supplies are unguarded.

Selected views:

- [Wall relief and hall](first-person/compact_wings-depth-3-hall.png)
- [Damp masonry](first-person/chamber_network-depth-1-damp-a.png)
- [Faded banner](first-person/chamber_network-depth-1-banner-b.png)
- [Fixed corner web](first-person/chamber_network-depth-1-web-a.png)
- [Bone scatter and empty cage](first-person/chamber_network-depth-1-bones-a.png)
- [Rack](first-person/compact_wings-depth-3-rack.png)
- [Closed chest](first-person/chamber_network-depth-1-chest-closed.png)
- [Opened chest](first-person/chamber_network-depth-1-chest-open.png)
- [Emptied persistent chest](first-person/chamber_network-depth-1-chest-empty.png)
- [Mid-turn](first-person/compact_wings-depth-3-mid-turn.png)

The maps show actual furniture occupancy: **C** chest, **X** cage, **R** rack.
The floor remains rendered under those occupied cells. Decorations are seeded
independently of the preserved room/population streams.

Release application build and the 16-check integration checkpoint passed; the
five affected asset-boundary/visibility/allocation checks passed after the last
renderer validation correction. Single-configuration generated publication/build
fixtures passed clean/no-op/change/touch/missing/add/remove/failure cases. Hidden
OpenGL review passed on NVIDIA RTX 3060 / OpenGL 3.3: draw errors, CPU/GPU picking
(including alpha and chest variants), and release of world resources back to the
persistent Presenter baseline. CPU/GPU interior comparisons allow fewer than 100
nonedge differing ID pixels per 500×282 view; no cross-device pixel guarantee is
claimed. Interactive play and final art acceptance remain user review.

No real lights, shadow maps or PBR were added. Props use painted shading plus
existing distance dimming/fog; terrain keeps its existing shading. Bags remain
for monster/party drops. Start a **new campaign**: schema/simulation 7/11 and
generator/catalog 9/7 reject older campaigns without modifying the files.

[Concepts, Blender/GLB sources and prompts](../../../assets/packs/underkeep/README.md)
· [Design and complete verification notes](../../underkeep_dressing_design.md)

To reproduce captures in a **new** output directory:

```sh
nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests \
  cmake-build-release/assets NEW_CAPTURE_DIRECTORY --underkeep-review
nice -n 15 ionice -c 3 python3 tools/underkeep_gallery.py \
  cmake-build-release/blobber_generate_floor NEW_CAPTURE_DIRECTORY NEW_GALLERY_DIRECTORY
```

The capture tool creates a hidden application window and uses in-process camera
poses. No mouse/keyboard injection, focus changes or desktop automation is used.
