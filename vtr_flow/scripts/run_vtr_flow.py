#!/usr/bin/env python3
"""
    Module to run the VTR Flow
"""
import sys
from pathlib import Path
import argparse
import shutil
import textwrap
import socket
from datetime import datetime
from collections import OrderedDict

# pylint: disable=wrong-import-position, import-error
sys.path.insert(0, str(Path(__file__).resolve().parent / "python_libs"))
import vtr

# pylint: enable=wrong-import-position, import-error

BASIC_VERBOSITY = 1

VTR_STAGES = ["odin", "parmys", "abc", "ace", "vpr"]

# pylint: disable=too-few-public-methods
class VtrStageArgparseAction(argparse.Action):
    """
    Class to parse the VTR stages to begin and end at.
    """

    def __call__(self, parser, namespace, value, option_string=None):
        if value == "odin":
            setattr(namespace, self.dest, vtr.VtrStage.ODIN)
        elif value == "parmys":
            setattr(namespace, self.dest, vtr.VtrStage.PARMYS)
        elif value == "abc":
            setattr(namespace, self.dest, vtr.VtrStage.ABC)
        elif value == "vpr":
            setattr(namespace, self.dest, vtr.VtrStage.VPR)
        elif value == "lec":
            setattr(namespace, self.dest, vtr.VtrStage.LEC)
        else:
            raise argparse.ArgumentError(self, "Invalid VTR stage '" + value + "'")


# pylint: enable=too-few-public-methods

