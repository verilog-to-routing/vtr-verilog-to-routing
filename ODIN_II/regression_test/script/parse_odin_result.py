#!/usr/bin/env python
from __future__ import division
import sys, os

current_line = ""

def time_to_millis(time_in):
    time_in.strip()
    time_in.replace(' ', '')
    
    result = 'NaN'


    if 'ms'	in time_in:
        result = str(assert_float(time_in.replace('ms','')))

    elif 's' in time_in:
        result = str(assert_float(time_in.replace('s','')) * 1000.0)

    elif 'm' in time_in:
        result = str(assert_float(time_in.replace('m','')) * 60.0 * 1000.0)

    elif 'h' in time_in:
        result = str(assert_float(time_in.replace('h','')) * 60.0 * 60.0 * 1000.0)

    
    return result

def size_to_MiB(size_in):
    size_in = format_token(size_in)

    base_size = 1.0
    if 'b' in size_in:
        base_size = 8.0

    size_in = size_in.replace('i','')
    size_in = size_in.replace('I','')
    size_in = size_in.replace('b','')
    size_in = size_in.replace('B','')

    if 'G' in size_in or 'g' in size_in:
        size_in = str(assert_float(size_in.replace('G','').replace('g','')) / 1024.0 / 1024.0 / base_size)

    elif 'M' in size_in or 'm' in size_in:
        size_in = str(assert_float(size_in.replace('M','').replace('m','')) / 1024.0 / base_size)

    elif 'K' in size_in or 'k' in size_in:
        size_in = str(assert_float(size_in.replace('K','').replace('k','')) / 1024.0 / base_size)

    #we are now in byte
    return str(assert_float(size_in) * 1024.0 * 1024.0)

def contains(line_in, on_tokens):
    for token in on_tokens:
        if token not in line_in:
            return False

    return True

def assert_float(input_str):
    try:
        return float(input_str)
    except ValueError:
        global current_line
        print("Error, line: " + current_line)
        exit(1);

    return str(flt)

def format_token(input_str):
    return input_str.strip().replace(' ', '').replace('(','').replace(')','').replace('%','').replace(',','')

# 1 indexed
def get_token(line_in, index):
    return str(line_in.split(" ")[index-1])

def stringify(value_map, key):
    if key not in value_map:
        return str(float("NaN"))
    else:
        return str(value_map[key])

def get_all_value(value_map, key_list):
    return_str = ""
    for key in key_list:
        return_str += stringify(value_map, key) + ","

    # lose the last comma
    return return_str

def get_all_key(value_map, key_list):
    return_str = ""
    for key in key_list:
        return_str += key + ","

    return return_str

def insert_string(value_map, key, input_str):
    value_map[key] = format_token(input_str)

def insert_decimal(value_map, key, input_str):
    value_map[key] = str(assert_float(format_token(input_str)))

