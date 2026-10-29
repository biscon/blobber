# Slice 12.4: character creator and reusable selection

Status: implemented, **accepted and closed** (2026-10-29). The user approved
this slice's plan with “Implement the plan.” Slice 12.3 is accepted and closed.
No advancement, XP controls, new gameplay content or run-save implementation.

## Ownership and flow

A fixed graphics-independent CreatorModel owns CharacterDraft, its initial baseline,
step/visited state and validation. Creation follows Identity, Class, Abilities,
reserved Skills, conditional spell stages, Equipment and Review. Menu orchestration
retains caller screen/active slot/companions, owns explicit disk operations, and
returns to the new UUID without assignment or launch. Review is shared read-only
with library View. Save is revision checked and exactly once; committed durability
warnings never retry a successful creation. Failed writes retain the draft.
Dirty cancellation defaults to Keep creating. No UI callback mutates live builds.

First class choice visibly installs recommendations. Race/class changes preserve
name, abilities and valid choices; invalid spell selections are pruned, equivalent
gear retained, incompatible portrait/default gear replaced with an explanation.
Reduced preparation allowance retains overfull valid picks for explicit removal.
Recommendations modify only their category. Wizard preparation uses the actual
spellbook. Names remain 1–64 printable ASCII bytes. All 27 points must be spent.

SpellSelectionContext supplies identity, immutable baseline, available and locked
masks, minimum/maximum choice count and editable result. One bounded panel supports
creation, camp preparation and advancement-shaped tests. Inspected and selected
are separate; grants are locked outside ordinary budgets. High Elf cantrips have
separate INT/source/count. No generic workflow engine or progression commit API.

## Presentation and bounds

Approved screens 05–16 guide dark green panels, gold headings, teal selections,
portrait sidebar, fixed navigation and scrolling content. Existing native fonts,
item stacks, portraits and prepared dungeon rendering remain. Shared geometry
routes pointer and keyboard input; text editing has a cursor/caret and fixed storage.
Confirmation/input transitions consume activation. Drafts, 32 spell options,
4 party preparation records, 128 library records and fixed widget arrays bound work.
Exhaustion is explicit; editing/rendering does not allocate. Disk/save/capture and
preparation are explicit allocation phases.

Larger spell/class details use preserved masters baked offline into one padded
2048x2048 QOI atlas, up to 36 regions of 324x324 with 2-pixel padding. Residency
adds 16 MiB, using the eighth existing texture slot. Bake workspace is bounded at
64 MiB; runtime checks dimensions/encoded bounds before upload. No new masters,
per-frame decoding/resizing or changes to dungeon banks. GPU lifetime remains RAII.

## Checked rest preparation

Long Rest has optional Prepare spells. Living Clerics/Wizards can stage prepared
masks using the same panel; learned/cantrip/domain/racial grants are unchanged.
Done stages, Cancel discards that editing pass, closing camp discards all pending
changes. Short Rest never applies preparation. Keeping current spells retains the
ordinary path. User confirmed that a legal changed preparation alone is a rest
benefit, with existing safety, eight-hour time and camp supply costs.

RestRequest gains an optional fixed party preparation payload with expected
inventory/preparation revisions. Party owns a monotonic preparation revision.
Pure preparation validation is independent of creation point buy/start gear.
Session rechecks world, single-use token, revisions, safety/life, eligibility and
supplies before mutation. All failures precede supply consumption/RNG/recovery.
Supply, recovery, prepared masks and token/time advancement commit once before
cosmetic playback, preserving existing deadline ordering. Playback can be skipped
without rollback/replay. Live identity/inventory and library blueprints stay separate.

## Verification and handoff

Generated fixtures cover all sixteen race/class combinations, portraits, gear,
point-buy boundaries, invalidation, recommendations, budgets/grants, text/focus,
scrolling and dirty cancel. Temporary libraries cover caller restoration, capacity,
stale/write/durability errors, repeated Save and reload/independent launch. Recovery
covers keep/change/cancel, preparation-only benefit, no-op, multi-caster, dead/stable,
invalid/stale/unsafe/no-supply requests, Short Rest isolation and exactly-once commit.
Advancement-shaped tests lock baseline spells and grant only new choices.
Extend engine C++ allocation guards, run fresh full Debug/Release and focused
ASan/UBSan, prepared atlas bounds/reproduction and hidden GPU lifetime checks.
--capture-creator uses temporary libraries and in-process scripts, exporting native,
1280x720 and letterboxed 1024x768 screens and the full menu/create/party/dungeon loop.
Never inject OS input or control windows. Record unavailable checks explicitly.
The implementation stopped at Ready for review; explicit user acceptance is recorded
in the verification handoff.

## Implemented interfaces and lifetimes

`game/creator` owns the fixed draft controller and shared `FlowControls` geometry.
`rules/spell_selection` owns available/locked masks, budgets and pure preparation
validation. `ui/creator_screen` draws creator/preparation screens and the read-only
build review reused by the library. `app/creator_flow` owns caller restoration and
revision-checked persistence; `runMenus` owns its lifecycle and platform input.
`input/text_edit` handles bounded cursor editing; only `platform/input` polls raylib.

Context identity strings borrow the draft or live Party, both of which outlive the
panel. A selection owns its search buffer and visible-ID array; result masks are
copied into the caller's draft. The 128-control array reports overflow and disables
routing rather than growing. IDs/masks borrow one immutable catalog. No allocation,
disk access, live grant mutation or entity creation occurs in panel callbacks.

`CampHud` stores four contexts, working masks, an editing request and a previously
staged request. `preparation_commands` switches the active caster, validates Done
and stages only. `Party::preparationRequest` snapshots live masks and both revisions;
`alternativePreparation` probes whether an ordinary-cost preparation-only rest is
available without mutation. `GameSession::rest` retains its world/token/safety/time
checks, calls the checked Party commit, then advances the token and rest deadlines.
The monotonically increasing preparation revision is checked for exhaustion.
Future advancement that changes preparation eligibility must invalidate this
revision or the rest token before accepting an old panel result.

The atlas contains the 24 existing spell masters in SpellKind order plus four class
symbols. It is baked as `assets/prepared/creator.qoi`; existing portrait IDs, catalog
schema 8 and versioned character-library schema remain unchanged. Input PNGs are
limited to 8 MiB, 8-bit RGB/RGBA, 324–2048 pixels per dimension. The bake holds one
master at a time; the 16 MiB atlas plus image/resize or QOI-export workspace stay
within the 64 MiB design budget. Runtime rejects unexpected QOI dimensions, channels
and encoded sizes before decoding/upload. The current-assets bake measured 37,116
KiB peak process RSS; this is one workload measurement, not a third-party allocator
guarantee. Prepared tiles are drawn directly; there is no frame-time image resize.

[Verification, retained captures and next-agent handoff](slice124_verification.md). The user accepted the slice on 2026-10-29 and chose to retain the current
27-point buy, 8–15 base scores and fixed racial bonuses for now.
