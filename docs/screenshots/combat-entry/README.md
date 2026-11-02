# Combat entry correction review — 2026-11-02

Hidden 1920×1080 application-owned captures using production Underkeep surfaces
and goblin art, with safe owned mechanics. Placeholder party names/portraits are
intentional test data. These fixtures do not load or modify the user's campaign.

- [Entry faces an exposed front](entry-faces-exposed-front.png): the initial
  North-facing diagonal showed only a protected rear body. Entry faces East,
  exposing a visible melee target before the fighter's decision.
- [Turn-based near-row arrival](turn-based-near-row-arrival.png): a healthy goblin
  approaches from a distant offset cell, joins the existing central-front cell
  and fills its remaining near-row slot. The party keeps its North heading.

Both frames validate a rendered/pickable target's melee command. Hidden GL drawing
and world resource release pass. CPU regressions additionally cover all four
headings, cleared-front movement, settled arrival, queued input, stale routes,
slot contention and ranged-retreat exceptions. See the
[design and verification](../../combat_entry_followup.md). Interactive review remains
with the user; no OS input was automated.
