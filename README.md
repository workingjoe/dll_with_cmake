# PluginExample

A minimal, modern-CMake example of a **plugin architecture**: a library
built as a CMake **`MODULE`** so it is *discovered and loaded at run time* by
a host program, rather than linked. The same source is also offered as a
`STATIC` library for callers who would rather link it directly.

This started life as a "load a shared library with `dlopen`" example; it has
been reworked so the run-time boundary is a real, versioned plugin ABI
instead of an ad-hoc symbol lookup.

## What's here

- **`include/mylib_plugin_abi.h`** — the entire contract between host and
  plugin. Plain C, no generated headers, no CMake. It defines:
  - `mylib_plugin_api` — a struct of function pointers plus `struct_size`
    and `abi_version` fields for compatibility checks,
  - `MYLIB_PLUGIN_ENTRY_NAME` (`"mylib_plugin_get_api"`) — the one symbol a
    plugin exports and the only symbol a host ever resolves.
- **`cmake/MylibPlugin.cmake`** — a reusable CMake module providing
  `add_mylib_plugin(<target> SOURCES ...)`. It creates an
  `add_library(... MODULE ...)` target, strips the `lib` prefix, forces
  hidden symbol visibility, routes the output into `<build>/plugins/`, and
  runs `generate_export_header()` for the `MYLIB_EXPORT` macro.
- **`src/mylib.cpp`** — the implementation. `version()` is the actual
  capability; `add_numbers()` / `square_value()` are internal helpers. Only
  `mylib_plugin_get_api()` is `MYLIB_EXPORT`-marked, so it is the *only*
  symbol the plugin exposes — `version()` itself is reachable from a host
  only through the api table.
- **`src/mylib_api.h`** — the static-link API (just `version()`), used only
  by the `mylib_static` target and `test_staticLib`.
- **`test/test_plugin.cpp`** — the **host**. Loads the plugin file given as
  `argv[1]`, resolves the entry point, checks the ABI version/size, calls
  `api->version()`, then verifies nothing else (`version`, `add_numbers`,
  `square_value`) is resolvable from the plugin.
- **`test/test_staticLib.cpp`** — the static-link alternative: links
  `mylib_static` and calls `version()` directly, no loader involved.

## Layout

```
.
├── CMakeLists.txt              top-level: adds cmake/ to CMAKE_MODULE_PATH, include(MylibPlugin)
├── cmake/
│   └── MylibPlugin.cmake       reusable module: add_mylib_plugin()  ->  MODULE library
├── include/
│   └── mylib_plugin_abi.h      the host <-> plugin ABI (the only shared header)
├── src/
│   ├── CMakeLists.txt          add_mylib_plugin(mylib ...) + the mylib_static target
│   ├── mylib_api.h             static-link API: version()
│   └── mylib.cpp               version() + 2 internal helpers + the plugin entry point
└── test/
    ├── CMakeLists.txt          test_plugin (host) and test_staticLib executables
    ├── test_plugin.cpp         loads the plugin at run time via the ABI
    └── test_staticLib.cpp      links mylib_static directly
```

Build products:

```
<build>/bin/       test_plugin(.exe), test_staticLib(.exe)
<build>/lib/       mylib_static.a
<build>/plugins/   mylib.dll / mylib.so / mylib.dylib   <- the loadable plugin
```

`mylib_export.h` (the `MYLIB_EXPORT` macro) is generated into the build tree
by `generate_export_header()` and is not checked in. `MODULE` vs `STATIC` is
handled automatically: for the plugin, `MYLIB_EXPORT` is
`__declspec(dllexport)` (MSVC) or `visibility("default")` (GCC/Clang/MinGW);
for the static library it expands to nothing (`MYLIB_STATIC_DEFINE`).

## `SHARED` vs `MODULE` — why this is a plugin

| | `add_library(SHARED)` | `add_library(MODULE)` |
|---|---|---|
| Linked with `target_link_libraries` | yes | **no** — not linkable |
| Import library (`.lib` / `.dll.a`) on Windows | produced | **not** produced |
| Intended use | shared dependency resolved by the loader at start-up | loaded explicitly with `dlopen` / `LoadLibrary` |

The host never names the `mylib` target except in `add_dependencies()` (a
build-order hint). It gets the plugin's path from CTest as
`$<TARGET_FILE:mylib>` and everything else happens at run time.

## Building on Linux (GCC)

```bash
cmake -S . -B build-debug   -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j

ctest --test-dir build-release --output-on-failure
# or run the host by hand, passing it the plugin:
./build-release/bin/test_plugin ./build-release/plugins/mylib.so
./build-release/bin/test_staticLib
```

## Building on Windows (MSVC, via Visual Studio)

Visual Studio's generator is *multi-config* — Debug/Release is chosen at
build time:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

ctest --test-dir build -C Release --output-on-failure
.\build\bin\Release\test_plugin.exe .\build\plugins\Release\mylib.dll
.\build\bin\Release\test_staticLib.exe
```

## Building on Windows (MinGW-w64 GCC or Ninja)

```powershell
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

ctest --test-dir build-release --output-on-failure
.\build-release\bin\test_plugin.exe .\build-release\plugins\mylib.dll
.\build-release\bin\test_staticLib.exe
```

(`-G "MinGW Makefiles"` works too; the `-DCMAKE_BUILD_TYPE=...` pattern is
the same.)

## Verifying the plugin exports only its entry point

`test_plugin` checks this at run time. You can also look directly:

```bash
# Linux
nm -D --defined-only build-release/plugins/mylib.so

# Windows (MinGW)
objdump -p build-release/plugins/mylib.dll | grep -A5 "Export Address Table"

# Windows (MSVC Developer Command Prompt)
dumpbin /exports build\plugins\Release\mylib.dll
```

You should see exactly one name: `mylib_plugin_get_api`. The internal helpers
have internal linkage (anonymous namespace) and the whole plugin is built
with hidden visibility, so `version` and the helpers never enter the export
table on any supported compiler.

## Adding another plugin

```cmake
# in some CMakeLists.txt, after include(MylibPlugin) at the top level
add_mylib_plugin(otherplugin
    SOURCES otherplugin.cpp
)
```

Implement `mylib_plugin_get_api()` returning a `mylib_plugin_api` table with
`abi_version = MYLIB_PLUGIN_ABI_VERSION`, drop the built file anywhere the
host looks, and `test_plugin <path>` will load it.

## Notes

- `mylib` (plugin) and `mylib_static` compile `mylib.cpp` twice, once per
  target, because the static build needs `MYLIB_STATIC_DEFINE`. Trivial for a
  one-file example; use an `OBJECT` library if it ever matters.
- `version()`'s buffer is filled once on first call and reused — fine for a
  single-threaded example; add synchronization if you call it concurrently
  before its first completion.
