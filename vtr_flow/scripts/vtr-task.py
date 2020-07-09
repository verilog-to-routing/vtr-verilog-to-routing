#!/usr/bin/env python3
from pathlib import Path
from pathlib import PurePath
import sys
import argparse
import itertools
import textwrap
import subprocess
import time
import shutil
from datetime import datetime
from multiprocessing import Pool

sys.path.insert(0, str(Path(__file__).resolve().parent / 'python_libs'))

from vtr import load_list_file, find_vtr_file, mkdir_p, print_verbose, find_vtr_root, CommandRunner, format_elapsed_time, RawDefaultHelpFormatter, VERBOSITY_CHOICES, argparse_str2bool, get_next_run_dir, get_latest_run_dir, load_task_config, TaskConfig, find_task_config_file, CommandRunner, load_pass_requirements, load_parse_results
from vtr.error import VtrError, InspectError, CommandError

BASIC_VERBOSITY = 1
FAILED_LOG_VERBOSITY = 2
ALL_LOG_VERBOSITY = 4

class Job:

    def __init__(self, task_name, arch, circuit, work_dir, command):
        self._task_name = task_name
        self._arch = arch
        self._circuit = circuit
        self._command = command
        self._work_dir = work_dir

    def task_name(self):
        return self._task_name

    def arch(self):
        return self._arch

    def circuit(self):
        return self._circuit

    def job_name(self):
        return str(PurePath(self.arch()).joinpath(self.circuit()))

    def command(self):
        return self._command

    def work_dir(self, run_dir):
        return str(PurePath(run_dir).joinpath(self._work_dir))

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
                        default=[],
                        metavar="TASK_LIST_FILE",
                        help="A file listing tasks to be run")

    parser.add_argument("--parse",
                        default=False,
                        action="store_true",
                        dest="parse",
                        help="Perform only parsing on the latest task run")

    parser.add_argument("--create_golden",
                        default=False,
                        action="store_true",
                        dest="create_golden",
                        help="Update or create golden results for the specified task")

    parser.add_argument("--check_golden",
                        default=False,
                        action="store_true",
                        dest="check_golden",
                        help="Check the latest task run against golden results")

    parser.add_argument('--system',
                        choices=['local'],
                        default='local',
                        help="What system to run the tasks on.")

    parser.add_argument('-j',
                        default=1,
                        type=int,
                        metavar="NUM_PROC",
                        help="How many processors to use for execution.")

    parser.add_argument('--timeout',
                        default=30*24*60*60, #30 days
                        metavar="TIMEOUT_SECONDS",
                        help="Time limit for this script.")

    parser.add_argument("-v", "--verbosity",
                        choices=VERBOSITY_CHOICES,
                        default=2,
                        type=int,
                        help="Sets the verbosity of the script. Higher values produce more output.")

    parser.add_argument("--work_dir",
                        default=None,
                        help="Directory to store intermediate and result files."
                             "If None, set to the relevant directory under $VTR_ROOT/vtr_flow/tasks.")

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

    args.run = True
    if args.parse or args.create_golden or args.check_golden:
        #Don't run if parsing or handling golden results
        args.run = False

    if args.run:
        #Always parse if running
        args.parse = True

    if args.print_metadata:
        print ("# {} {}\n".format(prog, ' '.join(arg_list)))

    num_failed = -1
    try:
        task_names = args.task

        for list_file in args.list_file:
            task_names += load_list_file(list_file)

        config_files = [find_task_config_file(task_name) for task_name in task_names]

        configs = [load_task_config(config_file) for config_file in config_files]

        num_failed = run_tasks(args, configs)

    except CommandError as e:
        print ("Error: {msg}".format(msg=e.msg))
        print ("\tfull command: ", e.cmd)
        print ("\treturncode  : ", e.returncode)
        print ("\tlog file    : ", e.log)
    except InspectError as e:
        print ("Error: {msg}".format(msg=e.msg))
        if e.filename:
            print ("\tfile: ", e.filename)
    except VtrError as e:
        print ("Error:", e.msg)
    finally:
        if args.print_metadata:
            print ("\n# {} took {} (exiting {})".format(prog, format_elapsed_time(datetime.now() - start), num_failed))
    sys.exit(num_failed)

def run_tasks(args, configs):
    """
    Runs the specified set of tasks (configs)
    """
    num_failed = 0

    #We could potentially support other 'run' systems (e.g. a cluster),
    #rather than just the local machine
    if args.system == "local":
        assert args.j > 0, "Invalid number of processors"

        #Generate the jobs, each corresponding to an invocation of vtr flow
        jobs = create_jobs(args, configs)

        if args.run:
            num_failed = run_parallel(args, configs, jobs)

        if args.parse:
            print_verbose(BASIC_VERBOSITY, args.verbosity, "")
            parse_tasks(args, configs, jobs)

        if args.create_golden:
            print_verbose(BASIC_VERBOSITY, args.verbosity, "")
            create_golden_results_for_tasks(args, configs)

        if args.check_golden:
            print_verbose(BASIC_VERBOSITY, args.verbosity, "")
            num_failed += check_golden_results_for_tasks(args, configs)

    else:
        raise VtrError("Unrecognized run system {system}".format(system=args.system))

    return num_failed

