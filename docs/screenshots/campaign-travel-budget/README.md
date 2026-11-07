# Campaign travel budget correction

Mechanically verified 2026-11-07; interactive user review remains pending.
The user requested a 1 GiB world-rendering budget. These two hidden,
application-owned captures use a read-only copy of the reported Sanctuary save:

- [Entered dungeon](entered-dungeon.png): extra forward input stays blocked, then
  explicit stair travel succeeds with the source and destination assets staged.
- [Returned to Sanctuary](returned-city.png): ascent commits and releases the
  departed dungeon artwork.

Cancellation also preserves the source campaign/image/resources. No OS input,
window selection or desktop focus control was used. The original save is unchanged.
The initial sandbox could not access the display socket; the hidden checks ran
with display access. See [design and verification](../../sc011_design.md#campaign-travel-residency-correction--2026-11-07).
Only these two selected captures are retained; disposable logs, probes and copied
save data are removed after verification. This evidence does not imply user acceptance.
