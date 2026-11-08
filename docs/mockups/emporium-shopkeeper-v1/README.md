# Arcane Emporium shopkeeper concept

Status: **approved by the user (2026-11-07)**. Named **Shadowtart**; authorized
implementation is recorded in the [resident design](../../shadowtart_design.md).
Runtime approval remains separate from this reference approval.

[Character sheet](turnaround-v1.png) · [Exact generation prompt](turnaround-v1.prompt.json)

An original adult human sorceress with long, wavy ebony hair, catlike amber eyes
and a delicate antique-gold circlet with an amber jewel at the forehead. A fitted
midnight-indigo/black velvet gown with gold embroidery gives her an alluring,
mysterious medieval fantasy appearance. Small red and blue belt vials echo her
planned health and mana potion stock.

Four complete cardinal views and one speaking portrait follow Duncan's approved
reference format and the game's painted character style. The mouth-open portrait
and neutral standing views establish the talk/idle direction; this sheet is an
approval reference, not registered animation frames or a runtime screenshot.

Implementation reuses the innkeeper/Duncan pipeline: four-view breathing/blinking
idle and front-facing talk, with planted feet and consistent identity. Use the
existing [emporium](../../emporium_interior_design.md) reserved merchant location
behind the counter at cell (6,3), subcell (1,1), facing south; preserve cell-based
customer/staff routes and current prepared-image rendering. Current progress and
verification are recorded in the linked design.

The user requests **health and mana potions only** for her initial stock: the
existing item IDs are `healing_potion` and `mana_potion`. Scrolls and new potion
mechanics are outside this slice. The existing inn stock is unchanged at this
checkpoint; the approved implementation moves potion retail stock to Shadowtart.

Generated with built-in imagegen using the blacksmith sheet as a style/format
reference only. Exact prompt and original generated-source location are retained
beside the durable PNG. Visual inspection confirms four standing views plus one
speaking bust, black hair, amber eyes and the jeweled circlet. PNG decode passed:
1920×819 RGBA, alpha spanning 0–255; the workspace copy matches the generated
source byte for byte, and the prompt JSON parses. No gameplay tests or desktop interaction are needed
for this artwork-only checkpoint. The owned disposable run is cleaned after
inspection; source artwork and prompt remain durable.
