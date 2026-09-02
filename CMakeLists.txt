cmake_minimum_required(VERSION 3.10)

project(PluginExample
    VERSION 1.0.0
    DESCRIPTION "Minimal modern-CMake example: a CMake MODULE plugin + a run-time host + a static-link alternative"
    LANGUAGES CXX
)

# Project CMake modules (cmake/MylibPlugin.cmake, ...) live here.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(MylibPlugin)

# ---------------------------------------------------------------------------
# Global project settings
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a Release build for single-config generators (Makefiles/Ninja on
# Linux, MinGW Makefiles on Windows) if the user didn't ask for one.
# Multi-config generators (Visual Studio) ignore this and let you pick the
# configuration at build time with `--config Debug` / `--config Release`.
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _is_multi_config AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Put build products somewhere predictable. Executables go in bin/, static
# archives in lib/. Plugins are handled separately by add_mylib_plugin(),
# which routes them into plugins/ - kept apart from everything else so it's
# obvious they are discovered and loaded, not linked.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

foreach(_cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${_cfg} _cfg_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg_upper} ${CMAKE_BINARY_DIR}/bin/${_cfg})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_cfg_upper} ${CMAKE_BINARY_DIR}/bin/${_cfg})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_cfg_upper} ${CMAKE_BINARY_DIR}/lib/${_cfg})
endforeach()

enable_testing()

add_subdirectory(src)
add_subdirectory(test)
