#!/usr/bin/env python2
import os
import sys
import argparse
import itertools
import textwrap
import subprocess
import time
from datetime import datetime
from multiprocessing import Pool

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'python_libs'))

import verilogtorouting as vtr
from verilogtorouting.error import *
from verilogtorouting.util import load_list_file, find_vtr_file, mkdir_p, print_verbose, find_vtr_root, CommandRunner, format_elapsed_time, RawDefaultHelpFormatter, VERBOSITY_CHOICES
from verilogtorouting.task import load_task_config, TaskConfig, find_task_config_file
from verilogtorouting.flow import CommandRunner

BASIC_VERBOSITY=1

def vtr_command_argparser(prog=None):
    description = textwrap.dedent(
                    """
                    Runs one or more VTR regression tests.
                    """
                  )
    epilog = textwrap.dedent(
                """
                Examples
                --------

                    Run the regression test 'vtr_strong':

                        %(prog)s vtr_strong

                    Run the regression tests 'vtr_basic' and 'vtr_strong':

                        %(prog)s vtr_basic vtr_strong

                    Run the regression tests 'vtr_basic' and 'vtr_strong' with 8 parallel workers:

                        %(prog)s vtr_basic vtr_strong -j8
                """
             )

    parser = argparse.ArgumentParser(
                prog=prog,
                description=description,
                epilog=epilog,
                formatter_class=RawDefaultHelpFormatter,
             )

    #
    # Major arguments
    #
    parser.add_argument('reg_test',
                        nargs="+",
                        choices=["vtr_basic", "vtr_strong", "vtr_nightly", "vtr_weekly", "odin_micro", "odin_full"],
                        help="Regression tests to be run")

    parser.add_argument("--create_golden",
                        default=False,
                        action="store_true",
                        help="Create golden reference results for the associated tasks")

    parser.add_argument('-j',
                        default=1,
                        type=int,
                        metavar="NUM_PROC",
                        help="How many processors to use for parallel execution.")

    parser.add_argument("-v", "--verbosity",
                        choices=VERBOSITY_CHOICES,
                        default=2,
                        type=int,
                        help="Sets the verbosity of the script. Higher values produce more output.")

    parser.add_argument("--work_dir",
                        default=None,
                        help="Directory to store intermediate and result files."
                             "If None, set to the relevante directory under $VTR_ROOT/vtr_flow/tasks.")
    return parser

def main():
    vtr_command_main(sys.argv[1:])

def vtr_command_main(arg_list, prog=None):
    start = datetime.now()

    #Load the arguments
    args = vtr_command_argparser(prog).parse_args(arg_list)

    print_verbose(BASIC_VERBOSITY, args.verbosity, "# {} {}\n".format(prog, ' '.join(arg_list)))

    num_func_failures = 0
    num_qor_failures = 0

    try:
        #Run any ODIN tests
        for reg_test in args.reg_test:
            if reg_test.startswith("odin"):
                num_func_failures += run_odin_test(args, reg_test)

        #Collect the task lists
        vtr_task_list_files = []
        for reg_test in args.reg_test:
            if reg_test.startswith("vtr"):

                base_testname = reg_test.split('_')[-1]
                task_list_filepath = os.path.join(find_vtr_root(), 
                                                  'vtr_flow', 'tasks', 'regression_tests', 
                                                  'vtr_reg_' + base_testname, 'task_list.txt')
                vtr_task_list_files.append(task_list_filepath)

        #Run the actual tasks, recording functionality failures
        num_func_failures += run_tasks(args, vtr_task_list_files)

        if args.create_golden:
            #Create golden results
            num_qor_failures = 0
            create_golden_tasks(args, vtr_task_list_files)
        else:
            #Check against golden results
            num_qor_failures += check_qor_tasks(args, vtr_task_list_files)


        #Final summary
        print_verbose(BASIC_VERBOSITY, args.verbosity, "")
        if num_func_failures == 0 and num_qor_failures == 0:
            print_verbose(BASIC_VERBOSITY, args.verbosity, 
                         "PASSED All Test(s)")
        elif num_func_failures != 0 or num_qor_failures != 0:
            print_verbose(BASIC_VERBOSITY, args.verbosity, 
                         "FAILED {} functionality and {} QoR tests".format(num_func_failures, num_qor_failures))

        sys.exit(num_func_failures + num_qor_failures)
    finally:
        print_verbose(BASIC_VERBOSITY, args.verbosity, "\n{} took {}".format(prog, format_elapsed_time(datetime.now() - start)))

def run_odin_test(args, test_name):
    odin_reg_script = None
    if test_name == "odin_micro":
        odin_reg_script = find_vtr_file("verify_microbenchmarks.sh", is_executabe=True)
    else:
        assert test_name == "odin_full"
        odin_reg_script = find_vtr_file("verify_regression_tests.sh", is_executabe=True)

    assert odin_reg_script

    odin_root = os.path.dirname(odin_reg_script)

    result = subprocess.call(odin_reg_script, cwd=odin_root)

    assert result != None
    if result != 0:
        #Error
        print_verbose(BASIC_VERBOSITY, args.verbosity, "FAILED test '{}'".format(test_name))
        return 1

    #Pass
    print_verbose(BASIC_VERBOSITY, args.verbosity, "PASSED test '{}'".format(test_name))
    return 0

def run_tasks(args, task_lists):
    #Call 'vtr task'
    vtr_task_cmd = ['vtr', 'task'] 
    vtr_task_cmd += ['-l'] + task_lists
    vtr_task_cmd += ['-j', str(args.j),
                     '-v', str(max(0, args.verbosity - 1)),
                     '--work_dir', args.work_dir,
                     #'--print_metadata', 'True']
                     '--print_metadata', 'False']

    #Exit code is number of failures
    return subprocess.call(vtr_task_cmd)

def check_qor_tasks(args, task_lists):
    num_qor_failures = 0
    for task_list in task_lists:
        with open(task_list) as f:
            for task in f:
                num_qor_failures += check_qor_task(args, task)

    return num_qor_failures

def check_qor_task(args, task):
    return -1

def create_golden_tasks(args, task_list):
    for task_list in task_lists:
        with open(task_list) as f:
            for task in f:
                create_task_golden_results(args, task)

def create_golden_task(args, task):
    pass

if __name__ == "__main__":
    main()
