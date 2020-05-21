#!/usr/bin/env python3.6

import subprocess
import pathlib
import sys
import shutil
import filecmp

# VTR paths
scripts_path = pathlib.Path(__file__).resolve().parent.parent
vtr_flow_path = scripts_path.parent
temp_dir = pathlib.Path.cwd() / "temp"

# run_vtr_flow.pl will be compared against vtr-flow.py using the following test cases as arguments:
arg_list = ["benchmarks/verilog/ch_intrinsics.v arch/timing/k6_N10_mem32K_40nm.xml -track_memory_usage --router_heap binary --min_route_chan_width_hint 38",
            "benchmarks/verilog/diffeq1.v arch/timing/k6_N10_mem32K_40nm.xml -track_memory_usage --router_heap bucket --min_route_chan_width_hint 46"
            ]

# The following output files will be compared to ensure they are identical (only one file must match each entry)
# The first list item is the glob pattern, the second is the number of lines to ignore at the beginning of the file
files_to_validate = [   ("*.route", 1),
                        ("report_timing.setup.rpt",0)
                    ]

def error(*msg, return_code=-1):
    print("!!!! ERROR:", " ".join(str(item) for item in msg))
    sys.exit(return_code)

# Find a single file matching a pattern at a given path
def find_file_matching(path, pattern):
    matches = [f for f in path.glob(pattern)]
    if len(matches) == 0:
        error("No files match", path / pattern)
    elif len(matches) > 1:
        error("Multiple files match", path / pattern)
    return matches[0]

# Run 
def run_vtr_flow(script_name, run_name, temp_dir_run, args):
    cmd = [scripts_path / script_name, *(args.split()), "-temp_dir",  temp_dir_run]
    print("  Running", run_name,"flow:", cmd)
    p = subprocess.run(cmd, cwd = vtr_flow_path, stdout = subprocess.PIPE, stderr=subprocess.STDOUT)
    if (p.returncode):
        print(p.stdout.decode())
        error("Run VTR failed")

# Remove lines from start of a file
def remove_file_lines_head(path, num_lines):
    with open(path, 'r') as fin:
        data = fin.read().splitlines(True)
    with open(path, 'w') as fout:
        fout.writelines(data[num_lines:])

def main():

    for test_str in arg_list:
        # Delete all files in temp directory
        if temp_dir.is_dir():
            shutil.rmtree(temp_dir)
        temp_dir.mkdir()

        print("Testing", test_str)
        temp_dir_pl = temp_dir / "pl"
        temp_dir_py = temp_dir / "py"
        
        # Run with old and new flows
        run_vtr_flow("run_vtr_flow.pl", "old pl", temp_dir_pl, test_str)
        run_vtr_flow("vtr-flow.py", "new py", temp_dir_py, test_str)
       
        # Check that output files match
        for (pattern, skip_lines) in files_to_validate:
            pl_file = find_file_matching(temp_dir_pl, pattern)
            remove_file_lines_head(pl_file, skip_lines)
            py_file = find_file_matching(temp_dir_py, pattern)
            remove_file_lines_head(py_file, skip_lines)
         
            if not filecmp.cmp(pl_file, py_file):
                error(pl_file, py_file, "differ")
            else:
                print(" ", pl_file, py_file, "match")
           
if __name__ == "__main__":
    main()