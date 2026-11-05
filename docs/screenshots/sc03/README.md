# SC03 complete dressing — Ready for review

Final SC03 uses the accepted shorter tree, a smaller eastern tree, planted beds,
bushes, selective ivy, two timber stalls, three service signs and an animated
stone fountain. [Verification, resource totals and limitations](../../sc03_verification.md).

| Final view | Capture |
| --- | --- |
| Fountain and square | [Square](dressing-v2/square.png), [mid-turn](dressing-v2/fountain-turn.png), [close](dressing-v2/fountain-close.png) |
| Market | [Oblique canopy view](dressing-v2/market-oblique.png), [counter](dressing-v2/market-front.png) |
| Inn | [Tankard sign](dressing-v2/sign-inn-near.png) |
| Smithy | [Anvil sign](dressing-v2/sign-smithy-near.png), [oblique](dressing-v2/sign-smithy.png) |
| Provisioner | [Provisions sign](dressing-v2/sign-provisioner.png) |
| Planting | [Courtyard](dressing-v2/tree-east.png), [eastern garden](dressing-v2/east-garden.png), [lane bed](dressing-v2/lane-garden.png) |
| Ivy | [Courtyard facade](dressing-v2/ivy-courtyard.png), [square context](dressing-v2/square.png) |
| Layout and measurements | [Final layout](dressing-v2/layout.svg), [24-view report](dressing-v2/report.txt) |

[Animated fountain preview](water-v1/fountain-loop.gif) is assembled from eight
application-owned captures. It is half-size, nearest-neighbor, with a shared GIF
palette. Native 1920×1080 PNGs in `water-v1/` are authoritative. The
[clock/cache report](water-v1/report.txt) records all eight phases, constant picking
revision and a paused update. The paused image is pixel-identical to the last
active frame; [pixel checks](water-v1/pixel-checks.txt).

The final captures are `dressing-v2/` and `water-v1/`. `dressing-v1/` retains the
initial sign-height and dark-stream comparison. Close views deliberately retain
the fixed camera; standing directly under signs/canopies may crop them. Final
visual acceptance remains user review.

[Layout/scenery checks](dressing-cpu-checks.txt),
[CPU integration](dressing-integration-checks.txt),
[GPU integration](dressing-gpu-checks.txt).
Generated art masters live in [the pack source directory](../../../assets/packs/sanctuary-city/source);
exact built-in ImageGen prompts are retained for [dressing](../../../assets/packs/sanctuary-city/source/dressing-art.prompt.json)
and [tree art](../../../assets/packs/sanctuary-city/source/tree-art.prompt.json).

## Accepted representative tree comparison

The user liked the initial tree but requested a shorter crown for close views.
The revision lowers overall height by 17.5% while retaining canopy width, texture
density and the existing camera. The user accepted this shorter tree and
authorized the remaining dressing, now delivered below.

| View | Initial tree | Shorter tree |
| --- | --- | --- |
| Eastern approach | [Before](tree-v1/tree-east.png) | [After](tree-v2/tree-east.png) |
| Western approach | [Before](tree-v1/tree-west.png) | [After](tree-v2/tree-west.png) |
| Street / oblique | [Before](tree-v1/tree-street.png) | [After](tree-v2/tree-street.png) |
| Northern close view | [Before](tree-v1/tree-north.png) | [After](tree-v2/tree-north.png) |
| Southern close view | [Before](tree-v1/tree-south.png) | [After](tree-v2/tree-south.png) |
| Square / architecture overlap | [Before](square-v1/tree-square.png) | [After](tree-v2/tree-square.png) |

`tree-v1/` retains twelve forward/back/turn samples. `tree-v2/` repeats the same
movement at 25%, 50%, 75% and settled phases; filenames identify the movement
action enum and sample. All captures use native 1920×1080 output, supersampling ON,
seed 5, a hidden application window and scripted in-process movement. No desktop
input or focus was controlled. Pixel-art readability and final visual acceptance
remain user review; close-camera crown cropping is expected.

[Tree checkpoint layout](tree-v2/layout.svg), [tree checkpoint poses/resource report](tree-v2/report.txt),
[CPU integration](integration-checks.txt), [GPU integration](gpu-checks.txt),
and [verification/handoff](../../sc03_verification.md).
