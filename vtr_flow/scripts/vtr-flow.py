#!/usr/bin/env python3
import sys
from pathlib import Path
import errno
import argparse
import subprocess
import time
import shutil
import re
import textwrap
from datetime import datetime

from collections import OrderedDict

sys.path.insert(0, str(Path(__file__).resolve().parent / 'python_libs'))
import vtr


BASIC_VERBOSITY = 1


class VtrStageArgparseAction(argparse.Action):
    def __call__(self, parser, namespace, value, option_string=None):
        if value == "odin":
            setattr(namespace, self.dest, vtr.VTR_STAGE.odin)
        elif value == "abc":
            setattr(namespace, self.dest, vtr.VTR_STAGE.abc)
        elif value == "vpr":
            setattr(namespace, self.dest, vtr.VTR_STAGE.vpr)
        elif value == "lec":
            setattr(namespace, self.dest, vtr.VTR_STAGE.lec)
        else:
            raise argparse.ArgumentError(self, "Invalid VTR stage '" + value + "'")

class OnOffArgparseAction(argparse.Action):
    def __call__(self, parser, namespace, value, option_string=None):
        if value.lower() == "on":
            setattr(namespace, self.dest, True)
        elif value.lower() == "off":
            setattr(namespace, self.dest, False)
        else:
            raise argparse.ArgumentError(self, "Invalid yes/no value '" + value + "'")

on_off_choices = ['on', 'off']

