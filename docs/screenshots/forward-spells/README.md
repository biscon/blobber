# Fixed party-front spell templates — 2026-11-08

User requested fixed forward placement; the corrected implementation is mechanically
verified and awaits user visual acceptance. These six hidden production-renderer
captures show the three fixed spells before and after casting. No desktop input or
focus automation was used. The six-goblin fixture includes creatures on both sides of the front formation;
all six lie inside the fan and the revised symmetric Thunderwave rectangle.

- [Burning Hands preview](burning_hands-preview.png) and [result](burning_hands-result.png): two near subcells, four far subcells; six recipients.
- [Thunderwave preview](thunderwave-preview.png) and [result](thunderwave-result.png): symmetric 4×2 rectangle beyond the front edge; eight subcells, six recipients, no third row.
- [Color Spray preview](color_spray-preview.png) and [result](color_spray-result.png): same six-subcell fan and fixed front edge.

The selected wizard remains rear-right. CPU regressions verify identical masks for
all four caster slots and headings, exact row membership and exclusion of every
party subcell. GPU checks verify counts, zero GL errors, allocation-free prepared
rendering and resource release. See [design/verification](../../spell_area_design.md#party-front-fixed-templates--follow-up-2026-11-08).

The two Thunderwave images replace the earlier asymmetric 3×3 evidence after the
user requested four near and four far subcells. The other four images remain current.
