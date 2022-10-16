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

YOSYS_LIB_FILES = {
    "YSMDL": "yosys_models.v",
    "SPRAM": "single_port_ram.v",
    "DPRAM": "dual_port_ram.v",
    "SPRAMR": "spram_rename.v",
    "DPRAMR": "dpram_rename.v",
    "DSPBB": "arch_dsps.v",
}

YOSYS_PARSERS = ["yosys", "surelog", "yosys-plugin"]

YOSYS_MAPPERS = ["parmys", "yosys"]


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
    yosys_models_full_path,
    yosys_spram_full_path,
    yosys_dpram_full_path,
    yosys_spram_rename_full_path,
    yosys_dpram_rename_full_path,
    architecture_dsp_full_path,
    circuit_list,
    output_netlist,
    memory_addr_width,
    min_hard_mult_size,
    min_hard_adder_size,
    architecture_file_path,
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
            "YYY": yosys_models_full_path,
            "SSS": yosys_spram_full_path,
            "DDD": yosys_dpram_full_path,
            "SSR": yosys_spram_rename_full_path,
            "DDR": yosys_dpram_rename_full_path,
            "CCC": architecture_dsp_full_path,
            "TTT": str(vtr.paths.yosys_lib_path),
            "ZZZ": output_netlist,
            "QQQ": architecture_file_path,
        },
    )

    # Update the config file
    vtr.file_replace(
        yosys_models_full_path,
        {"PPP": memory_addr_width, "MMM": min_hard_mult_size, "AAA": min_hard_adder_size},
    )

    # Update the config file files
    vtr.file_replace(yosys_spram_full_path, {"PPP": memory_addr_width})
    vtr.file_replace(yosys_dpram_full_path, {"PPP": memory_addr_width})
    vtr.file_replace(yosys_spram_rename_full_path, {"PPP": memory_addr_width})
    vtr.file_replace(yosys_dpram_rename_full_path, {"PPP": memory_addr_width})


# pylint: disable=too-many-arguments, too-many-locals
def init_config_file(
    odin_config_full_path,
    circuit_list,
    architecture_file,
    output_netlist,
    odin_parser_arg,
    memory_addr_width,
    min_hard_mult_size,
    min_hard_adder_size,
):
    """initializing the raw odin config file"""
    # specify the input files type
    file_extension = os.path.splitext(circuit_list[0])[-1]
    if file_extension not in FILE_TYPES:
        raise vtr.VtrError("Inavlid input file type '{}'".format(file_extension))
    input_file_type = FILE_TYPES[file_extension]

    # Check if the user specifically requested for the UHDM parser
    if odin_parser_arg == "-u":
        input_file_type = "uhdm"

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
    yosys_args="",
    log_filename="yosys.out",
    yosys_exec=None,
    yosys_script=None,
    min_hard_mult_size=3,
    min_hard_adder_size=1,
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
        if yosys_args["mapper"] == "parmys":
            yosys_base_script = str(vtr.paths.yosys_parmys_script_path)
        else:
            yosys_base_script = str(vtr.paths.yosys_script_path)
    else:
        yosys_base_script = str(Path(yosys_script).resolve())

    # Copy the script file
    yosys_script = "synthesis.tcl"
    yosys_script_full_path = str(temp_dir / yosys_script)
    shutil.copyfile(yosys_base_script, yosys_script_full_path)

    # Copy the yosys models file
    yosys_models = YOSYS_LIB_FILES["YSMDL"]
    yosys_base_models = str(vtr.paths.yosys_lib_path / YOSYS_LIB_FILES["YSMDL"])
    yosys_models_full_path = str(vtr.paths.scripts_path / temp_dir / yosys_models)
    shutil.copyfile(yosys_base_models, yosys_models_full_path)

    # Copy the VTR memory blocks file
    yosys_spram = YOSYS_LIB_FILES["SPRAM"]
    yosys_dpram = YOSYS_LIB_FILES["DPRAM"]
    yosys_spram_rename = YOSYS_LIB_FILES["SPRAMR"]
    yosys_dpram_rename = YOSYS_LIB_FILES["DPRAMR"]
    yosys_base_spram = str(vtr.paths.yosys_lib_path / YOSYS_LIB_FILES["SPRAM"])
    yosys_base_dpram = str(vtr.paths.yosys_lib_path / YOSYS_LIB_FILES["DPRAM"])
    yosys_base_spram_rename = str(vtr.paths.yosys_lib_path / YOSYS_LIB_FILES["SPRAMR"])
    yosys_base_dpram_rename = str(vtr.paths.yosys_lib_path / YOSYS_LIB_FILES["DPRAMR"])
    yosys_spram_full_path = str(vtr.paths.scripts_path / temp_dir / yosys_spram)
    yosys_dpram_full_path = str(vtr.paths.scripts_path / temp_dir / yosys_dpram)
    yosys_spram_rename_full_path = str(vtr.paths.scripts_path / temp_dir / yosys_spram_rename)
    yosys_dpram_rename_full_path = str(vtr.paths.scripts_path / temp_dir / yosys_dpram_rename)
    shutil.copyfile(yosys_base_spram, yosys_spram_full_path)
    shutil.copyfile(yosys_base_dpram, yosys_dpram_full_path)
    shutil.copyfile(yosys_base_spram_rename, yosys_spram_rename_full_path)
    shutil.copyfile(yosys_base_dpram_rename, yosys_dpram_rename_full_path)

    write_arch_bb_exec = str(vtr.paths.write_arch_bb_exe_path)
    architecture_dsp_full_path = str(vtr.paths.scripts_path / temp_dir / YOSYS_LIB_FILES["DSPBB"])
    architecture_file_path = str(vtr.paths.scripts_path / architecture_file)

    # executing write_arch_bb to extract the black box definitions of the given arch file
    command_runner.run_system_command(
        [
            write_arch_bb_exec,
            str(vtr.paths.scripts_path / architecture_file),
            architecture_dsp_full_path,
        ],
        temp_dir=temp_dir,
        log_filename="write_arch_bb.log",
        indent_depth=1,
    )

    # Create a list showing all (.v) and (.vh) files
    circuit_list = create_circuits_list(circuit_file, include_files)

    init_script_file(
        yosys_script_full_path,
        yosys_models_full_path,
        yosys_spram_full_path,
        yosys_dpram_full_path,
        yosys_spram_rename_full_path,
        yosys_dpram_rename_full_path,
        architecture_dsp_full_path,
        circuit_list,
        output_netlist.name,
        vtr.determine_memory_addr_width(str(architecture_file)),
        min_hard_mult_size,
        min_hard_adder_size,
        architecture_file_path,
    )

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
        None,
        vtr.determine_memory_addr_width(str(architecture_file)),
        min_hard_mult_size,
        min_hard_adder_size,
    )

    # set the parser
    if yosys_args["parser"] in YOSYS_PARSERS:
        os.environ["PARSER"] = yosys_args["parser"]
        del yosys_args["parser"]
    else:
        raise vtr.VtrError(
            "Invalid parser is specified for Yosys, available parsers are [{}]".format(
                " ".join(str(x) for x in YOSYS_PARSERS)
            )
        )

    # set the partial mapper
    if yosys_args["mapper"] in YOSYS_MAPPERS:
        os.environ["MAPPER"] = yosys_args["mapper"]
        del yosys_args["mapper"]
    else:
        raise vtr.VtrError(
            "Invalid partial mapper is specified for Yosys, available parsers are [{}]".format(
                " ".join(str(x) for x in YOSYS_MAPPERS)
            )
        )

    cmd = [yosys_exec]

    for arg, value in yosys_args.items():
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