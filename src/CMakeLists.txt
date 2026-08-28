include(GenerateExportHeader)

# ---------------------------------------------------------------------------
# Shared library target
# ---------------------------------------------------------------------------
add_library(mylib SHARED
    mylib.cpp
    mylib_api.h
)

set_target_properties(mylib PROPERTIES
    OUTPUT_NAME "mylib"
    # may need to set(CMAKE_STATIC_LIBRARY_PREFIX "") elsewhere
    PREFIX "" # do not prefix with 'lib' on gcc    
    # Only symbols explicitly marked MYLIB_EXPORT are visible outside the
    # shared library on GCC/Clang. (No effect on MSVC, which already only
    # exports symbols explicitly marked __declspec(dllexport).)
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(mylib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# Generates mylib_export.h in the build tree, defining MYLIB_EXPORT (and
# friends) correctly for whichever compiler is currently building the
# project. mylib_api.h #includes this generated header.
generate_export_header(mylib
    BASE_NAME MYLIB
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/mylib_export.h
)

# ---------------------------------------------------------------------------
# Static library target - same source code, built as a static archive
# instead, for anyone who'd rather link mylib directly into their app.
# ---------------------------------------------------------------------------
add_library(mylib_static STATIC
    mylib.cpp
    mylib_api.h
)

set_target_properties(mylib_static PROPERTIES
    # may need to set(CMAKE_STATIC_LIBRARY_PREFIX "") elsewhere
    PREFIX "" # do not prefix with 'lib' on gcc
    OUTPUT_NAME "mylib_static"
)

target_include_directories(mylib_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# Tells mylib_export.h (generated above) to make MYLIB_EXPORT expand to
# nothing for this target, which is required for static libraries on
# Windows/MSVC (no dllexport/dllimport involved) and is harmless elsewhere.
# PUBLIC so it also applies to anything that links against mylib_static.
target_compile_definitions(mylib_static PUBLIC MYLIB_STATIC_DEFINE)
