# Yosys+VTR library files
This directory includes required Verilog models to run the VTR flow using Yosys as its front-end.
The approach of utilizing Yosys as the VTR synthesizer is mainly driven by what Eddie Hung proposed
for the [`VTR-to-Bitstream`](http://eddiehung.github.io/vtb.html) (VTB), based upon VTR 7. Although 
some files, such as [`yosys_models.v`](./yosys_models.v) and [`multiply.v`](./multiply.v), are directly
copied from the VTB project, the other files have been subjected to a few changes due to significant 
alterations from VTR 7 to the current version of VTR. Furthermore, Hung's approach was specifically 
proposed for Xilinx Vertix-6 architecture. As a result, we have applied relevant changes to the remainder
of Yosys library files to make them compatible with the current VTR version and support routine architectures
used in the VTR regression tests.

## What is new compared to the VTB files?
Changes applied to the VTB files are outlined as follows:
 1. Replacing Vertix-6 adder black-box (`xadder`) with the conventional adder used in the current version of VTR.
 2. If required, performing a recursive depth splitting for memory hard blocks, i.e., `single_port_ram` and `dual_port_ram`, to make them adaptable with the VTR flow configurations.
 3. Converting asynchronous DFFs with enable signals (ADFFE) to synchronous form using [`adffe2dffe.v`](./../../../ODIN_II/techlib/adffe2dff.v).
 4. Adding `dffunmap` to transform complex DFF sub-circuits, such as SDFFE (DFF with synchronous reset and enable), to their soft logic implementation, i.e., the combination of multiplexers and latches.
 5. Removing the ABC commands from the Yosys synthesis script and letting the VTR flow's ABC stage performs the technology mapping. (NOTE: the LUT size is considered the one defined in the architecture file as the same as the regular VTR flow)

## How to add new changes?
The Yosys synthesis commands, including the generic synthesis and additional VTR specific configurations, are provided
in [`synthesis.ys`](./synthesis.ys). To make changes in the overall Yosys synthesis flow, the [`synthesis.ys`](./synthesis.ys)
script is perhaps the first file developers may require to change.

Moreover, the [`yosys_models.v`](./yosys_models.v) file includes the required definitions for Yosys to how it should infer implicit
memories and instantiate arithmetic operations, such as addition, subtraction, and multiplication. Therefore, to alter these 
behaviours or add more regulations such as how Yosys should behave when facing other arithmetic operations, for example modulo and division,
the [`yosys_models.v`](./yosys_models.v) Verilog file is required to be modified.

Except for [`single_port_ram.v`](./single_port_ram.v) and [`dual_port_ram.v`](./dual_port_ram.v) Verilog files that perform the depth splitting
process, the other files are defined as black-box, i.e., their declarations are required while no definition is needed. To add new black-box
components, developers should first provide the corresponding Verilog files similar to the [`adder.v`](./adder.v). Then, a new  `read_verilog -lib TTT/NEW_BB.v`
command should be added to the Yosys synthesis script. If there is an implicit inference of the new black-box component, the [`yosys_models.v`](./yosys_models.v)
Verilog file must also be modified, as mentioned earlier.
