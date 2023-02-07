# Parmys (Partial Mapper for Yosys) Plugin

This repository contains intellignet partial mapper plugin from [Odin-II](https://github.com/verilog-to-routing/vtr-verilog-to-routing/tree/master/ODIN_II) for [Yosys](https://github.com/YosysHQ/yosys.git).

The project build skeleton is based on [Yosys F4PGA Plugins](https://github.com/chipsalliance/yosys-f4pga-plugins.git) project.

It is highly recommended to utilize this plugin through the [Verilog to Routing (VTR)](https://github.com/verilog-to-routing/vtr-verilog-to-routing.git) project.

## Build as a plugin

- Clone, make, and install [Yosys](https://github.com/YosysHQ/yosys.git)
- Clone, make, and install [VTR](https://github.com/verilog-to-routing/vtr-verilog-to-routing.git)
- Build and install Parmys

```makefile
make VTR_INSTALL_DIR=`path to VTR build/install directory` plugins -j`nproc`
sudo make install
```

## Parameters
Available paramters are:
```
    -a ARCHITECTURE_FILE
        VTR FPGA architecture description file (XML)

    -c XML_CONFIGURATION_FILE
        Configuration file

    -top top_module
        set the specified module as design top module

    -nopass
        No additional passes will be executed.

    -exact_mults int_value
        To enable mixing hard block and soft logic implementation of adders

    -mults_ratio float_value
        To enable mixing hard block and soft logic implementation of adders

    -vtr_prim
        loads vtr primitives as modules, if the design uses vtr prmitives then this flag is mandatory for first run
```

## Usage (without VTR)

Example for simple partial mapping with parmys:

```sh
# run yosys
yosys

# load the plugin
plugin -i parmys

# read verilog files
read_verilog my_verilog.v

# use parmys to read the architecture file and partial mapping
parmys -a simple_vtr_fpga_architecture.xml

```

## Usage (within VTR flow)

- Clone [VTR](https://github.com/verilog-to-routing/vtr-verilog-to-routing.git)
- [Set up the Environment](https://docs.verilogtorouting.org/en/latest/BUILDING/#setting-up-your-environment)
- Build with the following command to enable both Yosys as the frontend and Parmys as the partial mapper:
```makefile
make CMAKE_PARAMS="-DWITH_YOSYS=on -DYOSYS_PARMYS_PLUGIN=on"
```
- Run vtr flow

```makefile
cd vtr_flow/scripts/

# this command runs the vtr flow [yosys+parmys, abc, vpr]
./run_vtr_flow.py my_verilog.v fpga_architecture.xml -start yosys -end vpr -mapper parmys
```

For detailed information please refer to the [VTR documentation](https://docs.verilogtorouting.org/en/latest/).

Detailed help on the supported command(s) can be obtained by running `help parmys` in Yosys.
