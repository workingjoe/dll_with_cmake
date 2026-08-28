// test_staticLib
//
// Same idea as test_sharedLib, but here `mylib_static` is a normal,
// compile-time/link-time dependency (see target_link_libraries in
// test/CMakeLists.txt) - no dlopen/LoadLibrary involved. This is the
// "statically linkable" alternative build the project also supports.

#include <cstdio>
#include <cstdlib>

#include "mylib_api.h"

int main()
{
    std::printf("test_staticLib: calling version() via a normal static link\n");
    std::printf("  version() -> \"%s\"\n", version());
    return EXIT_SUCCESS;
}
