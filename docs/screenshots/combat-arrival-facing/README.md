# Face the party on arrival — 2026-11-04

The existing hidden four-goblin positioning diagnostic was repeated after the
arrival-facing correction. Both flankers now face the party as their movement
finishes, before either starts its next attack. Travel-facing is preserved during
the walk. The two native arrival frames were inspected:

- [Left flanker arrived, before its first attack](flank-2-arrived.png)
- [Right flanker arrived, before its first attack](flank-3-arrived.png)

The other four PNGs retain the initial, walking and later engagement frames.
Owned mechanics use no-damage/high-HP goblins and placeholder party identities,
with production Underkeep/goblin artwork. Hidden-window, frontal attack, final
four-body picking, GL error and GPU-resource release checks pass on RTX 3060/
OpenGL 3.3. No campaign load, OS input or focus automation; interactive review
remains user-owned.

Reproduce in a new or empty output directory:

```sh
nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests \
  cmake-build-release/assets NEW_OUTPUT --combat-positioning-review
```

`manifest.json` records the executed runner and PNG SHA256 identities.
See [the design and focused test results](../../combat_positioning.md#arrival-facing-follow-up--2026-11-04).
