"""
Parse one or more task provided
"""

#!/usr/bin/env python3
from pathlib import Path
from pathlib import PurePath
import sys
import argparse
import textwrap
import shutil
from datetime import datetime
from contextlib import redirect_stdout

# pylint: disable=wrong-import-position
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from vtr import (
    load_list_file,
    RawDefaultHelpFormatter,
    get_latest_run_dir,
    load_task_config,
    find_task_config_file,
    load_pass_requirements,
    load_parse_results,
    parse_vtr_flow,
    get_latest_run_number,
    pretty_print_table,
    find_task_dir,
    CommandError,
    InspectError,
    VtrError,
    create_jobs,
    paths,
)

# pylint: enable=wrong-import-position

FIRST_PARSE_FILE = "parse_results.txt"
SECOND_PARSE_FILE = "parse_results_2.txt"
QOR_PARSE_FILE = "qor_results.txt"


def vtr_command_argparser(prog=None):
    """
    Argument parse for parse_vtr_flow.py
    Parses one or more VTR tasks.
    """

    description = textwrap.dedent(
        """
                    Parses one or more VTR tasks.
                    """
    )
    epilog = textwrap.dedent(
        """
                Examples
                --------

                    Parse the task named 'timing_chain':

                        %(prog)s timing_chain

                    Parse all the tasks listed in the file 'task_list.txt':

                        %(prog)s -l task_list.txt


                Exit Code
                ---------
                    The exit code equals the number failures
                    (i.e. exit code 0 indicates no failures).
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
        "-parse_qor",
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
        "-calc_geomean",
        default=False,
        action="store_true",
        help="QoR geomeans are not computed by default",
    )

    parser.add_argument("-run", default=None, type=str, help="")

    parser.add_argument("-revision", default="", help="Revision number")

    return parser


def vtr_command_main(arg_list, prog=None):
    """
    Main function for parse_vtr_task
    Parses in the results from run_vtr_task.py
    """
    # Load the arguments
    args = vtr_command_argparser(prog).parse_args(arg_list)
    try:
        task_names = args.task

        for list_file in args.list_file:
            task_names += load_list_file(list_file)

        config_files = [find_task_config_file(task_name) for task_name in task_names]

        configs = [load_task_config(config_file) for config_file in config_files]
        num_failed = 0

        jobs = create_jobs(args, configs, after_run=True)
        parse_tasks(configs, jobs)

        if args.create_golden:
            create_golden_results_for_tasks(configs)

        if args.check_golden:
            num_failed += check_golden_results_for_tasks(configs)

        if args.calc_geomean:
            summarize_qor(configs)
            calc_geomean(args, configs)

    except CommandError as error:
        print("Error: {msg}".format(msg=error.msg))
        print("\tfull command: ", error.cmd)
        print("\treturncode  : ", error.returncode)
        print("\tlog file    : ", error.log)
        num_failed += 1
    except InspectError as error:
        print("Error: {msg}".format(msg=error.msg))
        if error.filename:
            print("\tfile: ", error.filename)
        num_failed += 1
    except VtrError as error:
        print("Error:", error.msg)
        num_failed += 1

    return num_failed


def parse_tasks(configs, jobs):
    """
    Parse the selection of tasks specified in configs and associated jobs
    """
    for config in configs:
        config_jobs = [job for job in jobs if job.task_name() == config.task_name]
        parse_task(config, config_jobs)


def parse_task(config, config_jobs, flow_metrics_basename=FIRST_PARSE_FILE):
    """
    Parse a single task run.

    This generates a file parse_results.txt in the task's working directory,
    which is an amalgam of the parse_rests.txt's produced by each job (flow invocation)
    """
    run_dir = find_latest_run_dir(config)

    # Record max widths for pretty printing
    max_arch_len = len("architecture")
    max_circuit_len = len("circuit")
    for job in config_jobs:
        work_dir = job.work_dir(get_latest_run_dir(find_task_dir(config)))
        if job.parse_command():
            parse_filepath = str(PurePath(work_dir) / flow_metrics_basename)
            with open(parse_filepath, "w+") as parse_file:
                with redirect_stdout(parse_file):
                    parse_vtr_flow(job.parse_command())
        if job.second_parse_command():
            parse_filepath = str(PurePath(work_dir) / SECOND_PARSE_FILE)
            with open(parse_filepath, "w+") as parse_file:
                with redirect_stdout(parse_file):
                    parse_vtr_flow(job.second_parse_command())
        if job.qor_parse_command():
            parse_filepath = str(PurePath(work_dir) / QOR_PARSE_FILE)
            with open(parse_filepath, "w+") as parse_file:
                with redirect_stdout(parse_file):
                    parse_vtr_flow(job.qor_parse_command())
        max_arch_len = max(max_arch_len, len(job.arch()))
        max_circuit_len = max(max_circuit_len, len(job.circuit()))
    parse_files(config_jobs, run_dir, flow_metrics_basename)

    if config.second_parse_file:
        parse_files(config_jobs, run_dir, SECOND_PARSE_FILE)

    if config.qor_parse_file:
        parse_files(config_jobs, run_dir, QOR_PARSE_FILE)


def parse_files(config_jobs, run_dir, flow_metrics_basename=FIRST_PARSE_FILE):
    """ Parse the result files from the give jobs """
    task_parse_results_filepath = str(PurePath(run_dir) / flow_metrics_basename)
    with open(task_parse_results_filepath, "w") as out_f:

        # Start the header

        header = True
        for job in config_jobs:
            # Open the job results file
            #
            # The job results file is basically the same format,
            # but excludes the architecture and circuit fields,
            # which we prefix to each line of the task result file
            job_parse_results_filepath = Path(job.work_dir(run_dir)) / flow_metrics_basename
            if job_parse_results_filepath.exists():
                with open(job_parse_results_filepath) as in_f:
                    lines = in_f.readlines()
                    assert len(lines) == 2
                    if header:
                        # First line is the header
                        print(lines[0], file=out_f, end="")
                        header = False
                    # Second line is the data
                    print(lines[1], file=out_f, end="")
                pretty_print_table(job_parse_results_filepath)
            else:
                print(
                    "Warning: Flow result file not found (task QoR will be incomplete): {} ".format(
                        str(job_parse_results_filepath)
                    )
                )


def create_golden_results_for_tasks(configs):
    """ Runs create_golden_results_for_task on all of the give configuration """

    for config in configs:
        create_golden_results_for_task(config)


def create_golden_results_for_task(config):
    """
    Copies the latest task run's parse_results.txt into the config directory as golden_results.txt
    """
    run_dir = find_latest_run_dir(config)

    task_results = str(PurePath(run_dir).joinpath(FIRST_PARSE_FILE))
    golden_results_filepath = str(PurePath(config.config_dir).joinpath("golden_results.txt"))

    shutil.copy(task_results, golden_results_filepath)


def check_golden_results_for_tasks(configs):
    """ runs check_golden_results_for_task on all the input configurations """
    num_qor_failures = 0

    print("\nCalculating QoR results...")
    for config in configs:
        num_qor_failures += check_golden_results_for_task(config)

    return num_qor_failures


def check_golden_results_for_task(config):
    """
    Copies the latest task run's parse_results.txt into the config directory as golden_results.txt
    """
    num_qor_failures = 0
    run_dir = find_latest_run_dir(config)

    if not config.pass_requirements_file:
        print(
            "Warning: no pass requirements file for task {}, QoR will not be checked".format(
                config.task_name
            )
        )
    else:

        # Load the pass requirements file

        # Load the task's parse results
        task_results_filepath = str(PurePath(run_dir).joinpath(FIRST_PARSE_FILE))

        # Load the golden reference
        if config.second_parse_file:
            second_results_filepath = str(PurePath(run_dir).joinpath(SECOND_PARSE_FILE))
            num_qor_failures = check_two_files(
                config,
                task_results_filepath,
                second_results_filepath,
                second_name="second parse file",
            )
            pretty_print_table(second_results_filepath)

        else:
            golden_results_filepath = str(
                PurePath(config.config_dir).joinpath("golden_results.txt")
            )
            num_qor_failures = check_two_files(
                config, task_results_filepath, golden_results_filepath,
            )
        pretty_print_table(task_results_filepath)

    if num_qor_failures == 0:
        print("{}...[Pass]".format("/".join(str((Path(config.config_dir).parent)).split("/")[-3:])))

    return num_qor_failures


# pylint: disable=too-many-branches,too-many-locals
def check_two_files(
    config,
    first_results_filepath,
    second_results_filepath,
    first_name="task",
    second_name="golden",
):
    """ Compare two files results """
    first_results = load_parse_results(first_results_filepath)
    second_results = load_parse_results(second_results_filepath)
    # Verify that the architecture and circuit are specified
    for param in ["architecture", "circuit", "script_params"]:
        if param not in first_results.PRIMARY_KEYS:
            raise InspectError(
                "Required param '{}' missing from {} results: {}".format(
                    param, first_name, first_results_filepath
                ),
                first_results_filepath,
            )

        if param not in second_results.PRIMARY_KEYS:
            raise InspectError(
                "Required param '{}' missing from {} results: {}".format(
                    param, second_name, second_results_filepath
                ),
                second_results_filepath,
            )

    # Verify that all params and pass requirement metric are included in both the result files
    # We do not worry about non-pass_requriements elements being different or missing
    pass_req_filepath = str(paths.pass_requirements_path / config.pass_requirements_file)
    pass_requirements = load_pass_requirements(pass_req_filepath)

    for metric in pass_requirements.keys():
        for ((arch, circuit, script_params), result,) in first_results.all_metrics().items():
            if metric not in result:
                raise InspectError(
                    "Required metric '{}' missing from {} results".format(metric, first_name),
                    first_results_filepath,
                )

        for ((arch, circuit, script_params), result,) in second_results.all_metrics().items():
            if metric not in result:
                raise InspectError(
                    "Required metric '{}' missing from {} results".format(metric, second_name),
                    second_results_filepath,
                )

    # Load the primary keys for result files
    second_primary_keys = []
    for (arch, circuit, script_params), _ in second_results.all_metrics().items():
        second_primary_keys.append((arch, circuit, script_params))

    first_primary_keys = []
    for (arch, circuit, script_params), _ in first_results.all_metrics().items():
        first_primary_keys.append((arch, circuit, script_params))

    # Ensure that first result file  has all the second result file cases
    for arch, circuit, script_params in second_primary_keys:
        if first_results.metrics(arch, circuit, script_params) is None:
            raise InspectError(
                "Required case {}/{} missing from {} results: {}".format(
                    arch, circuit, first_name, first_results_filepath
                )
            )

    # Warn about any elements in first result file that are not found in second result file
    for arch, circuit, script_params in first_primary_keys:
        if second_results.metrics(arch, circuit, script_params) is None:
            print(
                "Warning: {} includes result for {}/{} missing in {} results".format(
                    first_name, arch, circuit, second_name
                )
            )
    num_qor_failures = 0
    # Verify that the first results pass each metric for all cases in the second results
    for (arch, circuit, script_params) in second_primary_keys:
        second_metrics = second_results.metrics(arch, circuit, script_params)
        first_metrics = first_results.metrics(arch, circuit, script_params)
        first_fail = True
        for metric in pass_requirements.keys():

            if not metric in second_metrics:
                print("Warning: Metric {} missing from {} results".format(metric, second_name))
                continue

            if not metric in first_metrics:
                print("Warning: Metric {} missing from {} results".format(metric, first_name))
                continue

            try:
                metric_passed, reason = pass_requirements[metric].check_passed(
                    second_metrics[metric], first_metrics[metric], second_name
                )
            except InspectError as error:
                metric_passed = False
                reason = error.msg

            if not metric_passed:
                if first_fail:
                    print(
                        "\n{}...[Fail]".format(
                            "/".join(str((Path(config.config_dir).parent)).split("/")[-3:])
                        )
                    )
                    first_fail = False
                print("[Fail]\n{}/{}/{} {} {}".format(arch, circuit, script_params, metric, reason))
                num_qor_failures += 1
    return num_qor_failures


# pylint: enable=too-many-branches,too-many-locals


def summarize_qor(configs):
    """ Summarize the Qor results """

    first = True
    task_path = Path(configs[0].config_dir).parent
    if len(configs) > 1 or (task_path.parent / "task_list.txt").is_file():
        task_path = task_path.parent
    task_path = task_path / "task_summary"
    task_path.mkdir(exist_ok=True)
    out_file = task_path / (str(Path(find_latest_run_dir(configs[0])).stem) + "_summary.txt")
    with out_file.open("w+") as out:
        for config in configs:
            with (Path(find_latest_run_dir(config)) / QOR_PARSE_FILE).open("r") as in_file:
                headers = in_file.readline()
                if first:
                    print("task_name \t{}".format(headers), file=out, end="")
                    first = False
                for line in in_file:
                    print("{}\t{}".format(config.task_name, line), file=out, end="")
            pretty_print_table(str(Path(find_latest_run_dir(config)) / QOR_PARSE_FILE))


def calc_geomean(args, configs):
    """ caclulate and ouput the geomean values to the geomean file """
    first = False
    task_path = Path(configs[0].config_dir).parent
    if len(configs) > 1 or (task_path.parent / "task_list.txt").is_file():
        task_path = task_path.parent
    out_file = task_path / "qor_geomean.txt"
    if not out_file.is_file():
        first = True
    summary_file = (
        task_path
        / "task_summary"
        / (str(Path(find_latest_run_dir(configs[0])).stem) + "_summary.txt")
    )

    with out_file.open("w" if first else "a") as out:
        with summary_file.open("r") as summary:
            params = summary.readline().strip().split("\t")[4:]
            if first:
                print("run", file=out, end="\t")
                for param in params:
                    print(param, file=out, end="\t")
                print("date\trevision", file=out)
                first = False
            lines = summary.readlines()
            print(
                get_latest_run_number(str(Path(configs[0].config_dir).parent)), file=out, end="\t",
            )
            for index in range(len(params)):
                geo_mean = 1
                num = 0
                previous_value = None
                geo_mean, num, previous_value = calculate_individual_geo_mean(
                    lines, index, geo_mean, num
                )
                if num:
                    geo_mean **= 1 / num
                    print(geo_mean, file=out, end="\t")
                else:
                    print(
                        previous_value if previous_value is not None else "-1", file=out, end="\t",
                    )
        print(datetime.date(datetime.now()), file=out, end="\t")
        print(args.revision, file=out)


def calculate_individual_geo_mean(lines, index, geo_mean, num):
    """ Calculate an individual line of parse results goe_mean """
    previous_value = None
    for line in lines:
        line = line.split("\t")[4:]
        current_value = line[index]
        try:
            if float(current_value) > 0:
                geo_mean *= float(current_value)
                num += 1
        except ValueError:
            if not previous_value:
                previous_value = current_value
            elif current_value != previous_value:
                previous_value = "-1"
    return geo_mean, num, previous_value


def find_latest_run_dir(config):
    """ Find the latest run directory for given configuration """
    task_dir = find_task_dir(config)

    run_dir = get_latest_run_dir(task_dir)

    if not run_dir:
        raise InspectError(
            "Failed to find run directory for task '{}' in '{}'".format(config.task_name, task_dir)
        )

    assert Path(run_dir).is_dir()

    return run_dir


if __name__ == "__main__":
    retval = vtr_command_main(sys.argv[1:])
    sys.exit(retval)
