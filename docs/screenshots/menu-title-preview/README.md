# Stone & Shadow title — production captures

Status: **artwork accepted; implemented and mechanically verified** (2026-11-08).
The user approved the image title and requested removal of the Continue summary
and a centered, brighter navigation hint. Final interactive review is not claimed.

Five application-owned captures from the unmodified production rendering path.
The title is the prepared image texture, the Continue summary is removed, and the
navigation hint is centered in bright ivory with a one-pixel dark shadow. The
runner sets Continue availability directly without reading any user save.

- [Main menu without Continue](main-menu.png)
- [Main menu with Continue](main-menu-continue.png)
- [Main menu without Continue, 1280×720](main-menu-1280.png)
- [Main menu with Continue, 1280×720](main-menu-continue-1280.png)
- [Library submenu, without the main-menu title](character-library.png)

[Source artwork, exact prompt and runner](../../mockups/menu-title-v1/README.md)
are retained. These replace the two initial pre-integration previews while
preserving the user's artwork approval. Hidden-window, preparation retry/reuse,
C++ frame allocation, GPU lifetime and OpenGL checks passed; interactive desktop
review was not performed. See [design and verification](../../menu_launch_design.md#stone--shadow-title-artwork--2026-11-08).
