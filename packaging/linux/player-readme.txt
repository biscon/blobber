Stone & Shadow — Linux x86-64

Extract the complete archive, then run ./blobber from its game directory, or
invoke that executable by its absolute path from another working directory.
No compiler, source checkout or administrator installation is required.

Requires glibc 2.35 or newer, X11 or XWayland and a working OpenGL 3.3 driver.
Native Wayland, musl/Alpine, ARM and private GPU drivers are not supported.
The fixed game presentation is 1920 x 1080. Run ./blobber --help for options.
Use --no-audio if an audio device is unavailable.

New Campaign starts with the party picker. Continue restores the saved expedition.
Settings: $XDG_CONFIG_HOME/blobber/settings.json, or ~/.config/blobber/settings.json.
Characters, scenarios and campaign saves: $XDG_DATA_HOME/blobber/, or
~/.local/share/blobber/. Keep that user data when upgrading. The installation may
remain read-only. Save compatibility follows the game's content/schema versions;
keep your older package when upgrading rather than overwriting it in place.

BUILD.json identifies this build and content. MANIFEST.json lists the other files
and their SHA-256 hashes; the adjacent download .sha256 checks the entire archive.
A name containing -dirty identifies an explicitly permitted development build.
Software-OpenGL verification does not establish physical GPU or audio compatibility.

Dependency and asset notices are included in notices/ and the relevant assets.
Existing font/music/audio provenance gaps are recorded in the development handoff;
this package makes no publication-clearance or project open-source-license claim.
