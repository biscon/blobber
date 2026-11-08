# Shadowtart

User-approved [character reference](../../../docs/mockups/emporium-shopkeeper-v1/README.md),
2026-11-07. Adult human sorceress with ebony waves, amber catlike eyes, jeweled
circlet and gold-trimmed indigo velvet gown. Runtime review remains separate.

Five built-in imagegen masters retain her approved identity: four two-pose
directional idles and four front talk poses. Exact prompts live beside the PNGs
in `animation/sheets`. No procedural image edits or body animation.

`python3 tools/register_resident_art.py shadowtart` measures alpha >15, planted
ground and torso pivots. Each sheet uses one uniform scale, maximum 464-pixel
body height, canvas 800×608 and anchor (400,584). Logical height .75 gives about
.572 cells / 1.83 m visible height. Left/right art faces screen left/right.
Idle durations are 1800/140 ms; Talk 180/160/180/220 ms, with directional idle
fallback for side/back views. The existing build extractor generates registered
frames, alpha cutouts and mips in the build tree.

Resident `sanctuary.shadowtart` stands at emporium cell (6,3), subcell (1,1),
facing south. Dialogue is `assets/dialogue/shadowtart.yaml`; shop is
`assets/shops/shadowtart.yaml`. See [design and verification](../../../docs/shadowtart_design.md).
