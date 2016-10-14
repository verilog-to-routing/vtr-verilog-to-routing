Overview
========
VTR uses [CMake](https://cmake.org) as it's build system.
CMake provides a protable cross-platform build systems with many useful features.

For unix-like systems we provide a wrapper Makefile which supports the traditional `make` and `make clean` commands, but calls CMake behind the scenes.

Building using the Makefile wrapper
==========================
Run `make` from the root of the VTR source tree

```shell
#In the VTR root
$ make
...
[100%] Built target vpr
```

Specifying the build type
-------------------------
You can specify the build type by passing the `BUILD_TYPE` parameter.

For instance to create a debug build (no optimization and debug symbols):

```shell
#In the VTR root
$ make BUILD_TYPE=debug
...
[100%] Built target vpr
```

Passing parameters to CMake
---------------------------
You can also pass parameters to CMake.

For instance to set the CMake configuration variable `VTR_ENABLE_SANITIZE` on:

```shell
#In the VTR root
$ make CMAKE_PARAMS="-DVTR_ENABLE_SANITIZE=ON"
...
[100%] Built target vpr
```

Both the `BUILD_TYPE` and `CMAKE_PARAMS` can be specified concurrently:
```shell
#In the VTR root
$ make BUILD_TYPE=debug CMAKE_PARAMS="-DVTR_ENABLE_SANITIZE=ON"
...
[100%] Built target vpr
```


Using CMake directly
====================
You can also use cmake directly.

First create a build directory under the VTR root:

```shell
#In the VTR root
$ mkdir build
$ cd build

#Call cmake pointing to the directory containing the root CMakeLists.txt
$ cmake ..

#Build
$ make
```

Changing configuration on the command line
------------------------------------------------
You can change the CMake configuration by passing command line parameters.

For instance to set the configuration to debug:

```shell
#In the build directory
$ cmake . -DCMAKE_BUILD_TYPE=debug

#Re-build
$ make
```

Changing configuration interactively with ccmake
------------------------------------------------
You can also use `ccmake` to to modify the build configuration.

```shell
#From the build directory
$ ccmake . #Make some configuration change

#Build
$ make
```

Non-unix-like platforms
=======================
CMake supports a variety of operating systems and can generate project files for a variety of build systems and IDEs.
While VTR is developed primarily on Linux, it should be possible to build on different platforms (your milage may varry).
See the [CMake Documentation](cmake.org) for more details about using cmake and generating project files on other platforms and build systems (e.g. Eclipse, Microsoft Visual Studio).

