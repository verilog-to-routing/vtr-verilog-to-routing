#!/usr/bin/env python3
import argparse
import re
from collections import OrderedDict

import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec

import numpy as np

from sklearn.metrics import mean_absolute_error, mean_squared_error

class TimingPath:
    def __init__(self, startpoint, endpoint):
        self.startpoint = startpoint
        self.endpoint = endpoint

def parse_args():

    parser = argparse.ArgumentParser()

    parser.add_argument("first_report");
    parser.add_argument("second_report");

    parser.add_argument("--type",
                        choices=["path", "endpoint"],
                        default="endpoint")

    return parser.parse_args()

def main():

    args = parse_args()

    print("Parsing {}".format(args.first_report))
    first_paths = parse_timing_report(args, args.first_report)

    print("Parsing {}".format(args.second_report))
    second_paths = parse_timing_report(args, args.second_report)

    plot_correlation(first_paths, second_paths, args.first_report, args.second_report)

def parse_timing_report(args, filename):

    regex = re.compile(r".*?Endpoint  : (?P<end>\S+).*?", re.DOTALL)

    start_regex = re.compile(r"Startpoint: (?P<start>\S+)")
    end_regex = re.compile(r"Endpoint\s*: (?P<end>\S+)")
    slack_regex = re.compile(r"slack \((VIOLATED|MET)\)\s+?(?P<slack>\S+)")
    position_regex = re.compile(r"at \((?P<x>\d+),(?P<y>\d+)\)\)")

    lines = None
    with open(filename) as f:
        lines = f.readlines()
        lines = ''.join(lines)

    paths_lines = lines.split("#Path")


    paths = OrderedDict()
    for path_lines in paths_lines:

        distance = None
        startpoint = None
        endpoint = None
        slack = None

        prev_x = None
        prev_y = None

        match = start_regex.search(path_lines)
        if match:
            startpoint = match.groupdict()['start']

        match = end_regex.search(path_lines)
        if match:
            endpoint = match.groupdict()['end']

        match = slack_regex.search(path_lines)
        if match:
            slack = float(match.groupdict()['slack'])

        for match in position_regex.finditer(path_lines):
            x = int(match.groupdict()['x'])
            y = int(match.groupdict()['y'])
            if prev_x == None and prev_y == None:
                distance = 0
            else:
                dx = abs(x - prev_x)
                dy = abs(y - prev_y)

                distance += dx + dy

            prev_x = x
            prev_y = y

        if endpoint == None:
            continue
        

        if args.type == "endpoint":
            start_end = (None, endpoint)
        else:
            start_end = (startpoint, endpoint)

        
        if start_end not in paths:
            paths[start_end] = slack, distance
        else:
            paths[start_end] = min(paths[start_end][0], slack), distance #Keep least slack


    return paths


def correlate_paths(first_paths, second_paths):

    first_keys = set(first_paths.keys())
    second_keys = set(second_paths.keys())

    common_keys = first_keys & second_keys
    first_unique_keys = first_keys.difference(common_keys)
    second_unique_keys = second_keys.difference(common_keys)

    correlated_paths = []
    first_only = []
    second_only = []

    for path in common_keys:
        correlated_paths.append( (path, first_paths[path], second_paths[path]) )

    for path in first_unique_keys:
        first_only.append( (path, first_paths[path]) )

    for path in second_unique_keys:
        second_only.append( (path, second_paths[path]) )

    return correlated_paths, first_only, second_only

