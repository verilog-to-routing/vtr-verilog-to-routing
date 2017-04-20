import os
import sys
import time
import errno
import subprocess
import distutils.spawn as distutils_spawn
import argparse

from verilogtorouting.error import *

class RawDefaultHelpFormatter(argparse.ArgumentDefaultsHelpFormatter, argparse.RawDescriptionHelpFormatter):
    """
    An argparse formatter which supports both default arguments and raw formatting of description/epilog
    """
    pass

class CommandRunner(object):

    def __init__(self, timeout_sec=None, max_memory_mb=None, track_memory=True, verbose=False, echo_cmd=False, indent="\t"):
        """
        An object for running system commands with timeouts, memory limits and varying verbose-ness 

        Arguments
        =========
            timeout_sec: maximum walk-clock-time of the command in seconds. Default: None
            max_memory_mb: maximum memory usage of the command in megabytes (if supported). Default: None
            track_memory: Whether to track usage of the command (disabled if not supported). Default: True
            verbose: Produce more verbose output. Default: False
            echo_cmd: Echo the command before running. Default: False
            indent: The string specifying a single indent (used in verbose mode)
        """
        self._timeout_sec = timeout_sec
        self._max_memory_mb = max_memory_mb
        self._track_memory = track_memory
        self._verbose = verbose
        self._echo_cmd = echo_cmd
        self._indent = indent

    def run_system_command(self, cmd, work_dir, log_filename=None, exepcted_return_code=0, indent_depth=0):
        """
        Runs the specified command in the system shell.

        Returns
        =======
            A tuple of the command output (list of lines) and the command's return code.

        Arguments
        =========
            cmd: list of tokens that form the command to be run
            log_filename: name of the log file for the command's output. Default: derived from command
            work_dir: The directory to run the command in. Default: None (uses object default).
            expected_return_code: The expected return code from the command. If the actula return code does not match, will generate an exception. Default: 0
            indent_depth: How deep to indent the tool output in verbose mode. Default 0
        """
        #Save the original command
        orig_cmd = cmd

        #If no log file is specified the name is based on the executed command
        if log_filename == None:
            log_filename = os.path.basename(orig_cmd[0]) + '.out'


        #Limit memory usage?
        memory_limit = ["ulimit", "-Sv", "{val};".format(val=self._max_memory_mb)]
        if self._max_memory_mb != None and self.check_command(memory_limit[0]):
            cmd = memory_limit + cmd

        #Enable memory tracking?
        memory_tracking = ["/usr/bin/time", "-v"]
        if self._track_memory and self.check_command(memory_tracking[0]):
            cmd = memory_tracking + cmd

        #Flush before calling subprocess to ensure output is ordered
        #correctly if stdout is buffered
        sys.stdout.flush()

        #Echo the command?
        if self._echo_cmd:
            #print ' '.join(cmd)
            print cmd

        #Begin timing
        start_time = time.time()

        cmd_output=[]
        cmd_returncode=None
        proc = None
        try:
            #Call the command
            proc = subprocess.Popen(cmd,
                                    stdout=subprocess.PIPE, #We grab stdout
                                    stderr=subprocess.STDOUT, #stderr redirected to stderr
                                    universal_newlines=True, #Lines always end in \n
                                    cwd=work_dir, #Where to run the command
                                    )

            # Read the output line-by-line and log it
            # to a file.
            #
            # We do this rather than use proc.communicate() 
            # to get interactive output
            with open(os.path.join(work_dir, log_filename), 'w') as log_f:
                #Print the command at the top of the log
                print >> log_f, " ".join(cmd)

                #Read from subprocess output
                for line in proc.stdout:

                    #Send to log file
                    print >> log_f, line,

                    #Save the output
                    cmd_output.append(line)

                    #Send to stdout
                    if self._verbose:
                        print indent_depth*self._indent + line,

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

        if cmd_returncode != exepcted_return_code:
            raise CommandError(msg="Executable {exec_name} failed".format(exec_name=os.path.basename(orig_cmd[0])), 
                               cmd=cmd,
                               log=os.path.join(work_dir, log_filename),
                               returncode=cmd_returncode)

        return cmd_output, cmd_returncode

    def check_command(self, command):
        """
        Return True if command can be run, False otherwise.
        """

        #TODO: actually check for this
        return True


def make_enum(*sequential, **named):
    """
    Makes an enumeration

    >> MY_ENUM = enum(ONE, TWO, THREE)
    """
    enums = dict(zip(sequential, range(len(sequential))), **named)
    reverse = dict((value, key) for key, value in enums.iteritems())
    enums['reverse_mapping'] = reverse
    return type('Enum', (), enums)

def mkdir_p(path):
    """
    Makes a directory (including parents) at the specified path
    """
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

def print_verbose(min_verbosity, curr_verbosity, string, endl=True):
    """
    Print string if curr_verbosity is gteq min_verbosity
    """
    if curr_verbosity >= min_verbosity:
        if endl:
            print string
        else:
            print string,

def find_vtr_file(filename, is_executabe=False):
    """
    Attempts to find a VTR related file by searching the environment.
    
    Checking the following places:
        1) System path (if is_executabe=True)
        2) The inferred vtr root from environment variables or the script file location

    """
    #
    #Next check if it is on the path (provided it is executable)
    #
    if is_executabe:
        result = distutils_spawn.find_executable(filename)
        if result:
            return result

    vtr_root = find_vtr_root()

    #Check the inferred VTR root
    result = find_file_from_vtr_root(filename, vtr_root, is_executabe=is_executabe)
    if result:
        return result

    raise ValueError("Could not find {type} {file}".format(type="executable" if is_executabe else "file",file=filename))

def find_file_from_vtr_root(filename, vtr_root, is_executabe=False):
    """
    Given a vtr_root and a filename searches for the file recursively under some common VTR directories
    """
    for subdir in ['vpr', 'abc', 'abc_with_bb_support', 'ODIN_II', 'vtr_flow', 'ace2']:

        for root, dirs, files in os.walk(os.path.join(vtr_root, subdir)):
            for file in files:
                if file == filename:
                    full_file_path = os.path.join(root, file)

                    if os.path.isfile(full_file_path):
                        if is_executabe:
                            #Found an executable file as required
                            if os.access(full_file_path, os.X_OK):
                                return full_file_path
                        else:
                            #Found a file as required
                            return full_file_path
    return None

def find_vtr_root():
    for env_var in ['VTR_ROOT', 'VTR']:
        if env_var in os.environ:
            return os.environ[env_var]

    inferred_script_dir = os.path.dirname(os.path.abspath(__file__))

    #We assume that this file is in <vtr_root>/vtr_flow/python_libs/verilogtorouting
    inferred_vtr_root = os.path.abspath(os.path.join(inferred_script_dir, '../../../..'))

    if os.path.isdir(inferred_vtr_root):
        return inferred_vtr_root
    else:
        raise VtrError("Could not find VTR root directory. Try setting VTR_ROOT environment variable.")

def file_replace(filename, search_replace_dict):
    lines = []
    with open(filename, 'r') as f:
        lines = f.readlines()

    with open(filename, 'w') as f:
        for line in lines:
            for search, replace in search_replace_dict.iteritems():
                line = line.replace(search, str(replace))
            print >>f, line,

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

def format_elapsed_time(time_delta):
    total_sec = int(round(time_delta.total_seconds()))
    m, s = divmod(total_sec, 60)
    h, m = divmod(m, 60)

    return "{:02}:{:02}:{:02}".format(h, m, s)
