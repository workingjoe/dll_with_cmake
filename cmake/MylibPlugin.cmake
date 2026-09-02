# MylibPlugin.cmake
#
# A small, reusable CMake module for this project. Put the directory holding
# this file on CMAKE_MODULE_PATH and `include(MylibPlugin)` to get the
# add_mylib_plugin() command.
#
# A "plugin" here is a CMake MODULE library: a shared object that is built to
# be discovered and loaded at run time with dlopen()/LoadLibrary() and is
# *never* linked against (no target_link_libraries, no import library). The
# host and the plugin agree only on the C ABI in
# include/mylib_plugin_abi.h - not on any build-time artifact.
#
#   add_mylib_plugin(<target>
#       SOURCES      <src>...          # required
#       [OUTPUT_NAME <name>]           # defaults to <target>
#   )
#
# The resulting target:
#   * is an add_library(... MODULE ...) target,
#   * has no "lib" prefix, so its file is <name>.so / <name>.dll / <name>.dylib,
#   * is built with hidden symbol visibility, so only symbols explicitly
#     marked MYLIB_EXPORT (from the generated mylib_export.h) escape it,
#   * lands in ${CMAKE_BINARY_DIR}/plugins (or .../plugins/<Config> for
#     multi-config generators), kept apart from executables and static libs.

include_guard(GLOBAL)

include(GenerateExportHeader)

function(add_mylib_plugin target)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "OUTPUT_NAME" "SOURCES")

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "add_mylib_plugin(${target}): SOURCES is required")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "add_mylib_plugin(${target}): unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_OUTPUT_NAME)
        set(ARG_OUTPUT_NAME "${target}")
    endif()

    # MODULE, not SHARED: this library exists only to be loaded at run time.
    add_library(${target} MODULE ${ARG_SOURCES})

    set_target_properties(${target} PROPERTIES
        PREFIX ""                       # <name>.so / <name>.dll, never lib<name>.*
        OUTPUT_NAME "${ARG_OUTPUT_NAME}"
        # Only MYLIB_EXPORT-marked symbols are visible on GCC/Clang (and on
        # MinGW). MSVC already exports nothing that isn't __declspec(dllexport).
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    )

    foreach(_cfg ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${_cfg}" _cfg_upper)
        set_target_properties(${target} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY_${_cfg_upper} "${CMAKE_BINARY_DIR}/plugins/${_cfg}"
            RUNTIME_OUTPUT_DIRECTORY_${_cfg_upper} "${CMAKE_BINARY_DIR}/plugins/${_cfg}"
        )
    endforeach()

    target_include_directories(${target} PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
    )

    # Generates mylib_export.h in the current binary dir, defining MYLIB_EXPORT
    # correctly for whichever compiler is building the plugin. Because the
    # target is built with hidden visibility, MYLIB_EXPORT is the *only* way a
    # symbol leaves the plugin.
    generate_export_header(${target}
        BASE_NAME MYLIB
        EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/mylib_export.h"
    )
endfunction()
