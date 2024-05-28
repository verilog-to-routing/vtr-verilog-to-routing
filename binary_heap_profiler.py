import os
import re
import subprocess


def extract_run_number(run):
    if run is None:
        return None
    match = re.match(r"run(\d+)", run)
    if match:
        return int(match.group(1))
    else:
        return None


def get_last_run_num(benchmarks_name):
    runs = os.listdir(
        "/home/shrevena/Documents/vtr/vtr-verilog-to-routing/fine-grained-parallel-router/testing/" + benchmarks_name)
    runs.remove("config")
    last_run_num = 0
    if len(runs) != 0:
        last_run = sorted(runs)[-1]
        last_run_num = extract_run_number(last_run)
        if last_run_num is None:
            last_run_num = 0
    return last_run_num


def create_dot_files(benchmark_name, folder_names):
    last_run = get_last_run_num(benchmark_name)

    for i in range(1, last_run + 1):
        run_name = "run{:03d}".format(i)

        for folder in folder_names:
            gmon_file = "/home/shrevena/Documents/vtr/vtr-verilog-to-routing/fine-grained-parallel-router/testing/" + benchmark_name + "/" + run_name + "/" + folder + "/gmon.out"
            gprof_file = "/home/shrevena/Documents/vtr/vtr-verilog-to-routing/fine-grained-parallel-router/testing/" + benchmark_name + "/" + run_name + "/" + folder + "/gprof.txt"
            dot_file = "/home/shrevena/Documents/vtr/vtr-verilog-to-routing/fine-grained-parallel-router/testing/" + benchmark_name + "/" + run_name + "/" + folder + "/vpr.dot"

            if os.path.isfile(dot_file):
                print(f"{dot_file} exists.")
                continue

            if not os.path.isfile(gmon_file):
                print(f"{gmon_file} does not exist.")
                continue

            gprof_command = f"gprof /home/shrevena/Documents/vtr/vtr-verilog-to-routing/vpr/vpr {gmon_file} > {gprof_file}"
            gprof_process = subprocess.Popen(f"{gprof_command}", shell=True, stdout=True, stderr=True)
            print("Running command:", gprof_command)
            print("PID:", gprof_process.pid)
            os.waitpid(gprof_process.pid, 0)

            dot_command = f"gprof2dot --strip --node-thres=0.0 --edge-thres=0.0 --node-label=total-time --node-label=total-time-percentage {gprof_file} > {dot_file}"
            dot_process = subprocess.Popen(f"{dot_command}", shell=True, stdout=True, stderr=True)
            print("Running command:", dot_command)
            print("PID:", dot_process.pid)
            os.waitpid(dot_process.pid, 0)


koios_medium_test_folders = ["k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/attention_layer.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/bnn.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/conv_layer.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/conv_layer_hls.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/dla_like.small.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/eltwise_layer.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/reduction_layer.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/robot_rl.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/softmax.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/spmv.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/tpu_like.small.os.pre-vpr.blif/common",
                             "k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/tpu_like.small.ws.pre-vpr.blif/common"
                             ]

bwave_like_test_folders = ["k6_frac_N10_frac_chain_mem32K_40nm.xml/bwave_like.float.large.blif/common"]

create_dot_files("koios_medium", koios_medium_test_folders)
create_dot_files("bwave_like", bwave_like_test_folders)
