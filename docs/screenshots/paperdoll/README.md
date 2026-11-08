# Expanded inventory paperdoll

Status, 2026-11-08: **initial mockup accepted; first runtime layout rejected;
readability correction implemented and verified, awaiting user review**.
The latest card-finish follow-up adds padding only below the item square, uses
one uniform caption-colored outer fill and removes its inset outline. It is
implemented and mechanically verified; appearance remains awaiting review.
The subsequent outline/alignment revision removes the remaining outer perimeter
entirely and aligns Main Hand/Ranged with Ring 1/Ring 2. Item-square borders and
cursor feedback remain; this is the version shown in the current captures.

## Production captures

- [Inventory, native 1920×1080](inventory-native.png): fighter, compact equipment,
  centered Head/Feet, horizontally paired rings and attached caption bands.
- [Inventory, reduced 1280×720](inventory-1280.png): label/icon readability.
- [Ranged equipment](inventory-ranged.png): rogue with alternate bow.
- [Empty Wrists selection](inventory-wrists.png): item-square cursor highlight.

These refreshed captures replace the rejected version's crossing connectors and
floating labels. Independent connector paths never cross or touch; a focused
geometric regression also excludes other cards and their caption bands. Each
caption is inside its own filled card and selects that slot. AC/safety readouts
and the bag/details panels remain present.

These are application-owned exports from the production presenter, real inventory
controller and shipped artwork. The isolated review session uses a small empty
room and never edits a campaign. A hidden OpenGL window and in-process input
verify compact shield drag/drop, AC updates, first/repeated C++ frame allocations,
stable prepared GPU residency and cleanup/OpenGL checks. Driver/C allocations are
excluded. No desktop mouse, keyboard or window selection was controlled.

## Historically accepted concept

- [Generated full inventory, 1920×1080](inventory-paperdoll-v1.png)
- [Generated equipment close-up](equipment-closeup-v1.png)

The original concept arranged thirteen positions: Head, Body, Hands, Ring 1 and Feet on the left; Cloak,
Amulet, Wrists, Belt and Ring 2 on the right; Main Hand, Shield and Ranged below.
The revised runtime arrangement above supersedes that placement following the
user's readability feedback. Hands holds gloves/gauntlets, while Wrists supports future bracers. The nine new
positions are empty; current armor, sword and shield illustrate occupied slots.

The accepted concept was produced with the built-in imagegen tool using the existing
[inventory screenshot](../merchants/inventory-purse.png) as the edit target.
Preserved [initial prompt](mockup.prompt.txt) and
[refinement prompt](refinement.prompt.txt). The refinement reduces weapon-box
sizes and cleans up anatomical connector placement.

Visual inspection covered the generated source and the 1280×720 export: all
thirteen labels are present, new slots are empty, the bag remains 8×6, and glove,
wrist and ring targets are distinct. ImageMagick exported the selected 1672×941
generation at 1920×1080 and extracted the close-up; the reduced inspection export
was disposable. The concept is illustrative; production captures above show the
actual 72-pixel targets, native size-specific fonts and original artwork.
The generator omitted the current AC/safety text beneath the paperdoll; that
omission was not a proposed removal. Production retains both readouts.

The original mockup-only delivery changed no game code or saves. Runtime expansion
now uses campaign schema/simulation 18/22; older campaigns reject unchanged.
No new wearable items or magical effects were added to shipped content. See the
[design and verification commands](../../inventory_design.md#expanded-paperdoll-mockup--2026-11-08).
Six images are retained: the accepted concept pair and four distinct production
review cases. Both rejected-layout runtime captures and disposable run output were cleaned.
Interactive user review remains unverified; mockup approval does not imply
acceptance of the runtime appearance.
