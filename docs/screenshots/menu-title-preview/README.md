# Stone & Shadow title — production captures

Status: **artwork accepted; implemented and mechanically verified** (2026-11-08).
The user approved the image title and requested removal of the Continue summary
and a centered, brighter navigation hint. Final interactive review is not claimed.
The user accepts the gleam's appearance; its cycle is now six seconds. Ken Burns
background motion is implemented and mechanically verified; framing/motion review
is pending. Original artwork acceptance is unchanged.

Five application-owned captures from the unmodified production rendering path.
The title is the prepared image texture, the Continue summary is removed, and the
navigation hint is centered in bright ivory with a one-pixel dark shadow. The
runner sets Continue availability directly without reading any user save.
The no-Continue images use phase zero; Continue images show the sweep midpoint
at two seconds. The shader adds a bevel-weighted gold/champagne sweep and a much
subtler warm variation between passes, preserving texture alpha and surrounding UI.
The background crop gently pans/zooms on a 48-second path; the logo and controls
stay fixed. The midpoint capture uses background time 24 seconds and title time
two seconds, making framing easy to compare with the initial Continue capture.

- [Main menu without Continue](main-menu.png)
- [Main menu with Continue](main-menu-continue.png)
- [Main menu with Continue, 1280×720](main-menu-continue-1280.png)
- [Library submenu, without the main-menu title](character-library.png)
- [Background at its midpoint, native](background-midpoint.png)
- [Twelve-second motion excerpt, 768×432](menu-motion.mp4)

The video excerpt runs at real speed and includes two gleams; the game's
background completes its smooth loop over 48 seconds. Five PNGs and one MP4
replace the previous five PNGs and logo-only WebP.

[Source artwork, exact prompt and runner](../../mockups/menu-title-v1/README.md)
are retained. These replace the two initial pre-integration previews while
preserving the user's artwork approval. Hidden-window, preparation retry/reuse,
C++ frame allocation, GPU lifetime, phase repetition, off-logo pixel preservation,
eight-shader world coexistence, background loop/crop bounds, fixed foreground and
OpenGL checks passed; interactive desktop
review was not performed. See [design and verification](../../menu_launch_design.md#stone--shadow-title-artwork--2026-11-08).
