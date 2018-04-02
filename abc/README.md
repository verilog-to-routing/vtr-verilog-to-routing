[![Build Status](https://travis-ci.org/berkeley-abc/abc.svg?branch=master)](https://travis-ci.org/berkeley-abc/abc)
[![Build status](https://ci.appveyor.com/api/projects/status/7q8gopidgvyos00d?svg=true)](https://ci.appveyor.com/project/berkeley-abc/abc)

# ABC: System for Sequential Logic Synthesis and Formal Verification

ABC is always changing but the current snapshot is believed to be stable. 

## Compiling:

To compile ABC as a binary, download and unzip the code, then type `make`.
To compile ABC as a static library, type `make libabc.a`.

When ABC is used as a static library, two additional procedures, `Abc_Start()` 
and `Abc_Stop()`, are provided for starting and quitting the ABC framework in 
the calling application. A simple demo program (file src/demo.c) shows how to 
create a stand-alone program performing DAG-aware AIG rewriting, by calling 
APIs of ABC compiled as a static library.

To build the demo program

 * Copy demo.cc and libabc.a to the working directory
 * Run `gcc -Wall -g -c demo.c -o demo.o`
 * Run `g++ -g -o demo demo.o libabc.a -lm -ldl -lreadline -lpthread`

To run the demo program, give it a file with the logic network in AIGER or BLIF. For example:

    [...] ~/abc> demo i10.aig
    i10          : i/o =  257/  224  lat =    0  and =   2396  lev = 37
    i10          : i/o =  257/  224  lat =    0  and =   1851  lev = 35
    Networks are equivalent.
    Reading =   0.00 sec   Rewriting =   0.18 sec   Verification =   0.41 sec

The same can be produced by running the binary in the command-line mode:

    [...] ~/abc> ./abc
    UC Berkeley, ABC 1.01 (compiled Oct  6 2012 19:05:18)
    abc 01> r i10.aig; b; ps; b; rw -l; rw -lz; b; rw -lz; b; ps; cec
    i10          : i/o =  257/  224  lat =    0  and =   2396  lev = 37
    i10          : i/o =  257/  224  lat =    0  and =   1851  lev = 35
    Networks are equivalent.

or in the batch mode:

    [...] ~/abc> ./abc -c "r i10.aig; b; ps; b; rw -l; rw -lz; b; rw -lz; b; ps; cec"
    ABC command line: "r i10.aig; b; ps; b; rw -l; rw -lz; b; rw -lz; b; ps; cec".
    i10          : i/o =  257/  224  lat =    0  and =   2396  lev = 37
    i10          : i/o =  257/  224  lat =    0  and =   1851  lev = 35
    Networks are equivalent.

## Compiling as C or C++

The current version of ABC can be compiled with C compiler or C++ compiler.

 * To compile as C code (default): make sure that `CC=gcc` and `ABC_NAMESPACE` is not defined.
 * To compile as C++ code without namespaces: make sure that `CC=g++` and `ABC_NAMESPACE` is not defined.
 * To compile as C++ code with namespaces: make sure that `CC=g++` and `ABC_NAMESPACE` is set to
   the name of the requested namespace. For example, add `-DABC_NAMESPACE=xxx` to OPTFLAGS.

## Building a shared library

 * Compile the code as position-independent by adding `ABC_USE_PIC=1`.
 * Build the `libabc.so` target: 
 
     make ABC_USE_PIC=1 libabc.so

## Bug reporting:

Please try to reproduce all the reported bugs and unexpected features using the latest 
version of ABC available from https://github.com/berkeley-abc/abc

If the bug still persists, please provide the following information:    

 1. ABC version (when it was downloaded from GitHub)
 1. Linux distribution and version (32-bit or 64-bit)
 1. The exact command-line and error message when trying to run the tool
 1. The output of the `ldd` command run on the exeutable (e.g. `ldd abc`).
 1. Versions of relevant tools or packages used.


## Troubleshooting:

 1. If compilation does not start because of the cyclic dependency check, 
try touching all files as follows: `find ./ -type f -exec touch "{}" \;`
 1. If compilation fails because readline is missing, install 'readline' library or
compile with `make ABC_USE_NO_READLINE=1`
 1. If compilation fails because pthreads are missing, install 'pthread' library or
compile with `make ABC_USE_NO_PTHREADS=1`
    * See http://sourceware.org/pthreads-win32/ for pthreads on Windows
    * Precompiled DLLs are available from ftp://sourceware.org/pub/pthreads-win32/dll-latest
 1. If compilation fails in file "src/base/main/libSupport.c", try the following:
    * Remove "src/base/main/libSupport.c" from "src/base/main/module.make"
    * Comment out calls to `Libs_Init()` and `Libs_End()` in "src/base/main/mainInit.c"
 1. On some systems, readline requires adding '-lcurses' to Makefile.

The following comment was added by Krish Sundaresan:

"I found that the code does compile correctly on Solaris if gcc is used (instead of 
g++ that I was using for some reason). Also readline which is not available by default 
on most Sol10 systems, needs to be installed. I downloaded the readline-5.2 package 
from sunfreeware.com and installed it locally. Also modified CFLAGS to add the local 
include files for readline and LIBS to add the local libreadline.a. Perhaps you can 
add these steps in the readme to help folks compiling this on Solaris."

The following tutorial is kindly offered by Ana Petkovska from EPFL:
https://www.dropbox.com/s/qrl9svlf0ylxy8p/ABC_GettingStarted.pdf

## Final remarks:

Unfortunately, there is no comprehensive regression test. Good luck!                                

This system is maintained by Alan Mishchenko <alanmi@berkeley.edu>. Consider also 
using ZZ framework developed by Niklas Een: https://bitbucket.org/niklaseen/abc-zz (or https://github.com/berkeley-abc/abc-zz)
