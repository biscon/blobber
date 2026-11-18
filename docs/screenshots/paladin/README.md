# Paladin through level four

Status: **ready for user review; acceptance pending**, 2026-11-16.
Existing level-four acceptance is unchanged.

Eight selected production UI captures, exported through an application-owned hidden
OpenGL window with in-process commands. Native presentation is 1920x1080; the
reduced healing image checks the 1280x720 presentation. All eight were visually
inspected.
The menu backdrop is deliberate: these isolate real UI composition using a small
owned world, the production catalog/source artwork, and prepared Debug atlases.
No OS input, window focus or desktop automation was used.

- [Creation](creation.png): five selectable classes, paladin description and generated icon.
- [Advancement](advancement.png): shared level-four flow, CHA 16 to 18, no class cantrip choice.
- [Character sheet](character-sheet.png): level four, d10 Hit Dice, Defense, explicitly named 20/20 healing points, six mana and CHA casting.
- [Abilities](abilities.png): Lay on Hands, Divine Smite, Sacred Weapon and automatic Defense; Smite is unavailable outside melee combat.
- [Healing](healing.png): opaque resting handle over the track, explicit 20/20 healing pool, eight HP selected/spent and twelve points kept; excess capacity is shaded.
- [Reduced healing presentation](healing-1280.png): same slider and confirmation at 1280x720.
- [Level-one character sheet](level-one-character-sheet.png): separate 5/5 healing points, one point per HP and long-rest recovery, before spell mana unlocks.
- [Level-one healing](level-one-healing.png): held between the one- and two-point positions with 2/5 healing points left; continuous handle/fill feedback, integer one-point preview, and no spending during the drag.

The two extra level-one cases are retained while user review is pending to show
the resource distinction before spellcasting, and the reproduced drag behavior.

[Design, exact artwork prompts, test commands/results and limitations](../../paladin_design.md).
The four generated PNGs and exact prompts are durable assets under
`assets/ui/spells/` (Lay on Hands, Divine Smite, Sacred Weapon and Protection from
Evil and Good); they are included by the shipping UI inventory selector.
The user reported the original drag failure during play; the corrected path has
in-process physical-input coverage but awaits interactive user review. Deferred
SRD features remain as specified in the design. Disposable logs and intermediary
captures are cleaned; only these eight review images are retained here. Cleanup
does not imply user acceptance.

The abilities capture was refreshed during the implementation review on
2026-11-16 to show the corrected Defense entry. Acceptance remains pending.
The current drag/resource follow-up replaced the level-four sheet and both healing
captures and added the two level-one cases. Creation, advancement and abilities
are unchanged. Acceptance remains pending.
The continuous-feedback follow-up replaced only the three healing images; the
other five images remain unchanged. The two-point held-drag image replaces the
earlier three-of-five selection. Interactive acceptance is still pending.
