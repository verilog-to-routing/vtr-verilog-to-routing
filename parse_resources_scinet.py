#!/scinet/gpc/tools/Python/Python272-shared/bin/python

try: import pudb
except ImportError: pass

import os
import sys
import re
import csv
import fnmatch
from collections import OrderedDict
from run_benchmarks_scinet import BenchmarkInfo

fieldnames =('Name', 'Total Blocks', 
             '# Input Pins', '# Output Pins', '# Bidir Pins',
             '# PLLs', '# LEs', '# ALUTs', '# REGs', '# DSP Elements', '# RAM Segments', '# RAM Bits', 
             '# BLIF blocks')


class BenchmarkResults(object):
    def __init__(self, fieldnames):
        self.benchmarks = {}
        self.fieldnames = fieldnames

    def add_benchmark(self, benchmark_result):
        self.benchmarks[benchmark_result.name] = benchmark_result
    
    def write_csv(self):
        with open('results_resources.csv', 'w') as f:
            writer = csv.DictWriter(f, fieldnames=self.fieldnames)
            headers = dict( (n,n) for n in self.fieldnames)
            writer.writerow(headers)
            for benchmark_name, benchmark_result in self.benchmarks.iteritems():
                try:
                    benchmark_result.write_csv_row(writer, fieldnames)
                except KeyError:
                    #No results for this benchmark, add a place holder
                    writer.writerow({'Name': benchmark_name})

    def print_summary(self):
        print "#### SUMARY ####"
        print
        for benchmark_result in self.benchmarks.values():
            benchmark_result.print_summary()

class BenchmarkResult(object):
    def __init__(self, name, job):
        self.name = name
        self.job = job
        self.dict = {}

    def get_name(self):
        return self.name

    def write_csv_row(self, writer, fieldnames):
        results = {'Name': self.name}
        results.update(self.dict)
        writer.writerow(results)

    def add_value(self, key, value):
        self.dict[key] = value

    def print_summary(self):
        print self.name
        for key, value in self.dict.iteritems():
            print '\t', key, value



            



def main():

    benchmarks = []
    pwd = os.getcwd()
    for job_dir in os.listdir(pwd):
        if job_dir == 'results.csv':
            continue

        output_dir = os.path.join(job_dir, 'output')

        for file_name in os.listdir(output_dir):
            if file_name == 'top.log':
                pass
            else:
                benchmarks.append((os.path.join(output_dir, file_name), file_name))

    all_results = BenchmarkResults(fieldnames)

    for job_dir, benchmark in benchmarks:
        print "Parsing " + benchmark + '...'
        print "\t", job_dir

        
        benchmark_result = BenchmarkResult(benchmark, job_dir)

        gather_resource_stats(benchmark_result, job_dir)

        all_results.add_benchmark(benchmark_result)

    all_results.print_summary()
    all_results.write_csv()

        

def gather_resource_stats(benchmark_result, job_dir):

    map_rpt_dict = parse_map_rpt(job_dir)
    merge_rpt_dict = parse_merge_rpt(job_dir)
    merge_summary_dict = parse_merge_summary(job_dir)
    synth_log_dict = parse_synth_log(job_dir, benchmark_result.get_name())

    merged_dict = dict(map_rpt_dict.items() + merge_rpt_dict.items() + synth_log_dict.items() + merge_summary_dict.items())

    for key, value in merged_dict.iteritems():
        benchmark_result.add_value(key, value)

def parse_map_rpt(job_dir):
    total_blocks_regex = re.compile(r"^.*Implemented (?P<total_blocks>\d+) device resources after synthesis.*$")
    LEs_regex = re.compile(r"^.*Implemented (?P<LEs>\d+) logic cells.*$")

    q2_out_dir = os.path.join(job_dir, "q2_out/")
    try:
        map_rpt_files = fnmatch.filter(os.listdir(q2_out_dir), "*map.rpt")
        assert(len(map_rpt_files) == 1)
        map_rpt_file = os.path.join(q2_out_dir, map_rpt_files[0])
    except OSError:
        print "Error parsing map report for %s" % q2_out_dir
        return {}

    blackbox_count = 0

    results = {}

    with open(map_rpt_file, 'r') as f:
        for line in f:
            result = total_blocks_regex.match(line)
            if result:
                results['Total Blocks'] = int(result.group('total_blocks'))
                continue


            result = LEs_regex.match(line)
            if result:
                results['# LEs'] = int(result.group('LEs'))
                continue



    return results

