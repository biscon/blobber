# Editing friendly NPC dialogue

Edit [dialogue.json](dialogue.json) in a text editor. Text is ordinary JSON, not
code. Restart the game to reload it. There is no live reload or scripting language.

For development, launch with this source directory so no build/copy is needed:

```sh
./cmake-build-release/blobber --dialogue-dir "$PWD/assets/dialogue"
```

The same option can be added to the CLion run configuration. Without it the game
reads `assets/dialogue/dialogue.json` beside its executable. That packaged copy is
refreshed by the normal build; you can also edit that copy and restart directly.
Art and prepared world assets are unaffected by dialogue changes.

## Character fields

- `id`: stable character identity, e.g. `sanctuary.innkeeper`. Keep it unchanged
  when revising prose: it identifies saved introductions and facts. Appearance
  sharing does not share history. Bind future resident placements with
  `resident_id` using this same string; the authored inn binds its keeper in C++.
- `name`: the heading shown in the conversation panel.
- `greeting`: a short return response, automatically revealed after the first visit.
- `introduction`: the first-meeting response; it plays automatically once and can
  be replayed by a topic with `kind: "introduction"`.
- `facts`: the names of the character's supported facts.
- `topics`: ordered menu entries. Each has an `id` and `label`. IDs use lowercase
  letters, digits, `.`, `_` and `-`; keep them stable across text edits.
- `camera`: optional `distance` and `eye_height`, in cells. Distance defaults
  to 0.6. Omit `eye_height` to meet the NPC at an approximate eye line, 92% of
  its visible standing height above the floor. Set a positive height explicitly
  for unusual artwork. The innkeeper uses the generic defaults. Furniture is
  not a framing target; the engine still stops the camera before world geometry.

Normal topics omit `kind` or use `"response"`. They have `responses`: exactly one
unconditional response, plus optionally one response with `when_fact`. The
conditional response wins once that fact is learned. A response's `set_fact`
records a fact when its final page becomes fully visible, including instant
reveal. Interrupting before that point records nothing. Facts are local to the
character. Names referenced by conditions/effects must appear in `facts`.

```json
{
  "id": "rumours",
  "label": "Rumours",
  "responses": [
    { "pages": ["Ask me about the dungeon first."] },
    { "when_fact": "heard_dungeon_advice", "pages": ["Here is another tip."] }
  ]
}
```

`kind: "farewell"` closes the conversation immediately. Special introduction and
farewell topics have no `responses`. Trade is deliberately absent until trading
exists. The first slice has no quest, currency or item effects.

## Writing pages

Every response has `pages`, an array of strings. Use `\n\n` inside a string for a
paragraph break. Explicit pages are respected; long ones are wrapped and split
again using the actual UI font. Text stays in place as it reveals. Use normal
ASCII quotes/apostrophes; the existing fonts' printable ASCII contract applies,
and unsupported characters fail loading with an error instead of displaying
broken glyphs. Example wording is editable game content, not fixed lore.

Limits: 1 MiB per catalog, 64 characters, 16 topics and 64 facts per character,
8 authored pages per response, and 1,024 bytes per authored page. Long words that
cannot fit the response width are rejected. There are at most 64 laid-out pages
per response. Unknown fields, duplicate IDs/keys and unresolved facts fail loading.

## Presentation settings and controls

The top-level `presentation` sets `transition_ms` (default 700), `fade_ms` (150),
and `characters_per_second` (45). The camera keeps the world viewport unchanged.
Topics are immediately usable during speech. Click a topic or use its visible
number; Up/Down and Enter also select. Scroll or Page Up/Page Down reveals topics
beyond the seven visible rows. Click the response or Space reveals a page, then
advances on a separate press. Skip introduction skips the whole introduction.
Escape or Farewell leaves. Returning greetings never block access to topics.

Dialogue history is saved with the campaign. This feature requires a new
campaign (currently schema/simulation 12/17 with the Sanctuary mayor and guards);
subsequent prose edits do not require one. The mayor has his own introduction and
topics; Boric and Balin intentionally have no dialogue entry yet.
