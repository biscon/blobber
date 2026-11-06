# Innkeeper

The first peaceful resident NPC, placed behind the Sanctuary inn bar. The user
approved [reference v1](reference/turnaround-v1.png) on 2026-11-06. Runtime visual
acceptance is separate from that approved character direction.

Five production masters were generated with the built-in imagegen tool from the
approved reference: two idle poses in each of four cardinal directions and four
front-facing talk poses. Exact requests sit beside each master in
`animation/sheets/`. Left/right use the existing anatomical-side convention;
anatomical left faces screen left. Runtime review exposed reversed original side
labels; both side masters, their unchanged exact requests, and registrations were
relabeled together. The original prompts' anatomical wording was mistaken; their
explicit screen-facing directions and image bytes are preserved. The stable
definition/frame paths are unchanged, preserving campaign content compatibility.
No walk, attack, hurt, death or prone artwork
is required for this resident.

Idle uses 1800/140 ms so the blink is brief. Talk uses 180/160/180/220 ms and
falls back to directional idle from the sides/rear. Normal gameplay selects idle;
talk is prepared for future dialogue and exercised by the review runner. No
dialogue prompt or trading interaction is exposed yet.

`animation/sheets.json` and `animation/registration_audit.json` record measured
alpha bounds, registered ground rows and upper-torso pivots. Each sheet uses one
uniform scale and 800×608 output canvases anchored at (400,584), with a maximum
480-pixel standing body height. Logical canvas height .75 cells and proportional
clip width preserve the same human scale as the existing villager artwork;
occupancy remains one subcell. The ordinary extractor/publication produces
registered derivatives and cropped mip chains in the build tree. Source masters,
prompts and registration inputs stay durable here.

See the [resident system design](../../../docs/innkeeper_design.md) for ownership,
hostility exclusions, persistence, verification and remaining review status.
