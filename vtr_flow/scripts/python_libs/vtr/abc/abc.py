"""
    Module to run ABC with its various options
"""
import shutil
from collections import OrderedDict
from pathlib import Path
from vtr import determine_lut_size, verify_file, CommandRunner
from vtr import paths
from vtr.error import InspectError

# pylint: disable=too-many-arguments, too-many-locals
def run(
    architecture_file,
    circuit_file,
    output_netlist,
    command_runner=CommandRunner(),
    temp_dir=Path("."),
    log_filename="abc.out",
    abc_exec=None,
    abc_script=None,
    abc_rc=None,
    use_old_abc_script=False,
    abc_args=None,
    keep_intermediate_files=True,
):
    """
    Runs ABC to optimize specified file.

    .. note :: Usage: vtr.abc.run(<architecture_file>,<circuit_file>,<output_netlist>,[OPTIONS])

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

        log_filename :
            File to log result to

        abc_exec :
            ABC executable to be run

        abc_script :
            The script to be run on abc

        abc_rc :
            The ABC rc file

        use_old_abc_script :
            Enables the use of the old ABC script

        abc_args :
            A dictionary of keyword arguments to pass on to ABC

        keep_intermediate_files :
            Determines if intermediate files are kept or deleted

    """
    temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir
    temp_dir.mkdir(parents=True, exist_ok=True)

    abc_args = OrderedDict() if abc_args is None else abc_args

    # Verify that files are Paths or convert them to Paths and check that they exist
    architecture_file = verify_file(architecture_file, "Architecture")
    circuit_file = verify_file(circuit_file, "Circuit")
    output_netlist = verify_file(output_netlist, "Output netlist", should_exist=False)

    blackbox_latches_script = str(paths.blackbox_latches_script_path)
    clk_list = []
    #
    # Parse arguments
    #
    (
        abc_args,
        abc_flow_type,
        lut_size,
        abc_run_args,
        use_old_latches_restoration_script,
    ) = parse_abc_args(abc_args)

    lut_size = determine_lut_size(str(architecture_file)) if lut_size is None else lut_size

    populate_clock_list(circuit_file, blackbox_latches_script, clk_list, command_runner, temp_dir)

    abc_exec = str(paths.abc_exe_path) if abc_exec is None else abc_exec
    abc_rc = str(paths.abc_rc_path) if abc_rc is None else abc_rc

    shutil.copyfile(str(abc_rc), str(temp_dir / "abc.rc"))

    iterations = len(clk_list)
    iterations = 1 if iterations == 0 or abc_flow_type != "iterative_bb" else iterations

    original_script = abc_script
    input_file = circuit_file.name
    for i in range(0, iterations):
        pre_abc_blif = temp_dir / (str(i) + "_" + circuit_file.name)
        post_abc_blif = temp_dir / (str(i) + "_" + output_netlist.name)
        post_abc_raw_blif = temp_dir / (
            str(i) + "_" + output_netlist.with_suffix("").stem + ".raw.abc.blif"
        )
        if abc_flow_type == "blanket_bb":
            command_runner.run_system_command(
                [blackbox_latches_script, "--input", input_file, "--output", pre_abc_blif.name,],
                temp_dir=temp_dir,
                log_filename=str(i) + "_blackboxing_latch.out",
                indent_depth=1,
            )

        elif len(clk_list) > i:
            command_runner.run_system_command(
                [
                    blackbox_latches_script,
                    "--clk_list",
                    clk_list[i],
                    "--input",
                    input_file,
                    "--output",
                    pre_abc_blif.name,
                ],
                temp_dir=temp_dir,
                log_filename=str(i) + "_blackboxing_latch.out",
                indent_depth=1,
            )
        else:
            pre_abc_blif = input_file

        abc_script = (
            "; ".join(
                [
                    'echo ""',
                    'echo "Load Netlist"',
                    'echo "============"',
                    "read {pre_abc_blif}".format(pre_abc_blif=pre_abc_blif.name),
                    "time",
                    'echo ""',
                    'echo "Circuit Info"',
                    'echo "=========="',
                    "print_stats",
                    "print_latch",
                    "time",
                    'echo ""',
                    'echo "LUT Costs"',
                    'echo "========="',
                    "print_lut",
                    "time",
                    'echo ""',
                    'echo "Logic Opt + Techmap"',
                    'echo "==================="',
                    "strash",
                    "ifraig -v",
                    "scorr -v",
                    "dc2 -v",
                    "dch -f",
                    "if -K {lut_size} -v".format(lut_size=lut_size),
                    "mfs2 -v",
                    "print_stats",
                    "time",
                    'echo ""',
                    'echo "Output Netlist"',
                    'echo "=============="',
                    "write_hie {pre_abc_blif} {post_abc_raw_blif}".format(
                        pre_abc_blif=pre_abc_blif.name, post_abc_raw_blif=post_abc_raw_blif.name,
                    ),
                    "time;",
                ]
            )
            if abc_script is None
            else "; ".join(
                [
                    "read {pre_abc_blif}".format(pre_abc_blif=pre_abc_blif.name),
                    "time",
                    "resyn",
                    "resyn2",
                    "if -K {lut_size}".format(lut_size=lut_size),
                    "time",
                    "scleanup",
                    "write_hie {pre_abc_blif} {post_abc_raw_blif}".format(
                        pre_abc_blif=pre_abc_blif.name, post_abc_raw_blif=post_abc_raw_blif.name,
                    ),
                    "print_stats",
                ]
            )
            if use_old_abc_script
            else abc_script
        )

        cmd = [abc_exec, "-c", abc_script]
        if abc_run_args:
            cmd.append(abc_run_args)

        command_runner.run_system_command(
            cmd,
            temp_dir=temp_dir,
            log_filename=Path(log_filename).stem + str(i) + Path(log_filename).suffix,
            indent_depth=1,
        )

        if abc_flow_type != "blanket_bb" and len(clk_list) > i:
            command_runner.run_system_command(
                [
                    blackbox_latches_script,
                    "--restore",
                    clk_list[i],
                    "--input",
                    post_abc_raw_blif.name,
                    "--output",
                    post_abc_blif.name,
                ],
                temp_dir=temp_dir,
                log_filename="restore_latch" + str(i) + ".out",
                indent_depth=1,
            )
        else:
            restore_multiclock_info_script = (
                str(paths.restore_multiclock_latch_old_script_path)
                if use_old_latches_restoration_script
                else str(paths.restore_multiclock_latch_script_path)
            )
            command_runner.run_system_command(
                [
                    restore_multiclock_info_script,
                    pre_abc_blif.name,
                    post_abc_raw_blif.name,
                    post_abc_blif.name,
                ],
                temp_dir=temp_dir,
                log_filename="restore_latch" + str(i) + ".out",
                indent_depth=1,
            )
        if abc_flow_type != "iterative_bb":
            break

        abc_script = original_script
        input_file = post_abc_blif.name

    command_runner.run_system_command(
        [
            blackbox_latches_script,
            "--input",
            post_abc_blif.name,
            "--output",
            output_netlist.name,
            "--vanilla",
        ],
        temp_dir=temp_dir,
        log_filename="restore_latch" + str(i) + ".out",
        indent_depth=1,
    )
    if not keep_intermediate_files:
        for file in temp_dir.iterdir():
            if file.suffix in (".dot", ".v", ".rc"):
                file.unlink()


