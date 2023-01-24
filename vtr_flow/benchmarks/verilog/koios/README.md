# Koios 2.0 Benchmarks

## Introduction
Koios benchmarks are a set of Deep Learning (DL) benchmarks for FPGA architecture and CAD research. They are suitable for DL related architecture and CAD research. There are 40 designs that include several medium-sized benchmarks and some large benchmarks. The designs target different network types (CNNs, RNNs, MLPs, RL) and layer types (fully-connected, convolution, activation, softmax, reduction, eltwise). Some of the designs are generated from HLS tools as well. These designs use many precisions including binary, different fixed point types int8/16/32, brain floating point (bfloat16), and IEEE half-precision floating point (fp16).

## Documentation
A brief documentation of Koios benchmarks is available [here](https://docs.verilogtorouting.org/en/latest/vtr/benchmarks/#koios-benchmarks).

## How to Use
Koios benchmarks are fully compatible with the full VTR flow. They can be used using the standard VTR flow described [here](https://docs.verilogtorouting.org/en/latest/vtr/running_vtr/). 

Koios benchmarks use advanced DSP features that are available in only a few FPGA architectures provided with VTR. These benchmarks instantiate hard DSP macros to implement native FP16 or BF16 multiplications or use the hard dedicated chains, and these are architecture-specific. However, these advanced/complex DSP features can be enabled or disabled. The macro ``complex_dsp`` can be used for this purpose. If `complex_dsp` is defined in a benchmark file (using `` `define complex_dsp``), then advanced DSP features mentioned above will be used. If `complex_dsp` is not defined, then equivalent functionality is obtained through behavioral Verilog that gets mapped to the FPGA soft logic.

Similarly, Koios benchmark instantiate hard memory blocks (single port and dual port BRAM). These blocks are available in most architectures provided with VTR, but may not be available in a user's architecture. These hard memory blocks can be controlled by using the macro ``hard_mem``. If `hard_mem` is defined in a benchmark file (using `` `define hard_mem``), then hard memory blocks will be used. If `hard_mem` is not defined, then equivalent functionality is obtained through behavioral Verilog that the synthesis tools can automatically infer as RAMs.

From a flow perspective, you can enable/disable a macro (like `complex_dsp`) without actually modifying the benchmark file(s). You can specify a separate Verilog header file while running a flow/task that contains these macros. For `run_vtr_flow` users, `-include <filename>` needs to be added. For `run_vtr_task` users, `includes_dir` and `include_list_add` need to be specified in the task file. An example task file can be seen [here](https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/tasks/regression_tests/vtr_reg_basic/hdl_include/config/config.txt).

Using hard macros for DSPs and BRAMs to utilize advanced features (like chaining) is common in modern designs used with contemporary FPGAs. When using these benchmarks and enabling these advanced features, an FPGA architecture that supports these features must be provided. Supporting these features implies that the architecture XML file provided to VTR must describe such features (e.g. by defining a hard block macro DSP slice). We provide such architectures with Koios. The FPGA architectures with advanced DSP that work out-of-the-box with Koios benchmarks are available here: 

    $VTR_ROOT/vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.*


When disabling these hard blocks (e.g. by not defining `complex_dsp` or `hard_mem` as mentioned above), users can run these benchmarks with FPGA architectures that don't have these hard blocks in them. That is, an architecture XML file without the required hard macro definitions can be used. For example, the flagship architectures available here: ::

    $VTR_ROOT/vtr_flow/arch/timing/k6_frac_N10_*_mem32K_40nm*

If users want to use a different FPGA architecture file, they can replace the macro instantiations in the benchmarks with their equivalents from the FPGA architectures they wish to use.

## Proxy benchmarks
In Koios 2.0, there are 8 synthetic/proxy benchmarks. These were generated using a framework that is present [here](https://github.com/UT-LCA/koios_proxy_benchmarks). To generate more benchmarks using this framework, use the generate_benchmark.py script.

## Regressions
Koios benchmarks are tested by the following regression tests in VTR:
| Suite         |Test Description      | Config file   | Wall-clock time   |
|---------------|----------------------|---------------|-------------------|
| Strong        | A test circuit. Goal is to check the architecture files.   | tasks/regression_tests/vtr_reg_strong/koios | 6 seconds |
| Strong        | Same test circuit without enabling complex dsp features    | tasks/regression_tests/vtr_reg_strong/koios_no_complex_dsp | 6 seconds|
| Nightly       | Small-to-medium sized designs from Koios run with one arch file                           | tasks/regression_tests/vtr_reg_nightly_test4/koios | 2 hours with -j3 |
| Nightly       | Small-to-medium sized designs from Koios run with an arch file without enabling complex dsp features  | tasks/regression_tests/vtr_reg_nightly_test4/koios_no_complex_dsp | 2 hours with -j3 |
| Nightly       | A small design from Koios run with various flavors of the arch file that enables complex dsp features  | tasks/regression_tests/vtr_reg_nightly_test4/koios_multi_arch | 2 hours with -j3 |
| Weekly        | Large designs from Koios run with one arch file                           | tasks/regression_tests/vtr_reg_weekly/koios | a little over 24 hours with -j4 |
| Weekly        | Large designs from Koios run with an arch file without enabling complex dsp features  | tasks/regression_tests/vtr_reg_weekly/koios_no_complex_dsp | a little over 24 hours with -j4 |

## Collecting QoR measurements
For collecting QoR measurements on Koios benchmarks, follow the instructions [here](https://docs.verilogtorouting.org/en/latest/README.developers/#example-koios-benchmarks-qor-measurement).

## VTR issues observed

### Fixed
Some of the main issues (and/or pull requests) that were seen and fixed during Koios 2.0 development:

#### Multiple PBs using the same Model
https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/2103

#### Random net names
https://github.com/verilog-to-routing/vtr-verilog-to-routing/pull/2086

#### Incorrect clock and reset signals
https://github.com/verilog-to-routing/vtr-verilog-to-routing/pull/2118

#### SystemVerilog support
https://github.com/verilog-to-routing/vtr-verilog-to-routing/pull/2015
https://github.com/verilog-to-routing/vtr-verilog-to-routing/pull/2068

#### Auto-name pass after optimization passes
https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/2031
https://github.com/verilog-to-routing/vtr-verilog-to-routing/pull/2035

#### Padding the multiplication output port if it's size is less than the sum of both input ports
https://github.com/verilog-to-routing/vtr-verilog-to-routing/pull/2013

#### Support for synthesis/mapping of reduce and shift operators
https://github.com/verilog-to-routing/vtr-verilog-to-routing/pull/1923

#### Error during placement with hard blocks with chains
https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/2036

### Open
Some issues that are still pending to be fixed are:

#### Placement failure with designs with hard block chains:
https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/2149


## How to Cite
The following paper may be used as a citation for Koios:

A. Arora, A. Boutros, D. Rauch, A. Rajen, A. Borda, S. A. Damghani, S. Mehta, S. Kate, P. Patel, K. B. Kent, V. Betz, L. K. John "Koios: A Deep Learning Benchmark Suite for FPGA Architecture and CAD Research", FPL, 2021.

Bibtex:
```
@inproceedings{koios_benchmarks,
  title={Koios: A Deep Learning Benchmark Suite for FPGA Architecture and CAD Research},
  author={Arora, Aman and Boutros, Andrew and Rauch, Daniel and Rajen, Aishwarya and Borda, Aatman and Damghani, Seyed A. and Mehta, Samidh and Kate, Sangram and Patel, Pragnesh and Kent, Kenneth B. and Betz, Vaughn and John, Lizy K.},
  booktitle={International Conference on Field Programmable Logic and Applications (FPL)},
  year={2021}
}
```

## License
Koios benchmarks are distributed under the [same license as VTR](https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/LICENSE.md). 

## Contact
If you have questions, email [Aman Arora](mailto:aman.kbm@utexas.edu). 

If you find any bugs, please file an issue [here](https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues) and tag @aman26kbm in the description.

## Contributing to Koios
We welcome contributions to this benchmark suite. If you'd like to contribute to Koios, contact [Aman Arora](mailto:aman.kbm@utexas.edu). 

## Contributors
*Please keep this up-to-date*

**Professors:** Lizy K. John, Vaughn Betz, Kenneth B. Kent

**Doctoral Students:** Aman Arora (University of Texas at Austin), Andrew Boutros (University of Toronto), Mohamed Elgammal (University of Toronto)

**Graduate Students:** Seyed Alireza Damghani (University of New Brunswick), Daniel Rauch (University of Texas at Austin), Aishwarya Rajen (University of Texas at Austin), , Sangram Kate (University of Texas at Austin)

**Undergraduate Students:** Karan Mathur (University of Texas at Austin), Vedant Mohanty (University of Texas at Austin), Tanmay Anand (University of Texas at Austin), Aatman Borda (University of Texas at Austin), Samidh Mehta (University of Texas at Austin), Pragnesh Patel (University of Texas at Austin), Helen Dai (University of Toronto), Zack Zheng (University of Toronto)

