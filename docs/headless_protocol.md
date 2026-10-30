# Headless protocol v1

Slice 13.1 implements a local, synchronous JSON-lines transport to the real arena
simulation. See [design](headless_design.md) and [verification](slice131_verification.md).
MCP, autonomous policies and batch experiments belong to later slices.

## Build and launch

```sh
cmake --preset headless-debug
cmake --build --preset headless-debug -j 6
ctest --preset headless-debug
cmake-build-headless-debug/blobber_headless --assets assets --mode diagnostic
```

The graphics-disabled presets build without discovering/downloading raylib, decoding
images or copying/baking game assets. They still need catalog JSON and NPC definition
JSON at runtime. Image references in those definitions need not exist. C++17, CMake,
Ninja and nlohmann/json are required; tests also use Python 3. System JSON is preferred
with the existing pinned fallback. No new runtime package is required.

`--mode visual` is available in the normal graphics build. Pass its prepared asset
directory, e.g. `--assets cmake-build-debug/assets`. It loads scene/NPC/loot banks and
uses the ordinary CPU renderer and body picking. It creates no window, context,
fonts or presentation surface; DISPLAY is unnecessary. Missing/incompatible banks
are errors. Diagnostic mode uses core validation without body-visibility filtering.

## Transport and identities

Each input line is one UTF-8 JSON object. Every request contains `version:1`, a string
`id` (at most 64 bytes), and `op`. Output consists of `type:"events"` chunks followed
by exactly one `type:"response"` with that ID. A response contains `ok` and either
`data` or `error:{code,message}`. An action can be a well-formed request (`ok:true`)
whose gameplay result is `accepted:false`; inspect both fields.

`session` and `revision` are **decimal strings**, including values that fit JS numbers.
Use the latest observation's values on start replacement, reset, preview, act and
advance. Actor indices and character indices belong to that session; inventory/loot
handles are `{slot:<integer>,generation:<decimal string>}`. Reset changes the session
identity. Stale requests/handles reject. Observations, discovery and preview neither
advance time nor consume revision. A well-formed act consumes a revision even when
rejected; advance consumes one even if it advances zero ticks. Schema failures and
stale requests have no simulation effect or revision cost.

Unknown object fields, duplicate keys, invalid enums/types, out-of-range integers
and unsupported versions reject. Invalid JSON is recoverable (`invalid_json`, null
ID); invalid requests are `invalid_request`. Startup/I/O failures and overlong input
lines produce `type:"fatal"`, a message on stderr and exit status 1. All stdout from
the runner is protocol JSON. Do not mix console text into stdin.

## Operations

| op | Additional request fields | Result |
| --- | --- | --- |
| `start` | `scenario` (existing arena schema-1 document), optional uint32 `seed`; session/revision when replacing | New observation; transactional replacement |
| `reset` | session/revision, optional `seed` | Reconstruct current initial scenario; new observation |
| `observe` | none | Current observation for selected mode |
| `inspect` | none | Full diagnostic observation; permanently marks inspection use in this run |
| `actions` | none | Bounded descriptors, candidate actor targets and eligibility reasons |
| `preview` | session/revision, `action` | `{accepted,reason}` with no costs or time |
| `act` | session/revision, `action` | `{accepted,reason,observation}` |
| `advance` | session/revision, `ticks` (0–1000), optional `until_decision` (default false) | `{ticks,stop,observation}`; ticks is actual progress |
| `close` | none | `{closed:true}`, finish recording and exit |

Scenario documents use the existing [arena format](arena_design.md). Four distinct,
valid level-one builds and 1–256 monsters are required. Formation is their initial
order. There are no teleport, damage, XP-grant, arbitrary-state or custom-map verbs.
Changes to party composition go in a new scenario. Earned advancement uses normal
safe-exploration checks. Full arena victory/defeat is terminal until reset; a local
victory with remaining monsters pauses until the `continue` action.

Each tick is 10 ms of the normal simulation with movement duration enabled, without
sleeping. Time freezes between requests. `until_decision:true` stops at an awaiting
party action, settled real-time phase or encounter result. At settled exploration it
can return zero immediately: send a move or use `until_decision:false` to wait. While
awaiting a party action, exact tick requests can advance visual time, but do not
invent an action or skip the turn. End Turn is an explicit feature.

`stop` is `ticks`, `decision`, `encounter_result`, `terminal`, `tick_limit` or
`mutation_limit`. Runs allow 360,000 ticks and 100,000 act/advance requests, reset on
start/reset. Reaching the mutation limit returns zero progress/accepted false without
changing revision. An act at that limit omits observation; use observe. No events are
silently discarded. Event chunks contain at most 256 events. Input lines are bounded
at 2 MiB and JSON nesting at 32. Closing/EOF ends a recording even if play is unfinished.

## Action objects

All actions have `kind`. Fields in brackets below are optional. Character indices
are 0–3. Spell names are stable catalog IDs; use `actions` to discover them. Preview
fully specified actions: descriptor eligibility without a required recipient may
correctly report a missing target. Discovery does not enumerate the Cartesian product
of spell recipients, inventory destinations or preparation choices.

| kind | Fields |
| --- | --- |
| `move` | `direction`: forward, back, left, right, turn_left, turn_right |
| `combat` | `character`, `action`, fields described below |
| `inventory` | `operation`: move/split/transfer, `item` handle, destination `character`, [`cell`], [`quantity`] |
| `drop` | `item` handle; drops whole stack at current cell |
| `pickup` | `loot` handle, recipient `character`; checked whole-stack acquisition |
| `rest` | `type`: short/long, [`dice`: four counts 0–3], [`prepared`: four spell-ID arrays] |
| `advancement` | `character`, [`learned`: new spell IDs], [`prepared`: newly prepared spell IDs] |
| `continue` | no extra fields |

