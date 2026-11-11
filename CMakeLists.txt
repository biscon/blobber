cmake_minimum_required(VERSION 3.24)
project(blobber VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BLOBBER_BUILD_GRAPHICS "Build the game, image tools and optional GPU replay capture" ON)
if(BLOBBER_BUILD_GRAPHICS)
    option(BLOBBER_USE_SYSTEM_RAYLIB "Prefer an installed raylib 6.2 package" ON)
    if(BLOBBER_USE_SYSTEM_RAYLIB)
        # Keep incompatible imported targets scoped out so the source fallback remains usable.
        function(blobber_find_audio_raylib)
            find_package(raylib 6.2 EXACT QUIET)
            if(TARGET raylib)
                include(CheckCXXSourceCompiles)
                set(CMAKE_REQUIRED_LIBRARIES raylib)
                unset(BLOBBER_SYSTEM_AUDIO CACHE)
                check_cxx_source_compiles("#include <raylib.h>
void process(void*, unsigned){}
int main(){InitAudioDevice(); Sound s{}; auto a=LoadSoundAlias(s); SetSoundPan(a,-1); UnloadSoundAlias(a); Music m{}; UpdateMusicStream(m); AttachAudioMixedProcessor(process); DetachAudioMixedProcessor(process); CloseAudioDevice();}" BLOBBER_SYSTEM_AUDIO)
                if(BLOBBER_SYSTEM_AUDIO)
                    set_property(TARGET raylib PROPERTY IMPORTED_GLOBAL TRUE)
                else()
                    message(STATUS "System raylib lacks required audio; using pinned fallback")
                endif()
            endif()
        endfunction()
        blobber_find_audio_raylib()
    endif()
    if(NOT TARGET raylib)
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
        set(SUPPORT_MODULE_RAUDIO ON CACHE BOOL "" FORCE)
        set(SUPPORT_FILEFORMAT_MP3 ON CACHE BOOL "" FORCE)
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

option(BLOBBER_USE_SYSTEM_YAML "Prefer a compatible installed yaml-cpp package" ON)
if(BLOBBER_USE_SYSTEM_YAML)
    find_package(yaml-cpp 0.8 CONFIG QUIET)
endif()
if(NOT TARGET yaml-cpp::yaml-cpp)
    include(FetchContent)
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG f7320141120f720aecc4c32be25586e7da9eb978)
    FetchContent_MakeAvailable(yaml-cpp)
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
add_library(blobber_rules_io src/persistence/campaign_store.cpp src/persistence/campaign_codec.cpp src/persistence/arena_store.cpp src/persistence/library_codec.cpp src/persistence/character_library.cpp src/assets/rules_loader.cpp src/assets/rules_character_loader.cpp)
target_link_libraries(blobber_rules_io PUBLIC blobber_rules PRIVATE nlohmann_json::nlohmann_json)
find_package(Threads REQUIRED)
add_library(blobber_core src/game/smithy_interior.cpp src/game/inn_interior.cpp src/world/sanctuary_city.cpp src/audio/music.cpp src/audio/events.cpp src/audio/policy.cpp src/world/surfaces.cpp src/game/sanctuary_hub.cpp src/render/attack_visuals.cpp src/game/checkpoint_validation.cpp src/game/save_commands.cpp src/game/campaign_checkpoint.cpp src/game/party_checkpoint.cpp src/game/navigation_checkpoint.cpp src/game/travel_commands.cpp src/game/floor_digest.cpp src/game/session_transfer.cpp src/game/campaign.cpp src/game/session_objective.cpp src/generation/population.cpp src/generation/encounters.cpp src/generation/encounter_planning.cpp src/world/fixtures.cpp src/game/session_fixtures.cpp src/generation/diagnostic_view.cpp src/game/starting_party.cpp src/generation/blueprint_rooms.cpp src/generation/generator.cpp src/generation/stone_layout.cpp src/generation/fallback.cpp src/generation/metrics.cpp src/generation/encounter_metrics.cpp src/generation/fixtures.cpp src/generation/geometry.cpp
    src/generation/architecture.cpp src/generation/dressing.cpp src/generation/validation.cpp src/game/selection_controls.cpp src/game/level_spells.cpp src/game/level_effects.cpp src/game/combat_progression.cpp src/game/advancement_commands.cpp src/game/party_progression.cpp src/game/session_progression.cpp src/game/arena.cpp src/game/preparation_commands.cpp src/game/creator.cpp src/game/loot_commands.cpp src/game/recovery_commands.cpp src/game/loot.cpp src/game/recovery.cpp src/game/session_recovery.cpp src/game/character_sheet.cpp src/game/inventory_commands.cpp src/ui/inventory_interaction.cpp src/game/inventory.cpp src/game/combat_items.cpp src/world/map.cpp src/world/npc.cpp src/game/movement.cpp src/game/npc_animation.cpp src/game/party.cpp src/game/actors.cpp src/game/target_commands.cpp src/game/spell_commands.cpp src/game/commands.cpp src/game/hud_state.cpp src/game/conditions.cpp src/game/status_summary.cpp src/game/spell_areas.cpp src/game/effects.cpp src/game/enemy_effects.cpp src/game/enemy_spells.cpp src/game/combat.cpp src/game/combat_spells.cpp src/game/spell_visuals.cpp src/game/support_visuals.cpp src/game/combat_actions.cpp src/game/combat_impacts.cpp src/game/damage_resolution.cpp src/game/combat_loop.cpp src/game/session.cpp src/game/navigation.cpp src/game/approach_slots.cpp src/game/npc_ai.cpp src/game/perception.cpp src/ui/interaction.cpp src/ui/button_targets.cpp src/input/key_repeat.cpp)
