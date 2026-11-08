# Compact music announcement — 2026-11-07

**Compact typography accepted by the user; requested top-left integration
implemented and mechanically verified.** The game now uses this layout, with
24 px bold title and 18 px regular caption/artist atlases drawn at loaded size
and whole native pixel positions. The panel sits 24 pixels inside the viewport's
top and left edges, preserving the approved internal padding and six-second
timing. Width stays fixed; long lines clip with measured ellipses.

- [Settled announcement](compact-preview.png): 540 × 90 px one-line panel.
- [Two-line announcement](compact-two-lines.png): 540 × 117 px panel.
- [Long-title clipping](long-title.png): wide glyphs and an overlong second line
  remain within the panel, with ellipses.
- [Entry](entry.png), [scramble/reveal](reveal.png), [exit](exit.png).

These six 1920 × 1080 captures use the production renderer and prepared fonts in
an application-owned hidden OpenGL window over the existing
[emporium capture](../shadowtart/front.png). They verify UI rendering, not a new
gameplay/listening session. No desktop input, focus or window manipulation was
used. Pixel comparison confirms top-left placement, viewport bounds and long-title
containment. Superseded bottom-left previews, old-size references and obsolete
settings evidence were pruned; this cleanup does not change historical acceptance.