# pylint: enable=too-many-arguments, too-many-locals
def parse_abc_args(abc_args):
    """
        function to parse abc_args
    """
    abc_flow_type = "iterative_bb"
    abc_run_args = ""
    lut_size = None
    use_old_latches_restoration_script = False
    if "iterative_bb" in abc_args:
        abc_flow_type = "iterative_bb"
        del abc_args["iterative_bb"]
    if "blanket_bb" in abc_args:
        abc_flow_type = "blanket_bb"
        del abc_args["blanket_bb"]
    if "once_bb" in abc_args:
        abc_flow_type = "once_bb"
        del abc_args["once_bb"]
    if "use_old_latches_restoration_script" in abc_args:
        use_old_latches_restoration_script = True
        del abc_args["use_old_latches_restoration_script"]
    if "lut_size" in abc_args:
        lut_size = abc_args["lut_size"]
        del abc_args["lut_size"]

    for arg, value in abc_args.items():
        if isinstance(value, bool) and value:
            abc_run_args += ["--" + arg]
        elif isinstance(value, (str, int, float)):
            abc_run_args += ["--" + arg, str(value)]
        else:
            pass
    return abc_args, abc_flow_type, lut_size, abc_run_args, use_old_latches_restoration_script


def populate_clock_list(circuit_file, blackbox_latches_script, clk_list, command_runner, temp_dir):
    """
        function to populate the clock list
    """
    clk_list_path = temp_dir / "report_clk.out"
    cmd = [
        blackbox_latches_script,
        "--input",
        circuit_file.name,
        "--output_list",
        clk_list_path.name,
    ]
    command_runner.run_system_command(
        cmd, temp_dir=temp_dir, log_filename="report_clocks.abc.out", indent_depth=1
    )
    with clk_list_path.open("r") as file:
        for line in file.readlines():
            clk_list.append(line.strip("\n"))


