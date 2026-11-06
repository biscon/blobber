# Cleanup edits (built-in ImageGen)

Each edit referenced its corresponding original master and requested transparency.
Selected outputs are `arcane-clean.png`, `contact-clean.png`, `portrait-clean.png`.

## Contact

Clean this sprite sheet for production. Preserve EXACT 8x8 grid, row identities,
sprite registration and eight sequential frames. Remove ALL background haze,
grey/white fog, halos, color noise and light spill between frames. Every individual
sprite must fit in the CENTRAL 70% of its own cell, surrounded by completely
transparent gutters on all sides, with no rectangular cut-off edges. Keep only
crisp painted pixel-art sprites. Row1 branching electricity is bare blue-white
jagged branches on transparency, no cloudy discs. Row2 necrotic wisps. Row3 acid
droplets. Row4 isolated muted green cloud lobes. Row5 broken pressure semicircles
and small dust at foot. Row6 thunder fracture fragments. Rows7/8 tiny particles.
Do not add labels or grid. No atmospheric glow outside sprite bodies. Make each
final frame sparse and fading.

## Arcane

Production cleanup only. Preserve the exact eight rows/eight columns and all
animation identities. Remove background haze, noise and halos everywhere. Shrink
sprites slightly so every frame fits inside central 70% of its cell with clear
transparent gutters. Preserve rough painted pixel clusters and dark outlines.
Row1 force dart, row2 force impact, row3 THIN fire streak, row4 fire flash, row5
radiant spear, row6 radiant impact, row7 downward sacred flame, row8 sparse gold
coating fragments. Keep all rows aligned to an exact uniform 8x8 grid, no irregular
placements. No clipped shapes, no text, no grid, no scenery. Transparent exterior
around every sprite and clean transparent spaces between coating fragments.

## Portrait

Clean this exact 8-column 4-row animation sheet for game use. Keep same row
identities: green healing upward shimmer, blue protective shimmer, WHITE neutral
damage pulse, violet harmful wisps. Each pair of wisps constitutes ONE frame in one
cell. Center every frame inside its equal cell, keep central portrait space
transparent. Remove ALL colored background noise, glow haze and halos between
cells; isolate each animation frame within central 70% of its cell with generous
transparent gutters. No text, badges, names, frame borders, symbols, characters or
scenery. Crisp painted pixel clusters, sparse delicate wisps with vivid small
edges. Keep neutral row3 white so runtime tint works. Preserve eight sequential
frames and fading ends.

## Pressure-ring correction

Edit ONLY row 5 (the pressure-wave semicircles) of this exact 8 by 8 sprite sheet.
Preserve all seven other rows pixel-for-pixel, their coordinates and colors.
Replace row 5 with eight animation frames of a BROKEN THIN PRESSURE RING /
semicircular rim, seen from front low angle. The center of each semicircle must be
completely transparent; NO filled blue dome or white disk. Use ragged pale
grey-blue shards along a thin irregular perimeter, with small brown dust puffs
along the bottom. Ring grows outward and breaks into fragments over frames,
ending sparse. Keep rim thickness below 15% of radius. Each frame entirely within
central 70% of equal cell, transparent gutters, exact existing grid registration.
Pixel-art painted texture. No haze, blur, vector circles, backgrounds or text.

Only row five of `contact-pressure-master.png` is selected by the offline packer;
all other rows stay byte-identical to the prior cleaned master selection.
