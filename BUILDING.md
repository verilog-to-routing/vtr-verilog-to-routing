# Building VTR #


## Overview ##

VTR uses [CMake](https://cmake.org) as it's build system.

CMake provides a portable cross-platform build systems with many useful features.

## Tested Compilers ##
VTR requires a C++-14 compliant compiler.
The following compilers are tested with VTR:

* GCC/G++: 5, 6, 7, 8, 9
* Clang/Clang++: 3.8, 6

Other compilers may work but are untested (your milage may vary).

## Unix-like ##
For unix-like systems we provide a wrapper Makefile which supports the traditional `make` and `make clean` commands, but calls CMake behind the scenes.

### Dependencies ###

For the basic tools you need:
 * Bison & Flex
 * cmake, make
 * A modern C++ compiler supporting C++14 (such as GCC >= 4.9 or clang >= 3.6)

For the VPR GUI you need:
 * Cairo
 * FreeType
 * Xft (libXft + libX11)
 * fontconfig
 * libgtk-3-dev

For the [regression testing and benchmarking](README.developers.md#running-tests) you will need:
 * Perl + List::MoreUtils
 * Python
 * time

It is also recommended you install the following development tools:
 * git
 * ctags
 * gdb
 * valgrind
 * clang-format-7

For Docs generation you will need:
 * Doxygen
 * python-sphinx
 * python-sphinx-rtd-theme
 * python-recommonmark

#### Debian & Ubuntu ####

The following should be enough to get the tools, VPR GUI and tests going on a modern Debian or Ubuntu system:

```shell
apt-get install \
	build-essential \
	flex \
	bison \
	cmake \
	fontconfig \
	libcairo2-dev \
	libfontconfig1-dev \
	libx11-dev \
	libxft-dev \
	perl \
	liblist-moreutils-perl \
	python \
	time
```

For documentation generation these additional packages are required:

```shell
apt-get install \
	doxygen \
	python-sphinx \
	python-sphinx-rtd-theme \
	python-recommonmark
```


For development the following additional packages are useful:

```shell
apt-get install \
	git \
	valgrind \
	gdb \
	ctags
```

### Building using the Makefile wrapper ###
Run `make` from the root of the VTR source tree

```shell
#In the VTR root
$ make
...
[100%] Built target vpr
```


#### Specifying the build type ####
You can specify the build type by passing the `BUILD_TYPE` parameter.

For instance to create a debug build (no optimization and debug symbols):

```shell
#In the VTR root
$ make BUILD_TYPE=debug
...
[100%] Built target vpr
```


#### Passing parameters to CMake ####
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


### Using CMake directly ###
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

#### Changing configuration on the command line ####
You can change the CMake configuration by passing command line parameters.

For instance to set the configuration to debug:

```shell
#In the build directory
$ cmake . -DCMAKE_BUILD_TYPE=debug

#Re-build
$ make
```

#### Changing configuration interactively with ccmake ####
You can also use `ccmake` to to modify the build configuration.

```shell
#From the build directory
$ ccmake . #Make some configuration change

#Build
$ make
```

## Other platforms ##

CMake supports a variety of operating systems and can generate project files for a variety of build systems and IDEs.
While VTR is developed primarily on Linux, it should be possible to build on different platforms (your milage may vary).
See the [CMake documentation](https://cmake.org) for more details about using cmake and generating project files on other platforms and build systems (e.g. Eclipse, Microsoft Visual Studio).


### Microsoft Windows ###

*NOTE: VTR support on Microsoft Windows is considered experimental*

#### Cygwin ####
[Cygwin](https://www.cygwin.com/) provides a POSIX (i.e. unix-like) environment for Microsoft Windows.

From within the cygwin terminal follow the Unix-like build instructions listed above.

Note that the generated executables will rely upon Cygwin (e.g. `cygwin1.dll`) for POSIX compatibility.

#### Cross-compiling from Linux to Microsoft Windows with MinGW-W64 ####
It is possible to cross-compile from a Linux host system to generate Microsoft Windows executables using the [MinGW-W64](https://mingw-w64.org) compilers.
These can usually be installed with your Linux distribution's package manager (e.g. `sudo apt-get install mingw-w64` on Debian/Ubuntu).

Unlike Cygwin, MinGW executables will depend upon the standard Microsoft Visual C++ run-time.

To build VTR using MinGW:
```shell
#In the VTR root
$ mkdir build_win64
$ cd build_win64

#Run cmake specifying the toolchain file to setup the cross-compilation environment
$ cmake .. -DCMAKE_TOOLCHAIN_FILE ../cmake/toolchains/mingw-linux-cross-compile-to-windows.cmake

#Building will produce Windows executables
$ make
```

Note that by default the MS Windows target system will need to dynamically link to the `libgcc` and `libstdc++` DLLs.
These are usually found under /usr/lib/gcc on the Linux host machine.

See the [toolchain file](cmake/toolchains/mingw-linux-cross-compile-to-windows.cmake) for more details.

#### Microsoft Visual Studio ####
CMake can generate a Microsft Visual Studio project, enabling VTR to be built with the Microsoft Visual C++ (MSVC) compiler.

##### Installing additional tools #####
VTR depends on some external unix-style tools during it's buid process; in particular the `flex` and `bison` parser generators.

One approach is to install these tools using [MSYS2](http://www.msys2.org/), which provides up-to-date versions of many unix tools for MS Windows.

To ensure CMake can find the `flex` and `bison` executables you must ensure that they are available on your system path.
For instance, if MSYS2 was installed to `C:\msys64` you would need to ensure that `C:\msys64\usr\bin` was included in the system PATH environment variable.

##### Generating the Visual Studio Project #####
CMake (e.g. the `cmake-gui`) can then be configured to generate the MSVC project.
