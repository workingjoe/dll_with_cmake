# ---------------------------------------------------------------------------
# mylib as a PLUGIN: a CMake MODULE library, built to be loaded at run time
# by a host (see test/test_plugin.cpp) and never linked against.
#
# add_mylib_plugin() comes from cmake/MylibPlugin.cmake (included at the top
# level). It creates the MODULE target, strips the "lib" prefix, applies
# hidden visibility, routes the output into <build>/plugins, and runs
# generate_export_header() to produce mylib_export.h.
# ---------------------------------------------------------------------------
add_mylib_plugin(mylib
    SOURCES
        mylib.cpp
        mylib_api.h
)

# ---------------------------------------------------------------------------
# Static library alternative - the same source compiled as a static archive,
# for callers who want to link mylib directly into their program instead of
# loading it as a plugin. This target DOES expose version() for linking.
# ---------------------------------------------------------------------------
add_library(mylib_static STATIC
    mylib.cpp
    mylib_api.h
)

set_target_properties(mylib_static PROPERTIES
    PREFIX ""                       # mylib_static.a, not libmylib_static.a
    OUTPUT_NAME "mylib_static"
)

target_include_directories(mylib_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}          # for the generated mylib_export.h
)

# Tells the generated mylib_export.h to make MYLIB_EXPORT expand to nothing:
# required for a static library on Windows/MSVC (no dllexport/dllimport) and
# harmless everywhere else. PUBLIC so it also applies to anything that links
# mylib_static.
target_compile_definitions(mylib_static PUBLIC MYLIB_STATIC_DEFINE)
