# SC02 town review

2026-11-05. **Accepted by the user.**
Native 1920×1080 captures, normal HUD/camera, supersampling ON. Hidden windows and
in-process movement only. [Verification and limits](../../sc02_verification.md).

Start with the authoritative [top-down SVG](review/layout.svg) or
[PNG preview](review/layout.png). Blue is reserved fountain base; ochre is future
market base; green is future planting. Dashed routes are the two tested street
loops. Dressing and service/travel interaction are not implemented in SC02.

| View | Purpose |
| --- | --- |
| [Arrival](review/arrival.png) / [square](review/square.png) | Compact access to northern sanctuary and dungeon landmarks |
| [Northern rooflines](review/roofline.png) | Bell-cote, dungeon entrance and inn silhouette |
| [Gate street](review/gate-street.png) / [farther gate](review/gate-roofline.png) | Closed paired timber gate and boundary composition |
| [West gate join](review/gate-west-join.png) / [east join](review/gate-east-join.png) | Oblique recess and neighboring buildings |
| [Gate strafe](gate-motion/motion-gate-0-2.png) | Gate face during normal lateral movement |
| [Alley](review/alley.png) / [moving](review/motion-alley-1-2.png) | One-cell passage and roof clearance |
| [Courtyard](review/courtyard.png) / [market edge](review/market.png) | Undressed SC03 reservation areas |
| [Sanctuary](review/door-sanctuary.png) / [dungeon](review/door-dungeon.png) | Named accessible landmark approaches |
| [Inn](review/door-inn.png) / [smithy](review/door-smithy.png) / [provisioner](review/door-provisioner.png) | Static service-building facades |
| [West loop mid-turn](review/motion-loop-0-turn-2.png) / [east loop mid-turn](review/motion-loop-1-turn-2.png) | Corner/depth continuity during turning |
| [Southern lane](review/south-lane.png) | Hipped/gabled residential variation |

The [main pose/resource report](review/report.txt) covers 55 frames: 23 named stills,
eight loop corners, 16 loop movement/turn samples, and eight alley forward/back
samples. Both complete loops execute through production movement. The
[gate report](gate-motion/report.txt) adds eight lateral movement frames and full
active/staged city cancellation/accounting. Samples use progress 0.25/0.5/0.75/1.0.
Current default capture emits all 63 frames, plus SVG and report.

`town/` is the initial partial capture attempt, stopped by an invalid diagnostic
camera pose. Use `review/` and `gate-motion/` for the delivered evidence.

```sh
cmake-build-debug/blobber --sanctuary-city-review
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_sanctuary_city cmake-build-debug/assets /path/to/new-directory
```

Append `stills`, `motion`, `materials`, `gate-motion`, or an exact shot name for a
subset. Append `layout` for an SVG-only export without graphics or asset loading.
The destination must not exist. Leave interactive input/focus to the user.
