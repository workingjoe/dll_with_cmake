// test_staticLib
//
// The static-link alternative to the plugin path. Here `mylib_static` is a
// normal compile-time/link-time dependency (see target_link_libraries in
// test/CMakeLists.txt) - no dlopen/LoadLibrary, no plugin ABI, no host. The
// program just calls version() directly, the traditional way.

#include <cstdio>
#include <cstdlib>

#include "mylib_api.h"

int main()
{
    std::printf("test_staticLib: calling version() via a normal static link\n");
    std::printf("  version() -> \"%s\"\n", version());
    return EXIT_SUCCESS;
}
