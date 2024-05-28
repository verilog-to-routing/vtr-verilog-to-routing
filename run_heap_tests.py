import subprocess
import os
import time
import re


def extract_run_number(run):
    if run is None:
        return None
    match = re.match(r"run(\d+)", run)
    if match:
        return int(match.group(1))
    else:
        return None


def get_run_num(benchmarks_name):
    runs = os.listdir(
        "/home/shrevena/Documents/vtr/vtr-verilog-to-routing/fine-grained-parallel-router/testing/" + benchmarks_name)
    runs.remove("config")
    last_run_num = 0
    if len(runs) != 0:
        last_run = sorted(runs)[-1]
        last_run_num = extract_run_number(last_run)
        if last_run_num is None:
            last_run_num = 0
    return last_run_num + 1


def write_test_info(benchmarks_name, settings):
    run_info_file = open("heap_tests_run_info.txt", "a")
    run_name = "run{:03d}".format(get_run_num(benchmarks_name))
    run_info_file.write(benchmarks_name + "/" + run_name + " using settings:\n")
    for line in settings:
        run_info_file.write(f"{line}")
    run_info_file.write("\n---------------------------------------\n")


def make_and_run(settings):
    # make vpr
    command = "make CMAKE_PARAMS=\"-DVTR_ENABLE_PROFILING=TRUE -DVTR_IPO_BUILD=on\" -j10 vpr"
    print("Running command:", command)
    make_process = subprocess.Popen(f"{command}", shell=True, stdout=True, stderr=True)
    print("PID:", make_process.pid)
    os.waitpid(make_process.pid, 0)

    run_processes = []

    # run koios_medium benchmarks
    command = "/home/shrevena/Documents/vtr/vtr-verilog-to-routing/fine-grained-parallel-router/testing/run_test.py koios_medium -j1 -vtr_dir /home/shrevena/Documents/vtr/vtr-verilog-to-routing | tee -a terminal_out.txt"
    for i in range(3):
        print("Running command:", command)
        write_test_info("koios_medium", settings)
        run_processes.append(subprocess.Popen(f"{command}", shell=True, stdout=True, stderr=True))
        print("PID:", run_processes[-1].pid)
        time.sleep(5)

    # run titan benchmarks
    command = "/home/shrevena/Documents/vtr/vtr-verilog-to-routing/fine-grained-parallel-router/testing/run_test.py bwave_like -j1 -vtr_dir /home/shrevena/Documents/vtr/vtr-verilog-to-routing | tee -a terminal_out.txt"
    for i in range(3):
        print("Running command:", command)
        write_test_info("bwave_like", settings)
        run_processes.append(subprocess.Popen(f"{command}", shell=True, stdout=True, stderr=True))
        print("PID:", run_processes[-1].pid)
        time.sleep(5)

    # wait for all tests to complete
    for process in run_processes:
        os.waitpid(process.pid, 0)


with open('vpr/src/route/binary_heap.h', 'r') as binary_heap_header:
    data = binary_heap_header.readlines()

# NO 1,9
data[4:9] = ['#define HEAP_DARITY_2\n', '//#define HEAP_DARITY_4\n', '\n', '//#define HEAP_USE_HEAP_ELEM\n',
             '//#define HEAP_USE_MEMORY_ALIGNMENT\n']

with open('vpr/src/route/binary_heap.h', 'w') as binary_heap_header:
    binary_heap_header.writelines(data)

make_and_run(data[4:9])

# NO 2,10
data[4:9] = ['#define HEAP_DARITY_2\n', '//#define HEAP_DARITY_4\n', '\n', '//#define HEAP_USE_HEAP_ELEM\n',
             '#define HEAP_USE_MEMORY_ALIGNMENT\n']

with open('vpr/src/route/binary_heap.h', 'w') as binary_heap_header:
    binary_heap_header.writelines(data)

make_and_run(data[4:9])

# NO 3,11
data[4:9] = ['#define HEAP_DARITY_2\n', '//#define HEAP_DARITY_4\n', '\n', '#define HEAP_USE_HEAP_ELEM\n',
             '//#define HEAP_USE_MEMORY_ALIGNMENT\n']

with open('vpr/src/route/binary_heap.h', 'w') as binary_heap_header:
    binary_heap_header.writelines(data)

make_and_run(data[4:9])

# NO 4,12
data[4:9] = ['#define HEAP_DARITY_2\n', '//#define HEAP_DARITY_4\n', '\n', '#define HEAP_USE_HEAP_ELEM\n',
             '#define HEAP_USE_MEMORY_ALIGNMENT\n']

with open('vpr/src/route/binary_heap.h', 'w') as binary_heap_header:
    binary_heap_header.writelines(data)

make_and_run(data[4:9])

# NO 5,13
data[4:9] = ['//#define HEAP_DARITY_2\n', '#define HEAP_DARITY_4\n', '\n', '//#define HEAP_USE_HEAP_ELEM\n',
             '//#define HEAP_USE_MEMORY_ALIGNMENT\n']

with open('vpr/src/route/binary_heap.h', 'w') as binary_heap_header:
    binary_heap_header.writelines(data)

make_and_run(data[4:9])

# NO 6,14
data[4:9] = ['//#define HEAP_DARITY_2\n', '#define HEAP_DARITY_4\n', '\n', '//#define HEAP_USE_HEAP_ELEM\n',
             '#define HEAP_USE_MEMORY_ALIGNMENT\n']

with open('vpr/src/route/binary_heap.h', 'w') as binary_heap_header:
    binary_heap_header.writelines(data)

make_and_run(data[4:9])

# NO 7,15
data[4:9] = ['//#define HEAP_DARITY_2\n', '#define HEAP_DARITY_4\n', '\n', '#define HEAP_USE_HEAP_ELEM\n',
             '//#define HEAP_USE_MEMORY_ALIGNMENT\n']

with open('vpr/src/route/binary_heap.h', 'w') as binary_heap_header:
    binary_heap_header.writelines(data)

make_and_run(data[4:9])

# NO 8,16
data[4:9] = ['//#define HEAP_DARITY_2\n', '#define HEAP_DARITY_4\n', '\n', '#define HEAP_USE_HEAP_ELEM\n',
             '#define HEAP_USE_MEMORY_ALIGNMENT\n']

with open('vpr/src/route/binary_heap.h', 'w') as binary_heap_header:
    binary_heap_header.writelines(data)

make_and_run(data[4:9])
