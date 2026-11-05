# SE01 — review evidence

Status: **SE01 accepted by the user (2026-11-05)**; local lighting specifically praised.
See [implementation and checks](../../se01_design.md) and the
[spell-effects roadmap](../../spell_effects_plan.md).
SE02 is Ready for review in its [six-file gallery](se02/README.md).

These captures use production artwork and rules, ordinary accepted spell commands,
seed 5, a legal spell-party preset, and application-owned hidden render targets.
No OS input, focus changes or desktop screenshots. Enemy AI is disabled to make
camera/phase comparisons deterministic. Combat feedback and HUD are unchanged.

## Motion

The three GIFs preserve 60 ms sampled timing through 1,320 ms, followed by a 600 ms
hold on the final frame before looping. They are 960×540 palette-quantized previews;
the selected PNGs below retain the application's exact 1920×1080 output. Sampling
is for review and does not change the renderer's continuous age-based trajectory.

- [Fire Bolt: corridor travel, light and attached aftermath](fire-bolt.gif)
- [Ray of Frost: lance, contact and fading coating](ray-of-frost.gif)
- [Burning Hands: instant area release, expanding fan and dying-pose remnants](burning-hands.gif)

## Selected stills

| Capture | Case |
| --- | --- |
| [Fire flight](fire-flight.png) | Range 3, 120 ms; warm wall/floor illumination in a narrow corridor |
| [Fire aftermath](fire-aftermath.png) | Range 3, 480 ms; attached flame after the 288 ms impact |
| [Frost flight](frost-flight.png) | Range 2, 120 ms; narrow lance and cold wake |
| [Quiet frost](frost-slow.png) | 1,700 ms; patchy coating follows authoritative Slow after the burst |
| [Crowded cone](cone-crowded.png) | 120 ms; three original recipients, instant damage and one broad layered fan |
| [Sparse cone](cone-sparse.png) | 120 ms; one original recipient, eligible empty cells still receive the fan |
| [Near fire](fire-near.png) | Range 1, 120 ms; near-body scale |
| [Far frost](frost-far.png) | Range 4, 120 ms; distant trajectory readability |
| [Daylight fire](fire-bright.png) | City pack/daylight, 120 ms; bright-surface treatment |

The cone victim count describes initial targets; dying or removed bodies may have
already disappeared by the selected frame. Remnants are cosmetic and do not add
damage. An entirely empty cast remains rejected by existing gameplay rules.

Twelve accepted reference images are retained as an exception to the six-image
default for subsequent spell slices: three motion sequences establish each spell’s
phase timing; nine full-resolution stills separately cover corridor light, attached
fire, frost travel, sustained Slow, crowded/sparse cones, near/far scale and daylight.
GIF previews cannot replace full-resolution checks, and reducing the stills would
lose distinct comparison cases. Superseded iterations and intermediate frames were
removed with the disposable run. SE02 adds six grouped files in its own directory;
the eighteen-file feature total retains these twelve accepted baseline cases plus
the new offensive/portrait comparisons, without changing SE01 acceptance.

## Reproduce

Build `blobber_world_integration_gpu_tests` and the normal `blobber` target so the
production publication is current, then create an owned artifact run:

```sh
python3 tools/agent_artifacts.py create
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/production --se01-review
```

`RUN/production` must be new or empty. Optional final argument selects one case:
`fire-bolt`, `ray-of-frost`, `burning-hands`, `fire-near`, `frost-far`, `cone-sparse`,
or `fire-bright`. The runner records impact offsets and memory totals in
`checks.txt`. Use `--se01-check` without asset/output paths for the small owned
hidden GPU regression fixture. Clean the run after selecting evidence.

Generated artwork and prompts are retained in
[assets/effects/se01](../../../assets/effects/se01/README.md); these captures are
review evidence, not source artwork. Interactive feel and cross-device appearance
remain unverified by the agent; slice acceptance comes from the user’s explicit
review, not from passing mechanical checks.
