# Opt-in distribution toolchain policy; never changes native build defaults.
option(BLOBBER_LINUX_BASELINE "Build the controlled Ubuntu 22.04 distribution" OFF)
if(NOT BLOBBER_LINUX_BASELINE)
    return()
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64"
    OR NOT BLOBBER_BUILD_GRAPHICS OR CMAKE_CONFIGURATION_TYPES
    OR NOT CMAKE_BUILD_TYPE STREQUAL "Release" OR NOT CMAKE_GENERATOR STREQUAL "Ninja"
    OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR NOT CMAKE_C_COMPILER_ID STREQUAL "GNU"
    OR NOT CMAKE_CXX_COMPILER_VERSION MATCHES "^11[.]" OR NOT CMAKE_C_COMPILER_VERSION MATCHES "^11[.]"
    OR NOT CMAKE_VERSION VERSION_EQUAL "3.31.12"
    OR NOT EXISTS "/opt/blobber-baseline/lock.json")
    message(FATAL_ERROR "Baseline requires the pinned LD02 image, GCC 11, Ninja, graphics ON and Release")
endif()
foreach(dependency RAYLIB JSON YAML)
    set(BLOBBER_USE_SYSTEM_${dependency} OFF CACHE BOOL "Baseline uses pinned source" FORCE)
endforeach()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(PLATFORM Desktop CACHE STRING "" FORCE)
set(OPENGL_VERSION "3.3" CACHE STRING "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
foreach(dependency RAYLIB NLOHMANN_JSON YAML-CPP)
    string(TOLOWER "${dependency}" source_name)
    set(FETCHCONTENT_SOURCE_DIR_${dependency} "/opt/blobber-deps/${source_name}" CACHE PATH "" FORCE)
endforeach()
set(FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "" FORCE)
add_compile_options(-march=x86-64 -mtune=generic
    -ffile-prefix-map=/src=blobber-source -ffile-prefix-map=/build=blobber-build
    -ffile-prefix-map=/opt/blobber-deps=blobber-deps)
add_link_options(-static-libstdc++ -static-libgcc)

function(blobber_baseline_targets)
    if(NOT BLOBBER_LINUX_BASELINE)
        return()
    endif()
    foreach(dependency raylib yaml-cpp)
        get_target_property(kind ${dependency} TYPE)
        if(NOT kind STREQUAL "STATIC_LIBRARY")
            message(FATAL_ERROR "Baseline dependency ${dependency} must be static")
        endif()
    endforeach()
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/baseline-build.json" CONTENT
"{\"baseline\":true,\"configuration\":\"$<CONFIG>\",\"compiler_id\":\"${CMAKE_CXX_COMPILER_ID}\",\"compiler_version\":\"${CMAKE_CXX_COMPILER_VERSION}\",\"cmake_version\":\"${CMAKE_VERSION}\",\"raylib_type\":\"$<TARGET_PROPERTY:raylib,TYPE>\",\"yaml_type\":\"$<TARGET_PROPERTY:yaml-cpp,TYPE>\"}\n")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    # Orchestrate staging after invalidation, so even a prerequisite build failure
    # cannot leave a previous audit looking current. The outer target has no work
    # in parallel with this serialized configured-CMake invocation.
    add_custom_target(dist-audit
        COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_BINARY_DIR}/dist-stage/Release.audit.json"
        COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target dist-stage --parallel 1
        COMMAND "${CMAKE_BINARY_DIR}/blobber_stage_runtime" verify ${BLOBBER_STAGE_ARGUMENTS}
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/distribution/elf_audit.py"
            --build "${CMAKE_BINARY_DIR}" --stage "${CMAKE_BINARY_DIR}/dist-stage/Release"
            --policy "${CMAKE_SOURCE_DIR}/packaging/linux/runtime-policy.json"
            --output "${CMAKE_BINARY_DIR}/dist-stage/Release.audit.json"
        VERBATIM)
endfunction()
