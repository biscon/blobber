# Dungeon flow and visual polish

Status: implemented and verified, 2026-10-31. Scoped follow-up to the implemented dungeon art revision.

Loot retains one billboard per occupied loot cell, now two-thirds of the former
projected size (including near/far clamps). The existing floor-projected soft blob
shadow path uses a radius proportional to the smaller bag. Shadows remain visual,
without depth writes or picking IDs; CPU picking uses the smaller actual billboard.
No new frame storage or uploads are needed.

Guardian gates retain their objective-controlled mechanics; the guardian-room
approach door retains its ordinary/keyed interaction. Both receive the sigil. Each asset pack gains
an optional transparent `guardianEmblem` texture binding; the default dungeon pack
uses a generated iron/brass skull-and-crossed-swords medallion. Door boxes retain
that resolved binding for the objective approach door and guardian gate. The session exposes the
immutable approach fixture index through FrameInput; visibility caches it with
its existing revision stamp. The GPU draws the emblem on both
broad faces using the leaf's current transform, above its horizontal strap. It
moves with the leaf, depth-tests against the world, and leaves the solid leaf's
picking unchanged. The art can be reused by other packs without replacing their
door texture. Missing optional bindings preserve standalone test publications.

The application consumes ordinary local Victory automatically after recording
combat events and settling accepted presentation tails. Defeat, campaign completion
and a completed arena keep their terminal navigation/rematch controls. The HUD only
draws result panels for those terminal cases. Combat simulation's explicit Victory
state and headless command contract remain available; the app's small shared result
flow function supplies automatic continuation without an input acknowledgement.
Survivors stabilize as before; no healing or resource restoration occurs.

Hub travel builds its existing bounded row list and previews the selected request.
A single valid hub destination immediately emits the same typed travel request,
including an unlocked waypoint after a return. Multiple entries retain the selector;
recall always retains its cost confirmation. Both paths still use the fade and
transactional travel preparation/commit sequence.

Terminal save handling extracts the run ID using the bounded, duplicate-key-checked
JSON parser, independently of schema/content admission. It only replaces a slot
owned by the ending run. An incompatible save belonging to a different run is
left unchanged and does not produce a write-failure warning. Matching saved runs
still get durable terminal markers; actual read/write failures retain fail-closed
in-process denial and retry. This addresses an unsaved run ending while an older
incompatible save occupies the slot, without migration or deleting the user's data.

Validation: focused campaign command/UI tests for single vs multiple entries and
recall, campaign save regressions for unsaved endings with missing/current/old slots,
matching markers and I/O failures, CPU geometry/picking and allocation tests for bags
and guardian bindings, plus hidden CPU/GPU integration and application captures.
No OS input automation, stress tests, or additional build configurations.

## Verification

Release only, at most two build workers and one test process, low CPU/I/O priority.
The focused CTest selection `campaign`, `campaign_save`, `dungeon_refresh`,
`world_visibility`, `world_integration`, and `world_visual_cli` all passed (4.37 s).
It covers single fresh/visited destinations, multiple-entry selection, recall
confirmation, automatic ordinary victory with health preserved, terminal navigation,
missing/old/unrelated slots, matching terminal markers and retained I/O failure
behavior. The new parser regression uses owned pre-refresh schema/simulation 1,
generator/templates 3 data; the user's actual slot is never written by verification.
Loot picking/allocation and all-heading emblem face placement/pack validation pass.

`blobber_world_integration_gpu_tests` passed in a hidden application-owned
1920×1080 window on the available NVIDIA GPU. Existing fixture door/effect/travel,
resource lifetime and allocation checks passed with zero CPU/GPU ID differences.
The new production-art capture path also requires exact full-pixel CPU/GPU picking
agreement and no GL errors for every captured bag/door/hub view, including the moving
emblem. Native screenshots were inspected directly. No desktop input or window
selection was used; manual interactive play remains for the user.

The self-owned small-image world-baker test passed in single-config mode, including
optional-emblem publication, source/manifest rebuilds and failure retention. The app,
headless tool and fixture executables build successfully. No additional build/test
matrix, sanitizer or stress workload was run. Visibility identity advances to
`world-visibility-v5` for the changed loot coverage; save schema/simulation remain 3/3
and publication format remains 2 (the emblem binding is optional).

Generated with the built-in ImageGen tool:
[guardian emblem source](../assets/packs/dungeon/source/guardian-emblem.png) and
[exact prompt](../assets/packs/dungeon/guardian-emblem.prompt.json). The source alpha
is preserved; the baker prepares a 256px transparent decal/mip chain. The default
pack publishes 127 textures; this adds 349,524 mip bytes and 65,536 alpha bytes.

Review captures:

- [Guardian entrance](screenshots/flow-polish/guardian-entrance.png)
- [Opposite side](screenshots/flow-polish/guardian-entrance-back.png)
- [Opening](screenshots/flow-polish/guardian-entrance-opening.png)
- [Fully open](screenshots/flow-polish/guardian-entrance-open.png)
- [Smaller loot bag and shadow](screenshots/flow-polish/loot-bag-shadow.png)
- [Single-destination hub prompt](screenshots/flow-polish/single-destination-hub.png)


## Contextual viewport strip (2026-10-31)

Removed the persistent location label and its dark viewport fill. The top bar
retains depth information. The bottom strip is empty in idle exploration, appearing
for nearby fixture interactions, sanctuary rest, or terminal guidance. Release
build and the campaign test passed, including idle-to-stair prompt transitions.
The hidden GPU integration passed with zero ID differences; the updated
idle dungeon capture (historical artifact removed) was inspected.
