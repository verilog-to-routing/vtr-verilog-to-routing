#!/usr/bin/env python2
import os
import sys
import argparse
import itertools
import textwrap
from datetime import datetime
from multiprocessing import Pool

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'python_libs'))

import verilogtorouting as vtr
from verilogtorouting.error import *
from verilogtorouting.util import load_list_file, find_vtr_file, mkdir_p, print_verbose, find_vtr_root, CommandRunner, format_elapsed_time, RawDefaultHelpFormatter, VERBOSITY_CHOICES
from verilogtorouting.task import load_task_config, TaskConfig, find_task_config_file
from verilogtorouting.flow import CommandRunner

def vtr_command_argparser(prog=None):
    description = textwrap.dedent(
                    """
                    Runs one or more VTR tasks.
                    """
                  )
    epilog = textwrap.dedent(
                """
                Examples
                --------

                    Run the task named 'timing_chain':

                        %(prog)s timing_chain

                    Run all the tasks listed in the file 'task_list.txt':

                        %(prog)s -l task_list.txt

                    Run 'timing_chain' with 4 jobs running in parallel:

                        %(prog)s timing_chain -j4
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
    parser.add_argument('task',
                        nargs="*",
                        help="Tasks to be run")

    parser.add_argument('-l', '--list_file',
                        metavar="TASK_LIST_FILE",
                        help="A file listing tasks to be run")

    parser.add_argument('--system',
                        choices=['local'],
                        default='local',
                        help="What system to run the tasks on.")

    parser.add_argument('-j',
                        default=1,
                        type=int,
                        metavar="NUM_PROC",
                        help="How many processors to use for parallel execution.")

    parser.add_argument('--timeout',
                        default=30*24*60*60, #30 days
                        metavar="TIMEOUT_SECONDS",
                        help="Time limit for this script.")

    parser.add_argument("-v", "--verbosity",
                        choices=VERBOSITY_CHOICES,
                        default=1,
                        type=int,
                        help="Sets the verbosity of the script. Higher values produce more output.")

    return parser

def main():
    vtr_command_main(sys.argv[1:])

def vtr_command_main(arg_list, prog=None):
    #import pudb
    #pudb.set_trace()

    #Load the arguments
    args = vtr_command_argparser(prog).parse_args(arg_list)

    try:
        task_names = args.task

        if args.list_file:
            task_names += load_list_file(args.list_file)

        config_files = [find_task_config_file(task_name) for task_name in task_names]

        configs = [load_task_config(config_file) for config_file in config_files]

        run_tasks(args, configs)
    except CommandError as e:
        print "Error: {msg}".format(msg=e.msg)
        print "\tfull command: ", e.cmd
        print "\treturncode  : ", e.returncode
        print "\tlog file    : ", e.log
        sys.exit(1)
    except VtrError as e:
        print "Error:", e.msg
        sys.exit(1)

    sys.exit(0)

def run_tasks(args, configs):
    start = datetime.now()

    if args.system == "local":
        assert args.j > 0, "Invalid number of processors"

        #Generate the arguments for each script invocation
        all_worker_args = build_worker_args(args, configs)

        if args.j == 1:

            #Run serially
            for worker_args in all_worker_args:
                local_worker(worker_args)

        else:
            #Run in parallel

            #Generate a pool of workers
            pool = Pool(processes=args.j)

            try:
                #map_async will call local_worker on every element of all_worker_args
                #the pool will handle the allocation of invocations to the worker processes
                #
                #We use map_asyc with a .get(timeout) to allow for clean keyboard interrupts
                # for discussion see: https://stackoverflow.com/questions/1408356
                res = pool.map_async(local_worker, all_worker_args)

                pool.close()

                res.get(args.timeout)
            except KeyboardInterrupt:
                print "Recieved keyboard interrupt, terminating workers..."
                pool.terminate()
    else:
        raise VtrError("Unrecognized run system {system}".format(system=args.system))

    end = datetime.now()
    elapsed = end - start
    print_verbose(1, args.verbosity, "Executing {} task(s) took {}".format(len(configs), format_elapsed_time(elapsed)))


def local_worker(worker_args):
    """
    Runs a single component of a task (i.e. a script invocation)
    based on the arguments provided in worker_args.
    """

    TIME_FMT = "%a %Y-%m-%d %H:%M:%S %z"
    ELAPSED_FMT = "%H:%M:%S"

    start = datetime.now()

    try:
        verbosity = worker_args['verbosity']

        #Pad out the task names so they alighn nicely
        operation = "{:<" + str(worker_args['task_name_length']) + "}"
        operation = operation.format(worker_args['task_name'])


        print_verbose(1, verbosity,  "Starting {operation} at {time}".format(operation=operation,
                                                                             time=start.strftime(TIME_FMT)))

        cmd_runner = worker_args['cmd_runner']
        work_dir = worker_args['work_dir']

        #Make the directory to run in
        mkdir_p(work_dir)

        #Build the command
        cmd = worker_args['exec'] + [worker_args['arch'], worker_args['circuit']]
        #if os.path.basename(cmd[0]) == "run_vtr_flow.py":
            ##Add default params to run_vtr_flow if they are not overriden
            #if '--work_dir' not in worker_args['script_args']:
                #cmd += ["--work_dir", "."]
            #if '--v' not in worker_args['script_args'] and '--verbosity' not in worker_args['script_args']:
                #cmd += ["-v", str(verbosity)]

        cmd += worker_args['script_args']


        #print_verbose(2, verbosity, "\t" + str(cmd))

        output, returncode = cmd_runner.run_system_command(cmd, work_dir=work_dir, indent_depth=1)
        assert returncode == 0
    except CommandError as e:
        end = datetime.now()
        elapsed = end - start
        print_verbose(1, verbosity, "Failed   {operation} at {time} (elapsed {length})".format(operation=operation,
                                                                                           time=end.strftime(TIME_FMT), 
                                                                                           length=format_elapsed_time(elapsed)))
    except KeyboardInterrupt as e:
        end = datetime.now()
        elapsed = end - start
        print_verbose(1, verbosity, "Killed   {operation} at {time} (elapsed {length})".format(operation=operation,
                                                                                           time=end.strftime(TIME_FMT), 
                                                                                           length=format_elapsed_time(elapsed)))
        #Since this is run as a subprocess we need to exit (rather than just return)
        sys.exit(1)

    else:
        end = datetime.now()
        elapsed = end - start
        print_verbose(1, verbosity, "Finished {operation} at {time} (elapsed {length})".format(operation=operation,
                                                                                           time=end.strftime(TIME_FMT), 
                                                                                           length=format_elapsed_time(elapsed)))

def build_worker_args(args, configs):
    all_worker_args = []

    cmd_runner = CommandRunner(track_memory=False,
                               verbose=True if args.verbosity > 1 else False,
                                )

    for config in configs:
        for arch, circuit in itertools.product(config.archs, config.circuits):
            executable = None
            if config.script_path:
                executable = [config.script_path]
            else:
                executable = [find_vtr_file('vtr', is_executabe=True), 'flow']

            script_params = []
            if config.script_params:
                script_params += config.script_params
            else:
                script_params += ["-v", str(min(0, args.verbosity - 1)), #Less verbose output
                                  "--work_dir", ".", #Run in the current directory
                                    ]

            if config.cmos_tech_behavior:
                script_params += ["--power_tech", resolve_vtr_source_file(config, config.cmos_tech_behavior, "tech")]

            if config.pad_file:
                script_params += ["--fix_pins", resolve_vtr_source_file(config, config.pad_file)]

            work_dir = os.path.join(config.task_name, os.path.basename(arch), os.path.basename(circuit))

            task_name = config.task_name + ": " + work_dir

            worker_info = {
                            'task_name': task_name, 
                            'exec': executable, 
                            'arch': resolve_vtr_source_file(config, arch, config.arch_dir),
                            'circuit': resolve_vtr_source_file(config, circuit, config.circuit_dir), 
                            'script_args': script_params, 
                            'work_dir': work_dir, 
                            'cmd_runner': cmd_runner,
                            'verbosity': args.verbosity
                            } 

            all_worker_args.append(worker_info)

    #Figure out the max task name length for nice output formatting
    max_len = None
    for worker_args in all_worker_args:
        length = len(worker_args['task_name'])
        if max_len == None or length > max_len:
            max_len = length

    for worker_args in all_worker_args:
        worker_args['task_name_length'] = max_len
    


    return all_worker_args

def resolve_vtr_source_file(config, filename, base_dir=None):
    """
    Resolves an filename with a base_dir

    Checks the following in order:
        1) filename as absolute path
        2) filename under config directory
        2) base_dir as absolute path (join filename with base_dir)
        3) filename and base_dir are relative paths (join under vtr_root)
    """
    
    #Absolute path
    if os.path.isabs(filename):
        return filename

    #Under config
    assert os.path.isabs(config.config_dir)
    joined_path = os.path.join(config.config_dir, filename)
    if os.path.exists(joined_path):
        return joined_path

    #Under base dir
    if os.path.isabs(base_dir):
        #Absolute base
        joined_path = os.path.join(base_dir, filename)
        if os.path.exists(joined_path):
            return joined_path
    else:
        #Relative base under the VTR flow directory
        joined_path = os.path.join(find_vtr_root(), 'vtr_flow', base_dir, filename)
        if os.path.exists(joined_path):
            return joined_path

    #Not found
    return None

if __name__ == "__main__":
    retval = main()
    sys.exit(retval)
