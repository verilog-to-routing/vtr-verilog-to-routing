#!/usr/bin/env python3

"""  Run the vtr_reg_basic regression test """

import subprocess

subprocess.run(["./run_reg_test.py", "vtr_reg_basic"], check=True)
