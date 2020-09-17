#!/usr/bin/env python3

""" This module is a wrapper around the scripts/python_libs/vtr,
allowing the user to run one or more VTR tasks. """


from pathlib import Path
from pathlib import PurePath
import sys
import argparse
import textwrap
import subprocess
from datetime import datetime
from contextlib import redirect_stdout
from multiprocessing import Pool, Manager
from difflib import SequenceMatcher

from run_vtr_flow import vtr_command_main as run_vtr_flow

# pylint: disable=wrong-import-position, import-error
sys.path.insert(0, str(Path(__file__).resolve().parent / "python_libs"))

from vtr import (
    load_list_file,
    format_elapsed_time,
    RawDefaultHelpFormatter,
    argparse_str2bool,
    get_next_run_dir,
    load_task_config,
    find_task_config_file,
    load_parse_results,
    parse_tasks,
    find_task_dir,
    shorten_task_names,
    find_longest_task_description,
    check_golden_results_for_tasks,
    create_golden_results_for_tasks,
    create_jobs,
    calc_geomean,
    summarize_qor,
    paths,
)
from vtr.error import VtrError, InspectError, CommandError

# pylint: enable=wrong-import-position, import-error


def vtr_command_argparser(prog=None):
    """ Argument parse for run_vtr_task """

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
        prog=prog, description=description, epilog=epilog, formatter_class=RawDefaultHelpFormatter,
    )

    #
    # Major arguments
    #
    parser.add_argument("task", nargs="*", help="Tasks to be run")

    parser.add_argument(
        "-l",
        nargs="*",
        default=[],
        metavar="TASK_LIST_FILE",
        dest="list_file",
        help="A file listing tasks to be run",
    )

    parser.add_argument(
        "-parse",
        default=False,
        action="store_true",
        help="Perform only parsing on the latest task run",
    )

    parser.add_argument(
        "-create_golden",
        default=False,
        action="store_true",
        help="Update or create golden results for the specified task",
    )

    parser.add_argument(
        "-check_golden",
        default=False,
        action="store_true",
        help="Check the latest task run against golden results",
    )

    parser.add_argument(
        "-system",
        choices=["local", "scripts"],
        default="local",
        help="What system to run the tasks on.",
    )

    parser.add_argument(
        "-script",
        default="run_vtr_flow.py",
        help="Determines what flow script is used for the tasks",
    )

    parser.add_argument(
        "-short_task_names", default=False, action="store_true", help="Output shorter task names.",
    )

    parser.add_argument(
        "-show_failures",
        default=False,
        action="store_true",
        help="Produce additional debug output",
    )

    parser.add_argument(
        "-j",
        "-p",
        default=1,
        type=int,
        metavar="NUM_PROC",
        help="How many processors to use for execution.",
    )

    parser.add_argument(
        "-timeout",
        default=30 * 24 * 60 * 60,  # 30 days
        metavar="TIMEOUT_SECONDS",
        help="Time limit for this script.",
    )

    parser.add_argument(
        "-verbosity",
        default=0,
        type=int,
        help="Sets the verbosity of the script. Higher values produce more output.",
    )

    parser.add_argument(
        "-minw_hint_factor",
        default=1,
        type=float,
        help="Minimum width hint factor to multiplied by the minimum width hint",
    )

    parser.add_argument("-revision", default="", help="Revision number")

    parser.add_argument(
        "-calc_geomean",
        default=False,
        action="store_true",
        help="QoR geomeans are not computed by default",
    )

    parser.add_argument(
        "-print_metadata",
        default=True,
        type=argparse_str2bool,
        help="Print meta-data like command-line arguments and run-time",
    )

    parser.add_argument(
        "-s",
        nargs=argparse.REMAINDER,
        default=[],
        dest="shared_script_params",
        help="Treat the remainder of the command line options as script parameters "
        "shared by all tasks",
    )

    return parser


