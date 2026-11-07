# Offline sprite resolution review

2026-11-07. Mechanically verified; **user visual acceptance pending**.
[Design, measurements and checks](../../sprite_resolution_design.md).

Each comparison has the original preparation on the left and the final preparation
on the right. Each cell is the actual 500×282 world target, presented at native
size, after the normal 1000×564 supersampled render. Hidden application-owned
captures; no desktop input. Source artwork, world scale and animation registration
are preserved. The comparison labels are added outside the captured pixels.

- [Rat](rat.png): half resolution, near idle, side walking, peak lunge and death.
- [Goblins](goblins.png): 87.5%, melee goblin and archer, near and attack poses.
- [Zombie and shaman](zombie-shaman.png): 87.5%, near idle and peak melee lunge.
- [Far views](far.png): four reduced actors two cells farther away.
- [Standalone sprites](standalone.png): loot, Grease, Web and Spiritual Weapon,
  reduced from 1254×1254 to 512×512.
- [Rat attack inspection](rat-attack-inspection.png): six selected moments from a
  baseline combat sequence, from 160 ms before impact to 160 ms after it. These are
  world-view crops from full application captures, resized for the contact sheet.
  The reported detached specks were not reproduced; this is not evidence of a fix.

All four unchanged resident dialogue close-ups also compared pixel-identically;
duplicate images were discarded. Retain these six images until superseded by a
new resolution review. Temporary publications, raw captures and logs were cleaned.