# pylint: disable=too-many-statements
def vtr_command_argparser(prog=None):
    """
    The VTR command arg parser
    """
    usage = "%(prog)s circuit_file architecture_file [options]"
    description = textwrap.dedent(
        """
                    Runs a single architecture and circuit through the VTR CAD flow - mapping
                    the circuit onto the target archietcture.

                    Any unrecognzied arguments are passed to VPR.
                    """
    )

    epilog = textwrap.dedent(
        """
                Examples
                --------

                    Produce more output (e.g. individual tool outputs):

                        %(prog)s arch.xml circuit.v -v 2


                    Route at a fixed channel width:

                        %(prog)s arch.xml circuit.v --route_chan_width 100

                        Passing '--route_chan_width 100' will force VPR to route at a fixed channel
                        width, rather then attempting to find the minimum channel width
                        (VPR's default behaviour).


                    Run in the directory 'test' (rather than in a temporary directory):

                        %(prog)s arch.xml circuit.v -temp_dir test


                    Enable power analysis:

                        %(prog)s arch.xml circuit.v -power_tech power_tech.xml


                    Only run specific stage (assumes required results have already been generated):

                        %(prog)s arch.xml circuit.blif -start vpr -end vpr

                """
    )

    parser = argparse.ArgumentParser(
        prog=prog,
        usage=usage,
        description=description,
        epilog=epilog,
        formatter_class=vtr.RawDefaultHelpFormatter,
    )

    #
    # Major arguments
    #
    parser.add_argument("circuit_file", help="The circuit to map to the target architecture.")
    parser.add_argument("architecture_file", help="The FPGA architecture to target.")
    parser.add_argument(
        "-start",
        "-starting_stage",
        choices=VTR_STAGES,
        default=vtr.VtrStage.PARMYS,
        action=VtrStageArgparseAction,
        help="Starting stage of the VTR flow.",
    )
    parser.add_argument(
        "-delete_intermediate_files",
        default=True,
        action="store_false",
        dest="keep_intermediate_files",
        help="Determines if intermediate files are deleted.",
    )
    parser.add_argument(
        "-delete_result_files",
        default=True,
        action="store_false",
        dest="keep_result_files",
        help="Determines if result files are deleted.",
    )
    parser.add_argument(
        "-end",
        "-ending_stage",
        choices=VTR_STAGES,
        default=vtr.VtrStage.VPR,
        action=VtrStageArgparseAction,
        help="Ending stage of the VTR flow.",
    )

    parser.add_argument(
        "-verbose",
        default=False,
        action="store_true",
        dest="verbose",
        help="Verbosity of the script.",
    )
    parser.add_argument(
        "-include",
        nargs="*",
        default=None,
        dest="include_list_file",
        help="List of include files to a benchmark circuit (pass to VTR"
        + " frontends as a benchmark design set)",
    )

    #
    # Power arguments
    #
    power = parser.add_argument_group("Power", description="Power Analysis Related Options")
    power.add_argument(
        "-power",
        default=False,
        action="store_true",
        dest="do_power",
        help="Track the memory usage for each stage. Requires /usr/bin/time -v,"
        + " disabled if not available.",
    )
    power.add_argument(
        "-cmos_tech",
        default=None,
        dest="power_tech",
        metavar="POWER_TECH_FILE",
        help="Enables power analysis using the specified technology file."
        + " Power analysis is disabled if not specified.",
    )

    #
    # House keeping arguments
    #
    house_keeping = parser.add_argument_group(
        "House Keeping",
        description="Configuration related to how files/time/memory are used by the script.",
    )

    house_keeping.add_argument(
        "-temp_dir",
        default=None,
        help="Directory to run the flow in (will be created if non-existent).",
    )

    house_keeping.add_argument("-name", default=None, help="Name for this run to be output.")

    house_keeping.add_argument(
        "-track_memory_usage",
        default=True,
        action="store_true",
        dest="track_memory_usage",
        help="Track the memory usage for each stage."
        + " Requires /usr/bin/time -v, disabled if not available.",
    )

    house_keeping.add_argument(
        "-show_failures",
        default=False,
        action="store_true",
        dest="show_failures",
        help="Tells the flow to display failures in the console.",
    )

    house_keeping.add_argument(
        "-limit_memory_usage",
        default=None,
        metavar="MAX_MEMORY_MB",
        help="Specifies the maximum memory usable by any stage. "
        "Not supported on some platforms (requires ulimit).",
    )

    house_keeping.add_argument(
        "-timeout",
        default=14 * 24 * 60 * 60,  # 14 days
        type=float,
        metavar="TIMEOUT_SECONDS",
        help="Maximum time in seconds to spend on a single stage.",
    )
    house_keeping.add_argument(
        "-expect_fail",
        default=None,
        type=str,
        help="Informs VTR that this run is expected to fail with this message.",
    )
    house_keeping.add_argument(
        "-valgrind",
        default=False,
        action="store_true",
        help="Runs the flow with valgrind with the following options (--leak-check=full,"
        + " --errors-for-leak-kinds=none, --error-exitcode=1, --track-origins=yes)",
    )
    #
    # ABC arguments
    #
    abc = parser.add_argument_group("abc", description="Arguments used by ABC")
    abc.add_argument(
        "-iterative_bb",
        default=False,
        action="store_true",
        dest="iterative_bb",
        help="Use iterative black-boxing flow for multi-clock circuits",
    )
    abc.add_argument(
        "-once_bb",
        default=False,
        action="store_true",
        dest="once_bb",
        help="Use the black-boxing flow a single time",
    )
    abc.add_argument(
        "-blanket_bb",
        default=False,
        action="store_true",
        dest="blanket_bb",
        help="Use iterative black-boxing on all the clocks",
    )
    abc.add_argument(
        "-use_old_latches_restoration_script",
        default=False,
        action="store_true",
        dest="use_old_latches_restoration_script",
        help="Use the old latches restoration script",
    )
    abc.add_argument(
        "-check_equivalent",
        default=False,
        action="store_true",
        help="Enables Logical Equivalence Checks",
    )
    abc.add_argument(
        "-use_old_abc_script",
        default=False,
        action="store_true",
        help="Enables use of legacy ABC script adapted for new ABC",
    )
    abc.add_argument("-lut_size", type=int, help="Tells ABC the LUT size of the FPGA architecture")
    #
    # ODIN II arguments
    #
    odin = parser.add_argument_group("Odin", description="Arguments to be passed to ODIN II")
    odin.add_argument(
        "-adder_type",
        default="default",
        help="Tells ODIN II the adder type used in this configuration",
    )
    odin.add_argument(
        "-adder_cin_global",
        default=False,
        action="store_true",
        dest="adder_cin_global",
        help="Tells ODIN II to connect the first cin in an adder/subtractor"
        + "chain to a global gnd/vdd net.",
    )
    odin.add_argument(
        "-disable_odin_xml",
        default=False,
        action="store_true",
        dest="disable_odin_xml",
        help="Disables the ODIN xml",
    )
    odin.add_argument(
        "-use_odin_simulation",
        default=False,
        action="store_true",
        dest="use_odin_simulation",
        help="Tells odin to run simulation.",
    )
    odin.add_argument(
        "-min_hard_mult_size",
        default=3,
        type=int,
        metavar="min_hard_mult_size",
        help="Tells ODIN II the minimum multiplier size to be implemented using hard multiplier.",
    )
    odin.add_argument(
        "-min_hard_adder_size",
        default=1,
        type=int,
        help="Tells ODIN II the minimum adder size that should be implemented using hard adder.",
    )
    odin.add_argument(
        "-odin_xml",
        default=None,
        dest="odin_config",
        help="Supplies Odin with a custom config file for optimizations.",
    )
    odin.add_argument(
        "-top_module",
        default=None,
        dest="top_module",
        help="Specify the name of the module in the design that should be considered as top",
    )
    odin.add_argument(
        "-mults_ratio",
        default=-1.0,
        dest="mults_ratio",
        help="Specify the percentage of multipliers optimizations",
    )
    odin.add_argument(
        "-adders_ratio",
        default=-1.0,
        dest="adders_ratio",
        help="Specify the percentage of adders optimizations",
    )
    #
    # PARMYS arguments
    #
    parmys = parser.add_argument_group("Parmys", description="Arguments to be passed to Parmys")
    parmys.add_argument(
        "-yosys_script",
        default=None,
        dest="yosys_script",
        help="Supplies Yosys with a .ys script file (similar to Tcl script)"
        + ", including synthesis steps.",
    )
    parmys.add_argument(
        "-parser",
        default="default",
        dest="parser",
        help="Specify a parser for the Yosys synthesizer [default (Verilog-2005), surelog (UHDM), "
        + "system-verilog]. The script used the Yosys conventional Verilog"
        + " parser if this argument is not specified.",
    )
    #
    # VPR arguments
    #
    vpr = parser.add_argument_group(
        "Vpr", description="Arguments to be parsed and then passed to VPR"
    )
    vpr.add_argument(
        "-crit_path_router_iterations",
        type=int,
        default=150,
        help="Tells VPR the amount of iterations allowed to obtain the critical path.",
    )
    vpr.add_argument(
        "-fix_pins",
        type=str,
        help="Controls how the placer handles I/O pads during placement.",
    )
    vpr.add_argument(
        "-relax_w_factor",
        type=float,
        default=1.3,
        help="Factor by which to relax minimum channel width for critical path delay routing",
    )
    vpr.add_argument(
        "-verify_rr_graph",
        default=False,
        action="store_true",
        help="Tells VPR to verify the routing resource graph.",
    )
    vpr.add_argument(
        "-no_second_run",
        default=False,
        action="store_true",
        help="Don't run VPR a second time to check if it can read intermediate files.",
    )
    vpr.add_argument(
        "-rr_graph_ext",
        default=".xml",
        type=str,
        help="Determines the output rr_graph files' extension.",
    )
    vpr.add_argument(
        "-check_route",
        default=False,
        action="store_true",
        help="Tells VPR to run final analysis stage.",
    )
    vpr.add_argument(
        "-check_place",
        default=False,
        action="store_true",
        help="Tells VPR to run routing stage",
    )
    vpr.add_argument(
        "-sdc_file", default=None, type=str, help="Path to SDC timing constraints file."
    )
    vpr.add_argument(
        "-read_vpr_constraints", default=None, type=str, help="Path to vpr constraints file."
    )
    vpr.add_argument(
        "-check_incremental_sta_consistency",
        default=False,
        action="store_true",
        help="Do a second-run of the incremental analysis to compare the result files",
    )
    vpr.add_argument(
        "-verify_inter_cluster_router_lookahead",
        default=False,
        action="store_true",
        help="Tells VPR to verify the inter-cluster router lookahead.",
    )
    vpr.add_argument(
        "-verify_intra_cluster_router_lookahead",
        default=False,
        action="store_true",
        help="Tells VPR to verify the intra-cluster router lookahead. \
        Intra-cluster router lookahead information \
        is stored in a separate data structure than the \
        inter-cluster router lookahead information, \
        and they are written into separate files.",
    )

    return parser


