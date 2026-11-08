# Music

The supplied MP3s are played directly; no authoring/model tools are
needed on a player's machine. Original downloads are unchanged. Titles are readable
versions of their filenames (these files do not contain embedded title/artist tags).

In **Settings / Audio**, use **Music** to drag the volume slider, or select it
with Tab and adjust with Left/Right. The separate On/Off button remembers the chosen
volume. Music starts enabled at 50%. Master also controls music. Pause/settings and
focus loss freeze playback; spell selection and other tactical panels keep it going.

Each encounter starts the next shuffled track from the beginning. Longer fights
advance through tracks. Ending combat fades only the current song for up to five
seconds; a new encounter interrupts that fade with the next song. Muting freezes the
current track, which resumes if the same encounter is still active. Dungeon
ambience pauses while combat music is audible, including its ending fade, then
resumes from its held position. Muted/zero-volume/unavailable combat music leaves
ambience available under its normal focus/pause rules. Muted encounters
do not consume playlist entries. The title announcement is hidden while muted.

Sanctuary, the inn, smithy and Arcane Emporium share the ten supplied tracks in
`sanctuary/`. Music plays continuously, advancing through a separate shuffled
rotation. City/interior doorway travel keeps the current song playing at its
normal level through visual fades and loading; the window thread refills its
prepared stream while the worker loads. Returning from the dungeon starts the
next sanctuary track.
Dialogue and inventory/character overlays defer announcements while music continues.
The ambient hiss is disabled throughout town, including when music is muted.

The supplied `mainmenu/` song plays once when menus first appear, continues across
connected screens (including Settings), then leaves silence when finished. Returning
from a campaign or arena starts it again from the beginning. Launching gameplay
stops it before loading. Menu music has no title announcement. All three banks share
the Music/Master controls; muting or losing focus suspends playback.

## Editing the bank

Edit `playlist.json` to change a display title, artist, relative path or `gain_db`.
Keep the `id` when replacing a song to preserve its place in the rotation. Add or
remove manifest entries to change the playlist (up to 64 tracks); simply dropping
an unlisted file into the directory does not activate it. Missing/invalid files are
skipped with a startup diagnostic. Paths stay within this music directory. Use
printable ASCII titles for the existing HUD font atlas; long titles wrap/truncate.

The combat manifest remains here. Sanctuary and menu each have their own
`playlist.json` inside their directory; their paths are relative to that directory.
Each bank accepts at most 64 entries. Keep the menu manifest to its one desired song.
The supplied additional credits follow their filenames: Nick Panek for sanctuary,
Surprising Media for the menu. No additional license verification is claimed.

`gain_db` is an attenuation-only trim (-60 through 0 dB), independent of the Music
slider. Combat tracks use 0 dB. Sanctuary and main-menu tracks use -4 dB to
balance menu/town music against combat while keeping effects prominent.
At 100% Master/Music,
runtime compensates raylib's centered-stereo attenuation so playback is at the
source level with the authored trim before the final limiter. Measurements remain beside each track for
reference; they do not impose a normalization target. The final limiter protects
combined effects/music sample peaks. Listen and adjust the slider to taste.

To remeasure the MP3s under `combat/` without changing audio bytes:

```sh
nice -n 15 ionice -c 3 python3 tools/audio/music_levels.py
# Select a different bank explicitly:
nice -n 15 ionice -c 3 python3 tools/audio/music_levels.py --bank sanctuary
nice -n 15 ionice -c 3 python3 tools/audio/music_levels.py --bank mainmenu
```

This rebuilds the manifest from that directory, preserving existing IDs/titles/artist
and manual trims for matching paths, updating measurements only. New tracks get
0 dB gain. It requires FFmpeg offline.
Set newly added sanctuary/menu tracks to -4 dB to match their current balance.

The shuffled order and cursor live in `music-state.json` beside your settings
(normally `~/.config/blobber/`). It is independent of campaign saves and is flushed
on menu exit from gameplay, settings, travel and orderly shutdown. A crash may lose
progress since the last checkpoint. To reset the rotation, remove that file while
Blobber is closed. The next encounter after restarting always starts a new song;
partial song positions are deliberately not saved.

Sanctuary uses a separate `sanctuary-music-state.json` beside settings with the same
rotation/persistence rules. Menu completion is not saved and has no state file.

Source/provenance: tracks supplied by the user as royalty-free Nick Panek downloads.
Original filenames and artist credit are retained; no new license verification or
source attribution beyond the supplied information is claimed.
