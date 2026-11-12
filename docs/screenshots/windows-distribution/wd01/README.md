# WD01 Windows distribution captures

Status: **WD01 accepted and closed by the user, 2026-11-12**, following working
Windows gameplay review and verified GUI/download corrections.
These six selected 1920×1080 application-owned hidden captures retain the initial
user-played build provenance. They come from its exact extracted
Windows ZIP. Seed 5, default party/quality. Native MSVC game on hosted Windows
Server 2022; external Mesa 26.2.6 llvmpipe (LLVM 23.1.4, 256 bits), OpenGL 4.6 Core.
The install was read-only, source/build assets were denied by checked NTFS DACLs,
and LocalAppData/TEMP/CWD were isolated under Unicode paths containing spaces.
No OS input injection, focus control, physical GPU or audio playback was used.

[Successful manual run](https://github.com/biscon/blobber/actions/runs/43153009429),
source `03159b3fc29f3a2bfbca8abab982d8802c12f8c2`.
ZIP SHA-256: `b8c90c9eff720cf4ee313ae3f37f347e93a72195cecb0f1500fb783241adabc5`.
New Campaign/save and fresh-process Continue both passed; the hidden bidirectional
depth-copy/readback/resource-cleanup check also passed. These images were inspected
for loaded content and layout; they do not establish Windows 11 physical-GPU or
audio-device behavior. Dialogue intentionally includes partly revealed text.

| Image | Distinct coverage |
| --- | --- |
| [01-menu.png](01-menu.png) | Menu and prepared title artwork |
| [02-welcome.png](02-welcome.png) | New-campaign welcome modal and fonts |
| [03-sanctuary.png](03-sanctuary.png) | Sanctuary, residents, portraits and HUD |
| [04-dialogue.png](04-dialogue.png) | Mayor dialogue, fonts and portraits |
| [05-inn.png](05-inn.png) | Authored inn interior after doorway travel |
| [06-dungeon.png](06-dungeon.png) | Generated floor one after stair travel |

The current GUI/download correction passed [run 43169845969](https://github.com/biscon/blobber/actions/runs/43169845969).
Five new captures are byte-identical to this set; the animated menu variation was
inspected and shows the same loaded content/layout. Keep these six selected images
for unchanged rendering; current captures also remain in optional review evidence.
The current intentional ZIP/checksum/build/test/smoke bundle is retained under
`build-windows-review/dist/blobber-0.1.0-g8259b872306f-windows-x86_64/`.
The player filename inside is plain `blobber-0.1.0-windows-x86_64.zip` and the
hosted game download has no ZIP wrapper or reports/screenshots.
See the [design/handoff](../../../linux_distribution_wd01.md#selected-review-artifact)
for exact evidence, limits and the Windows 11 review recipe. Accepted LD03 Linux
captures remain unchanged. The user explicitly accepted WD01; no review is pending
for this slice.