def parse_line(benchmarks, line):

    line.strip()
    line = " ".join(line.split())
    
    global current_line
    current_line = line

    if contains(line, {"Executing simulation with", "threads"}):
        insert_decimal(benchmarks, "max_thread_count", get_token(line,8))

    elif contains(line, {"Simulating", "vectors"}):
        insert_decimal(benchmarks, "number_of_vectors", get_token(line,2))	

    elif contains(line, {"Nodes:"}):
        insert_decimal(benchmarks, "number_of_nodes", get_token(line,2))	
        
    elif contains(line, {"Connections:"}):
        insert_decimal(benchmarks, "number_of_connections", get_token(line,2))

    elif contains(line, {"Threads:"}):
        insert_decimal(benchmarks, "used_threads", get_token(line,2))	

    elif contains(line, {"Degree:"}):
        insert_decimal(benchmarks, "degree", get_token(line,2))

    elif contains(line, {"Stages:"}):
        insert_decimal(benchmarks, "number_of_stages", get_token(line,2))

    elif contains(line, {"Simulation time:"}):
        insert_decimal(benchmarks, "simulation_time", time_to_millis(get_token(line,3)))

    elif contains(line, {"Elapsed time:"}):
        insert_decimal(benchmarks, "elapsed_time", time_to_millis(get_token(line,3)))

    elif contains(line, {"Coverage:"}):
        insert_decimal(benchmarks, "percent_coverage", get_token(line,3))

    elif contains(line, {"Odin ran with exit status:"}):
        insert_decimal(benchmarks, "exit_code", get_token(line,6))

    elif contains(line, {"Odin II took", "seconds", "(max_rss"}):
        insert_decimal(benchmarks, "total_time", time_to_millis(get_token(line,4) + 's'))
        insert_decimal(benchmarks, "max_rss", size_to_MiB(get_token(line,7) + get_token(line,8)))

    elif contains(line, {"context-switches #"}):
        insert_decimal(benchmarks, "context_switches", get_token(line,1))

    elif contains(line, {"cpu-migrations #"}):
        insert_decimal(benchmarks, "cpu_migration", get_token(line,1))

    elif contains(line, {"page-faults #"}):
        insert_decimal(benchmarks, "page_faults", get_token(line,1))

    elif contains(line, {"stalled-cycles-frontend #"}):
        insert_decimal(benchmarks, "stalled_cycle_frontend", get_token(line,1))

    elif contains(line, {"stalled-cycles-backend #"}):
        insert_decimal(benchmarks, "stalled_cycle_backend", get_token(line,1))

    elif contains(line, {"cycles #"}):
        insert_decimal(benchmarks, "cycles", get_token(line,1))

    elif contains(line, {"branches #"}):
        insert_decimal(benchmarks, "branches", get_token(line,1))

    elif contains(line, {"branch-misses #"}):
        insert_decimal(benchmarks, "branch_misses", get_token(line,1))

    elif contains(line, {"LLC-loads #"}):
        insert_decimal(benchmarks, "llc_loads", get_token(line,1))

    elif contains(line, {"LLC-load-misses #"}):
        insert_decimal(benchmarks, "llc_load_miss", get_token(line,1))

    elif contains(line, {"CPU:"}):
        insert_decimal(benchmarks, "percent_cpu_usage", get_token(line,2))

    elif contains(line, {"Minor PF:"}):
        insert_decimal(benchmarks, "minor_page_faults", get_token(line,3))


def main():
    benchmarks = {}

    key_list = [
        "max_thread_count"
        , "number_of_vectors"
        , "number_of_nodes"
        , "number_of_connections"
        , "used_threads"
        , "degree"
        , "number_of_stages"
        , "simulation_time"
        , "elapsed_time"
        , "percent_coverage"
        , "exit_code"
        , "total_time"
        , "max_rss"
        , "context_switches"
        , "cpu_migration"
        , "page_faults"
        , "stalled_cycle_frontend"
        , "stalled_cycle_backend"
        , "cycles"
        , "branches"
        , "branch_misses"
        , "llc_loads"
        , "llc_load_miss"
        , "percent_cpu_usage"
        , "minor_page_faults"
    ]

    if len(sys.argv) < 4:
        print("Wrong number of argument, expecting ./exec <input.log> <output.csv> <... (header value pair)>")
        exit(-1)

    log_file_to_parse = sys.argv[1]
    output_file = sys.argv[2]

    fileContext = open(log_file_to_parse, "r")

    for wholeLine in fileContext:
        parse_line(benchmarks, wholeLine)

    f = open(output_file,"w+")
    
    header_in = ""
    values_in = ""
    i = 0
    while i < (len(sys.argv)-3):
        header_in += sys.argv[i+3] + ","
        values_in += sys.argv[i+4] + ","
        i += 2

    header_in += get_all_key(benchmarks, key_list) + "\n"
    values_in += get_all_value(benchmarks, key_list) + "\n"

    f.write(header_in)
    f.write(values_in)
    f.close() 

    exit(0)


if __name__ == "__main__":
    main()