# SE01 authored spell artwork

Generated with the built-in ImageGen tool on 2026-11-05, then registered and packed
offline by `python3 tools/prepare_spell_effects.py`. Source masters are durable inputs;
body.png and emission.png are runtime publication inputs. No new icons or NPC art.

Each atlas has eight rows of eight frames: fire flight, fire impact, attached flame,
frost flight, frost impact, frost coating, Burning Hands fan, particles (four embers,
four ice shards). Tiles are 128²; content occupies the central 112² with an eight-pixel
gutter. Images use straight RGBA; the world baker produces premultiplied mip chains.
Sampling is capped at mip 3. Both atlases share registration and the emission mask
is intersected with body alpha. Frame cadence starts at 80 ms; phase-specific fades
and movement are sampled by the engine. Low-alpha generation residue is removed
only during offline preparation. Source sheets are 1254²; no assumed source tile size.

## Prompts and provenance

Original body prompt: production 2D game VFX sprite sheet, square transparent 8×8
cells; each row eight coherent animation frames. Painted pixel fantasy dungeon,
visible clusters, rough dark coloured edges, restrained cores. Rows: asymmetric
orange Fire Bolt pointing right; outward fire impact; attached upright curling
flames; narrow cyan ice lance pointing right; angular shard impact; sparse patchy
frost coating; coherent spreading upright fan of flame tongues; four ember and four
ice-shard particles. Consistent registration, transparent margins, no labels,
grid, circles, crosses, vector icons, bodies, background or excessive bloom.
Original output retained as source/body-master.png.

Cleanup edit prompt (body-master reference): preserve exact 8×8 row identities,
frame locations/directions/proportions; remove coloured background noise/haze and
speckles, keep central painted sprites with dark edges and compact cores, keep
sprites inside central 70% of cells. Make row-six coating sparse disconnected
crystal patches instead of cloudy disks. Preserve fire/frost palette and animation.
Selected result: source/body-clean.png.

Emission edit prompt (body-clean reference): preserve exact atlas/frame registration;
retain only small yellow-white fire cores and icy cyan-white highlights, remove
outer bodies/smoke/haze, transparent elsewhere, faint coating highlights, no large
bloom/halos, emission remains inside corresponding body silhouette.
Result: source/emission-master.png.

In-engine captures and hidden checks are complete; [selected motion/stills](../../../docs/screenshots/spell-effects/README.md)
were accepted by the user on 2026-11-05, particularly the local-light treatment.
