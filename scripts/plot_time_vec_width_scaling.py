#!/usr/bin/env python
import sys, argparse
import csv

import matplotlib.pyplot as plt
import numpy as np
from collections import OrderedDict

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv_files", nargs=1, default=None, help="CSV files with level runtimes")
    parser.add_argument("-f", default=None, help="Output filename")
    parser.add_argument("--min_fwd_width", type=int, default=None, help="Minimum forward width for parllel")
    parser.add_argument("--min_bck_width", type=int, default=None, help="Minimum backward width for parllel")

    args = parser.parse_args()

    return args

def main():
    args = parse_args()

    data = {}
    for filename in args.csv_files:
        with open(filename) as f:
            data[filename] = {}
            csv_reader = csv.DictReader(f)
            
            for field in csv_reader.fieldnames:
                data[filename][field] = []

            for row in csv_reader:
                for field in csv_reader.fieldnames:
                    data[filename][field].append(float(row[field]))

    for filename, series in data.iteritems():
        print "File: ", filename
        #for series_name, data_values in series.iteritems():
            #print "\tSeries: ", series_name
            #print "\tValuse: ", data_values


    series = OrderedDict()

    analyzer_colors = OrderedDict()
    analyzer_colors['Serial']= 'r'
    analyzer_colors['Dynamic']= 'b'
    analyzer_colors['Levelized']= 'g'
    analyzer_colors['No Dep']= 'y'

    for analyzer, color in analyzer_colors.iteritems():
        series['%s' % analyzer] = {'label': '%s' % analyzer, 'linestyle': "-", 'color': color}
        series['%s SIMD_AUTO' % analyzer] = {'label': '%s SIMD_AUTO' % analyzer, 'linestyle': "--", 'color': color}
        series['%s SIMD_INTRINSICS' % analyzer] = {'label': '%s SIMD_INTR' % analyzer, 'linestyle': "-.", 'color': color}
    

    #Plot results
    fig, ax = plt.subplots(1)

    for series_name, series_info in series.iteritems():
        ax.plot(data[args.csv_files[0]]["Time Vector Width"], data[args.csv_files[0]][series_name], linestyle=series_info['linestyle'], color=series_info['color'], label=series_info['label'], linewidth=2)
    ax.legend(loc='best', prop={'size': 13})
    ax.set_ylabel("Time (sec)")
    ax.set_xlabel("Time Vector Width")
    ax.set_title("Run-time Scaling With Time Vector Width")
    ax.set_xlim(left=1, right=max(data[args.csv_files[0]]["Time Vector Width"]))

    ##Cummulative Times
    #for series_name in derived_series.keys():
        #if series_name.startswith("Cumm Time"):
            #ax[0].plot(data[args.csv_files[0]]["Level"],  derived_series[series_name], label=series_name[len("Cumm Time"):])
    #ax[0].legend(loc='best')
    #ax[0].set_ylabel("Cummulative\nTime (sec)")
    #ax[0].set_title("Per Level Performance Characteristics")
    #ax[0].set_ylim(bottom=0)

    ##Per-level Speed-up
    #for traversal in traversals:
        #series_name = "Avg Level Speed-up %s" % traversal

        #ax[1].plot(data[args.csv_files[0]]["Level"], derived_series[series_name], label=traversal)
    #ax[1].legend(loc='best')
    ##ax[1].set_yscale('log')
    #ax[1].set_ylabel("Level Speed-Up\n(N_AVG %d)" % N_AVG)
    #ax[1].set_ylim(bottom=0)

    #for traversal in traversals:
        #series_name = "Cumm Speed-Up %s" % traversal

        #ax[2].plot(data[args.csv_files[0]]["Level"], derived_series[series_name], label=traversal)
    #ax[2].legend(loc='best')
    #ax[2].set_ylabel("Cummulative\nSpeed-Up")
    #ax[2].set_ylim(bottom=0)

    #ax[3].plot(data[args.csv_files[0]]["Level"], data[args.csv_files[0]]["Width"], label="width")

    #if args.min_fwd_width:
        #ax[3].plot(data[args.csv_files[0]]["Level"], [args.min_fwd_width for x in xrange(len(data[args.csv_files[0]]["Level"]))], label="Min // Fwd")
    #if args.min_fwd_width:
        #ax[3].plot(data[args.csv_files[0]]["Level"], [args.min_bck_width for x in xrange(len(data[args.csv_files[0]]["Level"]))], label="Min // Bck")

    #ax[3].set_yscale('log')
    #ax[3].set_xlabel("Level")
    #ax[3].set_ylabel("Level Width")
    ##ax[3].set_xlim(right=len(data[args.csv_files[0]]["Level"]))
    #ax[3].legend(loc='best')

    plt.tight_layout()
    if args.f:
        plt.savefig(args.f, dpi=300)
    else:
        plt.show()


if __name__ == "__main__":
    main()
