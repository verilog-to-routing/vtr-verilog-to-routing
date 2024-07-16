# Quickstart

## Prerequisites

- ctags
- bison
- flex
- gcc 5.x
- cmake 3.9 (minimum version)
- time
- cairo

## Building

To build you may use the Makefile wrapper in the $VTR_ROOT/odin_ii ``make build`` To build with debug symbols you may use the Makefile wrapper in $VTR_ROOT/odin_ii ``make debug``

> *NOTE*
>
> ODIN uses CMake as it’s build system. CMake provides a portable cross-platform build systems with many useful features.
> For unix-like systems we provide a wrapper Makefile which supports the traditional make and make clean commands, but calls CMake behind the scenes.

> *WARNING*
>
> After you build Odin, please run from the $VTR_ROOT/odin_ii ``make test``.
> This will simulate and verify all of the included microbenchmark circuits to ensure that Odin is working correctly on your system.

## Basic Usage

./odin_ii [arguments]

*Requires one and only one of `-c`, `-v`, or `-b`

| arg  | following argument     | Description                                                                           |
|------|---|---|
| `-c` | XML Configuration File | an XML configuration file dictating the runtime parameters of odin                    |
| `-v` | Verilog HDL File       | You may specify multiple space-separated verilog HDL files                            |
| `-b` | BLIF File              | You may specify multiple space-separated blif files                                   |
| `-o` | BLIF output file       | full output path and file name for the blif output file                               |
| `-a` | architecture file      | You may not specify the architecture file, which results in pure soft logic synthesis |
| `-h` |                        | Print help                                                                            |

## Example Usage

The following are simple command-line arguments and a description of what they do. 
It is assumed that they are being performed in the odin_ii directory.

```bash
   ./odin_ii -v <path/to/verilog/File>
```

Passes a verilog HDL file to Odin II where it is synthesized. 
Warnings and errors may appear regarding the HDL code.

```bash
   ./odin_ii -b <path/to/blif/file>
```

Passes a blif file to Odin II where it is synthesized.

```bash
   ./odin_ii -v <path/to/verilog/File> -a <path/to/arch/file> -o myModel.blif
```

Passes a verilog HDL file and and architecture to Odin II where it is synthesized. 
Odin will use the architecture to do technology mapping.
Odin will output the blif in the current directory at `./myModel.blif`
Warnings and errors may appear regarding the HDL code.
