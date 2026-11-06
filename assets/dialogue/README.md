# Dialogue scripts

Edit `mayor.yaml` or `innkeeper.yaml`, then restart the game. Each host file
contains its cast and complete conversation. `--dialogue-dir DIRECTORY` selects
an alternative directory of `.yaml`/`.yml` scripts; legacy JSON is no longer read.
Use printable ASCII for game text. YAML comments and `|` multiline blocks are
supported. Participant names are UI labels; `resident` is the existing stable NPC
identity. A guard listed in a cast does not gain a separate conversation prompt.

Each response has an ordered `beats` list. Every beat requires `speaker` (a cast
alias) and `text`. Optional fields:

- `camera: {target: boric, duration_ms: 200}` changes target or transition duration.
  Target defaults to the speaker. Changed targets and returns both take 200 ms;
  unchanged targets hold. Shorter authored durations (including zero) are clamped
  to 200 ms. Obstructed camera paths still use a safety cut.
- `animation: laugh` explicitly loops an existing `idle`, `talk`, `walk` or `laugh`
  clip. Talk and Laugh run only while text is revealing, then return to natural
  Idle, including click-to-complete. Wrapped pages pause in Idle and resume the
  clip while revealing the next page. Walk holds until the next beat without
  moving the actor. Missing explicit clips make the response unavailable;
  missing default Talk art uses Idle.
- `audio: dialogue/boric-chuckle.wav` starts one prepared sound at beat entry.
  Paths are relative to the game's `assets/audio` directory and must stay inside
  it, including when scripts are loaded with `--dialogue-dir`. Missing
  files produce a warning and silence. Omitted audio is silent. Next beat stops
  the previous sound. No sound restarts when a long beat wraps onto another page.

Participant `text_color: '#9FD6D3'` changes speech color. Optional participant
`camera: {distance: 0.6, eye_height: 0.55}` overrides automatic eye-level framing.
Omit eye height to adapt to the character's prepared artwork.

Topics, `when_fact` and `set_fact` retain the existing host-owned history contract.
A topic needs one unconditional response and may have one fact-conditioned variant.
Its fact is recorded when the last page finishes revealing. Rewriting dialogue or
adding interruptions does not invalidate campaign history. Greeting/introduction
cannot set facts. The guards topic demonstrates a repeat-visit running gag.

Optional file-level `presentation` supplies `transition_ms` (700), `fade_ms` (150)
and `characters_per_second` (59). `transition_ms` must be at least 200. These are catalog-wide settings; declarations in
multiple files must agree. Text reveals gradually; Next/Space reveals the rest,
then advances. The Next button appears on multipage responses and disables after
the final page is fully revealed. Clicking the speech panel does nothing. Space
also reveals single-page responses. Esc leaves; topic clicks can interrupt any
response, including the introduction.

Limits: 1 MiB total scripts, 64 hosts, 8 cast members/host, 16 topics/host,
64 facts/host, 32 beats/response, 1024 bytes/beat, 64 prepared pages/response and
64 distinct sound files. Errors identify the file, response and beat/line.

`../audio/dialogue/` contains provisional synthetic character reactions, not final voice
performances. Source/provenance is retained alongside the clips for replacement.

## Persistent pictures

A beat may show `overlay_asset: dialogue/onlyscrolls_overlay.png` or use
`clear_overlay: true` (never both). Paths are relative to the shared `assets/ui`
root, including when the script directory is overridden. By default pictures appear with
the first visible text, persist across following beats/pages, and clear on a new
response, skip or exit. The portrait is centered above the dialogue controls.
`overlay_delay_ms` delays the show relative to the beat start (0..60000; default 0).
`overlay_dismiss_on_advance: true` dismisses it on the first Next/reveal input;
an input before the delay cancels the pending show. The same input still reveals
text or advances a page normally. Neither pagination nor subsequent uncued beats
restore a dismissed image. The default is false; natural text completion alone
does not dismiss pictures. Both options require `overlay_asset`. PNGs are prepared before play: up to 16 unique
images, at most 2048 pixels per axis and 32 MiB decoded RGBA combined.

`pull_document` and `explosive_rage` require matching resident artwork. Document
pull plays once and holds its last frame; rage loops. Both return to Idle when
text reveal finishes or is completed by a click, like Talk/Laugh. Their beat clock
continues across page pauses, so the document is not pulled out again per page.

The Gwendolyn document beat uses these reusable cues so the camera pan and full
600 ms document-pull clip remain visible before the close-up:

```yaml
animation: pull_document
overlay_asset: dialogue/onlyscrolls_overlay.png
overlay_delay_ms: 600
overlay_dismiss_on_advance: true
```

Timing belongs to the script: adjust the delay if the authored animation or pan
duration changes. The engine does not recognize conversation or character names.
