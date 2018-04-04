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
DEFAULT_MINGW_COMPILER_VERSIONS=["5"]
DEFAULT_CLANG_COMPILER_VERSIONS=["3.6", "3.8"]

DEFAULT_EASYGL_CONFIGS = ["ON", "OFF"]
DEFAULT_TATUM_EXECUTION_ENGINE_CONFIGS = ["auto", "serial"]
DEFAULT_VTR_ASSERT_LEVELS= ["2", "3"]#, "1", "0"]

MINGW_TOOLCHAIN_FILE="cmake/toolchains/mingw-linux-cross-compile-to-windows.cmake"

ERROR_WARNING_REGEXES = [
        re.compile(r".*warning:.*"),
        re.compile(r".*error:.*"),
    ]

SUPPRESSION_ERROR_WARNING_REGEXES = [
        #We compile some .c files as C++, so we don't worry about these warnings from clang
        re.compile(r".*clang:.*warning:.*treating.*c.*as.*c\+\+.*"),
    ]

def parse_args():
    parser = argparse.ArgumentParser(description="Test building VTR for multiple different compilers and build configurations")

    parser.add_argument("targets", 
                        nargs="*",
                        default=DEFAULT_TARGETS_TO_BUILD,
                        help="What targets to build (default: %(default)s)")
    parser.add_argument("-j", 
                        type=int,
                        default=1,
                        metavar="NUM_JOBS",
                        help="How many parallel build jobs to allow (passed to make)")
    parser.add_argument("--exit_on_failure",
                        action="store_true",
                        default=False,
                        help="Exit on first failure intead of continuing")

    compiler_args = parser.add_argument_group("Compiler Configurations")
    compiler_args.add_argument("--gnu_versions", 
                        nargs="*",
                        default=DEFAULT_GNU_COMPILER_VERSIONS,
                        metavar="GNU_VERSION",
                        help="What versions of gcc/g++ to test (default: %(default)s)")
    compiler_args.add_argument("--clang_versions", 
                        nargs="*",
                        metavar="CLANG_VERSION",
                        default=DEFAULT_CLANG_COMPILER_VERSIONS,
                        help="What versions of clang/clang++ to test (default: %(default)s)")
    compiler_args.add_argument("--mingw_versions", 
                        nargs="*",
                        default=DEFAULT_MINGW_COMPILER_VERSIONS,
                        metavar="MINGW_W64_VERSION",
                        help="What versions of MinGW-W64 gcc/g++ to test for cross-compilation to Windows (default: %(default)s)")

    config_args = parser.add_argument_group("Build Configurations")
    config_args.add_argument("--easygl_configs", 
                        nargs="*",
                        default=DEFAULT_EASYGL_CONFIGS,
                        metavar="EASYGL_CONFIG",
                        help="What EaysGL configurations to test (default: %(default)s)")
    config_args.add_argument("--tatum_execution_engine_configs", 
                        nargs="*",
                        default=DEFAULT_TATUM_EXECUTION_ENGINE_CONFIGS,
                        metavar="TATUM_EXECUTION_ENGINE_CONFIG",
                        help="What parallel tatum configurations to test (default: %(default)s)")
    config_args.add_argument("--vtr_assert_levels", 
                        nargs="*",
                        default=DEFAULT_VTR_ASSERT_LEVELS,
                        metavar="VTR_ASSERT_LEVEL",
                        help="What VTR assert levels to test (default: %(default)s)")

    return parser.parse_args()

