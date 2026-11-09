# Inventory bag comparisons — 2026-11-09

Implemented and mechanically verified; interactive user review pending. These
three native 1920×1080 application-owned captures use a hidden window and
in-process input, with isolated review grants and no user-save changes.

- [Weapon comparison](inventory-weapon-comparison.png): Longsword +1 versus
  Alden's equipped longsword, including actual attack and damage values.
- [Wearable comparison](inventory-wearable-comparison.png): Belt of Vigor,
  +3 maximum HP and an explicitly empty equipped slot.
- [Restricted comparison](inventory-restricted-comparison.png): Lethiel's
  untrained chain mail preview, empty body slot and red equip restriction.

Produced by `blobber_review_trade`; the runner checks hidden-window state,
349/350 ms dwell, frozen gameplay time/pose, existing trade transactions and
zero released world texture/alpha residency. NVIDIA RTX 3060 / OpenGL 3.3.
See [design and verification](../../inventory_design.md#bag-comparison-tooltips--2026-11-09).
Captures do not establish interactive or listening acceptance. Retain these three
cases until superseded by a reviewed revision; disposable run output was cleaned.
