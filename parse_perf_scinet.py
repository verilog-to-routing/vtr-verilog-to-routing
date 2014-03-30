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

actions = {'VPR': ['quartus_synthesis', 'vpr_pack', 'vpr_place', 'vpr_route300', 'vpr_route500'],
           'Q2':  ['quartus_synthesis', 'quartus_fit'] }

fieldnames = {}
fieldnames['VPR'] = ('Name', 'Total Blocks', 'Status', 'Comment', 'Info', 
                     'Q2Synth Success',      'Q2Synth Time (s)',      'Q2Synth Memory (MB)', 
                     'VPR Pack Success',     'VPR Pack Time (s)',     'VPR Pack Memory (MB)', 
                     'VPR Place Success',    'VPR Place Time (s)',    'VPR Place Memory (MB)', 
                     'VPR Route300 Success', 'VPR Route300 Time (s)', 'VPR Route300 Memory (MB)', 
                     'VPR Route500 Success', 'VPR Route500 Time (s)', 'VPR Route500 Memory (MB)',
                     'VPR Misc Time (s)',    'Total Time [VPR] (s)',  'Peak Memory [VPR] (MB)')
fieldnames['Q2'] = ('Name', 'Total Blocks', 'Status', 'Comment', 'Info', 
                    'Q2Synth Success',     'Q2Synth Time (s)',     'Q2Synth Memory (MB)', 
                    'Q2 Pack Success',     'Q2 Pack Time (s)',     'Q2 Pack Memory (MB)', 
                    'Q2 Place Success',    'Q2 Place Time (s)',    'Q2 Place Memory (MB)', 
                    'Q2 Route300 Success', 'Q2 Route300 Time (s)', 'Q2 Route300 Memory (MB)', 
                    'Q2 Route500 Success', 'Q2 Route500 Time (s)', 'Q2 Route500 Memory (MB)',
                    'Q2 Misc Time (s)',    'Total Time [Q2] (s)',  'Peak Memory [Q2] (MB)')



class BenchmarkResults(object):
    def __init__(self, fieldnames):
        self.benchmarks = {}
        self.fieldnames = fieldnames

    def add_benchmark(self, benchmark_result):
        self.benchmarks[benchmark_result.name] = benchmark_result
    
    def write_csv(self, benchmark_info):
        sorted_benchmark_sizes = benchmark_info.get_names_by_size()
        with open('results_perf.csv', 'w') as f:
            writer = csv.DictWriter(f, fieldnames=self.fieldnames)
            headers = dict( (n,n) for n in self.fieldnames)
            writer.writerow(headers)
            for benchmark_name, benchmark_size in sorted_benchmark_sizes.iteritems():
                try:
                    self.benchmarks[benchmark_name].write_csv_row(writer, fieldnames, benchmark_size)
                except KeyError:
                    #No results for this benchmark, add a place holder
                    pass
                    #writer.writerow({'Name': benchmark_name})

    def print_summary(self):
        print "#### SUMARY ####"
        print
        for benchmark_result in self.benchmarks.values():
            benchmark_result.print_summary()

class BenchmarkResult(object):
    def __init__(self, name, job):
        self.action_results = OrderedDict()
        self.name = name
        self.job = job

    def add_action(self, action_result):
        self.action_results[action_result.name] = action_result

    def write_csv_row(self, writer, fieldnames, size):
        results = {'Name': self.name}
        for action_name, action_result in self.action_results.iteritems():
            results = action_result.add_csv_results(results)
        writer.writerow(results)

    def print_summary(self):
        print "{name}: {job}".format(name=self.name,job=self.job)
        for action_result in self.action_results.values():
            action_result.print_summary('   ')
        print

class ActionResult(object):
    def __init__(self, name):
        self.name = name
        self.time = None
        self.memory = None
        self.other_info = {}
        self.status = 'Failed' #default

    def add_time(self, time):
        self.time = time

    def add_memory(self, memory):
        self.memory = memory

    def add_status(self, status):
        self.status = status

    def add_other(self, key, value):
        self.other_info[key] = value

    def get_other(self, key):
        return self.other_info[key]

    def add_csv_results(self, results_dict):
        if self.name == 'quartus_synthesis':
            results_dict['Q2Synth Success'] = self.status
            if self.status == 'Success':
                results_dict['Q2Synth Time (s)'] = self.time
                results_dict['Q2Synth Memory (MB)'] = self.memory
        
        elif self.name == 'quartus_fit':
            #results_dict['Q2Synth Fit'] = self.status
            #if self.status == 'Success':
            results_dict['Total Time [Q2] (s)'] = self.time
            results_dict['Peak Memory [Q2] (MB)'] = self.memory
            results_dict.update(self.other_info)

        elif self.name == 'vpr_pack':
            results_dict['VPR Pack Success'] = self.status
            if self.status == 'Success':
                results_dict['VPR Pack Time (s)'] = self.time
                results_dict['VPR Pack Memory (MB)'] = self.memory

        elif self.name == 'vpr_place':
            results_dict['VPR Place Success'] = self.status
            if self.status == 'Success':
                results_dict['VPR Place Time (s)'] = self.time
                results_dict['VPR Place Memory (MB)'] = self.memory

        elif self.name == 'vpr_route300':
            results_dict['VPR Route300 Success'] = self.status
            if self.status == 'Success':
                results_dict['VPR Route300 Time (s)'] = self.time
                results_dict['VPR Route300 Memory (MB)'] = self.memory

        elif self.name == 'vpr_route500':
            results_dict['VPR Route500 Success'] = self.status
            if self.status == 'Success':
                results_dict['VPR Route500 Time (s)'] = self.time
                results_dict['VPR Route500 Memory (MB)'] = self.memory

        else:
            print "Error: unrecognized action '%s' while writing csv" % self.name
            sys.exit(1)

        return results_dict

    def print_summary(self, indent):
        if self.status == 'Failed':
            print indent+'{name}: {status}'.format(name=self.name, status=self.status)
        else:
            print indent+'{name}:'.format(name=self.name, status=self.status)
        print 2*indent+'Time (s): {time}'.format(time=self.time)
        print 2*indent+'Mem (MB): {memory}'.format(memory=self.memory)
        print

            