target_include_directories(blobber_core PUBLIC src)
target_sources(blobber_core PRIVATE src/world/blueprints.cpp src/world/blueprint_report.cpp)
target_sources(blobber_core PRIVATE src/game/emporium_interior.cpp)
target_sources(blobber_core PRIVATE src/game/tactical_navigation.cpp)
target_sources(blobber_core PRIVATE src/game/subcell_area.cpp)
target_sources(blobber_core PRIVATE src/game/dialogue.cpp src/game/dialogue_commands.cpp src/ui/map_model.cpp src/ui/screen_shake.cpp)
target_sources(blobber_core PRIVATE src/game/trade.cpp src/game/trade_inventory.cpp src/game/item_comparison.cpp src/game/trade_commands.cpp src/game/trade_interaction.cpp)
target_link_libraries(blobber_core PUBLIC blobber_rules Threads::Threads)
# Level recovery loading validates world cells and exposes core recovery data.
target_link_libraries(blobber_rules_io PUBLIC blobber_core)
add_library(blobber_npc_io src/assets/npc_metadata.cpp src/assets/dialogue.cpp src/assets/shops.cpp)
target_link_libraries(blobber_npc_io PUBLIC blobber_core PRIVATE nlohmann_json::nlohmann_json yaml-cpp::yaml-cpp)
blobber_warnings(blobber_npc_io)
foreach(target blobber_rules blobber_rules_io blobber_core)
    blobber_warnings(${target})
endforeach()

add_library(blobber_audio_io src/audio/bank.cpp src/audio/music_io.cpp)
target_link_libraries(blobber_audio_io PUBLIC blobber_core PRIVATE nlohmann_json::nlohmann_json)

add_library(blobber_world_assets src/assets/world_assets.cpp src/assets/world_residency.cpp)
target_link_libraries(blobber_world_assets PUBLIC blobber_core PRIVATE nlohmann_json::nlohmann_json)
blobber_warnings(blobber_world_assets)
add_library(blobber_blueprint_io src/assets/blueprint_loader.cpp)
target_link_libraries(blobber_blueprint_io PUBLIC blobber_core PRIVATE nlohmann_json::nlohmann_json)
blobber_warnings(blobber_blueprint_io)
add_library(blobber_blueprint_art src/assets/blueprint_blockouts.cpp src/assets/blueprint_art.cpp)
target_link_libraries(blobber_blueprint_art PUBLIC blobber_blueprint_io blobber_world_assets)
blobber_warnings(blobber_blueprint_art)
add_executable(blobber_blueprints EXCLUDE_FROM_ALL tools/blueprints.cpp)
target_link_libraries(blobber_blueprints PRIVATE blobber_blueprint_io)
blobber_warnings(blobber_blueprints)
if(BUILD_TESTING)
    add_executable(blobber_blueprint_tests tests/blueprint_tests.cpp)
    target_include_directories(blobber_blueprint_tests PRIVATE tests)
    target_link_libraries(blobber_blueprint_tests PRIVATE blobber_blueprint_io blobber_rules_io nlohmann_json::nlohmann_json)
    blobber_warnings(blobber_blueprint_tests)
    add_test(NAME blueprints COMMAND blobber_blueprint_tests)
endif()
add_library(blobber_world_cpu src/render/dialogue_camera.cpp src/render/world_stairs.cpp src/render/world_doors.cpp src/render/world_effects.cpp src/render/spell_effects.cpp src/render/spell_effects_offensive.cpp src/render/spell_effects_support.cpp src/render/spell_lights.cpp src/render/melee_swipe.cpp src/render/world_scene.cpp src/render/world_visibility.cpp src/render/world_raster.cpp src/render/world_billboard_clip.cpp)
# Keep gameplay/orchestration unoptimized in Debug; only the numerical picking
# kernels need optimized scalar loops. Symbols and Debug assertions are retained.
option(BLOBBER_OPTIMIZE_DEBUG_RASTER "Optimize numerical visibility kernels in Debug" ON)
if(BLOBBER_OPTIMIZE_DEBUG_RASTER AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/render/world_raster.cpp src/render/world_billboard_clip.cpp
        PROPERTIES COMPILE_OPTIONS "$<$<CONFIG:Debug>:-O2>")
endif()
target_link_libraries(blobber_world_cpu PUBLIC blobber_core blobber_world_assets PRIVATE nlohmann_json::nlohmann_json)
blobber_warnings(blobber_world_cpu)
if(BUILD_TESTING)
    add_executable(blobber_blueprint_render_tests tests/blueprint_render_tests.cpp)
    target_include_directories(blobber_blueprint_render_tests PRIVATE tests)
    target_link_libraries(blobber_blueprint_render_tests PRIVATE blobber_blueprint_art blobber_world_cpu nlohmann_json::nlohmann_json)
    blobber_warnings(blobber_blueprint_render_tests)
    add_test(NAME blueprint_render COMMAND blobber_blueprint_render_tests)
endif()

# Idempotent retirement of obsolete assets in this managed build tree only.
add_custom_target(blobber_retire_assets ALL
    COMMAND ${CMAKE_COMMAND} "-DMANAGED_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        "-DMANAGED_CONFIGS=${CMAKE_CONFIGURATION_TYPES}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RetireAssets.cmake"
    VERBATIM)

