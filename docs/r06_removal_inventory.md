# R06 removal inventory

The following 124 tracked source/tool/test/asset files are deleted, not disabled.
Useful shared regressions were ported to world tests, animation tests, live-session
command tests and graphics-independent automation allocation tests. Source artwork,
shared sheet/image preparation, native UI atlases and world publications remain.

The managed-output retirement script removes only known obsolete banks, their
temporary files, profile-C derivatives, copied preparation profile and authored-demo map/recovery content. Clean and
seeded incremental fixtures cover standard/custom multi-config layouts, idempotence
and external-directory protection. The one-time cutover also removed 160 retired
executables/libraries/object directories/files from existing project-managed builds.
A final symbol audit inspected 327 root binaries/libraries and removed 108 additional
stale outputs containing retired renderer/demo code; stale modified-source objects
were also discarded. Current Release and clean-build executables are preserved.

```text
assets/maze.txt
assets/preparation.json
assets/rules/recovery.json
src/app/actor_captures.cpp
src/app/actor_captures.hpp
src/app/ai_demo.cpp
src/app/ai_demo.hpp
src/app/archer_captures.cpp
src/app/archer_captures.hpp
src/app/arena_captures.cpp
src/app/build_captures.cpp
src/app/build_captures.hpp
src/app/captures.cpp
src/app/captures.hpp
src/app/creator_captures.cpp
src/app/creator_captures.hpp
src/app/door_captures.cpp
src/app/door_captures.hpp
src/app/environment_review.cpp
src/app/environment_review.hpp
src/app/guardian_captures.cpp
src/app/guardian_captures.hpp
src/app/hud_captures.cpp
src/app/hud_captures.hpp
src/app/hybrid_demo.cpp
src/app/hybrid_demo.hpp
src/app/inventory_captures.cpp
src/app/inventory_captures.hpp
src/app/progression_captures.cpp
src/app/progression_captures.hpp
src/app/projectile_captures.cpp
src/app/projectile_captures.hpp
src/app/recovery_demo.cpp
src/app/recovery_demo.hpp
src/app/save_captures.cpp
src/app/save_captures.hpp
src/app/spell_captures.cpp
src/app/spell_captures.hpp
src/app/spell_review_captures.cpp
src/app/starting_build_diagnostics.cpp
src/app/starting_build_diagnostics.hpp
src/app/ui_demo.cpp
src/app/ui_demo.hpp
src/app/world_interaction_preview.cpp
src/app/world_interaction_preview.hpp
src/app/world_preview.cpp
src/app/world_preview.hpp
src/assets/material_loader.cpp
src/assets/material_loader.hpp
src/assets/npc_derivatives.cpp
src/assets/npc_derivatives.hpp
src/assets/npc_loader.cpp
src/assets/npc_loader.hpp
src/assets/preparation_profile.cpp
src/assets/preparation_profile.hpp
src/assets/prepared_assets.hpp
src/assets/prepared_binary.cpp
src/assets/prepared_binary.hpp
src/assets/prepared_loot.cpp
src/assets/prepared_loot.hpp
src/assets/prepared_npcs.cpp
src/assets/prepared_publication.cpp
src/assets/prepared_scene.cpp
src/assets/recovery_loader.cpp
src/assets/recovery_loader.hpp
src/automation/visual.cpp
src/automation/visual.hpp
src/probe/assets.cpp
src/probe/assets.hpp
src/probe/gpu.cpp
src/probe/gpu.hpp
src/probe/legacy.cpp
src/probe/legacy.hpp
src/probe/review.cpp
src/probe/review.hpp
src/probe/scene.cpp
src/probe/scene.hpp
src/render/frame.hpp
src/render/frame_pipeline.cpp
src/render/frame_pipeline.hpp
src/render/lighting.hpp
src/render/loot_sprites.cpp
src/render/loot_sprites.hpp
src/render/material.cpp
src/render/material.hpp
src/render/npc_preparation.cpp
src/render/npc_projection.hpp
src/render/npc_sprites.cpp
src/render/npc_sprites.hpp
src/render/npc_travel.cpp
src/render/preparation_limits.hpp
src/render/projectiles.cpp
src/render/projectiles.hpp
src/render/projection.hpp
src/render/scene_cache.cpp
src/render/scene_cache.hpp
src/render/scene_renderer.cpp
src/render/scene_renderer.hpp
src/render/spectral_weapons.cpp
src/render/transitions.cpp
src/render/transitions.hpp
tests/actor_render_tests.cpp
tests/automation_visual_tests.cpp
tests/fixtures/regression_maze.txt
tests/fixtures_render_tests.cpp
tests/frame_allocations.cpp
tests/loot_render_tests.cpp
tests/npc_animation_tests.cpp
tests/npc_reactions_tests.cpp
tests/npc_tests.cpp
tests/picking_tests.cpp
tests/prepared_assets_tests.cpp
tests/probe_cpu_tests.cpp
tests/probe_gpu_tests.cpp
tests/projectile_tests.cpp
tests/render_tests.cpp
tests/resource_tests.cpp
tests/support/npc_fixtures.hpp
tests/support/probe_allocations.hpp
tests/support/render_fixtures.hpp
tools/bake_loot.cpp
tools/bake_runtime_assets.cpp
tools/renderer_probe.cpp
tools/retain_probe_review.py
```

Final source/CMake audit: no SceneRenderer, FramePipeline, PreparedNpcs, old bank
loaders/bakers, renderer selectors, R01 executable probe, standalone demo dispatch
or public height showcase implementation remains. Negative option/replay tests and
managed-output cleanup fixtures necessarily name retired interfaces. Historical
designs and galleries are evidence only. See [verification](r06_verification.md).
