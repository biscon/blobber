# SC04 campaign captures

2026-11-05 — Accepted and closed, including the entrance correction. Hidden application-owned captures using the normal
CampaignRun, GameView, Presenter and disk-save paths; no desktop input automation.
Seed 5, supersampling on, NVIDIA RTX 3060, 1920×1080.

| View | Evidence |
| --- | --- |
| New Game square arrival | [North-facing square](campaign/hub.png) |
| Initial partial exploration | [Initial map](campaign/hub-map-initial.png) |
| Working dungeon entrance | [Entrance and F prompt](entrance-fix/dungeon-entrance.png) |
| First dungeon floor | [Floor one](campaign/floor-one.png) |
| Upstairs return facing town | [Return view, fountain and closed gate](campaign/hub-returned.png) |
| Recall without implicit healing | [Sanctuary arrival, 11/12 HP and R prompt](campaign/recall-sanctuary.png) |
| Explicit sanctuary rest | [Restored health](campaign/sanctuary-rested.png) |
| Disk Continue | [Continued sanctuary](campaign/continued-city.png) |
| Discovery after Continue | [Preserved explored map](campaign/continued-map.png) |

[Residency measurements](campaign/memory.txt), [capture log](capture-log.txt),
[GPU integration](integration-gpu.txt), [architecture/water checks](architecture-gpu.txt).
See [verification/handoff](../../sc04_verification.md) for test limits and review steps.

Reproduce into a new output directory:

```sh
nice -n 15 ionice -c 3 cmake --build --preset debug --target blobber_review_world_residency -j 2
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_world_residency cmake-build-debug/assets NEW_OUTPUT
```

The tool creates an isolated `review-save.json`/lock alongside its captures.
Keep those fixture files in ignored build scratch rather than the documentation gallery.


## Entrance correction

The bright arch strips inside the black stair opening are hidden. These captures
supersede the [original entrance view](campaign/dungeon-entrance.png).

| View | Corrected capture |
| --- | --- |
| Street approach matching the reported issue | [Street](entrance-fix/entrance-street.png) |
| Close approach and F prompt | [Close](entrance-fix/dungeon-entrance.png) |
| Left approach | [Left](entrance-fix/entrance-left.png) |
| Right approach | [Right](entrance-fix/entrance-right.png) |

[Capture log](entrance-capture-log.txt), [GPU parity/allocation checks](entrance-gpu.txt).
Append `--entrance` to the capture command above for this shorter review run;
it exits after the entrance poses without travel or save creation.