if(BLOBBER_BUILD_GRAPHICS)
    function(blobber_development_asset_paths target)
        target_compile_definitions(${target} PRIVATE
            "$<$<CONFIG:Debug>:BLOBBER_SOURCE_ASSETS=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\">")
    endfunction()
    add_library(blobber_assets_io src/assets/map_icons.cpp src/assets/creator_art.cpp src/assets/portrait_preparation.cpp
        src/assets/npc_sheet.cpp src/assets/world_preparation.cpp src/render/image_sampling.cpp)
    target_link_libraries(blobber_assets_io PUBLIC blobber_core blobber_npc_io raylib PRIVATE nlohmann_json::nlohmann_json)
    add_library(blobber_platform
        src/platform/audio.cpp src/platform/dialogue_audio.cpp src/platform/music_player.cpp src/ui/music_banner.cpp
        src/platform/gpu_resources.cpp src/platform/environment_ao.cpp src/platform/world_geometry_draw.cpp src/platform/world_renderer.cpp src/platform/window.cpp src/platform/input.cpp
        src/ui/item_tooltip.cpp src/ui/trade_screen.cpp src/ui/dialogue_art.cpp src/ui/dialogue_screen.cpp src/ui/save_screen.cpp src/ui/creator_screen.cpp src/platform/presenter.cpp src/ui/loot_screen.cpp src/ui/camp_screen.cpp src/ui/travel_screen.cpp src/ui/character_sheet.cpp src/ui/inventory_screen.cpp src/ui/enemy_targets.cpp src/ui/spellbook.cpp src/ui/hud.cpp src/ui/hud_overlays.cpp src/ui/art.cpp src/ui/fonts.cpp)
    target_link_libraries(blobber_platform PUBLIC blobber_core blobber_world_cpu blobber_audio_io blobber_assets_io raylib)
    target_sources(blobber_platform PRIVATE src/ui/map_screen.cpp src/ui/menu_title.cpp)
    add_library(blobber_application src/app/city_review.cpp src/app/audio_settings.cpp src/app/campaign_saves.cpp src/app/arena_model.cpp src/app/arena_preparation.cpp src/app/arena_setup.cpp
        src/ui/arena_screen.cpp src/app/creator_flow.cpp src/app/session_launch.cpp src/ui/menu_screen.cpp
        src/app/menu.cpp src/app/menu_model.cpp src/app/application.cpp src/app/options.cpp src/app/gameplay.cpp)
    add_executable(blobber src/main.cpp)
    target_link_libraries(blobber_application PUBLIC blobber_platform blobber_assets_io blobber_rules_io nlohmann_json::nlohmann_json)
    target_link_libraries(blobber PRIVATE blobber_application)
    blobber_development_asset_paths(blobber_application)
    add_executable(blobber_prepare_assets EXCLUDE_FROM_ALL tools/prepare_assets.cpp)
    target_link_libraries(blobber_prepare_assets PRIVATE raylib)
    target_include_directories(blobber_prepare_assets PRIVATE src)
    add_executable(blobber_prepare_npc_animation EXCLUDE_FROM_ALL tools/prepare_npc_animation.cpp)
    target_link_libraries(blobber_prepare_npc_animation PRIVATE blobber_assets_io)
    set(BLOBBER_PREPARED_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/prepared")
    if(CMAKE_CONFIGURATION_TYPES)
        set(BLOBBER_PREPARED_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assets/prepared")
    endif()
    foreach(target blobber_assets_io blobber_platform blobber_application blobber blobber_prepare_assets blobber_prepare_npc_animation)
        blobber_warnings(${target})
    endforeach()

    set(BLOBBER_STAGE_ARGUMENTS
        --manifest "${CMAKE_CURRENT_SOURCE_DIR}/packaging/runtime-content.json"
        --source "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        --prepared "${BLOBBER_PREPARED_DIR}"
        --build "${CMAKE_CURRENT_BINARY_DIR}" --config "$<CONFIG>")
    if(CMAKE_CONFIGURATION_TYPES)
        list(APPEND BLOBBER_STAGE_ARGUMENTS --multi-config)
    endif()
    # Explicit tool staging and normal Release staging use the same inventory.
    add_custom_target(blobber_assets
        COMMAND blobber_stage_runtime raw ${BLOBBER_STAGE_ARGUMENTS}
        VERBATIM)
    add_dependencies(blobber_assets blobber_stage_runtime blobber_retire_assets)
    if(CMAKE_CONFIGURATION_TYPES OR NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_target(blobber_runtime_assets ALL
            COMMAND blobber_stage_runtime raw ${BLOBBER_STAGE_ARGUMENTS} --skip-debug
            VERBATIM)
        add_dependencies(blobber_runtime_assets blobber_stage_runtime blobber_retire_assets)
    else()
        # Debug keeps its direct source workflow and has no raw-copy/tool dependency.
        add_custom_target(blobber_runtime_assets)
    endif()
    add_dependencies(blobber blobber_runtime_assets)

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
    add_executable(blobber_sanctuary_city_tests tests/sanctuary_city_tests.cpp)
    target_link_libraries(blobber_sanctuary_city_tests PRIVATE blobber_core)
    target_include_directories(blobber_sanctuary_city_tests PRIVATE tests)
    blobber_warnings(blobber_sanctuary_city_tests)
    add_test(NAME sanctuary_city COMMAND blobber_sanctuary_city_tests)
    add_executable(blobber_architecture_tests tests/architecture_tests.cpp)
    target_link_libraries(blobber_architecture_tests PRIVATE blobber_world_cpu)
    target_include_directories(blobber_architecture_tests PRIVATE tests)
    blobber_warnings(blobber_architecture_tests)
    add_test(NAME architecture COMMAND blobber_architecture_tests)
    add_executable(blobber_scenery_tests tests/scenery_tests.cpp)
    target_link_libraries(blobber_scenery_tests PRIVATE blobber_world_cpu)
    target_include_directories(blobber_scenery_tests PRIVATE tests)
    blobber_warnings(blobber_scenery_tests)
    add_test(NAME scenery COMMAND blobber_scenery_tests)
    add_executable(blobber_map_tests tests/map_tests.cpp)
    target_include_directories(blobber_map_tests PRIVATE tests)
    target_link_libraries(blobber_map_tests PRIVATE blobber_core)
    blobber_warnings(blobber_map_tests)
    add_test(NAME map COMMAND blobber_map_tests)
    if(BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_options_tests tests/options_tests.cpp src/app/options.cpp)
    target_include_directories(blobber_options_tests PRIVATE src)
    target_link_libraries(blobber_options_tests PRIVATE raylib)
    blobber_development_asset_paths(blobber_options_tests)
    add_executable(blobber_packaged_options_tests tests/options_tests.cpp src/app/options.cpp)
    target_include_directories(blobber_packaged_options_tests PRIVATE src)
    target_link_libraries(blobber_packaged_options_tests PRIVATE raylib)
    blobber_warnings(blobber_packaged_options_tests)
    add_test(NAME packaged_options COMMAND blobber_packaged_options_tests)
    blobber_warnings(blobber_options_tests)
    add_test(NAME options COMMAND blobber_options_tests)
    add_test(NAME development_cli COMMAND blobber --development --help)
    endif()
    foreach(suite spell_geometry combat_movement trade dialogue residents screen_shake combat_positioning targeting dressing buttons shaman zombie giant_rat bugbear damage animation progression_allocations level_spells progression portraits creation library visual_review expanded_spells recovery  inventory spells effects  impact  image_preparation hybrid ai health combat hud commands  actors  core       rules rules_content party party_content)
        if(NOT BLOBBER_BUILD_GRAPHICS AND suite MATCHES "^(portraits|image_preparation)$")
            continue()
        endif()
        add_executable(blobber_${suite}_tests tests/${suite}_tests.cpp)
        target_include_directories(blobber_${suite}_tests PRIVATE tests)
        if(suite STREQUAL "portraits" OR suite STREQUAL "image_preparation")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_assets_io nlohmann_json::nlohmann_json)
        elseif(suite STREQUAL "rules" OR suite STREQUAL "health")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_rules)
        elseif(suite STREQUAL "rules_content" OR suite STREQUAL "party_content" OR suite STREQUAL "creation" OR suite STREQUAL "library")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_rules_io blobber_core nlohmann_json::nlohmann_json)
        elseif(suite STREQUAL "dialogue" OR suite STREQUAL "trade")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_world_cpu blobber_npc_io nlohmann_json::nlohmann_json)
        elseif(suite STREQUAL "visual_review" OR suite STREQUAL "targeting" OR suite STREQUAL "residents")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_world_cpu)
        else()
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_core)
        endif()
        blobber_warnings(blobber_${suite}_tests)
        add_test(NAME ${suite} COMMAND blobber_${suite}_tests)
    endforeach()
    if(BLOBBER_BUILD_GRAPHICS)
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
    add_executable(blobber_arena_tests tests/arena_tests.cpp src/app/menu_model.cpp src/app/arena_model.cpp src/app/arena_preparation.cpp)
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
add_executable(blobber_guardian_trial EXCLUDE_FROM_ALL tools/guardian_trial.cpp)
target_link_libraries(blobber_guardian_trial PRIVATE blobber_automation_io)
blobber_warnings(blobber_guardian_trial)
add_executable(blobber_headless tools/headless.cpp)
target_link_libraries(blobber_headless PRIVATE blobber_automation_io blobber_automation_world_visual)
blobber_warnings(blobber_headless)
if(BLOBBER_BUILD_GRAPHICS)
        target_sources(blobber_headless PRIVATE src/automation/capture.cpp)
        target_link_libraries(blobber_headless PRIVATE blobber_platform)
        target_compile_definitions(blobber_headless PRIVATE BLOBBER_HEADLESS_CAPTURE=1)
