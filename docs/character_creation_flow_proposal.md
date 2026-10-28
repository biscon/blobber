# Main menu and character creation: visual proposal

Status: **visual proposal accepted**, 2026-10-28, including the expanded spell
catalog. The user subsequently chose Color Spray instead of illustrative Ray of
Sickness. The [12.1–12.4 implementation roadmap](dnd5e_demo_design.md#character-creation-interlude-accepted-flow-and-shared-contracts)
is now recorded between slices 12 and 13, with one dedicated spell-content slice.
Each slice still receives its own detailed planning and implementation pass.
No gameplay implementation has started; slice 12.1 is next eligible for planning.

[Open the sixteen-screen gallery](mockups/character-creator-v1/index.html).
[Exact imagegen prompts, corrections, and provenance](mockups/character-creator-v1/prompts.json).
All images were generated with the built-in imagegen tool and visually inspected.
The PNGs are 1672×941 concept images for the existing fixed 1920×1080 presentation;
they are not runtime UI textures or production portrait exports.

## Routes and screens

| Screen | Purpose and transition |
| --- | --- |
| [01 Main menu](mockups/character-creator-v1/01-main-menu-v1.png) | Start new game → party selection. Character Creator → library. Quit → clean application shutdown. Opening the menu does not construct or load a dungeon session. |
| [02 Character library](mockups/character-creator-v1/02-character-library-v1.png) | Scrollable grid of saved characters, name search and race/class filters. New character starts a draft. View uses the review layout read-only. Delete opens confirmation. |
| [03 Party selection](mockups/character-creator-v1/03-party-selection-v1.png) | Choose four distinct characters and their front/rear positions; Enter dungeon loads the existing test level. A default-party shortcut preserves quick access. New character returns here after saving. |
| [04 Delete confirmation](mockups/character-creator-v1/04-delete-confirmation-v1.png) | Named character and portrait, explicit permanent library deletion, default focus on Keep character. |
| [05 Identity](mockups/character-creator-v1/05-identity-and-portrait-v1.png) | Enter name, choose race, choose portrait. The image shows revisiting this step after Wizard has been selected; a fresh draft has no class label yet. |
| [06 Class](mockups/character-creator-v1/06-class-v1.png) | Fighter, Cleric, Rogue, Wizard; clear role and implemented feature descriptions. |
| [07 Abilities](mockups/character-creator-v1/07-point-buy-v1.png) | 27-point buy, separate base/race/final values, modifiers, live derived previews, recommended allocation and reset. |
| [08 Skills](mockups/character-creator-v1/08-skills-reserved-v1.png) | Reserved non-blocking step. Explicitly unavailable, no fake selectable skills or applied bonuses. |
| [09 Cantrips](mockups/character-creator-v1/09-cantrips-v1.png) | Selection count, spell details and recommended picks. Cantrips do not consume mana or preparation allowance. |
| [10 Wizard spellbook](mockups/character-creator-v1/10-spellbook-v1.png) | Choose learned level-one spells. This intermediate stage is wizard-specific. |
| [11 Wizard preparation](mockups/character-creator-v1/11-prepared-spells-v1.png) | Choose ready spells from the spellbook; show how the preparation allowance is calculated. |
| [12 Equipment](mockups/character-creator-v1/12-starting-equipment-v1.png) | Class-appropriate starting gear choices, included supplies, equipped/backpack preview and derived stats. |
| [13 Review](mockups/character-creator-v1/13-review-and-save-v1.png) | Coherent full summary, Change links, validation, one Save character action; return to caller with new character highlighted. |
| [14 Level-up reuse](mockups/character-creator-v1/14-level-up-reuse-v1.png) | Future slice-13 example: existing spells locked, new choices counted separately, before/after values. |
| [15 Cleric preparation](mockups/character-creator-v1/15-cleric-preparation-v1.png) | Skip the wizard spellbook; prepare from the class list. Life Domain spells are separate always-prepared grants. |
| [16 Discard draft](mockups/character-creator-v1/16-discard-draft-v1.png) | Cancel/Escape on a changed draft confirms discard; Keep creating is the default. |

The user explicitly agreed to **choose the party on Start new game** during this
design session. The library is character management, while the party picker owns
the pending four-person formation. The picker is a step before loading the test
level, not a different level or campaign selector.

## Accepted flow and roadmap refinements

**Point buy.** Use the established SRD 5.1/2014 racial bonuses, with base scores
8–15 and 27 points. Score costs for 8 through 15 are 0, 1, 2, 3, 4, 5, 7, 9.
Recommend a valid class allocation by default; do not roll stats. Show remaining
points and a reason when an increase is unavailable. The roadmap requires spending
the full budget before completion; spent points never carry forward to level-up.
BG3 uses the same point-buy budget but different freely assigned +2/+1 bonuses;
adopting those bonuses is not part of this proposal.

**Spells — expanded catalog accepted.** The mockups use normal
level-one selection counts with an expanded curated catalog: wizard three
cantrips and six spellbook spells, cleric three cantrips. Preparation allows up
to class level plus the casting ability modifier, minimum one. The examples fill
the allowance; the UI must not incorrectly treat that maximum as a mandatory
minimum. Cleric Life Domain grants Bless and Cure Wounds without using that
allowance. Cantrips are separate. Keep the existing mana/action system.

The current implementation has only two cantrips per caster class, four first-level
wizard spells, six first-level cleric spells, and no per-character preparation.
It cannot support the illustrated choices without more content and authoritative
per-character spell ownership. The roadmap now specifies ten additions: Wizard
Ray of Frost, Shocking Grasp, Poison Spray, Sleep, False Life, Grease and Color
Spray; Cleric Guidance, Resistance and Sanctuary. These remain unimplemented.
Detailed targeting, condition, clock and action-economy adaptations are reviewed
in slice 12.1's planning pass. The user explicitly approved **Color Spray replacing
Ray of Sickness**: screens 10 and 14 keep their original illustrative text, while
the gallery captions and roadmap record the correction. Do not implement that
obsolete label. Guidance's current ability-check consumer is initiative; skills
remain reserved. The roster has five wizard and four cleric cantrips, eight wizard
and seven cleric first-level spells, making selection possible at normal counts.

Ordinary non-casters skip spells. The roadmap activates the High Elf bonus wizard
cantrip using a separate grant and Intelligence, even for a martial or cleric
character; duplicate choices across grants are invalid. This requires actual
per-grant casting support in 12.2; the trait is not functioning yet. Re-preparation
with a long rest reuses the same panel in 12.4, preserving learned spells and
committing choices with the checked rest transaction.

**Portraits and class identity.** Five portraits for each of Human, Hill Dwarf,
Lightfoot Halfling and High Elf: twenty total. Retain the existing four portraits
and add four alternatives per race. Cover armored fighter, robed mage, cleric,
rogue and general adventurer appearances, with varied adult ages, genders and
skin tones. Portrait choice is cosmetic and never restricts class. Concept-image
likenesses are illustrative; do not replace existing character art with crops
from these mockups. Production portraits will need their own generated masters,
stable identities and prepared UI sizes in slice 12.2.

Every supported race/class combination remains legal. Class cards disclose
currently supported choices: Fighter Defense and Cleric Life Domain remain the
existing defaults, with no pretend menu of unimplemented styles/domains.
Background and skill mechanics are reserved, not implemented here.

**Equipment and supplies.** Offer bounded class-appropriate gear choices, including
armor, shield and ranged options where supported. Start from the existing preset
loadouts. Show what is equipped and what is carried; preserve the separate readied
ranged slot and existing weight/proficiency constraints. Basic healing supplies
and caster mana supplies remain included. Allocate the current party-wide starting
camp-kit allowance once when launching a party, independent of class composition;
do not accidentally multiply it per character or require a Fighter to receive it.
The wizard example carries exactly 7 lb and has 135 lb capacity.

**Library persistence.** Saved characters are reusable level-one starting builds,
stored locally across app launches. Starting a dungeon instantiates independent
party state; adventuring or leveling does not mutate the saved starting build.
Deleting a library entry does not destroy an already-created session/save copy.
Full campaign save/resume remains slice 14. The accepted first-run experience seeds
the existing four presets once, so the player can enter the test level immediately;
an intentionally emptied library stays empty, with Create character and a deliberate
default-party shortcut. Repeated names are allowed because identity is separate
from display name. Name validity and storage errors are shown inline without losing
the draft. The roadmap preserves current 64-byte printable-name validation; use
that limit in the real UI instead of the mockup’s illustrative 32-character counter.

## Shared flow behavior and future ownership

The creator edits an isolated draft. Back and completed-step navigation preserve
choices; changing race/class invalidates only dependent choices and clearly marks
steps needing attention. Review and save revalidate the entire build. Save commits
once, reports errors without losing input, and returns to the originating screen.
Empty results offer clear reset/create actions. Party launch requires four valid,
distinct characters; it never silently duplicates a selected library record.

A small shared flow controller should accept creation versus advancement context,
an existing build and the choices granted by rules. The portrait/summary frame,
choice lists, details, counters, validation and review are reusable. Level-up
offers only newly earned choices, preserves identity/equipment/learned spells,
does not repeat point buy, and does not grant another starting kit. Committing
progression and XP still belongs to slice 13; this proposal only reserves the
reusable presentation and draft/validation boundary.

Later implementation design must separate application menu/session lifetime,
library storage, pure build validation, UI draft state and prepared portrait
resources. File I/O and asset preparation belong outside steady-state frames.
Bounded reusable UI storage and stable saved identities should preserve existing
allocation guarantees; detailed capacities and failure contracts belong in the
approved slice plans, not in these concept images.

Use readable native-resolution text, keyboard navigation and visible focus,
scrollbars/wheel navigation, and both checkmarks and color for selection. Keep
primary navigation fixed while grids scroll. The generated images vary slightly
in ornament, font and header alignment; implementation should use one consistent
layout and the existing UI fonts. Images are visual guidance, not pixel contracts.

## Review and verification boundary

All sixteen final images were inspected; corrected the delete/discard backgrounds
and misleading generated spell text. PNG decoding and gallery links are checked.
No application tests or desktop interaction were needed for this image/document-only
proposal. Production UI, input, portrait integration and rendering remain unverified.

The 12.1–12.4 roadmap now requires checks for point-buy boundaries, conditional
steps, invalidation/backtracking, spell ownership/preparation and grants, inventory
derivation, atomic save/delete failure paths, independent session copies, party
identity/formation, launch/quit resource lifetimes, and zero-allocation frame checks.
Use deterministic test-owned fixtures and hidden application-owned UI captures at
native, reduced and letterboxed sizes. No OS input automation.

## Acceptance record

On 2026-10-28 the user accepted the entire visual proposal and requested the 12.x
roadmap, including a dedicated slice for missing spells before slice 13. They
then selected Color Spray to preserve SRD 5.1 scope. This accepts the visual
direction and expanded scope, not unimplemented slices or unstated detailed
mechanical adaptations. Roadmap refinements above supersede illustrative labels.

## Rule references

- [2014 Basic Rules: character creation](https://www.dndbeyond.com/sources/dnd/basic-rules-2014/step-by-step-characters): ability-score construction and point buy.
- [2014 Basic Rules: classes](https://www.dndbeyond.com/sources/dnd/basic-rules-2014/classes): wizard/cleric selection, preparation and level-up spell learning.
- [BG3 point buy](https://bg3.wiki/wiki/Point_Buy): terminology and the distinction between its flexible bonuses and this project's racial bonuses.

Current code/design grounding: `app/application.cpp`, `rules/character.hpp`,
`assets/rules_character_loader.cpp`, `assets/rules/catalog.json`, and the existing
character, spell, character-sheet and system designs.