def format_human_readable_memory(num_kbytes):
    """format the number of bytes given as a human readable value"""
    if num_kbytes < 1024:
        value = "%.2f KiB" % (num_kbytes)
    elif num_kbytes < (1024 ** 2):
        value = "%.2f MiB" % (num_kbytes / (1024 ** 1))
    else:
        value = "%.2f GiB" % (num_kbytes / (1024 ** 2))
    return value


def get_max_memory_usage(temp_dir):
    """
    Extracts the maximum memory usage of the VTR flow from generated .out files
    """
    cnt = 0
    output_files = {
        "parmys": Path(temp_dir / "parmys.out"),
        "odin": Path(temp_dir / "odin.out"),
        "abc": Path(temp_dir / "abc{}.out".format(cnt)),
        "vpr": Path(temp_dir / "vpr.out"),
    }
    memory_usages = {"parmys": -1, "odin": -1, "abc": -1, "vpr": -1}

    if output_files["parmys"].is_file():
        memory_usages["parmys"] = get_memory_usage(output_files["parmys"])

    if output_files["odin"].is_file():
        memory_usages["odin"] = get_memory_usage(output_files["odin"])

    while output_files["abc"].is_file():
        new_abc_mem_usage = get_memory_usage(output_files["abc"])
        if new_abc_mem_usage > memory_usages["abc"]:
            memory_usages["abc"] = new_abc_mem_usage

        cnt += 1
        output_files["abc"] = Path(temp_dir / "abc{}.out".format(cnt))

    if output_files["vpr"].is_file():
        memory_usages["vpr"] = get_memory_usage(output_files["vpr"])

    max_mem_key = max(memory_usages, key=memory_usages.get)
    if memory_usages[max_mem_key] != -1:
        return format_human_readable_memory(memory_usages[max_mem_key]), max_mem_key
    return "-", "-"