def main(arg):
    tool = ''
    if arg.lower() == 'q2' or arg.lower() == 'quartus':
        tool = 'Q2'
        print "Parsing Q2 results"
    else:
        tool = 'VPR'
        print "Parsing VPR results"

    benchmark_info = BenchmarkInfo('/home/v/vaughn/kmurray/dev/trees/vqm_to_blif/benchmark_info.json')

    benchmarks = []
    pwd = os.getcwd()
    for job_dir in fnmatch.filter(os.listdir(pwd), 'output*'):

        for file_name in os.listdir(os.path.join(job_dir, 'output')):
            if file_name == 'top.log':
                pass
            else:
                benchmarks.append((job_dir, file_name))

    all_resulst = None;
    if tool == 'VPR':
        all_results = BenchmarkResults(fieldnames['VPR'])
    else:
        all_results = BenchmarkResults(fieldnames['Q2'])

    for job_dir, benchmark in benchmarks:
        #print
        #print job_dir
        print "Parsing " + benchmark + '...'

        
        benchmark_result = BenchmarkResult(benchmark, job_dir)

        for action in actions[tool]:
            time = 0
            mem = 0
            action_completed_successfully = False

            log_file = os.path.join(job_dir, "output/%s/%s-%s.log" % (benchmark, benchmark, action))
            if not os.path.isfile(log_file):
                continue

            if action == 'quartus_synthesis':
                result = parse_quartus_synthesis(log_file)
                action_completed_successfully = check_quartus_synthesis_success(job_dir, benchmark, action)
                if action_completed_successfully:
                    result.add_status('Success')

            elif action == 'quartus_fit':
                result = parse_quartus_fit(log_file)
                #action_completed_successfully = check_quartus_fit_success(job_dir, benchmark, action)
                #if action_completed_successfully:
                    #result.add_status('Success')
                
            else:
                result = parse_vpr(job_dir, log_file, benchmark, action)
                action_completed_successfully = check_vpr_success(job_dir, benchmark, action)
                if action_completed_successfully:
                    result.add_status('Success')

            benchmark_result.add_action(result)

        all_results.add_benchmark(benchmark_result)

    all_results.print_summary()
    all_results.write_csv(benchmark_info)




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

def time_to_seconds(hours, minutes, seconds):
    return hours*60*60 + minutes*60 + seconds

