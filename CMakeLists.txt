cmake_minimum_required(VERSION 3.24)
project(blobber VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

add_library(blobber_core src/world/map.cpp src/world/npc.cpp src/game/movement.cpp src/game/npc_animation.cpp src/input/key_repeat.cpp)
target_include_directories(blobber_core PUBLIC src)
add_library(blobber_render
    src/render/material.cpp src/render/npc_sprites.cpp src/render/scene_cache.cpp src/render/scene_renderer.cpp
    src/render/transitions.cpp src/render/frame_pipeline.cpp)
target_link_libraries(blobber_render PUBLIC blobber_core raylib)
add_library(blobber_assets_io src/assets/material_loader.cpp src/assets/npc_loader.cpp)
target_link_libraries(blobber_assets_io PUBLIC blobber_render PRIVATE nlohmann_json::nlohmann_json)
add_library(blobber_platform
    src/platform/gpu_resources.cpp src/platform/window.cpp src/platform/input.cpp
    src/platform/presenter.cpp src/ui/hud.cpp)
target_link_libraries(blobber_platform PUBLIC blobber_render)
add_executable(blobber src/main.cpp src/app/application.cpp src/app/options.cpp src/app/captures.cpp)
target_link_libraries(blobber PRIVATE blobber_platform blobber_assets_io)
add_executable(blobber_prepare_assets EXCLUDE_FROM_ALL tools/prepare_assets.cpp)
target_link_libraries(blobber_prepare_assets PRIVATE raylib)
target_include_directories(blobber_prepare_assets PRIVATE src)
add_executable(blobber_prepare_npc_animation EXCLUDE_FROM_ALL tools/prepare_npc_animation.cpp)
target_link_libraries(blobber_prepare_npc_animation PRIVATE raylib nlohmann_json::nlohmann_json)
target_include_directories(blobber_prepare_npc_animation PRIVATE src)
blobber_warnings(blobber_prepare_npc_animation)
foreach(target blobber_core blobber_render blobber_assets_io blobber_platform blobber blobber_prepare_assets)
    blobber_warnings(${target})
endforeach()

add_custom_target(blobber_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures" "$<TARGET_FILE_DIR:blobber>/assets/textures"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/maze.txt" "$<TARGET_FILE_DIR:blobber>/assets/maze.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/npcs" "$<TARGET_FILE_DIR:blobber>/assets/npcs"
    COMMENT "Copying dungeon assets")
add_dependencies(blobber blobber_assets)

include(CTest)
if(BUILD_TESTING)
    foreach(suite core render resource frame_allocations npc npc_animation)
        if(suite STREQUAL "frame_allocations")
            set(source tests/frame_allocations.cpp)
        else()
            set(source tests/${suite}_tests.cpp)
        endif()
        add_executable(blobber_${suite}_tests ${source})
        if(suite STREQUAL "core")
            target_link_libraries(blobber_${suite}_tests PRIVATE blobber_core)
        elseif(suite STREQUAL "resource" OR suite STREQUAL "npc")
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
    # Explicit desktop check; not part of headless CTest.
    add_executable(blobber_gpu_smoke EXCLUDE_FROM_ALL tests/gpu_smoke.cpp)
    target_link_libraries(blobber_gpu_smoke PRIVATE blobber_platform)
    blobber_warnings(blobber_gpu_smoke)
endif()