def parse_tasks(args, configs, jobs):
    """
    Parse the selection of tasks specified in configs and associated jobs
    """
    for config in configs:
        config_jobs = [job for job in jobs if job.task_name() == config.task_name]
        parse_task(args, config, config_jobs)

def parse_task(args, config, config_jobs, task_metrics_filepath=None, flow_metrics_basename="parse_results.txt"):
    """
    Parse a single task run.

    This generates a file parse_results.txt in the task's working directory,
    which is an amalgam of the parse_rests.txt's produced by each job (flow invocation)
    """
    run_dir = find_latest_run_dir(args, config)

    print_verbose(BASIC_VERBOSITY, args.verbosity, "Parsing task run {}".format(run_dir))

    for job in config_jobs:
        #Re-run parsing only
        cmd = job.command()
        cmd += ['--parse']
        cmd += ['-v', str(max(0, args.verbosity-3))]
        subprocess.check_call(cmd, cwd=job.work_dir(run_dir))

    if task_metrics_filepath is None:
        task_metrics_filepath = task_parse_results_filepath = str(PurePath(run_dir).joinpath("parse_results.txt"))

    #Record max widths for pretty printing
    max_arch_len = len("architecture")
    max_circuit_len = len("circuit")
    for job in config_jobs:
        max_arch_len = max(max_arch_len, len(job.arch()))
        max_circuit_len = max(max_circuit_len, len(job.circuit()))

    with open(task_parse_results_filepath, "w") as out_f:

        #Start the header
        print >>out_f, "{:<{arch_width}}\t{:<{circuit_width}}\t".format("architecture", "circuit", arch_width=max_arch_len, circuit_width=max_circuit_len),
        header = True

        for job in config_jobs:
            #Open the job results file
            #
            #The job results file is basically the same format, but excludes the architecture and circuit fields,
            #which we prefix to each line of the task result file
            job_parse_results_filepath = Path(run_dir) / job.arch / job.circuit / flow_metrics_basename
            if job_parse_results_filepath.exists:
                with job_parse_results_filepath.open() as in_f:
                    lines = in_f.readlines()

                    assert len(lines) == 2

                    if header:
                        #First line is the header
                        print >>out_f, lines[0],
                        header = False

                    #Second line is the data
                    print >>out_f, "{:<{arch_width}}\t{:<{circuit_width}}\t{}".format(job.arch(), job.circuit(), lines[1], arch_width=max_arch_len, circuit_width=max_circuit_len),
            else:
                print_verbose(BASIC_VERBOSITY, args.verbosity, "Warning: Flow result file not found (task QoR will be incomplete): {} ".format(str(job_parse_results_filepath)))

def create_golden_results_for_tasks(args, configs):
    for config in configs:
        create_golden_results_for_task(args, config)

def create_golden_results_for_task(args, config):
    """
    Copies the latest task run's parse_results.txt into the config directory as golden_results.txt
    """
    run_dir = find_latest_run_dir(args, config)

    task_results = str(PurePath(run_dir).joinpath("parse_results.txt"))
    golden_results_filepath = str(PurePath(config.config_dir).joinpath("golden_results.txt"))

    print_verbose(BASIC_VERBOSITY, args.verbosity, "Creating golden task results from {} -> {}".format(run_dir, golden_results_filepath))

    shutil.copy(task_results, golden_results_filepath)

def check_golden_results_for_tasks(args, configs):
    num_qor_failures = 0

    print_verbose(BASIC_VERBOSITY, args.verbosity, "Checking QoR:")
    for config in configs:
        num_qor_failures += check_golden_results_for_task(args, config)

    return num_qor_failures

