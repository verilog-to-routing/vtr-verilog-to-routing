import os
from pathlib import PurePath
from pathlib import Path
import sys
import re
import time
import errno
import subprocess
import distutils.spawn as distutils_spawn
import argparse
import csv
from collections import OrderedDict

from vtr.error import VtrError, InspectError, CommandError

VERBOSITY_CHOICES = range(5)

class RawDefaultHelpFormatter(argparse.ArgumentDefaultsHelpFormatter, argparse.RawDescriptionHelpFormatter):
    """
    An argparse formatter which supports both default arguments and raw formatting of description/epilog
    """
    pass

class CommandRunner(object):

    def __init__(self, timeout_sec=None, max_memory_mb=None, track_memory=True, verbose_error=None, verbose=False, echo_cmd=None, indent="\t", show_failures=False,valgrind=False):
        """
        An object for running system commands with timeouts, memory limits and varying verbose-ness 

        Arguments
        =========
            timeout_sec: maximum walk-clock-time of the command in seconds. Default: None
            max_memory_mb: maximum memory usage of the command in megabytes (if supported). Default: None
            track_memory: Whether to track usage of the command (disabled if not supported). Default: True
            verbose_error: Produce more verbose output if the commadn fails. Default: Equal to verbose
            verbose: Produce more verbose output. Default: False
            echo_cmd: Echo the command before running. Default: Equal to verbose
            indent: The string specifying a single indent (used in verbose mode)
            valgrind: Indicates if commands should be run with valgrind
        """
        if verbose_error==None:
            verbose_error = verbose
        if echo_cmd == None:
            echo_cmd = verbose

        self._timeout_sec = timeout_sec
        self._max_memory_mb = max_memory_mb
        self._track_memory = track_memory
        self._verbose_error = verbose_error
        self._verbose = verbose
        self._echo_cmd = echo_cmd
        self._indent = indent
        self._show_failures = show_failures
        self._valgrind = valgrind

    def run_system_command(self, cmd, temp_dir, log_filename=None, expected_return_code=0, indent_depth=0):
        """
        Runs the specified command in the system shell.

        Returns
        =======
            A tuple of the command output (list of lines) and the command's return code.

        Arguments
        =========
            cmd: list of tokens that form the command to be run
            log_filename: name of the log file for the command's output. Default: derived from command
            temp_dir: The directory to run the command in. Default: None (uses object default).
            expected_return_code: The expected return code from the command. If the actula return code does not match, will generate an exception. Default: 0
            indent_depth: How deep to indent the tool output in verbose mode. Default 0
        """
        #Save the original command
        orig_cmd = cmd

        #If no log file is specified the name is based on the executed command
        if log_filename == None:
            log_filename = PurePath(orig_cmd[0]).name + '.out'


        #Limit memory usage?
        memory_limit = ["ulimit", "-Sv", "{val};".format(val=self._max_memory_mb)]
        if self._max_memory_mb != None and self.check_command(memory_limit[0]):
            cmd = memory_limit + cmd

        #Enable memory tracking?
        memory_tracking = ["/usr/bin/env", "time", "-v"]
        if self._track_memory and self.check_command(memory_tracking[0]):
            if self._valgrind:
                valgrind_args = ["valgrind", "--leak-check=full", "--suppressions={}".format(find_vtr_file("valgrind.supp")), "--error-exitcode=1", "--errors-for-leak-kinds=none", "--track-origins=yes", "--log-file=valgrind.log","--error-limit=no"]
                cmd = memory_tracking + valgrind_args + cmd
            else:
                cmd = memory_tracking + cmd

        #Flush before calling subprocess to ensure output is ordered
        #correctly if stdout is buffered
        sys.stdout.flush()

        #Echo the command?
        if self._echo_cmd:
            print(cmd)

        #Begin timing
        start_time = time.time()

        cmd_output=[]
        cmd_returncode=None
        proc = None
        try:
            #Call the command
            stderr = subprocess.STDOUT
            if self._valgrind:
                stderr = None
            proc = subprocess.Popen(cmd,
                                    stdout=subprocess.PIPE, #We grab stdout
                                    stderr=stderr, #stderr redirected to stderr
                                    universal_newlines=True, #Lines always end in \n
                                    cwd=temp_dir, #Where to run the command
                                    )

            # Read the output line-by-line and log it
            # to a file.
            #
            # We do this rather than use proc.communicate() 
            # to get interactive output
            with (Path(temp_dir) / log_filename).open('w') as log_f:
                #Print the command at the top of the log
                log_f.write(" ".join(cmd))
                log_f.write("\n")

                #Read from subprocess output
                for line in proc.stdout:

                    #Send to log file
                    log_f.write(line)

                    #Save the output
                    cmd_output.append(line)

                    #Abort if over time limit
                    elapsed_time = time.time() - start_time
                    if self._timeout_sec and elapsed_time > self._timeout_sec:
                        proc.terminate()

                #Should now be finished (since we stopped reading from proc.stdout),
                #sets the return code
                proc.wait()

        finally:
            #Clean-up if we did not exit cleanly
            if proc:
                if proc.returncode == None:
                    #Still running, stop it
                    proc.terminate()

                cmd_returncode = proc.returncode

        cmd_errored = (cmd_returncode != expected_return_code)

        #Send to stdout
        if self._show_failures and (self._verbose or (cmd_errored and self._verbose_error)):
            for line in cmd_output:
                print (indent_depth*self._indent + line,)


        if (self._show_failures and cmd_errored):
            raise CommandError(msg="Executable {exec_name} failed".format(exec_name=PurePath(orig_cmd[0]).name), 
                               cmd=cmd,
                               log=str(PurePath(temp_dir).joinpath(log_filename)),
                               returncode=cmd_returncode)

        return cmd_output, cmd_returncode

    def check_command(self, command):
        """
        Return True if command can be run, False otherwise.
        """

        #TODO: actually check for this
        return True


