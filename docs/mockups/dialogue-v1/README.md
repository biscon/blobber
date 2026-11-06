# Friendly NPC dialogue — presentation exploration v1

2026-11-06. **Presentation direction approved by the user.** This is a generated visual
reference for discussion before an implementation plan, not a runtime capture.
Implementation is tracked in [the dialogue design](../../dialogue_design.md).
Trading and quests remain later work.

- [Dialogue screen](dialogue-screen-v1.png)
- [Exact built-in ImageGen request](dialogue-screen-v1.prompt.json)
- [Existing resident implementation](../../innkeeper_design.md)

## Presentation under discussion

Keep the NPC in the world and use a closer conversation camera. The innkeeper
remains behind the counter with his existing shelves and room around him; this
is not a separate painted portrait. Retain the party sidebar and replace the
exploration/combat controls with a response panel and a persistent topic list.
The image illustrates a first meeting, with Trade directly accessible during
the introduction. The wording is example copy, not final game lore.

The user requires human-readable JSON for editable dialogue text and topics.
The same conversation presentation and topic flow must support future friendly
NPCs. First-meeting state belongs to each character in the campaign, rather
than to all actors sharing an appearance. Characters supply their own topics;
Trade is only available for characters that provide it.

A longer introduction runs on the first meeting only. Later visits show a brief
greeting with topics immediately usable. Allow the introduction to be replayed
through an appropriate topic, such as About yourself. Click in the response area
or press Space to advance a page; Skip introduction skips the whole sequence;
clicking a topic goes straight to that topic. Number shortcuts select topics,
and Escape leaves. Avoid input falling through to another page or action.
These controls are the agreed direction for the first implementation slice.

The first gameplay slice remains greeting, topics, short linear responses,
return/farewell, and one response that changes after a recorded fact. The
mockup includes Trade to establish its eventual placement; trading and quests
remain later work. There is no requirement for an embedded scripting language.

## Camera and fidelity limits

The current renderer combines 3D room geometry with prepared directional NPC
images. A dialogue camera must frame those existing assets, respect furniture
occlusion, and leave the party's grid position unchanged. Framing should be
reusable across friendly NPCs, with character-specific adjustment where needed;
do not hardcode the whole interaction around this counter.

Generated close-up detail and typography are presentation targets. Actual sprite
resolution, safe camera placement, projection, framing and text layout require
engine verification in the eventual implementation. The NPC is not being
rescaled by this proposal. The user subsequently agreed to a 700 ms transition and panels overlaid on the
unchanged world viewport, rather than resizing the viewport.
The generator returned a 1717×916 image; it is preserved at its native size.
The implementation target remains the game's fixed 1920×1080 presentation.

## Artifact record

Generated with the built-in imagegen tool using the existing runtime front view
and approved character turnaround as references. This directory retains the
selected mockup and exact request as durable implementation reference material.
No desktop interaction or game capture was needed for this presentation study.
