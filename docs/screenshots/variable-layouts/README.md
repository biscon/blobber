# Variable stone layouts — review gallery

Generator/catalog 8/6; campaign schema/simulation 6/10. Implementation is verified;
interactive play and slice acceptance remain user review.

- [All twelve ordinary floors](all-floors.png): seeds 0–3, depths 1–3.
- [Depth 1](depth-1.png), [depth 2](depth-2.png), [depth 3](depth-3.png).
- [Both frozen fallback layouts](fallbacks.png).
- [First-person contact sheet](first-person-contact.png); [full-size captures](first-person/).
- [Pacing summary](metrics.json), [capture identities](captures.json).

Map markers: E entrance, G guardian chamber, O exit/objective, B optional branch.
Gold squares are ordinary doors/gates; orange squares are keyed sector doors.
Full maps are diagnostic; normal player discovery is not changed.

The JSON companions contain actual region cells/bounds, sockets, passage routes,
fixtures, population, fingerprints, retries and quality/memory metrics. The capture
source floors are depth 3, seeds 0 and 5; their identities are in `captures.json`.
`capture-seed-5-depth-3.json` supplies the additional ordinary floor used for the
chamber-network first-person images. All capture cameras use in-process preparation
poses in an application-owned hidden window, without OS input or desktop control.

The images use current production dungeon materials and NPC art. No new decoration
art was added. Final CPU integration checks passed 12/12; hidden raylib/OpenGL
captures passed GL and world-resource cleanup checks. See the [design and executed
verification](../../procedural_layout_design.md) for commands, memory accounting and
limitations. Current sample medians are 58/83.5/70 progression steps; this is a
structural/pacing review, not a combat balance study.