def write_tab_delimitted_csv(filepath, rows):
    """
    Write out the data provied in a tab-delimited CSV format

    filepath: The filepath to write the data to
    rows: An iterable of dictionary-like elements; each element 
          provides a set key-value pairs corresponding to a row 
          in the output file
    """
    #Calculate the max width of each column
    columns = OrderedDict()
    for row in rows:
        for key, value in row.items():

            if key not in columns:
                columns[key] = max(len(key), len(str(value)))
            else:
                columns[key] = max(columns[key], len(str(value)))

    #Write the elements
    with open(filepath, 'w+') as f:
        writer = csv.writer(f, delimiter='\t')

        #Write out the header
        header = []
        for col_name, col_width in columns.items():
            header.append("{:{width}}".format(col_name, width=col_width))
        writer.writerow(header)

        #Write rows
        for row in rows:
            values = []
            for col_name, col_width in columns.items():
                values.append("{:{width}}".format(row[col_name], width=col_width))
            writer.writerow(values)

def load_tab_delimited_csv(filepath):

    data = []
    with open(filepath) as f:
        reader = csv.reader(f, delimiter='\t')

        header = None
        for csv_row in reader:
            if header == None:
                header = [val.strip() for val in csv_row]
            else:
                data_row = OrderedDict()

                for i, value in enumerate(csv_row):
                    data_row[header[i]] = value.strip()

                data.append(data_row)

    return data

def make_enum(*sequential, **named):
    """
    Makes an enumeration

    >> MY_ENUM = enum(ONE, TWO, THREE)
    """
    enums = dict(zip(sequential, range(len(sequential))), **named)
    reverse = dict((value, key) for key, value in enums.items())
    enums['reverse_mapping'] = reverse
    return type('Enum', (), enums)

def mkdir_p(path):
    """
    Makes a directory (including parents) at the specified path
    """
    make_path = Path(path)
    try:
        make_path.mkdir(parents=True)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and make_path.is_dir():
            pass
        else:
            raise

def print_verbose(min_verbosity, curr_verbosity, string, endl=True):
    """
    Print string if curr_verbosity is gteq min_verbosity
    """
    if curr_verbosity >= min_verbosity:
        if endl:
            print(string)
        else:
            print (string,)

def find_vtr_file(filename, is_executable=False):
    """
    Attempts to find a VTR related file by searching the environment.
    
    Checking the following places:
        1) System path (if is_executable=True)
        2) The inferred vtr root from environment variables or the script file location

    """
    #We assume exectuables are specified in the unix style (no .exe),
    # if it was specified with .exe, strip it off
    file_path = PurePath(filename)
    if file_path.suffix == ".exe":
        filename = file_path.name

    #
    #Check if it is on the path (provided it is executable)
    #
    if is_executable:
        #Search first for the non-exe version
        result = distutils_spawn.find_executable(filename)
        if result:
            return result

        #If not found try the .exe version
        result = distutils_spawn.find_executable(filename + '.exe')
        if result:
            return result

    vtr_root = find_vtr_root()

    #Check the inferred VTR root
    result = find_file_from_vtr_root(filename, vtr_root, is_executable=is_executable)
    if result:
        return result

    #Since we stripped off the .exe, try looking for the .exe version as a last resort (i.e. on Windows/cygwin)
    if is_executable:
        result = find_file_from_vtr_root(filename + '.exe', vtr_root, is_executable=is_executable)
        if result:
            return result

    raise ValueError("Could not find {type} {file}".format(type="executable" if is_executable else "file",file=filename))

