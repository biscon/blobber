# Slice 12.1: expanded level-one spells

Status: **Ready for review**, 2026-10-28. Implemented after the user approved the
plan and requested implementation. [Verification and handoff](slice121_verification.md).
Slice 12 is accepted; 12.2–12.4 and 13 remain unstarted.
Source: [local SRD 5.1](references/README.md). No implementation acceptance implied.

## Approved mechanics

All spells use the existing action/shared six-second cooldown; cantrips cost zero,
level-one spells cost two mana. Only Guidance/Resistance among these additions
concentrate. Existing fourteen spells retain behavior and class-wide availability.

| Spell | Target and consequence | Timing |
| --- | --- | --- |
| Ray of Frost | Six-cell visible ranged attack; 1d8 cold and slow on surviving hit | Existing flight; slow at impact |
| Shocking Grasp | Exposed melee spell attack; 1d8 lightning; metal-body-armor advantage | 200 ms impact; reactions omitted |
| Poison Spray | Visible enemy in three-cell front, irrespective of row protection; Con save negates 1d12 poison | Immediate |
| Sleep | Aimed six-cell range, 3×3 area, 5d8 current-HP budget, both sides | Immediate; 60 gameplay seconds or wake |
| False Life | Self, 1d4+4 temporary HP; keep larger amount and refresh | Immediate; one hour |
| Grease | Aimed walkable cell within six cells, empty allowed; difficult terrain and Dex-save knockdown, both sides | Immediate; 60 seconds |
| Color Spray | Three front cells, 6d10 current-HP budget, blindness | Immediate; short expiry |
| Guidance | Conscious party recipient, next initiative or Aid check +1d4 automatically | Concentration, 60 seconds maximum |
| Resistance | Living party recipient, next rolled save +1d4 automatically | Concentration, 60 seconds maximum |
| Sanctuary | Living party recipient, hostile single-target attacker Wisdom save | 60 seconds, no concentration |

Ground centers require view and conservative LOS. Sleep's nine cells exclude walls
and center-to-cell LOS obstruction; all occupants count, not just visible bodies.
Sort HP recipients by current HP, then party before NPC and stable identity. Ignore
temporary HP, dead/dying/unconscious/sleeping and immune targets; Color Spray also
ignores blind targets. Stop at first unaffordable HP. Empty eligible Sleep/Color
areas reject before costs/RNG, an insufficient rolled budget does not refund.
Color Spray excludes the origin cell. Area UI shows party risk without hidden
silhouettes. High Elf magical-sleep immunity is activated; other racial features
remain deferred, including the racial cantrip until 12.2.

Sleep is independent of health/life: occupancy remains, actions/sight/concentration/
paired protection cease. Positive damage (even absorbed by temporary HP) or an Aid
action wakes; zero damage does not. Aid to wake is automatic, no ability check.
NPCs with no legal attack may wake a same/adjacent-cell ally, lowest stable ID first.
Sleep grants incoming advantage, automatic Str/Dex save failure, close-hit criticals.
Waking adds no stand-up penalty; independent Grease knockdown remains. Sleep alone
never causes victory/defeat. Accepted rolls remain frozen through subsequent changes.

Blind: incoming advantage/outgoing disadvantage; no sight-dependent spells, NPC
ranged attacks or visual acquisition. NPCs use remembered positions; contact melee
still works with disadvantage. Occupancy, concentration and protection remain.

Slow/Grease double travel and step cadence, capped at 2× together. Grease applies
when entering or leaving its cell. Per-step duration is immutable after commitment;
ordinary player 160 ms/NPC 600 ms timing stays unchanged. In turns, penalized NPC
steps take two movement actions: prepare then revalidate/complete. Changing destination
or choosing an attack clears preparation; expiry permits ordinary movement.
Knockdown automatically spends the next eligible turn standing, no manual button,
attack or cast. Outside turns it blocks action/movement six seconds and delays
readiness through recovery. It removes paired protection, not concentration or
occupancy; incoming contact advantage/distant disadvantage, no automatic critical.
Able companions retain the existing carrying policy; shared party travel slows on Grease.
Grease checks creation, entry (including pushes), and individual turn end; stationary
real-time occupants every six seconds. Overlaps check once per trigger at highest
DC, then oldest serial. Failure does not stack recovery. Standing can be followed
by another end-turn failure while still on Grease.

