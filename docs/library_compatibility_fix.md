# Character compatibility and inventory tab follow-up

Accepted with PC07.2, 2026-11-01: “slice accepted, record it in the plan doc”.
Implemented at the user's request after an incompatible
starting-gear record disabled the Character Creator.

CharacterLibrary now loads each compatible record independently. Incompatible
portraits, builds, gear and duplicate identities are skipped. An invalid/unsupported
library envelope opens as an empty usable library. Strict `decodeLibrary` remains
the default validation API; an optional skipped-count parameter selects record
filtering for disk admission. Parsing/retained raw bytes stay within the existing
1 MiB file limit and 128-character capacity, outside frame processing.

Loading preserves disk bytes. The next explicit character save backs up the old
file and atomically publishes accepted records plus the new draft. Exact observed
bytes are compared under the existing writer lock, so a filtered or empty snapshot
cannot overwrite a concurrent edit. Actual read/write failures remain reported.
A lock file alone no longer blocks first-run initialization; backup/partial-write
recovery protection stays in place. The user explicitly requested deleting the old
characters: removed the six-record `characters.json` and its five-record `.bak`
from the active user-data directory, under the character writer lock. Campaign
files and other user data were left intact.

Campaign inspection already makes corrupt/incompatible saves unavailable to Continue
while New Game remains usable. This follow-up adds regression coverage for creating,
saving and loading a new campaign over such a slot after explicit replacement
confirmation. No migration, save schema change or relaxed simulation validation.

Inventory/Character page tabs now paint their own pressed state before their labels:
subtle green fill, teal label and underline; the selected page keeps its gold
indicator. Party tabs use the same fill/underline without dimming portraits/text.
The final generic button overlay skips these tabs. Mouse release/cancellation
semantics and other button painting are unchanged. See the [two held-state captures](screenshots/ui-compatibility/README.md).

Release and Debug applications built successfully. Verification ran serially at low
priority with one build worker/test process:

```sh
nice -n 15 ionice -c 3 cmake --build --preset release -j1 --target blobber blobber_library_tests blobber_menu_tests blobber_creator_tests blobber_campaign_save_tests
nice -n 15 ionice -c 3 cmake --build --preset debug -j1 --target blobber
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-release -R '^(library|menu|creator|campaign_save_smoke)$' -j1 --output-on-failure
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-release -R '^(buttons|inventory)$' -j1 --output-on-failure
nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests cmake-build-release/assets output/library-compatibility/captures --pc072-controls
```

The compatibility/caller selection passed **4/4 in 1.18 s**; tab/input checks passed
**2/2 in 0.01 s**. Owned cases include mixed/all-incompatible records, invalid JSON,
unsupported versions, duplicate identities, save/reload, backup retention, stale
writer rejection and the bounded read failure. Hidden capture checks passed,
including release/cancel assertions for both page tabs, twelve composed frames
with zero engine C++ allocations or new GPU textures/targets, and no GL errors.
Both tab captures were visually inspected. Local logs are in
`output/library-compatibility/`. No OS input/focus automation or balance study ran.
