# Boric

Approved identity: [v2 concept](../../../docs/mockups/boric-v1/README.md).
Production animation artwork generated with built-in imagegen; exact prompts
sit beside each master in `animation/sheets/`. No superseded concept is used.
Runtime appearance approved (2026-11-06); dialogue idle correction reviewed separately.

Idle has two poses in each cardinal direction; Walk four per direction;
Talk four front poses. Boric and Balin also have four front Laugh poses.
Talk/Laugh fall back to directional Idle elsewhere. Side labels use screen-facing
directions: left noses point left, right noses point right.

`tools/register_resident_art.py` measures masters without changing their pixels.
Registration uses one uniform scale per sheet, 800×608 canvases, feet at (400,584),
and distinct human-scale body heights: mayor 464, Boric 500, Balin 420 pixels.
Existing C++ extraction and cropped mip publication derive runtime images in the
build tree. Source sheets, exact prompts and registration remain durable inputs.

See [Sanctuary resident design](../../../docs/sanctuary_residents_design.md)
for placement, dialogue, compatibility, verification and remaining review status.