Inventory cells 0–47 are backpack slots; 48 body, 49 main hand, 50 off hand, 51 ranged.
Use move to equip/unequip or rearrange, split with quantity, transfer with a recipient
and optional cell (omitted means automatic destination). There is no Take All macro;
repeat checked pickups. Rest preparation arrays are complete desired selections;
advancement arrays are **additions** to the retained build. Wizard advancement requires
its ordinary new learned spells. Discovery gives learn bounds and initial new
preparation choices; learning can unlock more candidates, checked by preview.

Combat `action` is melee, back_melee, defend, ranged, spell, item, aid or feature.
Optional fields are:

- `target`: actor index for weapon attacks; `ally`: character index for Aid/item use.
- `spell`: stable ID. `enemies` or `allies`: up to three recipient indices (exclusive).
  `darts`: matching counts for Magic Missile or Scorching Ray. `cell:[x,y]` for ground
  targeting. `restore_paralysis:true` selects Lesser Restoration's paralysis mode.
- `feature`: second_wind, arcane_recovery, dismiss_concentration, action_surge,
  preserve_life, cunning_action, spiritual_weapon, escape_web, end_turn.
  Features with recipients/cells use the same spell/recipient fields as normal play.
- `item`: generation-checked inventory handle for consumable use.

The runner fills internal turn/world/inventory tokens from the checked observation
revision. The simulation still validates ownership, targets, conditions, range,
resources, timing and costs. Movement during a transition rejects, with no queue.

Example action payloads (wrap in an act/preview request with tokens):

```json
{"kind":"combat","action":"spell","character":3,"spell":"magic_missile","enemies":[0,1],"darts":[2,1]}
{"kind":"combat","action":"feature","character":0,"feature":"end_turn"}
{"kind":"inventory","operation":"move","item":{"slot":7,"generation":"1"},"character":0,"cell":49}
{"kind":"rest","type":"short","dice":[1,0,1,0]}
```

## Observation and event information

Observations include mode, session/revision, ticks/mutations, simulation/gameplay
clocks, phase, decision/terminal flags, party resources/builds/inventory/statuses,
formation, active character, position/heading, current target indices and loot.
Heading is north=0, east=1, south=2, west=3. Time/serial fields that can grow to 64 bits
are strings. Some fixed simulation enums (life, effect clock/kind) are numeric v1
values from the corresponding C++ enums; clients should primarily use provided names
and eligibility results. Party-created ground zones include known placement/duration.

Visual observations include only currently body-pickable living enemies, with the
health-bar fraction and visible statuses. They omit exact enemy HP, AI awareness,
pending attack results and internal state. Picking is suspended during movement;
empty targets during that interval do not mean enemies disappeared. Corpse selection
is unavailable, just as in the UI. Loot locations follow rendered picking or the
current cell; contents appear only when the normal loot interaction can open them.
Resolved combat events follow the normal combat log's information, including attacks
from offscreen; pre-impact roll/damage events are withheld. This is a structured play
interface, not a screenshot API or a human reaction-time simulation.

Diagnostic observations include full actors, exact HP, AI state, pending attacks,
effects and revisions. `inspect` explicitly reveals this in visual mode and sets
`diagnostic_inspection_used`; subsequent observe remains filtered. Trace files always
contain diagnostic records, even for visual play. Keep them away from a player policy
whose experiment promises player-only observations.

## Client and replay recipe

Save a scenario from the Arena UI or use an existing exported schema-1 scenario.
The example client starts it and fills protocol IDs/tokens automatically. A script
contains one object per line, for example:

```json
{"op":"observe"}
{"op":"actions"}
{"op":"act","action":{"kind":"move","direction":"forward"}}
{"op":"advance","ticks":1000,"until_decision":true}
{"op":"close"}
```

```sh
python3 tools/headless_client.py --scenario /tmp/scenario.json \
  --script /tmp/actions.jsonl --record /tmp/arena-run.jsonl
cmake-build-headless-debug/blobber_headless --assets assets --mode diagnostic \
  --replay /tmp/arena-run.jsonl
```

Use a new trace path; existing files are never overwritten. `--record` and `--replay`
are exclusive. The client can also be imported as a context-managed `Client` with
`request(op, **fields)` and an optional event callback. It supplies no battle policy.
The fixture-owned `tests/headless_cli_tests.py RUNNER` demonstrates a complete scripted
fight and fresh-process replay without relying on shipped game content.

Trace records stream header, request, full events, response and normalized boundary
state, then a completion footer. The header fingerprints the executable, catalog,
NPC definitions and (visual mode) prepared banks/profile. Rebuilding the executable
or changing content can invalidate old traces, including changes irrelevant to a
particular encounter. These are strict reproduction records, not save games or a
cross-version format. Successful replay prints `{"replay":"matched"}` and exits 0;
mismatch reports operation and first JSON field path, exits 1. Missing footer,
truncation, incompatible identity and trailing data reject.

The entire trace file, across resets, is bounded at 256 MiB. Write failure stops the
runner; incomplete recordings cannot report a successful replay. Event batches are
consumed before the next simulation operation. C++ simulation/typed observation work
is allocation-guarded separately from JSON construction, encoding, transport and
trace I/O, which may allocate. No production throughput claim follows from those
allocation checks.
