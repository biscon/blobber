# Retained dependency notices

These are unchanged copies of notices available in the configured development
environment on 2026-11-11, not a license for Blobber:

- `raylib.txt`: pinned raylib 6.2 source `LICENSE`.
- `glfw.txt`: that raylib source's `src/external/glfw/LICENSE.md`.
- `yaml-cpp.txt`: installed yaml-cpp `/usr/share/licenses/yaml-cpp/LICENSE`.
- `nlohmann-json.txt`: installed JSON 3.12.0 `/usr/share/licenses/nlohmann-json/LICENSE.MIT`.

The runtime inventory also retains the repository's DejaVu license, SRD attribution,
music README/artist credits and selected audio replacement provenance. Authoring
provenance is retained here only where it records existing attribution/license
information; it is not treated as an executable runtime reference.

LD02 must reconcile notices with its actual pinned dependencies and audit raylib's
embedded codecs/other third-party code. IBM Plex and Inconsolata font notices are
absent from the current asset directory. Music source/license documentation and the
audio replacement model's linked terms have not been independently verified.
These are recorded distribution gaps, not a claim that publication is cleared.
Do not invent end-user terms or add a project open-source license to address them.

LD02 reconciles raylib, bundled GLFW, yaml-cpp and nlohmann/json notices against
its actual hash-pinned sources. `packaging/linux/notices.py` reproduces/checks
these exact copies inside the baseline image. `raylib-components.txt` retains
embedded header/library notice blocks (including optional formats conservatively),
QOI's attribution and MIT permission text. It records source-header SHA-256 values.
`gcc-runtime-copyright.txt` is the baseline gcc-11-base package's full copyright
file, including GCC Runtime Library Exception 3.1; `gcc-runtime-GPL-3.txt` is its
referenced GPLv3 text. These concern dependencies, not Blobber's project license.
The font/music/audio gaps above remain unresolved and publication is not cleared.
