# Playthrough corrections — 2026-11-09

Implemented and mechanically verified; interactive user review pending. These six
1920×1080 application-owned exports use shipped artwork and production UI drawing.
No user saves or OS input were used. These corrections preserve save compatibility.

- [Unseen upstairs](map-unseen-stairs.png): the party has never faced these stairs,
  but room revelation includes their icon. Closed-room isolation and the other
  fixture types are covered by the owned CPU regression.
- [Belt equipped](belt-equipped.png): Belt of Vigor selected in the fighter's belt
  card after a tracked physical drag; Unequip is available.
- [Belt removed](belt-removed.png): the same belt dragged into its original bag
  cell; its HP-cap bonus has been removed by the ordinary transaction.
- [Earned level while idle](level-idle.png): all four companions have earned a
  level and may advance safely.
- [Earned level while turning](level-moving.png): actual advancement is blocked;
  all four reminder regions remain pixel-identical to the idle capture.
- [Session-only buyback tooltip](trade-buyback.png): the sold dagger has its
  marked-up buyback price and the explicit trading-session lifetime.

The map/inventory/portrait runner uses a small isolated room and an intentionally
blank prepared world viewport to focus on UI behavior; the merchant runner uses
the production smithy. Both windows stay hidden and all input is in process.
Allocation, GPU residency/OpenGL lifecycle and buyback interaction checks pass;
driver/C allocations are excluded. CPU tests cover every close/rebind/destruction
path and reopening, including old saved buyback goods. See the
[design verification](../../map_revision.md#playthrough-correction-verification--2026-11-09)
for commands and limits. Raw captures and diagnostics are cleaned; only these six
selected images remain. Acceptance has not been inferred from mechanical checks.
