"""
    Module to utilize many of the tools needed for VTR.
"""
from pathlib import PurePath
from pathlib import Path
import sys
import re
import time
import subprocess
import argparse
import csv
from collections import OrderedDict
from prettytable import PrettyTable
import vtr.error
from vtr.error import CommandError
from vtr import paths


class RawDefaultHelpFormatter(
    argparse.ArgumentDefaultsHelpFormatter, argparse.RawDescriptionHelpFormatter
):
    """
    An argparse formatter which supports both default arguments and raw
    formatting of description/epilog
    """


# pylint: disable=too-many-arguments, too-many-instance-attributes, too-few-public-methods, too-many-locals
class CommandRunner:
    """
        An object for running system commands with timeouts, memory limits and varying verbose-ness

        Arguments
        =========
            timeout_sec: maximum walk-clock-time of the command in seconds. Default: None
            max_memory_mb: maximum memory usage of the command in megabytes (if supported).
                Default: None
            track_memory: Whether to track usage of the command (disabled if not supported).
                Default: True
            verbose_error: Produce more verbose output if the command fails.
                Default: Equal to verbose
            verbose: Produce more verbose output. Default: False
            echo_cmd: Echo the command before running. Default: Equal to verbose
            indent: The string specifying a single indent (used in verbose mode)
            valgrind: Indicates if commands should be run with valgrind
        """

    def __init__(
        self,
        timeout_sec=None,
        max_memory_mb=None,
        track_memory=True,
        verbose=False,
        echo_cmd=None,
        indent="\t",
        show_failures=False,
        valgrind=False,
        expect_fail=None,
    ):
        if echo_cmd is None:
            echo_cmd = verbose

        self._timeout_sec = timeout_sec
        self._max_memory_mb = max_memory_mb
        self._track_memory = track_memory
        self._verbose = verbose
        self._echo_cmd = echo_cmd
        self._indent = indent
        self._show_failures = show_failures
        self._valgrind = valgrind
        self._expect_fail = expect_fail

    def run_system_command(
        self, cmd, temp_dir, log_filename=None, expected_return_code=0, indent_depth=0
    ):
        """
        Runs the specified command in the system shell.

        Returns
        =======
            A tuple of the command output (list of lines) and the command's return code.

        Arguments
        =========
            cmd: list of tokens that form the command to be run
            log_filename: the log fiel name for the command's output. Default: derived from command
            temp_dir: The directory to run the command in. Default: None (uses object default).
            expected_return_code: The expected return code from the command.
            If the actula return code does not match, will generate an exception. Default: 0
            indent_depth: How deep to indent the tool output in verbose mode. Default: 0
        """
        # Save the original command
        orig_cmd = cmd
        temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir

        # If no log file is specified the name is based on the executed command
        log_filename = PurePath(orig_cmd[0]).name + ".out" if log_filename is None else log_filename

        # Limit memory usage?
        memory_limit = ["ulimit", "-Sv", "{val};".format(val=self._max_memory_mb)]
        cmd = memory_limit + cmd if self._max_memory_mb and check_cmd(memory_limit[0]) else cmd

        # Enable memory tracking?
        memory_tracking = ["/usr/bin/env", "time", "-v"]
        cmd = (
            (
                memory_tracking
                + [
                    "valgrind",
                    "--leak-check=full",
                    "--suppressions=" + str(paths.valgrind_supp),
                    "--error-exitcode=1",
                    "--errors-for-leak-kinds=none",
                    "--track-origins=yes",
                    "--log-file=valgrind.log",
                    "--error-limit=no",
                ]
                + cmd
                if self._valgrind
                else memory_tracking + cmd
            )
            if self._track_memory and check_cmd(memory_tracking[0])
            else cmd
        )

        # Flush before calling subprocess to ensure output is ordered
        # correctly if stdout is buffered
        sys.stdout.flush()

        # Echo the command?
        if self._echo_cmd:
            print(cmd)

        # Begin timing
        start_time = time.time()

        cmd_output = []
        cmd_returncode = None
        proc = None
        try:
            # Call the command
            stderr = None if self._valgrind else subprocess.STDOUT
            proc = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,  # We grab stdout
                stderr=stderr,  # stderr redirected to stderr
                universal_newlines=True,  # Lines always end in \n
                cwd=str(temp_dir),  # Where to run the command
            )

            # Read the output line-by-line and log it
            # to a file.
            #
            # We do this rather than use proc.communicate()
            # to get interactive output
            with (temp_dir / log_filename).open("w") as log_f:
                # Print the command at the top of the log
                log_f.write(" ".join(cmd))
                log_f.write("\n")

                # Read from subprocess output
                for line in proc.stdout:

                    # Send to log file
                    log_f.write(line)

                    # Save the output
                    cmd_output.append(line)

                    # Abort if over time limit
                    elapsed_time = time.time() - start_time
                    if self._timeout_sec and elapsed_time > self._timeout_sec:
                        proc.terminate()

                # Should now be finished (since we stopped reading from proc.stdout),
                # sets the return code
                proc.wait()

        finally:
            # Clean-up if we did not exit cleanly
            if proc:
                if proc.returncode is None:
                    # Still running, stop it
                    proc.terminate()

                cmd_returncode = proc.returncode

        cmd_errored = cmd_returncode != expected_return_code

        # Send to stdout
        if self._show_failures and self._verbose:
            for line in cmd_output:
                print(indent_depth * self._indent + line, end="")

        if self._show_failures and cmd_errored and not self._expect_fail:
            print("\nFailed log file follows({}):".format(str((temp_dir / log_filename).resolve())))
            for line in cmd_output:
                print(indent_depth * self._indent + "<" + line, end="")
        if cmd_errored:
            raise CommandError(
                "Executable {} failed".format(PurePath(orig_cmd[0]).name),
                cmd=cmd,
                log=str(temp_dir / log_filename),
                returncode=cmd_returncode,
            )
        return cmd_output, cmd_returncode

    # pylint: enable=too-many-arguments, too-many-instance-attributes, too-few-public-methods, too-many-locals


