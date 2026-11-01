# Underkeep review corrections

2026-11-01. [Contact sheet](contact.png) · [Capture identities](captures.json) ·
[Design and verification](../../underkeep_review_corrections.md).

Nine application-owned hidden-window captures, at native 1920×1080, using production
Underkeep art and small controlled review rooms. These are reproducible diagnostic
fixtures, not a capture of the user's running game or a campaign balance route.
No OS input, focus changes or desktop automation.

- [Half-size closed chest](first-person/chest-half-closed.png),
  [open chest](first-person/chest-half-open.png): same floor anchor and picking.
- [Moss at the floor](first-person/moss-at-floor.png): both padded source variants.
- [Narrower/lower door](first-person/door-smaller-closed.png),
  [mid-swing](first-person/door-smaller-mid-swing.png),
  [open](first-person/door-smaller-open.png),
  [closed from reverse](first-person/door-smaller-reverse.png): stone frame fills the aperture.
- [Before the visible front clears](first-person/combat-before-front-clears.png),
  [after automatic facing](first-person/combat-after-auto-facing.png): the outer
  diagonal goblin becomes visible and pickable; Alden retains his current action.
  Diagnostic ActorWorld relocation/death reproduces the reported edge case;
  the automatic facing and subsequent attack eligibility use ordinary game rules.

The hidden runner checked GL errors, CPU/GPU interior picking agreement, visible
chest/enemy IDs and world resource release. GPU: NVIDIA RTX 3060, OpenGL 3.3.
Visual acceptance and interactive play remain user review.