endif()
if(BUILD_TESTING)
    add_executable(blobber_automation_tests tests/automation_tests.cpp)
    target_link_libraries(blobber_automation_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_automation_tests)
    add_test(NAME automation COMMAND blobber_automation_tests)
endif()
if(BUILD_TESTING)
    add_executable(blobber_automation_allocations_tests tests/automation_allocations_tests.cpp)
    target_include_directories(blobber_automation_allocations_tests PRIVATE tests)
    target_link_libraries(blobber_automation_allocations_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_automation_allocations_tests)
    add_test(NAME automation_allocations COMMAND blobber_automation_allocations_tests)
    target_link_libraries(blobber_automation_allocations_tests PRIVATE blobber_automation_world_visual)
    add_executable(blobber_npc_metadata_tests tests/npc_metadata_tests.cpp)
    target_link_libraries(blobber_npc_metadata_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_npc_metadata_tests)
    add_test(NAME npc_metadata COMMAND blobber_npc_metadata_tests)
endif()

if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME agent_artifacts_tools COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/agent_artifacts_tests.py)
    set_tests_properties(agent_artifacts_tools PROPERTIES TIMEOUT 20 LABELS "tooling")
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
target_link_libraries(blobber_generate_floor PRIVATE blobber_generation_io blobber_blueprint_io)
blobber_warnings(blobber_generate_floor)

