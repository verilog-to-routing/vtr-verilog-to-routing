"""
    Module to interact with VPR and its various options
"""
from collections import OrderedDict
from pathlib import Path
from os import environ
from vtr import CommandRunner, relax_w, determine_min_w, verify_file, paths
from vtr.error import InspectError

# pylint: disable=too-many-arguments
def run_relax_w(
    architecture,
    circuit,
    circuit_name=None,
    command_runner=CommandRunner(),
    temp_dir=Path("."),
    relax_w_factor=1.3,
    vpr_exec=None,
    logfile_base="vpr",
    vpr_args=None,
):
    """
    Runs VPR twice:

      1st: To find the minimum channel width

      2nd: At relaxed channel width (e.g. for critical path delay)

    .. note :: Use: vtr.vpr.run_relax_w(<architecture_file>,<circuit_file>,[OPTIONS])

    Arguments
    =========
        architecture:
            Architecture file

        circuit:
            Input circuit netlist

    Other Parameters
    ----------------
        circuit_name:
            Name of the circuit file

        command_runner:
            CommandRunner object

        temp_dir:
            Directory to run in

        relax_w_factor:
            Factor by which to relax minimum channel width for critical path delay routing

        vpr_exec:
            Path to the VPR executable

        logfile_base:
            Base name for log files (e.g. "vpr" produces vpr.out, vpr.crit_path.out)

        vpr_args:
            Extra arguments for VPR
    """
    if vpr_args is None:
        vpr_args = OrderedDict()
    temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir
    temp_dir.mkdir(parents=True, exist_ok=True)

    # Verify that files are Paths or convert them to Paths and check that they exist
    architecture = verify_file(architecture, "Architecture")
    circuit = verify_file(circuit, "Circuit")

    vpr_min_w_log = ".".join([logfile_base, "out"])
    vpr_relaxed_w_log = ".".join([logfile_base, "crit_path", "out"])
    max_router_iterations = None

    if "max_router_iterations" in vpr_args:
        max_router_iterations = vpr_args["max_router_iterations"]
        del vpr_args["max_router_iterations"]

    if "write_rr_graph" in vpr_args:
        del vpr_args["write_rr_graph"]

    if vpr_exec is None:
        vpr_exec = str(paths.vpr_exe_path)

    run(
        architecture,
        circuit,
        circuit_name,
        command_runner,
        temp_dir,
        log_filename=vpr_min_w_log,
        vpr_exec=vpr_exec,
        vpr_args=vpr_args,
    )
    explicit = "pack" in vpr_args or "place" in vpr_args or "analysis" in vpr_args
    if explicit and "route" not in vpr_args:
        # Don't look for min W if routing was not run
        return
    if max_router_iterations:
        vpr_args["max_router_iterations"] = max_router_iterations
    min_w = determine_min_w(str(temp_dir / vpr_min_w_log))

    relaxed_w = relax_w(min_w, relax_w_factor)

    vpr_args["route"] = True  # Re-route only
    vpr_args["route_chan_width"] = relaxed_w  # At a fixed channel width

    # VPR does not support performing routing when fixed pins
    # are specified, and placement is not run; so remove the option

    run(
        architecture,
        circuit,
        circuit_name,
        command_runner,
        temp_dir,
        log_filename=vpr_relaxed_w_log,
        vpr_exec=vpr_exec,
        vpr_args=vpr_args,
    )


def run(
    architecture,
    circuit,
    circuit_name=None,
    command_runner=CommandRunner(),
    temp_dir=Path("."),
    log_filename="vpr.out",
    vpr_exec=None,
    vpr_args=None,
):
    """
    Runs VPR with the specified configuration

    .. note :: Usage: vtr.vpr.run(<architecture>,<circuit>,[OPTIONS])

    Arguments
    =========
        architecture:
            Architecture file

        circuit:
            Input circuit file

    Other Parameters
    ----------------
        circuit_name:
            Name of the circuit file

        command_runner:
            CommandRunner object

        temp_dir:
            Directory to run in

        log_filename :
            File to log result to

        vpr_exec:
            Path to the VPR executable

        vpr_args:
            Extra arguments for VPR


    """

    if vpr_args is None:
        vpr_args = OrderedDict()
    temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir
    temp_dir.mkdir(parents=True, exist_ok=True)

    if vpr_exec is None:
        vpr_exec = str(paths.vpr_exe_path)

    # Verify that files are Paths or convert them to Paths and check that they exist
    architecture = verify_file(architecture, "Architecture")
    circuit = verify_file(circuit, "Circuit")
    cmd = []
    if circuit_name:
        cmd = [
            vpr_exec,
            architecture.name,
            circuit_name,
            "--circuit_file",
            circuit.name,
        ]
    else:
        cmd = [vpr_exec, architecture.name, circuit.name]

    # Translate arbitrary keyword arguments into options for VPR

    for arg, value in vpr_args.items():
        if isinstance(value, bool):
            if not value:
                pass
            cmd += ["--" + arg]
        else:
            if isinstance(value, list):
                cmd += ["--" + arg]
                for item in value:
                    cmd += [str(item)]
            else:
                cmd += ["--" + arg, str(value)]

    # Extra options to fine-tune LeakSanitizer (LSAN) behaviour.
    # Note that if VPR was compiled without LSAN these have no effect
    # 'suppressions=...' Add the LeakSanitizer (LSAN) suppression file
    # 'exitcode=12' Use a consistent exitcode
    # (on some systems LSAN don't use the default exit code of 23)
    # 'fast_unwind_on_malloc=0' Provide more accurate leak stack traces

    environ["LSAN_OPTIONS"] = "suppressions={} exitcode=23 fast_unwind_on_malloc=0".format(
        str(paths.lsan_supp)
    )

    command_runner.run_system_command(
        cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1
    )