def main():

    args = parse_args();

    compilers_to_test = []

    for gnu_version in args.gnu_versions:
        cc = "gcc-" + gnu_version
        cxx = "g++-" + gnu_version
        compilers_to_test.append((cc, cxx, OrderedDict()))

    for clang_version in args.clang_versions:
        cc = "clang-" + clang_version
        cxx = "clang++-" + clang_version
        compilers_to_test.append((cc, cxx, OrderedDict()))

    for mingw_version in args.mingw_versions:
        config = OrderedDict() 
        config["CMAKE_TOOLCHAIN_FILE"] = MINGW_TOOLCHAIN_FILE
        if mingw_version != "":
            prefix = "x86_64-w64-mingw32"
            config["COMPILER_PREFIX"] = prefix
            config["MINGW_RC"] = "{}-windres".format(prefix)
            config["MINGW_CC"] = "{}-gcc-{}".format(prefix, mingw_version)
            config["MINGW_CXX"] = "{}-g++-{}".format(prefix, mingw_version)
        else:
            pass #Use defaults
        compilers_to_test.append((None, None, config))

    #Test all the regular compilers with the all build configs
    num_failed = 0
    num_configs = 0
    for vtr_assert_level in args.vtr_assert_levels: 
        for easygl_config in args.easygl_configs: 
            for tatum_execution_engine_config in args.tatum_execution_engine_configs: 
                for cc, cxx, config in compilers_to_test:
                    num_configs += 1
                    config["EASYGL_ENABLE_GRAPHICS"] = easygl_config
                    config["TATUM_EXECUTION_ENGINE"] = tatum_execution_engine_config
                    config["VTR_ASSERT_LEVEL"] = vtr_assert_level

                    targets = args.targets
                    if 'CMAKE_TOOLCHAIN_FILE' in config and targets == DEFAULT_TARGETS_TO_BUILD:
                        #Only build VPR with MINGW by default
                        # The updated version of ABC (and ace/odin which depend on ABC)
                        # fail to compile with MINGW
                        targets = ['vpr']

                    success = build_config(args, targets, config, cc, cxx)

                    if not success:
                        num_failed += 1

                        if args.exit_on_failure:
                            sys.exit(num_failed)



    if num_failed != 0:
        print "Failed to build {} of {} configurations".format(num_failed, num_configs)

    sys.exit(num_failed)

def build_config(args, targets, config, cc=None, cxx=None):

    if not compiler_is_found(cc):
        print "Failed to find C compiler {}, skipping".format(cc)
        return False

    if not compiler_is_found(cxx):
        print "Failed to find C++ compiler {}, skipping".format(cxx)
        return False

    config_strs = []
    for key, value in config.iteritems():
        config_strs += ["{}={}".format(key, value)]

    log_file = "build.log"
    # if cc != None:
        # log_file += "_{}".format(cc)
    # elif cxx != None:
        # log_file += "_{}".format(cxx)
    # #Remove an directory dividers from configs to yield a valid filename
    # escaped_config_strs = [str.replace("/", "_") for str in config_strs] 
    # log_file += "_{}.log".format('__'.join(escaped_config_strs))

    build_successful = True
    with open(log_file, 'w') as f:
        print      "Building with"
        print >>f, "Building with"
        if cc != None:
            print      " CC={}".format(cc)
            print >>f, " CC={}".format(cc)
        if cxx != None:
            print      " CXX={}:".format(cxx)
            print >>f, " CXX={}:".format(cxx)
        if "CMAKE_TOOLCHAIN_FILE" in config:
            print      " Toolchain={}".format(config["CMAKE_TOOLCHAIN_FILE"])
            print >>f, " Toolchain={}".format(config["CMAKE_TOOLCHAIN_FILE"])
        print      ""
        print >>f, ""
        f.flush()

        build_dir = "build"

        shutil.rmtree(build_dir, ignore_errors=True)

        os.mkdir(build_dir)

        #Copy the os environment
        new_env = {}
        new_env.update(os.environ)

        #Override CC and CXX
        if cc != None:
            new_env['CC'] = cc
        if cxx != None:
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
        build_cmd += targets

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
    if execname == None:
        #Nothing to find
        return True

    try:
        result = subprocess.check_output([execname, "--version"], stderr=subprocess.PIPE) == 0
    except OSError as e:
        return False
    return True

if __name__ == "__main__":
    main()
