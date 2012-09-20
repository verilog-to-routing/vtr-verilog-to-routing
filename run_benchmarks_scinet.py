#!/usr/bin/python
__version__ = """$Revison$
                 Last Change: $Author: kmurray $ $Date: 2012-08-30 22:54:17 -0400 (Thu, 30 Aug 2012) $
                 Orig Author: kmurray
                 $Id: run_benchmarks_scinet.py 2664 2012-06-21 22:20:09Z kmurray $"""
try: import pudb
except ImportError: pass

import argparse
import sys
import os
import json
from math import floor, ceil
from collections import OrderedDict

def parse_args():
    description="""\
    Runs the VQM2BLIF flow on the specified Quartus 2 Project to generate a BLIF file,
    and optionally run VPR on that file.

    The quartus project is compiled in its directory, and by default the
    vqm and blif files are generated in the directory where the script is called.
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument('--benchmarks_info', '-b', dest='benchmark_info_file', action='store',
                        required=True,
                        help='Benchmark information file')
    parser.add_argument('--actions', '-a', dest='actions', action='store',
                        default="quartus_synthesis vpr_pack vpr_place vpr_route",
                        help='Benchmark actions [defaults to "%(default)s"]')
    parser.add_argument('--benchmark_source_dir', '-s', dest='benchmark_src_dir', action='store',
                        required=True,
                        help='Directory containing the benchmarks')
    parser.add_argument('--default_memory', dest='default_memory', action='store',
                        default=6000,
                        help='Default memory used in MB [defaults to %(default)s MB], for benchmarks which do not specify it in the info info file')
    parser.add_argument('--default_time', dest='default_time', action='store',
                        default=8000,
                        help='Default time used in sec [defaults to %(default)s sec], for benchmarks which do not specify it in the info info file')
    parser.add_argument('--num_procs', '-n', dest='num_processors', action='store',
                        default=8,
                        help='Default number of processors used per node [defaults to %(default)s]')

    parser.add_argument('--version', action='version', version=__version__,
                        help='Version information')

    #Options effecting quartus operation
    #quartus_options = parser.add_argument_group('quartus options')

    #quartus_options.add_argument('-q', '--quartus_project', dest='quartus_project', action='store',
                            #help='The Quartus 2 project file of the design to be used')

    args = parser.parse_args()

    return process_args(args)

def process_args(args):
    if args.actions:
        args.actions = args.actions.split()
    else:
        args.actions = []
    return args

def main(args):

    #Get memory and CPU time information about the benchmarks
    benchmark_info = BenchmarkInfo(args.benchmark_info_file)

    #Generate the various action for each active benchmark
    # e.g. synthesize, pack, place, route
    benchmark_action_lists = generate_benchmark_actions(args, benchmark_info)

    #Partition the benchmarks into jobs
    partition_benchmarks(benchmark_action_lists, benchmark_info, args)

def parse_benchmark_info(json_file):
    return Benchmark_info(json_file)

def generate_benchmark_actions(args, benchmark_info):
    benchmark_action_dict = {}

    for benchmark_name in benchmark_info.get_names():
        action_list = BenchmarkActionList(benchmark_name)
        for action in args.actions:
            action = BenchmarkAction(benchmark_name, action, benchmark_info, default_memory=args.default_memory, default_time=args.default_time)
            action_list.add_action(action) 
        benchmark_action_dict[benchmark_name] = action_list

    return benchmark_action_dict


def partition_benchmarks(action_dict, benchmark_info, args):
    #Simple benchmark packer, based on estimated memory usage
    MAX_WALLTIME = 48*60*60 #48 hrs
    RESERVED_MEMORY = 3*1024 #3GB

    memory_capacity = {'16GB': 16*1024 - RESERVED_MEMORY,
                       '32GB': 32*1024 - RESERVED_MEMORY,
                       '128GB': 128*1024 - RESERVED_MEMORY}


    #Use an ordered dictionary so that we try to pack jobs into smaller machines first
    job_lists = OrderedDict()
    job_lists['16GB'] = []
    job_lists['32GB'] = []
    job_lists['128GB'] = []
    cnt = 0
    for benchmark, action_list in action_dict.iteritems():
        benchmark_memory = action_list.get_peak_memory()
        benchmark_walltime = action_list.get_total_walltime()

        benchmark_added = False
        for machine_type, job_list in job_lists.iteritems():

            if benchmark_memory > memory_capacity[machine_type]:
                #Move to a larger memory machine
                pass

            else:
                #Fits in this machine
                for job in job_list:
                    if job.get_available_memory() > benchmark_memory and job.get_available_walltime() > benchmark_walltime:
                        #It fits so add the benchmark
                        job.add_actionlist(action_list)
                        benchmark_added = True  
                        break

                #Create a new job and add the benchmark
                # if the pre-existing jobs are too full
                if not benchmark_added:
                    #Increment Job Counter
                    cnt += 1
                    new_job = ClusterJob(total_memory=memory_capacity[machine_type], total_walltime=MAX_WALLTIME, machine_type=machine_type, job_num=cnt)
                    new_job.add_actionlist(action_list)
                    job_list.append(new_job)
                    benchmark_added = True

                assert(benchmark_added)

                #Benchmark has been added, so stop looking at other machine types
                break

        if not benchmark_added:
            #Could not fit in any jobs
            print "Error: benchmakr {benchmark_name} {memory_usage}MB, {walltime} sec, could not fit in any machine types ({machine_types})".format (\
                    benchmark_name=benchmakr, memory_usage=benchmark_memory, walltime=benchmark_walltime, machine_types=job_lists.keys())

    job_files = []
    for machine_type, job_list in job_lists.iteritems():
        for job in job_list:
            print
            job.explain()
            file_name = job.generate_scripts(args)

            job_files.append(file_name)

    with open('go.sh', 'w') as f:
        print >>f, '#!/bin/bash --norc'
        print >>f, ''
        print >>f, '#Submit these jobs to the GPC'
        for job_script in job_files:
            print >>f, 'qsub {job_script}'.format(job_script=job_script)



            #print benchmark, action.get_name()
            #print "\tMemory: %s MB Wall Time: %s sec" % (action.get_memory_MB(), action.get_walltime_sec())



class ClusterJob(object):

    def __init__(self, total_memory, total_walltime, machine_type, job_num, walltime_margin=1.2, memory_margin=1.0):
        self.job_num = job_num
        self.machine_type = machine_type
        self.name = "job_%03d_%s" % (self.job_num, self.machine_type)

        self.total_memory = total_memory
        self.used_memory = 0
        self.memory_margin = memory_margin

        self.total_walltime = total_walltime
        self.used_walltime = 0
        self.walltime_margin = walltime_margin

        self.action_lists = []

    def get_available_memory(self):
        return (self.total_memory - self.used_memory)

    def get_used_memory(self):
        return self.used_memory

    def get_available_walltime(self):
        return (self.total_walltime - self.used_walltime)

    def get_used_walltime(self):
        return self.used_walltime

    def add_actionlist(self, action_list):
        action_list_mem = action_list.get_peak_memory()
        action_list_time = action_list.get_total_walltime()

        if action_list_mem > self.get_available_memory():
            print "Error: too much memory required to add", action_list.get_name()
            sys.exit(1)
        else:
            self.used_memory += action_list_mem

        if action_list_time > self.get_available_walltime():
            print "Error: too much walltime required to add", action_list.get_name()
            sys.exit(1)
        else:
            self.used_walltime = max(self.used_walltime, action_list_time)

        self.action_lists.append(action_list)

    def explain(self):
        print "JobName: %s" % (self.name)
        print "\tPeak Memory: %sMB (%.1f%% of %sMB)" % (self.get_used_memory(), (float(self.used_memory)/self.total_memory*100), self.total_memory)
        print "\tTotal time : %s (%.1f%% of %s hrs)" % (self.format_walltime(), (float(self.used_walltime)/self.total_walltime*100), (self.total_walltime/60/60))
        for cnt, action in enumerate(self.action_lists):
            print "\tAction %s: %s" % (cnt, action.get_name())
            print "\t\tMemory: %sMB" % action.get_peak_memory()

    def format_walltime(self):
        #Constants
        seconds_per_minute = 60
        minutes_per_hour = 60
        seconds_per_hour = seconds_per_minute * minutes_per_hour

        #Use a margin on walltime
        walltime = int(floor(self.walltime_margin * self.get_used_walltime()))

        walltime_hrs = 0
        walltime_min = 0
        #Hours
        if walltime > seconds_per_hour:
            walltime_hrs = int(floor(float(walltime)/seconds_per_hour))

        #Minutes
        if walltime < 15*seconds_per_minute:
            #Minimum runtime 15 mins
            walltime_min = 15
        else:
            walltime_min = int(ceil(float((walltime - walltime_hrs*seconds_per_hour))/seconds_per_minute))
        

        #Formating
        time_string = ""

        if walltime_hrs == 0:
            time_string += "00:"
        elif walltime_hrs < 10:
            time_string += "0%s:" % walltime_hrs
        else:
            time_string += "%s:" % walltime_hrs

        if walltime_min == 0:
            time_string += "00:"
        elif walltime_min < 10:
            time_string += "0%s:" % walltime_min
        else:
            time_string += "%s:" % walltime_min

        time_string += "00"

        return time_string

    def generate_scripts(self, args):
        file_name = self.name + '.sh'
        with open(file_name, 'w') as f:
            print >>f, "#!/bin/bash --norc"
            print >>f, "#MOAB/Torque submission script for SciNet GPC"

            resource_request = "#PBS -l nodes=1:"
            job_queue = '#PBS -q '

            #Machine type specific configurations
            if self.machine_type == '16GB':
                resource_request += ''
                job_queue += 'batch'

            elif self.machine_type == '32GB':
                #Special resource for 32GB machines
                resource_request += 'm32g:'
                job_queue += 'batch'

            elif self.machine_type == '128GB':
                #Special queue for 128GB machines
                job_queue += 'largemem'
            else:
                print "Error: unrecognized machine type %s" % self.machine_type

            resource_request += "ppn=8,walltime=%s" % self.format_walltime()


            print >>f, resource_request
            print >>f, job_queue
            print >>f, "#PBS -N %s" % self.name
            print >>f, ""
            print >>f, "#Job parameters:"
            print >>f, "JOBNAME=%s" % self.name
            print >>f, "export BENCHMARK_SRC_DIR=%s" % args.benchmark_src_dir
            print >>f, """
