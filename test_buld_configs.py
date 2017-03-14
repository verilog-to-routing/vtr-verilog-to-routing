#!/usr/bin/env python
import argparse
import subprocess
import sys
import os
import shutil
from collections import OrderedDict

DEFAULT_TARGETS_TO_BUILD=["vpr"]

GNU_COMPILER_VERSIONS=["4.9", "5", "6"]
CLANG_COMPILER_VERSIONS=[]
#CLANG_COMPILER_VERSIONS=["3.6", "3.8"]

EASYGL_CONFIGS = ["OFF", "ON"]
VTR_ASSERT_LEVELS= ["0", "1", "2", "3"]
TATUM_PARALLEL_CONFIGS = ["ON", "OFF"]

def parse_args():
    parser = argparse.ArgumentParser(description="Test building the specified targets with multiple compilers and build configurations")

    parser.add_argument("targets", 
                        nargs="*",
                        default=DEFAULT_TARGETS_TO_BUILD,
                        help="What targets to build (default: {})".format(DEFAULT_TARGETS_TO_BUILD))
    parser.add_argument("-j", 
                        type=int,
                        default=1,
                        help="How many parallel build jobs to allow (passed to make)")

    return parser.parse_args()

def main():

    args = parse_args();

    compilers_to_test = []

    for gnu_version in GNU_COMPILER_VERSIONS:
        cc = "gcc-" + gnu_version
        cxx = "g++-" + gnu_version
        compilers_to_test.append((cc, cxx))

    for clang_version in CLANG_COMPILER_VERSIONS:
        cc = "clang-" + gnu_version
        cxx = "clang++-" + gnu_version
        compilers_to_test.append((cc, cxx))

    #Test all the compilers with the default build config
    num_failed = 0
    num_configs = 0
    for cc, cxx in compilers_to_test:
        for easygl_config in EASYGL_CONFIGS: 
            for tatum_parallel_config in TATUM_PARALLEL_CONFIGS: 
                for vtr_assert_level in VTR_ASSERT_LEVELS: 
                    num_configs += 1
                    config = OrderedDict()
                    config["EASYGL_ENABLE_GRAPHICS"] = easygl_config
                    config["TATUM_ENABLE_PARALLEL_ANALYSIS"] = tatum_parallel_config
                    config["VTR_ASSERT_LEVEL"] = vtr_assert_level

                    success = build_config(args, cc, cxx, config)

                    if not success:
                        num_failed += 1

    if num_failed != 0:
        print "Failed to build {} of {} configurations".format(num_failed, num_configs)

    sys.exit(num_failed)

def build_config(args, cc, cxx, config):
    print "Building with CC={} CXX={}:".format(cc, cxx)

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
    subprocess.check_output(cmake_cmd, cwd=build_dir, env=new_env)

    #Run Make
    build_cmd = ["make"]
    build_cmd += ["-j", "{}".format(args.j)]
    build_cmd += args.targets

    try:
        print "  " + ' '.join(build_cmd)
        output = subprocess.check_output(build_cmd, env=new_env)
    except subprocess.CalledProcessError as e:
        print "\tERROR"
        return False

    print "\tOK"
    return True

if __name__ == "__main__":
    main()
