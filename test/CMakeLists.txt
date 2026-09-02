# ---------------------------------------------------------------------------
# test_plugin - the plugin HOST. Loads the `mylib` MODULE library at run
# time and talks to it through the ABI in include/mylib_plugin_abi.h.
#
# It deliberately does NOT target_link_libraries(... mylib ...): the whole
# point is that the host resolves the plugin file and its entry-point symbol
# at run time, not at link time.
# ---------------------------------------------------------------------------
add_executable(test_plugin test_plugin.cpp)

target_include_directories(test_plugin PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Build-order dependency only: make sure the plugin is built before we try to
# run a host against it, even though we don't link it.
add_dependencies(test_plugin mylib)

if(UNIX)
    # dlopen()/dlsym() live in libdl, folded into libc on some systems and
    # separate on others; CMAKE_DL_LIBS resolves to whatever is correct.
    target_link_libraries(test_plugin PRIVATE ${CMAKE_DL_LIBS})
endif()

# ---------------------------------------------------------------------------
# test_staticLib - links mylib_static directly, the traditional way.
# ---------------------------------------------------------------------------
add_executable(test_staticLib test_staticLib.cpp)
target_link_libraries(test_staticLib PRIVATE mylib_static)

# ---------------------------------------------------------------------------
# CTest wiring. $<TARGET_FILE:mylib> expands to the absolute path of the
# built plugin (per-config on multi-config generators) and is passed to the
# host as argv[1].
# ---------------------------------------------------------------------------
add_test(NAME test_plugin_runs    COMMAND test_plugin $<TARGET_FILE:mylib>)
add_test(NAME test_staticLib_runs COMMAND test_staticLib)