export ARCH_FILE=~/dev/trees/vqm_to_blif/REG_TEST/BENCHMARKS/ARCH/stratixiv_arch.simple.xml
export INPUT_SUBDIR=input    # sub-directory (within input_tar) with input files
export OUTPUT_SUBDIR=output  # sub-directory to contain of output files
export OUTPUT_TAR=output_${JOBNAME}.tar
export RAMDISK=/dev/shm/$USER #Ram disk location

#Move to the submission directory
cd $PBS_O_WORKDIR

#Track how long everything takes.
echo -n "Starting Job at "
date

function log_top {
    echo "Begining to log top in background"
    mkdir -p $RAMDISK/$OUTPUT_SUBDIR
    #Columns 512 ensures the full command line arguments are captured in the log file
    # The sed command removes any extra whitespace, if the full 512 columns aren't used
    COLUMNS=512 top -b -M -c -d 10 -u $USER | sed 's/  *$//' > $RAMDISK/$OUTPUT_SUBDIR/top.log &
}

function save_results {    
    echo -n "Copying from directory $RAMDISK/$OUTPUT_SUBDIR to file $PBS_O_WORKDIR/$OUTPUT_TAR on "
    date
    orig_pwd=`pwd`
    cd $RAMDISK
    tar -cf $OUTPUT_TAR $OUTPUT_SUBDIR/*
    cp $OUTPUT_TAR $PBS_O_WORKDIR
    echo -n "Copying of output complete on "
    date
    cd $orig_pwd
}

function cleanup_ramdisk {
    echo -n "Cleaning up ramdisk directory $RAMDISK on "
    date
    rm -rf $RAMDISK
    echo -n "done at "
    date
}

function trap_term {
    echo    "######################################################################"
    echo -n "# Trapped term (soft kill) signal on "; date;
    echo    "######################################################################"
    save_results
    cleanup_ramdisk
    exit
}

#Tracks memory/cpu usage
log_top

#Trap the termination signal (sent by scheduler 30sec before job is killed)
# Call the trap_term function to save results and cleanup ram disk
trap "trap_term" TERM

#Compiler pre-reqs for python
module load intel gcc python

module load gnu-parallel

module load use.own quartus

#Setup port forwarding for quartus license
~/altera/eecg_quartus_license.sh


#Copy to ramdisk
echo "Stage-in: copying files to ramdisk directory $RAMDISK"
mkdir -p $RAMDISK/$INPUT_SUBDIR
mkdir -p $RAMDISK/$OUTPUT_SUBDIR
Copy input tar files
#cp -rf $PBS_O_WORKDIR/input $RAMDISK
cp $ARCH_FILE $RAMDISK/$INPUT_SUBDIR/.

                  """

            print >>f, ""
            print >>f, """
############################
# Execute Parallel commands
############################
"""
            print >>f, 'echo    "######################################################################"'
            print >>f, 'echo -n "# Started parallel jobs on "; date'
            print >>f, 'echo    "######################################################################"'
            print >>f, ""
            print >>f, "parallel -u -j %d <<EOF" % args.num_processors
            for action_num, action_list in enumerate(self.action_lists, start=1):
                script_name = action_list.gen_script(file_name, self.job_num, action_num)
                print >>f, "./" + script_name

            print >>f, ""
            print >>f, "EOF"

            print >>f, ""
            print >>f, 'echo    "######################################################################"'
            print >>f, 'echo -n "# Ended   parallel jobs on "; date'
            print >>f, 'echo    "######################################################################"'
            print >>f, ""
            print >>f, """

save_results
cleanup_ramdisk
"""        
            
            return file_name




class BenchmarkActionList(object):
    def __init__(self, name):
        self.name = name
        self.action_list = []

    def add_action(self, action):
        self.action_list.append(action)

    def get_actions(self):
        return self.action_list

    def get_name(self):
        return self.name

    def get_peak_memory(self):
        peak_memory = 0
        for action in self.action_list:
            peak_memory = max(peak_memory, action.get_memory_MB())
        return peak_memory

    def get_total_walltime(self):
        total_time = 0
        for action in self.action_list:
            total_time += action.get_walltime_sec()
        return total_time

    def gen_script(self, job_name, job_num, action_num):
        cmd_name = '{job_name}_action_{action_num:03d}_{benchmark_name}.sh'.format(job_name=job_name.split('.')[0], action_num=action_num, benchmark_name=self.get_name())

        with open(cmd_name, 'w') as f:
            print >>f, 'echo "Started Job {job_num} Action {action_num} {benchmark_name} on `date`"'.format(job_num=job_num, action_num=action_num, benchmark_name=self.get_name())
            print >>f, ''

            print >>f, 'JOB_OUTPUT_DIR=$RAMDISK/$OUTPUT_SUBDIR/{benchmark_name}'.format(benchmark_name=self.get_name())
            print >>f, 'JOB_INPUT_DIR=$RAMDISK/$INPUT_SUBDIR/{benchmark_name}'.format(benchmark_name=self.get_name())
            print >>f, ''

            print >>f, 'mkdir -p $JOB_INPUT_DIR'
            print >>f, 'cp -rf $BENCHMARK_SRC_DIR/{benchmark_name}/quartus2_proj $JOB_INPUT_DIR/quartus2_proj'.format(benchmark_name=self.get_name())

            print >>f, ''
            print >>f, 'mkdir -p $JOB_OUTPUT_DIR'
            print >>f, 'cd $JOB_OUTPUT_DIR'
            print >>f, ''

            print >>f, '##########################'
            print >>f, '# Execute Action'
            print >>f, '##########################'
            for action in self.get_actions():
                print >>f, action.get_action_cmd(self.get_name()) 

            print >>f, ''
            print >>f, 'echo "Ended   Job {job_num} Action {action_num} {benchmark_name} on `date`"'.format(job_num=job_num, action_num=action_num, benchmark_name=self.get_name())

        return cmd_name

class BenchmarkAction(object):
    def __init__(self, benchmark_name, action_name, info, default_memory, default_time):
        self.benchmark_name = benchmark_name
        self.action_name = action_name
        self.memory_MB = info.get_info(benchmark_name, action_name, 'memory_MB')
        self.walltime_sec = info.get_info(benchmark_name, action_name, 'walltime_sec')
        self.default_memory = int(default_memory)
        self.default_time = int(default_time)
    
    def get_benchmark_name(self):
        return self.benchmark_name
    def get_name(self):
        return self.action_name
    def get_memory_MB(self):
        if self.memory_MB == 'unkown':
            return self.default_memory 
        elif self.memory_MB != '':
            return int(self.memory_MB)
        else:
            return 0
    def get_walltime_sec(self):
        if self.walltime_sec == 'unkown':
            return self.default_time
        elif self.walltime_sec != '':
            return int(self.walltime_sec)
        else:
            return 0

    def get_action_cmd(self, short_name):
        cmd = ''

        qpf_file = '$JOB_INPUT_DIR/quartus2_proj/*.qpf'

        arch_file = '$RAMDISK/$INPUT_SUBDIR/stratixiv_arch.simple.xml'
        blif_file = '$RAMDISK/$OUTPUT_SUBDIR/{short_name}/*_stratixiv.blif'.format(short_name=short_name)

        vpr_opts = {'pack' : ['--pack' , '--timing_analysis off', '--nodisp'], 
                    'place': ['--place', '--fast', '--timing_analysis off', '--nodisp'], 
                    'route': ['--route', '--route_chan_width 300', '--timing_analysis off', '--nodisp']}

        if self.get_name() == "quartus_synthesis":
            cmd = 'vqm2blif_flow.py -q {qpf_file} -a {arch_file} &> {short_name}-{action_name}.log'.format(qpf_file=qpf_file, \
                  arch_file=arch_file, short_name=short_name, action_name=self.get_name())

        elif self.get_name() == "quartus_fit":
            raise NotImplementedError

        elif self.get_name() == "vpr_pack":
            cmd = 'vpr {arch_file} {blif_file} {vpr_opts} &> {short_name}-{action_name}.log'.format(arch_file=arch_file, \
                  blif_file=blif_file, vpr_opts=' '.join(vpr_opts['pack']), short_name=short_name, action_name=self.get_name())

        elif self.get_name() == "vpr_place":
            cmd = 'vpr {arch_file} {blif_file} {vpr_opts} &> {short_name}-{action_name}.log'.format(arch_file=arch_file, \
                  blif_file=blif_file, vpr_opts=' '.join(vpr_opts['place']), short_name=short_name, action_name=self.get_name())

        elif self.get_name() == "vpr_route":
            cmd = 'vpr {arch_file} {blif_file} {vpr_opts} &> {short_name}-{action_name}.log'.format(arch_file=arch_file, \
                  blif_file=blif_file, vpr_opts=' '.join(vpr_opts['route']), short_name=short_name, action_name=self.get_name())

        else:
            print "ERROR: Unknown action %s" % self.get_name()
            sys.exit(1)

        return cmd


class BenchmarkInfo(object):
    def __init__(self, json_info_file):
        self.info = None
        loaded_info = None
        with open(json_info_file) as f:
            loaded_info = json.load(f)  

        if 'benchmarks' in loaded_info:
            self.info = loaded_info['benchmarks']
        else:
            print "Error: invalid benchmarks info, missing 'benchmarks' section"
            sys.exit(1)

    def get_info(self, benchmark, action, mem_or_cpu):
        data = ''

        tool, action_name = action.split('_')

        try: 
            data = self.info[benchmark][tool][action_name][mem_or_cpu]
        except KeyError:
            print "Error: %s %s %s %s not found" % (benchmark, tool, action, mem_or_cpu)
            sys.exit(1)

        return data

    def get_names(self):
        return self.info.keys()


if __name__ == '__main__':
    args = parse_args()

    main(args)


