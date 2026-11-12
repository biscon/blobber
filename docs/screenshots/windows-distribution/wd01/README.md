# WD01 Windows distribution captures

Status: **Ready for review — user acceptance pending, 2026-11-12.**
Six selected 1920×1080 application-owned hidden captures from the exact extracted
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

The selected ZIP/checksum/build/test/compatibility/smoke bundle is retained under
`build-windows-review/dist/blobber-0.1.0-g03159b3fc29f-windows-x86_64/`.
See the [design/handoff](../../../linux_distribution_wd01.md#selected-review-artifact)
for exact evidence, limits and the Windows 11 review recipe. Accepted LD03 Linux
captures remain unchanged. Only explicit user acceptance closes WD01.
