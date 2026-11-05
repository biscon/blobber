# SC01.2 residential corner captures

2026-11-05. **Ready for review; user visual acceptance pending.**
These are native 1920x1080 application-owned captures with the normal HUD and
supersampling ON, generated in hidden windows with in-process actions. No desktop
input or focus automation was used. See the [verification/handoff](../../sc01_verification.md)
for checks, resource totals and limitations.

| View | Purpose |
| --- | --- |
| [Corner](01-corner.png) | Starting residential L-corner |
| [Low-house entrance](02-low-door.png) | Closed timber leaf, rounded thick reveal and ring |
| [Tall-house entrance](03-tall-door.png) | Rotated building entrance and stone foundation |
| [Low roofline](04-low-roof.png) | Low-house scale from farther down the street |
| [Tall roofline](05-tall-roof.png) | Full upper storey and gable |
| [Low rear](06-low-back.png) / [tall rear](07-tall-back.png) | Connected rear access and inset shutters |
| [Side](08-side.png) | Material continuity and foundation contact |
| [Diagonal rooflines](09-roofline-diagonal.png) | Midturn silhouettes, roof overhangs and chimney |
| [Farther rooflines](roof/10-roof-materials.png) | Both complete silhouettes from the southwest |
| [Midturn corner](motion-6-2.png) | Oblique door reveal, window recesses and roof underside |

The 28 `motion-A-S.png` images record seven sequential normal gameplay actions from
spawn. `A` is the action index below; `S` is 1–4, sampling transition progress
0.25, 0.5, 0.75 and 1.0. Movement/heading and camera poses are in [report.txt](report.txt).

| Action index | Action |
| --- | --- |
| 0 | Forward |
| 1 | Forward |
| 2 | Turn left |
| 3 | Forward |
| 4 | Strafe right |
| 5 | Back |
| 6 | Turn right |

The main run contains 37 images. The additional farther roofline was exported
separately using the same final scene and assets; its [report](roof/report.txt)
records the selected shot. The current capture tool exports all 38 by default:

```sh
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_sanctuary_city cmake-build-debug/assets /path/to/new-directory
```

Append an exact shot name (for example `10-roof-materials`) to export just that
view. The destination must not exist. This tool needs graphics access; interactive
review is `cmake-build-debug/blobber --sanctuary-city-review`.

The fixed low camera crops nearby roofs; farther views show their silhouettes.
Existing far clipping/fog affects the distant enclosing wall in diagonal views.
These captures verify the selected review route, not a future full town layout.
