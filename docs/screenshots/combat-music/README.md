# Combat music UI checks

Hidden application-owned native rendering, 1920×1080. The banner tool draws over
an existing game capture (`../r06/combat-start.png`); it does not simulate a new
encounter. Settings uses the prepared build UI assets. No OS input or focus control.

- [Slide entry](entry.png)
- [Scrambled reveal](reveal.png)
- [Settled title](settled.png)
- [Sideways exit](exit.png)
- [Long title](two-lines.png)
- [Music volume and toggle](settings.png)

Inspected for text legibility, title/artist hierarchy, viewport clipping, two-line
layout and separation from party/action controls. Animation timing and listening
remain user review. Reproduce with `blobber_capture_music_banner` and
`blobber_capture_audio_settings`; see tools' command-line arguments.
