"""
    Module to run YOSYS with its various arguments
"""
import os
import shutil
from collections import OrderedDict
from pathlib import Path
import xml.etree.ElementTree as ET
import vtr

# supported input file type by Yosys
FILE_TYPES = {
    ".v": "Verilog", ".vh": "Verilog",
    ".sv": "SystemVerilog", ".blif": "BLIF",
    ".aig": "aiger", ".json": "JSON",
    ".lib": "Liberty", ".ys": "RTLIL"
}


def create_circuits_list(main_circuit, include_files):
    """Create a list of all (.v) and (.vh) files"""
    circuit_list = []
    # Check include files exist
    if include_files:
        # Verify that files are Paths or convert them to Paths + check that they exist
        for include in include_files:
            include_file = vtr.verify_file(include, "Circuit")
            circuit_list.append(include_file.name)

    # Append the main circuit design as the last one
    circuit_list.append(main_circuit.name)

    return circuit_list


# pylint: disable=too-many-arguments, too-many-locals
def init_script_file(
    yosys_script_full_path,
    yosys_models_full_path,
    circuit_list,
    output_netlist,
    memory_addr_width,
    min_hard_mult_size,
    min_hard_adder_size
):
    """initializing the raw yosys script file"""
    # specify the input files type
    for circuit in circuit_list:
        file_extension = os.path.splitext(circuit)[-1]
        if file_extension not in FILE_TYPES:
            raise vtr.VtrError(
                "Inavlid input file type '{}'".format(file_extension)
                )

    # Update the config file
    vtr.file_replace(
        yosys_script_full_path,
        {
            "XXX": circuit_list[0],
            "YYY": "./yosys_models.v",
            "TTT": str(vtr.paths.yosys_lib_path),
            "ZZZ": output_netlist
        },
    )

    # Update the config file
    vtr.file_replace(
        yosys_models_full_path,
        {
            "PPP": memory_addr_width,
            "MMM": min_hard_mult_size,
            "AAA": min_hard_adder_size

        },
    )


# pylint: disable=too-many-arguments, too-many-locals
def run(
    architecture_file,
    circuit_file,
    include_files,
    output_netlist,
    command_runner=vtr.CommandRunner(),
    temp_dir=Path("."),
    yosys_args="--adder_type default",
    log_filename="yosys.out",
    yosys_exec=None,
    yosys_script=None,
    min_hard_mult_size=3,
    min_hard_adder_size=1
):
    """
    Runs YOSYS on the specified architecture file and circuit file

    .. note :: Usage: vtr.yosys.run(<architecture_file>,<circuit_file>,<output_netlist>,[OPTIONS])

    Arguments
    =========
        architecture_file :
            Architecture file to target

        circuit_file :
            Circuit file to optimize

        output_netlist :
            File name to output the resulting circuit to

    Other Parameters
    ----------------
        command_runner :
            A CommandRunner object used to run system commands

        temp_dir :
            Directory to run in (created if non-existent)

        yosys_args:
            A dictionary of keyword arguments to pass on to YOSYS

        log_filename :
            File to log result to

        yosys_exec:
            YOSYS executable to be run

        yosys_script:
            The YOSYS script file

    """
    temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir
    temp_dir.mkdir(parents=True, exist_ok=True)

    if yosys_args is None:
        yosys_args = OrderedDict()

    # Verify that files are Paths or convert them to Paths and check that they exist
    architecture_file = vtr.verify_file(architecture_file, "Architecture")
    circuit_file = vtr.verify_file(circuit_file, "Circuit")
    output_netlist = vtr.verify_file(output_netlist, "Output netlist", False)

    if yosys_exec is None:
        yosys_exec = str(vtr.paths.yosys_exe_path)

    if yosys_script is None:
        yosys_base_script = str(vtr.paths.yosys_script_path)
    else:
        yosys_base_script = str(Path(yosys_script).resolve())

    # Copy the script file
    yosys_script = "synthesis.ys"
    yosys_script_full_path = str(temp_dir / yosys_script)
    shutil.copyfile(yosys_base_script, yosys_script_full_path)

    # Copy the yosys models file
    yosys_models = "yosys_models.v"
    yosys_base_models = str(vtr.paths.yosys_lib_path / "yosys_models.v")
    yosys_models_full_path = str(temp_dir / yosys_models)
    shutil.copyfile(yosys_base_models, yosys_models_full_path)

    # Create a list showing all (.v) and (.vh) files
    circuit_list = create_circuits_list(circuit_file, include_files)

    init_script_file(
        yosys_script_full_path,
        yosys_models_full_path,
        circuit_list,
        output_netlist.name,
        vtr.determine_memory_addr_width(str(architecture_file)),
        min_hard_mult_size,
        min_hard_adder_size
    )

    cmd = [yosys_exec]

    for arg, value in yosys_args.items():
        if isinstance(value, bool) and value:
            cmd += ["--" + arg]
        elif isinstance(value, (str, int, float)):
            cmd += ["--" + arg, str(value)]
        else:
            pass

    cmd += ["-s", yosys_script]

    command_runner.run_system_command(
        cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1
    )


# pylint: enable=too-many-arguments, too-many-locals
