import os
import pwd
import re
import sys

HOOKS = [
    'strip_paths',
    'anonymize',
    'make_expected_failures_pass',
    'patch_logs'
    ]

def strip_paths(line):
    """
    strip all path from known file extensions 
    this can prevent broken test do to mismatch in path
    """
    # its a bit limiting, maybe a better regex would do
    # but it seems to work fine for now
    path_regex = r"[\/]?[a-zA-Z_.\-0-9]*\/"
    file_regex = r"[^\/\s]*(_input|_output|\.xml|\.v|\.vh|\.blif|\.log|\.do|\.dot|_vectors|_activity)"
    return re.sub(r"(" + path_regex + r")+(?=" + file_regex + r"[\s\n]+)", "", line)

def anonymize(line):
    """
    strip all occurance of this user in the log file
    """
    user_name = pwd.getpwuid(os.getuid()).pw_name
    return re.sub(r"(" + user_name + r")", "",line)
    

def patch_logs(line):
    """
    Some librairies and tools Odin uses can have different display messages
    we do replacements for possible mismatch to stop false positives
    we don't use regex 
    """
    for old_str, new_string in {
        # bison used to call EOF $end, but switched to end of file since 
        r"syntax error, unexpected \$end": r"syntax error, unexpected end of file",
    }.items():
        line = re.sub(old_str, new_string,line)

    return line


IS_EXPECTED_TO_FAIL = False
def make_expected_failures_pass(line):
    # figure out if we expected this to fail
    global IS_EXPECTED_TO_FAIL
    if re.search(r"^EXPECT:: failure$",line) is not None:
        IS_EXPECTED_TO_FAIL = True

    # if this was expected to fail
    if IS_EXPECTED_TO_FAIL:
        # make it pass
        line = re.sub(r"(?!Odin exited with code: )(\d+)", "0",line)
            
    return line
