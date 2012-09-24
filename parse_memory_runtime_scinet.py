#!/usr/bin/python

try: import pudb
except ImportError: pass

import os
import re

actions = ['quartus_synthesis', 'vpr_pack', 'vpr_place', 'vpr_route']

def main():
    benchmarks = []
    pwd = os.getcwd()
    for job_dir in os.listdir(pwd):
        for file_name in os.listdir(os.path.join(job_dir, 'output')):
            if file_name == 'top.log':
                pass
            else:
                benchmarks.append((job_dir, file_name))

    for job_dir, benchmark in benchmarks:
        print
        print job_dir
        print "  " + benchmark
        for action in actions:
            time = 0
            mem = 0
            action_completed_successfully = False

            log_file = os.path.join(job_dir, "output/%s/%s-%s.log" % (benchmark, benchmark, action))
            if action == 'quartus_synthesis':
                (time, mem) = parse_quartus_synthesis(log_file)
                action_completed_successfully = check_quartus_synthesis_success(job_dir, benchmark, action)
            else:
                (time, mem) = parse_vpr(job_dir, log_file, benchmark, action)
                action_completed_successfully = check_vpr_success(job_dir, benchmark, action)


            #Time to the nearest 100 sec
            time = time - (time % 100) + 100

            #Memory to the nearest 100 MB
            mem = mem - (mem % 100) + 100
            if action_completed_successfully:
                print "  \t%s %s sec, %s MB" % (action, time, mem)
            else:
                print "  \tINVALID: %s %s sec, %s MB" % (action, time, mem)


def parse_quartus_synthesis(log_file):
    peak_memory = 0
    total_time = 0

    memory_usage_regex = re.compile(r"\s*Info: Peak virtual memory:\s+(?P<memory_MB>\S+)\s+megabytes")
    total_time_regex = re.compile(r"\s*INFO: Ended\s+'vqm2blif_flow.py script' after\s+(?P<walltime_sec>\S+)\s+sec")

    with open(log_file, 'r') as f:
        for line in f:
            result = memory_usage_regex.match(line)
            if result:
                peak_memory = max(peak_memory, int(float(result.group('memory_MB'))))
                continue
            result = total_time_regex.match(line)
            if result:
                total_time = int(round(float(result.group('walltime_sec'))))
                continue

    return (total_time, peak_memory)

def parse_vpr(job_dir, log_file, benchmark, action):
    peak_memory = 0
    total_time = 0

    total_time_regex = re.compile(r"The entire flow of VPR took\s+(?P<walltime_sec>\S+)\s+seconds")
    try:
        with open(log_file, 'r') as f:
            for line in f:
                result = total_time_regex.match(line)
                if result:
                    total_time = int(round(float(result.group('walltime_sec'))))
                    continue
    except IOError as e:
        print "Error: opening {log_file}".format(log_file=log_file) 

    command_regex = re.compile(r".*vpr.*{benchmark_name}.*{action}.*".format(benchmark_name=benchmark, action=action.split('_')[1]))

    peak_memory = parse_top_log(job_dir, command_regex)

    return (total_time, peak_memory)

def parse_top_log(job_dir, command_regex):
    peak_memory = 0 

    top_regex = re.compile(r"\s*(?P<PID>\S+)\s+(?P<USER>\S+)\s+(?P<PR>\S+)\s+(?P<NI>\S+)\s+(?P<VIRT>\S+)\s+(?P<RES>\S+)\s+(?P<SHR>\S+)\s+(?P<S>\S+)\s+(?P<CPU>\S+)\s+(?P<MEM>\S+)\s+(?P<TIME>\S+)\s+(?P<COMMAND>[\s\S]+)")

    with open(os.path.join(job_dir, 'output/top.log'), 'r') as f:
        for line in f:
            result = top_regex.match(line)
            if result:
                command = result.group('COMMAND')
                if command_regex.match(command):
                    peak_memory = max(peak_memory, convert_memory_usage_to_MB(result.group('RES')))
    return peak_memory
    
def convert_memory_usage_to_MB(string):
    if string[-1] == 'k':
        #Force to minimum 1MB
        memory_in_MB = 1

    elif string[-1] == 'm':
        memory_in_MB = int(round(float(string[0:-1])))

    elif string[-1] == 'g':
        memory_in_MB = int(round(float(string[0:-1])*1024))

    else:
        #IN Bytes
        #Force to minimum 1MB
        memory_in_MB = 1

    return memory_in_MB

def check_quartus_synthesis_success(job_dir, benchmark, action):
    found_vqm = False
    found_blif = False
    for file in os.listdir(os.path.join(job_dir, 'output/%s' % benchmark)):
        file_name, file_extension = os.path.splitext(file)
        if file_extension == '.vqm':
            found_vqm = True
        if file_extension == '.blif':
            found_blif = True

    if not found_vqm:
        print "{action} Failed (VQM generation)".format(action=action)
    if not found_blif:
        print "{action} Failed (BLIF generation)".format(action=action)

    return found_vqm and found_blif

def check_vpr_success(job_dir, benchmark, action):
    found_result_file = False
    
    file_extension_to_find = ''

    if action == 'vpr_pack':
        file_extension_to_find = '.net'

    elif action == 'vpr_place':
        file_extension_to_find = '.place'

    elif action == 'vpr_route':
        file_extension_to_find = '.route'
    else:
        print "Error: unrecognized action in check_vpr_success"
        sys.exit(1)

    for file in os.listdir(os.path.join(job_dir, 'output/%s' % benchmark)):
        file_name, file_extension = os.path.splitext(file)
        if file_extension == file_extension_to_find:
            found_result_file = True

    if not found_result_file:
        print "\t{action} Failed".format(benchmark=benchmark, action=action)

    return found_result_file


if __name__ == '__main__':
    main()
