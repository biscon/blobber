# LD03 Linux distribution captures

Status: **LD03 accepted and closed by the user, 2026-11-11.**
The user reports that all seems to work and explicitly accepts the slice.
Six selected 1920×1080 application-owned hidden captures from the extracted final
archive, with its installation mounted read-only and source/build assets absent.
Seed 5, default party, default quality. Ubuntu 22.04 / glibc 2.35; Mesa 23.2.1
llvmpipe, OpenGL 4.5 under Xvfb. No host desktop input, display, GPU or audio device.

Archive SHA-256: `ea9893ebafff0c20b0e8a778b2d2762f111f07dae57692113bfc319cdd7c71a7`.
The same archive also passes New Campaign/save/fresh-process Continue on Debian 13;
these six images show the Ubuntu run only. The dialogue capture intentionally
includes partially revealed typewriter text. Captures were inspected for loaded
content/layout; the user subsequently accepted LD03. These captures do not establish a broader
physical GPU compatibility matrix.

| Image | Distinct coverage |
| --- | --- |
| [01-menu.png](01-menu.png) | Menu and prepared title artwork |
| [02-welcome.png](02-welcome.png) | New-campaign welcome modal |
| [03-sanctuary.png](03-sanctuary.png) | Sanctuary world, residents and native HUD |
| [04-dialogue.png](04-dialogue.png) | Mayor dialogue, fonts and portraits |
| [05-inn.png](05-inn.png) | Authored inn interior after doorway travel |
| [06-dungeon.png](06-dungeon.png) | Generated floor one after stair travel |

The final archive/checksum/build/verification bundle is retained intentionally under
`build-linux-baseline/dist/blobber-0.1.0-g835be710b30e-dirty-linux-x86_64/`.
See [design/handoff](../../../linux_distribution_ld03.md) for commands, results,
limits and the interactive review recipe. All other captures/extracts are disposable.