def vtr_command_main(arg_list, prog=None):
    """ Run the vtr tasks given and the tasks in the lists given """
    # Load the arguments
    args = vtr_command_argparser(prog).parse_args(arg_list)

    # Don't run if parsing or handling golden results
    args.run = not (args.parse or args.create_golden or args.check_golden or args.calc_geomean)

    # Always parse if running
    if args.run:
        args.parse = True

    num_failed = -1
    try:
        task_names = args.task

        for list_file in args.list_file:
            task_names += load_list_file(list_file)

        config_files = [find_task_config_file(task_name) for task_name in task_names]
        configs = []
        longest_name = 0  # longest task name for use in creating prettier output
        common_task_prefix = None  # common task prefix to shorten task names
        for config_file in config_files:
            config = load_task_config(config_file)
            configs += [config]
            if common_task_prefix is None:
                common_task_prefix = config.task_name
            else:
                match = SequenceMatcher(
                    None, common_task_prefix, config.task_name
                ).find_longest_match(0, len(common_task_prefix), 0, len(config.task_name))
                common_task_prefix = common_task_prefix[match.a : match.a + match.size]
            if len(config.task_name) > longest_name:
                longest_name = len(config.task_name)
        if args.short_task_names:
            configs = shorten_task_names(configs, common_task_prefix)
        longest_arch_circuit = find_longest_task_description(
            configs
        )  # find longest task description for use in creating prettier output
        num_failed = run_tasks(args, configs, longest_name, longest_arch_circuit)

    except CommandError as exception:
        print("Error: {msg}".format(msg=exception.msg))
        print("\tfull command: ", exception.cmd)
        print("\treturncode  : ", exception.returncode)
        print("\tlog file    : ", exception.log)
    except InspectError as exception:
        print("Error: {msg}".format(msg=exception.msg))
        if exception.filename:
            print("\tfile: ", exception.filename)
    except VtrError as exception:
        print("Error:", exception.msg)
    if __name__ == "main":
        sys.exit(num_failed)
    return num_failed


def run_tasks(
    args, configs, longest_name, longest_arch_circuit,
):
    """
    Runs the specified set of tasks (configs)
    """
    start = datetime.now()
    num_failed = 0

    jobs = create_jobs(args, configs, longest_name, longest_arch_circuit)

    run_dirs = {}
    for config in configs:
        task_dir = find_task_dir(config)
        task_run_dir = get_next_run_dir(task_dir)
        run_dirs[config.task_name] = task_run_dir

    # We could potentially support other 'run' systems (e.g. a cluster),
    # rather than just the local machine
    if args.system == "local":
        assert args.j > 0, "Invalid number of processors"

        if args.run:
            num_failed = run_parallel(args, jobs, run_dirs)
            print("Elapsed time: {}".format(format_elapsed_time(datetime.now() - start)))

        if args.parse:
            print("\nParsing test results...")
            print("scripts/parse_vtr_task.py -l {}".format(args.list_file[0]))
            parse_tasks(configs, jobs)

        if args.create_golden:
            create_golden_results_for_tasks(configs)

        if args.check_golden:
            num_failed += check_golden_results_for_tasks(configs)

        if args.calc_geomean:
            summarize_qor(configs)
            calc_geomean(args, configs)
    elif args.system == "scripts":
        for _, value in run_dirs.items():
            Path(value).mkdir(parents=True)
        run_scripts = create_run_scripts(args, jobs, run_dirs)
        for script in run_scripts:
            print(script)
    else:
        raise VtrError("Unrecognized run system {system}".format(system=args.system))
    return num_failed


def run_parallel(args, queued_jobs, run_dirs):
    """
    Run each external command in commands with at most args.j commands running in parllel
    """
    # Determine the run dir for each config

    # We pop off the jobs of queued_jobs, which python does from the end,
    # so reverse the list now so we get the expected order. This also ensures
    # we are working with a copy of the jobs
    queued_jobs = list(reversed(queued_jobs))
    # Find the max taskname length for pretty printing

    queued_procs = []
    queue = Manager().Queue()
    for job in queued_jobs:
        queued_procs += [(queue, run_dirs, job, args.script)]
    # Queue of currently running subprocesses

    num_failed = 0
    with Pool(processes=args.j) as pool:
        for proc in queued_procs:
            pool.apply_async(run_vtr_flow_process, proc)
        pool.close()
        pool.join()
    for _ in queued_procs:
        num_failed += queue.get()

    return num_failed