def get_memory_usage(logfile):
    """Extracts the memory usage from the *.out log files"""
    with open(logfile, "r") as fpmem:
        for line in fpmem.readlines():
            if "Maximum resident set size" in line:
                return int(line.split()[-1])
    return -1


# pylint: enable=too-many-statements


def vtr_command_main(arg_list, prog=None):
    """
    Running VTR with the specified arguemnts.
    """
    start = datetime.now()
    # Load the arguments
    args, unknown_args = vtr_command_argparser(prog).parse_known_args(arg_list)
    error_status = "Error"
    if args.temp_dir is None:
        temp_dir = Path("./temp")
    else:
        temp_dir = Path(args.temp_dir)
    # Specify how command should be run
    command_runner = vtr.CommandRunner(
        track_memory=args.track_memory_usage,
        max_memory_mb=args.limit_memory_usage,
        timeout_sec=args.timeout,
        verbose=args.verbose,
        show_failures=args.show_failures,
        valgrind=args.valgrind,
        expect_fail=args.expect_fail,
    )
    exit_status = 0
    return_status = 0
    try:
        vpr_args = process_unknown_args(unknown_args)
        vpr_args = process_vpr_args(args, prog, temp_dir, vpr_args)
        if args.sdc_file:
            sdc_file_copy = get_sdc_file(args.sdc_file, prog, temp_dir)
            vpr_args["sdc_file"] = Path(sdc_file_copy).name
        if args.read_vpr_constraints:
            vpr_constraint_file_copy = get_read_vpr_constraints(
                args.read_vpr_constraints, prog, temp_dir
            )
            vpr_args["read_vpr_constraints"] = Path(vpr_constraint_file_copy).name

        print(
            args.name
            if args.name
            else Path(args.architecture_file).stem + "/" + Path(args.circuit_file).stem,
            end="\t\t",
        )
        # Run the flow
        vtr.run(
            Path(args.architecture_file),
            Path(args.circuit_file),
            power_tech_file=args.power_tech,
            include_files=args.include_list_file,
            temp_dir=temp_dir,
            start_stage=args.start,
            end_stage=args.end,
            command_runner=command_runner,
            vpr_args=vpr_args,
            abc_args=process_abc_args(args),
            odin_args=process_odin_args(args),
            parmys_args=process_parmys_args(args),
            keep_intermediate_files=args.keep_intermediate_files,
            keep_result_files=args.keep_result_files,
            min_hard_mult_size=args.min_hard_mult_size,
            min_hard_adder_size=args.min_hard_adder_size,
            odin_config=args.odin_config,
            yosys_script=args.yosys_script,
            check_equivalent=args.check_equivalent,
            check_incremental_sta_consistency=args.check_incremental_sta_consistency,
            use_old_abc_script=args.use_old_abc_script,
            relax_w_factor=args.relax_w_factor,
            check_route=args.check_route,
            check_place=args.check_place,
            no_second_run=args.no_second_run,
        )
        error_status = "OK"
    except vtr.VtrError as error:
        error_status, return_status, exit_status = except_vtr_error(
            error, args.expect_fail, args.verbose
        )

    except KeyboardInterrupt as error:
        print("{} received keyboard interrupt".format(prog))
        exit_status = 4
        return_status = exit_status

    finally:
        write_vtr_summary(start, error_status, exit_status, temp_dir)

    if __name__ == "__main__":
        sys.exit(return_status)
    return return_status


