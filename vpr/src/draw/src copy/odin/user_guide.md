# User guide

## Synthesis Arguments

|  arg  | following argument      | Description                                            |
|-------|:-----------------------:|------------------------------------------------------- |
| `-c`  |  XML Configuration File | XML runtime directives for the syntesizer such as the verilog file, and parametrized synthesis |
|  `-V` |  Verilog HDL FIle       | You may specify multiple verilog HDL files for synthesis|
|  `-b` |  BLIF File              |                                                        |
|  `-o` |  BLIF output file       | full output path and file name for the blif output file                              |
|  `-a` |  architecture file      | You may specify multiple verilog HDL files for synthesis                             |
|  `-G` |                         | Output netlist graph in GraphViz viewable .dot format. (net.dot, opens with dotty)  |
|  `-A` |                         | Output AST graph in in GraphViz viewable .dot format.                               |
|  `-W` |                         | Print all warnings. (Can be substantial.)                                           |
|  `-h` |                         | Print help                                                                          |

## Simulation Arguments

*To activate simulation you must pass one and only one of the following argument:*

- `-g <number of random vector>`
- `-t <input vector file>`
  
Simulation always produces the folowing files:

- input\_vectors
- output\_vectors
- test.do (ModelSim)

| arg    |  following argument         |  Description                                                                        |
|--------|:---------------------------:|------------------------------------------------------------------------------------|   
|  `-g`  |Number of random test vectors| will simulate the generated netlist with the entered number of clock cycles using pseudo-random test vectors. \\These vectors and the resulting output vectors are written to “input_vectors” and “output_vectors” respectively|
|  `-t`  | Input vector file                 | Supply a predefined input vector file           |
|  `-T` |  output vector file|The output vectors is verified against the supplied predefined output vector file              |
|  `-E` |                    |Output after both edges of the clock. (Default is to output only after the falling edge.)      |
|  `-R` |                    |Output after rising edge of the clock only. (Default is to output only after the falling edge.)|
|  `-p` |comma seperated list|Comma-separated list of additional pins/nodes to monitor during simulation. (view NOTES)       |
|  `-U0` |                                   |initial register value to 0      |
|  `-U1` |                                   |initial resigster value to 1 |
|  `-UX` |                                   |initial resigster value to X (unknown) (DEFAULT) |
|  `-L`  |  Comma seperated list|Comma-separated list of primary inputs to hold high at cycle 0, and low for all subsequent cycles.|
|  `-3`  |   |Generate three valued logic. (Default is binary.) |

## Examples

### Example for `-p`

|  argument                  |    explanation                                                    |
|-----------------------|:------------------------------------------------------:|
| `-p input~0,input~1`  | monitors pin 0 and 1 of input                          |
| `-p input`           | monitors all pins of input as a single port            |
| `-p input~`          | monitors all pins of input as separate ports. (split)  |

> **NOTE**
>
> Matching for `-p` is done via strstr so general strings will match all
> similar pins and nodes. (Eg: FF\_NODE will create a single port with
> all flipflops)

### Example of .xml configuration file for `-c`

```XML
<config>
    <verilog_files>
        <!-- Way of specifying multiple files in a project! -->
        <verilog_file>verilog_file.v</verilog_file>
    </verilog_files>
    <output>
        <!-- These are the output flags for the project -->
        <output_type>blif</output_type>
        <output_path_and_name>./output_file.blif</output_path_and_name>
        <target>
            <!-- This is the target device the output is being built for -->
            <arch_file>fpga_architecture_file.xml</arch_file>
        </target>
    </output>
    <optimizations>
        <!-- This is where the optimization flags go -->
    </optimizations>
    <debug_outputs>
        <!-- Various debug options -->
        <debug_output_path>.</debug_output_path>
        <output_ast_graphs>1</output_ast_graphs>
        <output_netlist_graphs>1</output_netlist_graphs>
    </debug_outputs>
</config>
```

