# Painted combat feedback artwork

Option A selected by the user, 2026-11-05. Built-in ImageGen generated the
text-free transparent source, using the [approved concept](../../../docs/screenshots/combat-feedback-concepts/a-painted-splash.png).
[Exact prompt](source/prompt.txt); source: `source/painted-splashes.png`.

`python3 tools/prepare_combat_feedback.py` registers three cells in `backdrops.png`:
crimson damage, charcoal outcome, slate immunity. Output is 768x128 RGBA with three
256x128 cells. Near-invisible source residue below alpha 16 is cleared before
fitting bounds within 224x112; transparent gutters isolate each cell. Drawing uses
that inner region. Offline premultiplied resampling preserves soft edges, followed by
straight RGBA for the existing UI draw path. No lettering is baked into the art.
Only `backdrops.png` is packaged and loaded at startup (393,216 GPU bytes).

Frame sampling animates opacity and damage-number position; the artwork is static.
Healing/buff/spell portrait shimmers retain their SE02 artwork. This file is not
part of world asset publication and does not alter save formats.
See [design and verification](../../../docs/combat_feedback_design.md).
