# Stone & Shadow

A C++17 / raylib 6.2 grid-based dungeon crawler with a four-character party,
discrete movement, procedural expeditions, an outdoor sanctuary hub, arena combat,
spells, class abilities, inventory, travel, and campaign save/Continue.

## Build and run

Requirements: a C++17 compiler, CMake 3.24+, Ninja, and desktop OpenGL 3.3 support.
CMake first looks for an installed raylib **6.2** package. If absent, it downloads
the pinned release through FetchContent and verifies its SHA-256. Subsequent
builds can run offline. Set `BLOBBER_USE_SYSTEM_RAYLIB=OFF` to force the pinned fallback.
NPC JSON uses **nlohmann/json 3.12.0** with the same installed-package preference
and a SHA-256-pinned download fallback (`BLOBBER_USE_SYSTEM_JSON=OFF` forces it).

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./cmake-build-debug/blobber
```

For optimization, substitute `release` for `debug` in all commands. To use a
different generator, configure normally, for example
`cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`, then `cmake --build build`.
Multi-configuration generators also work; select the desired configuration when
building and running tests.

On Debian/Ubuntu, the native build dependencies are typically:

```sh
sudo apt install build-essential cmake ninja-build libx11-dev libxrandr-dev \
    libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

Audio is disabled. On Windows use a configured MSVC or MinGW toolchain; on macOS,
Xcode command-line tools plus CMake and Ninja. Linux/X11 is the verified platform.

### CLion

Open this directory or its `CMakeLists.txt`. Enable the `debug` or `release`
CMake preset, wait for configuration, select the **blobber** executable, and Run.
CLion's ordinary CMake profiles also work. No machine-specific `.idea` files or
working-directory setup is required: assets are copied beside the executable.

## Development checks

Build presets use at most two workers; test presets run serially and skip labelled
slow integration tests. Run affected test targets in one configuration during
iteration. Full persistence tests remain explicitly available. Retired image-bank tests are deleted.
See [verification workflow and resource limits](docs/verification_workflow.md) for
when to run each check; full Debug/Release matrices and benchmarks are not routine.

## Controls

| Key | Action |
| --- | --- |
| W / S | Forward / backward one tile |
| A / D | Strafe left / right one tile |
| Q / E | Turn left / right 90 degrees |
| Tab | Toggle animated / instant steps |
| M | Toggle the discovered map overlay |
| T / compass | Open Travel for checked stairs, waypoint entry or recall |
| F | Interact with a nearby door, key or stair |
| R / pause-menu Rest | Open camp; Left/Right selects Short/Long Rest, Enter starts, Escape closes |
| Floor item hover / click / wheel | Inspect / collect into selected character / page through a crowded cell |
| Monster hover / click | Preview reach and damage / immediately attack a reachable enemy |
| 1 / sword | Select basic melee |
| 2 / shield | Immediately defend |
| 3 / bow | Select ranged attack |
| 4 / spell | Browse paused Spells / Abilities, then resume to select targets |
| I / 5 / backpack | Open the paused fullscreen panel on Inventory for the selected character |
| Portrait i | Open the fullscreen panel on Character for that portrait |
| Inventory / Character top tabs | Switch pages while keeping the viewed character |
| Fullscreen 1–4 / character strip | View another character; choose a recipient during inventory transfer |
| Drag / Shift-drag | Move, swap, merge, equip, or transfer / choose a split quantity on release |
| Inventory E / T / U | Applicable Equip/Unequip, Transfer, and Use Potion actions |
| Inventory arrows / Tab / Enter | Select a cell / switch bag and equipment / confirm selection or split quantity |
| Inventory wheel / Page Up / Page Down | Read further description pages; wheel adjusts a split quantity |
| 6 / aid | Arm Aid; click a dying ally's portrait to commit |
| Portrait click | Select a character in real time, or a friendly spell/potion/Aid target; use i for Character |
| Spellbook Up / Down, Enter | Browse descriptions, then choose a spell or ability |
| Spell targeting Left / Right, Up / Down, Enter | Move cursor, add/toggle or remove recipient, then cast |
| Casting B / Back | Return to the picker at the remembered scroll position |
| Magic Missile left / right / Shift-click | Add one dart / remove one dart |
| Left / Right, Enter | Cycle targets, then execute; Enter also acknowledges outcomes |
| Escape / Backspace | Cancel targeting or close an overlay; unarmed opens pause menu |
| Right-click outside casting | Cancel ordinary targeting or close an overlay |
| Mouse wheel over log / + | Scroll readable history / expand paused history |
| Gear | Open pause menu with Resume and Quit |

The first movement action occurs on keypress; holding a movement key repeats after 300 ms and then
every 180 ms. An animated action lasts 160 ms. At most one fresh keypress is
buffered during an animation. Losing focus clears the buffer and held-key state.
Collisions block the entire move, including its visual effect. HUD commands never
repeat while held. Controls held during focus loss require a release in the focused
window before their next press can act. Spell selections survive focus loss and are revalidated on return; ordinary attack targets clear. Movement clears targeting. Targeting
and attacks wait until transitions finish and the M map is closed. Each action/turn resets to basic melee.

