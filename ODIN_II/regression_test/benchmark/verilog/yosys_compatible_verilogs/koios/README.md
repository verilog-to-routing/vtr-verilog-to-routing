# Koios Benchmarks

## Introduction
Koios benchmarks are a set of Deep Learning (DL) benchmarks for FPGA architecture and CAD research. They are suitable for DL related architecture and CAD research. There are 19 designs that include several medium-sized benchmarks and some large benchmarks. The designs target different network types (CNNs, RNNs, MLPs, RL) and layer types (fully-connected, convolution, activation, softmax, reduction, eltwise). Some of the designs are generated from HLS tools as well. These designs use many precisions including binary, different fixed point types int8/16/32, brain floating point (bfloat16), and IEEE half-precision floating point (fp16).

## Documentation
A brief documentation of Koios benchmarks is available [here](https://docs.verilogtorouting.org/en/latest/vtr/benchmarks/#koios-benchmarks).

## How to Use
Koios benchmarks are fully compatible with the full VTR flow. They can be used using the standard VTR flow described [here](https://docs.verilogtorouting.org/en/latest/vtr/running_vtr/). 

Some Koios benchmarks use advanced DSP features that are available in only a few FPGA architectures provided with VTR. These benchmarks instantiate DSP macros to implement native FP16 or BF16 multiplications or use the hard dedicated chains, and these are architecture-specific. However, these advanced/complex DSP features can be enabled or disabled. The macro ``complex_dsp`` can be used for this purpose. If `complex_dsp` is defined in a benchmark file (using `` `define complex_dsp``), then advanced DSP features mentioned above will be used. If `complex_dsp` is not defined, then equivalent functionality is obtained through behavioral Verilog that gets mapped to the FPGA soft logic.

From a flow perspective, a feature was recently added in VTR (June 2021) that makes it easy to enable/disable a macro (like `complex_dsp`). The feature provides for specifying a separate Verilog header file while running a flow/task, so a benchmark's Verilog file doesn't have to be modified. For `run_vtr_flow` users, `-include <filename>` needs to be added. For `run_vtr_task` users, `includes_dir` and `include_list_add` need to be specified in the task file. An example task file can be seen [here](https://github.com/verilog-to-routing/vtr-verilog-to-routing/blob/master/vtr_flow/tasks/regression_tests/vtr_reg_basic/hdl_include/config/config.txt).

Using such advanced DSP features is common in modern designs used with contemporary FPGAs. When using these benchmarks and enabling these advanced features, an FPGA architecture that supports these features must be provided. Supporting these features implies that the architecture XML file provided to VTR must describe such features (e.g. by defining a hard block macro DSP slice). We provide such architectures with Koios. The FPGA architectures with advanced DSP that work out-of-the-box with Koios benchmarks are available here: 

    $VTR_ROOT/vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.*


When disabling these advanced features (by not defining `complex_dsp` as mentioned above), users can run these benchmarks with FPGA architectures that don't have these advanced DSP features. That is, an architecture XML file without the required hard macro definitions can be used. For example, the flagship architectures available here: ::

    $VTR_ROOT/vtr_flow/arch/timing/k6_frac_N10_*_mem32K_40nm*

If users want to use a different FPGA architecture file, they can replace the macro instantiations in the benchmarks with their equivalents from the FPGA architectures they wish to use.

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
For collecting QoR measurements on Koios benchmarks, follow the instructions [here](https://docs.verilogtorouting.org/en/latest/dev/developing/#collecting-qor-measurements).

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

**Doctoral Students:** Aman Arora (University of Texas at Austin), Andrew Boutros (University of Toronto)

**Graduate Students:** Daniel Rauch (University of Texas at Austin), Aishwarya Rajen (University of Texas at Austin), Seyed Alireza Damghani (University of New Brunswick), Sangram Kate (University of Texas at Austin)

**Undergraduate Students:** Aatman Borda (University of Texas at Austin), Samidh Mehta (University of Texas at Austin), Pragnesh Patel (University of Texas at Austin), Helen Dai (University of Toronto), Zack Zheng (University of Toronto)

