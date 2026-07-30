# Wildebeest

The Wildebeest project is an open-source RTL synthesis tool that builds on the mature [Yosys](https://github.com/YosysHQ/yosys) platform and extends it with advanced logic synthesis algorithms for state-of-the-art quality of results (QoR).

The project is initially focused on supporting [Platypus FPGAs](https://www.zeroasic.com/platypus). However, most of the Platypus-specific optimization passes are general-purpose and can be easily adapted for other targets. The long-term goal of Wildebeest is to serve as a common hierarchical synthesis engine, providing a library of high-performance optimization passes that can be shared between targets. The groundwork for this effort has already begun with the introduction of the `-config` option.

The table below shows how Wildebeest compares against both open-source and proprietary synthesis tools on the [picorv32 CPU design](https://raw.githubusercontent.com/zeroasiccorp/logikbench/refs/heads/main/logikbench/blocks/picorv32/rtl/picorv32.v). To run Wildebeest across a broader set of benchmarks, see [LogikBench](https://github.com/zeroasiccorp/logikbench).


| Device   | Arch   |  Tool       |    Synthesis Command      | LUTs  | Logic Depth |
|----------|--------|-------------|---------------------------|:-----:|:-----------:|
| z1060    |  LUT6  | wildebeest  | synth_fpga                | 2312  |     42      |
| z1060    |  LUT6  | wildebeest  | synth_fpga -opt delay     | 2677  |     6       |
| Vendor-1 |  LUT6  | vendor      | (proprietary)             | 2870  |     7       |
| Vendor-2 |  LUT6  | vendor      | (proprietary)             | 2947  |     8       |
| xc7      |  LUT6  | yosys (0.56)| synth_xilinx -nocarry     | 3072  |     17      |
|----------|--------|-------------|---------------------------|-------|-------------|
| z1010    |  LUT4  | wildebeest  | synth_fpga                | 3593  |     39      |
| z1010    |  LUT4  | wildebeest  | synth_fpga -opt delay     | 4112  |     8       |
| ice40    |  LUT4  | yosys (0.56)| synth_ice40 -dsp -nocarry | 4378  |     33      |


> **NOTE:** We made a best effort to isolate synthesis QoR from hardware-specific details (e.g., LUT4 vs. LUT6, presence or absence of carry cells). For Yosys, this required disabling carry cells to match the Platypus architecture, which does not yet support them.

## Prerequisites

* Compiler: >=GCC 11 or >=clang 17
* CMake: 3.20 ... 3.29
* Yosys: 0.47+0 ... 0.56+0 are supported.
* Yosys: 0.57+0 and above are now supported. 

**Important:** Please note that the Yosys version you use must not be in between official versions.
For instance, make sure you use 0.57+0 (with +0 new commit since the official version) and not for example 0.57+100 (official Yosys 0.57 + 100 commits after). 

## Building

```bash
git clone git@github.com:zeroasiccorp/wildebeest.git
cd wildebeest
cmake -S . -B build
cmake --build build
sudo cmake --install build
```

> **NOTE:** The `cmake` flow builds the plugin and copies all files to the yosys executable share directory. As an example, if the `yosys` executable is located at /usr/local/bin/yosys, then the `wildebeest` plugin will be copied to /usr/local/share/yosys/plugins/wildebeest.so.

## Quick start

The following "hello world" example runs synthesis on the [picorv32 CPU](https://raw.githubusercontent.com/zeroasiccorp/logikbench/refs/heads/main/logikbench/blocks/picorv32/rtl/picorv32.v). We encourage you to try your own RTL and report back results.

```
plugin -i wildebeest
read_verilog picorv32.v
hierarchy -check -top picorv32
synth_fpga -partname z1010
```

## Limitations

* No carry chain inference
* No tri-state circuit inference
* No latch inference

## License

The Wildebeest project is licensed under [Apache 2.0 license](LICENSE). Yosys derived code is licensed under the ISC license.

## Command Options

To get a listing of all `synth_fpga` options, use the yosys built-in `help` command.

```
yosys> plugin -i wildebeest
yosys> help synth_fpga

    synth_fpga [options]

This command runs Zero Asic FPGA synthesis flow.

    -top <module>
        Use the specified module as top module, in case more than one exists.

    -opt <mode>
        Specifies optimization mode [area, delay, fast] (default=area).

    -partname <name>
        Specifies architecture partname [z1000, z1010]. (default=z1010).

    -config <file name>
        Specifies the config file setting main 'synth_fpga' parameters.

    -show_config
        Show the parameters set by the config file.

    -no_flatten
        skip flatening. By default, design is flatened.

    -no_bram
        Bypass BRAM inference. It is off by default.

    -use_bram_tech [zeroasic]
        Invoke architecture specific DSP inference. It is off by default. -no_bram 
        overides -use_bram_tech.

    -no_dsp
        Bypass DSP inference. It is off by default.

    -use_dsp_tech [zeroasic, bare_mult]
        Invoke architecture specific DSP inference. It is off by default. -no_dsp 
        overides -use_dsp_tech.

    -no_dsp_pack
        Disable DSP packing.

    -fsm_encoding [one-hot, binary]
        Specifies FSM encoding : by default a 'one-hot' encoding is performed.

    -resynthesis
        switch synthesis flow to resynthesis mode which means a lighter flow.
        It can be used only after performing a first 'synth_fpga' synthesis pass 

    -insbuf
        performs buffers insertion (Off by default).

    -no_xor_tree_process
        Disable xor trees depth reduction for DELAY mode (Off by default).

    -autoname
        Generate, if possible, better wire and cells names close to RTL names rather than
        $abc generic names. This is off by default. Be careful because it may blow up runtime.

    -no_dff_enable
        specifies that DFF with enable feature is not supported. By default,
        DFF with enable is supported.

    -no_dff_async_set
        specifies that DFF with asynchronous set feature is not supported. By default,
        DFF with asynchronous set is supported.

    -no_dff_async_reset
        specifies that DFF with asynchronous reset feature is not supported. By default,
        DFF with asynchronous reset is supported.

    -no_opt_sat_dff
        Disable SAT-based DFF optimizations. This is off by default.

    -no_opt_const_dff
        Disable constant driven DFF optimization as it can create simulation differences (since it may ignore DFF init values in some cases). This is off by default.

    -set_dff_init_value_to_zero
        Set un-initialized DFF to initial value 0. Insert double inverters for DFF with initial value 1 and switch its initial value to 0 and modify its clear/set/reset functionalities if any. This is off by default.

    -show_dff_init_value
        Show all DFF initial values coming from the original RTL. This is off by default.

    -continue_if_latch
        Keep running Synthesis even if some Latch inference is involved. The final netlist will not be valid but it can be usefull to get the final netlist stats. This is off by default.

    -no_sdff
        Disable synchronous set/reset DFF mapping. It is off by default.

    -stop_if_undriven_nets
        Stop Synthesis if the final netlist has undriven nets. This is off by default.

    -obs_clean
        specifies to use 'obs_clean' cleanup function instead of regular 
        'opt_clean'. This is off by default.

    -lut_size
        specifies lut size. By default lut size is 4.

    -verilog <file>
        write the design to the specified Verilog netlist file. writing of an
        output file is omitted if this parameter is not specified.

    -show_max_level
        Show longest paths. This is off by default except if we are in delay mode.

    -csv
        Dump a 'stat.csv' file. This is off by default.

    -wait
        wait after each 'stat' report for user to touch <enter> key. Help for 
        flow analysis/debug.


```

## FPGA Specification Format
------------------------
The FPGA configuration option makes the `synth_fpga` command a powerful general purpose synthesis plugin rather than a bespoke command targeted at a single FPGA hardware target.

```
yosys > synth_fpga -config <configuration file>
```

The configuration format is still work in progress. A draft format specification is shown below.

```
{
        "version": <int, version of file schema, current version is 1>,
        "partname": <str, name of the fpga part>,
        "root_path": <path, absolute path where all other paths are relative to>, (optional),
        "lut_size": <int, size of lut>,
        "flipflops": {
                "features": [<str, list of features file async_set, flop_enable, async_reset, etc.>],
                "models": {
                        "<str, ff name>": <path, relative path to DFFs model file>
                },
                "techmap": <path, relative path to yosys DFFs tech mapping file>
        },
        "brams": { (optional)
                "memory_libmap": [<path, relative path to yosys memory mapping file>, ...],
                "memory_libmap_parameters": [<parameter setting>, ...] (optional),
                "techmap": [<path, relative path to yosys memory tech mapping>, ...]
        },
        "dsps": { (optional)
                "family": <str, name of dsp family>,
                "techmap": <path, relative path to yosys dsp tech mapping file>,
                "techmap_parameters": {
                        "<str, name of define>": <str or int, value of define>
                }
                "pack_command": "DSP packing command" (optional)
        }
}
```

Sections version, partname, lut_size, flip-flops, are required. Sections root_path, brams,and dsps are optional. If "root_path" is not specified, it will correspond to the path where the config file is located.

### Logic Only Example: No BRAMs or DSPs

```
{
    "version": 1,
    "partname": "z1000",
    "lut_size": 4,
    "flipflops": {
        "features": ["async_reset", "async_set", "flop_enable"],
        "models": {
            "dffers": "/ffs/dffers.v",
            "dffer": "/ffs/dffer.v",
            "dffes": "/ffs/dffes.v",
            "dffe": "/ffs/dffe.v",
            "dffrs": "/ffs/dffrs.v",
            "dffr": "/ffs/dffr.v",
            "dffs": "/ffs/dffs.v",
            "dff": "/ffs/dff.v"
        },
        "techmap": "/tech_flops.v"
    }
}
```
### Full Example: With BRAMs and DSPs

```json
{
    "version": 1,
    "partname": "z1010",
    "lut_size": 4,
    "flipflops": {
        "features": ["async_reset", "async_set", "flop_enable"],
        "models": {
            "dffers": "/ffs/dffers.v",
            "dffer": "/ffs/dffer.v",
            "dffes": "/ffs/dffes.v",
            "dffe": "/ffs/dffe.v",
            "dffrs": "/ffs/dffrs.v",
            "dffr": "/ffs/dffr.v",
            "dffs": "/ffs/dffs.v",
            "dff": "/ffs/dff.v"
        },
        "techmap": "/tech_flops.v"
    },
    "brams": {
        "memory_libmap": ["/bram_memory_map.txt"],
        "techmap": ["/tech_bram.v"]
    },
    "dsps": {
        "family": "DSP48",
        "techmap": "/mult18x18_DSP48.v",
        "techmap_parameters": {
            "DSP_A_MAXWIDTH": 18,
            "DSP_B_MAXWIDTH": 18,
            "DSP_A_MINWIDTH": 2,
            "DSP_B_MINWIDTH": 2,
            "DSP_Y_MINWIDTH": 9,
            "DSP_SIGNEDONLY": 1,
            "DSP_NAME": "$__MUL18X18"
        }
    }
}
```
### Third Party Example: With BRAM and DSPs
```json
{
  "version": 1,
  "partname": "z1010",
  "lut_size": 4,
  "root_path" : "/home/user/YOSYS_DYN/yosys/wildebeest/",
  "flipflops": {
                "features": ["async_reset", "async_set", "flop_enable"],
                "models": {
                        "dffers": "src/ff_models/dffers.v",
                        "dffer": "src/ff_models/dffer.v",
                        "dffes": "src/ff_models/dffes.v",
                        "dffe": "src/ff_models/dffe.v",
                        "dffrs": "src/ff_models/dffrs.v",
                        "dffr": "src/ff_models/dffr.v",
                        "dffs": "src/ff_models/dffs.v",
                        "dff": "src/ff_models/dff.v"
                },
                "techmap": "architecture/z1010/techlib/tech_flops.v"
        },
  "brams": {
            "memory_libmap": [ "architecture/z1010/bram/LSRAM.txt",
                               "architecture/z1010/bram/uSRAM.txt"],
            "memory_libmap_parameters": ["-logic-cost-rom 0.5"],
            "techmap": ["architecture/z1010/bram/LSRAM_map.v",
                        "architecture/z1010/bram/uSRAM_map.v"]
        },
  "dsps": {
          "family": "microchip",
          "techmap": "../techlibs/microchip/polarfire_dsp_map.v",
          "techmap_parameters": {
                   "DSP_A_MAXWIDTH": 18,
                   "DSP_B_MAXWIDTH": 18,
                   "DSP_A_MAXWIDTH_PARTIAL": 18,
                   "DSP_A_MINWIDTH": 2,
                   "DSP_B_MINWIDTH": 2,
                   "DSP_Y_MINWIDTH": 9,
                   "DSP_SIGNEDONLY": 1,
                   "DSP_NAME": "$__MUL18X18"
          },
          "pack_command": "microchip_dsp -family polarfire"
       }
}
```