def vtr_command_argparser(prog=None):
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

                        Passing '--route_chan_width 100' will force VPR to route at a fixed channel width, 
                        rather then attempting to find the minimum channel width (VPR's default behaviour).


                    Run in the directory 'test' (rather than in a temporary directory):

                        %(prog)s arch.xml circuit.v -temp_dir test


                    Enable power analysis:

                        %(prog)s arch.xml circuit.v -power_tech power_tech.xml


                    Only run a specific stage (assumes required results have already been generated):

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
    parser.add_argument('circuit_file',
                        help="The circuit to map to the target architecture.")
    parser.add_argument('architecture_file',
                        help="The FPGA architecture to target.")
    parser.add_argument("-start", "-starting_stage",
                        choices=vtr.VTR_STAGE.reverse_mapping.values(),
                        default=vtr.VTR_STAGE.odin,
                        action=VtrStageArgparseAction,
                        help="Starting stage of the VTR flow.")
    parser.add_argument("-delete_intermediate_files",
                               default=True,
                               action="store_false",
                               dest="keep_intermediate_files",
                               help="Determines if intermediate files are deleted.")
    parser.add_argument("-delete_result_files",
                               default=True,
                               action="store_false",
                               dest="keep_result_files",
                               help="Determines if result files are deleted.")
    parser.add_argument("-end", "-ending_stage",
                        choices=vtr.VTR_STAGE.reverse_mapping.values(),
                        default=vtr.VTR_STAGE.vpr,
                        action=VtrStageArgparseAction,
                        help="Ending stage of the VTR flow.")

    parser.add_argument("-verbose", "-v",
                        choices=vtr.VERBOSITY_CHOICES,
                        default=2,
                        type=int,
                        help="Verbosity of the script. Higher values produce more output.")

    parser.add_argument("--parse_config_file",
                        default=None,
                        help="Parse file to run after flow completion")

    parser.add_argument("--parse",
                        default=False,
                        action="store_true",
                        dest="parse_only",
                        help="Perform only parsing (assumes a previous run exists in work_dir)")


    #
    # Power arguments
    #
    power = parser.add_argument_group("Power", description="Power Analysis Related Options")
    power.add_argument("-power",
                        default=False,
                        action="store_true",
                        dest="do_power",
                        help="Track the memory usage for each stage. Requires /usr/bin/time -v, disabled if not available.")
    power.add_argument("-cmos_tech",
                       default=None,
                       dest="power_tech",
                       metavar="POWER_TECH_FILE",
                       help="Enables power analysis using the specified technology file. Power analysis is disabled if not specified.")

    #
    # House keeping arguments
    #
    house_keeping = parser.add_argument_group("House Keeping", description="Configuration related to how files/time/memory are used by the script.")

    house_keeping.add_argument("-temp_dir",
                               default=".",
                               help="Directory to run the flow in (will be created if non-existant).")
    
    house_keeping.add_argument("-name",
                               default=None,
                               help="Directory to run the flow in (will be created if non-existant).")

    house_keeping.add_argument("-track_memory_usage",
                               default=False,
                               action="store_true",
                               dest="track_memory_usage",
                               help="Track the memory usage for each stage. Requires /usr/bin/time -v, disabled if not available.")

    house_keeping.add_argument("-show_failures",
                               default=False,
                               action="store_true",
                               dest="show_failures",
                               help="Show failures")#needs updating

    house_keeping.add_argument("-limit_memory_usage",
                               default=None,
                               metavar="MAX_MEMORY_MB",
                               help="Specifies the maximum memory usable by any stage. "
                                    "Not supported on some platforms (requires ulimit).")

    house_keeping.add_argument("-timeout",
                               default=14*24*60*60, #14 days
                               type=float,
                               metavar="TIMEOUT_SECONDS",
                               help="Maximum time in seconds to spend on a single stage.")
    #
    # Iterative black-boxing flow arguments
    #
    iterative = parser.add_argument_group("Iterative", description="Iterative black-boxing flow for multi-clock circuits options")
    iterative.add_argument("-iterative_bb",
                            default=False,
                            action="store_true",
                            dest="iterative_bb",
                            help="Use iterative black-boxing flow for multi-clock circuits")
    iterative.add_argument("-once_bb",
                            default=False,
                            action="store_true",
                            dest="once_bb",
                            help="Use the black-boxing flow a single time")
    iterative.add_argument("-blanket_bb",
                            default=False,
                            action="store_true",
                            dest="blanket_bb",
                            help="Use iterative black-boxing flow with out clocks") #not sure if this is a correct statement. 
    iterative.add_argument("-use_old_latches_restoration_script",
                            default=False,
                            action="store_true",
                            dest="use_old_latches_restoration_script",
                            help="Use the new latches restoration script")
    return parser

def main():
    vtr_command_main(sys.argv[1:])

def vtr_command_main(arg_list, prog=None):
    start = datetime.now()

    #Load the arguments
    args, unkown_args = vtr_command_argparser(prog).parse_known_args(arg_list)

    vtr.print_verbose(BASIC_VERBOSITY, args.verbose, "# {} {}\n".format(prog, ' '.join(arg_list)))

    path_arch_file = Path(args.architecture_file)
    path_circuit_file = Path(args.circuit_file)
    if (args.temp_dir == "."):
        temp_dir="./temp"
    else:
        temp_dir=args.temp_dir


    #Specify how command should be run
    command_runner = vtr.CommandRunner(track_memory=args.track_memory_usage, 
                                   max_memory_mb=args.limit_memory_usage, 
                                   timeout_sec=args.timeout,
                                   verbose_error=True if args.verbose == 2 else False,
                                   verbose=True if args.verbose > 2 else False,
                                   echo_cmd=True if args.verbose >= 4 else False,
                                   show_failures = args.show_failures)
    exit_status = 0
    abc_args = []
    if(args.iterative_bb):
        abc_args.append("iterative_bb")

    if(args.once_bb):
        abc_args.append("once_bb")

    if(args.blanket_bb):
        abc_args.append("blanket_bb")

    if(args.use_old_latches_restoration_script):
        abc_args.append("use_old_latches_restoration_script")
    try:
        if not args.parse_only:
            try:
                vpr_args = process_unkown_args(unkown_args)
                name = ""
                if(args.name):
                    name=args.name
                else:
                    name=path_arch_file.name+"/"+path_circuit_file.name
                print(name)
                #Run the flow
                vtr.run(path_arch_file, 
                             path_circuit_file, 
                             power_tech_file=args.power_tech,
                             temp_dir=temp_dir,
                             start_stage=args.start, 
                             end_stage=args.end,
                             command_runner=command_runner,
                             verbosity=args.verbose,
                             vpr_args=vpr_args,
                             abc_args=abc_args,
                             keep_intermediate_files=args.keep_intermediate_files,
                             keep_result_files=args.keep_result_files
                             )
            except vtr.CommandError as e:
                #An external command failed
                print ("Error: {msg}".format(msg=e.msg))
                print ("\tfull command: ", ' '.join(e.cmd))
                print ("\treturncode  : ", e.returncode)
                print ("\tlog file    : ", e.log)
                exit_status = 1
            except vtr.InspectError as e:
                #Something went wrong gathering information
                print ("Error: {msg}".format(msg=e.msg))
                print ("\tfile        : ", e.filename)
                exit_status = 2

            except vtr.VtrError as e:
                #Generic VTR errors
                print ("Error: ", e.msg)
                exit_status = 3

            except KeyboardInterrupt as e:
                print ("{} recieved keyboard interrupt".format(prog))
                exit_status = 4

        #Parse the flow results
        try:
            vtr.parse_vtr_flow(temp_dir, args.parse_config_file, verbosity=args.verbose)
        except vtr.InspectError as e:
            print ("Error: {msg}".format(msg=e.msg))
            print ("\tfile        : ", e.filename)
            exit_status = 2

    finally:
        vtr.print_verbose(BASIC_VERBOSITY, args.verbose, "\n# {} took {} (exiting {})".format(prog, vtr.format_elapsed_time(datetime.now() - start), exit_status))
    sys.exit(exit_status)

def process_unkown_args(unkown_args):
    #We convert the unkown_args into a dictionary, which is eventually 
    #used to generate arguments for VPR
    vpr_args = OrderedDict()
    while len(unkown_args) > 0:
        #Get the first argument
        arg = unkown_args.pop(0)

        if arg == '':
            continue

        if not arg.startswith('-'):
            raise vtr.VtrError("Extra argument '{}' intended for VPR does not start with '-'".format(arg))

        #To make it a valid kwargs dictionary we trim the initial '-' or '--' from the
        #argument name
        assert len(arg) >= 2
        if arg[1] == '-':
            #Double-dash prefix
            arg = arg[2:]
        else:
            #Single-dash prefix
            arg = arg[1:]


        #Determine if there is a value associated with this argument
        if len(unkown_args) == 0 or unkown_args[0].startswith('-'):
            #Single value argument, we place these with value 'True'
            #in vpr_args
            vpr_args[arg] = True
        else:
            #Multivalue argument
            val = unkown_args.pop(0)
            vpr_args[arg] = val

    return vpr_args
    
if __name__ == "__main__":
    retval = main()
    sys.exit(retval)