target_link_libraries(blobber_automation_io PUBLIC blobber_generation_io)
if(BUILD_TESTING)
    add_executable(blobber_fixtures_automation_tests tests/fixtures_automation_tests.cpp)
    target_link_libraries(blobber_fixtures_automation_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_fixtures_automation_tests)
    add_test(NAME fixtures_automation COMMAND blobber_fixtures_automation_tests)
    set_tests_properties(fixtures_automation PROPERTIES LABELS "slow;integration" RUN_SERIAL TRUE TIMEOUT 60)
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
    add_executable(blobber_population_tests tests/population_tests.cpp)
    target_link_libraries(blobber_population_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_population_tests)
    add_test(NAME population COMMAND blobber_population_tests)
    add_executable(blobber_campaign_route_fixture tests/campaign_route_fixture.cpp)
    target_link_libraries(blobber_campaign_route_fixture PRIVATE blobber_automation_io)
    blobber_warnings(blobber_campaign_route_fixture)
    add_test(NAME campaign_route_fixture COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/campaign_route_fixture.py $<TARGET_FILE:blobber_campaign_route_fixture>)
    set_tests_properties(campaign_route_fixture PROPERTIES LABELS "slow;integration" RUN_SERIAL TRUE TIMEOUT 40)
    add_test(NAME campaign_city_route COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/campaign_route_fixture.py $<TARGET_FILE:blobber_campaign_route_fixture> --city-entry)
    set_tests_properties(campaign_city_route PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
    add_test(NAME campaign_route_tools COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/campaign_route_tests.py)
    add_executable(blobber_guardians_tests tests/guardians_tests.cpp)
    target_link_libraries(blobber_guardians_tests PRIVATE blobber_automation_io)
    blobber_warnings(blobber_guardians_tests)
    add_test(NAME guardians COMMAND blobber_guardians_tests)
endif()

if(BUILD_TESTING)
    add_executable(blobber_emporium_tests tests/emporium_tests.cpp)
    target_link_libraries(blobber_emporium_tests PRIVATE blobber_automation_io)
    target_include_directories(blobber_emporium_tests PRIVATE tests)
    blobber_warnings(blobber_emporium_tests)
    add_test(NAME emporium COMMAND blobber_emporium_tests)

    add_executable(blobber_smithy_tests tests/smithy_tests.cpp)
    target_link_libraries(blobber_smithy_tests PRIVATE blobber_automation_io)
    target_include_directories(blobber_smithy_tests PRIVATE tests)
    blobber_warnings(blobber_smithy_tests)
    add_test(NAME smithy COMMAND blobber_smithy_tests)
    add_executable(blobber_inn_tests tests/inn_tests.cpp)
    target_link_libraries(blobber_inn_tests PRIVATE blobber_automation_io)
    target_include_directories(blobber_inn_tests PRIVATE tests)
    blobber_warnings(blobber_inn_tests)
    add_test(NAME inn COMMAND blobber_inn_tests)
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

# Sole renderer texture and alpha-mask publication.
if(BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_bake_world tools/bake_world_assets.cpp)
    target_link_libraries(blobber_bake_world PRIVATE blobber_assets_io blobber_world_assets nlohmann_json::nlohmann_json)
    target_link_libraries(blobber_bake_world PRIVATE blobber_blueprint_art)
    blobber_warnings(blobber_bake_world)
    set(BLOBBER_WORLD_DIR "${BLOBBER_PREPARED_DIR}/world")
    include(cmake/WorldAssets.cmake)
    blobber_world_publication(blobber_bake_world_assets blobber_bake_world
        "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${BLOBBER_WORLD_DIR}")
    add_dependencies(blobber blobber_bake_world_assets)
endif()

if(BUILD_TESTING)
    add_executable(blobber_game_time_tests tests/game_time_tests.cpp)
    target_link_libraries(blobber_game_time_tests PRIVATE blobber_rules_io)
    target_include_directories(blobber_game_time_tests PRIVATE tests)
    blobber_warnings(blobber_game_time_tests)
    add_test(NAME game_time COMMAND blobber_game_time_tests)

    add_executable(blobber_dungeon_refresh_tests tests/dungeon_refresh_tests.cpp)
    target_link_libraries(blobber_dungeon_refresh_tests PRIVATE blobber_world_cpu nlohmann_json::nlohmann_json)
    target_include_directories(blobber_dungeon_refresh_tests PRIVATE tests)
    blobber_warnings(blobber_dungeon_refresh_tests)
    add_test(NAME dungeon_refresh COMMAND blobber_dungeon_refresh_tests)

    add_executable(blobber_world_residency_tests tests/world_residency_tests.cpp)
    target_link_libraries(blobber_world_residency_tests PRIVATE blobber_world_assets)
    target_include_directories(blobber_world_residency_tests PRIVATE tests)
    blobber_warnings(blobber_world_residency_tests)
    add_test(NAME world_residency COMMAND blobber_world_residency_tests)
    add_executable(blobber_dungeon_lighting_tests tests/dungeon_lighting_tests.cpp)
    target_include_directories(blobber_dungeon_lighting_tests PRIVATE src tests)
    blobber_warnings(blobber_dungeon_lighting_tests)
    add_test(NAME dungeon_lighting COMMAND blobber_dungeon_lighting_tests)
    add_executable(blobber_world_cpu_tests tests/world_cpu_tests.cpp)
    target_link_libraries(blobber_world_cpu_tests PRIVATE blobber_world_cpu nlohmann_json::nlohmann_json)
    target_include_directories(blobber_world_cpu_tests PRIVATE tests)
    blobber_warnings(blobber_world_cpu_tests)
    add_test(NAME world_cpu COMMAND blobber_world_cpu_tests)
    add_executable(blobber_sprite_crop_tests tests/sprite_crop_tests.cpp)
    target_link_libraries(blobber_sprite_crop_tests PRIVATE blobber_world_cpu nlohmann_json::nlohmann_json)
    target_include_directories(blobber_sprite_crop_tests PRIVATE tests)
    blobber_warnings(blobber_sprite_crop_tests)
    add_test(NAME sprite_crop COMMAND blobber_sprite_crop_tests)
    if(BLOBBER_BUILD_GRAPHICS)
        add_executable(blobber_game_time_gpu_tests EXCLUDE_FROM_ALL tests/game_time_gpu_tests.cpp)
        target_link_libraries(blobber_game_time_gpu_tests PRIVATE blobber_platform)
        target_include_directories(blobber_game_time_gpu_tests PRIVATE tests)
        blobber_warnings(blobber_game_time_gpu_tests)
        add_executable(blobber_dungeon_lighting_gpu_tests EXCLUDE_FROM_ALL tests/dungeon_lighting_gpu_tests.cpp)
        target_link_libraries(blobber_dungeon_lighting_gpu_tests PRIVATE blobber_platform)
        target_include_directories(blobber_dungeon_lighting_gpu_tests PRIVATE tests)
        blobber_warnings(blobber_dungeon_lighting_gpu_tests)
        add_executable(blobber_world_ao_gpu_tests EXCLUDE_FROM_ALL tests/world_ao_gpu_tests.cpp)
        target_link_libraries(blobber_world_ao_gpu_tests PRIVATE blobber_platform)
        target_include_directories(blobber_world_ao_gpu_tests PRIVATE tests)
        blobber_warnings(blobber_world_ao_gpu_tests)
        add_executable(blobber_architecture_gpu_tests EXCLUDE_FROM_ALL tests/architecture_gpu_tests.cpp)
        target_link_libraries(blobber_architecture_gpu_tests PRIVATE blobber_platform)
        target_include_directories(blobber_architecture_gpu_tests PRIVATE tests)
        blobber_warnings(blobber_architecture_gpu_tests)
        add_executable(blobber_world_residency_gpu_tests EXCLUDE_FROM_ALL tests/world_residency_gpu_tests.cpp)
        target_link_libraries(blobber_world_residency_gpu_tests PRIVATE blobber_platform)
        target_include_directories(blobber_world_residency_gpu_tests PRIVATE tests)
        blobber_warnings(blobber_world_residency_gpu_tests)
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
    add_executable(blobber_underkeep_render_tests tests/underkeep_render_tests.cpp)
    target_link_libraries(blobber_underkeep_render_tests PRIVATE blobber_world_cpu)
    target_include_directories(blobber_underkeep_render_tests PRIVATE tests)
    blobber_warnings(blobber_underkeep_render_tests)
    add_test(NAME underkeep_render COMMAND blobber_underkeep_render_tests)

    add_executable(blobber_world_visibility_tests tests/world_visibility_tests.cpp)
    target_link_libraries(blobber_world_visibility_tests PRIVATE blobber_automation_world_visual)
    target_include_directories(blobber_world_visibility_tests PRIVATE tests)
    blobber_warnings(blobber_world_visibility_tests)
    add_test(NAME world_visibility COMMAND blobber_world_visibility_tests)
endif()

if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_world_movement_profile EXCLUDE_FROM_ALL tests/world_movement_profile.cpp)
    target_link_libraries(blobber_world_movement_profile PRIVATE blobber_platform blobber_rules_io)
    blobber_warnings(blobber_world_movement_profile)
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
    target_include_directories(blobber_world_visual_fixture PRIVATE tests)
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
    add_executable(blobber_melee_swipe_tests tests/melee_swipe_tests.cpp)
    target_link_libraries(blobber_melee_swipe_tests PRIVATE blobber_world_cpu)
    target_include_directories(blobber_melee_swipe_tests PRIVATE tests)
    blobber_warnings(blobber_melee_swipe_tests)
    add_test(NAME melee_swipe COMMAND blobber_melee_swipe_tests)
    add_executable(blobber_spell_effects_tests tests/spell_effects_tests.cpp)
    target_link_libraries(blobber_spell_effects_tests PRIVATE blobber_world_cpu nlohmann_json::nlohmann_json)
    target_include_directories(blobber_spell_effects_tests PRIVATE tests)
    blobber_warnings(blobber_spell_effects_tests)
    add_test(NAME spell_effects COMMAND blobber_spell_effects_tests)

    add_executable(blobber_world_integration_tests tests/world_integration_tests.cpp)
    target_link_libraries(blobber_world_integration_tests PRIVATE blobber_world_cpu nlohmann_json::nlohmann_json)
    target_include_directories(blobber_world_integration_tests PRIVATE tests)
    blobber_warnings(blobber_world_integration_tests)
    add_test(NAME world_integration COMMAND blobber_world_integration_tests)
