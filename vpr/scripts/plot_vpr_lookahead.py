#!/usr/bin/env python
import argparse
import sys
import os
from matplotlib import pyplot as plt
from matplotlib.mlab import griddata

import pandas as pd
import numpy as np

def parse_args():

    parser = argparse.ArgumentParser()

    parser.add_argument("csv_file")

    parser.add_argument("--value",
                        required=True,)

    parser.add_argument("--min",
                        default=None)

    parser.add_argument("--max",
                        default=None)

    parser.add_argument("-f", "--file",
                        help="Output file name")

    return parser.parse_args()

def main():

    args = parse_args()

    df = pd.read_csv(args.csv_file)

    nx = len(df['dx'].unique())
    ny = len(df['dy'].unique())

    print "nx: {} ny: {}".format(nx, ny)

    x = df['dx'].values
    y = df['dy'].values
    v = df[args.value].values

    X = np.empty((nx+1, ny+1))
    Y = np.empty((nx+1, ny+1))
    V = np.empty((nx, ny))
    X[:] = np.nan
    Y[:] = np.nan
    V[:] = np.nan

    ix = 0
    for dx in df['dx'].unique():
        iy = 0
        for dy in df['dy'].unique():
            X[ix,iy] = dx
            Y[ix,iy] = dy

            entry = df[(df['dx'] == dx) & (df['dy'] == dy)]
            if not entry.empty:
                V[ix,iy] = entry[args.value]
            else:
                V[ix,iy] = np.nan
            iy += 1
        ix += 1

    print "Max Value: {}".format(np.nanmax(V))
    print "Min Value: {}".format(np.nanmin(V))

    # plt.scatter(X,Y,c=C)
    # plt.pcolor(V)
    # plt.pcolor(X, Y, V)
    # plt.imshow(V)
    pcol = plt.pcolor(X, Y, V, linewidth=0, rasterized=True, vmin=args.min, vmax=args.max)
    pcol.set_edgecolor('face')
    # plt.contourf(V)

    plt.colorbar()
    plt.xlabel('$\Delta x$')
    plt.ylabel('$\Delta y$')

    plt.tight_layout()
    if not args.file:
        plt.show()
    else:
        plt.savefig(args.file, dpi=300)


if __name__ == "__main__":
    main()
