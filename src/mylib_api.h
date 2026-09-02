#ifndef MYLIB_API_H
#define MYLIB_API_H

// Public API for the STATIC build of mylib (see src/CMakeLists.txt), for
// callers that would rather link the code straight into their program than
// load it as a plugin.
//
// The MODULE / plugin build does NOT expose this function. A plugin host
// talks to mylib exclusively through the ABI in <mylib_plugin_abi.h> and
// never needs this header.

#ifdef __cplusplus
extern "C" {
#endif

// Returns a pointer to a static, null-terminated ASCII string (at most 255
// characters plus the terminating '\0', i.e. it fits in a char[256])
// describing the library's version. The storage is owned by the library,
// is valid for the lifetime of the process, and must never be freed by the
// caller.
const char* version(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MYLIB_API_H