def check_cmd(command):
    """
    Return True if command can be run, False otherwise.
    """

    return Path(command).exists()


def pretty_print_table(file, border=False):
    """ Convert file to a pretty, easily read table """
    table = PrettyTable()
    table.border = border
    reader = None
    with open(file, "r") as csv_file:
        reader = csv.reader(csv_file, delimiter="\t")
        first = True
        for row in reader:
            row = [row_item.strip() + "\t" for row_item in row]
            while row[-1] == "\t":
                row = row[:-1]
            if first:
                table.field_names = list(row)
                for head in list(row):
                    table.align[head] = "l"
                first = False
            else:
                table.add_row(row)
    with open(file, "w+") as out_file:
        print(table, file=out_file)


def write_tab_delimitted_csv(filepath, rows):
    """
    Write out the data provied in a tab-delimited CSV format

    filepath: The filepath to write the data to
    rows: An iterable of dictionary-like elements; each element
          provides a set key-value pairs corresponding to a row
          in the output file
    """
    # Calculate the max width of each column
    columns = OrderedDict()
    for row in rows:
        for key, value in row.items():

            if key not in columns:
                columns[key] = max(len(key), len(str(value)))
            else:
                columns[key] = max(columns[key], len(str(value)))

    # Write the elements
    with open(filepath, "w+") as file:
        writer = csv.writer(file, delimiter="\t")

        # Write out the header
        header = []
        for col_name, col_width in columns.items():
            header.append("{:{width}}".format(col_name, width=col_width))
        writer.writerow(header)

        # Write rows
        for row in rows:
            values = []
            for col_name, col_width in columns.items():
                values.append("{:{width}}".format(row[col_name], width=col_width))
            writer.writerow(values)


def load_tab_delimited_csv(filepath):
    """
        loads a tab delimted csv as a list of ordered dictionaries
    """
    data = []
    with open(filepath) as file:
        reader = csv.reader(file, delimiter="\t")

        header = []
        for csv_row in reader:
            if len(header) == 0:
                header = [val.strip() for val in csv_row]
            else:
                data_row = OrderedDict()

                for i, value in enumerate(csv_row):
                    data_row[header[i]] = value.strip()

                data.append(data_row)

    return data


def print_verbose(min_verbosity, curr_verbosity, string, endl=True):
    """
    Print string if curr_verbosity is gteq min_verbosity
    """
    if curr_verbosity >= min_verbosity:
        if endl:
            print(string)
        else:
            print(string,)


def file_replace(filename, search_replace_dict):
    """
        searches file for specified values and replaces them with specified values.
    """
    lines = []
    with open(filename, "r") as file:
        lines = file.readlines()

    with open(filename, "w") as file:
        for line in lines:
            for search, replace in search_replace_dict.items():
                line = line.replace(search, str(replace))
            print(line, file=file)


