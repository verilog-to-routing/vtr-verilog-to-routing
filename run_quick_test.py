#!/usr/bin/env python3

"""  Run the vtr_reg_basic regression test """

import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("-j", "--jobs", type=int, default=1, help="Number of processes to use")
args = parser.parse_args()

subprocess.run(["./run_reg_test.py", "vtr_reg_basic", "-j", str(args.jobs)], check=True)
