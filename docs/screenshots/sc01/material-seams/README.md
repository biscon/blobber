# SC01.2 material seam correction

2026-11-05. Ready for review. Original generated artwork retained; offline extraction
now excludes its panel-divider/shadow bands. The atlas uses aligned 16-pixel gutters
and remains 4096² at 256 texels/cell. Geometry, runtime filtering and shaders are unchanged.

The same pose as the reported view, captured with native HUD and supersampling ON:

| Before | After |
| --- | --- |
| [Original exported materials](before/11-material-seams.png) | [Corrected materials](after/11-material-seams.png) |

The vertical/horizontal plaster lines and straight paving-divider line are removed.
Additional inspected views: [entrance](after/02-low-door.png),
[side](after/08-side.png), [diagonal distance](after/09-roofline-diagonal.png), and
[mid-turn corner](after/motion-6-2.png). Existing far clipping of the enclosing wall
remains visible in the distant diagonal shot.

[Before report](before/report.txt) and [after report](after/report.txt) record poses
and resource totals. Application GPU storage is still 271,277,264 bytes, with a
287,866,064-byte capture peak. Driver memory is excluded. World texture/model leases
return to zero after view release. The retained [check output](checks.txt) records
the five owned exporter tests and successful export/publication totals.

Reproduce the corrected views (graphics access required; hidden application-owned
windows and in-process actions only):

```sh
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_sanctuary_city cmake-build-debug/assets /path/to/new-directory materials
```

Use `11-material-seams` instead of `materials` for just the matching pose.
The original captures remain preserved. No desktop input or focus automation was used.
Full context and limitations: [verification/handoff](../../../sc01_verification.md).
