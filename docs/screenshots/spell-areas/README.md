# Precise spell area review — 2026-11-08

Implementation is mechanically verified; user acceptance of appearance and balance is pending.
These six 1920×1080 production-renderer captures use a hidden application-owned window,
a deterministic four-goblin fixture, and legal spell preparation. No desktop input or focus
automation was used. They replace disposable captures retained only until task cleanup.

- [Shatter preview](shatter-preview.png): ten-foot radius, twelve five-foot squares; corner squares excluded.
- [Shatter result](shatter-result.png): the four previewed occupants receive outcomes.
- [Grease preview](grease-preview.png): four subcells straddle four different dungeon cells.
- [Grease result](grease-result.png): matching persistent footprint and failed-save knockdown.
- [Sleep preview](sleep-preview.png): thirty-foot square, thirty-six subcells.
- [Web preview](web-preview.png): twenty-foot square, sixteen subcells.

All templates are aimed at intersection (12,12). Preview outlines include all four
occupied subcells, including visually overlapped goblins. GPU error, prepared-frame
C++ allocation and resource release checks pass. See [design and verification](../../spell_area_design.md).
