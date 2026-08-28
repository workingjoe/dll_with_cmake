# dll_with_cmake
example c++ callable DLL with GCC and MSVC (claude.ai)

```
I would like to create a simple example project in C++ using modern CMake.  I would like to make a shared-library project where the shared 
library contains a single actual callable function, "version" which returns a static character array of at most 256 ASCII characters.  The 
library will also contain 2 dummy functions which are not exported.  The Cmake project should also contain a test_sharedLib console 
application which dynamically loads the shared-library and calls the "version" function.  This project should compile with both 
GCC and Microsoft MSVC compilers on Windows, and also compile with GCC on linux -- and in both release and debug modes. 
The dynamic library should also be statically linkable, if desired, perhaps as another Cmake target so that it would be possible to 
build the same application with the static library.
```