def create_run_scripts(args, jobs, run_dirs):
    """ Create the bash script files for each job run """
    run_script_files = []
    for job in jobs:
        run_script_files += [create_run_script(args, job, job.work_dir(run_dirs[job.task_name()]))]
    return run_script_files


def create_run_script(args, job, work_dir):
    """ Create the bash run script for a particular job """

    runtime_estimate = ret_expected_runtime(job, work_dir)
    memory_estimate = ret_expected_memory(job, work_dir)
    if runtime_estimate < 0:
        runtime_estimate = 0

    if memory_estimate < 0:
        memory_estimate = 0

    human_readable_runtime_est = format_human_readable_time(runtime_estimate)
    human_readable_memory_est = format_human_readable_memory(memory_estimate)
    Path(work_dir).mkdir(parents=True)
    run_script_file = Path(work_dir) / "vtr_flow.sh"
    template = str(paths.flow_template_path)
    with open(template, "r") as in_file:
        template_string = in_file.readlines()
        template_string = "".join(template_string)
        with open(run_script_file, "w+") as out_file:
            print(
                template_string.format(
                    estimated_time=runtime_estimate,
                    estimated_memory=memory_estimate,
                    human_readable_time=human_readable_runtime_est,
                    human_readable_memory=human_readable_memory_est,
                    script=args.script,
                    command=job.run_command(),
                ),
                file=out_file,
                end="",
            )
    return str(run_script_file)


def ret_expected_runtime(job, work_dir):
    """ Returns the expected run-time (in seconds) of the specified run, or -1 if unkown """
    seconds = -1
    golden_results = load_parse_results(
        str(Path(work_dir).parent.parent.parent.parent / "config/golden_results.txt")
    )
    metrics = golden_results.metrics(job.arch(), job.circuit(), job.script_params())
    if "vtr_flow_elapsed_time" in metrics:
        seconds = float(metrics["vtr_flow_elapsed_time"])
    return seconds


def ret_expected_memory(job, work_dir):
    """ Returns the expected memory usage (in bytes) of the specified run, or -1 if unkown """
    memory_kib = -1
    golden_results = load_parse_results(
        str(Path(work_dir).parent.parent.parent.parent / "config/golden_results.txt")
    )
    metrics = golden_results.metrics(job.arch(), job.circuit(), job.script_params())
    for metric in ["max_odin_mem", "max_abc_mem", "max_ace_mem", "max_vpr_mem"]:
        if metric in metrics and int(metrics[metric]) > memory_kib:
            memory_kib = int(metrics[metric])
    return memory_kib


def format_human_readable_time(seconds):
    """ format the number of seconds given as a human readable value """
    if seconds < 60:
        return "%.0f seconds" % seconds
    if seconds < 60 * 60:
        minutes = seconds / 60
        return "%.0f minutes" % minutes
    hour = seconds / 60 / 60
    return "%.0f hours" % hour


def format_human_readable_memory(num_bytes):
    """ format the number of bytes given as a human readable value """
    if num_bytes < 1024 ** 3:
        return "%.2f MiB" % (num_bytes / (1024 ** 2))
    return "%.2f GiB" % (num_bytes / (1024 ** 3))


def run_vtr_flow_process(queue, run_dirs, job, script):
    """
    This is the function that the multiprocessing calls.
    It runs the vtr flow and allerts the multiprocessor through a queue if the flow failed.
    """
    work_dir = job.work_dir(run_dirs[job.task_name()])
    Path(work_dir).mkdir(parents=True, exist_ok=True)
    out = None
    vtr_flow_out = str(PurePath(work_dir) / "vtr_flow.out")
    with open(vtr_flow_out, "w+") as out_file:
        with redirect_stdout(out_file):
            if script == "run_vtr_flow.py":
                out = run_vtr_flow(job.run_command(), str(paths.run_vtr_flow_path))
            else:
                out = subprocess.call(
                    [str(paths.scripts_path / script)] + job.run_command(),
                    cwd=str(paths.root_path),
                    stdout=out_file,
                )

    with open(vtr_flow_out, "r") as out_file:
        for line in out_file.readlines():
            print(line, end="")

    # report flow failure to queue
    if out:
        queue.put(1)
    else:
        queue.put(0)


if __name__ == "__main__":
    vtr_command_main(sys.argv[1:])
