#!/usr/bin/env python3

import os
import sys
import argparse
import tempfile
import tarfile
import glob
import urllib.request
import math
import subprocess
import shutil

TATUM_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


tests = {
    'basic': [os.path.join(TATUM_ROOT, 'test', 'basic')],
    'mcnc20': ["http://www.eecg.utoronto.ca/~kmurray/tatum/golden_results/mcnc20_tatum_golden.tar.gz"],
    #'vtr': [

        #],
    #'titan_other': [

        #],
    #'titan': [

        #],
    #'tau15_unit_delay': [

        #],
}

def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("test_names",
                        help="Test to run",
                        nargs='+',
                        choices=tests.keys())

    parser.add_argument("--tatum_test_exec",
                        help="Path to tatum test executable (Default: from path)",
                        default=None)

    parser.add_argument("--tatum_nworkers",
                        help="How many parallel workers each tatum invokation should use. (Default: tatum default)",
                        type=int,
                        default=None)

    parser.add_argument("--tatum_nserial",
                        help="How serial runs tatum should perform per test. (Default: %(default)s)",
                        type=int,
                        default=3)

    parser.add_argument("--tatum_nparallel",
                        help="How parallel runs tatum should perform per test. (Default: %(default)s)",
                        type=int,
                        default=6)

    parser.add_argument("--debug",
                        help="Show directory path and don't clean-up",
                        default=False,
                        action='store_true')

    return parser.parse_args()

def main():
    args = parse_args()

    if args.tatum_test_exec:
        args.tatum_test_exec = os.path.abspath(args.tatum_test_exec)
    else:
        args.tatum_test_exec = "tatum_test" #From path

    num_failures = 0
    for test_name in args.test_names:
        for test_url in tests[test_name]:
            work_dir = tempfile.mkdtemp(prefix="tatum_reg_test")
            try:
                test_files = download_extract_test(args, work_dir, test_url)
                num_failures += run_test(args, work_dir, test_name, test_files)
            finally:
                if not args.debug:
                    shutil.rmtree(work_dir)
                else:
                    print(work_dir)

    sys.exit(num_failures)

def run_test(args, work_dir, test_name, test_files):
    num_failed = 0

    os.chdir(work_dir)

    print("Running {} ({} tests)".format(test_name, len(test_files)))
    for test_file in test_files:

        num_failed += run_single_test(args, work_dir, test_file)

    return num_failed;

def run_single_test(args, work_dir, test_file):
    num_failed = 0

    benchmark_name, ext = os.path.splitext(os.path.basename(test_file))

    log = benchmark_name + '.log'

    cmd = []
    cmd += [args.tatum_test_exec]
    cmd += ['--verify', "1"]

    cmd += ['--num_serial', str(args.tatum_nserial)]
    cmd += ['--num_parallel', str(args.tatum_nparallel)]

    if args.tatum_nworkers:
        cmd += ['--num_workers', str(args.tatum_nworkers)]

    cmd += [test_file]

    print(" {}".format(test_file), end='')

    if args.debug:
        print()
        print(" cmd: {} log: {}".format(' '.join(cmd), log))

    with open(log, 'w') as outfile:
        retcode = subprocess.call(cmd, stdout=outfile, stderr=outfile)

    if retcode == 0:
        print(" PASSED")
    else:
        num_failed += 1
        print(" FAILED")

        #Print log if failed
        with open(log, 'r') as log_file:
            for line in log_file:
                print("\t{}".format(line), end='')

    return num_failed

def download_extract_test(args, work_dir, test_url):

    test_files = []

    if '.tar' in test_url:
        #A tar file of benchmark files
        benchmark_tar = os.path.join(work_dir, os.path.basename(test_url))
    
        get_url(test_url, benchmark_tar)

        with tarfile.TarFile.open(benchmark_tar, mode="r|*") as tar_file:
            tar_file.extractall(path=work_dir)

        test_files += glob.glob("{}/*.tatum".format(work_dir))
        test_files += glob.glob("{}/*/*.tatum".format(work_dir))
    else:
        #A directory of benchmark files

        test_files += glob.glob("{}/*.tatum".format(test_url))
        test_files += glob.glob("{}/*/*.tatum".format(test_url))

    return test_files

def get_url(url, filename):
    if '://' in url:
        download_url(url, filename)
    else:
        shutl.copytree(url, filename)

def download_url(url, filename):
    """
    Downloads the specifed url to filename
    """
    print("Downloading {} to {}".format(url, filename))
    urllib.request.urlretrieve(url, filename, reporthook=download_progress_callback)

def download_progress_callback(block_num, block_size, expected_size):
    """
    Callback for urllib.urlretrieve which prints a dot for every percent of a file downloaded
    """
    total_blocks = int(math.ceil(expected_size / block_size))
    progress_increment = int(math.ceil(total_blocks / 100))

    if block_num % progress_increment == 0:
        print(".", end='', flush=False)
    if block_num*block_size >= expected_size:
        print("")

if __name__ == "__main__":
    main()