Short slow/blind effects expire at the caster's next scheduled turn start, including
skipped turns; outside turns they last six gameplay seconds. On entry bind remaining
effects to the next caster turn; on exit retain unused real-time budget. Transitions
never refresh it. Other durations retain gameplay time. Pauses freeze deadlines;
expiry precedes equal-time impacts. New consequences never reroll accepted attacks.
Temporary HP are separate health storage, absorb before HP, never heal/revive/stack,
clear on depletion/expiry/death. Concentration uses pre-buffer damage. Guidance and
Resistance consume only on actual rolls, combine correctly with Bless/Bane and
preserve natural death-save 1/20; no consumption on automatic failure.
Sanctuary tests initially chosen target then each other legal formation target once;
one action/cooldown regardless of success/all failures. No retroactive projectile
checks or area protection. Ward ends at accepted attack/hostile cast (including
empty Grease), never at rejection. Rest rejects Grease beneath the party; otherwise
skips movement/hazard retries while advancing expiry.

## Ownership, interfaces and bounded work

Rules metadata adds targeting/range/sight/timing; PartyCommand gains aimed cell.
The same pure area/eligibility queries feed preview and authoritative validation.
EffectController retains 64 ordinary instances with three recipients for existing
buffs and Guidance/Resistance/Sanctuary. Separate contiguous per-creature condition
records cover four party identities plus 256 stable actor slots, one record per
Sleep/Blind/Slow/Knockdown kind. Source/expiry retained; no stacking or allocation.
Action, concentration, sight and paired-protection queries have distinct meanings.
A session-owned 16-patch Grease pool uses world/serial handles, rejects exhaustion
before RNG/payment, and never evicts gameplay. Nine-cell/40-recipient area scratch
is fixed. Party owns temporary health and deadline. No general spell VM/ECS.

Schema 7 adds explicit metal-armor/sleep-immunity data; versions 1–6 default missing
capabilities false. No name/artwork inference. Catalog/class/picker limits remain
32/16/22. Explicit UI icon mapping avoids feature/item collisions. Events increase
to 8192; compile-time subsystem allowances cover pending attacks, effect cleanup,
condition expiry, 60-second bounded updates, hazard checks/recovery and synchronous
commands. The event backing vector allocates its fixed size once at owner construction;
no simulation operation resizes it. This avoids large event arrays on the stack.

Prepared floor projection extends to ground picking, nine-cell preview and Grease
including strafe phases/depth occlusion. Persistent revisions invalidate composition;
transition picking disabled. Ten painted icons retain masters/prompts, native atlas
and accepted casting UI. Prepared pulses/bolts and visible status/temporary-HP/log
feedback expose actual outcomes. Loading/art generation/export may allocate; normal
simulation, targeting, composition and UI do not.

## Verification and handoff

Deterministic test-owned catalogs/maps cover all ten casts, miss/save/immunity,
empty/low/tied budgets, friendly fire, LOS, stale IDs, capacity and RNG/resource
rollback. Cover waking, all-party sleep, victory prevention, blindness, knockdown,
repeated Grease, slow prepare/cancel/expiry, short-effect phase transitions, temporary
HP overflow/absorption/concentration/expiry, one-shot modifiers and natural saves,
Sanctuary retry bounds and impact ties. Preserve fourteen-spell regressions.
Run full Debug/Release, focused ASan/UBSan and first/repeated frame allocations;
separate maximum-capacity stress. Hidden GPU lifecycle and all new spell/status
captures at 1920×1080, 1280×720 and letterboxed 1024×768. No desktop automation.
Record exact commands/results/limitations and review recipe in slice121_verification.md.
Stop at Ready for review; only the user can accept. Do not advance to 12.2.
