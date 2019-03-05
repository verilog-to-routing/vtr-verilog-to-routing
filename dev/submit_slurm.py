#!/usr/bin/env python
import argparse
import math
import sys
import os
import time
import re

TIME_EST_REGEX = re.compile(r"VTR_RUNTIME_ESTIMATE_SECONDS=(?P<time_sec>.*)")
MEM_EST_REGEX = re.compile(r"VTR_MEMORY_ESTIMATE_BYTES=(?P<mem_bytes>.*)")
JOB_INFO_REGEX = re.compile(r"(.*/)?(?P<task>[^/]*)/run(?P<run>\d+)/(?P<arch>[^/]*)/(?P<benchmark>[^/]*)/common/vtr_flow.sh")

class Job:
    def __init__(self, script, time_minutes=None, memory_mb=None, num_cores=1):
        self.script = script
        self.time_minutes = time_minutes
        self.memory_mb = memory_mb
        self.num_cores = num_cores

def parse_args():

    parser = argparse.ArgumentParser("Helper script to submit VTR task jobs to a SLURM scheduler")

    parser.add_argument("infile",
                        nargs="?",
                        type=argparse.FileType('r'),
                        default=sys.stdin,
                        help="List of input commands (default: stdin)")

    parser.add_argument("--constraint",
                        default=None,
                        help="Sets sbatch's --constraint option")

    parser.add_argument("--dry_run", "-n",
                        default=False,
                        action='store_true',
                        help="Print what actions would be taken (but do not actually do anything)")

    parser.add_argument("--time_margin",
                        default=1.5,
                        help="Extra run-time margin factor (Default %(default)s)")

    parser.add_argument("--memory_margin",
                        default=1.25,
                        help="Extra memory margin factor (Default %(default)s)")

    parser.add_argument("--sleep",
                        default=1.,
                        help="How long to sleep between submitting jobs to prevent scheduler overload. (Default %(default)s)")

    parser.add_argument("--min_time",
                        default=1,
                        help="Minimum time in minutes (Default %(default)s)")
    parser.add_argument("--max_time",
                        default=48*60, #48 hours
                        help="Maximum time in minutes (Default %(default)s)")

    parser.add_argument("--job_name_fmt",
                        default="{task}_{circuit}_{run}",
                        help="Format string for job names (Default %(default)s)")

    return parser.parse_args()

def main():
    args = parse_args()

    scripts = [line.rstrip() for line in args.infile]

    jobs = []
    for script in scripts:

        time_minutes, mem_mb = get_resource_estimates(script)

        time_minutes = max(time_minutes, args.min_time)
        time_minutes = min(time_minutes, args.max_time)
        time_minutes *= args.time_margin

        mem_mb *= args.memory_margin

        jobs.append(Job(script, time_minutes=time_minutes, memory_mb=mem_mb))

    #TODO: could batch jobs here


    for job in jobs:

        job_name = None
        match = JOB_INFO_REGEX.match(job.script)
        if match:
            job_name = args.job_name_fmt.format(task=match.groupdict()['task'],
                                            arch=match.groupdict()['arch'],
                                            circuit=match.groupdict()['benchmark'],
                                            run=match.groupdict()['run'])

        submit_sbatch(job.script, 
                      time_minutes=job.time_minutes,
                      memory_mb=job.memory_mb,
                      num_cores=job.num_cores,
                      constraint=args.constraint,
                      job_name=job_name,
                      dry_run=args.dry_run,
                      submit_dir=os.path.dirname(job.script))

        if not args.dry_run:
            time.sleep(args.sleep)

def submit_sbatch(cmd, time_minutes=None, memory_mb=None, dry_run=None, num_cores=1, constraint=None, submit_dir=None, job_name=None):

    cwd = os.getcwd()

    if submit_dir:
        os.chdir(submit_dir)

    sbatch_cmd = ['sbatch']

    if num_cores is not None:
        sbatch_cmd += ['--cpus-per-task={}'.format(num_cores)]

    sbatch_cmd += ['--ntasks=1']

    if constraint is not None:
        sbatch_cmd += ['--constraint={}'.format(constraint)]

    if time_minutes is not None:
        sbatch_cmd += ['--time={}'.format(int(math.ceil(time_minutes)))]

    if memory_mb is not None:
        sbatch_cmd += ['--mem={}M'.format(int(math.ceil(memory_mb)))]

    if job_name is not None:
        sbatch_cmd += ['--job-name={}'.format(job_name)]

    if not isinstance(cmd, list):
        cmd = [cmd]

    sbatch_cmd += cmd

    #Print-it
    print " ".join(sbatch_cmd)

    if not dry_run:
        #Do-it
        os.system(" ".join(sbatch_cmd))


    os.chdir(cwd) #Revert to original directory

def get_resource_estimates(filepath):
    time_sec = None
    mem_bytes = None

    with open(filepath) as f:
        for line in f:

            match = TIME_EST_REGEX.match(line)
            if match:
                time_sec = float(match.groupdict()['time_sec'])

            match = MEM_EST_REGEX.match(line)
            if match:
                mem_bytes = float(match.groupdict()['mem_bytes'])

    time_minutes = time_sec / 60
    mem_mb = mem_bytes / (1024**2)

    return time_minutes, mem_mb



if __name__ == "__main__":
    main()
