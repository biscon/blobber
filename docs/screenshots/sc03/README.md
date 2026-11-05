# SC03 complete dressing — Ready for review

Current SC03 uses the accepted shorter tree, a smaller eastern tree, planted beds,
bushes, selective ivy, two timber stalls, three service signs and an animated
stone fountain. [Verification, resource totals and limitations](../../sc03_verification.md).

## Far enclosure and fountain tone — latest

The city view distance is now 40 cells; other packs retain 16. The fountain uses
the original carved stone with a subtle brightness lift. City fog remains capped.

| View | Capture |
| --- | --- |
| Whole northern enclosure from southern square | [Far view](distance-tone-v1/distance-south.png), [one step forward](distance-tone-v1/distance-step.png) |
| Fountain stone close-up | [Current tone](distance-tone-v1/fountain-north-facing.png) |
| Corner and diagonal | [Corner](distance-tone-v1/distance-corner.png), [half-turn](distance-tone-v1/distance-oblique.png) |
| Square | [Current](distance-tone-v1/square.png) |
| Resources and poses | [Report](distance-tone-v1/report.txt) |

[CPU checks](distance-cpu-checks.txt), [CLI check](distance-cli-checks.txt),
[hidden GPU checks](distance-gpu-checks.txt). All pass. Captures below predate this
view-distance and stone-tone correction and remain available for comparison.

## Earlier screenshot corrections

| Correction | Capture |
| --- | --- |
| Gentle city haze, visible far gate | [Current square](corrections-v1/square.png), [previous fog](dressing-v2/square.png) |
| Half-size stalls in the open square | [Market context](corrections-v1/market.png), [counter](corrections-v1/market-front.png), [grain stall](corrections-v1/market-grain.png), [cleared old alley](corrections-v1/former-market.png) |
| Room beside the inn | [Shifted dungeon entrance](corrections-v1/inn-spacing.png) |
| Ivy below the eaves | [Low roof](corrections-v1/ivy-alley.png), [close facade](corrections-v1/ivy-courtyard.png) |
| Grounded bushes and dedicated soil | [Lane bed](corrections-v1/lane-garden.png), [courtyard](corrections-v1/tree-east.png), [eastern bed](corrections-v1/east-garden.png) |
| Pale marble and solid stream silhouettes | [Close fountain](corrections-v1/fountain-close.png), [western approach](corrections-v1/fountain-west.png) |
| Layout and measurements | [Current layout](corrections-v1/layout.svg), [13-view report](corrections-v1/report.txt) |

Eight fountain angles: [N](fountain-orbit-v1/orbit-0.png),
[E](fountain-orbit-v1/orbit-1.png), [S](fountain-orbit-v1/orbit-2.png),
[W](fountain-orbit-v1/orbit-3.png), [NW](fountain-orbit-v1/orbit-4.png),
[NE](fountain-orbit-v1/orbit-5.png), [SE](fountain-orbit-v1/orbit-6.png),
[SW](fountain-orbit-v1/orbit-7.png). The four closed streams retain thickness;
normal occlusion by the fountain and overlap between streams remain expected.
Diagonal views are captured halfway through an in-process turn.

The revised animation is captured in `water-v2/`: [first frame](water-v2/water-0.png),
[last frame](water-v2/water-7.png), [clock/cache report](water-v2/report.txt),
[pixel checks](water-v2/pixel-checks.txt). Every 125ms step changes water pixels;
picking remains unchanged and the paused capture is pixel-identical to frame 7.

[Four focused CPU suites](corrections-cpu-checks.txt) and
[hidden GPU checks](corrections-gpu-checks.txt) pass. The atmosphere fixture verifies
both distant walls and actors, plus identical dungeon pixels with either profile.
Native captures are 1920×1080, supersampling ON, using hidden windows and in-process
movement only. Interactive acceptance remains user review.

Earlier `dressing-v1/`, `dressing-v2/` and `water-v1/` are retained for comparison;
the [earlier animation GIF](water-v1/fountain-loop.gif) predates the material/stream
corrections. Unchanged service signs: [inn](dressing-v2/sign-inn-near.png),
[smithy](dressing-v2/sign-smithy-near.png), [provisioner](dressing-v2/sign-provisioner.png).
Generated art and exact prompts live in the pack's
[source directory](../../../assets/packs/sanctuary-city/source), including
[soil/marble prompts](../../../assets/packs/sanctuary-city/source/review-art.prompt.json).

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