> **NOTE**
>
> Hard blocks can be simulated; given a hardblock named `block` in the
> architecture file with an instance of it named `instance` in the
> verilog file, write a C method with signature defined in
> `SRC/sim_block.h` and compile it with an output filename of
> `block+instance.so` in the directory you plan to invoke Odin\_II from.
>
> When compiling the file, you'll need to specify the following
> arguments to the compiler (assuming that you're in the SANBOX
> directory):
>
> `cc -I../../libarchfpga_6/include/ -L../../libarchfpga_6 -lvpr_6 -lm --shared -o block+instance.so block.c.`
>
> If the netlist generated by Odin II contains the definition of a
> hardblock which doesn't have a shared object file defined for it in
> the working directory, Odin II will not work if you specify it to use
> the simulator with the `-g` or `-t` options.

> **WARNING**
>
> Use of static memory within the simulation code necessitates compiling
> a distinct shared object file for each instance of the block you wish
> to simulate. The method signature the simulator expects contains only
> int and int[] parameters, leaving the code provided to simulate the
> hard blokc agnostic of the internal Odin II data structures. However,
> a cycle parameter is included to provide researchers with the ability
> to delay results of operations performed by the simulation code.
>

### Examples vector file for `-t` or `-T`

``` bash
## Example vector input file
GLOBAL_SIM_BASE_CLK intput_1 input_2 input_3 clk_input
## Comment
0 0XA 1011 0XD 0
0 0XB 0011 0XF 1
0 0XC 1100 0X2 0
```

``` bash
## Example vector output file
output_1 output_2
## Comment
1011 0Xf
0110 0X4
1000 0X5
```

> **NOTE**
>
> Each line represents a vector. Each value must be specified in binary
> or hex. Comments may be included by placing an \# at the start of the
> line. Blank lines are ignored. Values may be separated by non-newline
> whitespace. (tabs and spaces) Hex values must be prefixed with 0X or 0x.
>
> Each line in the vector file represents one cycle, or one falling edge
> and one rising edge. Input vectors are read on a falling edge, while
> output vectors are written on a rising edge.
>
> The input vector file does not have a clock input, it is assumed it is
> controlled by a single global clock that is why it is necessary to add
> a GLOBAL_SIM_BASE_CLK to the input. To read more about this please
> visit [here](http://www.cs.columbia.edu/~cs6861/sis/blif/index.html).

### Examples using vector files `-t` and `-T`

A very useful function of Odin II is to compare the simulated output vector file with the expected output vector file based on an input vector file and a verilog file.
To do this the command line should be:

```shell
./odin_II -V <Path/to/verilog/file> -t <Path/to/Input/Vector/File> -T <Path/to/Output/Vector/File>
```

An error will arrise if the output vector files do not match.

Without an expected vector output file the command line would be:

```shell
./odin_II -V <Path/to/verilog/file> -t <Path/to/Input/Vector/File>
```

The generated output file can be found in the current directory under the name output_vectors.

### Example using vector files `-g`

This function generates N amounnt of random input vectors for Odin II to simulate with.

```shell
./odin_II -V <Path/to/verilog/file> -g 10
```

This example will produce 10 autogenerated input vectors. These vectors can be found in the current directory under input_vectors  and the resulting output vectors can be found under output_vectors.

## Getting Help

If you have any questions or concerns there are multiple outlets to express them.
There is a [google group](https://groups.google.com/forum/#!forum/vtr-users) for users who have questions that is checked regularly by Odin II team members.
If you have found a bug please make an issue in the [vtr-verilog-to-routing GitHub repository](https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues?q=is%3Aopen+is%3Aissue+label%3AOdin).

## Reporting Bugs and Feature Requests

### Creating an Issue on GitHub

Odin II is still in development and there may be bugs present.
If Odin II doesn't perform as expected or doesn't adhere to the Verilog Standard, it is important to create a [bug report](https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/new/choose) in the GitHub repository.
There is a template included, but make sure to include micro-benchmark(s) that reproduces the bug. This micro-benchmark should be as simple as possible.
It is important to link some documentation that provides insight on what Odin II is doing that differs from the Verilog Standard.
Linked below is a pdf of the IEEE Standard of Verilog (2005) that could help.

[IEEE Standard for Verilog Hardware Description Language](http://staff.ustc.edu.cn/~songch/download/IEEE.1364-2005.pdf)

If unsure, there are several outlets to ask questions in the [Help](./help.md) section.

### Feature Requests

If there are any features that the Odin II system overlooks or would be a great addition, please make a [feature request](https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/new/choose) in the GitHub repository. There is a template provided and be as in-depth as possible.
