cmake_minimum_required(VERSION 3.24)
project(blobber VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BLOBBER_BUILD_GRAPHICS "Build the game, image tools and optional headless visual backend" ON)
if(BLOBBER_BUILD_GRAPHICS)
    option(BLOBBER_USE_SYSTEM_RAYLIB "Prefer an installed raylib 6.2 package" ON)
    if(BLOBBER_USE_SYSTEM_RAYLIB)
        find_package(raylib 6.2 EXACT QUIET)
    endif()
    if(NOT TARGET raylib)
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
        set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(raylib
            URL https://github.com/raysan5/raylib/archive/refs/tags/6.2.tar.gz
            URL_HASH SHA256=df7a13129b1bec9e73a6962efff170beba310ee6eab8f94cd000b7316b4db5ee
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        FetchContent_MakeAvailable(raylib)
    endif()

endif()

option(BLOBBER_USE_SYSTEM_JSON "Prefer an installed nlohmann/json 3.12.0 package" ON)
if(BLOBBER_USE_SYSTEM_JSON)
    find_package(nlohmann_json 3.12.0 EXACT QUIET)
endif()
if(NOT TARGET nlohmann_json::nlohmann_json)
    include(FetchContent)
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set(JSON_Install OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

function(blobber_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(blobber_rules src/rules/progression.cpp src/rules/spell_selection.cpp src/rules/creation.cpp src/rules/spells.cpp src/rules/resolve.cpp src/rules/health.cpp src/rules/content.cpp src/rules/character.cpp)
target_include_directories(blobber_rules PUBLIC src)
add_library(blobber_rules_io src/persistence/campaign_store.cpp src/persistence/campaign_codec.cpp src/persistence/arena_store.cpp src/persistence/library_codec.cpp src/persistence/character_library.cpp src/assets/rules_loader.cpp src/assets/rules_character_loader.cpp src/assets/recovery_loader.cpp)
target_link_libraries(blobber_rules_io PUBLIC blobber_rules PRIVATE nlohmann_json::nlohmann_json)
add_library(blobber_core src/world/surfaces.cpp src/world/environment_maps.cpp src/render/attack_visuals.cpp src/game/checkpoint_validation.cpp src/game/save_commands.cpp src/game/campaign_checkpoint.cpp src/game/party_checkpoint.cpp src/game/navigation_checkpoint.cpp src/game/travel_commands.cpp src/game/floor_digest.cpp src/game/session_transfer.cpp src/game/campaign.cpp src/game/session_objective.cpp src/generation/population.cpp src/world/fixtures.cpp src/game/session_fixtures.cpp src/generation/diagnostic_view.cpp src/game/starting_party.cpp src/generation/generator.cpp src/generation/fixtures.cpp src/generation/templates.cpp src/generation/validation.cpp src/game/selection_controls.cpp src/game/level_spells.cpp src/game/level_effects.cpp src/game/combat_progression.cpp src/game/advancement_commands.cpp src/game/party_progression.cpp src/game/session_progression.cpp src/game/arena.cpp src/game/preparation_commands.cpp src/game/creator.cpp src/game/loot_commands.cpp src/game/recovery_commands.cpp src/game/loot.cpp src/game/recovery.cpp src/game/session_recovery.cpp src/game/character_sheet.cpp src/game/inventory_commands.cpp src/ui/inventory_interaction.cpp src/game/inventory.cpp src/game/combat_items.cpp src/world/map.cpp src/world/npc.cpp src/game/movement.cpp src/game/npc_animation.cpp src/game/party.cpp src/game/actors.cpp src/game/spell_commands.cpp src/game/commands.cpp src/game/hud_state.cpp src/game/conditions.cpp src/game/status_summary.cpp src/game/spell_areas.cpp src/game/effects.cpp src/game/combat.cpp src/game/combat_spells.cpp src/game/combat_actions.cpp src/game/combat_impacts.cpp src/game/combat_loop.cpp src/game/session.cpp src/game/navigation.cpp src/game/npc_ai.cpp src/game/perception.cpp src/ui/interaction.cpp src/input/key_repeat.cpp)
target_include_directories(blobber_core PUBLIC src)
target_link_libraries(blobber_core PUBLIC blobber_rules)
# Level recovery loading validates world cells and exposes core recovery data.
target_link_libraries(blobber_rules_io PUBLIC blobber_core)
add_library(blobber_npc_io src/assets/npc_metadata.cpp)
target_link_libraries(blobber_npc_io PUBLIC blobber_core PRIVATE nlohmann_json::nlohmann_json)
blobber_warnings(blobber_npc_io)
foreach(target blobber_rules blobber_rules_io blobber_core)
    blobber_warnings(${target})
endforeach()

add_library(blobber_world_cpu src/render/world_doors.cpp src/render/world_effects.cpp src/render/world_scene.cpp src/render/world_visibility.cpp src/assets/world_assets.cpp)
target_link_libraries(blobber_world_cpu PUBLIC blobber_core PRIVATE nlohmann_json::nlohmann_json)
blobber_warnings(blobber_world_cpu)

if(BLOBBER_BUILD_GRAPHICS)
    add_library(blobber_render
        src/render/spectral_weapons.cpp
        src/render/loot_sprites.cpp
        src/render/projectiles.cpp
        src/render/image_sampling.cpp
        src/render/npc_preparation.cpp
        src/render/material.cpp src/render/npc_sprites.cpp src/render/npc_travel.cpp src/render/scene_cache.cpp src/render/scene_renderer.cpp
        src/render/transitions.cpp src/render/frame_pipeline.cpp)
    target_link_libraries(blobber_render PUBLIC blobber_core raylib)
    add_library(blobber_assets_io src/assets/creator_art.cpp src/assets/portrait_preparation.cpp src/assets/prepared_loot.cpp src/assets/material_loader.cpp src/assets/npc_loader.cpp src/assets/npc_sheet.cpp src/assets/npc_derivatives.cpp src/assets/preparation_profile.cpp src/assets/prepared_binary.cpp src/assets/prepared_scene.cpp src/assets/prepared_npcs.cpp src/assets/prepared_publication.cpp)
    target_link_libraries(blobber_assets_io PUBLIC blobber_render blobber_npc_io PRIVATE nlohmann_json::nlohmann_json)
    add_library(blobber_platform
        src/platform/gpu_resources.cpp src/platform/world_renderer.cpp src/platform/window.cpp src/platform/input.cpp
        src/ui/save_screen.cpp src/ui/creator_screen.cpp src/platform/presenter.cpp src/ui/loot_screen.cpp src/ui/camp_screen.cpp src/ui/travel_screen.cpp src/ui/character_sheet.cpp src/ui/inventory_screen.cpp src/ui/spellbook.cpp src/ui/hud.cpp src/ui/hud_overlays.cpp src/ui/art.cpp src/ui/fonts.cpp)
    target_link_libraries(blobber_platform PUBLIC blobber_render blobber_world_cpu)
    add_executable(blobber src/app/environment_review.cpp src/app/door_captures.cpp src/app/world_interaction_preview.cpp src/app/world_preview.cpp src/app/save_captures.cpp src/app/campaign_saves.cpp src/app/guardian_captures.cpp src/app/progression_captures.cpp src/app/arena_model.cpp src/app/arena_captures.cpp src/app/arena_setup.cpp src/ui/arena_screen.cpp src/app/creator_captures.cpp src/app/creator_flow.cpp src/app/session_launch.cpp src/ui/menu_screen.cpp src/app/menu.cpp src/app/menu_model.cpp src/app/starting_build_diagnostics.cpp src/app/build_captures.cpp src/app/recovery_demo.cpp src/main.cpp src/app/application.cpp src/app/options.cpp src/app/captures.cpp src/app/actor_captures.cpp src/app/archer_captures.cpp src/app/spell_captures.cpp src/app/spell_review_captures.cpp src/app/projectile_captures.cpp src/app/ui_demo.cpp src/app/gameplay.cpp src/app/hud_captures.cpp src/app/inventory_captures.cpp src/app/ai_demo.cpp src/app/hybrid_demo.cpp)
    target_link_libraries(blobber PRIVATE blobber_platform blobber_assets_io blobber_rules_io nlohmann_json::nlohmann_json)
    add_executable(blobber_prepare_assets EXCLUDE_FROM_ALL tools/prepare_assets.cpp)
    target_link_libraries(blobber_prepare_assets PRIVATE raylib)
    target_include_directories(blobber_prepare_assets PRIVATE src)
    add_executable(blobber_prepare_npc_animation EXCLUDE_FROM_ALL tools/prepare_npc_animation.cpp)
    target_link_libraries(blobber_prepare_npc_animation PRIVATE blobber_assets_io)
    target_include_directories(blobber_prepare_npc_animation PRIVATE src)
    blobber_warnings(blobber_prepare_npc_animation)
    add_executable(blobber_bake_assets tools/bake_runtime_assets.cpp)
    target_link_libraries(blobber_bake_assets PRIVATE blobber_assets_io nlohmann_json::nlohmann_json)
    blobber_warnings(blobber_bake_assets)
    file(GLOB_RECURSE BLOBBER_BAKE_INPUTS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/npcs/*.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/npcs/*.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/*.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/effects/*.png")
    set(BLOBBER_PREPARED_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/prepared")
    if(CMAKE_CONFIGURATION_TYPES)
        set(BLOBBER_PREPARED_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets/prepared")
    endif()
    add_custom_command(
        OUTPUT "${BLOBBER_PREPARED_DIR}/scene.bin" "${BLOBBER_PREPARED_DIR}/npcs.bin"
               "${BLOBBER_PREPARED_DIR}/metadata.json"
        COMMAND blobber_bake_assets "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${BLOBBER_PREPARED_DIR}"
        DEPENDS blobber_bake_assets ${BLOBBER_BAKE_INPUTS}
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/preparation.json" "${CMAKE_CURRENT_SOURCE_DIR}/assets/maze.txt"
        COMMENT "Baking profile-C NPCs and point-sampled dungeon images offline"
        VERBATIM)
    add_custom_target(blobber_bake_runtime_assets
        DEPENDS "${BLOBBER_PREPARED_DIR}/scene.bin" "${BLOBBER_PREPARED_DIR}/npcs.bin"
                "${BLOBBER_PREPARED_DIR}/metadata.json")
    file(GLOB BLOBBER_FIXTURE_ART CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/fixtures/*.png")
    add_executable(blobber_bake_loot tools/bake_loot.cpp)
    target_link_libraries(blobber_bake_loot PRIVATE blobber_assets_io)
    add_custom_command(OUTPUT "${BLOBBER_PREPARED_DIR}/loot.bin"
        COMMAND blobber_bake_loot "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/items/loot-bag.png" "${BLOBBER_PREPARED_DIR}/loot.bin" "${CMAKE_CURRENT_SOURCE_DIR}/assets/fixtures"
        DEPENDS blobber_bake_loot "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/items/loot-bag.png" ${BLOBBER_FIXTURE_ART}
        COMMENT "Baking floor loot sprites offline" VERBATIM)
    add_custom_target(blobber_bake_floor_loot DEPENDS "${BLOBBER_PREPARED_DIR}/loot.bin")
    add_dependencies(blobber blobber_bake_floor_loot)
    add_dependencies(blobber blobber_bake_runtime_assets)
    foreach(target blobber_render blobber_assets_io blobber_platform blobber blobber_prepare_assets)
        blobber_warnings(${target})
    endforeach()

    set(BLOBBER_RAW_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")
    if(CMAKE_CONFIGURATION_TYPES)
        set(BLOBBER_RAW_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets")
    endif()
    file(GLOB_RECURSE BLOBBER_COPY_INPUTS CONFIGURE_DEPENDS LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/rules/*"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/*"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/npcs/*"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/*")
    list(APPEND BLOBBER_COPY_INPUTS preparation.json maze.txt)
    set(BLOBBER_COPY_OUTPUTS)
    foreach(asset IN LISTS BLOBBER_COPY_INPUTS)
        set(source "${CMAKE_CURRENT_SOURCE_DIR}/assets/${asset}")
        set(destination "${BLOBBER_RAW_ASSET_DIR}/${asset}")
        get_filename_component(destination_directory "${destination}" DIRECTORY)
        # Refresh the output timestamp even when a touched source has identical bytes.
        add_custom_command(OUTPUT "${destination}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${destination_directory}"
            COMMAND ${CMAKE_COMMAND} -E copy "${source}" "${destination}"
            DEPENDS "${source}"
            COMMENT "Copying asset ${asset}" VERBATIM)
        list(APPEND BLOBBER_COPY_OUTPUTS "${destination}")
    endforeach()
    add_custom_target(blobber_assets ALL DEPENDS ${BLOBBER_COPY_OUTPUTS})
    add_dependencies(blobber blobber_assets)

    add_executable(blobber_bake_portraits tools/bake_portraits.cpp)
    target_link_libraries(blobber_bake_portraits PRIVATE blobber_assets_io blobber_rules)
    blobber_warnings(blobber_bake_portraits)
    file(GLOB PORTRAIT_MASTERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/portraits/*.png")
    set(PORTRAIT_OUTPUT "${BLOBBER_PREPARED_DIR}")
    add_custom_command(OUTPUT "${PORTRAIT_OUTPUT}/portraits-small.qoi" "${PORTRAIT_OUTPUT}/portraits-large.qoi" "${PORTRAIT_OUTPUT}/portraits.tsv"
        COMMAND blobber_bake_portraits "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui" "${PORTRAIT_OUTPUT}"
        DEPENDS blobber_bake_portraits ${PORTRAIT_MASTERS}
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/alden.png" "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/brunna.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/merrin.png" "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/lethiel.png"
        VERBATIM)
    add_custom_target(blobber_portraits DEPENDS "${PORTRAIT_OUTPUT}/portraits-small.qoi" "${PORTRAIT_OUTPUT}/portraits-large.qoi" "${PORTRAIT_OUTPUT}/portraits.tsv")
    add_dependencies(blobber blobber_portraits)

endif()

add_executable(blobber_build_diagnostic tools/build_diagnostic.cpp)
target_link_libraries(blobber_build_diagnostic PRIVATE blobber_rules_io)
blobber_warnings(blobber_build_diagnostic)

include(CTest)
if(BUILD_TESTING)
    if(BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_options_tests tests/options_tests.cpp src/app/options.cpp)
    target_include_directories(blobber_options_tests PRIVATE src)
    target_link_libraries(blobber_options_tests PRIVATE raylib)
    blobber_warnings(blobber_options_tests)
    add_test(NAME options COMMAND blobber_options_tests)
    add_test(NAME development_cli COMMAND blobber --development --help)
    endif()
    foreach(suite progression_allocations level_spells progression portraits creation library visual_review expanded_spells recovery loot_render inventory spells effects projectile impact prepared_assets image_preparation hybrid ai health combat hud commands picking actors actor_render core render resource frame_allocations npc npc_animation npc_reactions rules rules_content party party_content)
        if(NOT BLOBBER_BUILD_GRAPHICS AND suite MATCHES "^(portraits|visual_review|loot_render|projectile|prepared_assets|image_preparation|picking|actor_render|render|resource|frame_allocations|npc|npc_animation|npc_reactions)$")
            continue()
        endif()
        if(suite STREQUAL "frame_allocations")
            set(source tests/frame_allocations.cpp)
        else()
            set(source tests/${suite}_tests.cpp)
        endif()
        add_executable(blobber_${suite}_tests ${source})
        if(suite STREQUAL "loot_render" OR suite STREQUAL "portraits")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_assets_io)
        elseif(suite STREQUAL "rules" OR suite STREQUAL "health")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_rules)
        elseif(suite STREQUAL "rules_content" OR suite STREQUAL "party_content" OR suite STREQUAL "creation" OR suite STREQUAL "library")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_rules_io blobber_core nlohmann_json::nlohmann_json)
        elseif(suite STREQUAL "progression_allocations" OR suite STREQUAL "level_spells" OR suite STREQUAL "progression" OR suite STREQUAL "expanded_spells" OR suite STREQUAL "recovery" OR suite STREQUAL "inventory" OR suite STREQUAL "spells" OR suite STREQUAL "effects" OR suite STREQUAL "impact" OR suite STREQUAL "hybrid" OR suite STREQUAL "ai" OR suite STREQUAL "hud" OR suite STREQUAL "combat" OR suite STREQUAL "commands" OR suite STREQUAL "actors" OR suite STREQUAL "core" OR suite STREQUAL "party")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_core)
        elseif(suite STREQUAL "resource" OR suite STREQUAL "npc" OR suite STREQUAL "image_preparation" OR suite STREQUAL "prepared_assets")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_assets_io nlohmann_json::nlohmann_json)
        else()
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_render)
        endif()
        blobber_warnings(blobber_${suite}_tests)
        if(suite STREQUAL "render")
            add_test(NAME ${suite} COMMAND blobber_${suite}_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/regression_maze.txt")
        else()
            add_test(NAME ${suite} COMMAND blobber_${suite}_tests)
        endif()
    endforeach()
    if(BLOBBER_BUILD_GRAPHICS)
    target_link_libraries(blobber_frame_allocations_tests PRIVATE blobber_rules_io blobber_assets_io)
    # Explicit desktop check; not part of headless CTest.
    add_executable(blobber_gpu_smoke EXCLUDE_FROM_ALL tests/gpu_smoke.cpp)
    target_link_libraries(blobber_gpu_smoke PRIVATE blobber_platform)
    blobber_warnings(blobber_gpu_smoke)
    endif()
endif()

if(BUILD_TESTING)
    add_executable(blobber_menu_tests tests/menu_tests.cpp src/app/menu_model.cpp)
    target_link_libraries(blobber_menu_tests PRIVATE blobber_rules_io blobber_core nlohmann_json::nlohmann_json)
    blobber_warnings(blobber_menu_tests)
    add_test(NAME menu COMMAND blobber_menu_tests)
endif()

if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    target_sources(blobber_frame_allocations_tests PRIVATE src/app/menu_model.cpp src/app/arena_model.cpp)
endif()

if(BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_bake_creator tools/bake_creator.cpp)
    target_link_libraries(blobber_bake_creator PRIVATE blobber_assets_io)
    blobber_warnings(blobber_bake_creator)
    # Keep in sync with creatorArtNames in src/assets/creator_art.hpp.
    set(CREATOR_MASTERS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/sacred_flame.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/spare_the_dying.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/bless.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/bane.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/shield_of_faith.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/cure_wounds.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/guiding_bolt.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/inflict_wounds.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/fire_bolt.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/acid_splash.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/mage_armor.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/magic_missile.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/burning_hands.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/thunderwave.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/ray_of_frost.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/shocking_grasp.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/poison_spray.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/sleep.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/false_life.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/grease.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/color_spray.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/guidance.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/resistance.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/sanctuary.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/aid.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/hold_person.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/lesser_restoration.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/spiritual_weapon.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/scorching_ray.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/shatter.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/mirror_image.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/spells/web.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/melee.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/items/holy_symbol.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/items/dagger.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/items/spellbook.png"
    )
    add_custom_command(OUTPUT "${BLOBBER_PREPARED_DIR}/creator.qoi"
        COMMAND blobber_bake_creator "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui" "${BLOBBER_PREPARED_DIR}"
        DEPENDS blobber_bake_creator ${CREATOR_MASTERS} VERBATIM)
    add_custom_target(blobber_creator_art DEPENDS "${BLOBBER_PREPARED_DIR}/creator.qoi")
    add_dependencies(blobber blobber_creator_art)
endif()

if(BUILD_TESTING)
    add_executable(blobber_creator_tests tests/creator_tests.cpp src/app/creator_flow.cpp src/app/menu_model.cpp)
    target_link_libraries(blobber_creator_tests PRIVATE blobber_rules_io blobber_core)
    blobber_warnings(blobber_creator_tests)
    add_test(NAME creator COMMAND blobber_creator_tests)
endif()

if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_creator_art_tests tests/creator_art_tests.cpp)
    target_link_libraries(blobber_creator_art_tests PRIVATE blobber_assets_io)
    blobber_warnings(blobber_creator_art_tests)
    add_test(NAME creator_art COMMAND blobber_creator_art_tests)
endif()

if(BUILD_TESTING)
    add_executable(blobber_arena_tests tests/arena_tests.cpp src/app/menu_model.cpp src/app/arena_model.cpp)
    target_link_libraries(blobber_arena_tests PRIVATE blobber_rules_io blobber_core nlohmann_json::nlohmann_json)
    blobber_warnings(blobber_arena_tests)
    add_test(NAME arena COMMAND blobber_arena_tests)
endif()

# Automation keeps its typed simulation boundary independent of protocol allocation and graphics.
add_library(blobber_automation src/automation/session.cpp)
target_link_libraries(blobber_automation PUBLIC blobber_core blobber_rules_io blobber_npc_io)
blobber_warnings(blobber_automation)
add_library(blobber_automation_world_visual src/automation/world_visual.cpp)
target_link_libraries(blobber_automation_world_visual PUBLIC blobber_automation blobber_world_cpu)
blobber_warnings(blobber_automation_world_visual)
add_library(blobber_automation_io src/automation/persistence_trace.cpp src/automation/scenario_codec.cpp src/automation/decode.cpp src/automation/observe.cpp
    src/automation/actions.cpp src/automation/protocol.cpp src/automation/trace.cpp)
target_link_libraries(blobber_automation_io PUBLIC blobber_automation nlohmann_json::nlohmann_json)
blobber_warnings(blobber_automation_io)
add_executable(blobber_headless tools/headless.cpp)
target_link_libraries(blobber_headless PRIVATE blobber_automation_io blobber_automation_world_visual)
blobber_warnings(blobber_headless)
if(BLOBBER_BUILD_GRAPHICS)
        add_library(blobber_automation_visual src/automation/visual.cpp)
        target_link_libraries(blobber_automation_visual PUBLIC blobber_automation blobber_assets_io)
        blobber_warnings(blobber_automation_visual)
        target_sources(blobber_headless PRIVATE src/automation/capture.cpp)
        target_link_libraries(blobber_headless PRIVATE blobber_automation_visual blobber_platform)
        target_compile_definitions(blobber_headless PRIVATE BLOBBER_HEADLESS_VISUAL=1)
endif()
if(BUILD_TESTING)
    add_executable(blobber_automation_tests tests/automation_tests.cpp)
    target_link_libraries(blobber_automation_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_automation_tests)
    add_test(NAME automation COMMAND blobber_automation_tests)
endif()
if(BUILD_TESTING)
    add_executable(blobber_automation_allocations_tests tests/automation_allocations_tests.cpp)
    target_link_libraries(blobber_automation_allocations_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_automation_allocations_tests)
    add_test(NAME automation_allocations COMMAND blobber_automation_allocations_tests)
    if(BLOBBER_BUILD_GRAPHICS)
        target_link_libraries(blobber_automation_allocations_tests PRIVATE blobber_automation_visual)
        target_compile_definitions(blobber_automation_allocations_tests PRIVATE BLOBBER_AUTOMATION_VISUAL=1)
    endif()
    add_executable(blobber_npc_metadata_tests tests/npc_metadata_tests.cpp)
    target_link_libraries(blobber_npc_metadata_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_npc_metadata_tests)
    add_test(NAME npc_metadata COMMAND blobber_npc_metadata_tests)
endif()
if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_automation_visual_tests tests/automation_visual_tests.cpp)
    target_link_libraries(blobber_automation_visual_tests PRIVATE blobber_automation_visual blobber_automation_io)
    blobber_warnings(blobber_automation_visual_tests)
    add_test(NAME automation_visual COMMAND blobber_automation_visual_tests)
endif()
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME headless_cli COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/headless_cli_tests.py $<TARGET_FILE:blobber_headless>)
    set_tests_properties(headless_cli PROPERTIES TIMEOUT 60)
    # Fast policy/transport/report contracts only. Balance playthroughs and even
    # smoke matrices are explicit tools/arena_experiment.py commands, never CTest.
    add_test(NAME arena_experiment_tools COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/arena_experiment_tests.py)
    set_tests_properties(arena_experiment_tools PROPERTIES TIMEOUT 20 LABELS "tooling")
    # Stdlib process/adapter contracts only: no SDK install, engine fight or matrix.
    add_test(NAME mcp_adapter_tools COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcp_adapter_tests.py)
    set_tests_properties(mcp_adapter_tools PROPERTIES TIMEOUT 20 LABELS "tooling")
endif()

# Preparation-only generator inspection: no window, content files or balance simulation.
add_library(blobber_generation_io src/generation/report.cpp)
target_link_libraries(blobber_generation_io PUBLIC blobber_core nlohmann_json::nlohmann_json)
blobber_warnings(blobber_generation_io)
add_executable(blobber_generate_floor tools/generate_floor.cpp)
target_link_libraries(blobber_generate_floor PRIVATE blobber_generation_io)
blobber_warnings(blobber_generate_floor)

target_link_libraries(blobber_automation_io PUBLIC blobber_generation_io)
if(BUILD_TESTING)
    if(BLOBBER_BUILD_GRAPHICS)
        add_executable(blobber_fixtures_render_tests tests/fixtures_render_tests.cpp)
        target_link_libraries(blobber_fixtures_render_tests PRIVATE blobber_assets_io)
        blobber_warnings(blobber_fixtures_render_tests)
        add_test(NAME fixtures_render COMMAND blobber_fixtures_render_tests)
    endif()
    add_executable(blobber_fixtures_automation_tests tests/fixtures_automation_tests.cpp)
    target_link_libraries(blobber_fixtures_automation_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_fixtures_automation_tests)
    add_test(NAME fixtures_automation COMMAND blobber_fixtures_automation_tests)
    add_executable(blobber_fixtures_tests tests/fixtures_tests.cpp)
    target_link_libraries(blobber_fixtures_tests PRIVATE blobber_core)
    blobber_warnings(blobber_fixtures_tests)
    add_test(NAME fixtures COMMAND blobber_fixtures_tests)
    add_executable(blobber_generation_tests tests/generation_tests.cpp)
    target_link_libraries(blobber_generation_tests PRIVATE blobber_core)
    blobber_warnings(blobber_generation_tests)
    add_test(NAME generation COMMAND blobber_generation_tests)
endif()
if(BUILD_TESTING)
    add_executable(blobber_generation_automation_tests tests/generation_automation_tests.cpp)
    target_link_libraries(blobber_generation_automation_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_generation_automation_tests)
    add_test(NAME generation_automation COMMAND blobber_generation_automation_tests)
endif()
if(BUILD_TESTING)
    add_test(NAME generation_cli COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/generation_cli_tests.py
        $<TARGET_FILE:blobber_generate_floor> $<TARGET_FILE:blobber_headless>)
    set_tests_properties(generation_cli PROPERTIES TIMEOUT 30)
endif()

if(BUILD_TESTING)
    add_executable(blobber_guardians_tests tests/guardians_tests.cpp)
    target_link_libraries(blobber_guardians_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_guardians_tests)
    add_test(NAME guardians COMMAND blobber_guardians_tests)
endif()

if(BUILD_TESTING)
    add_executable(blobber_campaign_tests tests/campaign_tests.cpp)
    target_link_libraries(blobber_campaign_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_campaign_tests)
    add_test(NAME campaign COMMAND blobber_campaign_tests)
endif()

if(BUILD_TESTING)
    add_executable(blobber_campaign_save_tests tests/campaign_save_tests.cpp src/app/campaign_saves.cpp)
    target_link_libraries(blobber_campaign_save_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_campaign_save_tests)
    add_test(NAME campaign_save COMMAND blobber_campaign_save_tests)
    set_tests_properties(campaign_save PROPERTIES LABELS "slow;integration" RUN_SERIAL TRUE)
    add_test(NAME campaign_save_smoke COMMAND blobber_campaign_save_tests --smoke)
endif()
if(BUILD_TESTING AND Python3_Interpreter_FOUND)
    add_test(NAME campaign_save_cli COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/campaign_save_cli_tests.py $<TARGET_FILE:blobber_headless>)
endif()

# R01 is an isolated, explicitly built probe; it does not change normal launches.
add_library(blobber_probe_cpu src/probe/scene.cpp)
target_link_libraries(blobber_probe_cpu PUBLIC blobber_world_cpu)
blobber_warnings(blobber_probe_cpu)
if(BLOBBER_BUILD_GRAPHICS)
    add_library(blobber_probe_graphics EXCLUDE_FROM_ALL src/probe/assets.cpp src/probe/gpu.cpp src/probe/legacy.cpp src/probe/review.cpp)
    target_link_libraries(blobber_probe_graphics PUBLIC blobber_probe_cpu blobber_platform blobber_assets_io PRIVATE nlohmann_json::nlohmann_json)
    target_compile_definitions(blobber_probe_graphics PRIVATE BLOBBER_PROBE_ASSETS="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    blobber_warnings(blobber_probe_graphics)
    add_executable(blobber_renderer_probe EXCLUDE_FROM_ALL tools/renderer_probe.cpp)
    target_link_libraries(blobber_renderer_probe PRIVATE blobber_probe_graphics)
    blobber_warnings(blobber_renderer_probe)
endif()
if(BUILD_TESTING)
    add_executable(blobber_probe_cpu_tests tests/probe_cpu_tests.cpp)
    target_link_libraries(blobber_probe_cpu_tests PRIVATE blobber_probe_cpu)
    blobber_warnings(blobber_probe_cpu_tests)
    add_test(NAME probe_cpu COMMAND blobber_probe_cpu_tests)
endif()
if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_probe_gpu_tests EXCLUDE_FROM_ALL tests/probe_gpu_tests.cpp)
    target_link_libraries(blobber_probe_gpu_tests PRIVATE blobber_probe_graphics)
    blobber_warnings(blobber_probe_gpu_tests)
endif()

# R02 publication is independent of the retained perspective banks.
if(BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_bake_world tools/bake_world_assets.cpp)
    target_link_libraries(blobber_bake_world PRIVATE blobber_assets_io blobber_world_cpu nlohmann_json::nlohmann_json)
    blobber_warnings(blobber_bake_world)
    set(BLOBBER_WORLD_DIR "${BLOBBER_PREPARED_DIR}/world")
    include(cmake/WorldAssets.cmake)
    blobber_world_publication(blobber_bake_world_assets blobber_bake_world
        "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${BLOBBER_WORLD_DIR}")
    add_dependencies(blobber blobber_bake_world_assets)
endif()

if(BUILD_TESTING)
    add_executable(blobber_world_cpu_tests tests/world_cpu_tests.cpp)
    target_link_libraries(blobber_world_cpu_tests PRIVATE blobber_world_cpu nlohmann_json::nlohmann_json)
    target_include_directories(blobber_world_cpu_tests PRIVATE tests)
    blobber_warnings(blobber_world_cpu_tests)
    add_test(NAME world_cpu COMMAND blobber_world_cpu_tests)
    if(BLOBBER_BUILD_GRAPHICS)
        add_executable(blobber_world_gpu_tests EXCLUDE_FROM_ALL tests/world_gpu_tests.cpp)
        target_link_libraries(blobber_world_gpu_tests PRIVATE blobber_platform)
        target_include_directories(blobber_world_gpu_tests PRIVATE tests)
        blobber_warnings(blobber_world_gpu_tests)
    endif()
endif()
if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_world_stress EXCLUDE_FROM_ALL tests/world_stress.cpp)
    target_link_libraries(blobber_world_stress PRIVATE blobber_platform)
    target_include_directories(blobber_world_stress PRIVATE tests)
    blobber_warnings(blobber_world_stress)
endif()

if(BUILD_TESTING)
    add_executable(blobber_world_visibility_tests tests/world_visibility_tests.cpp)
    target_link_libraries(blobber_world_visibility_tests PRIVATE blobber_automation_world_visual)
    target_include_directories(blobber_world_visibility_tests PRIVATE tests)
    blobber_warnings(blobber_world_visibility_tests)
    add_test(NAME world_visibility COMMAND blobber_world_visibility_tests)
endif()

if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_world_visibility_gpu_tests EXCLUDE_FROM_ALL tests/world_visibility_gpu_tests.cpp)
    target_link_libraries(blobber_world_visibility_gpu_tests PRIVATE blobber_platform)
    target_include_directories(blobber_world_visibility_gpu_tests PRIVATE tests)
    blobber_warnings(blobber_world_visibility_gpu_tests)
endif()

if(BUILD_TESTING)
    add_executable(blobber_world_visibility_benchmark EXCLUDE_FROM_ALL tests/world_visibility_benchmark.cpp)
    target_link_libraries(blobber_world_visibility_benchmark PRIVATE blobber_world_cpu)
    target_include_directories(blobber_world_visibility_benchmark PRIVATE tests)
    blobber_warnings(blobber_world_visibility_benchmark)
endif()

if(BUILD_TESTING)
    add_executable(blobber_world_visual_fixture tests/world_visual_fixture.cpp)
    target_link_libraries(blobber_world_visual_fixture PRIVATE blobber_world_cpu blobber_npc_io)
    blobber_warnings(blobber_world_visual_fixture)
    add_test(NAME world_visual_cli COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/world_visual_cli_tests.py $<TARGET_FILE:blobber_headless>
        $<TARGET_FILE:blobber_world_visual_fixture>)
    set_tests_properties(world_visual_cli PROPERTIES TIMEOUT 60)
endif()

if(BUILD_TESTING)
    add_executable(blobber_world_visibility_stress EXCLUDE_FROM_ALL tests/world_visibility_stress.cpp)
    target_link_libraries(blobber_world_visibility_stress PRIVATE blobber_world_cpu)
    target_include_directories(blobber_world_visibility_stress PRIVATE tests)
    blobber_warnings(blobber_world_visibility_stress)
endif()

if(BUILD_TESTING)
    add_executable(blobber_world_integration_tests tests/world_integration_tests.cpp src/app/starting_build_diagnostics.cpp)
    target_link_libraries(blobber_world_integration_tests PRIVATE blobber_world_cpu nlohmann_json::nlohmann_json)
    target_include_directories(blobber_world_integration_tests PRIVATE tests)
    blobber_warnings(blobber_world_integration_tests)
    add_test(NAME world_integration COMMAND blobber_world_integration_tests)
endif()
if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_world_integration_gpu_tests EXCLUDE_FROM_ALL tests/world_integration_gpu_tests.cpp)
    target_link_libraries(blobber_world_integration_gpu_tests PRIVATE blobber_platform blobber_automation_io)
    target_include_directories(blobber_world_integration_gpu_tests PRIVATE tests)
    blobber_warnings(blobber_world_integration_gpu_tests)
endif()

if(BUILD_TESTING)
    add_executable(blobber_world_environment_tests tests/world_environment_tests.cpp)
    target_link_libraries(blobber_world_environment_tests PRIVATE blobber_world_cpu)
    target_include_directories(blobber_world_environment_tests PRIVATE tests)
    blobber_warnings(blobber_world_environment_tests)
    add_test(NAME world_environment COMMAND blobber_world_environment_tests)
endif()

if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_world_environment_gpu_tests EXCLUDE_FROM_ALL tests/world_environment_gpu_tests.cpp)
    target_link_libraries(blobber_world_environment_gpu_tests PRIVATE blobber_platform)
    target_include_directories(blobber_world_environment_gpu_tests PRIVATE tests)
    blobber_warnings(blobber_world_environment_gpu_tests)
endif()

# These legacy image-bank fixtures deliberately exercise complete prepared banks.
# Keep coverage available explicitly, without baking hundreds of MiB per ordinary edit.
if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    set_tests_properties(frame_allocations render actor_render loot_render npc_animation
        npc_reactions npc portraits image_preparation prepared_assets
        PROPERTIES LABELS "slow;legacy-render" RUN_SERIAL TRUE)
endif()