def relax_w(min_w, relax_factor, base=2):
    """
    Scale min_w by relax_factor and round to the nearest multiple of base.
    """
    relaxed_w = int(base * round(min_w * relax_factor / base))
    return relaxed_w


def load_list_file(list_file):
    """
    Loads a file containing a single value-per-line,
    potentially with '#' comments
    """
    values = []
    with open(list_file) as file:
        for line in file:
            line = line.strip()
            # Handle comments
            if "#" in line:
                line = line.split("#")[0]
            if line == "":
                continue
            values.append(line)
    return values


def load_config_lines(filepath, allow_includes=True):
    """
    Loads the lines of a file, stripping blank lines and '#' comments.

    If allow_includes is set then lines of the form:

        @include "another_file.txt"

    will cause the specified file to be included in-line.
    The @included filename is interpretted as relative to the directory
    containing filepath.

    Returns a list of lines
    """
    config_lines = []

    blank_regex = re.compile(r"^\s*$")
    try:
        with open(filepath) as file:
            for line in file:
                # Trim '\n'
                line = line.strip()

                # Trim comments
                if "#" in line:
                    line = line.split("#")[0]

                # Skip blanks
                if blank_regex.match(line):
                    continue

                if line.startswith("%include"):
                    if allow_includes:
                        components = line.split()
                        assert len(components) == 2

                        include_file = components[1].strip('"')  # Strip quotes
                        include_file_abs = str(Path(filepath).parent / include_file)

                        # Recursively load the config
                        config_lines += load_config_lines(
                            include_file_abs, allow_includes=allow_includes
                        )
                    else:
                        raise vtr.error.InspectError("@include not allowed in this file", filepath)
                else:
                    config_lines.append(line)
    except IOError as error:
        raise vtr.error.InspectError("Error opening config file ({})".format(error))

    return config_lines


def verify_file(file, file_type, should_exist=True):
    """
        Verifies that the file is a Pathlib object and if not makes it one.
        Ensures that the file exists by default.
        This makes it possible to pass files into the various files as strings or as pathlib objects
    """
    if not isinstance(file, Path):
        file = Path(file)
    if should_exist and not file.is_file():
        raise Exception(
            "{file_type} file does not exist: {file} ".format(file_type=file_type, file=file)
        )

    return file


def format_elapsed_time(time_delta):
    """
        formats a time into desired string format
    """
    return "%.2f seconds" % time_delta.total_seconds()


def argparse_str2bool(str_val):
    """
        parses a string boolean to a boolean
    """
    str_val = str_val.lower()
    if str_val in ["yes", "on", "true", "1"]:
        return True
    if str_val in ["no", "off", "false", "0"]:
        return False
    raise argparse.ArgumentTypeError("Boolean value expected.")


def get_next_run_dir(base_dir):
    """
    Returns the next unused run directory within base_dir.

    Does not create the directory
    """
    return str(PurePath(base_dir) / run_dir_name(get_next_run_number(base_dir)))


def find_task_dir(config):
    """
    find the task directory
    """
    task_dir = None
    # Task dir is just above the config directory
    task_dir = Path(config.config_dir).parent
    assert task_dir.is_dir

    return str(task_dir)


def get_latest_run_dir(base_dir):
    """
    Returns the run directory with the highest run number in base_dir
    """
    latest_run_number = get_latest_run_number(base_dir)

    if latest_run_number is None:
        return None

    return str(PurePath(base_dir) / run_dir_name(latest_run_number))


def get_next_run_number(base_dir):
    """
    Returns the next available (i.e. non-existing) run number in base_dir
    """
    latest_run_number = get_latest_run_number(base_dir)

    if latest_run_number is None:
        next_run_number = 1
    else:
        next_run_number = latest_run_number + 1

    return next_run_number


def get_latest_run_number(base_dir):
    """
    Returns the highest run number of all run directories with in base_dir
    """
    run_number = 1
    run_dir = Path(base_dir) / run_dir_name(run_number)

    if not run_dir.exists():
        # No existing run directories
        return None

    while run_dir.exists():
        run_number += 1
        run_dir = Path(base_dir) / run_dir_name(run_number)

    # Currently one-past the last existing run dir,
    # to get latest existing, subtract one
    return run_number - 1


def run_dir_name(run_num):
    """
    Returns the formatted directory name for a specific run number
    """
    return "run{:03d}".format(run_num)
