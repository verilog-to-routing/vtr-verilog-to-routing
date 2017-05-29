#!/usr/bin/env python2
import sys
import os
import errno
import argparse
import subprocess
import time
import shutil
import re
import textwrap
from datetime import datetime

from collections import OrderedDict

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'python_libs'))

from verilogtorouting.flow import run_vtr_flow, parse_vtr_flow, VTR_STAGE, vtr_stages, CommandRunner
from verilogtorouting.error import *
from verilogtorouting.util import print_verbose, RawDefaultHelpFormatter, VERBOSITY_CHOICES, find_vtr_file, format_elapsed_time


BASIC_VERBOSITY = 1



class VtrStageArgparseAction(argparse.Action):
    def __call__(self, parser, namespace, value, option_string=None):
        if value == "odin":
            setattr(namespace, self.dest, VTR_STAGE.odin)
        elif value == "abc":
            setattr(namespace, self.dest, VTR_STAGE.abc)
        elif value == "vpr":
            setattr(namespace, self.dest, VTR_STAGE.vpr)
        elif value == "lec":
            setattr(namespace, self.dest, VTR_STAGE.lec)
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
    usage = "%(prog)s architecture_file circuit_file [options]"
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

                        %(prog)s arch.xml circuit.v --work_dir test


                    Enable power analysis:

                        %(prog)s arch.xml circuit.v --power_tech power_tech.xml


                    Only run a specific stage (assumes required results have already been generated):

                        %(prog)s arch.xml circuit.blif --start vpr --end vpr

                """
             )

    parser = argparse.ArgumentParser(
                prog=prog,
                usage=usage,
                description=description,
                epilog=epilog,
                formatter_class=RawDefaultHelpFormatter,
             )

    #
    # Major arguments
    #
    parser.add_argument('architecture_file',
                        help="The FPGA architecture to target.")
    parser.add_argument('circuit_file',
                        help="The circuit to map to the target architecture.")

    parser.add_argument("--start", "--starting_stage",
                        choices=VTR_STAGE.reverse_mapping.values(),
                        default=VTR_STAGE.odin,
                        action=VtrStageArgparseAction,
                        help="Starting stage of the VTR flow.")

    parser.add_argument("--end", "--ending_stage",
                        choices=VTR_STAGE.reverse_mapping.values(),
                        default=VTR_STAGE.vpr,
                        action=VtrStageArgparseAction,
                        help="Ending stage of the VTR flow.")

    parser.add_argument("-v", "--verbosity",
                        choices=VERBOSITY_CHOICES,
                        default=2,
                        type=int,
                        help="Verbosity of the script. Higher values produce more output.")

    parser.add_argument("--parse_config_file",
                        default=find_vtr_file("vpr_standard.txt"),
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
    power.add_argument("--power",
                       default=None,
                       dest="power_tech",
                       metavar="POWER_TECH_FILE",
                       help="Enables power analysis using the specified technology file. Power analysis is disabled if not specified.")

    #
    # House keeping arguments
    #
    house_keeping = parser.add_argument_group("House Keeping", description="Configuration related to how files/time/memory are used by the script.")

    house_keeping.add_argument("--work_dir",
                               default=".",
                               help="Directory to run the flow in (will be created if non-existant).")

    house_keeping.add_argument("--track_memory_usage",
                               choices=on_off_choices,
                               default="on",
                               action=OnOffArgparseAction,
                               help="Track the memory usage for each stage. Requires /usr/bin/time -v, disabled if not available.")

    house_keeping.add_argument("--memory_limit",
                               default=None,
                               metavar="MAX_MEMORY_MB",
                               help="Specifies the maximum memory usable by any stage. "
                                    "Not supported on some platforms (requires ulimit).")

    house_keeping.add_argument("--timeout",
                               default=14*24*60*60, #14 days
                               type=float,
                               metavar="TIMEOUT_SECONDS",
                               help="Maximum time in seconds to spend on a single stage.")

    return parser

def main():
    vtr_command_main(sys.argv[1:])

def vtr_command_main(arg_list, prog=None):
    start = datetime.now()

    #Load the arguments
    args, unkown_args = vtr_command_argparser(prog).parse_known_args(arg_list)

    print_verbose(BASIC_VERBOSITY, args.verbosity, "# {} {}\n".format(prog, ' '.join(arg_list)))

    abs_path_arch_file = os.path.abspath(args.architecture_file)
    abs_path_circuit_file = os.path.abspath(args.circuit_file)
    abs_work_dir = os.path.abspath(args.work_dir)


    #Specify how command should be run
    command_runner = CommandRunner(track_memory=args.track_memory_usage, 
                                   max_memory_mb=args.memory_limit, 
                                   timeout_sec=args.timeout,
                                   verbose=True if args.verbosity >= 2 else False,
                                   echo_cmd=True if args.verbosity >= 4 else False)
    exit_status = 0
    try:
        if not args.parse_only:
            try:
                vpr_args = process_unkown_args(unkown_args)

                #Run the flow
                run_vtr_flow(abs_path_arch_file, 
                             abs_path_circuit_file, 
                             power_tech_file=args.power_tech,
                             work_dir=abs_work_dir,
                             start_stage=args.start, 
                             end_stage=args.end,
                             command_runner=command_runner,
                             verbosity=args.verbosity,
                             vpr_args=vpr_args
                             )
            except CommandError as e:
                #An external command failed
                print "Error: {msg}".format(msg=e.msg)
                print "\tfull command: ", ' '.join(e.cmd)
                print "\treturncode  : ", e.returncode
                print "\tlog file    : ", e.log
                exit_status = 1
            except InspectError as e:
                #Something went wrong gathering information
                print "Error: {msg}".format(msg=e.msg)
                print "\tfile        : ", e.filename
                exit_status = 2

            except VtrError as e:
                #Generic VTR errors
                print "Error: ", e.msg
                exit_status = 3

            except KeyboardInterrupt as e:
                print "{} recieved keyboard interrupt".format(prog)
                exit_status = 4

        #Parse the flow results
        parse_vtr_flow(abs_work_dir, args.parse_config_file, verbosity=args.verbosity)
    finally:
        print_verbose(BASIC_VERBOSITY, args.verbosity, "\n{} took {}".format(prog, format_elapsed_time(datetime.now() - start)))
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
            raise VtrError("Extra argument '{}' intended for VPR does not start with '-'".format(arg))

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