endif()
if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_gameplay_startup_tests tests/gameplay_startup_tests.cpp)
    target_link_libraries(blobber_gameplay_startup_tests PRIVATE blobber_application blobber_automation_io)
    target_include_directories(blobber_gameplay_startup_tests PRIVATE tests)
    blobber_warnings(blobber_gameplay_startup_tests)
    add_test(NAME gameplay_startup COMMAND blobber_gameplay_startup_tests)

    add_executable(blobber_world_integration_gpu_tests EXCLUDE_FROM_ALL tests/world_integration_gpu_tests.cpp)
    target_link_libraries(blobber_world_integration_gpu_tests PRIVATE blobber_application blobber_automation_io)
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


if(BUILD_TESTING)
    add_test(NAME retired_assets COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/retired_assets_tests.py
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RetireAssets.cmake)
endif()

if(BUILD_TESTING)
    add_executable(blobber_music_tests tests/music_tests.cpp)
    target_include_directories(blobber_music_tests PRIVATE tests)
    target_link_libraries(blobber_music_tests PRIVATE blobber_audio_io)
    blobber_warnings(blobber_music_tests)
    add_test(NAME music COMMAND blobber_music_tests)
    add_executable(blobber_audio_tests tests/audio_tests.cpp)
    target_include_directories(blobber_audio_tests PRIVATE tests)
    target_link_libraries(blobber_audio_tests PRIVATE blobber_audio_io)
    blobber_warnings(blobber_audio_tests)
    add_test(NAME audio COMMAND blobber_audio_tests)
