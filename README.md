# SharedLibExample

```
I would like to create a simple example project in C++ using modern CMake.  I would like to make a shared-library project where the shared 
library contains a single actual callable function, "version" which returns a static character array of at most 256 ASCII characters.  The 
library will also contain 2 dummy functions which are not exported.  The Cmake project should also contain a test_sharedLib console 
application which dynamically loads the shared-library and calls the "version" function.  This project should compile with both 
GCC and Microsoft MSVC compilers on Windows, and also compile with GCC on linux -- and in both release and debug modes. 
The dynamic library should also be statically linkable, if desired, perhaps as another Cmake target so that it would be possible to 
build the same application with the static library.
```

A minimal, modern-CMake example project demonstrating:

- A shared library (`mylib`) exporting exactly **one** callable function,
  `version()`, which returns a pointer to a static, null-terminated ASCII
  string of at most 255 characters (fits in a `char[256]`).
- Two "dummy" helper functions inside the library that are **not** exported
  and cannot be called from outside it.
- `test_sharedLib`: a console app that **dynamically loads** `mylib` at run
  time (`dlopen`/`dlsym` on Linux, `LoadLibrary`/`GetProcAddress` on
  Windows) and calls `version()` through the resolved function pointer.
- `mylib_static`: the same source code built as a static library instead,
  plus `test_staticLib`, a console app that links against it the normal,
  compile-time way — showing the same functionality is available without
  dynamic loading at all.

Builds and runs on:
- Linux + GCC (Debug and Release)
- Windows + MSVC (Debug and Release)
- Windows + GCC / MinGW-w64 (Debug and Release)

## Layout

```
.
├── CMakeLists.txt          top-level project configuration
├── src/
│   ├── CMakeLists.txt      defines the `mylib` (SHARED) and `mylib_static` (STATIC) targets
│   ├── mylib_api.h         public API: declares version(), MYLIB_EXPORT, version_func_t
│   └── mylib.cpp           implementation: version() + 2 non-exported helpers
└── test/
    ├── CMakeLists.txt      defines the `test_sharedLib` and `test_staticLib` executables
    ├── test_sharedLib.cpp  dynamically loads mylib and calls version()
    └── test_staticLib.cpp  links mylib_static directly and calls version()
```

`mylib_export.h`, which supplies the `MYLIB_EXPORT` macro, is generated
automatically by CMake's `GenerateExportHeader` module into the build
directory — it isn't checked in, and you don't need to write it by hand.
That's what makes the same source compile correctly as:
- a GCC/Clang shared library (`__attribute__((visibility("default")))`)
- an MSVC shared library (`__declspec(dllexport)` / `dllimport`)
- a static library on either compiler (the macro expands to nothing)

## Building on Linux (GCC)

```bash
# Debug
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j

# Release
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j

# Run everything
ctest --test-dir build-release --output-on-failure
# or run the executables directly:
./build-release/bin/test_sharedLib
./build-release/bin/test_staticLib
```

## Building on Windows (MSVC, via Visual Studio)

Visual Studio's CMake generator is *multi-config*, so Debug/Release are
chosen at build time, not configure time:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
cmake --build build --config Release

ctest --test-dir build -C Release --output-on-failure
.\build\bin\Release\test_sharedLib.exe
.\build\bin\Release\test_staticLib.exe
```

(You can also open the generated `build\SharedLibExample.sln` directly in
Visual Studio.)

## Building on Windows (MinGW-w64 GCC)

```powershell
cmake -S . -B build-debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

cmake -S . -B build-release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

.\build-release\bin\test_sharedLib.exe
.\build-release\bin\test_staticLib.exe
```

(Or use `-G Ninja` instead of `-G "MinGW Makefiles"` if you have Ninja
installed; the same `-DCMAKE_BUILD_TYPE=...` pattern applies.)

## Verifying the dummy functions really aren't exported

After building, `test_sharedLib` itself checks this at run time (it tries
`dlsym`/`GetProcAddress` on the internal helper names and reports that they
fail to resolve). You can also check manually:

```bash
# Linux
nm -D --defined-only build-release/bin/libmylib.so

# Windows (Developer Command Prompt)
dumpbin /exports build\bin\Release\mylib.dll
```

Either way, you should see only `version` listed — `add_numbers` and
`square_value` (the two internal helpers in `mylib.cpp`) are compiled with
internal linkage (anonymous namespace) and, on top of that, the shared
library target is built with hidden symbol visibility by default, so they
never make it into the library's exported symbol table on any of the
supported compilers.

## Notes / things you might want to change

- `mylib` and `mylib_static` compile `mylib.cpp` twice (once per target)
  rather than sharing object files, because the two builds need different
  preprocessor definitions (`MYLIB_STATIC_DEFINE` for the static target).
  For a two-file example like this it's simpler than an `OBJECT` library
  with per-consumer compile definitions, and the compile cost is trivial.
- `version()`'s buffer is filled in once, on its first call, and reused
  afterwards (a simple "static local" cache) — safe for a single-threaded
  example like this; add your own synchronization if you extend it to be
  called from multiple threads before its first call.