def parse_merge_summary(job_dir):
    comb_aluts_regex = re.compile(r"^\s+Combinational ALUTs\s+:\s+(?P<ALUTs>[,\d]+)\s+$")
    mem_aluts_regex = re.compile(r"^\s+Memory ALUTs\s+:\s+(?P<ALUTs>[,\d]+)\s+$")

    results = {}
    aluts = 0

    q2_out_dir = os.path.join(job_dir, "q2_out/")
    try:
        merge_summary_files = fnmatch.filter(os.listdir(q2_out_dir), "*merge.summary")
        assert(len(merge_summary_files) == 1)
        merge_summary_file = os.path.join(q2_out_dir, merge_summary_files[0])
    except OSError:
        print "Error parsing merge report for %s" % q2_out_dir
        return {}

    with open(merge_summary_file, 'r') as f:
        for line in f:
            #ALUTs
            result = comb_aluts_regex.match(line)
            if result:
                aluts += int(result.group('ALUTs').replace(',', ''))
                continue
            result = mem_aluts_regex.match(line)
            if result:
                aluts += int(result.group('ALUTs').replace(',', ''))
                continue

    results['# ALUTs'] = aluts
    return results

def parse_merge_rpt(job_dir):
    input_pins_regex = re.compile(r"^.*Implemented (?P<input_pins>\d+) input pins.*$")
    output_pins_regex = re.compile(r"^.*Implemented (?P<output_pins>\d+) output pins.*$")
    bidir_pins_regex = re.compile(r"^.*-- Bidir Ports\s+; (?P<bidir_pins>\d+)\s+;.*$")
    plls_regex = re.compile(r"^;\s+Total PLLs\s+; (?P<plls>\d+)\s+;.*$")
    regs_regex = re.compile(r"^.*Dedicated logic registers\s+; (?P<REGs>\d+)\s+;.*$")
    dsp_elems_regex = re.compile(r"^.*Implemented (?P<DSPs>\d+) DSP elements.*$")
    ram_segs_regex = re.compile(r"^.*Implemented (?P<ram_segs>\d+) RAM segments.*$")
    ram_bits_regex = re.compile(r"^.*Total block memory bits\s+; (?P<mem_bits>\d+)\s+;.*$")


    q2_out_dir = os.path.join(job_dir, "q2_out/")
    try:
        merge_rpt_files = fnmatch.filter(os.listdir(q2_out_dir), "*merge.rpt")
        assert(len(merge_rpt_files) == 1)
        merge_rpt_file = os.path.join(q2_out_dir, merge_rpt_files[0])
    except OSError:
        print "Error parsing merge report for %s" % q2_out_dir
        return {}

    results = {}


    with open(merge_rpt_file, 'r') as f:
        for line in f:
            #INPUT pins
            result = input_pins_regex.match(line)
            if result:
                results['# Input Pins'] = int(result.group('input_pins'))
                continue

            #OUTPUT pins
            result = output_pins_regex.match(line)
            if result:
                results['# Output Pins'] = int(result.group('output_pins'))
                continue

            #BIDIR pins
            result = bidir_pins_regex.match(line)
            if result:
                results['# Bidir Pins'] = int(result.group('bidir_pins'))
                continue

            #PLLs
            result = plls_regex.match(line)
            if result:
                results['# PLLs'] = int(result.group('plls'))
                continue


            #REGs
            result = regs_regex.match(line)
            if result:
                results['# REGs'] = int(result.group('REGs'))
                continue

            #DSPs
            result = dsp_elems_regex.match(line)
            if result:
                results['# DSP Elements'] = int(result.group('DSPs'))
                continue

            #RAM segs
            result = ram_segs_regex.match(line)
            if result:
                results['# RAM Segments'] = int(result.group('ram_segs'))
                continue

            #RAM bits
            result = ram_bits_regex.match(line)
            if result:
                results['# RAM Bits'] = int(result.group('mem_bits'))
                continue


    if '# DSP Elements' not in results:
        print "Info: found no DSP elements, setting to zero"
        results['# DSP Elements'] = 0

    if '# RAM Segments' not in results:
        print "Info: found no RAM Segments, setting to zero"
        results['# RAM Segments'] = 0

    if '# RAM Bits' not in results:
        print "Info: found no RAM Bits, setting to zero"
        results['# RAM Bits'] = 0

    return results


def parse_synth_log(job_dir, benchmark_name):
    introduced_blackbox_regex = re.compile(r"^\s+>> Introduced (?P<num_blackbox>\d+) instances of blackbox.*$")

    log_file = os.path.join(job_dir, "%s-quartus_synthesis.log" % benchmark_name)

    blackbox_count = 0

    try: 
        with open(log_file, 'r') as f:
            for line in f:
                result = introduced_blackbox_regex.match(line)
                if result:
                    blackbox_count += int(result.group('num_blackbox'))
    except IOError:
        print "Error could not open %s" % log_file
        return {}


    return {'# BLIF blocks': blackbox_count}



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

    results = ActionResult('quartus_synthesis')
    results.add_time(total_time)
    results.add_memory(peak_memory)

    return results

if __name__ == '__main__':
    main()