def write_vtr_summary(start, error_status, exit_status, temp_dir):
    """
    Write the summary of the results in vtr flow.
    Keep 15 variable limits of pylint in function vtr_command_main.
    """
    seconds = datetime.now() - start
    print(
        "{status} (took {time}, "
        "overall memory peak {stage[0]} consumed by {stage[1]} run)".format(
            status=error_status,
            time=vtr.format_elapsed_time(seconds),
            stage=get_max_memory_usage(temp_dir),
        )
    )
    temp_dir.mkdir(parents=True, exist_ok=True)
    out = temp_dir / "output.txt"
    out.touch()
    with out.open("w") as file:
        file.write("vpr_status=")
        if exit_status == 0:
            file.write("success\n")
        else:
            file.write("exited with return code {}\n".format(exit_status))
        file.write(
            "vpr_seconds=%d\nrundir=%s\nhostname=%s\nerror="
            % (seconds.total_seconds(), str(Path.cwd()), socket.gethostname())
        )
        file.write("\n")


def process_unknown_args(unknown_args):
    """
    We convert the unknown_args into a dictionary, which is eventually
    used to generate arguments for VPR
    """
    vpr_args = OrderedDict()
    while len(unknown_args) > 0:
        # Get the first argument
        arg = unknown_args.pop(0)

        if arg == "":
            continue

        if not arg.startswith("-"):
            raise vtr.VtrError(
                "Extra argument '{}' intended for VPR does not start with '-'".format(arg)
            )

        # To make it a valid kwargs dictionary we trim the initial '-' or '--' from the
        # argument name
        assert len(arg) >= 2
        if arg[1] == "-":
            # Double-dash prefix
            arg = arg[2:]
        else:
            # Single-dash prefix
            arg = arg[1:]

        # Determine if there is a value associated with this argument
        if len(unknown_args) == 0 or (
            unknown_args[0].startswith("-") and arg != "target_ext_pin_util"
        ):
            # Single value argument, we place these with value 'True'
            # in vpr_args
            vpr_args[arg] = True
        else:
            # Multivalue argument
            val = unknown_args.pop(0)
            if len(unknown_args) != 0 and not unknown_args[0].startswith("-"):
                temp = val
                val = []
                val.append(temp)
            while len(unknown_args) != 0 and not unknown_args[0].startswith("-"):
                val.append(unknown_args.pop(0))
            vpr_args[arg] = val

    return vpr_args


def process_abc_args(args):
    """
    Finds arguments needed in the ABC stage of the flow
    """
    abc_args = OrderedDict()
    if args.iterative_bb:
        abc_args["iterative_bb"] = True

    if args.once_bb:
        abc_args["once_bb"] = True

    if args.blanket_bb:
        abc_args["blanket_bb"] = True

    if args.use_old_latches_restoration_script:
        abc_args["use_old_latches_restoration_script"] = True

    if args.lut_size:
        abc_args["lut_size"] = args.lut_size
    return abc_args


def process_odin_args(args):
    """
    Finds arguments needed in the ODIN stage of the flow
    """
    odin_args = OrderedDict()
    odin_args["parser"] = args.parser
    odin_args["adder_type"] = args.adder_type
    odin_args["top_module"] = args.top_module
    odin_args["mults_ratio"] = args.mults_ratio
    odin_args["adders_ratio"] = args.adders_ratio

    if args.adder_cin_global:
        odin_args["adder_cin_global"] = True

    if args.disable_odin_xml:
        odin_args["disable_odin_xml"] = True

    if args.use_odin_simulation:
        odin_args["use_odin_simulation"] = True

    return odin_args


