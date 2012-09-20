#!/usr/bin/python

import os
import re

actions = ['quartus_synthesis', 'vpr_pack', 'vpr_place', 'vpr_route']

def main():
    benchmarks = []
    for file_name in os.listdir('output'):
        if file_name == 'top.log':
            pass
        else:
            benchmarks.append(file_name)

    for benchmark in benchmarks:
        print benchmark
        for action in actions:
            time = 0
            mem = 0

            log_file = "output/%s/%s-%s.log" % (benchmark, benchmark, action)
            if action == 'quartus_synthesis':
                (time, mem) = parse_quartus_synthesis(log_file)
            else:
                (time, mem) = parse_vpr(log_file, benchmark, action)
                #pass


            #Time to the nearest 100 sec
            time = time - (time % 100) + 100

            #Memory to the nearest 100 MB
            mem = mem - (mem % 100) + 100

            print "\t%s %s MB, %s sec" % (action, mem, time)

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

def parse_vpr(log_file, benchmark, action):
    peak_memory = 0
    total_time = 0

    total_time_regex = re.compile(r"The entire flow of VPR took\s+(?P<walltime_sec>\S+)\s+seconds")

    with open(log_file, 'r') as f:
        for line in f:
            result = total_time_regex.match(line)
            if result:
                total_time = int(round(float(result.group('walltime_sec'))))
                continue

    command_regex = re.compile(r".*vpr.*%s.*%s.*" % (benchmark, action.split('_')[1]))

    peak_memory = parse_top_log(command_regex)

    return (total_time, peak_memory)

def parse_top_log(command_regex):
    peak_memory = 0 

    top_regex = re.compile(r"\s*(?P<PID>\S+)\s+(?P<USER>\S+)\s+(?P<PR>\S+)\s+(?P<NI>\S+)\s+(?P<VIRT>\S+)\s+(?P<RES>\S+)\s+(?P<SHR>\S+)\s+(?P<S>\S+)\s+(?P<CPU>\S+)\s+(?P<MEM>\S+)\s+(?P<TIME>\S+)\s+(?P<COMMAND>[\s\S]+)")

    with open('output/top.log', 'r') as f:
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

if __name__ == '__main__':
    main()
