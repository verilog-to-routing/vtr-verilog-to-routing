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
from verilogtorouting.util import load_list_file, find_vtr_file, mkdir_p, print_verbose, find_vtr_root, CommandRunner, format_elapsed_time, RawDefaultHelpFormatter, VERBOSITY_CHOICES, argparse_str2bool
from verilogtorouting.task import load_task_config, TaskConfig, find_task_config_file
from verilogtorouting.flow import CommandRunner

BASIC_VERBOSITY = 1
FAILED_LOG_VERBOSITY = 2
ALL_LOG_VERBOSITY = 4

class Job:

    def __init__(self, task_name, job_name, command, work_dir):
        self._task_name = task_name
        self._job_name = job_name
        self._command = command
        self._work_dir = work_dir

    def task_name(self):
        return self._task_name

    def job_name(self):
        return self._job_name

    def command(self):
        return self._command

    def work_dir(self):
        return self._work_dir

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

                Exit Code
                ---------
                    The exit code equals the number failures (i.e. exit code 0 indicates no failures).
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
                        nargs="*",
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
                        default=2,
                        type=int,
                        help="Sets the verbosity of the script. Higher values produce more output.")

    parser.add_argument("--print_metadata",
                        default=True,
                        type=argparse_str2bool,
                        help="Print meta-data like command-line arguments and run-time")

    return parser

def main():
    vtr_command_main(sys.argv[1:])

def vtr_command_main(arg_list, prog=None):
    start = datetime.now()

    #Load the arguments
    args = vtr_command_argparser(prog).parse_args(arg_list)

    if args.print_metadata:
        print "# {} {}\n".format(prog, ' '.join(arg_list))

    try:
        task_names = args.task

        for list_file in args.list_file:
            task_names += load_list_file(list_file)

        config_files = [find_task_config_file(task_name) for task_name in task_names]

        configs = [load_task_config(config_file) for config_file in config_files]

        num_failed = run_tasks(args, configs)

    except CommandError as e:
        print "Error: {msg}".format(msg=e.msg)
        print "\tfull command: ", e.cmd
        print "\treturncode  : ", e.returncode
        print "\tlog file    : ", e.log
        sys.exit(-1)
    except VtrError as e:
        print "Error:", e.msg
        sys.exit(-1)
    finally:
        if args.print_metadata:
            print "\n{} took {}".format(prog, format_elapsed_time(datetime.now() - start))

    sys.exit(num_failed)

def run_tasks(args, configs):

    if args.system == "local":
        assert args.j > 0, "Invalid number of processors"

        #Generate the arguments for each script invocation
        all_worker_args = build_worker_args(args, configs)

        jobs = []
        for worker_args in all_worker_args:
            cmd_args = [worker_args['arch'],
                        worker_args['circuit'],
                        "-v", str(max(0, args.verbosity - 1)) #Less verbose
                        ]
            cmd = worker_args['exec'] + cmd_args
            job = Job(worker_args['task_name'], worker_args['job_name'], cmd, worker_args['work_dir'])
            jobs.append(job)

        return run_parallel(args, jobs)
    else:
        raise VtrError("Unrecognized run system {system}".format(system=args.system))

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
            script_params += config.script_params

            if config.cmos_tech_behavior:
                script_params += ["--power_tech", resolve_vtr_source_file(config, config.cmos_tech_behavior, "tech")]

            if config.pad_file:
                script_params += ["--fix_pins", resolve_vtr_source_file(config, config.pad_file)]

            work_dir = os.path.join(config.task_name, os.path.basename(arch), os.path.basename(circuit))

            job_name = os.path.join(arch, circuit)

            worker_info = {
                            'task_name': config.task_name,
                            'job_name': job_name, 
                            'exec': executable, 
                            'arch': resolve_vtr_source_file(config, arch, config.arch_dir),
                            'circuit': resolve_vtr_source_file(config, circuit, config.circuit_dir), 
                            'script_args': script_params, 
                            'work_dir': work_dir, 
                            'cmd_runner': cmd_runner,
                            'verbosity': args.verbosity
                            } 

            all_worker_args.append(worker_info)

    return all_worker_args

def run_parallel(args, queued_jobs):
    """
    Run each external command in commands with at most j commands running in parllel
    """

    #We pop off the jobs of queued_jobs, which python does from the end,
    #so reverse the list now so we get the expected order
    queued_jobs = list(reversed(queued_jobs))

    #Find the max taskname length for pretty printing
    max_taskname_len = 0
    for job in queued_jobs:
        max_taskname_len = max(max_taskname_len, len(job.task_name()))

    #Queue of currently running subprocesses
    running_procs = []

    num_failed = 0
    try:
        while len(queued_jobs) > 0 or len(running_procs) > 0: #Outstanding or running jobs

            #Launch outstanding jobs if workers are available
            while len(queued_jobs) > 0 and len(running_procs) < args.j:
                job = queued_jobs.pop()

                #Make the working directory
                work_dir = job.work_dir()
                mkdir_p(work_dir)

                log_filepath = os.path.join(work_dir, "vtr_flow.log")

                log_file = open(log_filepath, 'w+')

                #print "Starting {}: {}".format(job.task_name(), job.job_name())

                proc = subprocess.Popen(job.command(), 
                                        cwd=work_dir, 
                                        stderr=subprocess.STDOUT, 
                                        stdout=log_file)

                running_procs.append((proc, job, log_file))


            while len(running_procs) > 0:
                #Are any of the workers finished?
                for i in xrange(len(running_procs)):
                    proc, job, log_file = running_procs[i]

                    status = proc.poll()
                    if status is not None:
                        #Process has completed
                        if status == 0:
                            print_verbose(BASIC_VERBOSITY, args.verbosity, 
                                         "Finished {:<{w}}: {}".format(job.task_name(), job.job_name(), w=max_taskname_len))

                            if args.verbosity >= ALL_LOG_VERBOSITY:
                                print_log(log_file)
                        else:
                            #Failed
                            num_failed += 1
                            print_verbose(BASIC_VERBOSITY, args.verbosity, 
                                         "Failed   {:<{w}}: {}".format(job.task_name(), job.job_name(), w=max_taskname_len))

                            if args.verbosity >= FAILED_LOG_VERBOSITY:
                                print_log(log_file)

                        log_file.close()

                        #Remove the jobs from the run queue
                        del running_procs[i]
                        break

                if len(running_procs) < args.j:
                    #There are idle workers, allow new jobs to start
                    break;

                #Don't constantly poll
                time.sleep(0.1)

    except KeyboardInterrupt as e:
        print "Recieved KeyboardInterrupt -- halting workers"
        for proc, job, log_file in running_procs:
            proc.terminate()
            log_file.close()

        #Remove any procs finished after terminate
        running_procs = [(proc, job, log_file) for proc, job, log_file in running_procs if proc.returncode is None]

    finally:
        if len(running_procs) > 0:
            print "Killing {} worker processes".format(len(running_procs))
            for proc, job, log_file in running_procs:
                proc.kill()
                log_file.close()

    return num_failed

def print_log(log_file, indent="    "):
    #Save positino
    curr_pos = log_file.tell()

    log_file.seek(0) #Rewind to start

    #Print log
    for line in log_file:
        line = line.rstrip()
        print indent + line
    print ""

    #Return to original position
    log_file.seek(curr_pos)


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
