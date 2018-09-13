#!/usr/bin/env python
import argparse
import math
import sys
import os
import time
import re

TIME_EST_REGEX = re.compile(r"VTR_RUNTIME_ESTIMATE_SECONDS=(?P<time_sec>.*)")
MEM_EST_REGEX = re.compile(r"VTR_MEMORY_ESTIMATE_BYTES=(?P<mem_bytes>.*)")

class Job:
    def __init__(self, script, time_sec=None, memory_mb=None, num_cores=1):
        self.script = script
        self.time_sec = time_sec
        self.memory_mb = memory_mb
        self.num_cores = num_cores

def parse_args():

    parser = argparse.ArgumentParser("Helper script to submit VTR task jobs to a SLURM scheduler")

    parser.add_argument("infile",
                        nargs="?",
                        type=argparse.FileType('r'),
                        default=sys.stdin,
                        help="List of input commands (default: stdin)")

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
                        default=60,
                        help="Minimum time in seconds (Default %(default)s)")

    return parser.parse_args()

def main():
    args = parse_args()

    scripts = [line.rstrip() for line in args.infile]

    jobs = []
    for script in scripts:

        time_sec, mem_mb = get_resource_estimates(script)

        time_sec *= args.time_margin
        time_sec = max(time_sec, args.min_time)

        mem_mb *= args.memory_margin

        jobs.append(Job(script, time_sec=time_sec, memory_mb=mem_mb))

    #TODO: could batch jobs here

    for job in jobs:
        submit_sbatch(job.script, 
                      time_sec=job.time_sec,
                      memory_mb=job.memory_mb,
                      num_cores=job.num_cores,
                      dry_run=args.dry_run)

        if not args.dry_run:
            time.sleep(args.sleep)

def submit_sbatch(cmd, time_sec=None, memory_mb=None, dry_run=None, num_cores=1):

    sbatch_cmd = ['sbatch']

    if num_cores:
        sbatch_cmd += ['--cpus-per-task={}'.format(num_cores)]

    sbatch_cmd += ['--ntasks=1']

    if time_sec:
        sbatch_cmd += ['--time={}'.format(int(math.ceil(time_sec)))]

    if memory_mb:
        sbatch_cmd += ['--mem={}M'.format(int(math.ceil(memory_mb)))]

    if not isinstance(cmd, list):
        cmd = [cmd]

    sbatch_cmd += cmd

    #Print-it
    print " ".join(sbatch_cmd)

    if not dry_run:
        #Do-it
        os.system(sbatch_cmd)

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

    mem_mb = mem_bytes / (1024**2)

    return time_sec, mem_mb



if __name__ == "__main__":
    main()
