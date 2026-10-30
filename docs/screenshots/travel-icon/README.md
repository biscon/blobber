# Travel icon review (2026-10-30)

The generated transparent brass compass replaces the text Travel button beside
Camp and Settings. The same logical rectangle controls drawing and clicks;
hover shows Travel [T], and the existing shortcut and travel rules are unchanged.

Built-in `image_gen.imagegen` produced [the asset](../../../assets/ui/travel.png)
using Camp as its style reference. The exact prompt is retained in
[travel.prompt.json](../../../assets/ui/travel.prompt.json).

[Native 1920×1080 capture](travel-hud.png) · [1280×720 capture](travel-hud-1280.png) · [provenance](retained.json)

Both sizes were visually inspected. These hidden application-owned captures use
seed 5 and the existing save/load capture fixture, including diagnostic two-HP
injury; no fought expedition or balance experiment is claimed. No OS input or
window focus automation ran. Interactive review remains with the user.

Release HUD, campaign UI and allocation checks pass (3/3); final allocation guard
rerun passes with Travel hover covered. The hidden `blobber_gpu_smoke` check passes,
including the new icon region and existing eight-texture capacity check. Logs:
`/tmp/travel-icon-tests.log`, `/tmp/travel-icon-allocations-final.log`,
`/tmp/travel-icon-gpu.log`, `/tmp/travel-icon-captures.log`. Formatting and
`git diff --check` pass. Existing working-tree changes were preserved.
