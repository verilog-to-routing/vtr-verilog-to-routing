# F4PGA SystemVerilog Benchmarks

This folder contains the `button_controller`, `pulse_width_led` and `timer` benchmarks directly copied from the master branch of the F4PGA GitHub repository at commit [cad8afe](https://github.com/chipsalliance/f4pga/commits/cad8afe0842cd73f5b73949fa12eab1fda326055).
The benchmarks are directly copied to avoid dealing with a significant amount of code by adding the F4PGA repository as a subtree to the VTR repository.
The primary purpose of these benchmarks is to utilize them in VTR GitHub CI tests to continuously monitor the functionality of the Yosys SystemVerilog and UHDM plugins.

For more information please see the ['ChipsAlliance/F4PGA'](https://github.com/chipsalliance/f4pga) Github repository.

## SystemVerilog File Flattening with `make_sv_flattened.py`

The current SystemVerilog tool, **Synlig**, cannot process multiple files as input (e.g., a top module and its dependencies). To address this limitation, use the script `make_sv_flattened.py` to flatten the files into a single SystemVerilog file. This will convert any design with dependencies into one flattened SystemVerilog file, ensuring compatibility with Synlig.

### Instructions:
1. Ensure the `make_sv_flattened.py` script is located in the folder where your SystemVerilog files (e.g., the top module and its dependencies) are gathered.
2. Run the `make_sv_flattened.py` script on the gathered files in that folder.
3. The script will output a single flattened SystemVerilog file, ready for use with Synlig.