def check_golden_results_for_task(args, config):
    """
    Copies the latest task run's parse_results.txt into the config directory as golden_results.txt
    """
    num_qor_failures = 0
    run_dir = find_latest_run_dir(args, config)

    if not config.pass_requirements_file:
        print_verbose(BASIC_VERBOSITY, args.verbosity, 
                      "Warning: no pass requirements file for task {}, QoR will not be checked".format(config.task_name))
    else:

        #Load the pass requirements file
        pass_req_filepath = str(PurePath(find_vtr_root).joinpath('vtr_flow').joinpath('parse').joinpath('pass_requirements').joinpath(config.pass_requirements_file))
        pass_requirements = load_pass_requirements(pass_req_filepath)

        #Load the task's parse results
        task_results_filepath = str(PurePath(run_dir).joinpath("parse_results.txt"))
        task_results = load_parse_results(task_results_filepath)
         
        #Load the golden reference
        golden_results_filepath = str(PurePath(config.config_dir).joinpath("golden_results.txt"))
        golden_results = load_parse_results(golden_results_filepath)

        #Verify that the architecture and circuit are specified
        for param in ["architecture", "circuit"]:
            if param not in task_results.primary_keys():
                raise InspectError("Required param '{}' missing from task results: {}".format(param, task_results_filepath), task_results_filepath)

            if param not in golden_results.primary_keys():
                raise InspectError("Required param '{}' missing from golden results: {}".format(param, golden_results_filepath), golden_results_filepath)

        #Verify that all params and pass requirement metric are included in both the golden and task results
        # We do not worry about non-pass_requriements elements being different or missing
        for metric in pass_requirements.keys():
            for (arch, circuit), result in task_results.all_metrics().iteritems():
                if metric not in result:
                    #print_verbose(BASIC_VERBOSITY, args.verbosity,
                        #"Warning: Required metric '{}' missing from task results".format(metric), task_results_filepath) 
                    raise InspectError("Required metric '{}' missing from task results".format(metric), task_results_filepath) 

            for (arch, circuit), result in golden_results.all_metrics().iteritems():
                if metric not in result:
                    #print_verbose(BASIC_VERBOSITY, args.verbosity,
                        #"Warning: Required metric '{}' missing from golden results".format(metric), golden_results_filepath) 
                    raise InspectError("Required metric '{}' missing from golden results".format(metric), golden_results_filepath) 

        #Load the primary keys for golden and task
        golden_primary_keys = []
        for (arch, circuit), metrics in golden_results.all_metrics().iteritems():
            golden_primary_keys.append((arch, circuit))

        task_primary_keys = []
        for (arch, circuit), metrics in task_results.all_metrics().iteritems():
            task_primary_keys.append((arch, circuit))

        #Ensure that task has all the golden cases
        for arch, circuit in golden_primary_keys:
            if task_results.metrics(arch, circuit) == None:
                raise InspectError("Required case {}/{} missing from task results: {}".format(arch, circuit), parse_results_filepath) 
    
        #Warn about any elements in task that are not found in golden
        for arch, circuit in task_primary_keys:
            if golden_results.metrics(arch, circuit) == None:
                print_verbose(BASIC_VERBOSITY, args.verbosity,
                             "Warning: Task includes result for {}/{} missing in golden results".format(arch, circuit))

        #Verify that task results pass each metric for all cases in golden
        for (arch, circuit) in golden_primary_keys:
            golden_metrics = golden_results.metrics(arch, circuit)
            task_metrics = task_results.metrics(arch, circuit)

            for metric in pass_requirements.keys():

                if not metric in golden_metrics:
                    print_verbose(BASIC_VERBOSITY, args.verbosity, "Warning: Metric {} missing from golden results".format(metric))
                    continue

                if not metric in task_metrics:
                    print_verbose(BASIC_VERBOSITY, args.verbosity, "Warning: Metric {} missing from task results".format(metric))
                    continue

                try:
                    metric_passed, reason = pass_requirements[metric].check_passed(golden_metrics[metric], task_metrics[metric])
                except InspectError as e:
                    metric_passed = False
                    reason = e.msg

                if not metric_passed:
                    print_verbose(BASIC_VERBOSITY, args.verbosity, "    FAILED {} {} {}/{}: {} {}".format(PurePath(run_dir).name, config.task_name, arch, circuit, metric, reason))
                    num_qor_failures += 1

    if num_qor_failures == 0:
        print_verbose(BASIC_VERBOSITY, args.verbosity, 
                      "    PASSED {} {}".format(PurePath(run_dir).name, config.task_name))

    return num_qor_failures

def create_jobs(args, configs):
    jobs = []
    for config in configs:
        for arch, circuit in itertools.product(config.archs, config.circuits):
            abs_arch_filepath = resolve_vtr_source_file(config, arch, config.arch_dir)
            abs_circuit_filepath = resolve_vtr_source_file(config, circuit, config.circuit_dir)

            executable = None
            if config.script_path:
                #Custom flow script
                executable = [config.script_path]
            else:
                #Default flow script
                executable = [find_vtr_file('run_vtr_flow.py', is_executable=True)]

            #Collect any extra script params from the config file
            script_params = [abs_arch_filepath, abs_circuit_filepath]
            script_params += config.script_params

            #Apply any special config based parameters
            if config.cmos_tech_behavior:
                script_params += ["--power", resolve_vtr_source_file(config, config.cmos_tech_behavior, "tech")]

            if config.pad_file:
                script_params += ["--fix_pins", resolve_vtr_source_file(config, config.pad_file)]

            if config.parse_file:
                script_params += ["--parse_config_file", resolve_vtr_source_file(config, config.parse_file, str(PurePath("parse").joinpath("parse_config")))]

            #We specify less verbosity to the sub-script
            # This keeps the amount of output reasonable
            script_params += ["-v", str(max(0, args.verbosity - 1))]

            cmd = executable + script_params

            work_dir = str(PurePath(arch).joinpath(circuit))

            jobs.append(Job(config.task_name, arch, circuit, work_dir, cmd))

    return jobs

