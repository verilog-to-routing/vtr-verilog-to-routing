"""
    Module to run ODIN II with its various arguments
"""
import os
import shutil
from collections import OrderedDict
from pathlib import Path
import xml.etree.ElementTree as ET
import vtr

# supported input file type by Odin
FILE_TYPES = {
    ".v": "verilog",
    ".vh": "verilog",
    ".sv": "verilog",
    ".svh": "verilog",
    ".blif": "blif",
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
def init_config_file(
    odin_config_full_path,
    circuit_list,
    architecture_file,
    output_netlist,
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


# pylint: disable=too-many-arguments, too-many-locals
def run(
    architecture_file,
    circuit_file,
    include_files,
    output_netlist,
    command_runner=vtr.CommandRunner(),
    temp_dir=Path("."),
    odin_args="--adder_type default",
    log_filename="odin.out",
    odin_exec=None,
    odin_config=None,
    min_hard_mult_size=3,
    min_hard_adder_size=1,
):
    """
    Runs ODIN II on the specified architecture file and circuit file

    .. note :: Usage: vtr.odin.run(<architecture_file>,<circuit_file>,<output_netlist>,[OPTIONS])

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

        odin_args:
            A dictionary of keyword arguments to pass on to ODIN II

        log_filename :
            File to log result to

        odin_exec:
            ODIN II executable to be run

        odin_config:
            The ODIN II configuration file

        min_hard_mult_size :
            Tells ODIN II the minimum multiplier size that should be implemented using
            hard multiplier (if available)

        min_hard_adder_size :
            Tells ODIN II the minimum adder size that should be implemented
            using hard adder (if available).

    """
    temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir
    temp_dir.mkdir(parents=True, exist_ok=True)

    if odin_args is None:
        odin_args = OrderedDict()

    # Verify that files are Paths or convert them to Paths and check that they exist
    architecture_file = vtr.verify_file(architecture_file, "Architecture")
    circuit_file = vtr.verify_file(circuit_file, "Circuit")
    output_netlist = vtr.verify_file(output_netlist, "Output netlist", False)

    if odin_exec is None:
        odin_exec = str(vtr.paths.odin_exe_path)

    if odin_config is None:
        odin_base_config = str(vtr.paths.odin_cfg_path)
    else:
        odin_base_config = str(Path(odin_config).resolve())

    # Copy the config file
    odin_config = "odin_config.xml"
    odin_config_full_path = str(temp_dir / odin_config)
    shutil.copyfile(odin_base_config, odin_config_full_path)

    # Create a list showing all (.v) and (.vh) files
    circuit_list = create_circuits_list(circuit_file, include_files)

    init_config_file(
        odin_config_full_path,
        circuit_list,
        architecture_file.name,
        output_netlist.name,
        vtr.determine_memory_addr_width(str(architecture_file)),
        min_hard_mult_size,
        min_hard_adder_size,
    )

    cmd = [odin_exec]
    use_odin_simulation = False

    if "use_odin_simulation" in odin_args:
        use_odin_simulation = True
        del odin_args["use_odin_simulation"]

    for arg, value in odin_args.items():
        if isinstance(value, bool) and value:
            cmd += ["--" + arg]
        elif isinstance(value, (str, int, float)):
            cmd += ["--" + arg, str(value)]
        else:
            pass

    cmd += ["-U0"]

    if "disable_odin_xml" in odin_args:
        del odin_args["disable_odin_xml"]
        cmd += [
            "-a",
            architecture_file.name,
            "-V",
            circuit_list,
            "-o",
            output_netlist.name,
        ]
    else:
        cmd += ["-c", odin_config]

    command_runner.run_system_command(
        cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1
    )

    if use_odin_simulation:
        sim_dir = temp_dir / "simulation_init"
        sim_dir.mkdir()
        cmd = [
            odin_exec,
            "-b",
            output_netlist.name,
            "-a",
            architecture_file.name,
            "-sim_dir",
            str(sim_dir),
            "-g",
            "100",
            "--best_coverage",
            "-U0",
        ]
        command_runner.run_system_command(
            cmd,
            temp_dir=temp_dir,
            log_filename="sim_produce_vector.out",
            indent_depth=1,
        )


# pylint: enable=too-many-arguments, too-many-locals
