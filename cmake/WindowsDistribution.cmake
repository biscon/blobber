# Dedicated opt-in native Windows policy; native Linux defaults stay independent.
option(BLOBBER_WINDOWS_DISTRIBUTION "Build the native Windows review distribution" OFF)
if(NOT BLOBBER_WINDOWS_DISTRIBUTION)
    return()
endif()
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
    OR CMAKE_CONFIGURATION_TYPES OR NOT CMAKE_BUILD_TYPE STREQUAL "Release"
    OR NOT CMAKE_GENERATOR STREQUAL "Ninja")
    message(FATAL_ERROR "Windows distribution requires native MSVC x64, Ninja and Release")
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded CACHE STRING "Static compiler runtime" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(YAML_MSVC_SHARED_RT OFF CACHE BOOL "" FORCE)
foreach(dependency RAYLIB JSON YAML)
    set(BLOBBER_USE_SYSTEM_${dependency} OFF CACHE BOOL "Pinned distribution dependencies" FORCE)
endforeach()
set(PLATFORM Desktop CACHE STRING "" FORCE)
set(OPENGL_VERSION "3.3" CACHE STRING "" FORCE)
