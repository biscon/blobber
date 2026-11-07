# Blacksmith character mockup

Status: **approved by the user (2026-11-07)**. Named **Duncan Ironballs**;
[production implementation](../../blacksmith_design.md) is mechanically verified;
runtime appearance and interaction review remain separate from this approval.

[Character sheet](turnaround-v1.png) · [Exact generation prompt](turnaround-v1.prompt.json)

Tall, exceptionally muscular human smith with a magnificent full copper-red
beard, shaved sides and a swept-back braided top inspired by the TV show
*Vikings*. Worn leather bib apron, sleeveless charcoal shirt, dark trousers and
heavy boots. Calm, approachable face and empty hands suit the planned idle and
talk animations.

Four complete cardinal views and one speaking portrait match the painted style
and reference-sheet format of the existing innkeeper. Generated with built-in
imagegen using the innkeeper reference as a style/format input. This sheet is an
approval reference, not a registered sprite sheet or an animation preview.

Production uses the innkeeper's four-view idle/front-talk pipeline and smithy
resident cell (6,3), subcell (1,1), facing south. Cell-based movement and the
staff/customer routes are preserved. Runtime verification is recorded in the
linked design; this concept sheet itself is not production animation artwork.

Visual inspection and PNG decode/alpha checks passed: 1920×819 RGBA with
alpha spanning 0–255. At the concept checkpoint no gameplay code changed or
gameplay tests were needed; subsequent implementation has separate verification.
Source artwork and prompt are durable references; the owned disposable run is
cleaned after inspection.
