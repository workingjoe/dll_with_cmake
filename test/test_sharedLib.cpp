// test_sharedLib
//
// Deliberately does NOT link against the `mylib` target at build time.
// Instead it loads the shared library by name at run time
// (LoadLibrary/GetProcAddress on Windows, dlopen/dlsym everywhere else)
// and looks up the "version" symbol dynamically, the way you would for a
// plugin whose exact build isn't known until run time.

#include <cstdio>
#include <cstdlib>
#include <string>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__APPLE__)
    #include <dlfcn.h>
    #include <mach-o/dyld.h>
    #include <vector>
#else
    #include <dlfcn.h>
    #include <unistd.h>
    #include <vector>
#endif

// Must match the signature of `version` in src/mylib_api.h. Redeclared here
// on purpose: a program that loads a library dynamically only needs to
// agree on the exported symbol's name and calling signature, not on any of
// the library's build-time headers.
typedef const char* (*version_func_t)(void);

namespace {

std::string platform_library_filename(const std::string& base_name)
{
#if defined(_WIN32)
    return base_name + ".dll";
#elif defined(__APPLE__)
    return "lib" + base_name + ".dylib";
#else
    return "lib" + base_name + ".so";
#endif
}

// Returns the directory containing the currently-running executable, with
// a trailing path separator. mylib is placed in the same output directory
// as this executable (see the top-level CMakeLists.txt), so resolving it
// by an absolute path here sidesteps the shared-library search-path rules
// entirely (rules which, on Linux, do NOT apply to dlopen() of a bare
// filename purely because the calling program has an RPATH/RUNPATH -
// RUNPATH, which is what CMake sets by default, only affects a binary's
// own linked-at-build-time dependencies).
std::string executable_directory()
{
#if defined(_WIN32)
    char path[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string full(path, len);
    const std::size_t slash = full.find_last_of("\\/");
    return (slash == std::string::npos) ? std::string() : full.substr(0, slash + 1);
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size); // first call just fills in the required size
    std::vector<char> buf(size);
    if (_NSGetExecutablePath(buf.data(), &size) != 0)
    {
        return std::string();
    }
    std::string full(buf.data());
    const std::size_t slash = full.find_last_of('/');
    return (slash == std::string::npos) ? std::string() : full.substr(0, slash + 1);
#else
    std::vector<char> buf(4096);
    const ssize_t len = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
    if (len <= 0)
    {
        return std::string();
    }
    std::string full(buf.data(), static_cast<std::size_t>(len));
    const std::size_t slash = full.find_last_of('/');
    return (slash == std::string::npos) ? std::string() : full.substr(0, slash + 1);
#endif
}

} // namespace

int main()
{
    const std::string lib_path = executable_directory() + platform_library_filename("mylib");
    std::printf("test_sharedLib: attempting to dynamically load '%s'\n", lib_path.c_str());

#if defined(_WIN32)

    HMODULE handle = LoadLibraryA(lib_path.c_str());
    if (handle == nullptr)
    {
        std::fprintf(stderr, "  FAILED: LoadLibraryA error code %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    auto version_fn = reinterpret_cast<version_func_t>(GetProcAddress(handle, "version"));
    if (version_fn == nullptr)
    {
        std::fprintf(stderr, "  FAILED: GetProcAddress could not find 'version'\n");
        FreeLibrary(handle);
        return EXIT_FAILURE;
    }

    std::printf("  OK: library loaded, 'version' symbol resolved\n");
    std::printf("  version() -> \"%s\"\n", version_fn());

    // Sanity-check that the *dummy* helpers are NOT reachable - this should
    // always fail to resolve, proving they aren't exported.
    if (GetProcAddress(handle, "add_numbers") != nullptr ||
        GetProcAddress(handle, "square_value") != nullptr)
    {
        std::fprintf(stderr, "  WARNING: an internal helper was unexpectedly exported!\n");
    }
    else
    {
        std::printf("  OK: internal helper functions are not exported, as expected\n");
    }

    FreeLibrary(handle);

#else // POSIX (Linux, macOS, ...)

    dlerror(); // clear any pre-existing error
    void* handle = dlopen(lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr)
    {
        std::fprintf(stderr, "  FAILED: dlopen: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    dlerror();
    auto version_fn = reinterpret_cast<version_func_t>(dlsym(handle, "version"));
    if (const char* err = dlerror())
    {
        std::fprintf(stderr, "  FAILED: dlsym('version'): %s\n", err);
        dlclose(handle);
        return EXIT_FAILURE;
    }

    std::printf("  OK: library loaded, 'version' symbol resolved\n");
    std::printf("  version() -> \"%s\"\n", version_fn());

    // Sanity-check that the *dummy* helpers are NOT reachable.
    dlerror();
    void* internal1 = dlsym(handle, "add_numbers");
    void* internal2 = dlsym(handle, "square_value");
    if (internal1 != nullptr || internal2 != nullptr)
    {
        std::fprintf(stderr, "  WARNING: an internal helper was unexpectedly exported!\n");
    }
    else
    {
        std::printf("  OK: internal helper functions are not exported, as expected\n");
    }

    dlclose(handle);

#endif

    return EXIT_SUCCESS;
}
