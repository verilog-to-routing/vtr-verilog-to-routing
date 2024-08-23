"""
    Module to run Parmys with its various arguments
"""
import os
import shutil
from collections import OrderedDict
from pathlib import Path
import xml.etree.ElementTree as ET
import vtr

# supported input file type by Yosys
FILE_TYPES = {
    ".v": "Verilog",
    ".vh": "Verilog",
    ".sv": "SystemVerilog",
    ".svh": "SystemVerilog",
    ".blif": "BLIF",
    ".aig": "aiger",
    ".json": "JSON",
    ".lib": "Liberty",
    ".ys": "RTLIL",
}

YOSYS_PARSERS = ["default", "surelog", "system-verilog"]


def create_circuits_list(main_circuit, include_files):
    """Create a list of supported HDL files"""
    circuit_list = []
    # Check include files exist
    if include_files:
        # Verify that files are Paths or convert them to Paths + check that they exist
        for include in include_files:
            file_extension = os.path.splitext(include)[-1]
            # if the include file is not in the supported HDLs, we drop it
            # NOTE: the include file is already copied to the temp folder
            if file_extension not in FILE_TYPES:
                continue

            include_file = vtr.verify_file(include, "Circuit")
            circuit_list.append(include_file.name)

    # Append the main circuit design as the last one
    circuit_list.append(main_circuit.name)

    return circuit_list


# pylint: disable=too-many-arguments, too-many-locals
def init_script_file(
    yosys_script_full_path,
    circuit_list,
    output_netlist,
    architecture_file_path,
    odin_config_full_path,
    include_dir='.',
    top_module='-auto-top'
):
    """initializing the raw yosys script file"""
    # specify the input files type
    for circuit in circuit_list:
        file_extension = os.path.splitext(circuit)[-1]
        if file_extension not in FILE_TYPES:
            raise vtr.VtrError("Inavlid input file type '{}'".format(file_extension))

    # Update the config file
    vtr.file_replace(
        yosys_script_full_path,
        {
            "XXX": "{}".format(" ".join(str(s) for s in circuit_list)),
            # "TTT": str(vtr.paths.yosys_tcl_path),
            "CCC": odin_config_full_path,
            "ZZZ": output_netlist,
            "QQQ": architecture_file_path,
            "SEARCHPATH": include_dir,
            "TOPMODULE": top_module,
        },
    )


# pylint: disable=too-many-arguments, too-many-locals
def init_config_file(
    odin_config_full_path,
    circuit_list,
    architecture_file,
    output_netlist,
    memory_addr_width,
    min_hard_mult_size,
    min_hard_adder_size,
):
    """initializing the raw xml config file"""
    # specify the input files type
    file_extension = os.path.splitext(circuit_list[0])[-1]
    if file_extension not in FILE_TYPES:
        raise vtr.VtrError("Invalid input file type '{}'".format(file_extension))
    input_file_type = FILE_TYPES[file_extension]

    # Update the config file
    vtr.file_replace(
        odin_config_full_path,
        {
            "YYY": architecture_file,
            "TTT": input_file_type,
            "ZZZ": output_netlist,
            "PPP": memory_addr_width,
            "MMM": min_hard_mult_size,
            "AAA": min_hard_adder_size,
        },
    )

    # loading the given config file
    config_file = ET.parse(odin_config_full_path)
    root = config_file.getroot()

    # based on the base config file
    verilog_files_tag = root.find("inputs")
    # remove the template line XXX, verilog_files_tag [1] is a comment
    verilog_files_tag.remove(verilog_files_tag[1])
    for circuit in circuit_list:
        verilog_file = ET.SubElement(verilog_files_tag, "input_path_and_name")
        verilog_file.tail = "\n\n\t" if (circuit == circuit_list[-1]) else "\n\n\t\t"
        verilog_file.text = circuit

    # update the config file with new values
    config_file.write(odin_config_full_path)


