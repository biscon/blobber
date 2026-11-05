# Combat feedback backdrop concepts

Status: **Proposed mockups; awaiting user choice (2026-11-05)**.
These are built-in ImageGen illustrations, not application captures or implemented
UI. The generator slightly redraws the surrounding screenshot; only the proposed
feedback backdrop/lettering is under review. No gameplay or renderer code changed.

- [A — Painted splash](a-painted-splash.png): rounded, textured crimson blood pool
  behind damage numbers, with small droplets and subtle wet highlights. Charcoal
  ink for SAVED/MISS and slate for IMMUNE. Stronger classic party-RPG treatment.
- [B — Soft ink wash](b-soft-ink-wash.png): flatter matte blood-red stain for damage,
  with feathered brush edges and a lighter visual footprint. Neutral washes for
  non-damaging outcomes. More of the enemy/effect remains visible.

Both replace comic-style pointed starbursts. The enlarged examples are detail
studies, not proposed in-game size. Existing green healing and blue buff portrait
shimmers remain separate; their replacement is not implied by these concepts.

Proposed motion, subject to choice/tuning: show the complete readable result on
the resolution frame, without fade-in. Damage numerals get a tiny damped shake
(roughly 1–2 presentation pixels for 80–120 ms); the backdrop remains stable.
Hold briefly, then fade backdrop and lettering out over approximately 200–250 ms,
finishing around 600 ms. SAVED/MISS/IMMUNE stay steady. This is local text motion,
not party-card or camera shake. Timing strips are static illustrations only;
legibility and reduced-motion behavior require implementation review later.

Both images were visually inspected for style difference, rounded/soft silhouettes,
readable labels and neutral no-damage treatment. No tests or builds were needed
for this mockup-only task. SE02 remains Ready for review, not Accepted. The next
step is user choice or refinement, followed by an implementation scoped to that
choice. Original ImageGen outputs remain in the tool's default directory.

[Retained reference and exact prompts](../../../assets/effects/combat-feedback-concepts/README.md).
Two selected boards retained. The owned scratch run was cleaned after promotion;
no temporary diagnostics retained.
