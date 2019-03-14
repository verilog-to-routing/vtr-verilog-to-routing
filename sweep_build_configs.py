#!/usr/bin/env python
import argparse
import subprocess
import sys
import os
import re
import shutil
import time
from collections import OrderedDict
from copy import deepcopy

DEFAULT_TARGETS_TO_BUILD=["all"]

DEFAULT_GNU_COMPILER_VERSIONS=["8", "7", "6", "5", "4.9"]
DEFAULT_MINGW_COMPILER_VERSIONS=["5"]
DEFAULT_CLANG_COMPILER_VERSIONS=["3.8", "3.6"]

DEFAULT_BUILD_CONFIGS = ["release", "debug", "release_pgo"]
DEFAULT_EASYGL_CONFIGS = ["ON", "OFF"]
DEFAULT_TATUM_EXECUTION_ENGINE_CONFIGS = ["auto", "serial"]
DEFAULT_VTR_ASSERT_LEVELS = ["4", "3", "2", "1", "0"]
DEFAULT_BLIF_EXPLORER_CONFIGS = ["ON", "OFF"]

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

    parser.add_argument("-n", "--dry_run",
                        action="store_true",
                        default=False,
                        help="Print what would happen instead of actually invoking the operations")

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
    config_args.add_argument("--build_types", 
                        nargs="*",
                        default=DEFAULT_BUILD_CONFIGS,
                        metavar="BUILD_TYPE",
                        help="What build types to test (default: %(default)s)")
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
    config_args.add_argument("--blif_explorer_configs", 
                        nargs="*",
                        default=DEFAULT_BLIF_EXPLORER_CONFIGS,
                        metavar="BLIF_EXPLORER_CONFIG",
                        help="What BlifExplorer configurations to test (default: %(default)s)")

    return parser.parse_args()

def main():

    args = parse_args();

    compiler_configs = []

    for gnu_version in args.gnu_versions:
        cc = "gcc-" + gnu_version
        cxx = "g++-" + gnu_version

        config = OrderedDict()
        config['CMAKE_PARAMS'] = OrderedDict() 
        config['CC'] = cc
        config['CXX'] = cxx
        compiler_configs.append(config)

    for clang_version in args.clang_versions:
        cc = "clang-" + clang_version
        cxx = "clang++-" + clang_version

        config = OrderedDict()
        config['CMAKE_PARAMS'] = OrderedDict() 
        config['CC'] = cc
        config['CXX'] = cxx
        compiler_configs.append(config)

    for mingw_version in args.mingw_versions:
        config = OrderedDict() 
        config['CMAKE_PARAMS'] = OrderedDict() 
        config['CMAKE_PARAMS']["CMAKE_TOOLCHAIN_FILE"] = MINGW_TOOLCHAIN_FILE
        if mingw_version != "":
            prefix = "x86_64-w64-mingw32"
            config["COMPILER_PREFIX"] = prefix
            config["MINGW_RC"] = "{}-windres".format(prefix)
            config["MINGW_CC"] = "{}-gcc-{}".format(prefix, mingw_version)
            config["MINGW_CXX"] = "{}-g++-{}".format(prefix, mingw_version)
        else:
            pass #Use defaults
        compiler_configs.append(config)

    assert len(compiler_configs) > 0, "Must have compilers to test"

    #Take the first compiler config as the default
    default_compiler_config = compiler_configs[0]

    #The actual test compilation configurations
    test_configs = []

    #Add a default configuration test for each compiler
    for compiler_config in compiler_configs:
        test_configs.append(deepcopy(compiler_config))

    #For the default compiler, check each build type
    for build_type in args.build_types:
        config = deepcopy(default_compiler_config)
        config["BUILD_TYPE"] = build_type
        test_configs.append(config)

    #For the default compiler, check all the different build configurations
    #
    #Note that for configurations which are expected to interact (e.g. features
    #within a single tool) it may be better to test the various combinations to
    #test for bugs in non-standard configurations
    for vtr_assert_level in args.vtr_assert_levels: 
        config = deepcopy(default_compiler_config)
        config["CMAKE_PARAMS"]["VTR_ASSERT_LEVEL"] = vtr_assert_level

        test_configs.append(config)

    #EasyGL is a library, so don't expect any interactions and can test seperately
    for easygl_config in args.easygl_configs: 
        config = deepcopy(default_compiler_config)
        config["CMAKE_PARAMS"]["EASYGL_ENABLE_GRAPHICS"] = easygl_config

        test_configs.append(config)

    #Tatum is a library, so don't expect any interactions and can test seperately
    for tatum_execution_engine_config in args.tatum_execution_engine_configs: 
        config = deepcopy(default_compiler_config)
        config["CMAKE_PARAMS"]["TATUM_EXECUTION_ENGINE"] = tatum_execution_engine_config

        test_configs.append(config)

    #Check that BlifExplorer builds with the default configuration
    for blif_explorer_config in args.blif_explorer_configs:
        config = deepcopy(default_compiler_config)
        config["CMAKE_PARAMS"]["WITH_BLIFEXPLORER"] = blif_explorer_config

        test_configs.append(config)

    #Test all the build configs
    num_failed = 0
    for config in test_configs:
        targets = args.targets
        if 'CMAKE_TOOLCHAIN_FILE' in config['CMAKE_PARAMS'] and targets == DEFAULT_TARGETS_TO_BUILD:
            #Only build VPR with MINGW by default
            # The updated version of ABC (and ace/odin which depend on ABC)
            # fail to compile with MINGW
            targets = ['vpr']

        success = build_config(args, targets, config)

        if not success:
            num_failed += 1

            if args.exit_on_failure:
                sys.exit(num_failed)

    if num_failed != 0:
        print "Failed to build {} of {} configurations".format(num_failed, len(test_configs))
    else:
        print "Successfully built {} configurations".format(len(test_configs))

    sys.exit(num_failed)

