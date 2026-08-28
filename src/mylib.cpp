#include "mylib_api.h"

#include <cstdio>
#include <cstring>

namespace {

// ---------------------------------------------------------------------
// Internal helpers below. Neither is declared in mylib_api.h, neither is
// marked with MYLIB_EXPORT, and both live in an anonymous namespace, so:
//
//   * They have internal linkage in C++ terms (the compiler won't even
//     put an external symbol for them in the object file).
//   * On top of that, the shared-library target is built with
//     CXX_VISIBILITY_PRESET = hidden (see src/CMakeLists.txt), so even a
//     symbol that *did* have external linkage would still not appear in
//     the shared library's dynamic symbol table on GCC/Clang.
//   * On MSVC nothing is exported from a DLL unless it is explicitly
//     marked __declspec(dllexport), so these are invisible there too.
//
// Net effect: on every supported compiler/platform, `version` is the only
// symbol a caller can find via dlsym()/GetProcAddress(), or link against
// at all. You can check this yourself after building with, e.g.:
//   nm -D --defined-only bin/libmylib.so     (Linux)
//   dumpbin /exports bin\mylib.dll           (MSVC)
// ---------------------------------------------------------------------

int add_numbers(int a, int b)
{
    return a + b;
}

double square_value(double x)
{
    return x * x;
}

} // namespace

extern "C" MYLIB_EXPORT const char* version(void)
{
    static char buffer[256] = {0};

    if (buffer[0] == '\0')
    {
        // Built once, on first call. The two "dummy" helpers above are
        // exercised here just to show they're real, callable code from
        // inside the library - they're simply never reachable from
        // outside of it.
        const int sum = add_numbers(1, 2025);
        const double sq = square_value(2.0);

        std::snprintf(buffer, sizeof(buffer),
                      "mylib version 1.0.0 (internal check values: %d, %.1f)",
                      sum, sq);
    }

    return buffer;
}
