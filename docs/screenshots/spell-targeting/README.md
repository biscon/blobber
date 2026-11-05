# Spell targeting corrections

Status: **Implemented; Ready for review (2026-11-06)**.
See [rules, controls and focused checks](../../spells_design.md#targeting-correction-verification--2026-11-06).

- [Paired motion preview](spell-targeting.gif): Acid Splash across a cell boundary
  and Shocking Grasp with Lethiel staying in the back row.
- [Acid Splash, native still](acid-boundary.png): two actual selected recipients;
  one saves, one takes damage, at 240ms after casting.
- [Rear-row Shocking Grasp, native still](rear-row-grasp.png): unchanged 200ms impact,
  shown at 300ms; front-row companions remain in their original positions.

These hidden application captures use production rules/art, seed 5 and accepted
in-process spell commands. The Acid Splash recipients are half a cell apart across
a boundary; it no longer requires identical cell IDs. Separate headless UI tests
cover both selection orders and one-click Spiritual Weapon/Shocking Grasp controls,
including floor-only movement and repeated-input rejection. The captures do not
simulate OS mouse or keyboard input.

The GIF compares reduced world crops sampled every 60ms through 1320ms, then holds
600ms before looping. Labels are review annotations; PNGs preserve exact 1920×1080
application output. Three files are retained and intermediate captures/logs were
cleaned. Interactive feel and cross-device appearance remain for user review.

After building `blobber` and `blobber_world_integration_gpu_tests`, create an owned
run with `python3 tools/agent_artifacts.py create`, then run each case sequentially:

```sh
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/acid --se02-review acid-boundary
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/grasp --se02-review shocking-grasp
```

Each output directory must be new or empty. Clean the run after selecting evidence.
