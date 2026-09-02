#ifndef MYLIB_PLUGIN_ABI_H
#define MYLIB_PLUGIN_ABI_H

/* mylib_plugin_abi.h - the contract between a plugin host and a mylib plugin.
 *
 * This is the ONLY header the two sides share. It is plain C, has no
 * dependency on any generated file or on CMake, and deliberately says
 * nothing about how either side is built. A host loads a plugin at run time
 * (dlopen()/LoadLibrary()), resolves exactly one symbol from it
 * (MYLIB_PLUGIN_ENTRY_NAME), calls it to obtain a `const mylib_plugin_api *`,
 * checks the version/size fields, and then calls through the function
 * pointers in that table. Nothing else about the plugin is touched.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bumped whenever mylib_plugin_api changes in a way that isn't
 * backward-compatible. A host must refuse a plugin whose abi_version differs
 * from the value it was compiled against. */
#define MYLIB_PLUGIN_ABI_VERSION 1

/* The one symbol every mylib plugin exports, and the only symbol a host ever
 * looks up. Keep the string and the typedef below in sync. */
#define MYLIB_PLUGIN_ENTRY_NAME "mylib_plugin_get_api"

typedef struct mylib_plugin_api {
    /* sizeof(struct mylib_plugin_api) as the plugin saw it at compile time.
     * Lets a newer host that has grown the struct detect an older, shorter
     * table (and vice versa) before reading past the end of it. */
    size_t struct_size;

    /* Must equal MYLIB_PLUGIN_ABI_VERSION. */
    int abi_version;

    /* The capability this plugin provides. Returns a pointer to a static,
     * plugin-owned, null-terminated ASCII string (at most 255 characters,
     * so it fits in char[256]) describing the plugin's version. The pointer
     * is valid until the plugin is unloaded and must never be freed by the
     * caller. */
    const char *(*version)(void);
} mylib_plugin_api;

/* Signature of the exported entry point. Returns a pointer to a static api
 * table owned by the plugin, or NULL on failure. */
typedef const mylib_plugin_api *(*mylib_plugin_get_api_fn)(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MYLIB_PLUGIN_ABI_H */
