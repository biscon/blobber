# PC07.2 — Population and generated-route checks

Status: **Accepted and closed, including review corrections**, 2026-11-01.
User: “slice accepted, record it in the plan doc”. The roadmap current handoff
records acceptance; verification and historical balance limitations stay unchanged.
The later [variety and controls correction](pc072_variety_controls.md) supersedes
starting kit count, normal topology, rogue completion and version identities.
The [review correction design](pc072_review_corrections.md) supersedes the original
potion budgets, lock placement and version identities below. The retained eight-route
study describes the original population delivery; encounters and XP stay unchanged. The user approved the
reviewed plan with “Implement the plan”, including eight bounded serial routes.
PC07.1 stays accepted. PC08 exploration agents remain outside this slice.

## Experience and budgets

Use ordinary supported monsters and the measured PC07.1 recipes. Floor 1 has two
goblin pairs, goblin-support guardians and an optional rat pack (400/75 XP).
Floor 2 has an undead pair, goblin support, goblin-support guardians and an optional
goblin pair (500/100 XP). Floor 3 has goblin/undead/goblin pairs, bugbear-support
guardians and an optional undead pair (300 before guardian, 300 guardian, 100 optional).
The intended direct route earns 1,200 monster XP before the final guardian, 300 per
living member in a four-survivor party. Ordinary fights remain bypassable; this
explicitly supersedes a structurally compulsory XP interpretation. Advancement is
an ordinary checked player action. No stats, XP multipliers, respawns or free refills.

Each pre-guardian cache contains two healing/four mana potions and twenty arrows.
Each reserve has one healing/two mana potions and ten arrows. Floors 1–2 retain
one healing potion/one kit beyond the gate; final victory adds no consumable cache.
Starting supplies stay unchanged. These are finite candidate budgets, not balance claims.

## Responsibilities, data and ownership

Generation retains at most five fixed encounter records, thirty-two species/member
placements and existing bounded room/fixture data. Stable encounter IDs and member
ranges describe direct/optional/guardian roles. A deterministic graph route visits
the key when needed and the approach; shortest-path ties use room IDs. Distinct
route junctions hold ordinary fights, with the key room quiet. Canonical subslots
rotate toward the approach. Closed ordinary doors isolate populated-room ports,
reusing existing doors where present. Final tiles, keys, gates, supplies, actor
access and landings still validate within sixteen attempts plus a checked fallback.

One population resolver produces bounded initial actors, recovery and objective
values for both launch and save validation. Only preparation constructs NpcWorld
vectors. CampaignRun retains its single Party and snapshot ownership. Frame paths
gain no allocation or lookup layer. Generation/fingerprint/diagnostics and save
fields include the new records. Generator/templates become 5/5; campaign
schema/simulation become 5/8. Old incompatible saves reject unchanged; no migration.
Arena schema 2/protocol v1 and asset formats remain unchanged. Full generation
metadata is diagnostic-only; public seed/version/fingerprint identity remains
available, and player discovery does not reveal encounter membership.

## Scripted evidence and bounds

A focused diagnostic route script reuses the client, aggressive-v2 combat tactics,
metrics and recording; declared waypoints replace arena navigation. It uses checked
pickup, advancement, rest and travel. Builds and level-two choices match PC07.1;
new preparation slots remain empty. Resources persist across fights, and expired
preparation is reapplied with ordinary costs. Collect consumables/ammunition/kits,
retain starting equipment, and allow one field long rest before each guardian.
Record insufficient/additional recovery needs rather than adding hidden recovery.
Epoch-scoped burst/encounter accounting prevents local clocks from crossing floors;
pickups/transfers and advancement/rest gains must not masquerade as spending/healing.

Approved sample: balanced/no-wizard × seeds 5/8 × direct/reserve = eight serial
expeditions. Reserve clears optional groups before guardians. Bounds: 120 wall
seconds/route, 60,000 simulation ticks, existing finite request/decision ceilings,
256 MiB engine trace, thirty minutes and three GiB overall including replay. Inspect
each result; stop on tooling/coverage/artifact/budget failure. Retain losses and all
inputs, decisions and traces. Replay one completed route per party, prioritizing
downings. No automatic reruns or study expansion. Report damage/bursts, downings,
deaths, mana, healing, ammunition, supplies, levels, rests, merges and traversal time
separately, with unsupported decisions and incomplete evidence explicitly classified.

## Verification and acceptance

Use small owned CPU fixtures for recipes/XP, all depths/rotations, routes/fallbacks,
invalid metadata/capacity, actual AI approaches/isolation, safe revisits, exactly-once
rewards, save corruption/rollback and script cadence/failure cases. One Release
configuration; focused generation/fixtures/guardian/campaign/automation/allocation
checks, smoke saves during iteration and full persistence once at integration.
Inspect permitted application captures where available, without OS input/focus.
One intensive job, low CPU/I/O priority, at most two build workers/one test process;
check memory and disk first. Keep experiments outside routine tests. Record exact
commands, failures and limitations in pc072_verification.md. Stop Ready for review;
only the user accepts PC07/P-C, and PC08 does not start automatically.

## Delivered result

[Verification and handoff](pc072_verification.md), [route costs and retained inputs](pc072_routes/README.md)
and [capture gallery](screenshots/pc072/index.html) record delivery. All eight routes
won, with 32 observed downings, no permanent deaths and no recorded merges. Every
route reached level two before the final guardian using three kit-funded rests.
Both selected retained replays matched. These small diagnostic samples establish
the tested routes, not universal balance; reserve detours had real costs and frequent
downings remain a review concern. Player observations retain generation identity
but never receive hidden route/encounter details. No automatic PC08 work or acceptance.
