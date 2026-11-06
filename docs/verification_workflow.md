# Verification workflow and resource limits

This policy supersedes historical slice recipes that list four full build/test
configurations, long allocation loops, repeated captures or benchmarks. Those
recipes describe available coverage, not a checklist for every edit.

## Choose checks by the change

| Change | Development check | Broader check only when relevant |
| --- | --- | --- |
| World rules, movement, sight | Affected CPU test targets in one build | Headless protocol tests if observation/admission changes |
| Save fields or restore | `campaign_save_smoke` | Full `campaign_save` for persistence/progression changes before handoff |
| World rendering | World CPU/environment tests; one hidden GPU integration test | Selected application-owned captures for changed appearance |
| Asset publication/build rules | Generated publication fixtures | Production bake once when required, not each test invocation |
| Performance or capacity regression | Short, named experiment answering a concrete question | Stress/benchmark only if representative checks cannot answer it |
| Comments, documentation, test selection | Review/diff/configuration checks | No gameplay matrix or benchmark |

Use one configuration during iteration. Release is useful for checking expensive
serialization/rendering code; Debug/sanitizers are useful for a specific memory or
assertion concern. Running both is a choice requiring a concrete reason, not the
next automatic step. An integration checkpoint may justify broader coverage once.
After a localized follow-up, rerun the affected checks, not the entire matrix.

## Commands and classification

Build presets cap workers at two. Test presets run one process and exclude `slow`,
`stress` and `benchmark` labels. `campaign_save_smoke` retains the new environment
roundtrip, ordinary campaign roundtrip/continuation, atomic save/terminal behavior
and save UI checks. Full campaign persistence/effect/progression coverage remains
in `campaign_save`, labelled slow. CPU animation tests use timing metadata directly. World visibility/effects allocation guards use small owned fixtures. The old
perspective-bank renderer, tests and bakers are retired in R06.

Examples (substitute the affected target/test; do not run every example):

```sh
nice -n 15 ionice -c 3 cmake --build --preset headless-release --target blobber_world_environment_tests
nice -n 15 ionice -c 3 ctest --preset headless-release -R '^world_environment$'
nice -n 15 ionice -c 3 ctest --preset headless-release -R '^campaign_save_smoke$'
# Explicit extended persistence validation, when that behavior changed:
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-headless-release -R '^campaign_save$' -j 1 --output-on-failure
```

Only one intensive command may run at a time, across all configurations and tools.
The worker cap is global, not two workers per concurrent build. Keep internal
library threads bounded. For large fixtures set TMPDIR to a disk-backed scratch
directory with free space; `/tmp` on this machine is RAM-backed. Test fixtures must
honor the selected temporary directory and clean up their own files. Never delete
unrelated scratch data. Do not run benchmarks in default CTest or repeat them to
obtain a reassuring number.

## Artifact housekeeping

Each task owns a disposable run directory. From the repository root:

```sh
python3 tools/agent_artifacts.py create
# Pass the printed absolute run path to capture/review tools as their output root.
python3 tools/agent_artifacts.py clean RUN_DIRECTORY
python3 tools/agent_artifacts.py clean RUN_DIRECTORY --apply
```

Creation prints a unique marked directory under ignored `output/agent-runs/`.
Cleanup defaults to a preview of file count/bytes and only accepts one marked direct
child. It refuses symlinks, special files and tracked content, and fails closed if
Git inspection fails. Repeat cleanup of an absent run is a no-op. Close its runners
before cleanup; the helper is housekeeping for owned output, not a concurrent writer
or a hostile-filesystem sandbox. It never sweeps other runs or ages out evidence.

Generate into the run, then promote selected images to the existing feature gallery:
at most six, or twelve awaiting user review. Explain exceptions in the README using
the distinct cases that need review. Replace superseded images and repair links;
keep test results and limitations in the existing relevant design/handoff. Do not
create another report for each run. Archive completed historical verification in
[the indexed archive](verification/archive/README.md). Preserve tool inputs, source
artwork, references and user saves; raw captures, copied binaries, trace bodies and
machine logs are disposable. A retained unresolved-failure diagnostic needs
reproduction instructions and an explicit cleanup condition. Finish by cleaning the
run and reporting cleanup/any exception in the final response, including after a
failed check. Do not remove unrelated scratch, build caches or Git history.

For changes to the housekeeping helper, use isolated fixture checks only:

```sh
python3 tests/agent_artifacts_tests.py
```

Historical output paths and broad retention promises in archived reports describe
the original checkpoint. The current retention policy supersedes them. Pruned exact
replay bodies and pinned historical binaries cannot be replayed locally; recorded
hashes/outcomes are provenance, not a promise that the artifacts remain available.

## Why some checks cost more

Full campaign persistence checks repeatedly encode/decode four locations, fixed
actor/loot/effect slots, navigation workspaces and RNG state for many effects and
failure cases. This is correctness coverage, not a frame-rate benchmark. Splitting
out a smaller smoke path avoids repeating the exhaustive matrix during routine work.
Further serialization/fixture optimizations should target a measured bottleneck;
do not change persistence semantics or weaken boundary tests merely to reduce time.
