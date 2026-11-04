# Sanctuary city — visual exploration v1

Generated 2026-11-04 with the built-in ImageGen tool for user review before
implementation planning. These are proposed art targets, not engine screenshots.
The user favors an older, slightly overgrown town with occasional two-storey houses.
The user endorsed these as useful visual references on 2026-11-04, acknowledging
that generated images are an art target rather than an exact runtime result.
No engine or gameplay changes accompany them. The agreed delivery direction is
recorded in the [four-slice city roadmap](../../sanctuary_city_plan.md).

## References

- [Fountain square](01-fountain-square.png): modest octagonal fountain, market
  stalls, planted tree, mostly low houses and one two-storey landmark. Clear
  routes around the fountain and between buildings.
- [Residential lane](02-residential-lane.png): close three-quarter view of a
  deep stone arch, recessed iron-strapped timber doors, stepped roof heights,
  plaster and masonry variations, ivy and a sheltered planted courtyard.
- [Exact generation prompts](prompts.json).

## Visual vocabulary to evaluate

Warm honey-grey limestone, weathered pale ochre plaster, dark slate roofs,
substantial stone voussoirs, dark oak doors, dull iron straps and ring handles,
small shuttered windows, timber eave brackets, restrained olive greenery.
Age should read as wear and modest plant growth in a still-used town.
The user's Wizardry 7 images supplied architectural inspiration; these new
compositions establish an original town direction rather than reproduce a scene.

## Notes for eventual modeling and planning

Use these for proportions, materials and architectural relationships, not as
literal map layouts or camera calibration. Validate scale and visibility using
the actual engine camera and discrete walk positions before building the full kit.
Plan flat grid-aligned streets and explicit building/prop footprints. Keep
walkable routes clear of foliage, stalls and fountain geometry.

Model real arch depth, recessed leaves, simple roofs and readable silhouettes.
Match the existing painted-prop workflow and texture density described in
[the authoring guide](../../prop_modelling_guide.md). Generated shading, water and
foliage are visual aspirations; they do not establish runtime rendering support.
The lane image shows adjacent single doorways, not a validated double-door design.
Model/texture repetition, geometry intersections and exact paving scale still
need authored decisions rather than literal tracing of generated pixels.

House interiors and submap travel remain later work. These references do not
constitute a detailed implementation plan; see the roadmap for staged delivery.