def parse_quartus_fit(log_file):
    print "Parsing file ", log_file
    peak_memory = 0
    total_time = 0
    pack_time = 0
    place_time = 0
    route_time = 0
    success = False
    pack_success = False
    place_success = False
    route_success = False
    memory_usage_regex = re.compile(r"^\s*Info: Peak virtual memory:\s+(?P<memory_MB>\S+)\s+megabytes")
    total_time_regex = re.compile(r"^\s*Info: Total CPU time \(on all processors\):\s+(?P<time_hrs>\d+):(?P<time_min>\d+):(?P<time_sec>\d+)")
    total_time_days_regex = re.compile(r"^\s*Info: Total CPU time \(on all processors\):\s+(?P<time_days>\d+):(?P<time_hrs>\d+):(?P<time_min>\d+):(?P<time_sec>\d+)")
    success_regex = re.compile(r"^\s*INFO: Fitting \(quartus_fit\) was successful.")
    pack_time_regex = re.compile(r"^Info.*Fitter placement preparation operations ending: elapsed time is (?P<time_hrs>\d+):(?P<time_min>\d+):(?P<time_sec>\d+)")
    place_time_regex = re.compile(r"^Info.*Fitter placement operations ending: elapsed time is (?P<time_hrs>\d+):(?P<time_min>\d+):(?P<time_sec>\d+)")
    route_time_regex = re.compile(r"^Info.*Fitter routing operations ending: elapsed time is (?P<time_hrs>\d+):(?P<time_min>\d+):(?P<time_sec>\d+)")
    
    with open(log_file, 'r') as f:
        for line in f:
            result = memory_usage_regex.match(line)
            if result:
                peak_memory = max(peak_memory, int(float(result.group('memory_MB'))))
                continue
            result = total_time_regex.match(line)
            if result:
                temp_time_sec = time_to_seconds(int(result.group('time_hrs')), int(result.group('time_min')), int(result.group('time_sec')))
                total_time = max(total_time, int(round(float(temp_time_sec))))

                #Check if there is a day value in this time
                result = total_time_days_regex.match(line)
                if result:
                    temp_time_sec = int(result.group('time_days'))*24*60*60 + time_to_seconds(int(result.group('time_hrs')), int(result.group('time_min')), int(result.group('time_sec')))
                    total_time = max(total_time, int(round(float(temp_time_sec))))
                    continue

            result = pack_time_regex.match(line)
            if result:
                pack_time = time_to_seconds(int(result.group('time_hrs')), int(result.group('time_min')), int(result.group('time_sec')))
                pack_success = True
                print "Found Pack Time"
                continue
            result = place_time_regex.match(line)
            if result:
                place_time += time_to_seconds(int(result.group('time_hrs')), int(result.group('time_min')), int(result.group('time_sec')))
                place_success = True
                print "Found Place Time"
                continue
            result = route_time_regex.match(line)
            if result:
                route_time = time_to_seconds(int(result.group('time_hrs')), int(result.group('time_min')), int(result.group('time_sec')))
                route_success = True
                print "Found Route Time"
                continue
            result = success_regex.match(line)
            if result:
                success = True
                continue

    result = ActionResult('quartus_fit')
    result.add_time(total_time)
    result.add_memory(peak_memory)
    result.add_other('Q2 Pack Time (s)', pack_time)
    result.add_other('Q2 Place Time (s)', place_time)
    result.add_other('Q2 Route300 Time (s)', route_time)

    if pack_success:
        result.add_other('Q2 Pack Success', 'Success')
    else:
        result.add_other('Q2 Pack Success', 'Failed')
    if place_success:
        result.add_other('Q2 Place Success', 'Success')
    else:
        result.add_other('Q2 Place Success', 'Failed')
    if route_success:
        result.add_other('Q2 Route300 Success', 'Success')
    else:
        result.add_other('Q2 Route300 Success', 'Failed')
    if success:
        result.add_status('Success')
    else:
        result.add_status('Failed')

    #return (success, {'quartus_synthesis': {'total_time': total_time, 'peak_memory': peak_memory}})
    return result

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
    
    search_key = action.split('_')[1]
    if action == 'vpr_route300' or action == 'vpr_route500':
        search_key = 'route'

    regex = r".*vpr.*{benchmark_name}.*{search_key}".format(benchmark_name=benchmark, search_key=search_key)
    command_regex = re.compile(regex)

    peak_memory = parse_top_log(job_dir, command_regex)
    
    results = ActionResult(action)
    results.add_time(total_time)
    results.add_memory(peak_memory)

    return results

def parse_top_log(job_dir, command_regex):
    peak_memory = 0 

    top_regex = re.compile(r"\s*(?P<PID>\S+)\s+(?P<USER>\S+)\s+(?P<PR>\S+)\s+(?P<NI>\S+)\s+(?P<VIRT>\S+)\s+(?P<RES>\S+)\s+(?P<SHR>\S+)\s+(?P<S>\S+)\s+(?P<CPU>\S+)\s+(?P<MEM>\S+)\s+(?P<TIME>\S+)\s+(?P<COMMAND>[\s\S]+)")

    try:
        with open(os.path.join(job_dir, 'output/top.log'), 'r') as f:
            for line in f:
                result = top_regex.match(line)
                if result:
                    command = result.group('COMMAND')
                    if command_regex.match(command):
                        peak_memory = max(peak_memory, convert_memory_usage_to_MB(result.group('RES')))
    except IOError:
        peak_memory = 0
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
        #print "{action} Failed (VQM generation)".format(action=action)
        pass
    if not found_blif:
        #print "{action} Failed (BLIF generation)".format(action=action)
        pass

    return found_vqm and found_blif

def check_vpr_success(job_dir, benchmark, action):
    found_result_file = False
    
    file_extension_to_find = ''

    if action == 'vpr_pack':
        file_extension_to_find = '.net'

    elif action == 'vpr_place':
        file_extension_to_find = '.place'

    elif action == 'vpr_route' or action == 'vpr_route300' or action == 'vpr_route500':
        file_extension_to_find = '.route'
    else:
        print "Error: unrecognized action '%s' in check_vpr_success" % action
        sys.exit(1)

    for file in os.listdir(os.path.join(job_dir, 'output/%s' % benchmark)):
        file_name, file_extension = os.path.splitext(file)
        if file_extension == file_extension_to_find:
            found_result_file = True

    if not found_result_file:
        #print "\t{action} Failed".format(benchmark=benchmark, action=action)
        pass

    return found_result_file


if __name__ == '__main__':
    main(sys.argv[1])