def process_parmys_args(args):
    """
    Finds arguments needed in the PARMYS stage of the flow
    """
    parmys_args = OrderedDict()
    parmys_args["parser"] = args.parser

    return parmys_args


def process_vpr_args(args, prog, temp_dir, vpr_args):
    """
    Finds arguments needed in the VPR stage of the flow
    """
    if args.crit_path_router_iterations:
        vpr_args["crit_path_router_iterations"] = args.crit_path_router_iterations
    if args.fix_pins:
        new_file = str(temp_dir / Path(args.fix_pins).name)
        shutil.copyfile(str((Path(prog).parent.parent / args.fix_pins)), new_file)
        vpr_args["fix_pins"] = new_file
    if args.verify_rr_graph:
        rr_graph_out_file = "rr_graph" + args.rr_graph_ext
        vpr_args["write_rr_graph"] = rr_graph_out_file
    if args.verify_inter_cluster_router_lookahead:
        vpr_args["write_router_lookahead"] = "inter_cluster_router_lookahead.capnp"
    if args.verify_intra_cluster_router_lookahead:
        assert (
            "flat_routing" in vpr_args
        ), "Flat router should be enabled if intra cluster router lookahead is to be verified"
        vpr_args["write_intra_cluster_router_lookahead"] = "intra_cluster_router_lookahead.capnp"

    return vpr_args


def get_sdc_file(sdc_file, prog, temp_dir):
    """
    takes in the sdc_file and returns a path to that file if it exists.
    """
    if not sdc_file.startswith("/"):
        sdc_file = Path(prog).parent.parent / sdc_file
    sdc_file_name = Path(sdc_file).name
    sdc_file_copy = Path(temp_dir) / Path(sdc_file_name)
    shutil.copy(str(sdc_file), str(sdc_file_copy))

    return str(vtr.verify_file(sdc_file_copy, "sdc file"))


def get_read_vpr_constraints(read_vpr_constraints, prog, temp_dir):
    """
    takes in the read_vpr_constraints and returns a path to that file if it exists.
    """
    if not read_vpr_constraints.startswith("/"):
        read_vpr_constraints = Path(prog).parent.parent / read_vpr_constraints
    vpr_constraint_file_name = Path(read_vpr_constraints).name
    vpr_constraint_file_copy = Path(temp_dir) / Path(vpr_constraint_file_name)
    shutil.copy(str(read_vpr_constraints), str(vpr_constraint_file_copy))

    return str(vtr.verify_file(vpr_constraint_file_copy, "vpr constraint file"))


def except_vtr_error(error, expect_fail, verbose):
    """
    Handle vtr exceptions
    """
    error_status = None
    actual_error = None
    exit_status = None
    if isinstance(error, vtr.CommandError):
        # An external command failed
        return_status = 1
        if expect_fail:
            expect_string = expect_fail
            actual_error = None
            if "exited with return code" in expect_string:
                actual_error = "exited with return code {}".format(error.returncode)
            else:
                actual_error = error.msg
            if expect_string != actual_error:
                error_status = "failed: expected '{expected}' but was '{actual}'".format(
                    expected=expect_string, actual=actual_error
                )
                exit_status = 1
            else:
                error_status = "OK"
                return_status = 0
                if verbose:
                    error_status += " (as expected {})".format(expect_string)
                else:
                    error_status += "*"
        else:
            error_status = "failed: {}".format(error.msg)
        if not expect_fail or exit_status:
            print("Error: {msg}".format(msg=error.msg))
            print("\tfull command: ", " ".join(error.cmd))
            print("\treturncode  : ", error.returncode)
            print("\tlog file    : ", error.log)
        exit_status = error.returncode
    elif isinstance(error, vtr.InspectError):
        # Something went wrong gathering information
        print("\tfile        : ", error.filename)
        exit_status = 2
        return_status = exit_status
        error_status = "failed: {}".format(error.msg)

    elif isinstance(error, vtr.VtrError):
        # Generic VTR errors
        exit_status = 3
        return_status = exit_status
        error_status = "failed: {}".format(error.msg)
    return error_status, return_status, exit_status


if __name__ == "__main__":
    sys.exit(vtr_command_main(sys.argv[1:], prog=sys.argv[0]))