endif()
if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    # Fake raylib functions exercise our runtime without opening a device or window.
    add_executable(blobber_audio_runtime_tests tests/audio_runtime_tests.cpp src/platform/audio.cpp src/platform/dialogue_audio.cpp src/platform/music_player.cpp)
    target_include_directories(blobber_audio_runtime_tests PRIVATE tests)
    target_link_libraries(blobber_audio_runtime_tests PRIVATE blobber_audio_io blobber_world_cpu raylib)
    blobber_warnings(blobber_audio_runtime_tests)
    add_test(NAME audio_runtime COMMAND blobber_audio_runtime_tests)
endif()
if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_audio_device_check EXCLUDE_FROM_ALL tests/audio_device_check.cpp)
    target_include_directories(blobber_audio_device_check PRIVATE tests)
    target_link_libraries(blobber_audio_device_check PRIVATE blobber_platform)
    blobber_warnings(blobber_audio_device_check)
endif()
if(BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_review_dungeon_lighting EXCLUDE_FROM_ALL tools/review_dungeon_lighting.cpp)
    add_executable(blobber_review_game_time EXCLUDE_FROM_ALL tools/review_game_time.cpp)
    target_link_libraries(blobber_review_game_time PRIVATE blobber_application)
    blobber_warnings(blobber_review_game_time)
    target_link_libraries(blobber_review_dungeon_lighting PRIVATE blobber_application)
    blobber_warnings(blobber_review_dungeon_lighting)
    add_executable(blobber_review_smithy EXCLUDE_FROM_ALL tools/review_smithy.cpp)
    add_executable(blobber_review_emporium EXCLUDE_FROM_ALL tools/review_emporium.cpp)
    target_link_libraries(blobber_review_emporium PRIVATE blobber_application)
    blobber_warnings(blobber_review_emporium)
    target_link_libraries(blobber_review_smithy PRIVATE blobber_application)
    blobber_warnings(blobber_review_smithy)
    add_executable(blobber_review_inn EXCLUDE_FROM_ALL tools/review_inn.cpp)
    target_link_libraries(blobber_review_inn PRIVATE blobber_application)
    blobber_warnings(blobber_review_inn)
    add_executable(blobber_review_trade EXCLUDE_FROM_ALL tools/review_trade.cpp)
    target_link_libraries(blobber_review_trade PRIVATE blobber_application)
    blobber_warnings(blobber_review_trade)
    add_executable(blobber_review_early_equipment EXCLUDE_FROM_ALL tools/review_early_equipment.cpp)
    target_link_libraries(blobber_review_early_equipment PRIVATE blobber_application)
    blobber_warnings(blobber_review_early_equipment)
    add_executable(blobber_review_paperdoll EXCLUDE_FROM_ALL tools/review_paperdoll.cpp)
    target_include_directories(blobber_review_paperdoll PRIVATE tests)
    target_link_libraries(blobber_review_paperdoll PRIVATE blobber_application)
    blobber_warnings(blobber_review_paperdoll)
    add_executable(blobber_review_camera_framing EXCLUDE_FROM_ALL tools/review_camera_framing.cpp)
    target_link_libraries(blobber_review_camera_framing PRIVATE blobber_application)
    blobber_warnings(blobber_review_camera_framing)
    add_executable(blobber_review_sanctuary_city EXCLUDE_FROM_ALL tools/review_sanctuary_city.cpp)
    add_executable(blobber_review_blueprints EXCLUDE_FROM_ALL tools/review_blueprints.cpp)
    target_link_libraries(blobber_review_blueprints PRIVATE blobber_application blobber_blueprint_art)
    blobber_warnings(blobber_review_blueprints)
    target_link_libraries(blobber_review_sanctuary_city PRIVATE blobber_application)
    blobber_warnings(blobber_review_sanctuary_city)
    add_executable(blobber_review_environment_ao EXCLUDE_FROM_ALL tools/review_environment_ao.cpp)
    target_link_libraries(blobber_review_environment_ao PRIVATE blobber_application)
    blobber_warnings(blobber_review_environment_ao)
    add_executable(blobber_review_world_residency EXCLUDE_FROM_ALL tools/review_world_residency.cpp)
    target_link_libraries(blobber_review_world_residency PRIVATE blobber_application)
    blobber_warnings(blobber_review_world_residency)
    add_executable(blobber_review_actor_resolution EXCLUDE_FROM_ALL tools/review_actor_resolution.cpp)
    target_link_libraries(blobber_review_actor_resolution PRIVATE blobber_application)
    blobber_warnings(blobber_review_actor_resolution)
    add_executable(blobber_capture_audio_settings EXCLUDE_FROM_ALL tools/capture_audio_settings.cpp)
    target_link_libraries(blobber_capture_audio_settings PRIVATE blobber_application)
    blobber_warnings(blobber_capture_audio_settings)