# pylint: disable=too-many-arguments
def run_lec(
    reference_netlist,
    implementation_netlist,
    command_runner=CommandRunner(),
    temp_dir=Path("."),
    log_filename="abc.lec.out",
    abc_exec=None,
):
    """
    Run Logical Equivalence Checking (LEC) between two netlists using ABC

    .. note :: Usage: vtr.abc.run_lec(<reference_netlist>,<implementation_netlist>,[OPTIONS])

    Arguments
    =========
        reference_netlist :
            The reference netlist to be commpared to

        implementation_netlist :
            The implemeted netlist to compare to the reference netlist


    Other Parameters
    ----------------
        command_runner :
            A CommandRunner object used to run system commands

        temp_dir :
            Directory to run in (created if non-existent)

        log_filename :
            File to log result to

        abc_exec :
            ABC executable to be run

    """
    temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir
    temp_dir.mkdir(parents=True, exist_ok=True)

    if abc_exec is None:
        abc_exec = str(paths.abc_exe_path)

        abc_script = ("dsec {ref} {imp}".format(ref=reference_netlist, imp=implementation_netlist),)
        abc_script = "; ".join(abc_script)

    cmd = [abc_exec, "-c", abc_script]

    output, _ = command_runner.run_system_command(
        cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1
    )

    # Check if ABC's LEC engine passed
    lec_passed, errored = check_abc_lec_status(output)
    if errored:
        abc_script = ("cec {ref} {imp}".format(ref=reference_netlist, imp=implementation_netlist),)
        abc_script = "; ".join(abc_script)
        cmd = [abc_exec, "-c", abc_script]
        output, _ = command_runner.run_system_command(
            cmd, temp_dir=temp_dir, log_filename="abc.cec.out", indent_depth=1
        )
        lec_passed, errored = check_abc_lec_status(output)
    if lec_passed is None:
        raise InspectError(
            "Couldn't determine Logical Equivalence status between {input} <-> {output}".format(
                input=reference_netlist, output=implementation_netlist
            ),
            filename=log_filename,
        )
    if lec_passed is False:
        raise InspectError(
            "Logical Equivalence Check failed between {input} <-> {output}".format(
                input=reference_netlist, output=implementation_netlist
            ),
            filename=log_filename,
        )

    assert lec_passed


# pylint: enable=too-many-arguments


def check_abc_lec_status(output):
    """
        Reads abc_lec output and determines if the files were equivelent and
        if there were errors when preforming lec.
    """
    equivalent = None
    errored = False
    for line in output:
        if "Error: The network has no latches." in line:
            errored = True
        if line.startswith("Networks are NOT EQUIVALENT"):
            equivalent = False
        elif line.startswith("Networks are equivalent"):
            equivalent = True

    # Returns None if could not determine LEC status
    return equivalent, errored
