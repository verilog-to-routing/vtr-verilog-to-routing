#!/usr/bin/env python3

import sys
import re
import os
import argparse

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--output_dir", default=".", help="Directory to place arch xml files")
args = parser.parse_args()

openlist = []

side = ['lt','lr','lb','tr','tb','rb']
W = 100
filename = "/home/vm/VTR-Tools/workspace/Basic_Architecture/switchbox_out.xml"

#<func type="lr" formula="t"/>

# def xcopy(fsrc):
#     src = open(fsrc, "r")
#     for line in src:
#         xprint(line)
#     src.close()
#     f.write("\n")

f = open(filename, "w")

for x in range(len(side)):
    for y in range(W):
        f.write('<func type="')
        f.write("%s" % side[x])
        f.write('" formula="t+')
        f.write("%i" % y)
        f.write('"/>\n')
