# Slice 12.2: character builds, library and portraits

Status: implemented, **accepted and closed** (2026-10-29), by explicit user acceptance.
[Verification and next-agent handoff](slice122_verification.md). Slice 12.1 is
accepted; 12.3 is not started. Normal startup remains the existing dungeon route.

## Responsibilities and ownership

Pure rules own bounded creation drafts, point buy, loadout choices, spell grants,
portrait metadata, validation and derived previews. JSON/filesystem code owns
library encoding and checked disk transactions. Library records are reusable
level-one blueprints; Party and GameSession own independent live copies. Catalog
references remain valid for the catalog lifetime; persisted records use stable
content IDs and separate UUID saved-character identities, never runtime handles.

A shared spell-access query supplies grant source, casting ability, mana cost,
attack and save statistics to both UI and authoritative commands. Class cantrips,
learned/prepared spells, Life domain grants and racial cantrips remain distinct.
High Elf racial magic uses Intelligence without adding mana to martial classes.
Inventory initialization and previews reuse character derivation and checked
inventory rules. New explicit-party constructors retain the preset convenience
route. Three camp kits are assigned once to fresh parties, first fitting member
in formation order, in a staged transaction. Blueprints contain no camp kits.

## Rules and bounded storage

Names: 1–64 printable ASCII bytes, at least one nonspace. Draft storage is fixed.
Point buy: 27 points exactly, bases 8–15, costs 0/1/2/3/4/5/7/9; current racial
bonuses apply separately. Class recommendations use existing preset arrays.
Fighter Defense and Cleric Life Domain are the supported fixed choices.
Wizard/Cleric choose three class cantrips. Wizard learns six first-level spells;
ordinary preparation is 1..max(1, level + INT/WIS modifier). Cleric Bless/Cure
Wounds are granted outside that allowance. High Elf chooses one distinct wizard
cantrip. Spell storage is bounded by 32 catalog entries; startup gear by 16.

Gear options (independent choices within each class): Fighter longsword/shortsword,
chain/leather, shield/shortbow+20 arrows; Cleric mace/quarterstaff, scale/chain,
shield/shortbow+20 arrows; Rogue shortsword/dagger with leather and bow+20 arrows;
Wizard dagger/quarterstaff. Everyone receives two healing potions; casters also
receive two mana potions. Cleric includes holy symbol; Wizard includes spellbook
and arcane focus. Default gear matches existing presets. Quarterstaff is 4 lb,
1d6 bludgeoning, versatile 1d8. Existing hand/proficiency/weight rules apply.

Brunna: Sacred Flame, Spare the Dying, Guidance; ordinary Bane, Shield of Faith,
Guiding Bolt, Inflict Wounds; domain Bless/Cure Wounds. Lethiel: class Fire Bolt,
Acid Splash, Ray of Frost; racial Shocking Grasp; learn Mage Armor, Magic Missile,
Burning Hands, Thunderwave, Sleep, Grease; prepare first four. Other High Elves
recommend Fire Bolt, skipping class duplicates. No unrestricted casting fallback.
Catalog schema 8 authors explicit grants/portraits; older files normalize bounded
recommendations or report missing content. Test/capture scenarios select explicit
spells rather than gaining a production bypass.

## Library transaction contract

Version 1; 128 records; 1 MiB input; depth 16. Reject duplicate keys/IDs, unknown
fields/references, invalid builds and unsupported versions. UUIDv4 platform entropy
uses eight collision attempts. Monotonic revisions reject stale writes/overflow.
Default path: XDG_DATA_HOME/blobber/characters.json, else
HOME/.local/share/blobber/characters.json; explicit override supported.
Only a missing library with no prior artifacts seeds four presets. Empty libraries
remain empty. Corrupt/unsupported/interrupted files block mutations and retain data;
backup recovery is explicit/manual, never automatic reseeding.
Nonblocking writer lock, same-directory temporary file, flush, previous-good .bak,
atomic primary replacement. Publish memory after replacement; distinguish a
pre-commit failure from committed-but-durability-uncertain directory flush failure.
Disk operations may allocate; steady-state queries/drafts may not.

## Portrait resources

