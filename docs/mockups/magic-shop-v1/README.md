# Arcane Emporium — magic shop concepts

Status: **both designs and the name approved by the user (2026-11-07)**.
[Implementation design](../../emporium_interior_design.md) records the completed
room/sign slice and mechanical verification; runtime acceptance remains separate.

- [Interior](01-interior.png) · [Exact generation prompt](01-interior.prompt.json)
- [Exterior sign face](02-exterior-sign.png) · [Exact generation prompt](02-exterior-sign.prompt.json)

Proposed replacement for Sanctuary's provisioner: a magic shop/apothecary
specializing in potions, spell scrolls and arcane supplies. Ordinary provisions
remain available at the inn. **Arcane Emporium** is the proposed descriptive name,
not an asserted official D&D shop category. Apothecary/alchemist's shop would
emphasize the potion side; the broader name leaves room for scrolls and books.
For a setting example, [Sorcerous Sundries](https://bg3.wiki/wiki/Sorcerous_Sundries)
trades in potions, spell scrolls, books and other magical goods.

## Interior direction

Warm plaster, stone and oak match the existing inn/smithy. Indigo and gold details
distinguish the shop. Scroll cubbies and books line the left wall; potion shelves
and small drawers sit at the rear; a compact copper alembic, ingredients and
mortar occupy the right-hand workbench. The low rear counter leaves an empty
merchant standing volume and a staff bypass at its right end.

The broad central approach and rectangular wall-side fixtures are intended for
cell-based movement. This is an art reference, not a measured grid plan or a
runtime capture. After approval, define whole-cell collision footprints and check
the actual customer/merchant range, scale and reachable aisles before exporting
geometry. NPC identity/animation and stock, including scroll mechanics, remain
separate implementation work.

## Exterior sign — required follow-through

**Implemented:** the outside provisioner sign is replaced with the approved design.
The proposed indigo sign uses a large teal potion flask, cream parchment scroll
and gold magical star, with no lettering. It preserves the weathered wooden
board/border style of the existing signs and should remain readable at small size.

The current building/door identifier is `provisioner`; its sign input is
`assets/packs/sanctuary-city/source/provisioner-sign.png`. The dressing exporter
`tools/export_city_dressing.py` packs that artwork into the city atlas and sign
meshes, so replacing the source image alone is insufficient. Plan the packed
Blender/GLB, runtime sign mesh/atlas regeneration and an exterior capture after
approval. Keep the existing building footprint and street approach.

At the concept checkpoint no live assets or gameplay changed. The subsequent
room/sign implementation is complete; merchant inventory and user saves remain
unchanged. Inn supplies/potions stay as they were until the merchant slice.

## Provenance and checks

Both images were made with built-in imagegen. The interior uses the inn runtime
capture and approved smithy concept as style/scale references; the sign edits a
copy of the existing provisioner artwork. Exact requests and generated-source
locations are retained beside each PNG; these workspace copies are durable.

Visual inspection and PNG decode/dimension checks passed. Interior: 1672x941;
sign: 1254x1254, both opaque RGB/RGBA. The interior shows empty circulation and
merchant space; the sign has no remaining food/wheat/sack emblem. No gameplay
tests or desktop interaction were needed. The owned empty disposable run is
cleaned after inspection. Both concepts were subsequently approved; runtime review
and its selected evidence are recorded in the linked implementation design.
