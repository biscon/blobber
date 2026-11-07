# Stone & Shadow approved title artwork

Status: **artwork accepted for integration** (2026-11-08). The user also requested
removal of the Continue summary and a centered, brighter navigation hint.

- [Generated transparent wordmark](../../../assets/ui/menu-title.png), 2172×724 RGBA.
- [Exact prompt and built-in imagegen provenance](../../../assets/ui/menu-title.prompt.json). Existing
  `assets/ui/menu-background.png` supplied as a style/palette reference only.
- [Production capture runner](../../../tools/review_menu_title.cpp).
- [Current menu captures](../../screenshots/menu-title-preview/README.md).

The approved preview used `GetImageAlphaBorder(image, .1f)` to select source rectangle
`{61, 211, 2058, 270}`, drawn at native `{450, 165, 1020, 134}` without Continue or
`{450, 88, 1020, 134}` with Continue. Production preparation crops the same source
region and uses the same destinations. The temporary overlay preview is superseded
by direct production captures; its obsolete runner has been removed.

Build the opt-in `blobber_review_menu_title` target and run it with two paths:
`blobber_review_menu_title cmake-build-debug/assets NEW_OUTPUT`.
A live display is required, but the window is hidden and checked before and after
capture. No OS input, focus control or user library/save access is used.