Facing an adjacent goblin cell begins combat and locks movement/turning. Each
character and enemy group rolls initiative once; the initiative strip puts the active participant first and removes resolved turns. Melee, Defend and Aid spend one action. Defend imposes disadvantage until the
next turn; Aid attempts DC 10 Wisdom stabilization of another dying ally.

Front slots protect their paired rear slots on **both sides** (FL/BL and FR/BR).
A rear member can attack or be targeted as soon as its own front slot falls.
An exposed melee goblin moves into that vacant front slot on its next turn, then
attacks once. Party members retain their slots. Body visibility is also
required. Hover explains paired-slot protection and other reach restrictions. The Rogue can melee from behind an ally and gains conditional Sneak Attack; enemy
paired protection still applies. Inflict Wounds uses ordinary exposed melee reach.
Ranged attacks and spells ignore enemy row protection. Potions use the checked inventory controls.

At zero HP, characters make death saves on their turns. Aid stabilizes without
healing. Victory stabilizes surviving downed allies and preserves HP/deaths;
Continue resumes exploration. No conscious party member means defeat; Restart
reconstructs the party and monsters. Map/log browsing and focus loss pause gameplay.
Cleric and Wizard start with four mana; first-level spells cost two and cantrips
cost none. Casting takes one action or the shared six-second cooldown in real time.
Mana and feature charges do not reset after encounters. Safe, stationary exploration
allows camp: a Short Rest takes one gameplay hour and spends the Hit Dice you choose
(class die + Constitution healing), refreshing Second Wind but not mana. A Long Rest
takes eight hours, costs one party camp kit outside an authored sanctuary, and restores
living members' HP, mana, class charges and half their maximum Hit Dice (rounded down,
minimum one). Dead companions stay dead; dying companions must first be stabilized.
The campaign begins at a sanctuary hub; expedition floors require camp kits. Cancel before starting costs nothing; Escape after starting only skips the
animation. Portrait selection is distinct from the active turn.

Loot stays in each monster's death cell after the corpse disappears. Click its loot
bag in your cell or the three reachable front cells to browse everything inside.
Click a stack to take it, hover for stats, or use Take All; use the wheel/page
buttons to browse crowded bags without taking unwanted gear. Click a party portrait
while the dialog is open to change the recipient. Whole stacks that cannot fit stay
in the bag. Take All closes the dialog after collection; Close or Escape also
leaves leftovers in place. Gold weighs 0.02 lb per coin;
camp kits weigh 8 lb each.

The small campfire icon in the top bar, beside Settings, opens camp. It is dimmed and disabled
when no beneficial, legal rest is available. R and the pause-menu Rest entry also
open camp, where unavailable choices explain their requirements.

## Rendering and assets

The GPU world renderer is the sole graphical path, with reusable prepared material
and sprite textures, mip chains and alpha masks. The world renders at 500×282 inside
fixed 1920×1080 presentation; UI art/text draw at native resolution. Supersampling
is on by default; `--supersampling off` selects native filtered quality. No renderer
selector or CPU color fallback remains. Desktop gameplay requires OpenGL 3.3.

Normal builds prepare `assets/prepared/world/{metadata.json,textures.bin,sources.json}`
and native creator/portrait atlases beside the executable. Source artwork and NPC
sheet extraction remain; scene/NPC/loot perspective banks and their bakers are gone.
Incremental builds also remove obsolete banks from managed build asset outputs.
Missing/corrupt world publication or incompatible NPC definitions fail clearly.

Runtime options: `--assets DIRECTORY`, `--campaign-save FILE`, `--size WIDTH HEIGHT`,
`--seed UINT32`, `--development`, `--supersampling on|off`, `--help`. The development
flag retains existing in-game diagnostics; standalone demos, previews, public height
showcase and dedicated screenshot commands have been retired. A future cheat/debug
interface is separate work.

NPC source definitions retain registered four-direction animation and attachment
metadata. Living occupants block cells; hurt/death, prone posture, loot, doors and
raised surfaces share world depth/IDs for interaction. See the [system design](docs/system_design.md)
and [NPC metadata](docs/npc_design.md) for content authoring and ownership.

## Automation and screenshots

`blobber_headless --mode diagnostic|visual` drives the regular session engine through
bounded JSON lines. Visual mode always uses CPU world depth/IDs and needs no display,
raylib context or source PNGs, including graphics-disabled builds; provide the prepared
world publication in its `--assets` directory. Diagnostic mode needs only rules/NPC
JSON. The Python client, local MCP adapter and opt-in arena experiments share this path.
See [protocol](docs/headless_protocol.md), [MCP usage](docs/mcp_usage.md) and
[arena experiments](experiments/arena/README.md).

Replay image export remains available in graphics builds using `--replay FILE`,
`--capture-dir NEW_DIRECTORY` and 1–8 `--capture-operation N` arguments. It creates
its own hidden GPU context and exports the current scene/HUD without advancing time.
It never captures other windows or injects OS input. Strict replay rejects old
renderer recordings. Review evidence: [R06 verification](docs/r06_verification.md).

Keep disposable captures/experiments in ignored `captures/`, `scratch/` or a suitably
sized temporary directory. Selected source art, prompts, animation sheets and curated
historical screenshots remain versioned. The source texture normalization and NPC
sheet tools remain explicit authoring tools, independent of runtime rendering.