def plot_correlation(first_paths, second_paths, first_name, second_name):

    correlated_paths, first_only, second_only = correlate_paths(first_paths, second_paths)

    print("Correlated {} paths".format(len(correlated_paths)))

    print("First only {} paths".format(len(first_only)))
    print("Second only {} paths".format(len(second_only)))


    first_slacks = [x[0] for x in first_paths.values()]
    second_slacks = [x[0] for x in second_paths.values()]

    first_only_slacks = [x[1][0] for x in first_only]
    second_only_slacks = [x[1][0] for x in second_only]

    min_value = min(min(first_slacks), min(second_slacks))
    max_value = max(max(first_slacks), max(second_slacks))


    x = []
    y = []
    dist = []
    for (start, end), (first_slack, first_distance), (second_slack, second_distance) in correlated_paths:
        x.append(first_slack)
        y.append(second_slack)
        dist.append(first_distance) #Use first distance for now...

    if 0:
        #Correlation plot
        plt.subplot (5, 1, 1)
        plt.scatter(x, y)

        equal = np.linspace(*plt.xlim())
        plt.plot(equal, equal)

        plt.xlabel("{} slack".format(first_name))
        plt.ylabel("{} slack".format(second_name))

        plt.xlim(min_value, max_value)
        plt.ylim(min_value, max_value)

        #First histogram
        nbins=30
        plt.subplot (5, 1, 2)
        plt.hist(first_slacks, range=(min_value,max_value), log=True, bins=nbins)
        plt.xlim(min_value, max_value)
        plt.ylim(bottom=0.5)
        plt.xlabel("{} Slack".format(first_name))
        plt.ylabel("Path Count")
        

        #Second histogram
        plt.subplot (5, 1, 3)
        plt.hist(second_slacks, range=(min_value,max_value), log=True, bins=nbins)
        plt.xlim(min_value, max_value)
        plt.ylim(bottom=0.5)
        plt.xlabel("{} Slack".format(second_name))
        plt.ylabel("Path Count")

        #First residuals histogram
        plt.subplot (5, 1, 4)
        plt.hist(first_only_slacks, range=(min_value,max_value), log=True, bins=nbins)
        plt.xlim(min_value, max_value)
        plt.ylim(bottom=0.5)
        plt.xlabel("{} Residuals Slack".format(first_name))
        plt.ylabel("Path Count")
        

        #Second residuals histogram
        plt.subplot (5, 1, 5)
        plt.hist(second_only_slacks, range=(min_value,max_value), log=True, bins=nbins)
        plt.xlim(min_value, max_value)
        plt.ylim(bottom=0.5)
        plt.xlabel("{} Residuals Slack".format(second_name))
        plt.ylabel("Path Count")

    else:
        fig = plt.figure()
        gs = GridSpec(7,4)

        nbins=30


        #Axies
        ax_scatter = fig.add_subplot(gs[1:4,0:3])
        ax_cb = fig.add_axes([0.124, 0.375, 0.573, 0.03])
        ax_hist_x = fig.add_subplot(gs[0,0:3])
        ax_hist_y = fig.add_subplot(gs[1:4,3])
        ax_error = fig.add_subplot(gs[0,3])

        ax_second_hist = fig.add_subplot(gs[6,0:2])
        ax_first_hist = fig.add_subplot(gs[5,0:2], sharex=ax_second_hist)

        ax_second_only_hist = fig.add_subplot(gs[6,2:4], sharey=ax_second_hist)
        ax_first_only_hist = fig.add_subplot(gs[5,2:4], sharex=ax_second_only_hist, sharey=ax_first_hist)

        #errors
        mae = mean_absolute_error(x, y)
        mse = mean_squared_error(x, y)

        ax_error.text(0.1, 0.1, s="MAE: {:.3f}\nMSE: {:.3f}\nWNS: {:.3f}".format(mae, mse, min(first_slacks)))
        ax_error.set_axis_off()
        

        #Scatter
        sc = ax_scatter.scatter(x,y, c=dist, vmin=0, facecolors='none', label="Paths/Endpoints")
        ax_scatter.set_xlim(min_value, max_value)
        ax_scatter.set_ylim(min_value, max_value)

        #Linear
        equal = np.linspace(*ax_scatter.get_xlim())
        ax_scatter.plot(equal, equal, label="Ideal")

        ax_scatter.legend()
        fig.colorbar(sc, cax=ax_cb, orientation='horizontal', label="Path Distance")

        #Marginals
        ax_hist_x.hist(x, log=True, bins=nbins)
        ax_hist_x.set_ylim(bottom=0.5)
        ax_hist_x.set_xlim(min_value, max_value)

        ax_hist_y.hist(y, log=True, bins=nbins, orientation="horizontal")
        ax_hist_y.set_xlim(left=0.5)
        ax_hist_y.set_ylim(min_value, max_value)

        # Turn off tick labels
        plt.setp(ax_hist_x.get_xticklabels(), visible=False)
        plt.setp(ax_hist_y.get_yticklabels(), visible=False)

        #Full histograms
        ax_first_hist.set_title("Full Histograms")
        ax_first_hist.hist(first_slacks, log=True, bins=nbins)
        ax_first_hist.set_xlim(min_value, max_value)
        ax_first_hist.set_ylim(bottom=0.5)
        ax_first_hist.set_ylabel("{}\nCount".format(first_name))
        plt.setp(ax_first_hist.get_xticklabels(), visible=False)

        ax_second_hist.hist(second_slacks, log=True, bins=nbins)
        ax_second_hist.set_xlim(min_value, max_value)
        ax_second_hist.set_ylim(bottom=0.5)
        ax_second_hist.set_ylabel("{}\nCount".format(second_name))
        ax_second_hist.set_xlabel("Slack")

        #Residual histograms
        ax_first_only_hist.set_title("Residual Histograms")
        ax_first_only_hist.hist(first_only_slacks, log=True, bins=nbins)
        ax_first_only_hist.set_xlim(min_value, max_value)
        ax_first_only_hist.set_ylim(bottom=0.5)
        plt.setp(ax_first_only_hist.get_xticklabels(), visible=False)

        ax_second_only_hist.hist(second_only_slacks, log=True, bins=nbins)
        ax_second_only_hist.set_xlim(min_value, max_value)
        ax_second_only_hist.set_ylim(bottom=0.5)
        ax_second_only_hist.set_xlabel("Slack")


        # Set labels on joint
        ax_scatter.set_xlabel('{} slack'.format(first_name))
        ax_scatter.set_ylabel('{} slack'.format(second_name))

        # Set labels on marginals
        ax_hist_y.set_xlabel('Path/Endpoint Count')
        ax_hist_x.set_ylabel('Path/Endpoint Count')

    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
