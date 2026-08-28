# ---------------------------------------------------------------------------
# test_sharedLib - loads mylib's shared library dynamically at run time.
# It intentionally does NOT target_link_libraries(... mylib ...), since the
# whole point is to resolve the library and the "version" symbol at run
# time instead of at link time.
# ---------------------------------------------------------------------------
add_executable(test_sharedLib test_sharedLib.cpp)

# Build-order dependency only: make sure mylib exists before we try to run
# test_sharedLib, even though we don't link against it.
add_dependencies(test_sharedLib mylib)

if(UNIX)
    # dlopen()/dlsym() live in libdl, which is folded into libc on some
    # systems (e.g. modern glibc) but still needs to be requested
    # explicitly on others - CMAKE_DL_LIBS resolves to whatever is correct.
    target_link_libraries(test_sharedLib PRIVATE ${CMAKE_DL_LIBS})
endif()

if(WIN32)
    # On Windows, LoadLibrary("mylib.dll") searches the executable's own
    # directory first, so copy the freshly-built DLL next to the .exe.
    add_custom_command(TARGET test_sharedLib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:mylib>"
                "$<TARGET_FILE_DIR:test_sharedLib>"
        COMMENT "Copying mylib shared library next to test_sharedLib.exe"
    )
endif()
# On Linux/macOS, test_sharedLib.cpp resolves its own executable directory
# at run time (via /proc/self/exe or _NSGetExecutablePath) and dlopen()s
# mylib there by absolute path, since both binaries land in the same output
# directory. This sidesteps RPATH/RUNPATH search-path rules entirely, which
# (on Linux, with the RUNPATH tags CMake sets by default) don't apply to
# dlopen() of an unrelated library by a bare filename anyway.

# ---------------------------------------------------------------------------
# test_staticLib - links mylib_static directly, the traditional way.
# ---------------------------------------------------------------------------
add_executable(test_staticLib test_staticLib.cpp)
target_link_libraries(test_staticLib PRIVATE mylib_static)

# ---------------------------------------------------------------------------
# Wire both up to CTest so `ctest` (or `ctest -C Debug` on multi-config
# generators) exercises them automatically.
# ---------------------------------------------------------------------------
add_test(NAME test_sharedLib_runs COMMAND test_sharedLib)
add_test(NAME test_staticLib_runs COMMAND test_staticLib)
