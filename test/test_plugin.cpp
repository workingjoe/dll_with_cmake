// test_plugin - a minimal plugin HOST.
//
// `mylib` is built as a CMake MODULE library (see cmake/MylibPlugin.cmake):
// a shared object meant only to be loaded at run time, never linked. This
// program does NOT link against it. It:
//
//   1. loads the plugin file given as argv[1] (dlopen / LoadLibrary),
//   2. resolves the single entry point the plugin publishes
//      (mylib_plugin_get_api, see include/mylib_plugin_abi.h),
//   3. validates the returned ABI table (version + size fields),
//   4. calls the capability it advertises, api->version(),
//   5. confirms the plugin exports *nothing else* - not the raw version()
//      function, not the internal helpers.
//
// The build passes the exact plugin path to this program as a CTest
// argument ($<TARGET_FILE:mylib>); run by hand, pass it yourself.

#include <cstdio>
#include <cstdlib>

#include "mylib_plugin_abi.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {
void* dl_open(const char* path) { return LoadLibraryA(path); }
void* dl_sym(void* h, const char* name)
{
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(h), name));
}
void dl_close(void* h) { FreeLibrary(static_cast<HMODULE>(h)); }
const char* dl_error()
{
    static char buf[128];
    std::snprintf(buf, sizeof(buf), "Windows error %lu", GetLastError());
    return buf;
}
} // namespace

#else

#include <dlfcn.h>

namespace {
void* dl_open(const char* path) { return dlopen(path, RTLD_NOW | RTLD_LOCAL); }
void* dl_sym(void* h, const char* name) { return dlsym(h, name); }
void dl_close(void* h) { dlclose(h); }
const char* dl_error()
{
    const char* e = dlerror();
    return e ? e : "(no error)";
}
} // namespace

#endif

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: %s <path-to-mylib-plugin>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* plugin_path = argv[1];
    std::printf("test_plugin: loading plugin '%s'\n", plugin_path);

    void* handle = dl_open(plugin_path);
    if (handle == nullptr)
    {
        std::fprintf(stderr, "  FAILED: could not load plugin: %s\n", dl_error());
        return EXIT_FAILURE;
    }

    auto get_api = reinterpret_cast<mylib_plugin_get_api_fn>(
        dl_sym(handle, MYLIB_PLUGIN_ENTRY_NAME));
    if (get_api == nullptr)
    {
        std::fprintf(stderr, "  FAILED: entry point '%s' not found: %s\n",
                     MYLIB_PLUGIN_ENTRY_NAME, dl_error());
        dl_close(handle);
        return EXIT_FAILURE;
    }
    std::printf("  OK: entry point '%s' resolved\n", MYLIB_PLUGIN_ENTRY_NAME);

    const mylib_plugin_api* api = get_api();
    if (api == nullptr)
    {
        std::fprintf(stderr, "  FAILED: %s() returned NULL\n", MYLIB_PLUGIN_ENTRY_NAME);
        dl_close(handle);
        return EXIT_FAILURE;
    }

    if (api->abi_version != MYLIB_PLUGIN_ABI_VERSION)
    {
        std::fprintf(stderr, "  FAILED: plugin ABI version %d, host expects %d\n",
                     api->abi_version, MYLIB_PLUGIN_ABI_VERSION);
        dl_close(handle);
        return EXIT_FAILURE;
    }
    if (api->struct_size != sizeof(mylib_plugin_api))
    {
        std::fprintf(stderr, "  FAILED: plugin api table is %zu bytes, host expects %zu\n",
                     api->struct_size, sizeof(mylib_plugin_api));
        dl_close(handle);
        return EXIT_FAILURE;
    }
    std::printf("  OK: ABI version %d and table size %zu agree with the host\n",
                api->abi_version, api->struct_size);

    if (api->version == nullptr)
    {
        std::fprintf(stderr, "  FAILED: api->version is NULL\n");
        dl_close(handle);
        return EXIT_FAILURE;
    }
    std::printf("  plugin api->version() -> \"%s\"\n", api->version());

    // The plugin must publish ONLY its entry point. Neither the raw version()
    // function nor the internal helpers may be resolvable from outside.
    const char* must_not_leak[] = {"version", "add_numbers", "square_value"};
    bool leaked = false;
    for (const char* name : must_not_leak)
    {
        if (dl_sym(handle, name) != nullptr)
        {
            std::fprintf(stderr, "  WARNING: symbol '%s' is unexpectedly exported by the plugin\n",
                         name);
            leaked = true;
        }
    }
    if (!leaked)
    {
        std::printf("  OK: plugin exports only '%s', nothing else\n", MYLIB_PLUGIN_ENTRY_NAME);
    }

    dl_close(handle);
    return leaked ? EXIT_FAILURE : EXIT_SUCCESS;
}
