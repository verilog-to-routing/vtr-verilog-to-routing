#!/usr/bin/env python
import sys, argparse
import csv
import os

import matplotlib.pyplot as plt
import numpy as np

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv_file", default=None, help="CSV file with level runtimes")
    parser.add_argument("--scale_size", default=False, action="store_true", help="Scale point size by serial level runtime")
    parser.add_argument("--average", default=False, action="store_true", help="Draw average lines")
    parser.add_argument("-f", default=None, help="Output filename")

    args = parser.parse_args()

    return args

def main():
    args = parse_args()

    data = {}
    with open(args.csv_file) as f:
        csv_reader = csv.DictReader(f)
        for field in csv_reader.fieldnames:
            data[field] = []

        for row in csv_reader:
            for field in csv_reader.fieldnames:
                data[field].append(float(row[field]))

    for series_name, data_values in data.iteritems():
        print "\tSeries: ", series_name
        print "\t# Values: ", len(data_values)


    #Calculate derived series
    derived_series = {}

    speedup_fwd = {}
    speedup_bck = {}
    size_factor = 0
    if args.scale_size:
        size_factor = 2000
    size_min = 10
    serial_total = sum(data['serial_fwd'][:] + data['serial_bck'][:])

    for i in xrange(len(data['serial_fwd'])):
        width = data['Width'][i]
        serial_fwd = data['serial_fwd'][i]
        serial_bck = data['serial_bck'][i]
        parrallel_fwd = data['parallel_fwd'][i]
        parrallel_bck = data['parallel_bck'][i]
        if parrallel_fwd != 0.0:
            speedup = serial_fwd / parrallel_fwd
            serial_frac = serial_fwd / serial_total
            val = (speedup, serial_frac)
            try:
                speedup_fwd[width].append(val)
            except KeyError:
                speedup_fwd[width] = [val]

        if parrallel_bck != 0.0:
            speedup = serial_bck / parrallel_bck
            serial_frac = serial_bck / serial_total
            val = (speedup, serial_frac)
            try:
                speedup_bck[width].append(val)
            except KeyError:
                speedup_bck[width] = [val]

    fwd_x = []
    fwd_y = []
    fwd_s = []
    for width, values in speedup_fwd.iteritems():
        for speedup, serial_frac in values:
            fwd_x.append(width)
            fwd_y.append(speedup)
            fwd_s.append(size_factor*serial_frac + size_min)
    bck_x = []
    bck_y = []
    bck_s = []
    for width, values in speedup_bck.iteritems():
        for speedup, serial_frac in values:
            bck_x.append(width)
            bck_y.append(speedup)
            bck_s.append(size_factor*serial_frac + size_min)

    #Averages
    fwd_x_avg = []
    fwd_y_avg = []
    for width, values in sorted(speedup_fwd.iteritems()):
        speedups = [x[0] for x in values]
        avg = sum(speedups) / len(speedups)
        #print "Width, avg", width, values, speedups
        fwd_x_avg.append(width)
        fwd_y_avg.append(avg)

    bck_x_avg = []
    bck_y_avg = []
    for width, values in sorted(speedup_bck.iteritems()):
        speedups = [x[0] for x in values]
        avg = sum(speedups) / len(speedups)
        #print "Width, avg", width, values, speedups
        bck_x_avg.append(width)
        bck_y_avg.append(avg)

    plt.scatter(fwd_x, fwd_y, fwd_s, c='b', label="speedup_fwd")
    plt.scatter(bck_x, bck_y, bck_s, c='g', label="speedup_bck")

    if args.average:
        plt.plot(fwd_x_avg, fwd_y_avg, c='b', label="Average FWD Speed-Up")
        plt.plot(bck_x_avg, bck_y_avg, c='g', label="Average BCK Speed-Up")

    plt.xscale("log")

    xmin, xmax = plt.xlim()
    ymin, ymax = plt.ylim()
    ymin = 0
    xmin = 1
    plt.ylim(ymin,ymax)
    plt.xlim(xmin,xmax)

    plt.title(os.path.splitext(os.path.basename(args.csv_file))[0])
    plt.xlabel("Level Width")
    plt.ylabel("Parallel Speed-Up")
    plt.legend(loc='upper left')

    if args.f:
        plt.savefig(args.f, dpi=300)
    else:
        plt.show()

def runningMean(x, N):
    return np.convolve(x, np.ones((N,))/N, mode='same')

if __name__ == "__main__":
    main()
