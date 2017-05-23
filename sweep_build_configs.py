#!/usr/bin/env python
import argparse
import subprocess
import sys
import os
import re
import shutil
from collections import OrderedDict

DEFAULT_TARGETS_TO_BUILD=["all"]

DEFAULT_GNU_COMPILER_VERSIONS=["4.9", "5", "6", "7"]
DEFAULT_CLANG_COMPILER_VERSIONS=["3.6", "3.8"]

DEFAULT_EASYGL_CONFIGS = ["OFF", "ON"]
DEFAULT_TATUM_PARALLEL_CONFIGS = ["ON", "OFF"]
DEFAULT_VTR_ASSERT_LEVELS= ["2", "3"]#, "1", "0"]


ERROR_WARNING_REGEXES = [
        re.compile(r".*warning:.*"),
        re.compile(r".*error:.*"),
    ]

SUPPRESSION_ERROR_WARNING_REGEXES = [
        #We compile some .c files as C++, so we don't worry about these warnings from clang
        re.compile(r".*clang:.*warning:.*treating.*c.*as.*c\+\+.*"),
    ]

def parse_args():
    parser = argparse.ArgumentParser(description="Test building VTR for multiple different build configurations and compilers")

    parser.add_argument("targets", 
                        nargs="*",
                        default=DEFAULT_TARGETS_TO_BUILD,
                        help="What targets to build (default: %(default)s)")
    parser.add_argument("--gnu_versions", 
                        nargs="*",
                        default=DEFAULT_GNU_COMPILER_VERSIONS,
                        metavar="GNU_VERSION",
                        help="What versions of gcc/g++ to test (default: %(default)s)")
    parser.add_argument("--clang_versions", 
                        nargs="*",
                        metavar="CLANG_VERSION",
                        default=DEFAULT_CLANG_COMPILER_VERSIONS,
                        help="What versions of clang/clang++ to test (default: %(default)s)")
    parser.add_argument("--easygl_configs", 
                        nargs="*",
                        default=DEFAULT_EASYGL_CONFIGS,
                        metavar="EASYGL_CONFIG",
                        help="What EaysGL configurations to test (default: %(default)s)")
    parser.add_argument("--tatum_parallel_configs", 
                        nargs="*",
                        default=DEFAULT_TATUM_PARALLEL_CONFIGS,
                        metavar="TATUM_PARALLEL_CONFIG",
                        help="What parallel tatum configurations to test (default: %(default)s)")
    parser.add_argument("--vtr_assert_levels", 
                        nargs="*",
                        default=DEFAULT_VTR_ASSERT_LEVELS,
                        metavar="VTR_ASSERT_LEVEL",
                        help="What VTR assert levels to test (default: %(default)s)")
    parser.add_argument("-j", 
                        type=int,
                        default=1,
                        help="How many parallel build jobs to allow (passed to make)")
    parser.add_argument("--exit_on_failure",
                        action="store_true",
                        default=False,
                        help="Exit on first failure intead of continuing")

    return parser.parse_args()

def main():

    args = parse_args();

    compilers_to_test = []

    for gnu_version in args.gnu_versions:
        cc = "gcc-" + gnu_version
        cxx = "g++-" + gnu_version
        compilers_to_test.append((cc, cxx))

    for clang_version in args.clang_versions:
        cc = "clang-" + clang_version
        cxx = "clang++-" + clang_version
        compilers_to_test.append((cc, cxx))

    #Test all the compilers with the default build config
    num_failed = 0
    num_configs = 0
    for vtr_assert_level in args.vtr_assert_levels: 
        for easygl_config in args.easygl_configs: 
            for tatum_parallel_config in args.tatum_parallel_configs: 
                for cc, cxx in compilers_to_test:
                    num_configs += 1
                    config = OrderedDict()
                    config["EASYGL_ENABLE_GRAPHICS"] = easygl_config
                    config["TATUM_ENABLE_PARALLEL_ANALYSIS"] = tatum_parallel_config
                    config["VTR_ASSERT_LEVEL"] = vtr_assert_level

                    success = build_config(args, cc, cxx, config)

                    if not success:
                        num_failed += 1

                        if args.exit_on_failure:
                            sys.exit(num_failed)

    if num_failed != 0:
        print "Failed to build {} of {} configurations".format(num_failed, num_configs)

    sys.exit(num_failed)

def build_config(args, cc, cxx, config):

    if not compiler_is_found(cc):
        print "Failed to find C compiler {}, skipping".format(cc)
        return False

    if not compiler_is_found(cxx):
        print "Failed to find C++ compiler {}, skipping".format(cxx)
        return False

    config_strs = []
    for key, value in config.iteritems():
        config_strs += ["{}={}".format(key, value)]

    log_file = "build_{}_{}_{}.log".format(cc, cxx, '__'.join(config_strs))

    build_successful = True
    with open(log_file, 'w') as f:
        print "Building with CC={} CXX={}:".format(cc, cxx)
        print >>f, "Building with CC={} CXX={}:".format(cc, cxx)
        f.flush()

        build_dir = "build"

        shutil.rmtree(build_dir, ignore_errors=True)

        os.mkdir(build_dir)

        #Copy the os environment
        new_env = {}
        new_env.update(os.environ)

        #Override CC and CXX
        new_env['CC'] = cc
        new_env['CXX'] = cxx

        #Run CMAKE

        cmake_cmd = ["cmake"]
        for key, value in config.iteritems():
            cmake_cmd += ["-D{}={}".format(key, value)]
        cmake_cmd += [".."]

        print "  " + ' '.join(cmake_cmd)
        print >>f, "  " + ' '.join(cmake_cmd)
        f.flush()
        subprocess.check_call(cmake_cmd, cwd=build_dir, stdout=f, stderr=f, env=new_env)

        #Run Make
        build_cmd = ["make"]
        build_cmd += ["-j", "{}".format(args.j)]
        build_cmd += args.targets

        print "  " + ' '.join(build_cmd)
        print >>f, "  " + ' '.join(build_cmd)
        f.flush()
        try:
            subprocess.check_call(build_cmd, cwd=build_dir, stdout=f, stderr=f, env=new_env)
        except subprocess.CalledProcessError as e:
            build_successful = False

    #Look for errors and warnings in the build log
    issue_count = 0
    with open(log_file) as f:
        for line in f:
            if is_valid_warning_error(line):
                print "    " + line,
                issue_count += 1

    if not build_successful:
        print "  ERROR: failed to compile"
    else:
        print "  OK",
        if issue_count > 0:
            print " ({} warnings)".format(issue_count),
        print

    return build_successful

def is_valid_warning_error(line):
    for issue_regex in ERROR_WARNING_REGEXES:
        if issue_regex.match(line):
            for suppression_regex in SUPPRESSION_ERROR_WARNING_REGEXES:
                if suppression_regex.match(line):
                    return False #Suppressed
            return True #Valid error/warning
    return False #Not a problem

def compiler_is_found(execname):

    try:
        result = subprocess.check_output([execname, "--version"], stderr=subprocess.PIPE) == 0
    except OSError as e:
        return False
    return True

if __name__ == "__main__":
    main()
