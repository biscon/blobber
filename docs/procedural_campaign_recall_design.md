# PC05: paid recall and entry waypoints

Status: **Implemented; Accepted and closed**, 2026-10-30.
User approved the reviewed PC05 plan with “Implement the plan” and accepted
the slice with “slice accepted, record it in the plan doc” (2026-10-30). PC04 remains
Accepted and closed; PC06 and milestone P-B remain Planned.

## Ownership and transaction

Extend CampaignRun's existing TravelRequest/PreparedTravel transaction with
stairs, recall and waypoint kinds. Requests bind campaign revision and source
world; recall also binds the selected camp-kit handle and inventory revision.
Source safety remains GameSession::travelSafety. Preview is allocation-free and
checks source eligibility, not destination restoration. Preparation validates the
restored landing, transfers portable effects and stages simulation/view borrowers.
No new campaign owner, effect-transfer path or saved departure position is added.

Party exposes a read-only camp-supply query shared with recovery: scan characters
then bag cells in stable order, counting Supply items and choosing the first.
A recall ticket stages a copied Inventory and supplier derived stats with one kit
removed. Source Party stays untouched until final revision/Party/safety validation.
The nonthrowing commit publishes the charge, rebased party deadlines, source
snapshot and destination owner together. Failure leaves source/resources/unlocks
intact. Ticket storage is bounded and measured. Preparation may allocate;
ordinary preview, controller updates and frame processing may not.

Recall is legal only from a live dungeon at the existing safe travel boundary.
It costs one kit even at the entry, reaches the hub sanctuary landing, and advances
no clock or recovery. Recovery remains explicit. Waypoints cost nothing and require
the reachable/faced hub entrance and a visited floor. CampaignLocation::visited
unlocks only on committed stairs arrival; selection, previews and failures never
unlock anything. Ordinary stairs remain free, including with zero kits. Arrivals
use protected original entry poses; local clocks freeze and portable effects obey
PC04. No respawns, refills, resurrection, disk saves or tuning.

## Presentation and input

A dedicated paused Travel panel opens through T or a labelled header control in
normal exploration. Preserve Inventory's contextual T transfer. In a dungeon the
panel explicitly confirms Recall — 1 camp kit, with count, sanctuary destination
and entry-return explanation. Unavailable reasons disable confirmation; full health
does not block recall. Escape/Cancel has no gameplay effect.

At the hub, F/click on the entrance opens the selector; T requires the same reach
and facing. Offer unlocked depths only, plus ordinary Enter floor 1 for a fresh run.
Select a row then confirm free travel. Never show hidden layouts/actors. Panel
state has three fixed destination rows and fixed text buffers. Opening cannot also
confirm; old targeting/hover/pending input is cleared and held input must release
across transitions. Existing dungeon stair interaction stays immediate.

## Automation

Add recall and waypoint action kinds through the existing typed Session boundary,
preview/act, CLI/client and MCP tools. Explicit act confirms; preview does not.
Retain interact for ordinary stairs, including hub-to-floor-1. Observations and
action descriptors include source eligibility, reasons, kit cost/count and unlocked
choices. Transition events identify kind and actual kit charge; first committed
stairs arrival emits entry_unlocked. Strict replay includes resources, unlocks and
inactive digests. Protocol/scenario remain version 1 with additive extensions;
generator/templates and prepared banks are unchanged. No expedition policy.

## Verification and review

Owned fixtures cover all depths; last/multiple/split supplies; stale handles,
inventory, worlds and tickets; preview/cancel/repeat purity; unsafe phases, movement,
hazards, leadership/concentration/summon restrictions and terminal runs; invalid or
locked depths and occupied/unsafe landings; capacity and allocation failures.
Compare Party identity/resources, gear/formation/XP, loot/reward/death ledgers,
keys/doors/discovery, RNG and portable/local deadlines across both travel modes,
explicit hub recovery and free walking. Exercise UI keyboard/click parity, modal
input, Inventory T and held-input release. Extend CLI/client/local MCP and strict
replay/corruption checks. Run all four Debug/Release graphics/headless presets,
focused ASan/UBSan lifetime checks, allocation guards and available hidden GPU and
application-owned captures. No OS input/focus automation, balance experiment or
broad seed sweep. Record executed commands, limitations and reproduction separately.

Review: camp in place, recall from deeper exploration, verify exactly one kit and
no arrival healing, rest explicitly, return to an entry, verify persistent mutations
and loss of departure position, then demonstrate zero-kit walking. The implementation stopped at Ready
for review and the user subsequently accepted PC05. PC06 does not start automatically.


## Implemented interfaces and evidence

TravelRequest lives in the small shared travel header; CampaignRun supplies request,
recallRequest and waypointRequest factories. PreparedTravel carries optional fixed
Inventory charge storage and supplier stats alongside its existing source snapshot,
Party stamp and rebased deadlines. Commit also rechecks destination pose/landing.
Party::campSupplies is shared with previewRest; private stageCampSupply derives the
charged supplier stats without touching the live Party. The ticket is 10,680 bytes
on the measured build, with no additional persistent floor snapshot fields.

CommandController owns TravelHud's three rows and emits a complete typed intent;
the app consumes it before another tick. UI routing preserves contextual Inventory
T and prevents open/selection input from also confirming. A separate small travel
screen uses native HUD fonts and geometry; it adds no prepared art or GPU resources.
Automation::Session reuses one travel helper for stairs/recall/waypoints and stages
its visibility borrower before commit. All normal paths use the same authority.

Full headless Debug/Release 39/39 each, graphics Debug/Release 58/58 each, focused
ASan/UBSan, zero tracked runtime C++ allocations and all exercised preparation
allocation failures pass. CLI/client/local MCP and strict replay checks pass.
Fourteen retained native/reduced PNGs match an independent hidden capture run.
[Verification, commands, limits and review recipe](pc05_verification.md) and
[gallery](screenshots/pc05/index.html) provide the evidence. PC05 is Accepted
and closed; verification results and limitations are unchanged. No save, tuning,
experiment, respawn or PC06 work was added. PC06 is next for detailed planning only,
in a fresh context when requested.