def find_file_from_vtr_root(filename, vtr_root, is_executable=False):
    """
    Given a vtr_root and a filename searches for the file recursively under some common VTR directories
    """
    for subdir in ['vpr', 'abc', 'abc_with_bb_support', 'ODIN_II', 'vtr_flow', 'ace2']:
        directory_path = (Path(vtr_root) / subdir)
        for file_path in directory_path.glob('**/*'):
            if file_path.name == filename:
                if file_path.is_file():
                    if is_executable:
                        #Found an executable file as required
                        if os.access(str(file_path), os.X_OK):
                            return str(file_path)
                    else:
                        #Found a file as required
                        return str(file_path)
    return None

def find_vtr_root():
    for env_var in ['VTR_ROOT']:
        if env_var in os.environ:
            return os.environ[env_var]

    #We assume that this file is in <vtr_root>/vtr_flow/python_libs/verilogtorouting
    inferred_vtr_root = Path(__file__).parent / '../../../..'

    if inferred_vtr_root.is_dir:
        return str(inferred_vtr_root)
    else:
        raise VtrError("Could not find VTR root directory. Try setting VTR_ROOT environment variable.")

def file_replace(filename, search_replace_dict):
    lines = []
    with open(filename, 'r') as f:
        lines = f.readlines()

    with open(filename, 'w') as f:
        for line in lines:
            for search, replace in search_replace_dict.items():
                line = line.replace(search, str(replace))
            print (line,file=f)

def relax_W(min_W, relax_factor, base=2):
    """
    Scale min_W by relax_factor and round to the nearest multiple of base.
    """
    relaxed_W = int(base * round(min_W*relax_factor/base))
    return relaxed_W

def load_list_file(list_file):
    """
    Loads a file containing a single value-per-line,
    potentially with '#' comments
    """
    values = []
    with open(list_file) as f:
        for line in f:
            line = line.strip()
            #Handle comments
            if '#' in line:
                line = line.split('#')[0]
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
        with open(filepath) as f:
            for line in f:
                #Trim '\n'
                line = line.strip()

                #Trim comments
                if '#' in line:
                    line = line.split('#')[0]

                #Skip blanks
                if blank_regex.match(line):
                    continue

                if line.startswith("%include"):
                    if allow_includes:
                        components = line.split()
                        assert len(components) == 2

                        include_file = components[1].strip('"') #Strip quotes
                        include_file_abs = str(Path(filepath).parent / include_file)

                        #Recursively load the config
                        config_lines += load_config_lines(include_file_abs, allow_includes=allow_includes) 
                    else:
                        raise InspectError("@include not allowed in this file", filepath)
                else:
                    config_lines.append(line)
    except IOError as e:
        raise InspectError("Error opening config file ({})".format(e))

    return config_lines            

def verify_file(file, file_type,should_exist=True):
    """
        Verifies that the file is a Pathlib object and if not makes it one. 
        Ensures that the file exists by default.
        This makes it possible to pass files into the various files as strings or as pathlib objects.
    """
    if(not isinstance(file,Path)):
        file = Path(file)
    if(should_exist and not file.is_file()):
        raise Exception("{file_type} file does not exist: {file} ".format(file_type = file_type, file=file))

    return file
    
def format_elapsed_time(time_delta):
    return "%.2f seconds" % time_delta.total_seconds()
        

def argparse_str2bool(str_val):
    str_val = str_val.lower()
    if str_val in ['yes', 'on', 'true', '1']:
        return True
    elif str_val in ['no', 'off', 'false', '0']:
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')


def get_next_run_dir(base_dir):
    """
    Returns the next unused run directory within base_dir.

    Does not create the directory
    """
    return str(PurePath(base_dir) / run_dir_name(get_next_run_number(base_dir)))

def get_latest_run_dir(base_dir):
    """
    Returns the run directory with the highest run number in base_dir
    """
    latest_run_number = get_latest_run_number(base_dir)

    if latest_run_number == None:
        return None

    return str(PurePath(base_dir) / run_dir_name(latest_run_number))

def get_next_run_number(base_dir):
    """
    Returns the next available (i.e. non-existing) run number in base_dir
    """
    latest_run_number = get_latest_run_number(base_dir)

    if latest_run_number == None:
        next_run_number = 0
    else:
        next_run_number = latest_run_number + 1

    return next_run_number

def get_latest_run_number(base_dir):
    """
    Returns the highest run number of all run directories with in base_dir
    """
    run_number = 0
    run_dir = Path(base_dir) / run_dir_name(run_number)

    if not run_dir.exists:
        #No existing run directories
        return None

    while run_dir.exists:
        run_number += 1
        run_dir = Path(base_dir) / run_dir_name(run_number)

    #Currently one-past the last existing run dir,
    #to get latest existing, subtract one
    return run_number - 1


def run_dir_name(run_num):
    """
    Returns the formatted directory name for a specific run number
    """
    return "run{:03d}".format(run_num)