def run_second_time(
    architecture,
    circuit,
    circuit_name=None,
    command_runner=CommandRunner(),
    temp_dir=Path("."),
    vpr_exec=None,
    second_run_args=None,
    rr_graph_ext=".xml",
):
    """
    Run vpr again with additional parameters.
    This is used to ensure that files generated by VPR can be re-loaded by it

    .. note :: Usage: vtr.vpr.run_second_time(<architecture>,<circuit>,[OPTIONS])

    Arguments
    =========
        architecture:
            Architecture file

        circuit:
            Input circuit file

    Other Parameters
    ----------------
        circuit_name:
            Name of the circuit file

        command_runner:
            CommandRunner object

        temp_dir:
            Directory to run in

        log_filename :
            File to log result to

        vpr_exec:
            Path to the VPR executable

        second_run_args:
            Extra arguments for VPR

    """
    temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir
    temp_dir.mkdir(parents=True, exist_ok=True)

    rr_graph_out_file = ""
    if "write_rr_graph" in second_run_args:
        rr_graph_out_file = second_run_args["write_rr_graph"]
        rr_graph_ext = Path(rr_graph_out_file).suffix

    rr_graph_out_file2 = "rr_graph2" + rr_graph_ext
    if "write_rr_graph" in second_run_args:
        second_run_args["read_rr_graph"] = rr_graph_out_file
        second_run_args["write_rr_graph"] = rr_graph_out_file2

    # run VPR
    run(
        architecture,
        circuit,
        circuit_name,
        command_runner,
        temp_dir,
        log_filename="vpr_second_run.out",
        vpr_exec=vpr_exec,
        vpr_args=second_run_args,
    )

    if "write_rr_graph" in second_run_args:
        cmd = ["diff", rr_graph_out_file, rr_graph_out_file2]
        _, diff_result = command_runner.run_system_command(
            cmd, temp_dir, log_filename="diff.rr_graph.out", indent_depth=1
        )
        if diff_result:
            raise InspectError("failed: vpr (RR Graph XML output not consistent when reloaded)")


def cmp_full_vs_incr_sta(
    architecture,
    circuit,
    circuit_name=None,
    command_runner=CommandRunner(),
    vpr_args=None,
    temp_dir=Path("."),
    vpr_exec=None,
):
    """
    Sanity check that full STA and the incremental STA produce the same *.net, *.place, *.route
    files as well as identical timing report files

    .. note :: Use: vtr.vpr.cmp_full_vs_incr_sta(<architecture>,<circuit_name>,<circuit>,[OPTIONS])

    Arguments
    =========
        architecture:
            Architecture file

        circuit:
            Input circuit file

    Other Parameters
    ----------------
        circuit_name:
            Name of the circuit file

        command_runner:
            CommandRunner object

        temp_dir:
            Directory to run in

        vpr_exec:
            Path to the VPR executable

        vpr_args:
            Extra arguments for VPR
    """

    # Verify that files are Paths or convert them to Paths and check that they exist
    architecture = verify_file(architecture, "Architecture")
    circuit = verify_file(circuit, "Circuit")
    if not circuit_name:
        circuit_name = circuit.stem
    default_output_filenames = [
        "{}.net".format(circuit_name),
        "{}.place".format(circuit_name),
        "{}.route".format(circuit_name),
        "report_timing.setup.rpt",
        "report_timing.hold.rpt",
        "report_unconstrained_timing.setup.rpt",
        "report_unconstrained_timing.hold.rpt",
    ]

    # The full STA flow should have already been run
    # directly rename the output files
    for filename in default_output_filenames:
        cmd = ["mv", filename, "full_sta_{}".format(filename)]
        command_runner.run_system_command(
            cmd, temp_dir=temp_dir, log_filename="move.out", indent_depth=1
        )

    # run incremental STA flow
    incremental_vpr_args = vpr_args
    incremental_vpr_args["timing_update_type"] = "incremental"

    run(
        architecture,
        circuit,
        circuit_name,
        command_runner,
        temp_dir,
        log_filename="vpr.incr_sta.out",
        vpr_exec=vpr_exec,
        vpr_args=incremental_vpr_args,
    )

    # Rename the incremental STA output files
    for filename in default_output_filenames:
        cmd = ["mv", filename, "incremental_sta_{}".format(filename)]
        command_runner.run_system_command(
            cmd, temp_dir=temp_dir, log_filename="move.out", indent_depth=1
        )

    failed_msg = "Failed with these files (not identical):"
    identical = True

    for filename in default_output_filenames:
        cmd = [
            "diff",
            "full_sta_{}".format(filename),
            "incremental_sta_{}".format(filename),
        ]
        _, cmd_return_code = command_runner.run_system_command(
            cmd, temp_dir=temp_dir, log_filename="diff.out", indent_depth=1
        )
        if cmd_return_code:
            identical = False
            failed_msg += " {}".format(filename)

    if not identical:
        raise InspectError(failed_msg)


# pylint: disable=too-many-arguments