# pylint: disable=too-many-arguments, too-many-locals, too-many-statements, too-many-branches
def run(
    architecture_file,
    circuit_file,
    include_files,
    output_netlist,
    command_runner=vtr.CommandRunner(),
    temp_dir=Path("."),
    parmys_args="",
    log_filename="parmys.out",
    yosys_exec=None,
    yosys_script=None,
    min_hard_mult_size=3,
    min_hard_adder_size=1,
):
    """
    Runs Yosys on the specified architecture file and circuit

    .. note :: Usage: vtr.parmys.run(<architecture_file>,<circuit_file>,<output_netlist>,[OPTIONS])

    Arguments
    =========
        architecture_file :
            Architecture file to target

        circuit_file :
            Circuit file to optimize

        include_files :
            list of header files

        output_netlist :
            File name to output the resulting circuit to

    Other Parameters
    ----------------
        command_runner :
            A CommandRunner object used to run system commands

        temp_dir :
            Directory to run in (created if non-existent)

        parmys_args:
            A dictionary of keyword arguments to pass on to Parmys

        log_filename :
            File to log result to

        yosys_exec:
            Yosys executable to be run

        yosys_script:
            The Yosys script file

    """
    temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir
    temp_dir.mkdir(parents=True, exist_ok=True)

    if parmys_args is None:
        parmys_args = OrderedDict()

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
    yosys_script = "synthesis.tcl"
    yosys_script_full_path = str(temp_dir / yosys_script)
    shutil.copyfile(yosys_base_script, yosys_script_full_path)

    # Copy the VTR memory blocks file

    architecture_file_path = str(vtr.paths.scripts_path / architecture_file)

    # Create a list showing all (.v) and (.vh) files
    circuit_list = create_circuits_list(circuit_file, include_files)

    # parse search directory
    if ('searchpath' in parmys_args):
        if (parmys_args['searchpath'] is not None) and (parmys_args['searchpath'] != ''):
            include_dir = parmys_args['searchpath']
        del parmys_args['searchpath']
    else:
        include_dir = '.'

    # parse top module
    # NOTE: the default value is '-auto-top'
    top_module = '-auto-top'
    if ('topmodule' in parmys_args):
        if (parmys_args['topmodule'] is not None) and (parmys_args['topmodule'] != ''):
            top_module = '-top ' + parmys_args['topmodule']
        del parmys_args['topmodule']

    odin_base_config = str(vtr.paths.odin_cfg_path)

    # Copy the config file
    odin_config = "odin_config.xml"
    odin_config_full_path = str(temp_dir / odin_config)
    shutil.copyfile(odin_base_config, odin_config_full_path)

    init_config_file(
        odin_config_full_path,
        circuit_list,
        architecture_file.name,
        output_netlist.name,
        vtr.determine_memory_addr_width(str(architecture_file)),
        min_hard_mult_size,
        min_hard_adder_size,
    )

    init_script_file(
        yosys_script_full_path,
        circuit_list,
        output_netlist.name,
        architecture_file_path,
        odin_config_full_path,
        include_dir=include_dir,
        top_module=top_module
    )

    # set the parser
    if parmys_args["parser"] in YOSYS_PARSERS:
        os.environ["PARSER"] = parmys_args["parser"]
        del parmys_args["parser"]
    else:
        raise vtr.VtrError(
            "Invalid parser is specified for Yosys, available parsers are [{}]".format(
                " ".join(str(x) for x in YOSYS_PARSERS)
            )
        )

    cmd = [yosys_exec]

    for arg, value in parmys_args.items():
        if isinstance(value, bool) and value:
            cmd += ["--" + arg]
        elif isinstance(value, (str, int, float)):
            cmd += ["--" + arg, str(value)]
        else:
            pass

    cmd += ["-c", yosys_script]

    command_runner.run_system_command(
        cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1
    )


# pylint: enable=too-many-arguments, too-many-locals, too-many-statements, too-many-branches