def build_config(args, targets, config):
    start_time = time.time()
    cc = None
    cxx = None
    build_type = None
    cmake_params = []

    #Set cc/cxx seperatley from the rest of the config parameters,
    #CC/CXX are set seperately in the command environment, rathern than
    #as command-line options
    if 'CC' in config:
        cc = config['CC']
    if 'CXX' in config:
        cxx = config['CXX']
    if 'BUILD_TYPE' in config:
        build_type = config['BUILD_TYPE']

    if 'CMAKE_PARAMS' in config:
        for key, value in config['CMAKE_PARAMS'].iteritems():
            cmake_params.append("{}={}".format(key, value))

    if not compiler_is_found(cc):
        print "Failed to find C compiler {}, skipping".format(cc)
        return False

    if not compiler_is_found(cxx):
        print "Failed to find C++ compiler {}, skipping".format(cxx)
        return False

    log_file = "build.log"

    build_successful = True
    with open(log_file, 'w') as f:
        print      "Building with",
        print >>f, "Building with",
        if cc != None:
            print      " CC={}".format(cc),
            print >>f, " CC={}".format(cc),
        if cxx != None:
            print      " CXX={}:".format(cxx),
            print >>f, " CXX={}:".format(cxx),
        if "CMAKE_TOOLCHAIN_FILE" in config['CMAKE_PARAMS']:
            print      " Toolchain={}".format(config['CMAKE_PARAMS']["CMAKE_TOOLCHAIN_FILE"]),
            print >>f, " Toolchain={}".format(config['CMAKE_PARAMS']["CMAKE_TOOLCHAIN_FILE"]),
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

        #Use the Makefile wrapper
        build_cmd = ["make"]
        build_cmd += ["BUILD_DIR={}".format(build_dir)]
        if build_type:
            build_cmd += ["BUILD_TYPE={}".format(build_type)]
        if len(cmake_params) > 0:
            build_cmd += ["CMAKE_PARAMS='{}'".format(" ".join(cmake_params))]
        build_cmd += ["-j", "{}".format(args.j)]
        build_cmd += targets

        print "  " + ' '.join(build_cmd)
        print >>f, "  " + ' '.join(build_cmd)
        f.flush()
        if not args.dry_run:
            try:
                subprocess.check_call(build_cmd, stdout=f, stderr=f, env=new_env)
            except subprocess.CalledProcessError as e:
                build_successful = False

    #Look for errors and warnings in the build log
    issue_count = 0
    with open(log_file) as f:
        for line in f:
            if is_valid_warning_error(line):
                print "    " + line,
                issue_count += 1

    elapsed_time = time.time() - start_time
    if not build_successful:
        print "  ERROR: failed to compile"
    else:
        print "  OK",
        if issue_count > 0:
            print " ({} warnings)".format(issue_count),
        print
    print "  Took {:.2f} seconds".format(elapsed_time)
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