Preserve four existing masters; imagegen adds four per race, with saved provenance.
Five per race span fighter, mage, cleric, rogue and adventurer clothing with varied
adult faces. Stable IDs carry through every portrait consumer. Invalid build IDs
reject; invalid presentation handles use neutral fallback; missing files fail load.
Offline small 216x252 and large 432x648 crop derivatives occupy padded 2048x1024
and 2048x4096 atlases (40 MiB RGBA). Workspace limit 96 MiB.
Preparation writes one atlas at a time as lossless QOI (bounded worst-case encoder
output: 5 bytes/pixel plus header), avoiding PNG export scratch growth. Masters
remain PNG; input headers restrict dimensions to 2048 square, 8-bit RGB/RGBA
and files to 8 MiB before decode. Prepared immutable
atlases use existing RAII GPU ownership; no per-frame decode/resize/filesystem work.

## Validation and completion

Test all 16 race/class pairs and gear choices; point-buy boundaries; count/subset/
domain/duplicate grants; INT vs WIS racial casts; forged commands; independent party
copies; formation portraits; exactly-once three-kit launch. File fixtures own temp
directories and cover first run/empty, round trips, repeated names, corruption,
versions/capacity, stale writers and injected replacement/entropy failures.
Extend allocation tests; run Debug/Release full suites, focused sanitizers, hidden
GPU checks and deterministic native/reduced/letterboxed captures. Inspect all art
and crops. No OS input automation. Deliver headless diagnostic and durable handoff;
the implementation stopped Ready for review. The user accepted slice 12.2 on
2026-10-29; 12.3 is next eligible for planning and remains unstarted.

## Implemented interfaces and integration notes

`rules/creation.hpp` owns `CharacterDraft`, `BuildPreview`, `recommendedDraft`,
`previewBuild`, `finishBuild`, `draftFromBuild`, portrait lookup and `spellAccess`.
Drafts store a 65-byte name, typed catalog/portrait indices, six base scores,
feature flags, five 32-bit spell masks and three gear-choice indices. They own no
heap storage. Previews use a borrowed `CharacterIdentityView` and the existing
`deriveCharacter`/loadout validation; errors are a bit mask plus a derivation error.
Only `finishBuild` and explicit loading/construction copy strings. Recommendations
leave name entry to the caller. Invalid/incomplete drafts cannot become builds.

`CreationClass` records live in Catalog as contiguous vectors with bounded inline
groups/options/grants. Each option has a stable persisted ID. Catalog validation
rejects duplicates, invalid item/slot/count references and blueprint camp supplies.
Schema 8 requires creation metadata for every class and legal curated presets.
Schemas 1–7 keep their original mechanical data but normalize caster grants to
legal bounded choices, or reject incomplete spell content. Direct C++ simulation
fixtures without creation metadata can describe non-creation spell counts; they
still supply explicit eligible masks. This is not a JSON/library fallback.

`CharacterLibrary` owns a fixed `LibrarySnapshot` and borrows an immutable Catalog.
`initialize/load/create/replace/erase` return `LibraryResult`; all mutations require
the expected revision. `startingParty` accepts four distinct UUIDs and a formation
permutation and returns owning starting-build copies. `committed` is authoritative
when a directory flush fails after rename; do not repeat the original save merely
because `error` is nonzero. Saved identities never reuse a mutable library index.
The POSIX backend uses `/dev/urandom`, `flock`, `fsync`, and same-directory `rename`;
porting to a non-POSIX platform requires an equivalent storage backend.

The explicit `Party(Catalog, PresetParty)` and `GameSession(Map, Catalog,
PresetParty, ...)` overloads copy four builds and validate formation/identity,
curated loadouts and grants. Rvalue Catalog/Map borrowers are deleted. Formation
selects only display/placement, never portrait identity. Three camp kits are
assigned at launch by checked acquisitions; their weight is reflected in live
stats, whereas draft previews show only blueprint gear. A launch that cannot place
all supplies fails without publishing a session.

The picker has 38 fixed entries (32 catalog spells plus six feature slots), with
one shared access query driving picker entries, pending-cast statistics, and
command validation/resolution. Racial cantrips coexist with martial features.
Sheet summaries label class INT/WIS separately from racial INT; martial racial
magic does not expose a mana bar.

`preparePortraits` and `blobber_bake_portraits` prepare both QOI atlases and a TSV
manifest. CMake runs the offline baker and copies masters/provenance unchanged.
Source bounds are checked before decoding; runtime checks QOI header dimensions,
channels and encoded size before upload. Two immutable GPU handles reuse the
existing eight-texture registry and RAII lifetime; portraits add 40 MiB to UI
residency independently of scene/NPC preparation budgets. No portrait filesystem,
decode, crop, or resize work runs in a frame. Final production bake peak RSS was
63,684 KiB (about 62 MiB); worst-case QOI encoder plus largest atlas is 72 MiB.