def find_latest_run_dir(args, config):
    task_dir = find_task_dir(args, config)

    run_dir = get_latest_run_dir(task_dir)

    if not run_dir:
        raise InspectError("Failed to find run directory for task '{}' in '{}'".format(config.task_name, task_dir))

    assert Path(run_dir).is_dir()

    return run_dir

def find_task_dir(args, config):
    task_dir = None
    if args.work_dir:
        task_dir = str(PurePath(args.work_dir).joinpath(config.task_name))

    else:
        #Task dir is just above the config directory
        task_dir = Path(config.config_dir).parent
        assert task_dir.is_dir

    return str(task_dir)

def run_parallel(args, configs, queued_jobs):
    """
    Run each external command in commands with at most args.j commands running in parllel
    """
    #Determine the run dir for each config
    run_dirs = {}
    for config in configs:
        task_dir = find_task_dir(args, config)
        task_run_dir = get_next_run_dir(task_dir)
        run_dirs[config.task_name] = task_run_dir


    #We pop off the jobs of queued_jobs, which python does from the end,
    #so reverse the list now so we get the expected order. This also ensures
    #we are working with a copy of the jobs
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
                work_dir = job.work_dir(run_dirs[job.task_name()])
                mkdir_p(work_dir)

                log_filepath = str(PurePath(work_dir).joinpath("vtr_flow.log"))

                log_file = open(log_filepath, 'w+')

                #print "Starting {}: {}".format(job.task_name(), job.job_name())
                #print job.command()
                proc = subprocess.Popen(job.command(), 
                                        cwd=work_dir, 
                                        stderr=subprocess.STDOUT, 
                                        stdout=log_file)

                running_procs.append((proc, job, log_file))


            while len(running_procs) > 0:
                #Are any of the workers finished?
                procs_to_del = set()
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

                        #Record jobs to be removed from the run queue
                        procs_to_del.add(proc)

                if len(procs_to_del) > 0:
                    #Remove jobs from run queue
                    running_procs = [info for info in running_procs if info[0] not in procs_to_del]

                    assert len(running_procs) < args.j

                    #There are idle workers, allow new jobs to start
                    break;

                #Don't constantly poll
                time.sleep(0.1)

    except KeyboardInterrupt as e:
        print ("Recieved KeyboardInterrupt -- halting workers")
        for proc, job, log_file in running_procs:
            proc.terminate()
            log_file.close()

        #Remove any procs finished after terminate
        running_procs = [(proc, job, log_file) for proc, job, log_file in running_procs if proc.returncode is None]

    finally:
        if len(running_procs) > 0:
            print ("Killing {} worker processes".format(len(running_procs)))
            for proc, job, log_file in running_procs:
                proc.kill()
                log_file.close()

    return num_failed

def print_log(log_file, indent="    "):
    #Save position
    curr_pos = log_file.tell()

    log_file.seek(0) #Rewind to start

    #Print log
    for line in log_file:
        line = line.rstrip()
        print (indent + line)
    print ("")

    #Return to original position
    log_file.seek(curr_pos)


def resolve_vtr_source_file(config, filename, base_dir=""):
    """
    Resolves an filename with a base_dir

    Checks the following in order:
        1) filename as absolute path
        2) filename under config directory
        3) base_dir as absolute path (join filename with base_dir)
        4) filename and base_dir are relative paths (join under vtr_root)
    """
    
    #Absolute path
    if PurePath(filename).is_absolute():
        return filename

    #Under config
    config_path = Path(config.config_dir)
    assert config_path.is_absolute()
    joined_path = config_path / filename
    if joined_path.exists():
        return str(joined_path)

    #Under base dir
    base_path = Path(base_dir)
    if base_path.is_absolute():
        #Absolute base
        joined_path = base_path / filename
        if joined_path.exists():
            return str(joined_path)
    else:
        #Relative base under the VTR flow directory
        joined_path = Path(find_vtr_root()) / 'vtr_flow' / base_dir / filename
        if joined_path.exists():
            return str(joined_path)

    #Not found
    raise InspectError("Failed to resolve VTR source file {}".format(filename))

if __name__ == "__main__":
    main()