endif()

if(BUILD_TESTING AND BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_music_device_check EXCLUDE_FROM_ALL tests/music_device_check.cpp)
    target_include_directories(blobber_music_device_check PRIVATE tests)
    target_link_libraries(blobber_music_device_check PRIVATE blobber_platform)
    blobber_warnings(blobber_music_device_check)
endif()
if(BLOBBER_BUILD_GRAPHICS)
    add_executable(blobber_capture_music_banner EXCLUDE_FROM_ALL tools/capture_music_banner.cpp)
    target_link_libraries(blobber_capture_music_banner PRIVATE blobber_platform)
    blobber_warnings(blobber_capture_music_banner)
    add_executable(blobber_review_menu_title EXCLUDE_FROM_ALL tools/review_menu_title.cpp)
    target_include_directories(blobber_review_menu_title PRIVATE tests)
    target_link_libraries(blobber_review_menu_title PRIVATE blobber_application)
    blobber_warnings(blobber_review_menu_title)
endif()

if(BLOBBER_BUILD_GRAPHICS AND BUILD_TESTING)
    add_executable(blobber_capture_targeting EXCLUDE_FROM_ALL tools/capture_targeting.cpp)
    target_include_directories(blobber_capture_targeting PRIVATE tests)
    target_link_libraries(blobber_capture_targeting PRIVATE blobber_platform blobber_npc_io blobber_rules_io)
    blobber_warnings(blobber_capture_targeting)
endif()

# Blueprint content is loaded only at application/tool boundaries.
target_link_libraries(blobber_headless PRIVATE blobber_blueprint_io)
if(TARGET blobber_application)
    target_link_libraries(blobber_application PUBLIC blobber_blueprint_io)
endif()
if(TARGET blobber_guardian_trial)
    target_link_libraries(blobber_guardian_trial PRIVATE blobber_blueprint_io)
endif()

if(TARGET blobber_application)
    add_executable(blobber_review_blueprint_campaign EXCLUDE_FROM_ALL tools/review_blueprint_campaign.cpp)
    target_link_libraries(blobber_review_blueprint_campaign PRIVATE blobber_application blobber_generation_io)
    blobber_warnings(blobber_review_blueprint_campaign)
endif()

# Build-only runtime inventory/staging; these executables never enter the payload.
add_library(blobber_distribution EXCLUDE_FROM_ALL
    tools/distribution/runtime_inventory.cpp tools/distribution/staging.cpp)
target_include_directories(blobber_distribution PUBLIC tools)
target_link_libraries(blobber_distribution PUBLIC nlohmann_json::nlohmann_json
    PRIVATE blobber_npc_io blobber_audio_io blobber_rules_io blobber_blueprint_io blobber_world_assets)
blobber_warnings(blobber_distribution)
add_executable(blobber_stage_runtime EXCLUDE_FROM_ALL tools/stage_runtime.cpp)
target_link_libraries(blobber_stage_runtime PRIVATE blobber_distribution)
blobber_warnings(blobber_stage_runtime)

if(BLOBBER_BUILD_GRAPHICS AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
    AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$"
    AND NOT CMAKE_CONFIGURATION_TYPES AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_target(dist-stage
        COMMAND blobber_stage_runtime dist ${BLOBBER_STAGE_ARGUMENTS}
            --executable "$<TARGET_FILE:blobber>"
        DEPENDS blobber blobber_stage_runtime
        VERBATIM)
else()
    add_custom_target(dist-stage
        COMMAND ${CMAKE_COMMAND} -E echo
            "dist-stage requires Linux x86-64, graphics ON and single-config Release. Use the release Ninja preset."
        COMMAND ${CMAKE_COMMAND} -E false
        VERBATIM)
endif()
if(BUILD_TESTING)
    add_executable(blobber_distribution_tests tests/distribution_tests.cpp)
    target_include_directories(blobber_distribution_tests PRIVATE tests)
    target_link_libraries(blobber_distribution_tests PRIVATE blobber_distribution blobber_npc_io)
    blobber_warnings(blobber_distribution_tests)
    add_test(NAME runtime_inventory COMMAND blobber_distribution_tests inventory)
    add_test(NAME dist_staging COMMAND blobber_distribution_tests staging)
    set_tests_properties(runtime_inventory dist_staging PROPERTIES TIMEOUT 20 LABELS "tooling")
endif()
