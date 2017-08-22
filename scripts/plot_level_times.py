#!/usr/bin/env python
import sys, argparse
import csv

import matplotlib.pyplot as plt
import numpy as np

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv_files", nargs=2, default=None, help="CSV files with level runtimes")
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


    #Calculate derived series
    derived_series = {}

    traversals = ['Fwd', 'Bck']

    N_AVG = 100
    for traversal in ['Fwd', 'Bck']:
        #Per-level speed-up
        first_array = data[args.csv_files[0]]["%s_Time" % traversal]
        second_array = data[args.csv_files[1]]["%s_Time" % traversal]

        derived_series["Level Speed-up %s" % traversal] = []
        for i in xrange(len(first_array)):
            if(second_array[i] == 0.0):
                derived_series["Level Speed-up %s" % traversal].append(None)
            else:
                derived_series["Level Speed-up %s" % traversal].append(first_array[i] / second_array[i])

        #Averaged level speed-up
        derived_series["Avg Level Speed-up %s" % traversal] = []
        for i in xrange(len(first_array)):
            first_avg = sum(first_array[max(0,i-N_AVG):i]) / N_AVG
            second_avg = sum(second_array[max(0,i-N_AVG):i]) / N_AVG
            if(second_avg == 0.0):
                derived_series["Avg Level Speed-up %s" % traversal].append(None)
            else:
                derived_series["Avg Level Speed-up %s" % traversal].append(first_avg / second_avg)
        

        #Cummulative time
        for filename in args.csv_files:
            derived_series["Cumm Time %s %s" % (filename.split('_')[0], traversal)] = []
            val_sum = 0.
            for value in data[filename]["%s_Time" % traversal]:
                val_sum += value
                derived_series["Cumm Time %s %s" % (filename.split('_')[0], traversal)].append(val_sum)

        #Cummulative Speed-up
        derived_series["Cumm Speed-Up %s" % traversal] = []
        first_cumm_array = derived_series["Cumm Time %s %s" % (args.csv_files[0].split('_')[0], traversal)]
        second_cumm_array = derived_series["Cumm Time %s %s" % (args.csv_files[1].split('_')[0], traversal)]
        for i in xrange(len(first_cumm_array)):
            if(second_array[i] == 0.0):
                derived_series["Cumm Speed-Up %s" % traversal].append(None)
            else:
                derived_series["Cumm Speed-Up %s" % traversal].append(first_cumm_array[i] / second_cumm_array[i])



    #Plot results
    fig, ax = plt.subplots(4, sharex=True)


    #Cummulative Times
    for series_name in derived_series.keys():
        if series_name.startswith("Cumm Time"):
            ax[0].plot(data[args.csv_files[0]]["Level"],  derived_series[series_name], label=series_name[len("Cumm Time"):])
    ax[0].legend(loc='best')
    ax[0].set_ylabel("Cummulative\nTime (sec)")
    ax[0].set_title("Per Level Performance Characteristics")
    ax[0].set_ylim(bottom=0)

    #Per-level Speed-up
    for traversal in traversals:
        series_name = "Avg Level Speed-up %s" % traversal

        ax[1].plot(data[args.csv_files[0]]["Level"], derived_series[series_name], label=traversal)
    ax[1].legend(loc='best')
    #ax[1].set_yscale('log')
    ax[1].set_ylabel("Level Speed-Up\n(N_AVG %d)" % N_AVG)
    ax[1].set_ylim(bottom=0)

    for traversal in traversals:
        series_name = "Cumm Speed-Up %s" % traversal

        ax[2].plot(data[args.csv_files[0]]["Level"], derived_series[series_name], label=traversal)
    ax[2].legend(loc='best')
    ax[2].set_ylabel("Cummulative\nSpeed-Up")
    ax[2].set_ylim(bottom=0)

    ax[3].plot(data[args.csv_files[0]]["Level"], data[args.csv_files[0]]["Width"], label="width")

    if args.min_fwd_width:
        ax[3].plot(data[args.csv_files[0]]["Level"], [args.min_fwd_width for x in xrange(len(data[args.csv_files[0]]["Level"]))], label="Min // Fwd")
    if args.min_fwd_width:
        ax[3].plot(data[args.csv_files[0]]["Level"], [args.min_bck_width for x in xrange(len(data[args.csv_files[0]]["Level"]))], label="Min // Bck")

    ax[3].set_yscale('log')
    ax[3].set_xlabel("Level")
    ax[3].set_ylabel("Level Width")
    #ax[3].set_xlim(right=len(data[args.csv_files[0]]["Level"]))
    ax[3].legend(loc='best')

    #ax[0].set_xscale('log')
    #ax[1].set_xscale('log')
    #ax[2].set_xscale('log')
    #ax[3].set_xscale('log')
    plt.tight_layout()
    if args.f:
        plt.savefig(args.f, dpi=300)
    else:
        plt.show()


if __name__ == "__main__":
    main()
